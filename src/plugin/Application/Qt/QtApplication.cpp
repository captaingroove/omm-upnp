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

#include <Poco/ClassLibrary.h>

#include "QtApplication.h"
#include "QtMainWindow.h"
#include "QtVisual.h"
#include "QtEventFilter.h"
#include "QtController.h"

Q_DECLARE_METATYPE(std::string);


QtApplication::QtApplication()
{
}


QtApplication::~QtApplication()
{
}


void
QtApplication::addToolBar(QToolBar* pToolBar)
{
    _pMainWindow->addToolBar(Qt::BottomToolBarArea, pToolBar);
}


void
QtApplication::initApplication(int argc, char** argv)
{
    Omm::Log::instance()->upnp().debug("init qt application ...");

//    setupUnixSignalHandlers();
    qRegisterMetaType<std::string>();

    _fullscreenStyleSheet =
            "*         { font-size: 32pt } \
             QTreeView { background-color: black;\
                         selection-color: white; \
                         selection-background-color: darkblue; } \
             QTreeView { color: white }";

    // TODO: command line arguments can be passed after Poco::Application has processed them.
    _pApp = new QApplication(_argc, 0);
//    _pEventFilter = new QtEventFilter(this);
//    _pApp->installEventFilter(_pEventFilter);
//    connect(this, SIGNAL(doQuit()), _pApp, SLOT(quit()));
//    installSignalHandler();

    _pMainWidget = new QStackedWidget;
    _pMainWindow = new QtMainWindow(_pMainWidget);

    
//    _pVisual = new QtVisual(_pMainWidget);
//
//    _pBrowserWidget = new QtBrowserWidget(_pMainWindow, this);
//
//    _pPlayerRack = new QtPlayerRack(this);
//
//    _pMainWidget->addWidget(_pVisual);
//    _pMainWidget->addWidget(_pBrowserWidget);
//    _pMainWidget->setCurrentWidget(_pBrowserWidget);
//    _menuVisible = true;
//
//    _pMainWindow->addDockWidget(Qt::RightDockWidgetArea, _pPlayerRack);
//    _pMainWindow->setWindowTitle("OMM");
//
//    _pControlPanel = new QtControlPanel(this, _pMainWindow);
//    _pMainWindow->addToolBar(Qt::BottomToolBarArea, _pControlPanel);
//
//    connect(_pVisual, SIGNAL(showMenu(bool)), this, SLOT(showMenu(bool)));
//    connect(this, SIGNAL(volSliderMoved(int)), _pControlPanel, SIGNAL(volSliderMoved(int)));

    _pMainWindow->show();
    
    Omm::Log::instance()->upnp().debug("finished init qt application.");
}


Omm::Controller*
QtApplication::createController()
{
    return new QtController(this);
}


void
QtApplication::addController()
{
    _pMainWidget->addWidget(static_cast<QtController*>(_pController));
}


void
QtApplication::removeController()
{
    _pMainWidget->removeWidget(static_cast<QtController*>(_pController));
}


void
QtApplication::setWindowTitle(const std::string& title)
{
    _pMainWindow->setWindowTitle(QString::fromStdString(title));
}


void
QtApplication::eventLoop()
{
    _pApp->exec();
}


void
QtApplication::quit()
{
    emit doQuit();
}

//void
//QtApplication::showMainWindow()
//{
//    _pMainWindow->show();
//}



