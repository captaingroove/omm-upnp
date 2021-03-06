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

#include <iostream>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"


#include "DimmableLightCtrlImpl.h"

using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;


class MyController : public Omm::Controller
{
    virtual void deviceAdded(Omm::DeviceContainer* pDeviceContainer)
    {
        std::cout << "MyController::deviceAdded()" << std::endl;
        std::cout << "uuid: " << pDeviceContainer->getRootDevice()->getUuid() << std::endl;
        std::cout << "type: " << pDeviceContainer->getRootDevice()->getDeviceTypeFullString() << std::endl;
//         std::cout << "friendly name: " << pDeviceContainer->getRootDevice()->getFriendlyName() << std::endl;

        // NOTE: could write a DeviceFactory here ...
        // NOTE: could iterate through all devices of DeviceContainer here ...
        if (pDeviceContainer->getRootDevice()->getDeviceTypeFullString() == "urn:schemas-upnp-org:device:DimmableLight:1") {
            Poco::Timestamp t;
            std::cout << "TIME: " << t.elapsed() << std::endl;
            DimmableLightController* pDimmableLightCtrl = new DimmableLightController(
                pDeviceContainer->getRootDevice(),
                new SwitchPowerControllerImpl,
                new DimmingControllerImpl);

            bool status;
            std::cout << "sync call of GetStatus() started" << std::endl;
            std::cout << "TIME: " << t.elapsed() << std::endl;
            pDimmableLightCtrl->SwitchPower()->GetStatus(status);
            std::cout << "TIME: " << t.elapsed() << std::endl;
            std::cout << "sync call of GetStatus() finished" << std::endl;
            std::cout << "sync call of GetStatus() returns: " << status << std::endl;
            std::cout << "async request _reqGetStatus() started" << std::endl;
            std::cout << "TIME: " << t.elapsed() << std::endl;
            pDimmableLightCtrl->SwitchPower()->_reqGetStatus();
            std::cout << "TIME: " << t.elapsed() << std::endl;
            std::cout << "async request _reqGetStatus() finished" << std::endl;
            std::cout << "state variable Status lookup" << std::endl;
            status = pDimmableLightCtrl->SwitchPower()->_getStatus();
            std::cout << "state variable Status value: " << status << std::endl;
        }
    }

    virtual void deviceRemoved(Omm::DeviceContainer* pDeviceContainer)
    {
        std::cerr << "MyController::deviceRemoved() uuid: " << pDeviceContainer->getRootDevice()->getUuid() << std::endl;
    }
};


class ControllerTest: public Poco::Util::ServerApplication
{
public:
    ControllerTest(): _helpRequested(false)
    {
    }

    ~ControllerTest()
    {
    }

protected:
    void initialize(Application& self)
    {
        loadConfiguration(); // load default configuration files, if present
        ServerApplication::initialize(self);
    }

    void uninitialize()
    {
        ServerApplication::uninitialize();
    }

    void defineOptions(OptionSet& options)
    {
        ServerApplication::defineOptions(options);

        options.addOption(
                           Option("help", "h", "display help information on command line arguments")
                           .required(false)
                           .repeatable(false));
    }

    void handleOption(const std::string& name, const std::string& value)
    {
        ServerApplication::handleOption(name, value);

        if (name == "help")
            _helpRequested = true;
    }

    void displayHelp()
    {
        HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setUsage("OPTIONS");
        helpFormatter.setHeader("A simple UPnP Controller.");
        helpFormatter.format(std::cout);
    }

    int main(const std::vector<std::string>& args)
    {
        if (_helpRequested)
        {
            displayHelp();
        }
        else
        {
        // get parameters from configuration file
//             unsigned short port = (unsigned short) config().getInt("EchoServer.port", 9977);

            MyController controller;
            controller.setState(Omm::DeviceManager::Public);

            std::cerr << "ControllerTest::main() waiting for termination request" << std::endl;
            waitForTerminationRequest();
        }
        return Application::EXIT_OK;
    }

private:
    bool _helpRequested;
};


int main(int argc, char** argv)
{
    ControllerTest app;
    return app.run(argc, argv);
}