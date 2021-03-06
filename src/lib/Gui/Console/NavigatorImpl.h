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

#ifndef NavigatorImpl_INCLUDED
#define NavigatorImpl_INCLUDED

//#include <stack>
#include "ViewImpl.h"

namespace Omm {
namespace Gui {

class NavigatorViewImpl : public ViewImpl
{
public:
    NavigatorViewImpl(View* pView);
    virtual ~NavigatorViewImpl();

    void pushView(View* pView, const std::string label);
    void popView(bool keepRootView);
    void popToRootView();
    View* getVisibleView();
    int viewCount();
    void showNavigatorBar(bool show);
    void showSearchBox(bool show);
    void clearSearchText();
    void showRightButton(bool show);
    void setRightButtonLabel(const std::string& label);
    void setRightButtonColor(const Color& color);

private:
    void removeView(View* pView);
    void exposeView(View* pView);
    void changedSearchText(const std::string& searchText);
    void poppedToRoot();
    void poppedToView(View* pView);
    void rightButtonPushed();
};


} // namespace Gui
} // namespace Omm


#endif

