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

#include <iostream>

#include <Poco/NotificationCenter.h>
#include <Poco/Observer.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/StreamCopier.h>
#include <Poco/NamedMutex.h>
#include <Poco/Util/Application.h>

#include "UpnpGui/ControllerWidget.h"
#include "UpnpGui/UpnpApplication.h"
#include "UpnpGui/GuiVisual.h"

#include "Util.h"
#include "Gui/GuiLogger.h"
#include "UpnpAv.h"
#include "UpnpAvCtlServer.h"
#include "MediaImages.h"


// FIXME: make engines also loadable, so that ommcontroller executable
//        doesn't need to link against an engine.
#ifdef __IPHONE__
#include <Omm/X/EngineMPMoviePlayer.h>
#else
#include <Omm/X/EngineVlc.h>
#include <Omm/X/EnginePhonon.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Net/NameValueCollection.h>
#endif

namespace Omm {

void
ConfigRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    LOG(upnp, debug, "omm application http request \"" + request.getURI() + "\" from " + request.getHost());

    Poco::URI requestUri(request.getURI());

    if (requestUri.getPath() == UpnpApplication::PLAYLIST_URI) {
        std::stringstream* pPlaylistResource = _pApp->_pControllerWidget->getPlaylistResource();
        if (pPlaylistResource) {
            std::ostream& outStream = response.send();
            Poco::StreamCopier::copyStream(*pPlaylistResource, outStream);
            delete pPlaylistResource;
        }
    }
    else if (requestUri.getPath() == UpnpApplication::CONFIG_URI) {
//        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        Poco::Net::HTMLForm form(request, request.stream());
        if (requestUri.getQuery() == UpnpApplication::CONFIG_APP_QUERY) {
            _pApp->handleAppConfigRequest(form);
        }
        else if (requestUri.getQuery() == UpnpApplication::CONFIG_DEV_QUERY) {
            _pApp->handleDevConfigRequest(form);
        }

        std::ostream& outStream = response.send();
        std::istream* pConfigPage = _pApp->generateConfigPage();
        Poco::StreamCopier::copyStream(*pConfigPage, outStream);
        delete pConfigPage;
    }
    else {
        response.send();
    }
}


Poco::Net::HTTPRequestHandler*
ConfigRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
    return new ConfigRequestHandler(_pApp);
}


const std::string UpnpApplication::PLAYLIST_URI = "/Playlist";
const std::string UpnpApplication::CONFIG_URI = "/Config";
const std::string UpnpApplication::CONFIG_APP_QUERY = "saveForm=app";
const std::string UpnpApplication::CONFIG_DEV_QUERY = "saveForm=dev";
const std::string UpnpApplication::ModeFull = "ModeFull";
const std::string UpnpApplication::ModeRendererOnly = "ModeRendererOnly";

UpnpApplication::UpnpApplication(int argc, char** argv) :
//Poco::Util::Application(argc, argv),
_argc(argc),
_argv(argv),
_helpRequested(false),
_lockInstance(true),
_ignoreConfig(false),
//#ifdef __IPHONE__
//    _rendererName("iPhone Renderer"),
//#else
//    _rendererName("OMM Renderer"),
//#endif
_enableRenderer(false),
_enableServer(false),
_enableController(true),
_showRendererVisualOnly(false),
_pControllerWidget(0),
_pLocalDeviceServer(new DeviceServer),
_pLocalDeviceContainer(new DeviceContainer),
_pLocalMediaRenderer(0),
_pConf(0),
_mode(ModeFull),
_instanceMutexName("OmmApplicationMutex"),
_appStandardPort(4009)
{
    setUnixOptions(true);
}


UpnpApplication::~UpnpApplication()
{
    delete _pLocalDeviceServer;
    delete _pHttpServer;
}


void
UpnpApplication::defineOptions(Poco::Util::OptionSet& options)
{
    Poco::Util::Application::defineOptions(options);

    options.addOption(Poco::Util::Option("help", "", "display help information on command line arguments")
                      .required(false)
                      .repeatable(false));
    options.addOption(Poco::Util::Option("width", "w", "width of application window")
                      .binding("application.width")
                      .required(false)
                      .repeatable(false)
                      .argument("width", true));
    options.addOption(Poco::Util::Option("height", "h", "height of application window")
                      .binding("application.height")
                      .required(false)
                      .repeatable(false)
                      .argument("height", true));
    options.addOption(Poco::Util::Option("fullscreen", "f", "option passed to plugin")
                      .binding("application.fullscreen")
                      .required(false)
                      .repeatable(false));
    options.addOption(Poco::Util::Option("zoom", "z", "application window scale factor")
                      .binding("application.scale")
                      .required(false)
                      .repeatable(false)
                      .argument("scale", true));
    options.addOption(Poco::Util::Option("renderer", "r", "enable renderer  \"name:uuid:engine\"")
                      .required(false)
                      .repeatable(false)
                      .argument("renderername", true));
    options.addOption(Poco::Util::Option("server", "s", "add server \"name:uuid:datamodel:basepath\"")
                      .required(false)
                      .repeatable(true)
                      .argument("device", true));
}


