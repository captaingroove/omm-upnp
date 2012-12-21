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
#include <qt4/QtGui/qsplitter.h>

#include "SplitterImpl.h"
#include "Gui/Splitter.h"
#include "Gui/GuiLogger.h"

namespace Omm {
namespace Gui {


SplitterViewImpl::SplitterViewImpl(View* pView, View::Orientation orientation)
{
//    LOG(gui, debug, "splitter view impl ctor");
    QSplitter* pNativeView = new QSplitter;

    initViewImpl(pView, pNativeView);

    if (orientation == View::Vertical) {
        pNativeView->setOrientation(Qt::Vertical);
    }
}


SplitterViewImpl::~SplitterViewImpl()
{
}


void
SplitterViewImpl::setOrientation(View::Orientation orientation)
{
    switch(orientation) {
        case View::Horizontal:
            static_cast<QSplitter*>(_pNativeView)->setOrientation(Qt::Horizontal);
            break;
        case View::Vertical:
            static_cast<QSplitter*>(_pNativeView)->setOrientation(Qt::Vertical);
            break;
    }
}


void
SplitterViewImpl::insertView(View* pView, int index)
{
    static_cast<QSplitter*>(_pNativeView)->insertWidget(index, static_cast<QWidget*>(pView->getNativeView()));
}


}  // namespace Omm
}  // namespace Gui