//QtAvInterface::QtAvInterface() :
//_argc(0),
//_fullscreen(false),
//_playerRackVisible(false)
//{
//}
//
//
//QtAvInterface::~QtAvInterface()
//{
//    delete _pApp;
//    delete _pEventFilter;
//    delete _pVisual;
//    delete _pMainWindow;
//    delete _pMainWidget;
//    delete _pBrowserWidget;
//    delete _pControlPanel;
//}
//
//
//void
//QtAvInterface::initGui()
//{
////    Omm::Av::Log::instance()->upnpav().debug("init qt gui ...");
//
//    setupUnixSignalHandlers();
//    qRegisterMetaType<std::string>();
//
////    _defaultStyleSheet = _pApp->styleSheet();
////    Omm::Av::Log::instance()->upnpav().debug("default style sheet: " + _defaultStyleSheet.toStdString());
////    _defaultStyleSheet +=
////            "* {font-size: 12px}";
////    _defaultStyleSheet +=
////            "* {font-size: 9pt}";
//    _fullscreenStyleSheet =
//            "*         { font-size: 32pt } \
//             QTreeView { background-color: black;\
//                         selection-color: white; \
//                         selection-background-color: darkblue; } \
//             QTreeView { color: white }";
//
//    _pApp = new QApplication(_argc, 0);
//    _pEventFilter = new QtEventFilter(this);
//    _pApp->installEventFilter(_pEventFilter);
////    _pApp->setStyleSheet(_defaultStyleSheet);
//
//    _pMainWidget = new QStackedWidget;
//    _pMainWindow = new QtMainWindow(_pMainWidget);
//
//    _pVisual = new QtVisual(_pMainWidget);
//
//    _pBrowserWidget = new QtBrowserWidget(_pMainWindow, this);
//
//    _pPlayerRack = new QtPlayerRack(this);
//
//    _pMainWidget->addWidget(_pVisual);
//    _pMainWidget->addWidget(_pBrowserWidget);
//    _pMainWidget->setCurrentWidget(_pBrowserWidget);
//    _menuVisible = true;
//
//    _pMainWindow->addDockWidget(Qt::RightDockWidgetArea, _pPlayerRack);
//    _pMainWindow->setWindowTitle("OMM");
//
//    _pControlPanel = new QtControlPanel(this, _pMainWindow);
//    _pMainWindow->addToolBar(Qt::BottomToolBarArea, _pControlPanel);
//
//    connect(_pVisual, SIGNAL(showMenu(bool)), this, SLOT(showMenu(bool)));
//    connect(this, SIGNAL(volSliderMoved(int)), _pControlPanel, SIGNAL(volSliderMoved(int)));
//
//// TODO: starting of local servers should go in controller application.
//
////    Omm::Util::PluginLoader<Omm::Av::AbstractMediaObject> pluginLoader;
////    Omm::Av::AbstractMediaObject* pContainerPlugin;
////    std::string containerPlugin("server-webradio");
////    std::string home = Omm::Util::Home::getHomePath();
////    std::string name("Web Radio");
////    try {
////        pContainerPlugin = pluginLoader.load(containerPlugin);
////    }
////    catch(Poco::NotFoundException) {
////        std::cerr << "Error could not find server plugin: " << containerPlugin << std::endl;
////        return;
////    }
////    std::clog << "container plugin: " << containerPlugin << " loaded successfully" << std::endl;
////    pContainerPlugin->setOption("basePath", home + "/.omm/webradio.conf");
////    pContainerPlugin->setTitle(name);
////
////    Omm::Av::AvServer* pServer = new Omm::Av::AvServer;
////    pServer->setRoot(pContainerPlugin);
////    pServer->setFriendlyName(name);
////    Omm::Icon* pIcon = new Omm::Icon(32, 32, 8, "image/png", "device.png");
////    pServer->addIcon(pIcon);
////
////    addLocalServer(pServer);
////    startLocalServers();
//
////    Omm::Av::Log::instance()->upnpav().debug("finished init qt gui.");
//}
//
//
//int
//QtAvInterface::eventLoop()
//{
//    return _pApp->exec();
//}
//
//
//void
//QtAvInterface::showMainWindow()
//{
//    _pMainWindow->show();
//}
//
//
//Omm::Sys::Visual*
//QtAvInterface::getVisual()
//{
//    return _pVisual;
//}
//
//
//void
//QtAvInterface::beginNetworkActivity()
//{
////    Omm::Av::Log::instance()->upnpav().debug("begin network activity");
//    emit startNetworkActivity();
//}
//
//
//void
//QtAvInterface::endNetworkActivity()
//{
////    Omm::Av::Log::instance()->upnpav().debug("end network activity");
//    emit stopNetworkActivity();
//}
//
//
//bool
//QtAvInterface::menuVisible()
//{
//    return _menuVisible;
//}
//
//
//bool
//QtAvInterface::isFullscreen()
//{
//    return _fullscreen;
//}
//
//
//void
//QtAvInterface::showMenu(bool show)
//{
//    _menuVisible = show;
//    if (show) {
//        _pMainWidget->setCurrentWidget(_pBrowserWidget);
//    }
//    else {
//        _pMainWidget->setCurrentWidget(_pVisual);
//    }
//}
//
//
//void
//QtAvInterface::setFullscreen(bool fullscreen)
//{
//    _fullscreen = fullscreen;
//    if (fullscreen) {
//        _pControlPanel->hide();
//        showPlayerRack(false);
//        _pMainWindow->setCursor(QCursor(Qt::BlankCursor));
//        _pApp->setStyleSheet(_fullscreenStyleSheet);
////        _browserWidget._browserView->setAlternatingRowColors(false);
//    }
//    else {
//        _pControlPanel->show();
//        if (_playerRackVisible) {
//            showPlayerRack(true);
//        }
//        else {
//            showPlayerRack(false);
//        }
//        _pMainWindow->setCursor(QCursor(Qt::ArrowCursor));
////        _pApp->setStyleSheet(_defaultStyleSheet);
//        _pApp->setStyleSheet("");
////        _browserWidget._browserView->setAlternatingRowColors(true);
//    }
//}
//
//
//void
//QtAvInterface::resize(int width, int height)
//{
//    _pMainWindow->resize(width, height);
//    _pVisual->setWidth(width);
//    _pVisual->setHeight(height);
//}
//
//
//void
//QtAvInterface::showPlayerRack(bool show)
//{
//    _playerRackVisible = show;
//    if (show) {
//        _pPlayerRack->show();
//    }
//    else {
//        _pPlayerRack->hide();
//    }
//}
//
//
//void
//QtAvInterface::showControlPanel(bool show)
//{
//    if (show) {
//        _pControlPanel->show();
//    }
//    else {
//        _pControlPanel->hide();
//    }
//}
//
//
//void
//QtAvInterface::rendererSelected(Omm::Av::AvRendererView* pRenderer)
//{
//    AvUserInterface::rendererSelected(pRenderer);
////    _pPlayerRackButton->setPlayerName(pRenderer->getName());
//    _pControlPanel->setPlayerName(pRenderer->getName());
//}
//
////void
////QtAvInterface::rendererSelectionChanged(const QItemSelection& selected,
////                                         const QItemSelection& /*deselected*/)
////{
////    if (selected.count() > 1) {
////        return;
////    }
////    if (selected.empty()) {
////        return;
////    }
////
////    QModelIndex index = selected.indexes().first();
////
////    Omm::Av::RendererView* selectedRenderer = static_cast<Omm::Av::RendererView*>(index.internalPointer());
////
////    if (selectedRenderer == 0) {
////        return;
////    }
////    rendererSelected(selectedRenderer);
////    if (isPlaying(selectedRenderer)) {
////        _pPlayButton->setIcon(_pBrowserWidget->style()->standardIcon(QStyle::SP_MediaPause));
////        _pPlayButton->setEnabled(true);
////        _playToggle = false;
////        _pStopButton->setEnabled(true);
////    }
////    _pPlayerRackButton->setPlayerName(selectedRenderer->getName());
//////    _pPlayerRackButton->setText(selectedRenderer->getName().c_str());
////}
//
//
//void
//QtAvInterface::skipForwardButtonPressed()
//{
//    Omm::Av::Log::instance()->upnpav().debug("skipping to next track in browser list ...");
//    QModelIndex current = _pBrowserWidget->getCurrentIndex();
////    QModelIndex current = _browserWidget._browserView->currentIndex();
//    if (current.isValid()) {
//        Omm::Av::CtlMediaObject* pCurrentObject = static_cast<Omm::Av::CtlMediaObject*>(current.internalPointer());
//        Omm::Av::Log::instance()->upnpav().debug("current title is: " + pCurrentObject->getTitle());
//        QModelIndex next;
//        do {
//            next = current.sibling(current.row() + 1, 0);
//            if (next.isValid()) {
//                Omm::Av::CtlMediaObject* pNextObject = static_cast<Omm::Av::CtlMediaObject*>(next.internalPointer());
//                if (pNextObject->isContainer()) {
//                    current = next;
//                }
//                else {
//                    Omm::Av::Log::instance()->upnpav().debug("next title is: " + pNextObject->getTitle());
//                    _pBrowserWidget->setCurrentIndex(next);
////                    _browserWidget._browserView->setCurrentIndex(next);
//                    mediaObjectSelected(pNextObject);
//                    playPressed();
////                    _pPlayButton->setIcon(_pBrowserWidget->style()->standardIcon(QStyle::SP_MediaPause));
////                    _playToggle = false;
////                    _pStopButton->setEnabled(true);
//                }
//            }
//        } while(next.isValid() && current == next);
//    }
//}
//
//
//void
//QtAvInterface::skipBackwardButtonPressed()
//{
//    Omm::Av::Log::instance()->upnpav().debug("skipping to previous track in browser list ...");
//    QModelIndex current = _pBrowserWidget->getCurrentIndex();
////    QModelIndex current = _browserWidget._browserView->currentIndex();
//    if (current.isValid()) {
//        Omm::Av::CtlMediaObject* pCurrentObject = static_cast<Omm::Av::CtlMediaObject*>(current.internalPointer());
//        Omm::Av::Log::instance()->upnpav().debug("current title is: " + pCurrentObject->getTitle());
//        QModelIndex previous;
//        do {
//            previous = current.sibling(current.row() - 1, 0);
//            if (previous.isValid()) {
//                Omm::Av::CtlMediaObject* pPreviousObject = static_cast<Omm::Av::CtlMediaObject*>(previous.internalPointer());
//                if (pPreviousObject->isContainer()) {
//                    current = previous;
//                }
//                else {
//                    Omm::Av::Log::instance()->upnpav().debug("previous title is: " + pPreviousObject->getTitle());
//                    _pBrowserWidget->setCurrentIndex(previous);
////                    _browserWidget._browserView->setCurrentIndex(previous);
//                    mediaObjectSelected(pPreviousObject);
//                    playPressed();
////                    _pPlayButton->setIcon(_pBrowserWidget->style()->standardIcon(QStyle::SP_MediaPause));
////                    _playToggle = false;
////                    _pStopButton->setEnabled(true);
//                }
//            }
//        } while(previous.isValid() && current == previous);
//    }
//}
//
//
//void
//QtAvInterface::beginAddServer(int position)
//{
//    _pBrowserWidget->beginAddServer(position);
//}
//
//
//void
//QtAvInterface::endAddServer(int position)
//{
//    _pBrowserWidget->endAddServer();
//}
//
//
//void
//QtAvInterface::beginRemoveServer(int position)
//{
//    _pBrowserWidget->beginRemoveServer(position);
//}
//
//
//void
//QtAvInterface::endRemoveServer(int position)
//{
//    _pBrowserWidget->endRemoveServer();
//}
//
//
//void
//QtAvInterface::beginAddRenderer(int position)
//{
//    _pPlayerRack->beginAddRenderer(position);
//}
//
//
//void
//QtAvInterface::endAddRenderer(int position)
//{
//    _pPlayerRack->endAddRenderer();
//}
//
//
//void
//QtAvInterface::beginRemoveRenderer(int position)
//{
//    _pPlayerRack->beginRemoveRenderer(position);
//}
//
//
//void
//QtAvInterface::endRemoveRenderer(int position)
//{
//    _pPlayerRack->endRemoveRenderer();
//}
//
//
//#ifdef __LINUX__
//#include <signal.h>
//#endif
//
//void
//QtAvInterface::setupUnixSignalHandlers()
//{
//#ifdef __LINUX__
//     struct sigaction hup, term;
//
//     hup.sa_handler = QtAvInterface::unixSignalHandler;
//     sigemptyset(&hup.sa_mask);
//     hup.sa_flags = 0;
//     hup.sa_flags |= SA_RESTART;
//
////     if (sigaction(SIGHUP, &hup, 0) > 0)
////        return 1;
//
//     term.sa_handler = QtAvInterface::unixSignalHandler;
//     sigemptyset(&term.sa_mask);
//     term.sa_flags |= SA_RESTART;
//
////     if (sigaction(SIGTERM, &term, 0) > 0)
////        return 2;
////
////     return 0;
//#endif
//}
//
//
//void
//QtAvInterface::unixSignalHandler(int)
//{
//    Omm::Av::Log::instance()->upnpav().debug("caught unix signal");
////    _pApp->quit();
//}