void
UpnpApplication::handleOption(const std::string& name, const std::string& value)
{
    Poco::Util::Application::handleOption(name, value);

    if (name == "help") {
        _helpRequested = true;
    }
    else if (name == "renderer") {
        Poco::StringTokenizer rendererSpec(value, ":");
        if (rendererSpec.count() < 3) {
            LOGNS(Av, upnpav, information, "renderer spec \"" + value + "\" needs four parameters, \"name:uuid:engine\", ignoring");
        }
        else {
            std::string uuid = rendererSpec[1];
            // uuid may be a valid uuid or empty, when empty assign a random uuid
            if (uuid == "") {
                uuid = Poco::UUIDGenerator().createRandom().toString();
            }
            config().setString("renderer.enable", "true");
            config().setString("renderer.friendlyName", rendererSpec[0]);
            config().setString("renderer.uuid", uuid);
            config().setString("renderer.plugin", "engine-" + rendererSpec[2]);
        }
    }
    else if (name == "server") {
        Poco::StringTokenizer serverSpec(value, ":");
        if (serverSpec.count() < 4) {
            LOGNS(Av, upnpav, information, "server spec \"" + value + "\" needs four parameters, \"name:uuid:datamodel:basepath\", ignoring");
        }
        else {
            std::string uuid = serverSpec[1];
            // uuid may be a valid uuid or empty, when empty assign a random uuid
            if (uuid == "") {
                uuid = Poco::UUIDGenerator().createRandom().toString();
            }
            config().setString("server." + uuid + ".enable", "true");
            config().setString("server." + uuid + ".friendlyName", serverSpec[0]);
            config().setString("server." + uuid + ".uuid", uuid);
            config().setString("server." + uuid + ".plugin", "model-" + serverSpec[2]);
            config().setString("server." + uuid + ".basePath", serverSpec[3]);
        }
    }
    else if (name == "fullscreen") {
        config().setString("application.fullscreen", "true");
    }
}


int
UpnpApplication::main(const std::vector<std::string>& args)
{
    if (_helpRequested)
    {
        displayHelp();
    }
    else
    {
        Poco::Util::Application::init(_argc, _argv);
        if (instanceAlreadyRunning()) {
            LOG(upnp, information, "omm application instance running, starting in controller mode");
            setLockInstance(false);
            setIgnoreConfig(true);
        }
        loadConfig();
        initConfig();
        Gui::Application::runEventLoop(_argc, _argv);
    }
    return Poco::Util::Application::EXIT_OK;
}


Omm::Gui::View*
UpnpApplication::createMainView()
{
    _pControllerWidget = new Omm::ControllerWidget(this);
    if (!_showRendererVisualOnly) {
        addControlPanel(_pControllerWidget->getControlPanel());
        addControlPanel(_pControllerWidget->getStatusBar());
    }
    return _pControllerWidget;
}


void
UpnpApplication::presentedMainView()
{
    if (config().getBool("application.fullscreen", false)) {
        _pControllerWidget->setHandlesHidden(true);
//        _pControllerWidget->showOnlyBasicDeviceGroups(true);
    }
    else if (_showRendererVisualOnly) {
        _pControllerWidget->showOnlyRendererVisual(true);
        _pControllerWidget->setHandlesHidden(true);
    }
    _pControllerWidget->init();
}


void
UpnpApplication::start()
{
    startAppHttpServer();
    initLocalDevices();
    if (_enableController) {
        _pControllerWidget->setState(config().getString("application.state", DeviceManager::Started));
    }
    _pLocalDeviceServer->setState(config().getString("application.state", DeviceManager::Started));
}


void
UpnpApplication::stop()
{
    LOGNS(Av, upnpav, debug, "omm application stopping ...");
    _pLocalDeviceServer->setState(DeviceManager::Stopped);
    if (_enableController) {
        _pControllerWidget->setState(DeviceManager::Stopped);
    }
    stopAppHttpServer();
    saveConfig();
    Poco::Util::Application::uninitialize();
    LOGNS(Av, upnpav, debug, "omm application stopped.");
}


