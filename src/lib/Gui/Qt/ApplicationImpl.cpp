/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2011                                                       |
|  Jörg Bakker (jb'at'open-multimedia.org)                                  |
|                                                                           |
|  This file is part of OMM.                                                |
|                                                                           |
|  OMM is free software: you can redistribute it and/or modify              |
|  it under the terms of the GNU General Public License as published by     |
|  the Free Software Foundation version 3 of the License.                   |
|                                                                           |
|  OMM is distributed in the hope that it will be useful,                   |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of           |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            |
|  GNU General Public License for more details.                             |
|                                                                           |
|  You should have received a copy of the GNU General Public License        |
|  along with this program.  If not, see <http://www.gnu.org/licenses/>.    |
 ***************************************************************************/

#include <Poco/NumberFormatter.h>

#include <QtGui>

#include "ApplicationImpl.h"
#include "QtViewImpl.h"
#include "Gui/Application.h"
#include "Gui/GuiLogger.h"


namespace Omm {
namespace Gui {


ApplicationImpl::ApplicationImpl(Application* pApplication) :
_pApplication(pApplication),
_pMainWindow(0),
_visible(true),
_width(800),
_height(480),
_fullscreen(false)
{
    LOG(gui, debug, "application impl ctor");
    _pFullscreenStyleSheet = new QString(
        "* { \
            font-size: 36pt; \
            color: #5f5f5f; \
        } \
        QTabWidget { \
            border-width: 0; \
        } \
        QScrollArea { \
            background-color: black; \
            color: #afafaf; \
        } \
        QLabel { \
            background-color: black; \
            color: #afafaf; \
        } \
        QSlider { \
            background-color: black; \
            color: #afafaf; \
        } \
        QButton { \
            background-color: black; \
            color: #afafaf; \
        }"
    );
}


ApplicationImpl::~ApplicationImpl()
{
}


void
ApplicationImpl::show(bool show)
{
    if (_pMainWindow) {
        _pMainWindow->setVisible(show);
    }
    else {
        _visible = show;
    }
}


void
ApplicationImpl::resize(int width, int height)
{
    _width = width;
    _height = height;
    if (_pMainWindow) {
        _pMainWindow->resize(width, height);
    }
}


int
ApplicationImpl::width()
{
    if (_pMainWindow) {
        return _pMainWindow->width();
    }
    return _width;
}


int
ApplicationImpl::height()
{
    if (_pMainWindow) {
        return _pMainWindow->height();
    }
    return _height;
}


void
ApplicationImpl::setFullscreen(bool fullscreen)
{
    _fullscreen = fullscreen;
    if (fullscreen) {
        _pQtApplication->setStyleSheet(*_pFullscreenStyleSheet);
        _pMainWindow->setCursor(QCursor(Qt::BlankCursor));
        _pMainWindow->showFullScreen();
    }
    else {
        _pQtApplication->setStyleSheet(*_pDefaultStyleSheet);
        _pMainWindow->setCursor(QCursor(Qt::ArrowCursor));
        _pMainWindow->showNormal();
    }
}


void
ApplicationImpl::addControlPanel(View* pView)
{
    QToolBar* pToolBar = new QToolBar;
    _pToolBars.insert(pToolBar);
    pToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    QWidget* pWidget = static_cast<QWidget*>(pView->getNativeView());
    pToolBar->addWidget(pWidget);
    _pMainWindow->addToolBar(Qt::BottomToolBarArea, pToolBar);
}


void
ApplicationImpl::showControlPanels(bool show)
{
    for (std::set<QToolBar*>::iterator it = _pToolBars.begin(); it != _pToolBars.end(); ++it) {
        (*it)->setVisible(show);
    }
}


int
ApplicationImpl::run(int argc, char** argv)
{
    LOG(gui, debug, "event loop exec ...");
#ifdef __WINDOWS__
//    QApplication::setStyle(new QPlastiqueStyle);        // avoid the standard QWindowsStyle (WinNT)
    QApplication::setStyle(new QCleanlooksStyle);        // avoid the standard QWindowsStyle (WinNT)
//    QApplication::setStyle(new QWindowsStyle);
//    QApplication::setStyle(new QWindowsXPStyle);      // linking error, not contained in Qt libs
//    QApplication::setStyle(new QWindowsVistaStyle);   // linking error, not contained in Qt libs
#endif
    _pQtApplication = new QApplication(argc, argv);
    _pMainWindow = new QMainWindow;
    _pDefaultStyleSheet = new QString(_pQtApplication->styleSheet());

    _pApplication->_pMainView = _pApplication->createMainView();
    _pMainWindow->setCentralWidget(static_cast<QWidget*>(_pApplication->_pMainView->getNativeView()));
    _pApplication->createdMainView();
    if (_visible) {
        _pMainWindow->show();
    }
    _pMainWindow->resize(_width, _height);
    _pApplication->presentedMainView();

//    _pEventFilter = new QtEventFilter(_pApplication->_pMainView->getViewImpl());
    // FIXME: search field needs focus or global keys must be enabled
    // for now key navigation is only enabled in fullscreen mode
//    if (_fullscreen) {
//        _pQtApplication->installEventFilter(_pEventFilter);
//    }
    _pPermanentEventFilter = new QtPermanentEventFilter(_pApplication->_pMainView->getViewImpl());
    _pQtApplication->installEventFilter(_pPermanentEventFilter);

    _pApplication->start();
    int ret = _pQtApplication->exec();
    LOG(gui, debug, "event loop exec finished.");
    _pApplication->finishedEventLoop();
    _pApplication->stop();
    return ret;
}


void
ApplicationImpl::quit()
{
    _pQtApplication->quit();
}


}  // namespace Omm
}  // namespace Gui
