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

#include <Poco/Exception.h>

#include <Omm/UpnpAvController.h>
#include <Omm/UpnpAvRenderer.h>
#include <Omm/Util.h>


int main(int argc, char** argv)
{
    Omm::Util::PluginLoader<Omm::Av::AvUserInterface> pluginLoader;
    Omm::Av::AvUserInterface* pUserInterface;
    try {
        pUserInterface = pluginLoader.load("avinterface-qt", "AvInterface");
    }
    catch(Poco::NotFoundException) {
        Omm::Av::Log::instance()->upnpav().error("controller application could not find plugin for user interface");
        return 1;
    }

    Omm::Util::PluginLoader<Omm::Av::Engine> enginePluginLoader;
    Omm::Av::Engine* pEnginePlugin;
    try {
        pEnginePlugin = enginePluginLoader.load("engine-vlc");
    }
    catch(Poco::NotFoundException) {
        Omm::Av::Log::instance()->upnpav().error("controller application could not find plugin for engine");
        return 1;
    }

    pEnginePlugin->setVisual(pUserInterface->getVisual());

    pEnginePlugin->createPlayer();

    Omm::Av::AvRenderer renderer(pEnginePlugin);
    Omm::Icon* pIcon = new Omm::Icon(22, 22, 8, "image/png", "renderer.png");
    renderer.addIcon(pIcon);
    renderer.setFriendlyName("OMM Renderer");
    renderer.start();

    Omm::Av::AvController controller;

    controller.setUserInterface(pUserInterface);
    pUserInterface->initGui();
    pUserInterface->showMainWindow();
    controller.start();
    Omm::Av::Log::instance()->upnpav().debug("ControllerApplication: starting event loop");
    return pUserInterface->eventLoop();
}