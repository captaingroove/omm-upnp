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

#include <string>
#include "View.h"
#include "Model.h"
#include "Controller.h"


namespace Omm {
namespace Gui {


class ButtonModel : public Model
{
public:
    virtual const std::string& getLabel() const;
    void setLabel(const std::string& label);
    
private:
    std::string _label;
};


class ButtonView : public View
{
    friend class ButtonModel;
    
public:
    ButtonView(View* pParent = 0);
    
private:
    virtual void syncView(Model* pModel);
};


class ButtonController : public Controller
{
public:
    virtual void pushed() {}
};


class Button : public ButtonView, public ButtonModel, public ButtonController
{
public:
    Button(View* pParent = 0);
    virtual ~Button();
};


class ButtonControllerModel : public ControllerModel<ButtonModel, ButtonController>
{

};


}  // namespace Omm
}  // namespace Gui

#endif