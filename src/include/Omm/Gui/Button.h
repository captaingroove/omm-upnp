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

#ifndef Button_INCLUDED
#define Button_INCLUDED

#include "Widget.h"

namespace Omm {
namespace Gui {


class ButtonModel : public Model
{
public:
    virtual const std::string& getLabel() const;
    void setLabel(const std::string& label);
    
    virtual void pushed() {}

private:
    std::string _label;
};


class Button : public Widget, public ButtonModel
{
    friend class ButtonImpl;
    friend class ButtonModel;
    
public:
    Button(Widget* pParent = 0);
    Button(ButtonModel* pModel, Widget* pParent = 0);
    virtual ~Button();

    virtual void pushed();

private:
    virtual void syncView();
};


}  // namespace Omm
}  // namespace Gui

#endif
