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

#include <QtGui>
#include <Poco/NumberFormatter.h>

#include "NavigatorImpl.h"
#include "QtNavigatorPanel.h"
#include "Gui/Navigator.h"
#include "Gui/GuiLogger.h"
#include "Log.h"


namespace Omm {
namespace Gui {


QtNavigatorPanelButton::QtNavigatorPanelButton(View* pView) :
_pView(pView)
{
    setFlat(true);
    setIcon(QIcon(QString(":/images/right_arrow.gif")));
//    setStyle(new QCleanlooksStyle);
//    setStyle(new QPlastiqueStyle);
//    setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
//    setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
//    setIcon(style()->standardIcon(QStyle::PE_IndicatorArrowRight));
}


//void
//QtNavigatorPanelButton::paintEvent(QPaintEvent* event)
//{
//    QPainter painter(this);
//
//    QStyleOptionFocusRect option;
//    option.initFrom(this);
//    option.backgroundColor = palette().color(QPalette::Background);
//
//    style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter, this);
//}


QtNavigatorPanel::QtNavigatorPanel(NavigatorViewImpl* pNavigatorView) :
QWidget(pNavigatorView->getNativeView()),
_pNavigatorView(pNavigatorView)
{
    _pPanelLayout = new QHBoxLayout(this);
    _pButtonWidget = new QWidget(this);
    _pPanelLayout->addWidget(_pButtonWidget);
    _pSearchWidget = new QLineEdit(this);
    _pPanelLayout->addWidget(_pSearchWidget);
    _pSearchWidget->setHidden(true);
    _pButtonLayout = new QHBoxLayout(_pButtonWidget);
    _pButtonLayout->setAlignment(Qt::AlignLeft);
    _pButtonLayout->setSpacing(0);
    _pButtonLayout->setMargin(0);
    _pButtonLayout->setContentsMargins(0, 0, 0, 0);

    _pIconProvider = new QFileIconProvider;

    connect(this, SIGNAL(popSignal()), this, SLOT(popSlot()));
    connect(this, SIGNAL(popToRootSignal()), this, SLOT(popToRootSlot()));
    connect(_pSearchWidget, SIGNAL(textEdited(const QString&)), this, SLOT(textEdited(const QString&)));
}


void
QtNavigatorPanel::push(View* pView, const std::string name)
{
    QtNavigatorPanelButton* pButton = new QtNavigatorPanelButton(pView);
    pButton->_pView = pView;
    pButton->setText(QString::fromStdString(name));
    connect(pButton, SIGNAL(pressed()), this, SLOT(buttonPushed()));
    _pButtonLayout->addWidget(pButton);
    _buttonStack.push(pButton);
}


void
QtNavigatorPanel::pop(View* pView)
{
    while(!_buttonStack.empty() && _buttonStack.top()->_pView != pView) {
        QtNavigatorPanelButton* pButton = _buttonStack.top();
        _pNavigatorView->removeView(pButton->_pView);
        disconnect(pButton, SIGNAL(pressed()), this, SLOT(buttonPushed()));
        _pButtonLayout->removeWidget(pButton);
        delete pButton;
        _buttonStack.pop();
    }
    if (!_buttonStack.empty()) {
        _pNavigatorView->exposeView(pView);
    }
}


void
QtNavigatorPanel::popSlot()
{
    if (_buttonStack.empty()) {
        return;
    }
    QtNavigatorPanelButton* pButton = _buttonStack.top();
    _pNavigatorView->removeView(pButton->_pView);
    disconnect(pButton, SIGNAL(pressed()), this, SLOT(buttonPushed()));
    _pButtonLayout->removeWidget(pButton);
    delete pButton;
    _buttonStack.pop();
    if (!_buttonStack.empty()) {
        _pNavigatorView->exposeView(_buttonStack.top()->_pView);
    }
}


void
QtNavigatorPanel::popToRootSlot()
{
    while (_buttonStack.size() > 1) {
        popSlot();
    }
}


void
QtNavigatorPanel::buttonPushed()
{
    QtNavigatorPanelButton* pButton = static_cast<QtNavigatorPanelButton*>(QObject::sender());
    pop(pButton->_pView);
}


void
QtNavigatorPanel::textEdited(const QString& text)
{
    LOG(gui, debug, "search text changed: " + text.toStdString());
    _pNavigatorView->changedSearchText(text.toStdString());
}


long unsigned int
QtNavigatorPanel::buttonCount()
{
    return _buttonStack.size();
}


View*
QtNavigatorPanel::topView()
{
    if (_buttonStack.empty()) {
        return 0;
    }
    else {
        return _buttonStack.top()->_pView;
    }
}


NavigatorViewImpl::NavigatorViewImpl(View* pView)
{
    QWidget* pNativeView = new QWidget;
    initViewImpl(pView, pNativeView);

    _pNavigatorPanel = new QtNavigatorPanel(this);
    _pStackedWidget = new QStackedWidget(pNativeView);
    _pNavigatorLayout = new QVBoxLayout(pNativeView);
    _pNavigatorLayout->setContentsMargins(0, 0, 0, 0);
    _pNavigatorLayout->addWidget(_pNavigatorPanel);
    _pNavigatorLayout->addWidget(_pStackedWidget);
}


NavigatorViewImpl::~NavigatorViewImpl()
{
    delete _pNavigatorLayout;
    delete _pStackedWidget;
    delete _pNavigatorPanel;
}


void
NavigatorViewImpl::pushView(View* pView, const std::string name)
{
    QWidget* pWidget = static_cast<QWidget*>(pView->getNativeView());
    _pStackedWidget->addWidget(pWidget);
    _pStackedWidget->setCurrentWidget(pWidget);
    _pNavigatorPanel->push(pView, name);
    pWidget->show();
}


void
NavigatorViewImpl::popView(bool keepRootView)
{
    if (keepRootView && _pNavigatorPanel->buttonCount() == 1) {
        return;
    }
    emit _pNavigatorPanel->popSignal();
}


void
NavigatorViewImpl::popToRootView()
{
    emit _pNavigatorPanel->popToRootSignal();
}


View*
NavigatorViewImpl::getVisibleView()
{
    return _pNavigatorPanel->topView();
}


void
NavigatorViewImpl::showSearchBox(bool show)
{
    _pNavigatorPanel->_pSearchWidget->setHidden(!show);
}


void
NavigatorViewImpl::removeView(View* pView)
{
    QWidget* pWidget = static_cast<QWidget*>(pView->getNativeView());
    _pStackedWidget->removeWidget(pWidget);
}


void
NavigatorViewImpl::exposeView(View* pView)
{
    QWidget* pWidget = static_cast<QWidget*>(pView->getNativeView());
    _pStackedWidget->setCurrentWidget(pWidget);
    pWidget->show();
}


void
NavigatorViewImpl::changedSearchText(const std::string& searchText)
{
    IMPL_NOTIFY_CONTROLLER(NavigatorController, changedSearchText, searchText);
}


} // namespace Gui
} // namespace Omm