void
UpnpApplication::displayHelp()
{
    Poco::Util::HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("OMM application running a UPnP controller, renderer and servers.");
    helpFormatter.format(std::cout);
}


void
UpnpApplication::setLockInstance(bool lock)
{
    _lockInstance = lock;
}


void
UpnpApplication::setIgnoreConfig(bool ignore)
{
    _ignoreConfig = ignore;
}


void
UpnpApplication::printConfig()
{
//    std::vector<std::string> rootKeys;
//    config().keys(rootKeys);
//    for (std::vector<std::string>::iterator it = rootKeys.begin(); it != rootKeys.end(); ++it) {
//        LOGNS(Av, upnpav, debug, "omm config, root keys: " + *it);
//    }

    std::vector<std::string> rendererKeys;
    config().keys("renderer", rendererKeys);
    for (std::vector<std::string>::iterator it = rendererKeys.begin(); it != rendererKeys.end(); ++it) {
        LOGNS(Av, upnpav, debug, "omm config renderer." + *it + ": " + config().getString("renderer." + *it, ""));
    }

    std::vector<std::string> serverKeys;
    config().keys("server", serverKeys);
    for (std::vector<std::string>::iterator it = serverKeys.begin(); it != serverKeys.end(); ++it) {
        std::vector<std::string> serverConfigKeys;
        config().keys("server." + *it, serverConfigKeys);
        for (std::vector<std::string>::iterator cit = serverConfigKeys.begin(); cit != serverConfigKeys.end(); ++cit) {
            LOGNS(Av, upnpav, debug, "omm config server." + *it + "." + *cit + ": " + config().getString("server." + *it + "." + *cit, ""));
        }
    }

//    if (_pConf) {
//        std::vector<std::string> confKeys;
//        _pConf->keys(confKeys);
//        for (std::vector<std::string>::iterator it = confKeys.begin(); it != confKeys.end(); ++it) {
//            LOGNS(Av, upnpav, debug, "omm config, config file keys: " + *it + ", value: " + _pConf->getString(*it, ""));
//        }
//    }

//    std::vector<std::string> appKeys;
//    config().keys("application", appKeys);
//    for (std::vector<std::string>::iterator it = appKeys.begin(); it != appKeys.end(); ++it) {
//        LOGNS(Av, upnpav, debug, "omm config, application keys: " + *it + ", value: " + config().getString("application." + *it, ""));
//    }
//
//    std::vector<std::string> sysKeys;
//    config().keys("system", sysKeys);
//    for (std::vector<std::string>::iterator it = sysKeys.begin(); it != sysKeys.end(); ++it) {
//        LOGNS(Av, upnpav, debug, "omm config, system keys: " + *it + ", value: " + config().getString("system." + *it, ""));
//    }
}


void
UpnpApplication::printForm(const Poco::Net::HTMLForm& form)
{
    for (Poco::Net::NameValueCollection::ConstIterator it = form.begin(); it != form.end(); ++it)
    {
        LOGNS(Av, upnpav, debug, "form " + it->first + ": " + it->second);
    }
}


void
UpnpApplication::loadConfig()
{
    if (!_ignoreConfig) {
        LOGNS(Av, upnpav, information, "reading config file ...");

        _confFilePath = Omm::Util::Home::instance()->getConfigDirPath("/") + "omm.properties";
        _pConf = new Poco::Util::PropertyFileConfiguration;
        try {
            _pConf->load(_confFilePath);
            LOGNS(Av, upnpav, information, "reading config file done.");
        }
        catch (Poco::Exception& e) {
            LOGNS(Av, upnpav, error, "could not read config file: " + e.displayText());
        }
    //        config().addWriteable(_pConf, -200);
        config().addWriteable(_pConf, 0);
    }
    printConfig();
}


void
UpnpApplication::initConfig()
{
    setFullscreen(config().getBool("application.fullscreen", false));
    resizeMainView(config().getInt("application.width", 800), config().getInt("application.height", 480));
    scaleMainView(config().getDouble("application.scale", 1.0));
}


