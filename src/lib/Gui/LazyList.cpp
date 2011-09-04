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

#include "Gui/LazyList.h"
#include "Gui/GuiLogger.h"
#include "Gui/ListModel.h"
#include "Gui/View.h"

#include "Gui/Button.h"

#ifdef __GUI_QT_PLATFORM__
#include "Qt/ListImpl.h"
#endif


namespace Omm {
namespace Gui {


LazyListView::LazyListView(View* pParent) :
ListView(new ListViewImpl(this, pParent), pParent)
{

}


void
LazyListView::setModel(LazyListModel* pModel)
{
    Log::instance()->gui().debug("lazy list view set model ...");

    View::setModel(pModel);

    int rows = visibleRows();
    int rowsFetched = pModel->fetch(std::min(pModel->totalItemCount(), rows));
    extendViewPool(rows);

    // insert items that are already in the model.
    Log::instance()->gui().debug("inserting number of items: " + Poco::NumberFormatter::format(rowsFetched));
    for (int i = 0; i < rowsFetched; i++) {
        insertItem(i);
    }

    Log::instance()->gui().debug("lazy list view set model finished.");
}


void
LazyListView::scrolledToRow(int rowOffset)
{
    Log::instance()->gui().debug("lazy list view scroll view ...");

    LazyListModel* pModel = static_cast<LazyListModel*>(_pModel);
    
    int rowDelta = rowOffset - _rowOffset;

    if (rowDelta == 0) {
        return;
    }
    if (rowOffset + _visibleViews.size() > pModel->totalItemCount()) {
        return;
    }

    int rowDeltaAbsolute = std::abs(rowDelta);

    if (rowOffset + _visibleViews.size() + rowDeltaAbsolute >= pModel->lastFetched()) {
        pModel->fetch(_visibleViews.size() + rowDeltaAbsolute);
    }

    Log::instance()->gui().debug("lazy list view scroll view to row offset: " + Poco::NumberFormatter::format(rowOffset) + ", delta: " + Poco::NumberFormatter::format(rowDeltaAbsolute));
    while (rowDeltaAbsolute--) {
        scrollDelta(rowDelta);
    }
}


void
LazyListView::resize(int rows, int width)
{
    LazyListModel* pModel = static_cast<LazyListModel*>(_pModel);

    int rowDelta = rows - _viewPool.size();
    Log::instance()->gui().debug("lazy list view resize row delta: " + Poco::NumberFormatter::format(rowDelta));
    if (rowDelta > 0) {
        if (_rowOffset + _visibleViews.size() + rowDelta >= pModel->lastFetched()) {
            pModel->fetch(_visibleViews.size() + rowDelta);
        }
    }
    resizeDelta(rowDelta, width);
}


} // namespace Gui
} // namespace Omm