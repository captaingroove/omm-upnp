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

#include "SplitterImpl.h"
#include "Gui/Splitter.h"
#include "Gui/GuiLogger.h"

namespace Omm {
namespace Gui {


SplitterViewImpl::SplitterViewImpl(View* pView, View::Orientation orientation)
{
}


SplitterViewImpl::~SplitterViewImpl()
{
}


void
SplitterViewImpl::setOrientation(View::Orientation orientation)
{
}


void
SplitterViewImpl::insertView(View* pView, int index)
{
}


std::vector<float>
SplitterViewImpl::getSizes()
{
    std::vector<float> res;
    return res;
}


void
SplitterViewImpl::setSizes(const std::vector<float>& sizes)
{
}


}  // namespace Omm
}  // namespace Gui
