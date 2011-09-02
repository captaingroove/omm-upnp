/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2009, 2010                                                 |
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

#include <Omm/Gui/EventLoop.h>
#include <Omm/Gui/MainWindow.h>
#include <Omm/Gui/Button.h>


class HelloButtonController : public Omm::Gui::ButtonController
{
private:
    void pushed()
    {
        UPDATE_MODEL(Omm::Gui::ButtonModel, setLabel, "works!");
    }
};


int main(int argc, char** argv)
{
    Omm::Gui::EventLoop loop(argc, argv);
    Omm::Gui::MainWindow mainWindow;
    HelloButtonController buttonController;
    Omm::Gui::Button button;
    button.attachController(&buttonController);
    buttonController.attachModel(&button);
    button.setLabel("Hello GUI");
    mainWindow.setMainView(&button);

    mainWindow.show();

    loop.run();
}