void
UpnpApplication::saveConfig()
{
    if (!_ignoreConfig) {
        LOGNS(Av, upnpav, information, "saving config file ...");
        _pConf->setInt("application.width", width());
        _pConf->setInt("application.height", height());
        if (!config().getBool("application.fullscreen", false)) {
            _pConf->setString("application.cluster", _pControllerWidget->getConfiguration());
        }
        try {
            _pConf->save(_confFilePath);
            LOGNS(Av, upnpav, information, "saving config file done.");
        }
        catch (Poco::Exception& e) {
            LOGNS(Av, upnpav, error, "could not write config file present" + e.displayText());
        }
    }
}



std::stringstream*
UpnpApplication::generateConfigPage()
{
    std::stringstream* pOutStream = new std::stringstream;
    *pOutStream << "<html>\n"
                    "<head>\n"
                    "<title>OMM Configuration</title>\n"
                    "</head>\n"
                    "<body>\n"
                    "<h1>OMM Configuration</h1>\n";

    *pOutStream << "Address of this page is " << getAppHttpUri() << CONFIG_URI << "<br>\n";

    if (!_pConf) {
        *pOutStream << "configuration file not found<br>\n";
        return pOutStream;
    }

    *pOutStream << "<form method=\"POST\" action=\"" + CONFIG_URI + "?" + CONFIG_APP_QUERY + "\"><br>\n";

    std::string appState = _pConf->getString("application.state", "Started");
    *pOutStream << std::string("state<select name=\"application.state\" size=\"1\">") +
                "<option " + (appState == DeviceManager::Started ? "selected" : "") + ">" + DeviceManager::Started + "</option>" +
                "<option " + (appState == DeviceManager::Local ? "selected" : "") + ">" + DeviceManager::Local + "</option>" +
                "<option " + (appState == DeviceManager::Stopped ? "selected" : "") + ">" + DeviceManager::Stopped + "</option>" +
                "</select><br>\n";

    *pOutStream << "<input type=\"submit\" value=\"Save\">\n";
    *pOutStream << "</form>\n";

    *pOutStream << "<form method=\"POST\" action=\"" + CONFIG_URI + "?" + CONFIG_DEV_QUERY + "\">\n";

    *pOutStream << "<h2>Media Renderer</h2>\n";
    bool rendererEnable = _pConf->getBool("renderer.enable", false);
    std::string rendererName = _pConf->getString("renderer.friendlyName", "");
    std::string rendererUuid = _pConf->getString("renderer.uuid", "");
    std::string rendererPlugin = _pConf->getString("renderer.plugin", "");
    *pOutStream << "<fieldset><legend>" + rendererName + "</legend>"
        "<table>"
        "<tr><td>friendly name</td><td><input type=\"text\" name=\"renderer.friendlyName\" size=\"32\" value=\"" + rendererName +  "\"></td></tr>\n"
        "<tr><td>uuid</td><td><input type=\"text\" name=\"renderer.uuid\" size=\"32\" value=\"" + rendererUuid +  "\"></td></tr>\n"
        "<tr><td>plugin</td><td><input type=\"text\" name=\"renderer.plugin\" size=\"32\" value=\"" + rendererPlugin +  "\"></td></tr>\n"
        "<tr><td>enable</td><td><input type=\"checkbox\" name=\"renderer.enable\" value=\"true\"" +  (rendererEnable ? "checked" : "") + " ></td></tr>\n"
        "</table>"
        "</fieldset>";

    *pOutStream << "<h2>Media Servers</h2>\n";
    std::string serversString = _pConf->getString("servers", "");
    Poco::StringTokenizer servers(serversString, ",");
    for (Poco::StringTokenizer::Iterator it = servers.begin(); it != servers.end(); ++it) {
        std::string serverKey = "server." + *it;
        bool serverEnable = _pConf->getBool(serverKey + ".enable", false);
        std::string serverName = _pConf->getString(serverKey + ".friendlyName", "");
        std::string serverUuid = _pConf->getString(serverKey + ".uuid", "");
        std::string serverPlugin = _pConf->getString(serverKey + ".plugin", "");
        std::string basePath = _pConf->getString(serverKey + ".basePath", "");
        std::string pollUpdateId = _pConf->getString(serverKey + ".pollUpdateId", "0");
        bool checkMod = _pConf->getBool(serverKey + ".checkMod", false);
        std::string layout = _pConf->getString(serverKey + ".layout", "");

        *pOutStream << "<fieldset><legend>" + serverName + "</legend>"
            "<table>"
            "<tr><td>enable</td><td><input type=\"hidden\" name=\"" + serverKey + ".enable\" value=\"false\"/><input type=\"checkbox\" name=\"" + serverKey + ".enable\" value=\"true\" " +  (serverEnable ? "checked" : "") + "></td></tr>\n"
            "<tr><td>friendly name</td><td><input type=\"text\" name=\"" + serverKey + ".friendlyName\" size=\"32\" value=\"" + serverName +  "\"></td></tr>\n"
            "<tr><td>uuid</td><td><input type=\"text\" name=\"" + serverKey + ".uuid\" size=\"32\" value=\"" + serverUuid +  "\"></td></tr>\n"
            "<tr><td>plugin</td><td><input type=\"text\" name=\"" + serverKey + ".plugin\" size=\"32\" value=\"" + serverPlugin +  "\"></td></tr>\n"
            "<tr><td>base path</td><td><input type=\"text\" name=\"" + serverKey + ".basePath\" size=\"32\" value=\"" + basePath +  "\"></td></tr>\n"
            "<tr><td>poll</td><td><input type=\"text\" name=\"" + serverKey + ".pollUpdateId\" size=\"10\" value=\"" + pollUpdateId +  "\"></td></tr>\n"
            "<tr><td>check mod</td><td><input type=\"hidden\" name=\"" + serverKey + ".checkMod\" value=\"false\"/><input type=\"checkbox\" name=\"" + serverKey + ".checkMod\" value=\"true\" " +  (checkMod ? "checked" : "") + "></td></tr>\n"
            "<tr><td>layout</td><td><select name=\"" + serverKey + ".layout\" size=\"1\">"
                "<option " + (layout == Av::ServerContainer::LAYOUT_FLAT ? "selected" : "") + ">" + Av::ServerContainer::LAYOUT_FLAT + "</option>"
                "<option " + (layout == Av::ServerContainer::LAYOUT_DIR_STRUCT ? "selected" : "") + ">" + Av::ServerContainer::LAYOUT_DIR_STRUCT + "</option>"
                "<option " + (layout == Av::ServerContainer::LAYOUT_PROPERTY_GROUPS ? "selected" : "") + ">" + Av::ServerContainer::LAYOUT_PROPERTY_GROUPS + "</option>"
                "</select></td></tr>\n"
            "<tr><td><input type=\"submit\" name=\"delete." + *it + "\" value=\"Delete\"></td></tr>\n"
            "</table>"
            "</fieldset><br>";
    }

    std::string newServerKey = "server.new";
    *pOutStream << "<fieldset><legend>New Server</legend>"
            "<table>"
            "<tr><td>enable</td><td><input type=\"hidden\" name=\"" + newServerKey + ".enable\" value=\"false\"/><input type=\"checkbox\" name=\"" + newServerKey + ".enable\" value=\"true\"></td></tr>\n"
            "<tr><td>friendly name</td><td><input type=\"text\" name=\"" + newServerKey + ".friendlyName\" size=\"32\" value=\"\"></td></tr>\n"
            "<tr><td>uuid</td><td><input type=\"text\" name=\"" + newServerKey + ".uuid\" size=\"32\" value=\"" + Poco::UUIDGenerator().createRandom().toString() +  "\"></td></tr>\n"
            "<tr><td>plugin</td><td><input type=\"text\" name=\"" + newServerKey + ".plugin\" size=\"32\" value=\"\"></td></tr>\n"
            "<tr><td>base path</td><td><input type=\"text\" name=\"" + newServerKey + ".basePath\" size=\"32\" value=\"\"></td></tr>\n"
            "<tr><td>poll</td><td><input type=\"text\" name=\"" + newServerKey + ".pollUpdateId\" size=\"10\" value=\"0\"></td></tr>\n"
            "<tr><td>check mod</td><td><input type=\"hidden\" name=\"" + newServerKey + ".checkMod\" value=\"false\"/><input type=\"checkbox\" name=\"" + newServerKey + ".checkMod\" value=\"true\"></td></tr>\n"
            "<tr><td>layout</td><td><select name=\"" + newServerKey + ".layout\" size=\"1\"> <option>" + Av::ServerContainer::LAYOUT_FLAT + "</option>"
                "<option>" + Av::ServerContainer::LAYOUT_DIR_STRUCT + "</option>"
                "<option>" + Av::ServerContainer::LAYOUT_PROPERTY_GROUPS + "</option></select></td></tr>\n"
            "<tr><td><input type=\"submit\" name=\"create\" value=\"New\"></td></tr>\n"
            "</table>"
            "</fieldset><br>";

    *pOutStream << "<input type=\"submit\" value=\"Save\">\n";
    *pOutStream << "</form>\n";
    *pOutStream << "</body></html>\n";

    return pOutStream;
}


