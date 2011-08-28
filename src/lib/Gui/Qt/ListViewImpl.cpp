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

#include <Omm/Util.h>
#include <Omm/UpnpAvLogger.h>

#include "ListViewImpl.h"
#include "Gui/ListView.h"
#include "Gui/GuiLogger.h"
#include "Gui/ListItemView.h"

namespace Omm {
namespace Gui {


ListViewImpl::ListViewImpl(View* pView, bool movableViews, View* pParent) :
QGraphicsView(static_cast<QWidget*>(pParent ? pParent->getNativeView() : 0)),
ViewImpl(pView, this),
_movableViews(movableViews)
{
    setAlignment((Qt::AlignLeft | Qt::AlignTop));

    _pGraphicsScene = new QGraphicsScene;
    setScene(_pGraphicsScene);

    connect(this, SIGNAL(moveWidgetSignal(int, ListItemView*)), this, SLOT(moveWidgetSlot(int, ListItemView*)));
    connect(this, SIGNAL(extendPoolSignal()), this, SLOT(extendPoolSlot()));
}


ListViewImpl::~ListViewImpl()
{
}


int
ListViewImpl::visibleRows()
{
    ListView* pListView =  static_cast<ListView*>(_pView);
    int rows = viewport()->geometry().height() / pListView->_viewHeight;
    Omm::Gui::Log::instance()->gui().debug("widget canvas number of visible rows: " + Poco::NumberFormatter::format(rows));
    return rows;
}


void
ListViewImpl::initView(ListItemView* pView)
{
    ListView* pListView =  static_cast<ListView*>(_pView);
    pView->resize(viewport()->width(), pListView->_viewHeight);

    QWidget* pQWidget = static_cast<QWidget*>(pView->getNativeView());
    if (_movableViews) {
        _proxyWidgets[pView] = _pGraphicsScene->addWidget(pQWidget, Qt::Window);
    }
    else {
        _proxyWidgets[pView] = _pGraphicsScene->addWidget(pQWidget);
   }
}


void
ListViewImpl::moveView(int row, ListItemView* pView)
{
    Omm::Gui::Log::instance()->gui().debug("widget canvas move item widget to row: " + Poco::NumberFormatter::format(row));
    emit moveWidgetSignal(row, pView);
}


void
ListViewImpl::extendViewPool()
{
    emit extendPoolSignal();
}


void
ListViewImpl::moveWidgetSlot(int row, ListItemView* pView)
{
    ListView* pListView =  static_cast<ListView*>(_pView);
    _proxyWidgets[pView]->setPos(0, pListView->_viewHeight * row);
}


void
ListViewImpl::extendPoolSlot()
{
    ListView* pListView =  static_cast<ListView*>(_pView);
    pListView->extendViewPool(visibleRows());
}


//LazyListViewImpl::LazyListViewImpl(Widget* pView, Widget* pParent) :
//QScrollArea(static_cast<QWidget*>(pParent ? pParent->getNativeWidget() : 0)),
//_pScrollWidget(0)
//{
//    _pScrollWidget = new QWidget;
//    _pScrollWidget->resize(viewport()->size());
//    setWidget(_pScrollWidget);
//
//    connect(this, SIGNAL(moveWidgetSignal(int, ListWidget*)), this, SLOT(moveWidgetSlot(int, ListWidget*)));
//    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(viewScrolledSlot(int)));
//}
//
//
//LazyListViewImpl::~LazyListViewImpl()
//{
//}
//
//
//int
//LazyListViewImpl::visibleRows()
//{
//    int rows = viewport()->geometry().height() / _widgetHeight;
//    Omm::Gui::Log::instance()->gui().debug("widget list number of visible rows: " + Poco::NumberFormatter::format(rows));
//    return rows;
//}
//
//
//void
//LazyListViewImpl::initWidget(ListWidget* pView)
//{
//    static_cast<QtListWidget*>(pView)->resize(viewport()->width(), _widgetHeight);
//    static_cast<QtListWidget*>(pView)->setParent(_pScrollWidget);
//}
//
//
//void
//LazyListViewImpl::moveWidget(int row, ListWidget* pView)
//{
//    Omm::Gui::Log::instance()->gui().debug("widget list move item widget to row: " + Poco::NumberFormatter::format(row));
//    emit moveWidgetSignal(row, pView);
//}
//
//
//void
//LazyListViewImpl::updateScrollWidgetSize()
//{
//   _pScrollWidget->resize(geometry().width(), _pModel->totalItemCount() * _widgetHeight);
//}
//
//
//int
//LazyListViewImpl::getOffset()
//{
////    Omm::Gui::Log::instance()->gui().debug("scroll widget offset: " + Poco::NumberFormatter::format(_pScrollWidget->geometry().y()));
//    return _pScrollWidget->geometry().y();
//}
//
//
//void
//LazyListViewImpl::moveWidgetSlot(int row, ListWidget* pView)
//{
//    static_cast<QtListWidget*>(pView)->move(0, _widgetHeight * row);
//}
//
//
//void
//LazyListViewImpl::viewScrolledSlot(int value)
//{
//    scrolledToRow(-getOffset() / _widgetHeight);
//}
//
//
//void
//LazyListViewImpl::resizeEvent(QResizeEvent* pEvent)
//{
//    int rows = pEvent->size().height() / _widgetHeight;
//    Omm::Gui::Log::instance()->gui().debug("Qt widget list resize: " + Poco::NumberFormatter::format(rows));
//    if (pEvent->oldSize().height() > 0) {
//        WidgetListView::resize(rows);
//    }
//}


}  // namespace Omm
}  // namespace Gui