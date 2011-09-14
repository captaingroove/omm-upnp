/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2009, 2010, 2011                                           |
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

#include <Omm/Gui/Application.h>
#include <Omm/UpnpGui.h>


class Application : public Omm::Gui::Application
{
    virtual Omm::Gui::View* createMainView()
    {
        resize(800, 480);
        _pController = new Omm::ControllerWidget;
        return _pController;
    }

    virtual void presentedMainView()
    {
        _pController->init();
        _pController->start();
    }

    virtual void finishedEventLoop()
    {
        _pController->stop();
    }

    Omm::ControllerWidget* _pController;
};


int main(int argc, char** argv)
{
    Application app;
    return app.run(argc, argv);
}