void
UpnpApplication::handleAppConfigRequest(const Poco::Net::HTMLForm& form)
{
    if (!form.empty()) {
        printForm(form);
        for (Poco::Net::NameValueCollection::ConstIterator it = form.begin(); it != form.end(); ++it) {
            _pConf->setString(it->first, it->second);
            if (it->first == "application.state") {
                if (_enableController) {
                    _pControllerWidget->setState(it->second);
                }
                _pLocalDeviceServer->setState(it->second);
            }
        }
    }
}


void
UpnpApplication::handleDevConfigRequest(const Poco::Net::HTMLForm& form)
{
    if (!form.empty()) {
        LOGNS(Av, upnpav, debug, "omm local devices config update ...");
        printConfig();
        printForm(form);

        // read form data and write all entries to file configuration
        LOGNS(Av, upnpav, debug, "omm config read in devices form config ...");
        std::set<std::string> serverIds;
        std::string deleteUuid;
        bool createServer = false;
        for (Poco::Net::NameValueCollection::ConstIterator it = form.begin(); it != form.end(); ++it) {
            // NOTE: When delete is pressed, an additional form entry is generated: delete.uuid (same for create w/o uuid)
            Poco::StringTokenizer key(it->first, ".");
            if (key[0] == "delete") {
                deleteUuid = key[1];
            }
            else if (it->first == "create") {
                createServer = true;
            }
            else {
                _pConf->setString(it->first, it->second);
            }
            if (key[0] == "server") {
                serverIds.insert(key[1]);
            }
        }
        serverIds.erase(deleteUuid);
        serverIds.erase("new");

        if (createServer) {
            // find new or deleted config file id for local server.
            int id = 0;
            std::string newId = "0";
            while (serverIds.find(newId) != serverIds.end()) {
                id++;
                newId = Poco::NumberFormatter::format(id);
            }
            // copy data of new server into new server entry.
            LOGNS(Av, upnpav, debug, "omm config create new local device with id: " + newId);
            std::vector<std::string> newServerKeys;
            _pConf->keys("server.new", newServerKeys);
            for (std::vector<std::string>::iterator it = newServerKeys.begin(); it != newServerKeys.end(); ++it) {
                LOGNS(Av, upnpav, debug, std::string("add config entry ") + "server." + newId + "." + *it + ": " + _pConf->getString(*it, ""));
                _pConf->setString("server." + newId + "." + *it, _pConf->getString("server.new." + *it, ""));
            }
            serverIds.insert(newId);
        }

        std::string servers;
        for (std::set<std::string>::const_iterator it = serverIds.begin(); it != serverIds.end(); ++it) {
            servers.append(*it);
            if (it != --serverIds.end()) {
                servers.append(",");
            }
        }
        _pConf->setString("servers", servers);

        // restart local device container and all devices contained
        _pLocalDeviceServer->setState(DeviceManager::Stopped);
        _pLocalDeviceServer = new DeviceServer;
//        _pLocalDeviceServer->removeDeviceContainer(_pLocalDeviceContainer);
        // delete local devices
//        delete _pLocalDeviceContainer;
        _pLocalDeviceContainer = new DeviceContainer;
        initConfig();
        initLocalDevices();
        _pLocalDeviceServer->setState(DeviceManager::Started);
        LOGNS(Av, upnpav, debug, "omm local devices config update done.");
    }
}