/*

 *
 * Better use something like Poco::Util::ServerApplication::waitForTerminationRequest()
 * unix style:
void ServerApplication::waitForTerminationRequest()
{
	sigset_t sset;
	sigemptyset(&sset);
	if (!std::getenv("POCO_ENABLE_DEBUGGER"))
	{
		sigaddset(&sset, SIGINT);
	}
	sigaddset(&sset, SIGQUIT);
	sigaddset(&sset, SIGTERM);
	sigprocmask(SIG_BLOCK, &sset, NULL);
	int sig;
	sigwait(&sset, &sig);
}
 *
 * windows style:
 void ServerApplication::waitForTerminationRequest()
{
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	std::string evName("POCOTRM");
	NumberFormatter::appendHex(evName, Process::id(), 8);
	NamedEvent ev(evName);
	ev.wait();
	_terminated.set();
}

 */


//void
//QtAvInterface::newPosition(int duration, int position)
//{
//    emit setSlider(duration, position);
//}
//
//
//void
//QtAvInterface::newTrack(const std::string& title, const std::string& artist, const std::string& album)
//{
//    _pPlayerRackButton->setTitleName(title);
//
//    emit nowPlaying(QString(title.c_str()), QString(artist.c_str()), QString(album.c_str()));
//}
//

//void
//QtAvInterface::newVolume(const int volume)
//{
//    emit volSliderMoved(volume);
//}


#ifdef OMMPLUGIN
POCO_BEGIN_MANIFEST(Omm::UpnpApplication)
POCO_EXPORT_CLASS(QtApplication)
POCO_END_MANIFEST
#endif