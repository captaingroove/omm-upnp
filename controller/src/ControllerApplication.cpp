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

#include <Poco/Exception.h>
#include <Omm/UpnpController.h>

int main(int argc, char** argv)
{
    Omm::PluginLoader<Omm::Av::UpnpAvUserInterface> guiLoader;
    try {
        guiLoader.loadPlugin("c-av-ui-qt");
    }
    catch(Poco::NotFoundException) {
        std::cerr << "Error in ControllerApplication: could not find plugin for user interface" << std::endl;
        return 1;
    }
    std::clog << "ControllerApplication: user interface loaded successfully" << std::endl;
    
    Omm::Av::UpnpAvUserInterface* pUserInterface;
    Omm::Av::UpnpAvController controller;
    
    pUserInterface = guiLoader.create("ControllerGui");
    controller.setUserInterface(pUserInterface);
    pUserInterface->initGui();
    pUserInterface->showMainWindow();
    controller.start();
    std::clog << "ControllerApplication: starting event loop" << std::endl;
    return pUserInterface->eventLoop();
}