void
UpnpApplication::enableController(bool enable)
{
//    if (_pControllerWidget) {
//        enable ? _pControllerWidget->start() : _pControllerWidget->stop();
//    }
    _enableController = enable;
}


void
UpnpApplication::showRendererVisualOnly(bool show)
{
//    _pControllerWidget->setTabBarHidden(!show);
//    _pControllerWidget->showOnlyRendererVisual(show);
    _showRendererVisualOnly = show;
    _mode = ModeRendererOnly;
    showControlPanels(false);
}


void
UpnpApplication::initLocalDevices()
{
    // add local renderer
    LOGNS(Av, upnpav, debug, "omm application init local devices ...");
    if (config().getBool("renderer.enable", false)) {
        setLocalRenderer(config().getString("renderer.friendlyName", "OMM Renderer"),
                config().getString("renderer.uuid", ""),
                config().getString("renderer.plugin", ""));
    }

    // add local servers from config file
    std::string serversString;
    if (_pConf) {
        serversString = _pConf->getString("servers", "");
    }
    Poco::StringTokenizer servers(serversString, ",");
    for (Poco::StringTokenizer::Iterator it = servers.begin(); it != servers.end(); ++it) {
        LOGNS(Av, upnpav, debug, "omm config, server: " + *it);
        if (_pConf->getBool("server." + *it + ".enable", false)) {
            addLocalServer(*it);
        }
    }
    // add local servers from command line
    std::vector<std::string> serverKeys;
    config().keys("server", serverKeys);
    for (std::vector<std::string>::iterator it = serverKeys.begin(); it != serverKeys.end(); ++it) {
        if (!(_pConf && _pConf->hasProperty("server." + *it + ".enable"))) {
            LOGNS(Av, upnpav, debug, "omm config, server: " + *it);
            if (config().getBool("server." + *it + ".enable", false)) {
                addLocalServer(*it);
            }
        }
    }

//#ifndef __IPHONE__
    if (_enableRenderer) {
        setLocalRenderer();
    }
    if (_enableRenderer || _enableServer) {
        _pLocalDeviceServer->addDeviceContainer(_pLocalDeviceContainer);
    }
//#endif
    _pLocalDeviceServer->init();
//#ifndef __IPHONE__
    if (_enableRenderer && _pLocalMediaRenderer) {
        // TODO: get default renderer from config file via uuid
        _pControllerWidget->setDefaultRenderer(_pLocalMediaRenderer);
    }
//#endif
   LOGNS(Av, upnpav, debug, "omm application init local devices done.");
}


void
UpnpApplication::setLocalRenderer(const std::string& name, const std::string& uuid, const std::string& pluginName)
{
    _enableRenderer = true;
    _rendererName = name;
    _rendererUuid = uuid;
    _rendererPlugin = pluginName;
}


void
UpnpApplication::setLocalRenderer()
{
    LOGNS(Av, upnpav, debug, "omm application set local renderer ...");

    Omm::Av::Engine* pEngine;
#ifdef __IPHONE__
//        pEngine = new MPMoviePlayerEngine;
#else
    Omm::Util::PluginLoader<Omm::Av::Engine> pluginLoader;
    try {
        pEngine = pluginLoader.load(_rendererPlugin);
    }
    catch(Poco::NotFoundException) {
        LOGNS(Av, upnpav, error, "could not find engine plugin: " + _rendererPlugin);
        pEngine = new VlcEngine;
    }
    LOGNS(Av, upnpav, information, "engine plugin: " + _rendererPlugin + " loaded successfully");
#endif

    pEngine->createPlayer();
    pEngine->setVisual(_pControllerWidget->getLocalRendererVisual());

    Av::MediaRenderer* pMediaRenderer = new Av::MediaRenderer;
    _pLocalMediaRenderer = pMediaRenderer;
    pMediaRenderer->addEngine(pEngine);
    Omm::Icon* pRendererIcon = new Omm::Icon(22, 22, 8, "image/png", "renderer.png");
    pMediaRenderer->addIcon(pRendererIcon);
    pMediaRenderer->setFriendlyName(_rendererName);
    pMediaRenderer->setUuid(_rendererUuid);
    if (_enableRenderer) {
        _pLocalDeviceContainer->addDevice(pMediaRenderer);
        _pLocalDeviceContainer->setRootDevice(pMediaRenderer);
    }
    else {
        if (_pLocalDeviceContainer->getDeviceCount()) {
            _pLocalDeviceContainer->setRootDevice((*_pLocalDeviceContainer->beginDevice()));
        }
    }
    LOGNS(Av, upnpav, debug, "omm application set local renderer finished.");
}


Av::MediaRenderer*
UpnpApplication::getLocalRenderer()
{
    return _pLocalMediaRenderer;
}


void
UpnpApplication::addLocalServer(const std::string& id)
{
    LOGNS(Av, upnpav, debug, "omm application add local server with id: " + id + " ...");

    std::string pluginName = config().getString("server." + id + ".plugin", "model-webradio");
    Omm::Av::AbstractDataModel* pDataModel;
    Omm::Util::PluginLoader<Omm::Av::AbstractDataModel> pluginLoader;
    try {
        pDataModel = pluginLoader.load(pluginName);
    }
    catch(Poco::NotFoundException) {
        LOGNS(Av, upnpav, error, "could not find server plugin: " + pluginName);
        return;
    }
    LOGNS(Av, upnpav, information, "container plugin: " + pluginName + " loaded successfully");

    pDataModel->setCacheDirPath(Util::Home::instance()->getCacheDirPath());
    pDataModel->setMetaDirPath(Util::Home::instance()->getMetaDirPath());
    pDataModel->setTextEncoding(config().getString("server." + id + ".textEncoding", "UTF8"));
    pDataModel->setCheckObjectModifications(config().getBool("server." + id + ".checkMod", false));

    _enableServer = true;
    Omm::Av::MediaServer* pMediaServer = new Av::MediaServer;

    std::string name = config().getString("server." + id + ".friendlyName", "OMM Server");
    Omm::Av::ServerContainer* pContainer = new Av::ServerContainer(pMediaServer);
    pContainer->setTitle(name);
    pContainer->setClass(Omm::Av::AvClass::className(Omm::Av::AvClass::CONTAINER));
    pContainer->setDataModel(pDataModel);
    std::string basePath = config().getString("server." + id + ".basePath", "webradio.conf");
    std::string path = basePath;
    // if relative path then search file in config directory
    if (basePath[0] != '/') {
        path = Omm::Util::Home::instance()->getConfigDirPath("/") + basePath;
    }
    pContainer->setBasePath(path);
    pContainer->setLayout(config().getString("server." + id + ".layout", "Flat"));

    pMediaServer->setRoot(pContainer);
    pMediaServer->setFriendlyName(name);
    pMediaServer->setUuid(config().getString("server." + id + ".uuid", ""));
    Omm::Icon* pIcon = new Omm::Icon(32, 32, 8, "image/png", "server.png");
    pMediaServer->addIcon(pIcon);
    pMediaServer->setPollSystemUpdateIdTimer(config().getInt("server." + id + ".pollUpdateId", 0));

    _pLocalDeviceContainer->addDevice(pMediaServer);
    if (!_enableRenderer) {
        _pLocalDeviceContainer->setRootDevice(pMediaServer);
    }

    LOGNS(Av, upnpav, debug, "omm application add local server finished.");
}


void
UpnpApplication::startAppHttpServer()
{
    int port = config().getInt("application.configPort", _appStandardPort);
    bool useRandomPort = true;
    if (!instanceAlreadyRunning()) {
        try {
            _socket = Poco::Net::ServerSocket(port);
            useRandomPort = false;
        }
        catch (Poco::Exception& e) {
            LOG(upnp, error, "failed to start application http server on port " + Poco::NumberFormatter::format(port) + "(" + e.displayText() + ") , using random port.");
        }
    }
    if (useRandomPort) {
        try {
            _socket = Poco::Net::ServerSocket(0);
        }
        catch (Poco::Exception& e) {
            LOG(upnp, error, "failed to start application http server: " + e.displayText());
        }
    }
    Poco::Net::HTTPServerParams* pParams = new Poco::Net::HTTPServerParams;
    _pHttpServer = new Poco::Net::HTTPServer(new ConfigRequestHandlerFactory(this), _socket, pParams);
    _pHttpServer->start();
    LOG(upnp, information, "omm application http server listening on: " + _socket.address().toString());
}


void
UpnpApplication::stopAppHttpServer()
{
    _pHttpServer->stop();
    LOG(upnp, information, "omm application http server stopped on port: " + Poco::NumberFormatter::format(_socket.address().port()));
}


bool
UpnpApplication::instanceAlreadyRunning()
{
#ifdef __IPHONE__
    return false;
#endif

    if (_lockInstance) {
        _lockInstance = false;
        Poco::NamedMutex mutex(_instanceMutexName);
        return !mutex.tryLock();
    }
    else {
        return false;
    }
}


std::string
UpnpApplication::getAppHttpUri()
{
    std::string address = Net::NetworkInterfaceManager::instance()->getValidIpAddress().toString();
    return "http://" + address + ":" + Poco::NumberFormatter::format(_socket.address().port());
}


std::string
UpnpApplication::getConfigHttpUri()
{
    // FIXME: on second process instance, configuration is ignored, but config page of first process instance
    // should be displayed. If standard port is used, this works, but not if another port is configured.
    // However, getting the configured port doesn't solve the problem, as it may be in use when first instance was started,
    // and so first instance chose a random port.
    return "http://localhost:" + config().getString("application.configPort", Poco::NumberFormatter::format(_appStandardPort)) + "/Config";
//    return "http://localhost:" + config().getString("application.configPort", Poco::NumberFormatter::format(_socket.address().port())) + "/Config";
//    return "http://localhost:" + Poco::NumberFormatter::format(_socket.address().port()) + "/Config";
}


std::string
UpnpApplication::getMode()
{
    return _mode;
}


} // namespace Omm
