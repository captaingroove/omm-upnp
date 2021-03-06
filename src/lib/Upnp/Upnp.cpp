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

#include <iostream>
#include <fstream>
#include <sstream>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/Message.h>
#include <Poco/NObserver.h>
#include <Poco/StreamCopier.h>
#include <Poco/LineEndingConverter.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/URI.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/NetworkInterface.h>
#include <Poco/Net/MessageHeader.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/MediaType.h>
#include <Poco/DOM/Node.h>
#include <Poco/NumberParser.h>
#include <Poco/Timer.h>

#include "Upnp.h"
#include "UpnpPrivate.h"

namespace Omm {

#ifndef NDEBUG
Log* Log::_pInstance = 0;

// possible log levels: trace, debug, information, notice, warning, error, critical, fatal

Log::Log()
{
    Poco::Channel* pChannel = Util::Log::instance()->channel();

    _pUpnpLogger = &Poco::Logger::create("UPNP.GENERAL", pChannel, Poco::Message::PRIO_DEBUG);
    _pSsdpLogger = &Poco::Logger::create("UPNP.SSDP", pChannel, Poco::Message::PRIO_DEBUG);
    _pHttpLogger = &Poco::Logger::create("UPNP.HTTP", pChannel, Poco::Message::PRIO_DEBUG);
    _pDescriptionLogger = &Poco::Logger::create("UPNP.DESC", pChannel, Poco::Message::PRIO_DEBUG);
    _pControlLogger = &Poco::Logger::create("UPNP.CONTROL", pChannel, Poco::Message::PRIO_DEBUG);
    _pEventLogger = &Poco::Logger::create("UPNP.EVENT", pChannel, Poco::Message::PRIO_DEBUG);

//    _pUpnpLogger = &Poco::Logger::create("UPNP.GENERAL", pChannel, Poco::Message::PRIO_ERROR);
//    _pSsdpLogger = &Poco::Logger::create("UPNP.SSDP", pChannel, Poco::Message::PRIO_ERROR);
//    _pHttpLogger = &Poco::Logger::create("UPNP.HTTP", pChannel, Poco::Message::PRIO_ERROR);
//    _pDescriptionLogger = &Poco::Logger::create("UPNP.DESC", pChannel, Poco::Message::PRIO_ERROR);
//    _pControlLogger = &Poco::Logger::create("UPNP.CONTROL", pChannel, Poco::Message::PRIO_ERROR);
//    _pEventLogger = &Poco::Logger::create("UPNP.EVENT", pChannel, Poco::Message::PRIO_ERROR);
}


Log*
Log::instance()
{
    if (!_pInstance) {
        _pInstance = new Log;
    }
    return _pInstance;
}


Poco::Logger&
Log::upnp()
{
    return *_pUpnpLogger;
}


Poco::Logger&
Log::ssdp()
{
    return *_pSsdpLogger;
}


Poco::Logger&
Log::http()
{
    return *_pHttpLogger;
}


Poco::Logger&
Log::desc()
{
    return *_pDescriptionLogger;
}


Poco::Logger&
Log::ctrl()
{
    return *_pControlLogger;
}


Poco::Logger&
Log::event()
{
    return *_pEventLogger;
}
#endif // NDEBUG


Icon::Icon(int width, int height, int depth, const std::string& mime, const std::string& uri) :
_width(width),
_height(height),
_depth(depth),
_mime(mime),
_requestPath(""),
_searchPath(":/usr/lib/omm:/usr/local/lib/omm")
{
    LOG(upnp, debug, "search default icon path and OMM_ICON_PATH for icon: " + uri);
    try {
        _searchPath = Poco::Environment::get("OMM_ICON_PATH") + _searchPath;
    }
    catch (Poco::NotFoundException) {
    }

    if (uri != "") {
        try {
            retrieve(uri);
        }
        catch (Poco::NotFoundException) {
            LOG(upnp, error, "failed to retrieve icon from: " + uri);
        }
    }
}


Icon::~Icon()
{
    _buffer.clear();
}


const std::string&
Icon::getBuffer()
{
    return _buffer;
}


std::string
Icon::getIconRequestPath()
{
    return _requestPath;
}


void
Icon::setIconRequestPath(const std::string& path)
{
    _requestPath = path;
}


void
Icon::retrieve(const std::string& uri)
{
    LOG(upnp, debug, "retrieve icon: " + uri);
    Poco::URI iconUri(uri);

    if (iconUri.getScheme() == "file") {
        Poco::File f(iconUri.getPath());
        if (!f.exists()) {
            throw Poco::NotFoundException();
        }
        LOG(upnp, debug, "reading icon from file: " + iconUri.getPath());
        std::ifstream ifs(iconUri.getPath().c_str());
        char* pData = new char[f.getSize()];
        ifs.read(pData, f.getSize());
        _buffer = std::string(pData, f.getSize());

    }
    else if (iconUri.getScheme() == "http") {
        LOG(upnp, debug, "download icon: " + uri);
        Poco::Net::HTTPStreamFactory streamOpener;

        _buffer.clear();
        std::size_t size;
        try {
            std::istream* pInStream = streamOpener.open(Poco::URI(uri));
            if (pInStream) {
                size = Poco::StreamCopier::copyToString(*pInStream, _buffer);
            }
            delete pInStream;
        }
        catch (Poco::Exception& e) {
            LOG(upnp, error, "download icon failed: " + e.displayText());
            return;
        }
        if (size == 0) {
            LOG(upnp, error, "download icon failed, no bytes received.");
            throw Poco::NotFoundException();
        }
        else {
            LOG(upnp, debug, "download icon success, bytes: " + Poco::NumberFormatter::format(size));
        }
    }
    else if (iconUri.isRelative()) {
        Poco::StringTokenizer pathSplitter(_searchPath, ":");
        Poco::StringTokenizer::Iterator it;
        for (it = pathSplitter.begin(); it != pathSplitter.end(); ++it) {
            if (*it == "") {
                continue;
            }
            Poco::URI baseUri(*it + "/");
            baseUri.resolve(iconUri);
            LOG(upnp, debug, "try to read icon from file: " + baseUri.getPath());
            Poco::File f(baseUri.getPath());
            if (!f.exists()) {
                continue;
            }
            LOG(upnp, debug, "reading icon from file: " + baseUri.getPath());
            std::ifstream ifs(baseUri.getPath().c_str());
            char* pData = new char[f.getSize()];
            ifs.read(pData, f.getSize());
            _buffer = std::string(pData, f.getSize());
            break;
        }
        if (it == pathSplitter.end()) {
            throw Poco::NotFoundException();
        }
    }
}


Urn::Urn(const std::string& urn) :
_urn(urn)
{
    std::vector<std::string> vec;
    std::string::size_type left = 0;
    std::string::size_type right = 0;
    do
    {
        left = _urn.find(':', right);
        right = _urn.find(':', ++left);
        vec.push_back(_urn.substr(left, right - left));
    } while (right != std::string::npos);
    _domainName = vec[0];
    _type = vec[1];
    _typeName = vec[2];
    _version = vec[3];
}


SsdpSocket::SsdpSocket() :
_mode(NotConfigured),
_pSsdpListenerSocket(0),
_pSsdpSenderSocket(0),
_pMulticastReactor(0),
_pMulticastListenerThread(0)
//_pSsdpBusListenerSocket(0),
//_pSsdpBusSenderSocket(0),
//_pBusMessageReactor(0),
//_pBusMessageListenerThread(0)
{
}


SsdpSocket::~SsdpSocket()
{
}


void
SsdpSocket::init()
{
    _mode = NotConfigured;
    _pBuffer = new char[BUFFER_SIZE];
    // listen to UDP unicast and send out to multicast
    LOG(ssdp, debug, "create multicast socket");
    _pSsdpSenderSocket = new Poco::Net::MulticastSocket(Poco::Net::SocketAddress("0.0.0.0", SSDP_PORT), true);

    // listen to UDP multicast
    LOG(ssdp, debug, "create listener socket ...");
    _pSsdpListenerSocket = new Poco::Net::MulticastSocket(Poco::Net::SocketAddress("0.0.0.0", SSDP_PORT), true);

//    LOG(ssdp, debug, "create bus socket");
//    _pSsdpBusSenderSocket = new Poco::Net::DatagramSocket;

//    // listen to UDP connection to ommbus
//    LOG(ssdp, debug, "create bus listener socket ...");
//    _pSsdpBusListenerSocket = new Poco::Net::DatagramSocket(Poco::Net::SocketAddress("0.0.0.0", 0), false);

    setupSockets();
}


//void
//SsdpSocket::deinit()
//{
//    delete _pSsdpListenerSocket;
//    delete _pSsdpLocalSenderSocket;
//    delete _pSsdpLocalListenerSocket;
//    delete _pBuffer;
//    _mode = NotConfigured;
//}


//void
//SsdpSocket::addInterface(const std::string& name)
//{
//    LOG(ssdp, information, "add interface: " + name);
////    setupSockets();
//}


//void
//SsdpSocket::removeInterface(const std::string& name)
//{
//    LOG(ssdp, information, "remove interface: " + name);
////    resetSockets();
////    deinit();
////    init();
////    setupSockets();
//}


void
SsdpSocket::addObserver(const Poco::AbstractObserver& observer)
{
    _ssdpSocketNotificationCenter.addObserver(observer);
}


void
SsdpSocket::startListen()
{
    if (_mode == NotConfigured) {
        LOG(ssdp, error, "failed to start SSDP socket, not configured.");
        return;
    }

    if ((_mode & Multicast) && !_pMulticastListenerThread) {
        LOG(ssdp, information, "starting SSDP multicast listener ...");
        _pMulticastListenerThread = new Poco::Thread;
        _pMulticastReactor = new Poco::Net::SocketReactor;
        _pMulticastReactor->addEventHandler(*_pSsdpListenerSocket, Poco::Observer<SsdpSocket, 
                Poco::Net::ReadableNotification>(*this, &SsdpSocket::onListenerMulticastSsdpMessage));
        _pMulticastReactor->addEventHandler(*_pSsdpSenderSocket, Poco::Observer<SsdpSocket, 
                Poco::Net::ReadableNotification>(*this, &SsdpSocket::onSenderMulticastSsdpMessage));
        _pMulticastListenerThread->start(*_pMulticastReactor);
    }
    
//    if ((_mode & Bus) && !_pBusMessageListenerThread) {
//        LOG(ssdp, information, "starting SSDP bus listener ...");
//        _pBusMessageListenerThread = new Poco::Thread;
//        _pBusMessageReactor = new Poco::Net::SocketReactor;
//        _pBusMessageReactor->addEventHandler(*_pSsdpBusListenerSocket, Poco::Observer<SsdpSocket, Poco::Net::ReadableNotification>(*this, &SsdpSocket::onListenerMulticastSsdpMessage));
//        _pBusMessageReactor->addEventHandler(*_pSsdpBusSenderSocket, Poco::Observer<SsdpSocket, Poco::Net::ReadableNotification>(*this, &SsdpSocket::onSenderMulticastSsdpMessage));
//        _pBusMessageListenerThread->start(*_pBusMessageReactor);
//    }
    
    if (_mode & LocalProcess) {
        LOG(ssdp, information, "starting SSDP local listener ...");
        Poco::NotificationCenter::defaultCenter().addObserver(Poco::NObserver<SsdpSocket, SsdpMessage>(*this, &SsdpSocket::onLocalSsdpMessage));
    }

    LOG(ssdp, information, "SSDP listener started.");
}


void
SsdpSocket::stopListen()
{
    if (_mode == NotConfigured) {
        LOG(ssdp, error, "failed to stop SSDP socket, not configured.");
        return;
    }

    if (_pMulticastListenerThread) {
        LOG(ssdp, information, "stopping SSDP multicast listener ...");
        _pMulticastReactor->stop();
        _pMulticastListenerThread->join();
        delete _pMulticastReactor;
        _pMulticastReactor = 0;
        delete _pMulticastListenerThread;
        _pMulticastListenerThread = 0;
    }

    if (_mode & LocalProcess) {
        LOG(ssdp, information, "stopping SSDP local listener ...");
        Poco::NotificationCenter::defaultCenter().removeObserver(Poco::NObserver<SsdpSocket, SsdpMessage>(*this, &SsdpSocket::onLocalSsdpMessage));
    }

    LOG(ssdp, information, "SSDP listener stopped.");
}


void
SsdpSocket::sendMessage(const Poco::AutoPtr<SsdpMessage>& pMessage, const Poco::Net::SocketAddress& receiver) const
{
    std::string m = pMessage->toString();

    int bytesSent = 0;
    LOG(ssdp, debug, "sending SSDP message to address: " + receiver.toString() + " ..." + Poco::LineEnding::NEWLINE_DEFAULT + m);
    try {
        if (_mode & Multicast) {
            LOG(ssdp, debug, "sending SSDP message through multicast socket ...");
            bytesSent = _pSsdpSenderSocket->sendTo(m.c_str(), m.length(), receiver);
            LOG(ssdp, debug, "SSDP message with " + Poco::NumberFormatter::format(bytesSent) + 
                    " bytes sent through multicast socket.");
        }
//        if (_mode & Bus) {
//            LOG(ssdp, debug, "sending SSDP message through bus socket ...");
//            bytesSent = _pSsdpBusSenderSocket->sendTo(m.c_str(), m.length(), receiver);
//        }
        if (_mode & LocalProcess) {
            LOG(ssdp, debug, "sending SSDP message through default notification center ...");
            Poco::NotificationCenter::defaultCenter().postNotification(pMessage);
            LOG(ssdp, debug, "SSDP message sent through default notification center.");
        }
    }
    catch(Poco::Net::NetException& e) {
        LOG(ssdp, error, "sending SSDP message failed: " + e.message());
    }
}


void
SsdpSocket::setupSockets()
{
    LOG(ssdp, debug, "setting up SSDP sockets ...");

    // start with local mode
    setMode(LocalProcess);
    
//    setMode(Bus);
//    LOG(ssdp, debug, "setting up SSDP sockets in bus mode finished.");
//    return;
    
    try {
        _pSsdpListenerSocket->joinGroup(Poco::Net::IPAddress(SSDP_ADDRESS));
    }
    catch(Poco::Exception& e) {
        LOG(ssdp, error, "failed to join multicast group: " + e.message());
        LOG(ssdp, warning, "MULTICAST socket option and route to multicast address on loopback interface probably need to be set.");
        LOG(ssdp, warning, "as superuser do something like \"ifconfig lo multicast; route add 239.255.255.250 lo\".");
        LOG(ssdp, warning, "switching to non-standard compliant local mode (non-network, non-ipc).");
        return;
    }
    // set TTL on windows raises Poco::InvalidArgumentException.
    try {
        _pSsdpSenderSocket->setTimeToLive(4);
    }
    catch(Poco::Exception& e) {
        LOG(ssdp, error, "failed to set TTL of SSDP socket: " + e.message());
    }
    // switch to multicast was succesfull, so we turn on multicast flag.
    setMode(Multicast);
    LOG(ssdp, debug, "setting up SSDP sockets in multicast mode finished.");


//#ifndef __DARWIN__
//    if (Net::NetworkInterfaceManager::instance()->loopbackOnly()) {
//        LOG(ssdp, warning, "loopback is the only network interface, forcing broadcast mode.");
//        setBroadcast();
//    }
//    else
//#endif
//    {
//        if (_mode != Multicast) {
//            try {
//                LOG(ssdp, debug, "set ttl to 4");
//                // FIXME: setting TTL on windows segfaults
//    //            _pSsdpSenderSocket->setTimeToLive(4);  // TODO: let TTL be configurable
//                setMulticast();
//            }
//            catch (Poco::IOException) {
//                LOG(ssdp, error, "failed to join multicast group");
//                LOG(ssdp, warning, "MULTICAST socket option and route to multicast address on loopback interface probably need to be set.");
//                LOG(ssdp, warning, "as superuser do something like \"ifconfig lo multicast; route add 239.255.255.250 lo\".");
//                LOG(ssdp, warning, "switching to non-standard compliant broadcast mode for loopback interface.");
//                setBroadcast();
//            }
//        }
//    }
}


//void
//SsdpSocket::resetSockets()
//{
//    LOG(ssdp, debug, "reset SSDP sockets ...");
//    if (_mode == Multicast) {
//        LOG(ssdp, debug, "leave multicast group ...");
//        _pSsdpListenerSocket->leaveGroup(Poco::Net::IPAddress(SSDP_ADDRESS));
//    }
//    else if (_mode == Broadcast) {
//        LOG(ssdp, debug, "disable broadcast mode ...");
//        _pSsdpLocalSenderSocket->setBroadcast(false);
//    }
//    _mode = NotConfigured;
//}


void
SsdpSocket::setMode(unsigned int mode)
{
    if (mode == NotConfigured) {
        LOG(ssdp, debug, "set SSDP socket mode to not configured");
    }
//    else if (mode == Bus) {
//        LOG(ssdp, debug, "set SSDP socket mode to bus");
//    }
    else if (mode == LocalProcess) {
        LOG(ssdp, debug, "set SSDP socket mode to local process");
    }
    else if (mode == Multicast) {
        LOG(ssdp, debug, "set SSDP socket mode to multicast");
    }
    else if (mode & (LocalProcess | Multicast)) {
        LOG(ssdp, debug, "set SSDP socket mode to local process and multicast");
    }
    else {
        LOG(ssdp, error, "set SSDP socket mode to unkown mode, ignoring");
        return;
    }
    _mode = mode;
}


void
SsdpSocket::onListenerMulticastSsdpMessage(Poco::Net::ReadableNotification* pNotification)
{
    Poco::Net::SocketAddress sender;
    int n = _pSsdpListenerSocket->receiveFrom(_pBuffer, BUFFER_SIZE, sender);
    if (n > 0) {
        LOG(ssdp, debug, "received message from: " + sender.toString() + "" + Poco::LineEnding::NEWLINE_DEFAULT + std::string(_pBuffer, n));
        _ssdpSocketNotificationCenter.postNotification(new SsdpMessage(std::string(_pBuffer, n), sender));
    }
}


void
SsdpSocket::onSenderMulticastSsdpMessage(Poco::Net::ReadableNotification* pNotification)
{
    Poco::Net::SocketAddress sender;
    int n = _pSsdpSenderSocket->receiveFrom(_pBuffer, BUFFER_SIZE, sender);
    if (n > 0) {
        LOG(ssdp, debug, "received message from: " + sender.toString() + "" + Poco::LineEnding::NEWLINE_DEFAULT + std::string(_pBuffer, n));
        _ssdpSocketNotificationCenter.postNotification(new SsdpMessage(std::string(_pBuffer, n), sender));
    }
}


void
SsdpSocket::onLocalSsdpMessage(const Poco::AutoPtr<SsdpMessage>& pMessage)
{
    LOG(ssdp, debug, "received message through default notification center ");
    _ssdpSocketNotificationCenter.postNotification(pMessage);
}


//SsdpBus::SsdpBus() :
//_pMessageReactor(0),
//_pMessageListenerThread(0),
//_pSsdpListenerSocket(0),
//_pSsdpSenderSocket(0),
//_pBuffer(new char[BUFFER_SIZE]),
//_port(SSDP_BUS_PORT)
//{
//}


//SsdpBus::~SsdpBus()
//{
//}


//void
//SsdpBus::start()
//{
//    if (!_pMessageListenerThread) {
//        LOG(ssdp, information, "starting SSDP bus listener ...");
//        _pSsdpListenerSocket = new Poco::Net::DatagramSocket;
//        _pSsdpListenerSocket->bind(Poco::Net::SocketAddress("0.0.0.0", _port), true);
//        _pSsdpSenderSocket = new Poco::Net::DatagramSocket;
//        _pMessageListenerThread = new Poco::Thread;
//        _pMessageReactor = new Poco::Net::SocketReactor;
//        _pMessageReactor->addEventHandler(*_pSsdpListenerSocket, Poco::Observer<SsdpBus, Poco::Net::ReadableNotification>(*this, &SsdpBus::onListenerBusMessage));
////    _pMessageReactor->addEventHandler(*_pSsdpSenderSocket, Poco::Observer<SsdpSocket, Poco::Net::ReadableNotification>(*this, &SsdpSocket::onSenderMulticastSsdpMessage));
//        _pMessageListenerThread->start(*_pMessageReactor);
//    }
//}


//void
//SsdpBus::stop()
//{
//    if (_pMessageListenerThread) {
//        LOG(ssdp, information, "stopping SSDP bus listener ...");
//        _pMessageReactor->stop();
//        _pMessageListenerThread->join();
//        delete _pMessageReactor;
//        _pMessageReactor = 0;
//        delete _pMessageListenerThread;
//        _pMessageListenerThread = 0;
//    }

//    delete _pSsdpListenerSocket;
//}


//void
//SsdpBus::onListenerBusMessage(Poco::Net::ReadableNotification* pNotification)
//{
//    Poco::Net::SocketAddress sender;
//    int n = _pSsdpListenerSocket->receiveFrom(_pBuffer, BUFFER_SIZE, sender);
//    if (n > 0) {
//        LOG(ssdp, debug, "received message from: " + sender.toString() + Poco::LineEnding::NEWLINE_DEFAULT + std::string(_pBuffer, n));
//        _pSsdpSenderSocket->connect(sender);
//        LOG(ssdp, debug, "connected to: " + sender.toString());
////        int bytesSent = _pSsdpSenderSocket->sendTo(_pBuffer, n, sender);
//        int bytesSent = 0;
//        while (bytesSent < n) {
//            bytesSent += _pSsdpSenderSocket->sendBytes(_pBuffer + bytesSent, n - bytesSent);
//        }
//        LOG(ssdp, debug, "sent " + Poco::NumberFormatter::format(bytesSent) + " byte(s) to: " + sender.toString() + Poco::LineEnding::NEWLINE_DEFAULT + std::string(_pBuffer, n));
//    }
//}


DescriptionReader::DescriptionReader()
{
}


DescriptionReader::~DescriptionReader()
{
    releaseDescriptionDocument();
}


void
DescriptionReader::getDeviceDescription(const std::string& deviceDescriptionUri)
{
    _deviceDescriptionUri = deviceDescriptionUri;
    parseDescription(retrieveDescription(deviceDescriptionUri));
}


DeviceContainer*
DescriptionReader::deviceContainer()
{
    LOG(desc, debug, "parsing device container ...");
    DeviceContainer* pRes = new DeviceContainer;
    Poco::XML::Node* pNode = _pDocStack.top()->documentElement()->firstChild();
    bool deviceNodeFound = false;
    while (pNode)
    {
        if (pNode->nodeName() == "device" && pNode->hasChildNodes()) {
            deviceNodeFound = true;
            // device must be created and added to device container first, so that
            // service types in device container are populated.
            Device* pRootDevice = new Device;
            // parse device, which must be the root device at this place in description
            // and add the generated device tree to our Device.
            DeviceData* pDeviceData = deviceData(pNode->firstChild(), pRes);
            pRootDevice->setDeviceData(pDeviceData);
            pRes->addDevice(pRootDevice);
            pRes->setRootDevice(pRootDevice);
        }
        pNode = pNode->nextSibling();
    }
    if (!deviceNodeFound) {
        LOG(desc, error, "file does not contain a UPnP device description.");
    }
    return pRes;
}


DeviceData*
DescriptionReader::rootDeviceData()
{
    LOG(desc, debug, "parsing root device ...");
    DeviceData* pRes = 0;
    Poco::XML::Node* pNode = _pDocStack.top()->documentElement()->firstChild();
    bool deviceNodeFound = false;
    while (pNode)
    {
        if (pNode->nodeName() == "device" && pNode->hasChildNodes()) {
            deviceNodeFound = true;
            pRes = deviceData(pNode->firstChild(), 0);
        }
        pNode = pNode->nextSibling();
    }
    if (!deviceNodeFound) {
        LOG(desc, error, "file does not contain a UPnP root device.");
    }
    LOG(desc, debug, "parsing root device finished.");
    return pRes;
}


void
DescriptionReader::parseDescription(const std::string& description)
{
    Poco::XML::DOMParser parser;
#if (POCO_VERSION & 0xFFFFFFFF) < 0x01040000
    parser.setFeature(Poco::XML::DOMParser::FEATURE_WHITESPACE, false);
#else
    parser.setFeature(Poco::XML::DOMParser::FEATURE_FILTER_WHITESPACE, true);
#endif
//    LOG(desc, debug, "parsing description:" + Poco::LineEnding::NEWLINE_DEFAULT + description);
    _pDocStack.push(parser.parseString(description.substr(0, description.rfind('>') + 1)));
//    LOG(desc, debug, "parsing description finished.");
}


void
DescriptionReader::releaseDescriptionDocument()
{
     if (!_pDocStack.empty()) {
         _pDocStack.top()->release();
         _pDocStack.pop();
     }
}


DeviceData*
DescriptionReader::deviceData(Poco::XML::Node* pNode, DeviceContainer* pDeviceContainer)
{
    LOG(desc, debug, "parsing device ...");
    DeviceData* pRes = new DeviceData();

    while (pNode)
    {
        if (pNode->nodeName() == "deviceType") {
            pRes->setDeviceType(pNode->innerText());
        }
        else if (pNode->nodeName() == "UDN") {
            pRes->setUuid(pNode->innerText().substr(5));
        }
        else if (pNode->nodeName() == "serviceList") {
            if (pNode->hasChildNodes()) {
                Poco::XML::Node* pChild = pNode->firstChild();
                while (pChild) {
                    if (pChild->nodeName() == "service") {
                        if (pChild->hasChildNodes()) {
                            pRes->addService(service(pChild->firstChild()));
                        }
                        else {
                            LOG(desc, error, "empty service");
                        }
                    }
                    pChild = pChild->nextSibling();
                }
            }
            else {
                LOG(desc, error, "service list without services");
            }
        }
        else if (pDeviceContainer && pNode->nodeName() == "deviceList") {
            if (pNode->hasChildNodes()) {
                Poco::XML::Node* pChild = pNode->firstChild();
                while (pChild) {
                    if (pChild->nodeName() == "device") {
                        if (pChild->hasChildNodes()) {
                            Device* pSubDevice = new Device;
                            DeviceData* pDeviceData = deviceData(pChild->firstChild(), pDeviceContainer);
                            pSubDevice->setDeviceData(pDeviceData);
                            pDeviceContainer->addDevice(pSubDevice);
                        }
                        else {
                            LOG(desc, error, "empty embedded device");
                        }
                    }
                    pChild = pChild->nextSibling();
                }
            }
            else {
                LOG(desc, error, "device list without embedded devices");
            }
        }
        else if (pNode->nodeName() == "iconList") {
            // FIXME: exception under windows with icon support
            if (pNode->hasChildNodes()) {
                Poco::XML::Node* pChild = pNode->firstChild();
                while (pChild) {
                    if (pChild->nodeName() == "icon") {
                        int width = 0;
                        int height = 0;
                        int depth = 0;
                        std::string mime;
                        std::string uri;
                        Poco::URI targetUri(_deviceDescriptionUri);
                        if (pChild->hasChildNodes()) {
                            Poco::XML::Node* pIconAttr = pChild->firstChild();
                            while (pIconAttr) {
                                if (pIconAttr->nodeName() == "mimetype") {
                                    mime = pIconAttr->innerText();
                                }
                                else if (pIconAttr->nodeName() == "width") {
                                    width = Poco::NumberParser::parse(pIconAttr->innerText());
                                }
                                else if (pIconAttr->nodeName() == "height") {
                                    height = Poco::NumberParser::parse(pIconAttr->innerText());
                                }
                                else if (pIconAttr->nodeName() == "depth") {
                                    depth = Poco::NumberParser::parse(pIconAttr->innerText());
                                }
                                else if (pIconAttr->nodeName() == "url") {
                                    targetUri.resolve(pIconAttr->innerText());
                                    uri = targetUri.toString();
                                }
                                pIconAttr = pIconAttr->nextSibling();
                            }
                        Icon* pIcon = new Icon(width, height, depth, mime, uri);
                        pRes->addIcon(pIcon);
                        }
                        else {
                            LOG(desc, error, "empty icon specification");
                        }
                    }
                    pChild = pChild->nextSibling();
                }
            }
        }
        else {
            pRes->addProperty(pNode->nodeName(), pNode->innerText());
        }
        pNode = pNode->nextSibling();
    }
    LOG(desc, debug, "parsing device finished.");
    return pRes;
}


Service*
DescriptionReader::service(Poco::XML::Node* pNode)
{
    Service* pRes = new Service();

    while (pNode)
    {
        if (pNode->nodeName() == "serviceType" && pNode->hasChildNodes()) {
            pRes->setServiceTypeFullString(pNode->innerText());
        }
        if (pNode->nodeName() == "serviceId" && pNode->hasChildNodes()) {
            pRes->setServiceId(pNode->innerText());
        }
        else if (pNode->nodeName() == "SCPDURL" && pNode->hasChildNodes()) {
            std::string descriptionPath = pNode->innerText();
            pRes->setDescriptionPath(descriptionPath);
            // load the service description into the Service object.
            pRes->setServiceDescription(retrieveDescription(descriptionPath));
            parseDescription(*pRes->getDescription());
            Poco::XML::Node* pScpd = _pDocStack.top()->documentElement()->firstChild();
            while (pScpd) {
                if (pScpd->nodeName() == "actionList" && pScpd->hasChildNodes()) {
                    Poco::XML::Node* pChild = pScpd->firstChild();
                    while (pChild) {
                        if (pChild->nodeName() == "action") {
                            pRes->addAction(action(pChild->firstChild()));
                        }
                        pChild = pChild->nextSibling();
                    }
                }
                else if (pScpd->nodeName() == "serviceStateTable" && pScpd->hasChildNodes()) {
                    Poco::XML::Node* pChild = pScpd->firstChild();
                    while (pChild) {
                        if (pChild->nodeName() == "stateVariable") {
                            StateVar* pStateVar = stateVar(pChild->firstChild());
                            if (pChild->hasAttributes()) {
                                Poco::XML::NamedNodeMap* attr = pChild->attributes();
                                pStateVar->setSendEvents(attr->getNamedItem("sendEvents")->nodeValue());
                                attr->release();
                            }
                            pRes->addStateVar(pStateVar);
                        }
                        pChild = pChild->nextSibling();
                    }
                }
                pScpd = pScpd->nextSibling();
            }
            releaseDescriptionDocument();
        }
        else if (pNode->nodeName() == "controlURL" && pNode->hasChildNodes()) {
            std::string controlPath = pNode->innerText();
            pRes->setControlPath(controlPath);
        }
        else if (pNode->nodeName() == "eventSubURL" && pNode->hasChildNodes()) {
            std::string eventPath = pNode->innerText();
            pRes->setEventSubscriptionPath(eventPath);
        }

        pNode = pNode->nextSibling();
    }
    return pRes;
}


Action*
DescriptionReader::action(Poco::XML::Node* pNode)
{
    Action* pRes = new Action();

    while (pNode)
    {
        if (pNode->nodeName() == "name" && pNode->hasChildNodes()) {
            pRes->setName(pNode->innerText());
        }
        else if (pNode->nodeName() == "argumentList" && pNode->hasChildNodes()) {
            Poco::XML::Node* pChild = pNode->firstChild();
            while (pChild) {
                if (pChild->nodeName() == "argument") {
                    pRes->appendArgument(argument(pChild->firstChild()));
                }
                pChild = pChild->nextSibling();
            }
        }
        pNode = pNode->nextSibling();
    }
    return pRes;
}


Argument*
DescriptionReader::argument(Poco::XML::Node* pNode)
{
    Argument* pRes = new Argument();

    while (pNode)
    {
        if (pNode->nodeName() == "name" && pNode->hasChildNodes()) {
            pRes->setName(pNode->innerText());
        }
        else if (pNode->nodeName() == "relatedStateVariable" && pNode->hasChildNodes()) {
            pRes->setRelatedStateVar(pNode->innerText());
        }
        else if (pNode->nodeName() == "direction" && pNode->hasChildNodes()) {
            pRes->setDirection(pNode->innerText());
        }

        pNode = pNode->nextSibling();
    }

    return pRes;
}


StateVar*
DescriptionReader::stateVar(Poco::XML::Node* pNode)
{
    StateVar* pRes = new StateVar();

    while (pNode)
    {
        if (pNode->nodeName() == "name" && pNode->hasChildNodes()) {
            pRes->setName(pNode->innerText());
        }
        else if (pNode->nodeName() == "dataType" && pNode->hasChildNodes()) {
            pRes->setType(pNode->innerText());
        }
        else if (pNode->nodeName() == "defaultValue" && pNode->hasChildNodes()) {
            std::string val = pNode->innerText();
            pRes->setDefaultValue(val);
            LOG(desc, debug, "set default value for state variable \"" + pRes->getName() + "\" to: " + val);

            pRes->setValue(val);
        }
        pNode = pNode->nextSibling();
    }
    return pRes;
}


std::string&
UriDescriptionReader::retrieveDescription(const std::string& relativeUri)
{
    LOG(desc, information, "base URI: " + _deviceDescriptionUri);
    LOG(desc, information, "relative URI: " + relativeUri);
    std::string* res;
    Poco::URI targetUri(_deviceDescriptionUri);

    if (targetUri.getScheme() == "file") {
        // NOTE: resolving file path against description uri doesn't work because
        // base directory is deleted, when relativeUri has a leading slash
        Poco::Path descriptionPath(targetUri.getPath());
        Poco::Path relativePath(relativeUri);
        std::string path = descriptionPath.parent().toString() + relativePath.getFileName();
        LOG(desc, information, "reading description from file: " + path);
        std::ifstream ifs(path.c_str());
        std::stringstream ss;
        Poco::StreamCopier::copyStream(ifs, ss);
        res = new std::string(ss.str());
    }
    else if (targetUri.getScheme() == "http") {
        targetUri.resolve(relativeUri);
        LOG(desc, information, "retrieving description from: " + targetUri.toString());
        LOG(desc, information, "downloading description from host: " + targetUri.getHost()
             + ", port: " + Poco::NumberFormatter::format(targetUri.getPort()) + ", path: " + targetUri.getPathEtc());
        Poco::Net::HTTPClientSession session(targetUri.getHost(), targetUri.getPort());
        Poco::Net::HTTPRequest request("GET", targetUri.getPathEtc());
        session.sendRequest(request);

        Poco::Net::HTTPResponse response;
        std::istream& rs = session.receiveResponse(response);
        LOG(desc, information, "HTTP " + Poco::NumberFormatter::format(response.getStatus()) + " " + response.getReason());
        std::stringstream header;
        response.write(header);
        LOG(desc, debug, "response header:" + Poco::LineEnding::NEWLINE_DEFAULT + header.str());
        res = new std::string;
        Poco::StreamCopier::copyToString(rs, *res);
    }
    else {
        LOG(desc, error, "Error in UriDescriptionReader: unknown scheme in description uri");
        res = new std::string;
        return *res;
    }
    LOG(desc, debug, "retrieved description:" + Poco::LineEnding::NEWLINE_DEFAULT + *res);
    return *res;
}


MemoryDescriptionReader::MemoryDescriptionReader(DescriptionProvider& deviceDescriptionProvider) :
_deviceDescriptionProvider(deviceDescriptionProvider)
{
}


std::string&
MemoryDescriptionReader::retrieveDescription(const std::string& descriptionKey)
{
    if (descriptionKey == "") {
        return _deviceDescriptionProvider.getDeviceDescription();
    }
    else {
        return _deviceDescriptionProvider.getServiceDescription(descriptionKey);
    }
}


ActionRequestReader::ActionRequestReader(const std::string& requestBody, Action* pActionTemplate) : _pActionTemplate(pActionTemplate)
{
    LOG(ctrl, debug, "action request:" + Poco::LineEnding::NEWLINE_DEFAULT + requestBody);

    Poco::XML::DOMParser parser;
#if (POCO_VERSION & 0xFFFFFFFF) < 0x01040000
    parser.setFeature(Poco::XML::DOMParser::FEATURE_WHITESPACE, false);
#else
    parser.setFeature(Poco::XML::DOMParser::FEATURE_FILTER_WHITESPACE, true);
#endif
    try {
        _pDoc = parser.parseString(requestBody.substr(0, requestBody.rfind('>') + 1));
    }
    catch (Poco::XML::SAXParseException) {
        LOG(ctrl, error, "could not parse action request, SAX parser exception.");
        return;
    }
    catch (Poco::XML::DOMException) {
        LOG(ctrl, error, "could not parse action request, DOM exception.");
        return;
    }
}


Action*
ActionRequestReader::action()
{
    Action* pRes = _pActionTemplate;
    Poco::XML::Node* pNode = _pDoc->documentElement()->firstChild();

    while(pNode && (pNode->nodeName() != pNode->prefix() + ":Body")) {
        pNode = pNode->nextSibling();
    }
    Poco::XML::Node* pBody = pNode;
    if (pBody && pBody->hasChildNodes()) {
        Poco::XML::Node* pAction = pBody->firstChild();

        if (pAction->hasChildNodes()) {
            Poco::XML::Node* pArgument = pAction->firstChild();

            while (pArgument) {
                pRes->setArgument(pArgument->nodeName(), pArgument->innerText());
                pArgument = pArgument->nextSibling();
            }
        }
        else {
            LOG(ctrl, debug, "action without arguments");
        }
    }
    else {
        LOG(ctrl, error, "action without body");
    }
    return pRes;
}


ActionResponseReader::ActionResponseReader(const std::string& responseBody, Action* pActionTemplate) :
_pActionTemplate(pActionTemplate),
_pDoc(0)
{
//    LOG(ctrl, debug, "action response:" + Poco::LineEnding::NEWLINE_DEFAULT + responseBody);
    Poco::XML::DOMParser parser;
    // there's coming a lot of rubbish thru the wire, decorated with white-spaces all over the place ...
#if (POCO_VERSION & 0xFFFFFFFF) < 0x01040000
    parser.setFeature(Poco::XML::DOMParser::FEATURE_WHITESPACE, false);
#else
    parser.setFeature(Poco::XML::DOMParser::FEATURE_FILTER_WHITESPACE, true);
#endif
    try {
        // some servers send terminating 0 char at the end of the message, we chop that rubbish off ...
        _pDoc = parser.parseString(responseBody.substr(0, responseBody.rfind('>') + 1));
    }
    catch (Poco::XML::SAXParseException) {
        LOG(ctrl, error, "could not parse action response, SAX parser exception.");
        return;
    }
    catch (Poco::XML::DOMException) {
        LOG(ctrl, error, "could not parse action response, DOM exception.");
        return;
    }
}


Action*
ActionResponseReader::action()
{
    // FIXME: action shouldn't be called, when _pDoc is 0 (see ctor of ActionResponseReader)
    if (!_pDoc) {
        return 0;
    }
    // TODO: same code as in ActionRequestReader
    Action* pRes = _pActionTemplate;
    Poco::XML::Node* pNode = _pDoc->documentElement()->firstChild();

    while(pNode && (pNode->nodeName() != pNode->prefix() + ":Body")) {
        pNode = pNode->nextSibling();
    }
    Poco::XML::Node* pBody = pNode;
    if (pBody && pBody->hasChildNodes()) {
        Poco::XML::Node* pAction = pBody->firstChild();
        if (pAction->hasChildNodes()) {
            Poco::XML::Node* pArgument = pAction->firstChild();

            while (pArgument) {
                LOG(ctrl, debug, "action response arg: " + pArgument->nodeName() + " = " + pArgument->innerText());
                pRes->setArgument(pArgument->nodeName(), pArgument->innerText());
                pArgument = pArgument->nextSibling();
            }
        }
        else {
            LOG(ctrl, debug, "action without arguments");
        }
    }
    else {
        LOG(ctrl, error, "action without body");
    }
    return pRes;
}


EventMessageReader::EventMessageReader(const std::string& responseBody, Service* pService) :
_pService(pService)
{
    Poco::XML::DOMParser parser;
#if (POCO_VERSION & 0xFFFFFFFF) < 0x01040000
    parser.setFeature(Poco::XML::DOMParser::FEATURE_WHITESPACE, false);
#else
    parser.setFeature(Poco::XML::DOMParser::FEATURE_FILTER_WHITESPACE, true);
#endif
    try {
        // some servers send terminating 0 char at the end of the message, we chop that rubbish off ...
//        responseBody.resize(responseBody.rfind('>') + 1);
        _pDoc = parser.parseString(responseBody.substr(0, responseBody.rfind('>') + 1));
    }
    catch (Poco::XML::SAXParseException) {
        LOG(event, error, "could not parse event message, SAX parser exception.");
        return;
    }
    catch (Poco::XML::DOMException) {
        LOG(event, error, "could not parse event message, DOM exception.");
        return;
    }
}


void
EventMessageReader::stateVarValues()
{
    if (!_pDoc) {
        return;
    }
    Poco::XML::Node* pProperty = _pDoc->documentElement()->firstChild();
    while (pProperty && pProperty->hasChildNodes()) {
        stateVar(pProperty);
        pProperty = pProperty->nextSibling();
    }
}


void
EventMessageReader::stateVar(Poco::XML::Node* pNode)
{
    if (pNode->nodeName() == pNode->prefix() + ":property" && pNode->hasChildNodes()) {
        LOG(event, debug, "event message reader got property");
        Poco::XML::Node* pStateVarNode = pNode->firstChild();
        std::string stateVarName = pStateVarNode->nodeName();
        std::string stateVarValue = pStateVarNode->innerText();
        _pService->setStateVar<std::string>(stateVarName, stateVarValue);
        LOG(event, debug, "calling event handler of state var: " + stateVarName + ", value: " + stateVarValue + " ...");
        StateVar* pStateVar = _pService->getStateVarReference(stateVarName);
        _pService->getDevice()->getCtlDeviceCode()->eventHandler(pStateVar);
        LOG(event, debug, "calling event handler of state var: " + stateVarName + ", value: " + stateVarValue + " finished.");
    }
    else {
        LOG(event, error, "event message without state var element in property set.");
    }
}


DescriptionWriter::DescriptionWriter()
{
    _pDoc = new Poco::XML::Document;
}


void
DescriptionWriter::deviceContainer(DeviceContainer& deviceContainer)
{
    Poco::AutoPtr<Poco::XML::Element> pRoot = _pDoc->createElement("root");
    pRoot->setAttribute("xmlns", "urn:schemas-upnp-org:device-1-0");
    Poco::AutoPtr<Poco::XML::Element> pSpecVersion = _pDoc->createElement("specVersion");
    Poco::AutoPtr<Poco::XML::Element> pMajor = _pDoc->createElement("major");
    Poco::AutoPtr<Poco::XML::Element> pMinor = _pDoc->createElement("minor");
    Poco::AutoPtr<Poco::XML::Text> pMajorVersion = _pDoc->createTextNode("1");
    Poco::AutoPtr<Poco::XML::Text> pMinorVersion = _pDoc->createTextNode("0");
    pMajor->appendChild(pMajorVersion);
    pMinor->appendChild(pMinorVersion);
    pSpecVersion->appendChild(pMajor);
    pSpecVersion->appendChild(pMinor);
    pRoot->appendChild(pSpecVersion);
    // write root device
    Poco::AutoPtr<Poco::XML::Element> pRootDevice = device(*deviceContainer.getRootDevice());
    pRoot->appendChild(pRootDevice);
    _pDoc->appendChild(pRoot);
    // end root device

    if (deviceContainer.getDeviceCount() > 1) {
        // if there are embedded devices open a deviceList
        Poco::AutoPtr<Poco::XML::Element> pDeviceList = _pDoc->createElement("deviceList");
        pRootDevice->appendChild(pDeviceList);
        for (DeviceContainer::DeviceIterator it = deviceContainer.beginDevice(); it != deviceContainer.endDevice(); ++it) {
            // write embedded devices
            if (*it != deviceContainer.getRootDevice()) {
                Poco::AutoPtr<Poco::XML::Element> pSubDevice = device(**it);
                pDeviceList->appendChild(pSubDevice);
            }
        }
    }
    // end embedded devices

    // end DeviceContainer
}


Poco::XML::Element*
DescriptionWriter::device(Device& device)
{
    Poco::XML::Element* pDevice = _pDoc->createElement("device");

    // device type
    Poco::AutoPtr<Poco::XML::Element> pDeviceType = _pDoc->createElement("deviceType");
    Poco::AutoPtr<Poco::XML::Text> pDeviceTypeVal = _pDoc->createTextNode(device.getDeviceTypeFullString());
    pDeviceType->appendChild(pDeviceTypeVal);
    pDevice->appendChild(pDeviceType);
    // UUID
    Poco::AutoPtr<Poco::XML::Element> pUuid = _pDoc->createElement("UDN");
    Poco::AutoPtr<Poco::XML::Text> pUuidVal = _pDoc->createTextNode("uuid:" + device.getUuid());
    pUuid->appendChild(pUuidVal);
    pDevice->appendChild(pUuid);

    // Properties
    for (Device::PropertyIterator it = device.beginProperty(); it != device.endProperty(); ++it) {
        Poco::AutoPtr<Poco::XML::Element> pProp = _pDoc->createElement((*it).first);
        Poco::AutoPtr<Poco::XML::Text> pVal = _pDoc->createTextNode(*((*it).second));
        pProp->appendChild(pVal);
        pDevice->appendChild(pProp);
        LOG(desc, debug, "writer added property: " + (*it).first + " = " + *(*it).second);
    }

    // Icons
    Poco::AutoPtr<Poco::XML::Element> pIconList = _pDoc->createElement("iconList");
    pDevice->appendChild(pIconList);
    for (Device::IconIterator it = device.beginIcon(); it != device.endIcon(); ++it) {
//        if ((*it)->_pData) {
        if ((*it)->_buffer.size()) {
            pIconList->appendChild(icon(*it));
            LOG(desc, debug, "writer added icon: " + (*it)->_requestPath);
        }
    }

    // Services
    Poco::AutoPtr<Poco::XML::Element> pServiceList = _pDoc->createElement("serviceList");
    pDevice->appendChild(pServiceList);
    for (Device::ServiceIterator it = device.beginService(); it != device.endService(); ++it) {
        pServiceList->appendChild(service(*it));
        LOG(desc, debug, "writer added service: " + (*it)->getServiceTypeFullString());
    }

    return pDevice;
}


Poco::XML::Element*
DescriptionWriter::service(Service* pService)
{
    Poco::XML::Element* pServiceElement = _pDoc->createElement("service");

    // serviceType
    Poco::AutoPtr<Poco::XML::Element> pServiceType = _pDoc->createElement("serviceType");
    Poco::AutoPtr<Poco::XML::Text> pServiceTypeVal = _pDoc->createTextNode(pService->getServiceTypeFullString());
    pServiceType->appendChild(pServiceTypeVal);
    pServiceElement->appendChild(pServiceType);
    // serviceId
    Poco::AutoPtr<Poco::XML::Element> pServiceId = _pDoc->createElement("serviceId");
    Poco::AutoPtr<Poco::XML::Text> pServiceIdVal = _pDoc->createTextNode(pService->getServiceId());
    pServiceId->appendChild(pServiceIdVal);
    pServiceElement->appendChild(pServiceId);
    // SCPDURL
    Poco::AutoPtr<Poco::XML::Element> pDescription = _pDoc->createElement("SCPDURL");
    Poco::AutoPtr<Poco::XML::Text> pDescriptionVal = _pDoc->createTextNode(pService->getDescriptionPath());
    pDescription->appendChild(pDescriptionVal);
    pServiceElement->appendChild(pDescription);
    // controlURL
    Poco::AutoPtr<Poco::XML::Element> pControl = _pDoc->createElement("controlURL");
    Poco::AutoPtr<Poco::XML::Text> pControlVal = _pDoc->createTextNode(pService->getControlPath());
    pControl->appendChild(pControlVal);
    pServiceElement->appendChild(pControl);
    // eventSubURL
    Poco::AutoPtr<Poco::XML::Element> pEvent = _pDoc->createElement("eventSubURL");
    Poco::AutoPtr<Poco::XML::Text> pEventVal = _pDoc->createTextNode(pService->getEventSubscriptionPath());
    pEvent->appendChild(pEventVal);
    pServiceElement->appendChild(pEvent);

    return pServiceElement;
}


Poco::XML::Element*
DescriptionWriter::icon(Icon* pIcon)
{
    Poco::XML::Element* pIconElement = _pDoc->createElement("icon");

    // mimetype
    Poco::AutoPtr<Poco::XML::Element> pMimetype = _pDoc->createElement("mimetype");
    Poco::AutoPtr<Poco::XML::Text> pMimetypeVal = _pDoc->createTextNode(pIcon->_mime.toString());
    pMimetype->appendChild(pMimetypeVal);
    pIconElement->appendChild(pMimetype);
    // width
    Poco::AutoPtr<Poco::XML::Element> pWidth = _pDoc->createElement("width");
    Poco::AutoPtr<Poco::XML::Text> pWidthVal = _pDoc->createTextNode(Poco::NumberFormatter::format(pIcon->_width));
    pWidth->appendChild(pWidthVal);
    pIconElement->appendChild(pWidth);
    // height
    Poco::AutoPtr<Poco::XML::Element> pHeight = _pDoc->createElement("height");
    Poco::AutoPtr<Poco::XML::Text> pHeightVal = _pDoc->createTextNode(Poco::NumberFormatter::format(pIcon->_height));
    pHeight->appendChild(pHeightVal);
    pIconElement->appendChild(pHeight);
    // depth
    Poco::AutoPtr<Poco::XML::Element> pDepth = _pDoc->createElement("depth");
    Poco::AutoPtr<Poco::XML::Text> pDepthVal = _pDoc->createTextNode(Poco::NumberFormatter::format(pIcon->_depth));
    pDepth->appendChild(pDepthVal);
    pIconElement->appendChild(pDepth);
    // url
    Poco::AutoPtr<Poco::XML::Element> pUrl = _pDoc->createElement("url");
    Poco::AutoPtr<Poco::XML::Text> pUrlVal = _pDoc->createTextNode(pIcon->_requestPath);
    pUrl->appendChild(pUrlVal);
    pIconElement->appendChild(pUrl);

    return pIconElement;
}


std::string*
DescriptionWriter::write()
{
    Poco::XML::DOMWriter writer;
    writer.setNewLine("\r\n");
    writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION);

    std::stringstream ss;
    writer.writeNode(ss, _pDoc);
    LOG(desc, debug, "rewrote device description:" + Poco::LineEnding::NEWLINE_DEFAULT + ss.str());
    return new std::string(ss.str());
}


ServiceDescriptionWriter::ServiceDescriptionWriter()
{
    _pDoc = new Poco::XML::Document;
}


void
ServiceDescriptionWriter::service(Service& service)
{
    Poco::AutoPtr<Poco::XML::Element> pRoot = _pDoc->createElement("scpd");
    pRoot->setAttribute("xmlns", "urn:schemas-upnp-org:service-1-0");
    Poco::AutoPtr<Poco::XML::Element> pSpecVersion = _pDoc->createElement("specVersion");
    Poco::AutoPtr<Poco::XML::Element> pMajor = _pDoc->createElement("major");
    Poco::AutoPtr<Poco::XML::Element> pMinor = _pDoc->createElement("minor");
    Poco::AutoPtr<Poco::XML::Text> pMajorVersion = _pDoc->createTextNode("1");
    Poco::AutoPtr<Poco::XML::Text> pMinorVersion = _pDoc->createTextNode("0");
    pMajor->appendChild(pMajorVersion);
    pMinor->appendChild(pMinorVersion);
    pSpecVersion->appendChild(pMajor);
    pSpecVersion->appendChild(pMinor);
    pRoot->appendChild(pSpecVersion);
    _pDoc->appendChild(pRoot);

    // write action list
    Poco::AutoPtr<Poco::XML::Element> pActionList = _pDoc->createElement("actionList");
    pRoot->appendChild(pActionList);
    for (Service::ActionIterator it = service.beginAction(); it != service.endAction(); ++it) {
        pActionList->appendChild(action(*it));
        LOG(desc, debug, "writer added action: " + (*it)->getName());
    }

    // write service state table
    Poco::AutoPtr<Poco::XML::Element> pStateTableList = _pDoc->createElement("serviceStateTable");
    pRoot->appendChild(pStateTableList);
    for (Service::StateVarIterator it = service.beginStateVar(); it != service.endStateVar(); ++it) {
        pStateTableList->appendChild(stateVar(*it));
        LOG(desc, debug, "writer added state variable: " + (*it)->getName());
    }
}


std::string*
ServiceDescriptionWriter::write()
{
    Poco::XML::DOMWriter writer;
    writer.setNewLine("\r\n");
    writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION);

    std::stringstream ss;
    writer.writeNode(ss, _pDoc);
    LOG(desc, debug, "rewrote service description:" + Poco::LineEnding::NEWLINE_DEFAULT + ss.str());
    return new std::string(ss.str());
}


Poco::XML::Element*
ServiceDescriptionWriter::action(Action* pAction)
{
    Poco::XML::Element* pActionElement = _pDoc->createElement("action");

    // name
    Poco::AutoPtr<Poco::XML::Element> pActionName = _pDoc->createElement("name");
    Poco::AutoPtr<Poco::XML::Text> pActionNameVal = _pDoc->createTextNode(pAction->getName());
    pActionName->appendChild(pActionNameVal);
    pActionElement->appendChild(pActionName);

    // argumentList
    Poco::AutoPtr<Poco::XML::Element> pArgumentList = _pDoc->createElement("argumentList");
    pActionElement->appendChild(pArgumentList);
    for (Action::ArgumentIterator it = pAction->beginArgument(); it != pAction->endArgument(); ++it) {
        pArgumentList->appendChild(argument(*it));
        LOG(desc, debug, "writer added action argument: " + (*it)->getName());
    }

    return pActionElement;
}


Poco::XML::Element*
ServiceDescriptionWriter::argument(Argument* pArgument)
{
    Poco::XML::Element* pArgumentElement = _pDoc->createElement("argument");

    // name
    Poco::AutoPtr<Poco::XML::Element> pArgumentName = _pDoc->createElement("name");
    Poco::AutoPtr<Poco::XML::Text> pArgumentNameVal = _pDoc->createTextNode(pArgument->getName());
    pArgumentName->appendChild(pArgumentNameVal);
    pArgumentElement->appendChild(pArgumentName);

    // direction
    Poco::AutoPtr<Poco::XML::Element> pDirection = _pDoc->createElement("direction");
    Poco::AutoPtr<Poco::XML::Text> pDirectionNameVal = _pDoc->createTextNode(pArgument->getDirection());
    pDirection->appendChild(pDirectionNameVal);
    pArgumentElement->appendChild(pDirection);

    // relatedStateVariable
    Poco::AutoPtr<Poco::XML::Element> pRelatedStateVar = _pDoc->createElement("relatedStateVariable");
    Poco::AutoPtr<Poco::XML::Text> pRelatedStateVarVal = _pDoc->createTextNode(pArgument->getRelatedStateVarName());
    pRelatedStateVar->appendChild(pRelatedStateVarVal);
    pArgumentElement->appendChild(pRelatedStateVar);

    return pArgumentElement;
}


Poco::XML::Element*
ServiceDescriptionWriter::stateVar(StateVar* pStateVar)
{
    Poco::XML::Element* pStateVariable = _pDoc->createElement("stateVariable");

    // sendEvents attribute
    pStateVariable->setAttribute("sendEvents", (pStateVar->getSendEvents() ? "yes" : "no"));

    // name
    Poco::AutoPtr<Poco::XML::Element> pStateVarName = _pDoc->createElement("name");
    Poco::AutoPtr<Poco::XML::Text> pStateVarNameVal = _pDoc->createTextNode(pStateVar->getName());
    pStateVarName->appendChild(pStateVarNameVal);
    pStateVariable->appendChild(pStateVarName);

    // dataType
    Poco::AutoPtr<Poco::XML::Element> pStateVarType = _pDoc->createElement("dataType");
    Poco::AutoPtr<Poco::XML::Text> pStateVarTypeVal = _pDoc->createTextNode(pStateVar->getType());
    pStateVarType->appendChild(pStateVarTypeVal);
    pStateVariable->appendChild(pStateVarType);

    // allowedValueList
    // TODO: allowed value stuff for state variables

    return pStateVariable;
}


void
ActionRequestWriter::action(Poco::AutoPtr<Action>& pAction)
{
    // TODO: nearly same code as in ActionResponseWriter
    _pDoc = new Poco::XML::Document;
    Poco::AutoPtr<Poco::XML::Element> pEnvelope = _pDoc->createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "Envelope");
    pEnvelope->setAttributeNS("http://schemas.xmlsoap.org/soap/envelope/", "encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
    Poco::AutoPtr<Poco::XML::Element> pBody = _pDoc->createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "Body");
    Poco::AutoPtr<Poco::XML::Element> pActionRequest = _pDoc->createElementNS(pAction->getService()->getServiceTypeFullString(), pAction->getName());

    for(Action::ArgumentIterator i = pAction->beginInArgument(); i != pAction->endInArgument(); ++i) {
        Poco::AutoPtr<Poco::XML::Element> pArgument = _pDoc->createElement((*i)->getName());
        Poco::AutoPtr<Poco::XML::Text> pArgumentValue = _pDoc->createTextNode(pAction->getArgument<std::string>((*i)->getName()));
        pArgument->appendChild(pArgumentValue);
        pActionRequest->appendChild(pArgument);
    }

    pBody->appendChild(pActionRequest);
    pEnvelope->appendChild(pBody);
    _pDoc->appendChild(pEnvelope);
}


void
ActionRequestWriter::write(std::string& actionMessage)
{
    Poco::XML::DOMWriter writer;
    writer.setNewLine("\r\n");
    writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION);

    std::stringstream ss;
    writer.writeNode(ss, _pDoc);
    actionMessage = ss.str();
}


ActionResponseWriter::ActionResponseWriter(std::string& responseBody) :
_responseBody(&responseBody)
{
}


void
ActionResponseWriter::action(Action* pAction)
{
    Poco::AutoPtr<Poco::XML::Document> pDoc = new Poco::XML::Document;
    Poco::AutoPtr<Poco::XML::Element> pEnvelope = pDoc->createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "Envelope");
    pEnvelope->setAttributeNS("http://schemas.xmlsoap.org/soap/envelope/", "encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
    Poco::AutoPtr<Poco::XML::Element> pBody = pDoc->createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "Body");
    Poco::AutoPtr<Poco::XML::Element> pActionResponse = pDoc->createElementNS(pAction->getService()->getServiceTypeFullString(), pAction->getName() + "Response");

    for(Action::ArgumentIterator i = pAction->beginOutArgument(); i != pAction->endOutArgument(); ++i) {
        Poco::AutoPtr<Poco::XML::Element> pArgument = pDoc->createElement((*i)->getName());
        Poco::AutoPtr<Poco::XML::Text> pArgumentValue = pDoc->createTextNode(pAction->getArgument<std::string>((*i)->getName()));
        pArgument->appendChild(pArgumentValue);
        pActionResponse->appendChild(pArgument);
    }

    pBody->appendChild(pActionResponse);
    pEnvelope->appendChild(pBody);
    pDoc->appendChild(pEnvelope);

    Poco::XML::DOMWriter writer;
    writer.setNewLine("\r\n");
    writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION);

    std::stringstream ss;
    writer.writeNode(ss, pDoc);
    *_responseBody = ss.str();
//     LOG(ctrl, debug, Omm::Util::format("response body:\n%s", *_responseBody));
}


StateVar*
Argument::getRelatedStateVarReference() const
{
    return _pAction->getService()->getStateVarReference(_relatedStateVar);
}


EventMessageWriter::EventMessageWriter()
{
    _pDoc = new Poco::XML::Document;
    _pPropertySet = _pDoc->createElementNS("urn:schemas-upnp-org:event-1-0", "propertyset");
    _pDoc->appendChild(_pPropertySet);
}


void
EventMessageWriter::write(std::string& eventMessage)
{
    Poco::XML::DOMWriter writer;
    writer.setNewLine("\r\n");
    writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION);

    std::stringstream ss;
    writer.writeNode(ss, _pDoc);
    eventMessage = ss.str();
    LOG(event, debug, "event message:" + Poco::LineEnding::NEWLINE_DEFAULT + ss.str());
}


void
EventMessageWriter::stateVar(StateVar& stateVar)
{
    LOG(event, debug, "event message writer for state var: " + stateVar.getName());

    Poco::AutoPtr<Poco::XML::Element> pProperty = _pDoc->createElementNS("urn:schemas-upnp-org:event-1-0", "property");
    Poco::AutoPtr<Poco::XML::Element> pStateVar = _pDoc->createElement(stateVar.getName());
    Poco::AutoPtr<Poco::XML::Text> pStateVarValue = _pDoc->createTextNode(stateVar.getValue());
    pStateVar->appendChild(pStateVarValue);
    pProperty->appendChild(pStateVar);
    _pPropertySet->appendChild(pProperty);
}


Poco::AtomicCounter Subscription::_timerReleaseCounter(0);

Subscription::Subscription(Service* pService) :
_pService(pService),
_pSession(0),
_pSessionUri(0),
_pQueueThread(0),
_queueThreadRunnable(*this, &Subscription::eventMessageQueueThread),
_queueThreadRunning(true),
_expireCallback(*this, &Subscription::expire),
_expireControllerCallback(*this, &Subscription::expireController),
_pTimer(0)
{
}


Subscription::~Subscription()
{
    if (_pTimer) {
        delete _pTimer;
        _timerReleaseCounter++;
    }
    if (_pSession) {
        delete _pSession;
    }
}


void
Subscription::initDevice()
{
    _uuid = Poco::UUIDGenerator().createRandom().toString();
    LOG(event, debug, "creating subscription with SID: " + _uuid);
    _eventKey = 0;
    startEventMessageQueueThread();
}


void
Subscription::startEventMessageQueueThread()
{
    _pQueueThread = new Poco::Thread;
    LOG(event, debug, "starting subscription queue thread ...");
    _pQueueThread->start(_queueThreadRunnable);
}


void
Subscription::stopEventMessageQueueThread()
{
    LOG(event, debug, "stopping subscription queue thread ...");
    _queueLock.lock();
    _queueThreadRunning = false;
    _messageQueue.push("");
    _queueCondition.broadcast();
    _queueLock.unlock();
    if (_pQueueThread->isRunning() && !_pQueueThread->tryJoin(1000)) {
        LOG(event, error, "failed to join subscription queue thread");
    }
    else {
        LOG(event, debug, "subscription queue thread finished.");
    }
}


std::string
Subscription::getUuid()
{
     return _uuid;
}


void
Subscription::setSid(const std::string& sid)
{
    LOG(event, debug, "set controller subscription sid: " + sid.substr(5));
    _uuid = sid.substr(5);
}


void
Subscription::addCallbackUri(const std::string& uri)
{
    LOG(event, debug, "add callback uri: " + uri);

    try {
        _callbackUris.push_back(Poco::URI(uri));
        if (_pSession) {
            LOG(event, warning, "connection to callback uri already established, skip adding of uri: " + uri);
        }
        else {
            _pSessionUri = &_callbackUris.back();
            _pSession = new Poco::Net::HTTPClientSession(_pSessionUri->getHost(), _pSessionUri->getPort());
            _pSession->setKeepAlive(true);
            _pSession->setTimeout(Poco::Timespan(5, 0)); // set http timeout to 5 secs. Should be 30 secs according to specs.
            _pSession->setKeepAliveTimeout(Poco::Timespan(5, 0)); // set http timeout to 5 secs. Should be 30 secs according to specs.
        }
    }
    catch(...) {
        LOG(event, warning, "callback uri not valid or reachable, skip adding of uri: " + uri);
    }
}


std::string
Subscription::getEventKey()
{
    // TODO: should lock this
    std::string res = Poco::NumberFormatter::format(_eventKey);
    // event key wrap when overflow (eventKey == 0, again), then set to 1. Otherwise just increment.
    // event key for initial event message must be 0 (not 1)
    _eventKey++;
    if (_eventKey == 0) {
        _eventKey = 1;
    }
    return res;
}


void
Subscription::sendEventMessage(const std::string& eventMessage)
{
    LOG(event, debug, "subscription queue event message ...");
    Poco::ScopedLock<Poco::FastMutex> queueLock(_queueLock);
    _messageQueue.push(eventMessage);
    _queueCondition.broadcast();
    LOG(event, debug, "subscription queue event message done.");
}


void
Subscription::renew(unsigned int seconds)
{
    LOG(event, debug, "renew subscription timer milliseconds: " + Poco::NumberFormatter::format(seconds * 1000));
    if (_pTimer) {
        _pTimer->restart(seconds * 1000);
    }
    else {
        LOG(event, debug, "create timer for subscription");
        // NOTE: thread pool gets too large when subscriptions appear over a long period of time, so we have to
        // count released timers and don't allocate a new thread in the default thread pool, when there is a released one
        if (_timerReleaseCounter <= 0) {
            Poco::ThreadPool::defaultPool().addCapacity(1);
        }
        else {
            _timerReleaseCounter--;
        }
        _pTimer = new Poco::Timer;
        _pTimer->setStartInterval(seconds * 1000);
        try {
            _pTimer->start(_expireCallback);
        }
        catch (Poco::Exception& e) {
            LOG(event, error, "failed to start subscription timer: " + e.displayText());
        }
    }
}


void
Subscription::expire(Poco::Timer& timer)
{
    // unregisterSubscription() deletes this ...
    LOG(event, debug, "subscription expired.");
    _pTimer->restart(0);
    _pService->unregisterSubscription(this);
}


void
Subscription::renewController(unsigned int seconds)
{
    // renew the subsciption at half of the time before it expires.
    long milliseconds = seconds * 1000 / 2;
    LOG(event, debug, "controller renew subscription timer milliseconds: " + Poco::NumberFormatter::format(milliseconds));

    if (_pTimer) {
        _pTimer->restart(milliseconds);
    }
    else {
        LOG(event, debug, "create timer for controller subscription");
        // NOTE: thread pool gets too large when subscriptions appear over a long period of time, so we have to
        // count released timers and don't allocate a new thread in the default thread pool, when there is a released one
        if (_timerReleaseCounter <= 0) {
            Poco::ThreadPool::defaultPool().addCapacity(1);
        }
        else {
            _timerReleaseCounter--;
        }
        _pTimer = new Poco::Timer;
        _pTimer->setStartInterval(milliseconds);
        try {
            _pTimer->start(_expireControllerCallback);
        }
        catch (Poco::Exception& e) {
            LOG(event, error, "failed to start controller subscription timer: " + e.displayText());
        }
    }
}


void
Subscription::expireController(Poco::Timer& timer)
{
    LOG(event, debug, "controller subscription timer expired, renewing subscription.");
    try {
        _pService->sendSubscriptionRequest(true);
    }
    catch (Poco::Exception&e) {
        LOG(event, error, "controller subscription renewing subscription failed: " + e.displayText());
    }
}


void
Subscription::stopExpirationTimer()
{
    if (_pTimer) {
        LOG(event, debug, "subscription expiration timer stopping ...");
        _pTimer->stop();
        delete _pTimer;
        _pTimer = 0;
        LOG(event, debug, "subscription expiration timer stopped.");
    }
    else {
        LOG(event, debug, "subscription expiration timer not active, no need to stop.");
    }
}


void
Subscription::deliverEventMessage(const std::string& eventMessage)
{
    // FIXME: timeout should be 30 sec for event notifications (see 4.2.1 Eventing: Event messages: NOTIFY)
    if (!eventMessage.size()) {
        // empty message means "end of messages" and event message delivery thread should be stopped
        LOG(event, debug, "empty event message queued, end of messages.")
        return;
    }
    Poco::Net::HTTPRequest request("NOTIFY", _pSessionUri->getPath(), "HTTP/1.1");
    request.set("HOST", _pSessionUri->getAuthority());
    request.setContentType("text/xml");
    request.set("NT", "upnp:event");
    request.set("NTS", "upnp:propchange");
    request.set("SID", "uuid:" + _uuid);

    request.set("SEQ", getEventKey());
    request.setContentLength(eventMessage.size());
    LOG(event, debug, "sending event notification request to: " + _pSessionUri->toString());
    std::stringstream ss;
    request.write(ss);

    // set request body and send request
    try {
        std::ostream& ostr = getSession()->sendRequest(request);
        ostr << eventMessage;
    }
    catch (Poco::Exception& e) {
        LOG(event, error, "delivering event message failed: " + e.displayText());
        return;
    }

    LOG(event, debug, "event notification request sent: " + Poco::LineEnding::NEWLINE_DEFAULT + ss.str() + eventMessage);

    // receive answer ...
    Poco::Net::HTTPResponse response;
    try {
        getSession()->receiveResponse(response);
        LOG(event, debug, "HTTP " + Poco::NumberFormatter::format(response.getStatus()) + " " + response.getReason());
    }
    catch (Poco::Exception& e) {
        LOG(event, error, "no reponse while delivering event message: " + e.displayText());
    }
}


void
Subscription::eventMessageQueueThread()
{
    LOG(event, debug, "subscription event message queue thread running.");
    while (eventMessageQueueThreadRunning()) {
        _queueLock.lock();
        if (_messageQueue.size() == 0) {
            _queueCondition.wait<Poco::FastMutex>(_queueLock);
        }
        deliverEventMessage(_messageQueue.front());
        _messageQueue.pop();
        _queueLock.unlock();
    }
    LOG(event, debug, "subscription event message queue thread finished.");
}


bool
Subscription::eventMessageQueueThreadRunning()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_queueLock);
    return _queueThreadRunning;
}


EventMessageQueue::EventMessageQueue(Service* pService) :
_pService(pService),
_maxEventRate(200),
_pModeratorTimer(0),
_timerIsRunning(false),
_firstStart(true)
{
}


EventMessageQueue::~EventMessageQueue()
{
    if (_pModeratorTimer) {
        delete _pModeratorTimer;
    }
}


void
EventMessageQueue::queueStateVar(StateVar& stateVar)
{
    LOG(event, debug, "queue state var: " + stateVar.getName() + " ...");
    Poco::ScopedLock<Poco::FastMutex> lock(_lock);

    _stateVars.insert(&stateVar);
    if (!_timerIsRunning) {
//        LOG(event, debug, "set up moderation timer with event rate: " + Poco::NumberFormatter::format(_maxEventRate));
        _timerIsRunning = true;
        if (_pModeratorTimer) {
            delete _pModeratorTimer;
        }
        // need to create a new Poco::Timer, because same timer can't be reused (works perhaps twice, but not the third time).
//        LOG(event, debug, "create timer for moderation");
        _pModeratorTimer = new Poco::Timer(_maxEventRate);
//        LOG(event, debug, "starting moderation timer ...");
        try {
            _pModeratorTimer->start(Poco::TimerCallback<EventMessageQueue> (*this, &EventMessageQueue::sendEventMessage));
//            LOG(event, debug, "moderation timer started.");
        }
        catch(Poco::Exception& e) {
            LOG(event, error, "failed to start moderation timer: " + e.displayText());
        }
    }
    LOG(event, debug, "queue state var: " + stateVar.getName() + " finished.");
}


void
EventMessageQueue::sendEventMessage(Poco::Timer& timer)
{
    LOG(event, debug, "event message queue sends event notifications ...");
    Poco::ScopedLock<Poco::FastMutex> lock(_lock);

//    _lock.lock();
    _timerIsRunning = false;
    std::string eventMessage;
    EventMessageWriter messageWriter;
    for (std::set<StateVar*>::iterator it = _stateVars.begin(); it != _stateVars.end(); ++it) {
        messageWriter.stateVar(**it);
    }
    _stateVars.clear();
    messageWriter.write(eventMessage);
//    _lock.unlock();

    for (Service::SubscriptionIterator i = _pService->beginEventSubscription(); i != _pService->endEventSubscription(); ++i) {
        (*i)->sendEventMessage(eventMessage);
    }
    LOG(event, debug, "event message queue sends event notifications finished.");
}


std::string
StateVar::getName() const
{
    return _name;
}


Service*
StateVar::getService()
{
    return _pService;
}


const std::string&
StateVar::getType() const
{
    return _type;
}


void
StateVar::setService(Service* pService)
{
    _pService = pService;
}


void
StateVar::setName(std::string name)
{
    _name = name;
}


void
StateVar::setType(std::string type)
{
    _type = type;
}


void
StateVar::setDefaultValue(std::string defaultValue)
{
    _defaultValue = defaultValue;
}


void
StateVar::setSendEvents(std::string sendEvents)
{
    _sendEvents = (sendEvents=="yes") ? true : false;
}


bool
StateVar::getSendEvents() const
{
    return _sendEvents;
}


bool
StateVar::getIsArgType() const
{
    return _name.substr(0, 11) == "A_ARG_TYPE_";
}


Service::Service() :
_pControllerSubscriptionData(new Subscription(this)),
_deviceEnableEventing(false),
_controllerSubscribeEventing(false),
_pEventMessageQueue(new EventMessageQueue(this)),
_subscriptionDuration(1800),
_pDelegate(0)
{
}


Service::~Service()
{
    if (_pEventMessageQueue) {
        delete _pEventMessageQueue;
    }
    if (_pControllerSubscriptionData) {
        delete _pControllerSubscriptionData;
    }
    // NOTE: be carefull with Container<Action>, these are Poco::Notifications
    //       and should be stored in Poco::AutoPtr, which is currently not possible with Container
    for (ActionIterator it = beginAction(); it != endAction(); ++it) {
        (*it)->release();
    }
    // TODO: delete the other members that are allocated on the heap
}


Service::StateVarIterator
Service::beginStateVar()
{
    return _stateVars.begin();
}


Service::StateVarIterator
Service::endStateVar()
{
    return _stateVars.end();
}


Service::StateVarIterator
Service::beginEventedStateVar()
{
    return _eventedStateVars.begin();
}


Service::StateVarIterator
Service::endEventedStateVar()
{
    return _eventedStateVars.end();
}


Service::ActionIterator
Service::beginAction()
{
    return _actions.begin();
}


Service::ActionIterator
Service::endAction()
{
    return _actions.end();
}


Service::SubscriptionIterator
Service::beginEventSubscription()
{
    return _eventSubscriptions.begin();
}


Service::SubscriptionIterator
Service::endEventSubscription()
{
    return _eventSubscriptions.end();
}


Service::EventCallbackPathIterator
Service::beginEventCallbackPath()
{
    return _eventCallbackPaths.begin();
}


Service::EventCallbackPathIterator
Service::endEventCallbackPath()
{
    return _eventCallbackPaths.end();
}


std::string
Service::getServiceTypeFullString() const
{
    return _serviceType;
}


std::string
Service::getServiceId() const
{
    return _serviceId;
}


std::string
Service::getDescriptionPath() const
{
    return _descriptionPath;
}


std::string*
Service::getDescription() const
{
    return _pDescription;
}


std::string
Service::getControlPath() const
{
    return _controlPath;
}


std::string
Service::getEventSubscriptionPath() const
{
    return _eventSubscriptionPath;
}


std::string
Service::getEventCallbackPath(int callbackPathIndex)
{
    return _eventCallbackPaths[callbackPathIndex];
}


bool
Service::getControllerSubscribeEventing()
{
    return _controllerSubscribeEventing;
}


DescriptionRequestHandler*
Service::getDescriptionRequestHandler() const
{
    return _pDescriptionRequestHandler;
}


ControlRequestHandler*
Service::getControlRequestHandler() const
{
    return _controlRequestHandler;
}


Device*
Service::getDevice() const
{
//    LOG(upnp, debug, "service, get device pointer: " + Poco::NumberFormatter::format(_pDeviceData->getDevice()));
    return _pDeviceData->getDevice();
}


Action*
Service::getAction(const std::string& actionName)
{
    return &_actions.get(actionName);
}


Subscription*
Service::getSubscription(const std::string& sid)
{
    return &_eventSubscriptions.get(sid);
}


StateVar*
Service::getStateVarReference(const std::string& key)
{
    return &_stateVars.get(key);
}


void
Service::setServiceTypeFullString(std::string serviceType)
{
    _serviceType = serviceType;
}


void
Service::setServiceId(std::string serviceId)
{
    _serviceId = serviceId;
}


void
Service::setDescriptionPath(std::string descriptionPath)
{
    LOG(upnp, debug, "service, set description path: " + descriptionPath);
    _descriptionPath = descriptionPath;
}


void
Service::setServiceDescription(std::string& description)
{
    _pDescription = &description;
}


void
Service::setControlPath(std::string controlPath)
{
    _controlPath = controlPath;
}


void
Service::setEventSubscriptionPath(std::string eventPath)
{
    _eventSubscriptionPath = eventPath;
}


void
Service::setControllerSubscribeEventing(bool subscribe)
{
    _controllerSubscribeEventing = subscribe;
}


void
Service::setDeviceData(DeviceData* pDeviceData)
{
//    LOG(upnp, debug, "service, set device data: " + Poco::NumberFormatter::format(pDeviceData));
    _pDeviceData = pDeviceData;
}


void
Service::setServiceDelegate(ServiceDelegate* pDelegate)
{
    _pDelegate = pDelegate;
}


void
Service::addAction(Action* pAction)
{
    _actions.append(pAction->getName(), pAction);
    pAction->setService(this);
}


void
Service::addStateVar(StateVar* pStateVar)
{
//     LOG(upnp, debug, "Service::addStateVar() name: " + pStateVar->getName() + " is evented: " << pStateVar->getSendEvents());

    _stateVars.append(pStateVar->getName(), pStateVar);
    if(pStateVar->getSendEvents()) {
        _eventedStateVars.append(pStateVar->getName(), pStateVar);
    }
    pStateVar->setService(this);
}


void
Service::addEventCallbackPath(const std::string path)
{
    _eventCallbackPaths.push_back(path);
}


void
Service::initController()
{
    LOG(upnp, debug, "service, init controller ...");
    _baseUri = Poco::URI(getDevice()->getDeviceContainer()->getDescriptionUri());
    LOG(upnp, debug, "service, init controller finished.");
}


// FIXME: queue a notification in DeviceManager on network activity events, don't call
// user interface methods directly.
void
Service::actionNetworkActivity(bool begin)
{
    Controller* pController = getDevice()->getDeviceContainer()->getController();
    if (pController) {
        pController->signalNetworkActivity(begin);
    }
}


void
Service::sendAction(Poco::AutoPtr<Action>& pAction)
{
    std::string actionMessage;
    ActionRequestWriter requestWriter;
    requestWriter.action(pAction);
    requestWriter.write(actionMessage);

    _serviceLock.lock();
    Poco::URI baseUri(getDevice()->getDeviceContainer()->getDescriptionUri());
    Poco::URI controlUri(baseUri);
    controlUri.resolve(_controlPath);
    Poco::Net::HTTPRequest request("POST", controlUri.getPathEtc(), "HTTP/1.1");
    request.set("HOST", baseUri.getAuthority());
    request.setContentType("text/xml; charset=\"utf-8\"");
    request.set("SOAPACTION", "\"" + _serviceType + "#" + pAction->getName() + "\"");
    request.setContentLength(actionMessage.size());
    Poco::Net::HTTPClientSession controlRequestSession(Poco::Net::SocketAddress(_baseUri.getAuthority()));
    _serviceLock.unlock();

    LOG(ctrl, debug, "*** sending action \"" + pAction->getName() + "\" to " + baseUri.getAuthority() + request.getURI() + " ***");

    actionNetworkActivity(true);
    try {
        std::ostream& ostr = controlRequestSession.sendRequest(request);
        ostr << actionMessage;
        LOG(ctrl, debug, "action request sent:" + Poco::LineEnding::NEWLINE_DEFAULT + actionMessage);
    }
    catch(Poco::Net::ConnectionRefusedException) {
        actionNetworkActivity(false);
        LOG(ctrl, error, "sending of action request failed, connection refused");
        throw Poco::Exception("sending of action request failed, connection refused");
    }
    catch (...) {
        actionNetworkActivity(false);
        LOG(ctrl, error, "sending of action request failed for some reason");
        throw Poco::Exception("sending of action request failed for some reason");
    }
    // receive response ...
    Poco::Net::HTTPResponse response;
    try {
        std::istream& rs = controlRequestSession.receiveResponse(response);
        LOG(ctrl, debug, "HTTP " + Poco::NumberFormatter::format(response.getStatus()) + " " + response.getReason());
        std::string responseBody;
        Poco::StreamCopier::copyToString(rs, responseBody);
        LOG(ctrl, debug, "action response received:" + Poco::LineEnding::NEWLINE_DEFAULT + responseBody);
        ActionResponseReader responseReader(responseBody, pAction);
        responseReader.action();
        actionNetworkActivity(false);
    }
    catch (Poco::Net::NoMessageException) {
        actionNetworkActivity(false);
        LOG(ctrl, error, "no response to action request \"" + pAction->getName()+ "\"");
        throw Poco::Exception("no response to action request \"" + pAction->getName()+ "\"");
    }
    catch (...) {
        actionNetworkActivity(false);
        LOG(ctrl, error, "no response to action request \"" + pAction->getName()+ "\" received for some reason");
        throw Poco::Exception("no response to action request \"" + pAction->getName()+ "\" received for some reason");
    }
    LOG(ctrl, debug, "*** action \"" + pAction->getName() + "\" completed ***");
}


void
Service::setSubscriptionDuration(unsigned int duration)
{
    _subscriptionDuration = duration;
}


void
Service::stopSubscriptionExpirationTimer()
{
    if (_pControllerSubscriptionData) {
        _pControllerSubscriptionData->stopExpirationTimer();
    }
}


void
Service::sendSubscriptionRequest(bool renew)
{
    Poco::URI baseUri(getDevice()->getDeviceContainer()->getDescriptionUri());
    Poco::URI eventSubscriptionUri(baseUri);
    eventSubscriptionUri.resolve(getEventSubscriptionPath());
//    LOG(event, debug, "preparing subscription request for uri: "  + eventSubscriptionUri.toString());
    Poco::Net::HTTPRequest request("SUBSCRIBE", eventSubscriptionUri.getPathEtc(), "HTTP/1.1");
    request.set("HOST", baseUri.getAuthority());
    if (renew) {
        request.set("SID", "uuid:" + _pControllerSubscriptionData->getUuid());
    }
    else {
        std::string callbackUris;
        for (EventCallbackPathIterator it = beginEventCallbackPath(); it != endEventCallbackPath(); ++it) {
            callbackUris += "<" + getDevice()->getDeviceContainer()->getDeviceManager()->getHttpServerUri() + (*it) + ">";
        }
        request.set("CALLBACK", callbackUris);
        request.set("NT", "upnp:event");
    }
    if (_subscriptionDuration == 0) {
        request.set("TIMEOUT", "Second-infinite");
    }
    else {
        request.set("TIMEOUT", "Second-" + Poco::NumberFormatter::format(_subscriptionDuration));
    }
    std::stringstream ss;
    request.write(ss);
    LOG(event, debug, "subscription request: " + Poco::LineEnding::NEWLINE_DEFAULT + ss.str());
    LOG(event, debug, "set up client session to uri: " + baseUri.getAuthority());
    Poco::Net::HTTPClientSession eventSubscriptionSession(Poco::Net::SocketAddress(baseUri.getAuthority()));

    LOG(event, debug, "sending subscription request to " + baseUri.getAuthority() + request.getURI());
    try {
        eventSubscriptionSession.sendRequest(request);
        LOG(event, debug, "subscription request sent");
    }
    catch(Poco::Net::ConnectionRefusedException) {
        LOG(event, error, "sending of subscription request failed, connection refused");
        throw Poco::Exception("connection to subscription publisher refused");
    }
    catch (...) {
        LOG(event, error, "sending of subscription request failed for some reason");
        throw Poco::Exception("no connection to subscription publisher for some reason");
    }
    // receive response ...
    Poco::Net::HTTPResponse response;
    try {
        eventSubscriptionSession.receiveResponse(response);
//        LOG(event, debug, "HTTP " + Poco::NumberFormatter::format(response.getStatus()) + " " + response.getReason());
        std::stringstream ss;
        response.write(ss);
        LOG(event, debug, "subscription response: " + Poco::LineEnding::NEWLINE_DEFAULT + ss.str());
    }
    catch (Poco::Net::NoMessageException) {
        LOG(event, error, "no response to subscription request.");
        throw Poco::Exception("no response from subscription publisher");
    }
    catch (...) {
        LOG(event, error, "no response to subscription received for some reason.");
        throw Poco::Exception("no response from subscription publisher for some reason");
    }

    if (!renew) {
        _pControllerSubscriptionData->setSid(response.get("SID"));
    }
    int timeout = 0;
    std::string timeoutString = response.get("TIMEOUT").substr(7);
    if (timeoutString != "infinite") {
        try {
            timeout = Poco::NumberParser::parse(timeoutString);
            setSubscriptionDuration(timeout);
            _pControllerSubscriptionData->renewController(timeout);
        }
        catch(Poco::Exception& e) {
            LOG(event, error, "cannot parse subscription timeout: " + e.displayText());
        }
    }

    LOG(event, debug, "subscription request completed");
}


void
Service::sendCancelSubscriptionRequest()
{
    LOG(event, debug, "cancel subscription request");
    // FIXME: crash here, because wrong device pointer returned from getDevice() ?
    Poco::URI baseUri(getDevice()->getDeviceContainer()->getDescriptionUri());
    Poco::URI eventSubscriptionUri(baseUri);
    eventSubscriptionUri.resolve(getEventSubscriptionPath());
    Poco::Net::HTTPRequest request("UNSUBSCRIBE", eventSubscriptionUri.getPath(), "HTTP/1.1");
    request.set("HOST", baseUri.getAuthority());
    // controller stores only one subscription in each service.
    request.set("SID", "uuid:" + _pControllerSubscriptionData->getUuid());
    Poco::Net::HTTPClientSession eventSubscriptionSession(Poco::Net::SocketAddress(baseUri.getAuthority()));
    eventSubscriptionSession.setTimeout(Poco::Timespan(5, 0)); // set unsubscription request timeout to 5 secs, should be 30 secs according to specs.

    std::stringstream ss;
    request.write(ss);
    LOG(event, debug, "cancel subscription request: " + Poco::LineEnding::NEWLINE_DEFAULT + ss.str());

    LOG(event, debug, "sending cancel subscription request to " + baseUri.getAuthority() + request.getURI());
    try {
        eventSubscriptionSession.sendRequest(request);
        LOG(event, debug, "cancel subscription request sent");
    }
    catch(Poco::Net::ConnectionRefusedException) {
        LOG(event, error, "sending of cancel subscription request failed, connection refused");
        throw Poco::Exception("sending of cancel subscription request failed, connection refused");
    }
    catch (...) {
        LOG(event, error, "sending of cancel subscription request failed for some reason");
        throw Poco::Exception("sending of cancel subscription request failed for some reason");
    }
    // receive response ...
    Poco::Net::HTTPResponse response;
    try {
        std::istream& rs = eventSubscriptionSession.receiveResponse(response);
//        LOG(event, debug, "HTTP " + Poco::NumberFormatter::format(response.getStatus()) + " " + response.getReason());
        std::stringstream ss;
        response.write(ss);
        std::string body;
        Poco::StreamCopier::copyToString(rs, body);
        LOG(event, debug, "cancel subscription response: " + Poco::LineEnding::NEWLINE_DEFAULT + ss.str() + body);
    }
    catch (Poco::Net::NoMessageException) {
        LOG(event, error, "no response to cancel subscription request.");
        throw Poco::Exception("");
    }
    catch (...) {
        LOG(event, error, "no response to cancel subscription received for some reason.");
        throw Poco::Exception("");
    }

    LOG(event, debug, "cancel subscription request completed");
}


void
Service::deleteSubscription()
{
    if (_pControllerSubscriptionData) {
        delete _pControllerSubscriptionData;
        _pControllerSubscriptionData = 0;
    }
}


void
Service::registerSubscription(Subscription* pSubscription)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_serviceLock);
    // TODO: only register a Subscription once from one distinct Controller
    //       note that Subscription has a new SID
    std::string sid = pSubscription->getUuid();
    LOG(event, debug, "register subscription with SID: " + sid);
    _eventSubscriptions.append(sid, pSubscription);
}


void
Service::unregisterSubscription(Subscription* pSubscription)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_serviceLock);
    std::string sid = pSubscription->getUuid();
    LOG(event, debug, "unregister subscription with SID: " + sid + " ...");
    _eventSubscriptions.remove(sid);
    pSubscription->stopExpirationTimer();
    pSubscription->stopEventMessageQueueThread();
    delete pSubscription;
    LOG(event, debug, "unregister subscription with SID: " + sid + " finished");
}


void
Service::enableEventing(bool enable)
{
    _deviceEnableEventing = true;
}


void
Service::queueEventMessage(StateVar& stateVar)
{
    _pEventMessageQueue->queueStateVar(stateVar);
}


void
Service::sendEventMessage(StateVar& stateVar)
{
    std::string eventMessage;
    EventMessageWriter messageWriter;
    messageWriter.stateVar(stateVar);
    messageWriter.write(eventMessage);

    for (SubscriptionIterator i = beginEventSubscription(); i != endEventSubscription(); ++i) {
        (*i)->sendEventMessage(eventMessage);
    }
}


void
Service::sendInitialEventMessage(Subscription* pSubscription)
{
    if (_pDelegate) {
        _pDelegate->writeInitialEventMessage();
    }

    std::string eventMessage;
    EventMessageWriter messageWriter;
    for (StateVarIterator i = beginEventedStateVar(); i != endEventedStateVar(); ++i) {
        messageWriter.stateVar(**i);
    }
    messageWriter.write(eventMessage);
    LOG(event, debug, "sending initial event message:" + Poco::LineEnding::NEWLINE_DEFAULT + eventMessage);
    pSubscription->sendEventMessage(eventMessage);
}


void
Service::setDescriptionRequestHandler()
{
    _pDescriptionRequestHandler = new DescriptionRequestHandler(_pDescription);
}


Action*
Action::clone()
{
    Action* res = new Action;
    res->_actionName = _actionName;
    res->_pService = _pService;
    // make a deep copy of the Arguments
    for (Container<Argument>::Iterator i = _arguments.begin(); i != _arguments.end(); ++i) {
        res->appendArgument(new Argument(**i));
    }
    return res;
}


void
Action::appendArgument(Argument* pArgument)
{
    pArgument->setAction(this);
    _arguments.append(pArgument->getName(), pArgument);
    if (pArgument->getDirection() == "in") {
        _inArguments.append(pArgument->getName(), pArgument);
    }
    else {
        _outArguments.append(pArgument->getName(), pArgument);
    }
}


UpnpRequestHandlerFactory::UpnpRequestHandlerFactory(HttpSocket* pHttpSocket):
_pHttpSocket(pHttpSocket)
{
}


Poco::Net::HTTPRequestHandler*
UpnpRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
    LOG(http, debug, "dispatching request: " + request.getURI());
    Poco::Path path(request.getURI());
    std::string absolutePath = path.absolute("/").toString();
    std::map<std::string,UpnpRequestHandler*>::iterator i = _requestHandlerMap.find(absolutePath);
    if (i != _requestHandlerMap.end()) {
        return i->second->create();
    }
    else {
        return new RequestNotFoundRequestHandler();
    }
}


void
UpnpRequestHandlerFactory::registerRequestHandler(std::string uri, UpnpRequestHandler* requestHandler)
{
    Poco::Path path(uri);
    std::string absolutePath = path.absolute("/").toString();
    _requestHandlerMap[absolutePath] = requestHandler;
}


RequestNotFoundRequestHandler*
RequestNotFoundRequestHandler::create()
{
    return new RequestNotFoundRequestHandler();
}


void
RequestNotFoundRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    LOG(http, debug, "unknown request " + request.getURI() + " HTTP 404 - not found error");
    response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
}


DescriptionRequestHandler::DescriptionRequestHandler(std::string* pDescription):
_pDescription(pDescription)
{
}


DescriptionRequestHandler*
DescriptionRequestHandler::create()
{
    // TODO: can we somehow avoid to make a copy of the RequestHandler on each request?
    return new DescriptionRequestHandler(_pDescription);
}


void
DescriptionRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    std::ostringstream requestHeader;
    request.write(requestHeader);
    LOG(desc, debug, "description request: " + request.getURI());
    LOG(desc, debug, "description request header: " + Poco::LineEnding::NEWLINE_DEFAULT + requestHeader.str());
    if (request.has("Accept-Language")) {
        std::string lang = request.get("Accept-Language");
        // TODO: set proper lang in description response
        response.set("Content-Language", lang);
//         response.set("CONTENT-LANGUAGE", lang);
    }
    response.setContentLength(_pDescription->size());
    response.setContentType("text/xml");
    response.setDate(Poco::Timestamp());
//     response.setVersion("HTTP/1.1");
//     response.set("CONTENT-LENGTH", Poco::NumberFormatter::format(_pDescription->size()));
//     response.set("CONTENT-TYPE", "text/xml; charset=UTF-8");
//     response.set("DATE", Poco::DateTimeFormatter::format(Poco::Timestamp(), Poco::DateTimeFormat::HTTP_FORMAT));
//     response.set("CONNECTION", "close");
//     response.set("SERVER", "Linux/2.6.27-17-generic, UPnP/1.0, Omm/0.1.0");
//     response.set("LAST-MODIFIED", "Sun, 14 Mar 2010 11:30:22 GMT");
    std::ostringstream responseHeader;
    response.write(responseHeader);
    LOG(desc, debug, "description response header: " + Poco::LineEnding::NEWLINE_DEFAULT + responseHeader.str());
    response.sendBuffer(_pDescription->c_str(), _pDescription->size());
    if (response.sent()) {
        LOG(desc, debug, "description (header) successfully sent");
    }


//    mediatomb needs this for SAMSUNGs:
//    <custom-http-headers>
//    <!-- Samsung needs it -->
//    <add header="transferMode.dlna.org: Streaming"/>
//    <add header="contentFeatures.dlna.org: DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"/>
//    </custom-http-headers>

}


ControlRequestHandler::ControlRequestHandler(Service* service):
_pService(service)
{
}


ControlRequestHandler*
ControlRequestHandler::create()
{
    return new ControlRequestHandler(_pService);
}


void
ControlRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    LOG(ctrl, debug, "*** action request: " + request.getURI() + " ***");
    // synchronous action handling: wait until handling of action request has finished. This must be done in under 30 sec,
    // otherwise it should return and an event should be sent on finishing the action request.
    int length = request.getContentLength();
    char buf[length];
    request.stream().read(buf, length);
    std::string requestBody(buf, length);

    std::string soap = request.get("SOAPACTION");
    std::string soapAction = soap.substr(1, soap.size()-2);
    std::string::size_type hash = soapAction.find('#');
    std::string serviceType = soapAction.substr(0, hash);
    std::string actionName = soapAction.substr(hash+1);
    LOG(ctrl, debug, "action received: \"" + actionName + "\" (service type " + serviceType + ")");

    Action* pAction = 0;
    try {
        pAction = _pService->getAction(actionName);
    }
    catch (Poco::Exception& e) {
        LOG(ctrl, error, "unkown action with name: " + actionName + ", ignoring");
        // TODO: respond the request with an error message
        return;
    }
    // allocate a copy of the requested Action
    pAction = pAction->clone();
    // TODO: introduce ActionRequestReader::write(Action*) to get rid of confusing pAction stuff
    ActionRequestReader requestReader(requestBody, pAction);
    pAction = requestReader.action();
    // action is posted through the notification center of the device
    _pService->getDevice()->postAction(pAction);

    // return Action response with out arguments filled in by Notification Handler
    std::string responseBody;
    ActionResponseWriter responseWriter(responseBody);
    responseWriter.action(pAction);
    // there is only one registered notification handler per device, so we can release the Action here
    pAction->release();

    response.setContentType("text/xml");
    // TODO: set EXT header
    // TODO: set SERVER header
    response.setContentLength(responseBody.size());
    std::ostream& ostr = response.send();
    ostr << responseBody;
    LOG(ctrl, debug, "action response sent:" + Poco::LineEnding::NEWLINE_DEFAULT + responseBody);
    LOG(ctrl, debug, "*** action request completed: " + request.getURI() + " ***");
}


EventSubscriptionRequestHandler*
EventSubscriptionRequestHandler::create()
{
    return new EventSubscriptionRequestHandler(_pService);
}


void
EventSubscriptionRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    std::stringstream header;
    request.write(header);
    std::string sid;
    Subscription* pSubscription;

    if (request.getMethod() == "SUBSCRIBE") {
        LOG(event, debug, "subscription request from: " + request.clientAddress().toString() + Poco::LineEnding::NEWLINE_DEFAULT + header.str());
        unsigned int timeout = 0;
        std::string timeoutString = request.get("TIMEOUT").substr(7);
        if (timeoutString != "infinite") {
            try {
                timeout = Poco::NumberParser::parse(timeoutString);
            }
            catch(Poco::Exception& e) {
                LOG(event, error, "cannot parse subscription timeout, assuming infinite");
            }
        }
        if (!request.has("SID")) {
            std::string callbackUriString = request.get("CALLBACK");
            Poco::StringTokenizer callbackUris(callbackUriString, "<>", Poco::StringTokenizer::TOK_IGNORE_EMPTY | Poco::StringTokenizer::TOK_TRIM);
            pSubscription = new Subscription(_pService);
            pSubscription->initDevice();
            sid = pSubscription->getUuid();
            for (Poco::StringTokenizer::Iterator it = callbackUris.begin(); it != callbackUris.end(); ++it) {
                pSubscription->addCallbackUri(*it);
            }
            _pService->registerSubscription(pSubscription);
        }
        else {
            sid = request.get("SID").substr(5);
            // renew subscription
            pSubscription = _pService->getSubscription(sid);
            if (!pSubscription) {
                LOG(event, error, "cannot find subscription for renewal, id: " + sid);
            }
        }
        if (timeout != 0) {
            pSubscription->renew(timeout);
        }
//         Poco::Timestamp t;
//         response.set("DATE", Poco::DateTimeFormatter::format(t, Poco::DateTimeFormat::HTTP_FORMAT));
        response.setDate(Poco::Timestamp());
        response.set("SERVER",
                     Poco::Environment::osName() + "/"
                     + Poco::Environment::osVersion() + ", "
                    + "UPnP/" + UPNP_VERSION + ", "
                    + "OMM/" + OMM_VERSION);
        response.set("SID", "uuid:" + sid);
        if (timeout == 0) {
            response.set("TIMEOUT", "Second-infinite");
        }
        else {
            response.set("TIMEOUT", "Second-" + Poco::NumberFormatter::format(timeout));
        }
        response.setContentLength(0);
        // TODO: choose timeout according to controller activity
        // TODO: provide TCP FIN flag or Content-Length=0 before initial event message (see specs p. 65)
        // TODO: may make subscription uuid's persistance
        std::stringstream responseHeader;
        response.write(responseHeader);
        response.send();
        LOG(event, debug, "event subscription response sent: " + Poco::LineEnding::NEWLINE_DEFAULT + responseHeader.str());
        if (!request.has("SID")) {
            _pService->sendInitialEventMessage(pSubscription);
        }
    }
    else if (request.getMethod() == "UNSUBSCRIBE") {
        LOG(event, debug, "cancel subscription request from: " + request.clientAddress().toString() + Poco::LineEnding::NEWLINE_DEFAULT + header.str());
        sid = request.get("SID").substr(5);
        try {
            Subscription* pSubscription = _pService->getSubscription(sid);
            if (pSubscription) {
                _pService->unregisterSubscription(pSubscription);
            }
        }
        catch (...) {
            // TODO: forward error in response to request
            LOG(event, error, "unregister subscription failed, ignoring.");
        }
        std::stringstream responseHeader;
        response.write(responseHeader);
        response.send();
        LOG(event, debug, "event cancel subscription response sent: " + Poco::LineEnding::NEWLINE_DEFAULT + responseHeader.str());
    }
}


EventNotificationRequestHandler*
EventNotificationRequestHandler::create()
{
    return new EventNotificationRequestHandler(_pService);
}


void
EventNotificationRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    std::string sid;

    if (request.getMethod() == "NOTIFY") {
        std::stringstream headerString;
        request.write(headerString);
        std::istream& body = request.stream();
        std::string bodyString;
        Poco::StreamCopier::copyToString(body, bodyString);
        LOG(event, debug, "event notification request from: " + request.clientAddress().toString() + Poco::LineEnding::NEWLINE_DEFAULT + headerString.str() + bodyString);
        if (request.has("SID")) {
            sid = request.get("SID");
        }
        else {
        }
        EventMessageReader eventMessageReader(bodyString, _pService);
        eventMessageReader.stateVarValues();
        LOG(event, debug, "event notification request handled, sending response ...");
        response.send();
        LOG(event, debug, "event notification request response sent.");
    }
    else {
        LOG(event, warning, "unkown event request on notification listener coming from: " + request.clientAddress().toString());
    }
    LOG(event, debug, "event notification request handler finished.");
}


IconRequestHandler*
IconRequestHandler::create()
{
    return new IconRequestHandler(_pIcon);
}


void
IconRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    LOG(upnp, debug, "icon request from: " + request.getHost());

    response.sendBuffer(_pIcon->_buffer.data(), _pIcon->_buffer.size());
}


HttpSocket::HttpSocket() :
_isRunning(false)
{
}


HttpSocket::~HttpSocket()
{
//    delete _pHttpServer;
    // FIXME: segfault on deleting _pDeviceRequestHandlerFactory
    //delete _pDeviceRequestHandlerFactory;
}


void
HttpSocket::init()
{
    _pDeviceRequestHandlerFactory = new UpnpRequestHandlerFactory(this);
    Poco::Net::ServerSocket socket(0);
    _httpServerPort = socket.address().port();
    Poco::Net::HTTPServerParams* pParams = new Poco::Net::HTTPServerParams;
    _pHttpServer = new Poco::Net::HTTPServer(_pDeviceRequestHandlerFactory, socket, pParams);
}


void
HttpSocket::startServer()
{
    if (!_isRunning) {
        _pHttpServer->start();
        _isRunning = true;
        LOG(http, information, "server started on port: " + Poco::NumberFormatter::format(_httpServerPort));
    }
}


void
HttpSocket::stopServer()
{
    if (_isRunning) {
        _pHttpServer->stop();
        _isRunning = false;
        LOG(http, information, "server stopped on port: " + Poco::NumberFormatter::format(_httpServerPort));
    }
}


std::string
HttpSocket::getServerUri()
{
//    if (_isRunning) {
    Poco::Net::IPAddress validIpAddress = Net::NetworkInterfaceManager::instance()->getValidIpAddress();
    std::string validIpAddressString = validIpAddress.toString();
    if (validIpAddress.family() == Poco::Net::IPAddress::IPv6) {
        validIpAddressString = "[" + validIpAddressString + "]";
    }
    return "http://" + validIpAddressString + ":" + Poco::NumberFormatter::format(_httpServerPort) + "/";
//    }
//    else {
//        return "";
//    }
}


const std::string Socket::Null = "null";
const std::string Socket::Local = "local";
//const std::string Socket::Bus = "bus";
const std::string Socket::Public = "public";
const std::string Socket::PublicLocal = "publicLocal";

Socket::Socket()
{

}


Socket::~Socket()
{

}


void
Socket::initSockets()
{
    _ssdpSocket.init();
    _httpSocket.init();
}


void
Socket::registerHttpRequestHandler(std::string path, UpnpRequestHandler* requestHandler)
{
    LOG(http, debug, "registering request handler for path: " + path);
    _httpSocket._pDeviceRequestHandlerFactory->registerRequestHandler(path, requestHandler);
}


void
Socket::registerSsdpMessageHandler(const Poco::AbstractObserver& observer)
{
    _ssdpSocket.addObserver(observer);
}


void
Socket::setMode(const Mode& mode)
{
    if (mode == Null) {
        _ssdpSocket.setMode(SsdpSocket::NotConfigured);
    }
    else if (mode == Local) {
//        _ssdpSocket.setMode(SsdpSocket::Broadcast);
//        _ssdpSocket.setMode(SsdpSocket::SharedMemory);
        _ssdpSocket.setMode(SsdpSocket::LocalProcess);
    }
//    else if (mode == Bus) {
//        _ssdpSocket.setMode(SsdpSocket::Bus);
//    }
    else if (mode == Public) {
        _ssdpSocket.setMode(SsdpSocket::Multicast);
    }
    else if (mode == PublicLocal) {
        _ssdpSocket.setMode(SsdpSocket::LocalProcess | SsdpSocket::Multicast);
    }
}


void
Socket::startSsdp()
{
    _ssdpSocket.startListen();
}


void
Socket::stopSsdp()
{
    _ssdpSocket.stopListen();
}


void
Socket::startHttp()
{
    _httpSocket.startServer();
}


void
Socket::stopHttp()
{
    _httpSocket.stopServer();
}


std::string
Socket::getHttpServerUri()
{
    return _httpSocket.getServerUri();
}


void
Socket::sendSsdpMessage(const Poco::AutoPtr<SsdpMessage>& pSsdpMessage, const Poco::Net::SocketAddress& receiver)
{
    _ssdpSocket.sendMessage(pSsdpMessage, receiver);
}


void
Socket::sendSsdpMessageSet(SsdpMessageSet& ssdpMessageSet, int repeat, long delay, const Poco::Net::SocketAddress& receiver)
{
    ssdpMessageSet.send(_ssdpSocket, repeat, delay, 0, false, receiver);
}


void
Socket::startSendSsdpMessageSet(SsdpMessageSet& ssdpMessageSet)
{
    ssdpMessageSet.startSendContinuous(_ssdpSocket);
}


void
Socket::stopSendSsdpMessageSet(SsdpMessageSet& ssdpMessageSet)
{
    ssdpMessageSet.stopSendContinuous();
}


//void
//Socket::handleNetworkInterfaceChangedNotification(Net::NetworkInterfaceNotification* pNotification)
//{
//    LOG(upnp, debug, "socket receives network interface change notification");
//    Mode mode = getMode();
//    if (mode == Null) {
//        _ssdpSocket.setMode(SsdpSocket::Broadcast);
//    }
////    handleNetworkInterfaceChange(pNotification->_interfaceName, pNotification->_added);
//}


//void
//Socket::handleNetworkInterfaceChange(const std::string& interfaceName, bool added)
//{
//    LOG(upnp, debug, "network listener adds network interface: " + interfaceName);
////    writeSsdpMessages();
//    if (added) {
//        _ssdpSocket.addInterface(interfaceName);
//        // TODO: send alive messages only once at startup of server
//        // (now triggered by loopback device and real network device)
////        sendSsdpAliveMessages();
//    }
//    else {
//        _ssdpSocket.removeInterface(interfaceName);
//    }
//}


std::string&
DescriptionProvider::getDeviceDescription()
{
    return *_pDeviceDescription;
}


std::string&
DescriptionProvider::getServiceDescription(const std::string& path)
{
    return *_serviceDescriptions[path];
}


const std::string DeviceManager::Transitioning = "transitioning";
const std::string DeviceManager::Stopped = "stopped";
const std::string DeviceManager::Local = "local";
//const std::string DeviceManager::Bus = "bus";
const std::string DeviceManager::Public = "public";
const std::string DeviceManager::PublicLocal = "publicLocal";

DeviceManager::DeviceManager(Socket* pSocket) :
_pSocket(pSocket),
_state(Stopped)
{
}


DeviceManager::~DeviceManager()
{
//    for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
//        delete *it;
//    }
//    _deviceContainers.deepClear();
    delete _pSocket;
}


void
DeviceManager::registerDeviceNotificationHandler(const Poco::AbstractObserver& observer)
{
    _deviceNotificationCenter.addObserver(observer);
}


void
DeviceManager::registerHttpRequestHandler(std::string path, UpnpRequestHandler* requestHandler)
{
    _pSocket->registerHttpRequestHandler(path, requestHandler);
}


void
DeviceManager::handleNetworkInterfaceChangedNotification(const Poco::AutoPtr<Net::NetworkInterfaceNotification>& pNotification)
{
    LOG(upnp, debug, "device manager receives network interface change notification");

    if (Net::NetworkInterfaceManager::instance()->loopbackOnly()) {
        _pSocket->setMode(Socket::Local);
        startSsdp();
    }
    else if (Net::NetworkInterfaceManager::instance()->interfaceCount() == 0) {
        _pSocket->setMode(Socket::Null);
        stopSsdp();
    }
    else {
        _pSocket->setMode(Socket::Public);
        startSsdp();
    }
}


DeviceManager::DeviceContainerIterator
DeviceManager::beginDeviceContainer()
{
    return _deviceContainers.begin();
}


DeviceManager::DeviceContainerIterator
DeviceManager::endDeviceContainer()
{
    return _deviceContainers.end();
}


void
DeviceManager::addDeviceContainer(DeviceContainer* pDeviceContainer)
{
    // TODO: handle "alive refreshments"
    // TODO: handle subdevices
    std::string uuid = pDeviceContainer->getRootDevice()->getUuid();
    if (!_deviceContainers.contains(uuid)) {
        LOG(upnp, debug, "device manager adds device container with root device uuid: " + uuid);
        _deviceContainers.append(uuid, pDeviceContainer);
        pDeviceContainer->setDeviceManager(this);
    }
//    _devices.get("").eventHandler(0);
}


void
DeviceManager::removeDeviceContainer(DeviceContainer* pDeviceContainer)
{
    std::string uuid = pDeviceContainer->getRootDevice()->getUuid();
    if (_deviceContainers.contains(uuid)) {
        LOG(upnp, debug, "device manager removes device container with root device uuid: " + uuid);
        _deviceContainers.remove(uuid);
    }
}


int
DeviceManager::getDeviceContainerCount()
{
    return _deviceContainers.size();
}


void
DeviceManager::init()
{
    // FIXME: *this is wrong, should point to DeviceContainer
//    Net::NetworkInterfaceManager::instance()->registerInterfaceChangeHandler
//        (Poco::Observer<DeviceContainer,Net::NetworkInterfaceNotification>(*this, &DeviceContainer::handleNetworkInterfaceChangedNotification));
//    LOG(upnp, debug, "device root network interface manager installed");

    _pSocket->initSockets();
    _pSocket->registerSsdpMessageHandler(Poco::NObserver<DeviceManager, SsdpMessage>(*this, &DeviceManager::handleSsdpMessage));
    Net::NetworkInterfaceManager::instance()->registerInterfaceChangeHandler(Poco::NObserver<DeviceManager, Net::NetworkInterfaceNotification>(*this, &DeviceManager::handleNetworkInterfaceChangedNotification));
}


void
DeviceManager::setState(State newState)
{
    LOG(upnp, debug, "device manager state change: " + _state + " -> " + newState);
    if (_state == newState) {
        LOG(upnp, debug, "new state equal to old state, ignoring");
        return;
    }
    State oldState = _state;
//    _stateLock.lock();
//    _state == Transitioning;
//    _stateLock.unlock();

    if (newState == Stopped) {
        stopSsdp();
        stopHttp();
        _pSocket->setMode(Socket::Null);
    }
    else {
        if (oldState != Stopped) {
            stopSsdp();
            stopHttp();
        }
        if (newState == Public) {
            _pSocket->setMode(Socket::Public);
        }
//        else if (newState == Bus) {
//            _pSocket->setMode(Socket::Bus);
//        }
        else if (newState == Local) {
            _pSocket->setMode(Socket::Local);
        }
        else if (newState == PublicLocal) {
            _pSocket->setMode(Socket::PublicLocal);
        }
        startHttp();
        startSsdp();
    }
    _stateLock.lock();
    _state = newState;
    _stateLock.unlock();
    LOG(upnp, debug, "device manager state change finished");
}


DeviceManager::State
DeviceManager::getState()
{
    Poco::ScopedLock<Poco::FastMutex> _lock(_stateLock);

    return _state;
}


std::string
DeviceManager::getHttpServerUri()
{
    return _pSocket->getHttpServerUri();
}


void
DeviceManager::startSsdp()
{
    LOG(ssdp, information, "starting socket ...");
    _pSocket->startSsdp();
    LOG(ssdp, information, "socket started.");
}


void
DeviceManager::stopSsdp()
{
    LOG(ssdp, information, "stopping socket ...");
    _pSocket->stopSsdp();
    LOG(ssdp, information, "socket stopped.");
}


void
DeviceManager::startHttp()
{
    LOG(http, information, "starting socket...");
    _pSocket->startHttp();
    LOG(http, information, "socket started.");
}


void
DeviceManager::stopHttp()
{
    LOG(http, information, "stopping socket...");
    _pSocket->stopHttp();
    LOG(http, information, "socket stopped.");
}


//void
//DeviceManager::clear()
//{
//    LOG(upnp, debug, "device manager clear");
//    // TODO: also delete device containers and devices. DeviceGroup only clears _devices vector (pointing to same devices)
//    // other possibility: sync device containers with current network status (and for example preserve info in controller about them)
////    for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
////        (*it)->stopSsdpTimer();
////    }
////    _deviceContainers.clear();
////    _deviceContainers.deepClear();
//}


void
DeviceManager::postDeviceNotification(Poco::Notification* pNotification)
{
    LOG(upnp, debug, "posting device notification to device manager");
    _deviceNotificationCenter.postNotification(pNotification);
}


Poco::AtomicCounter DeviceContainer::_ssdpTimerReleaseCounter(0);

DeviceContainer::DeviceContainer() :
_pDeviceManager(0),
_pController(0),
_pSsdpNotifyAliveMessages(new SsdpMessageSet),
_pSsdpNotifyByebyeMessages(new SsdpMessageSet),
_expireSsdpCallback(*this, &DeviceContainer::expireSsdp),
_pSsdpTimer(0)
{
}


DeviceContainer::~DeviceContainer()
{
    // TODO: free all Devices, Services, Actions, ...
    for (DeviceIterator it = beginDevice(); it != endDevice(); ++it) {
        (*it)->deepDelete();
        delete *it;
    }
//     delete _descriptionRequestHandler;
    delete _pSsdpNotifyAliveMessages;
    delete _pSsdpNotifyByebyeMessages;
    if (_pSsdpTimer) {
        delete _pSsdpTimer;
    }
}


//Service*
//DeviceContainer::getServiceType(const std::string& serviceType)
//{
//    std::map<std::string,Service*>::iterator i = _serviceTypes.find(serviceType);
//    if (i == _serviceTypes.end()) {
//        LOG(upnp, error, "unknown service type: " + serviceType);
//    }
//    return _serviceTypes[serviceType];
//}


void
DeviceContainer::addDevice(Device* pDevice)
{
    _devices.append(pDevice->getUuid(), pDevice);
    pDevice->setDeviceContainer(this);
    for (Device::ServiceIterator it = pDevice->beginService(); it != pDevice->endService(); ++it) {
        addServiceType(*it);
    }
}


void
DeviceContainer::clear()
{
    // TODO: this is incomplete and currently not usable
//    _devices.clear();
    _serviceTypes.clear();
    _pRootDevice = 0;
    if (_pDeviceDescription) {
        delete _pDeviceDescription;
        _pDeviceDescription = 0;
    }
    if (_pSsdpNotifyByebyeMessages) {
        delete _pSsdpNotifyByebyeMessages;
        _pSsdpNotifyByebyeMessages = 0;
    }
    if (_pSsdpNotifyAliveMessages) {
        delete _pSsdpNotifyAliveMessages;
        _pSsdpNotifyAliveMessages = 0;
    }
}


void
DeviceContainer::replaceDevice(Device* pOldDevice, Device* pNewDevice)
{
    // set pointers to nodes up and down the device tree.
    pNewDevice->setDeviceContainer(pOldDevice->getDeviceContainer());
    pNewDevice->setDeviceData(pOldDevice->getDeviceData());
    // set the reverse pointers pointing to pNewDevice.
    pOldDevice->getDeviceData()->setDevice(pNewDevice);
    _devices.replace(pOldDevice->getUuid(), pNewDevice);
    if (pOldDevice->getUuid() == _pRootDevice->getUuid()) {
        _pRootDevice = pNewDevice;
    }
    delete pOldDevice;
}


DeviceContainer::DeviceIterator
DeviceContainer::beginDevice()
{
    return _devices.begin();
}


DeviceContainer::DeviceIterator
DeviceContainer::endDevice()
{
    return _devices.end();
}


DeviceContainer::ServiceTypeIterator
DeviceContainer::beginServiceType()
{
    return _serviceTypes.begin();
}


DeviceContainer::ServiceTypeIterator
DeviceContainer::endServiceType()
{
    return _serviceTypes.end();
}


DeviceManager*
DeviceContainer::getDeviceManager()
{
    return _pDeviceManager;
}


Device*
DeviceContainer::getDevice(std::string uuid)
{
    return &_devices.get(uuid);
}


int
DeviceContainer::getDeviceCount()
{
    return _devices.size();
}


Device*
DeviceContainer::getRootDevice() const
{
    return _pRootDevice;
}


Controller*
DeviceContainer::getController() const
{
    return _pController;
}


std::string*
DeviceContainer::getDeviceDescription() const
{
    return _pDeviceDescription;
}


const std::string&
DeviceContainer::getDescriptionUri() const
{
    return _stringDescriptionUri;
}


std::string
DeviceContainer::getDescriptionPath() const
{
    return _descriptionUri.getPath();
}


void
DeviceContainer::setDeviceManager(DeviceManager* pDeviceManager)
{
    _pDeviceManager = pDeviceManager;
}


void
DeviceContainer::setRootDevice(Device* pDevice)
{
//    LOG(upnp, debug, "set root device: " + pDevice->getUuid());
    _pRootDevice = pDevice;
}


void
DeviceContainer::setDeviceDescription(std::string& description)
{
    _pDeviceDescription = &description;
}


void
DeviceContainer::setDescriptionUri(const std::string uri)
{
    _stringDescriptionUri = uri;
    try {
        _descriptionUri = _stringDescriptionUri;
    }
    catch(Poco::Exception& e) {
        LOG(http, error, "could not parse description URI: " + e.displayText());
    }
    LOG(upnp, debug, "set description URI finished.");
}


void
DeviceContainer::addServiceType(Service* pService)
{
    _serviceTypes[pService->getServiceTypeFullString()] = pService;
}


//void
//DeviceContainer::print()
//{
//    for(DeviceIterator d = beginDevice(); d != endDevice(); ++d) {
//        std::cout << "DeviceType: " << (*d)->getDeviceTypeFullString() << std::endl;
//        for(Device::ServiceIterator s = (*d)->beginService(); s != (*d)->endService(); ++s) {
//            std::cout << "    Service pointer: " << *s << std::endl;
//            std::cout << "    ServiceType: " << (*s)->getServiceTypeFullString() << std::endl;
//            int stateVarCount = 0;
//            for(Service::StateVarIterator v = (*s)->beginStateVar(); v != (*s)->endStateVar(); ++v) {
//                stateVarCount++;
//                std::string val;
//                (*v)->getValue(val);
//                std::cout
//                    << "        StateVar: " << (*v)->getName() << std::endl
//                    << "          number: " << stateVarCount << std::endl
//                    << "            type: " << (*v)->getType() << std::endl
//                    << "         evented: " << (*v)->getSendEvents() << std::endl
//                    << "             val: " << val << std::endl;
//            }
//        }
//    }
//}


void
DeviceContainer::initUuid()
{
    for(DeviceIterator d = beginDevice(); d != endDevice(); ++d) {
        if ((*d)->getUuid() == "") {
            LOG(upnp, debug, "init device container: setting random uuid for device");
            (*d)->setRandomUuid();
        }
        // FIXME: this should override a base uri, if already present in the device description
//         (*d)->addProperty("URLBase", _httpSocket.getServerUri());
    }
}


//void
//DeviceContainer::initStateVars()
//{
//    LOG(upnp, debug, "init device container: init state vars");
//    for(DeviceIterator d = beginDevice(); d != endDevice(); ++d) {
//        (*d)->initStateVars();
//    }
//}


void
DeviceContainer::initDeviceDescriptionHandler()
{
    _descriptionRequestHandler = new DescriptionRequestHandler(_pDeviceDescription);

}


void
DeviceContainer::rewriteDescriptions()
{
    LOG(upnp, debug, "init device container: rewrite descriptions");
    for(DeviceIterator d = beginDevice(); d != endDevice(); ++d) {
        for(Device::ServiceIterator s = (*d)->beginService(); s != (*d)->endService(); ++s) {
            ServiceDescriptionWriter serviceDescriptionWriter;
            serviceDescriptionWriter.service(**s);
            (*s)->setServiceDescription(*serviceDescriptionWriter.write());
        }
    }
    DescriptionWriter descriptionWriter;
    descriptionWriter.deviceContainer(*this);
    setDeviceDescription(*descriptionWriter.write());
}


void
DeviceContainer::initDevice()
{
    LOG(upnp, debug, "init device container (device)");

    initUuid();

    // initialize http request paths.
    setDescriptionUri(_pDeviceManager->getHttpServerUri() + getRootDevice()->getUuid() +  "/Description.xml");
    for(DeviceIterator d = beginDevice(); d != endDevice(); ++d) {
        for(Device::IconIterator i = (*d)->beginIcon(); i != (*d)->endIcon(); ++i) {
            // prepend the already assigned icon request path with the device uuid.
            (*i)->setIconRequestPath("/" + (*d)->getUuid() + "/" + (*i)->getIconRequestPath());
        }
        for(Device::ServiceIterator s = (*d)->beginService(); s != (*d)->endService(); ++s) {
            std::string servicePathPrefix = "/" + (*d)->getUuid() + "/" + (*s)->getServiceTypeFullString();
            (*s)->setDescriptionPath(servicePathPrefix + "/Description.xml");
            (*s)->setControlPath(servicePathPrefix + "/Control");
            (*s)->setEventSubscriptionPath(servicePathPrefix + "/EventSubscription");
        }
    }

    rewriteDescriptions();
    initDeviceDescriptionHandler();
    writeSsdpMessages();

    // initialize http request handlers and state vars.
    _pDeviceManager->registerHttpRequestHandler(getDescriptionPath(), _descriptionRequestHandler);
    for(DeviceIterator d = beginDevice(); d != endDevice(); ++d) {
        (*d)->initStateVars();
        for(Device::IconIterator i = (*d)->beginIcon(); i != (*d)->endIcon(); ++i) {
            _pDeviceManager->registerHttpRequestHandler((*i)->getIconRequestPath(), new IconRequestHandler(*i));
        }
        for(Device::ServiceIterator s = (*d)->beginService(); s != (*d)->endService(); ++s) {
            // TODO: to be totally correct, all relative URIs should be resolved to base URI (=description uri)
            _pDeviceManager->registerHttpRequestHandler((*s)->getDescriptionPath(), new DescriptionRequestHandler((*s)->getDescription()));
            _pDeviceManager->registerHttpRequestHandler((*s)->getControlPath(), new ControlRequestHandler((*s)));
            _pDeviceManager->registerHttpRequestHandler((*s)->getEventSubscriptionPath(), new EventSubscriptionRequestHandler((*s)));
            (*s)->enableEventing();
        }
        (*d)->registerActionHandler(Poco::Observer<DevDeviceCode, Action>(*(*d)->_pDevDeviceCode, &DevDeviceCode::actionHandler));
    }
}


void
DeviceContainer::writeSsdpMessages()
{
    LOG(ssdp, debug, "writing messages ...");

    _pSsdpNotifyAliveMessages->clear();
    _pSsdpNotifyByebyeMessages->clear();

    SsdpMessageSetWriter aliveWriter(_pSsdpNotifyAliveMessages, SsdpMessageSetWriter::Alive);
    SsdpMessageSetWriter byebyeWriter(_pSsdpNotifyByebyeMessages, SsdpMessageSetWriter::ByeBye);
    aliveWriter.deviceContainer(this);
    byebyeWriter.deviceContainer(this);

    for(DeviceIterator d = beginDevice(); d != endDevice(); ++d) {
        LOG(ssdp, debug, "writing messages for device: " + (*d)->getDeviceTypeFullString());
        aliveWriter.device(*d);
        byebyeWriter.device(*d);
        for(Device::ServiceIterator s = (*d)->beginService(); s != (*d)->endService(); ++s) {
            LOG(ssdp, debug, "writing messages for service: " + (*s)->getServiceTypeFullString());
            aliveWriter.service(*s);
            byebyeWriter.service(*s);
        }
    }
    LOG(ssdp, debug, "writing messages finished.");
}


void
DeviceContainer::restartSsdpTimer(int maxAge)
{
    if (maxAge == 0) {
        return;
    }

    int maxAgeMillisec = maxAge * 1000;
    LOG(ssdp, debug, "device container restart ssdp expiration timer for max age milliseconds: " + Poco::NumberFormatter::format(maxAgeMillisec));

    if (_pSsdpTimer) {
        _pSsdpTimer->restart(maxAgeMillisec);
    }
    else {
        LOG(ssdp, debug, "create timer for device ssdp expiration");
        // NOTE: thread pool gets too large when subscriptions appear over a long period of time, so we have to
        // count released timers and don't allocate a new thread in the default thread pool, when there is a released one
        if (_ssdpTimerReleaseCounter <= 0) {
            Poco::ThreadPool::defaultPool().addCapacity(1);
        }
        else {
            _ssdpTimerReleaseCounter--;
        }
        _pSsdpTimer = new Poco::Timer;
        _pSsdpTimer->setStartInterval(maxAgeMillisec);
        try {
            _pSsdpTimer->start(_expireSsdpCallback);
        }
        catch (Poco::Exception& e) {
            LOG(ssdp, error, "failed to start ssdp expiration timer: " + e.displayText());
        }
    }
}


void
DeviceContainer::stopSsdpTimer()
{
    if (_pSsdpTimer) {
        LOG(ssdp, debug, "device container ssdp expiration timer stopping ...");
        _pSsdpTimer->stop();
        LOG(event, debug, "device container ssdp expiration timer stopped.");
    }
}


void
DeviceContainer::expireSsdp(Poco::Timer& timer)
{
    LOG(ssdp, debug, "device container ssdp expiration timer expired.");
    timer.restart(0);
    if (_pController) {
        _pController->removeDeviceContainer(this);
    }
}


Device::Device() :
_pDeviceContainer(0),
_pDeviceData(0),
_pDevDeviceCode(0),
_pCtlDeviceCode(0)
{
}


Device::~Device()
{
}


void
Device::deepDelete()
{
    if (_pDeviceData) {
        delete _pDeviceData;
        _pDeviceData = 0;
    }
    if (_pDevDeviceCode) {
        delete _pDevDeviceCode;
        _pDevDeviceCode = 0;
    }
    if (_pCtlDeviceCode) {
        delete _pCtlDeviceCode;
        _pCtlDeviceCode = 0;
    }
}


void
Device::setRandomUuid()
{
    Poco::UUIDGenerator uuidGenerator;
    _pDeviceData->_uuid = uuidGenerator.createRandom().toString();
}


void
Device::addIcon(Icon* pIcon)
{
    _pDeviceData->addIcon(pIcon);
}


void
Device::initStateVars()
{
    for(ServiceIterator s = beginService(); s != endService(); ++s) {
        _pDevDeviceCode->initStateVars(*s);
    }
}


void
Device::controllerSubscribeEventing()
{
    for(ServiceIterator s = beginService(); s != endService(); ++s) {
        if ((*s)->getControllerSubscribeEventing()) {
            (*s)->addEventCallbackPath(getUuid() + "/" + (*s)->getServiceTypeFullString() + "/EventNotification");
            _pDeviceContainer->getDeviceManager()->registerHttpRequestHandler((*s)->getEventCallbackPath(), new EventNotificationRequestHandler((*s)));

            (*s)->setSubscriptionDuration(EVENT_SUBSCRIPTION_DURATION);
            // TODO: event notifications should go into Device, after it is accepted and added to the controller
            // subscribe to event notifications
            try {
                (*s)->sendSubscriptionRequest();
            }
            catch (Poco::Exception& e) {
                LOG(event, error, "controller failed to initialize device while subscribing to events, ignoring: " + e.displayText());
            }
        }
    }
}


void
Device::controllerUnsubscribeEventing()
{
    LOG(event, debug, "controller unsubscribe eventing ...");
    for(ServiceIterator s = beginService(); s != endService(); ++s) {
        if ((*s)->getControllerSubscribeEventing()) {
            (*s)->stopSubscriptionExpirationTimer();
            try {
                (*s)->sendCancelSubscriptionRequest();
            }
            catch (Poco::Exception& e) {
                LOG(event, error, "controller failed to unsubscribe service type: " + (*s)->getServiceTypeFullString() + ", subscription path: " + (*s)->getEventSubscriptionPath() + ", error: " + e.displayText());
            }
            (*s)->deleteSubscription();
        }
    }
    LOG(event, debug, "controller unsubscribe eventing finished.");
}


void
Device::stopSubscriptionTimer()
{
    for (ServiceIterator it = beginService(); it != endService(); ++it) {
        (*it)->stopSubscriptionExpirationTimer();
    }
}


Device::ServiceIterator
Device::beginService()
{
    return _pDeviceData->_services.begin();
}


Device::ServiceIterator
Device::endService()
{
    return _pDeviceData->_services.end();
}


Device::PropertyIterator
Device::beginProperty()
{
    return _pDeviceData->_properties.beginKey();
}


Device::PropertyIterator
Device::endProperty()
{
    return _pDeviceData->_properties.endKey();
}


Device::IconIterator
Device::beginIcon()
{
    return _pDeviceData->_iconList.begin();
}


Device::IconIterator
Device::endIcon()
{
    return _pDeviceData->_iconList.end();
}


DeviceContainer*
Device::getDeviceContainer() const
{
//    LOG(upnp, debug, "get device container: " + Poco::NumberFormatter::format(_pDeviceContainer));
    return _pDeviceContainer;
}


DeviceData*
Device::getDeviceData() const
{
    return _pDeviceData;
}


DevDeviceCode*
Device::getDevDeviceCode() const
{
    return _pDevDeviceCode;
}


CtlDeviceCode*
Device::getCtlDeviceCode() const
{
//    LOG(upnp, debug, "get device ctl code: " + Poco::NumberFormatter::format(_pCtlDeviceCode));
    return _pCtlDeviceCode;
}


std::string
Device::getUuid() const
{
    return _pDeviceData->_uuid;
}


std::string
Device::getDeviceTypeFullString() const
{
    return _pDeviceData->_deviceType;
}


const std::string&
Device::getFriendlyName()
{
    return getProperty("friendlyName");
}


Service*
Device::getServiceForTypeFullString(std::string serviceType)
{
    Service* pRes = 0;
    try {
        pRes = &_pDeviceData->_services.get(serviceType);
    }
    catch (...) {
        LOG(upnp, debug, "service not available: " + serviceType);
    }
    return pRes;
}


const std::string&
Device::getProperty(const std::string& name)
{
    return _pDeviceData->_properties.get(name);
}


void
Device::setDeviceContainer(DeviceContainer* pDeviceContainer)
{
    LOG(upnp, debug, "device, set device container to: " + Poco::NumberFormatter::format(pDeviceContainer));
    _pDeviceContainer = pDeviceContainer;
}


void
Device::setDeviceData(DeviceData* pDeviceData)
{
//    LOG(upnp, debug, "device, set device data to: " + Poco::NumberFormatter::format(pDeviceData));
    _pDeviceData = pDeviceData;
    pDeviceData->setDevice(this);
}


void
Device::setDevDeviceCode(DevDeviceCode* pDevDevice)
{
    _pDevDeviceCode = pDevDevice;
}


void
Device::setCtlDeviceCode(CtlDeviceCode* pCtlDevice)
{
//    LOG(upnp, debug, "device, set device controller code to: " + Poco::NumberFormatter::format(pCtlDevice));
    _pCtlDeviceCode = pCtlDevice;
}


void
Device::setUuid(std::string uuid)
{
    _pDeviceData->setUuid(uuid);
}


void
Device::setProperty(const std::string& name, const std::string& val)
{
    _pDeviceData->_properties.get(name) = val;
}


void
Device::setFriendlyName(const std::string& name)
{
    setProperty("friendlyName", name);
}


void
Device::addProperty(const std::string& name, const std::string& val)
{
    _pDeviceData->addProperty(name, val);
}


void
Device::addService(Service* pService)
{
    _pDeviceData->addService(pService);
    _pDeviceData->setDevice(this);
}


void
Device::postAction(Action* pAction)
{
    _notificationCenter.postNotification(pAction);
}


void
Device::registerActionHandler(const Poco::AbstractObserver& observer)
{
    _notificationCenter.addObserver(observer);
}


DeviceData::DeviceData() :
_pDevice(0)
{
}


DeviceData::~DeviceData()
{
    _services.deepClear();
}


Device*
DeviceData::getDevice()
{
//    LOG(upnp, debug, "device data, get device: " + Poco::NumberFormatter::format(_pDevice));
    return _pDevice;
}


void
DeviceData::setDeviceType(std::string deviceType)
{
    _deviceType = deviceType;
}


void
DeviceData::setUuid(std::string uuid)
{
    _uuid = uuid;
}


void
DeviceData::setDevice(Device* pDevice)
{
    _pDevice = pDevice;
}


void
DeviceData::addProperty(const std::string& name, const std::string& val)
{
    _properties.append(name, new std::string(val));
}


void
DeviceData::addService(Service* pService)
{
    _services.append(pService->getServiceTypeFullString(), pService);
    pService->setDeviceData(this);
}


void
DeviceData::addIcon(Icon* pIcon)
{
    _iconList.push_back(pIcon);
    pIcon->_requestPath = "DeviceIcon" + Poco::NumberFormatter::format(_iconList.size());
}


DevDeviceCode::DevDeviceCode() :
_pDevice(0)
{
};


DevDeviceCode::~DevDeviceCode()
{
}


SsdpMessageSet::SsdpMessageSet() :
_pSsdpSocket(0),
_sendTimerIsRunning(false)
{
    _randomTimeGenerator.seed();
}


SsdpMessageSet::~SsdpMessageSet()
{
    clear();
}


void
SsdpMessageSet::clear()
{
    if (_continuous) {
        _sendTimer.stop();
    }
    // Poco::AutoPtr automatically handles deletion of the object it points to,
    // when it is deleted by the container
    _ssdpMessages.clear();
}


void
SsdpMessageSet::addMessage(SsdpMessage* pMessage)
{
    _ssdpMessages.push_back(pMessage);
}


void
SsdpMessageSet::send(SsdpSocket& socket, int repeat, long delay, Poco::UInt16 cacheDuration, bool continuous, const Poco::Net::SocketAddress& receiver)
{
    if (_ssdpMessages.size() == 0) {
        return;
    }
    Poco::ScopedLock<Poco::FastMutex> lock(_sendTimerLock);
    // check if continuous Timer is already running and if so then cancel sending of message set and return.
     if (_sendTimerIsRunning) {
         LOG(ssdp, warning, "send timer for ssdp message set is already running, sending cancelled.");
         return;
     }
    _pSsdpSocket = &socket;
    _repeat = repeat;
    _delay = delay;
    _cacheDuration = cacheDuration;
    _continuous = continuous;
    _receiver = receiver;
    if (_continuous) {
        if (_delay > 0) {
            _sendTimer.setStartInterval(_randomTimeGenerator.next(_delay));
        }
        // if continuous, start a timer thread that restarts itself after random delays within cacheDuration.
        _sendTimerIsRunning = true;
        _sendTimer.start(Poco::TimerCallback<SsdpMessageSet>(*this, &SsdpMessageSet::onTimer));
    }
    else {
        // if not continuous, no thread is started, we sleep for delay milliseconds and send out
        // the ssdp messages synchronously. This way, send returns when the message set is sent out
        // and the message set can be a temporary (e.g. must not be stored on the heap). This is the case
        // when a device responds to an msearch from a controller.
        if (_delay > 0) {
            Poco::Thread::sleep(_randomTimeGenerator.next(_delay));
        }
        onTimer(_sendTimer);
    }
}


void
SsdpMessageSet::startSendContinuous(SsdpSocket& socket, long delay, Poco::UInt16 cacheDuration)
{
    // ssdp messages are sent within the first half of cache duration: cacheDuration * 1000 / 2
    send(socket, 2, delay, cacheDuration * 1000 / 2, true);
}


void
SsdpMessageSet::stopSendContinuous()
{
     Poco::ScopedLock<Poco::FastMutex> lock(_sendTimerLock);
    _sendTimer.stop();
    _sendTimerIsRunning = false;
}


void
SsdpMessageSet::onTimer(Poco::Timer& timer)
{
    int r = _repeat;
    while (r--) {
        LOG(ssdp, debug, "#message sets left to send: " + Poco::NumberFormatter::format(r+1));

        for (MessageSetT::const_iterator i = _ssdpMessages.begin(); i != _ssdpMessages.end(); ++i) {
            _pSsdpSocket->sendMessage(*i, _receiver);
        }
    }
    if (_continuous && _cacheDuration) {
        timer.restart(std::max(_delay, _randomTimeGenerator.next(_cacheDuration)));
    }
}


void
CtlDeviceCode::init()
{
    for (Device::ServiceIterator i = _pDevice->beginService(); i != _pDevice->endService(); ++i) {
        (*i)->initController();
    }
}


const std::string Controller::SearchTargetAll("ssdp:all");
const std::string Controller::SearchTargetRootDevice("upnp:rootdevice");
const std::string Controller::SearchTargetUpnpDeviceType("urn:schemas-upnp-org:device:");
const std::string Controller::SearchTargetUpnpServiceType("urn:schemas-upnp-org:service:");
const std::string Controller::SearchTargetUuid("uuid:");
const std::string Controller::SearchTargetUrn("urn:");


Controller::Controller() :
DeviceManager(new Socket),
_featureSubscribeToEvents(true)
{
}


Controller::~Controller()
{
}


void
Controller::setState(State newState)
{
    LOG(upnp, debug, "controller state change: " + _state + " -> " + newState);
    if (_state == newState) {
        LOG(upnp, debug, "new state equal to old state, ignoring");
        return;
    }
    _stateLock.lock();
    _state = Transitioning;
    _stateLock.unlock();

    if (newState == Public || newState == Local || newState == PublicLocal) {
        DeviceManager::setState(newState);
    }
    else if (newState == Stopped) {
        stopDeviceContainerTimers();
        unsubscribeAllDevices();
        DeviceManager::setState(Stopped);
        deleteDeviceContainers();
//        DeviceManager::clear();
    }
    LOG(upnp, debug, "controller state change finished");
}


void
Controller::registerDeviceGroup(DeviceGroup* pDeviceGroup)
{
    pDeviceGroup->_pController = this;
    _deviceGroups[pDeviceGroup->getDeviceType()] = pDeviceGroup;
    pDeviceGroup->init();
    pDeviceGroup->initDelegate();
}


DeviceGroup*
Controller::getDeviceGroup(const std::string& deviceType)
{
    std::map<std::string, DeviceGroup*>::iterator it = _deviceGroups.find(deviceType);
    if (it != _deviceGroups.end()) {
        return (*it).second;
    }
    else {
        return 0;
    }
}


void
Controller::startSsdp()
{
    LOG(upnp, debug, "controller start sending ssdp messages");
    DeviceManager::startSsdp();
//    sendMSearch();

//    sendMSearch(SearchTargetAll);
    sendMSearch(SearchTargetRootDevice);
//    sendMSearch(SearchTargetUuid + "84944fac-9a5e-42fb-bc3c-09edb315f292");  // omm-server-music.sh
//    sendMSearch(SearchTargetUuid + "636719d1-0081-4538-b1df-2bc6d625104b");  // win media server on tristan mini
//    sendMSearch(SearchTargetUpnpDeviceType + "MediaServer:1");
//    sendMSearch(SearchTargetUpnpServiceType + "ContentDirectory:1");
}


void
Controller::sendMSearch(const std::string& searchTarget)
{
    // FIXME: network exception in controller when sending MSearch after network device removal
    LOG(upnp, debug, "controller sends M-SEARCH");

    Poco::AutoPtr<SsdpMessage> pM = new SsdpMessage(SsdpMessage::REQUEST_SEARCH);
    pM->setSearchTarget(searchTarget);
    _pSocket->sendSsdpMessage(pM);
}


void
Controller::discoverDeviceContainer(const std::string& location)
{
    LOG(upnp, debug, "controller discovers device location: " + location);

    UriDescriptionReader descriptionReader;
    descriptionReader.getDeviceDescription(location);
    DeviceContainer* pDeviceContainer = descriptionReader.deviceContainer();
    pDeviceContainer->setDescriptionUri(location);
    addDeviceContainer(pDeviceContainer);
}


void
Controller::subscribeAllDevicesInContainer(DeviceContainer* pDeviceContainer)
{
    if (!_featureSubscribeToEvents) {
        return;
    }
    for(DeviceContainer::DeviceIterator d = pDeviceContainer->beginDevice(); d != pDeviceContainer->endDevice(); ++d) {
        (*d)->controllerSubscribeEventing();
    }
}


void
Controller::unsubscribeAllDevices()
{
    if (!_featureSubscribeToEvents) {
        return;
    }
    for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
        for(DeviceContainer::DeviceIterator d = (*it)->beginDevice(); d != (*it)->endDevice(); ++d) {
            (*d)->controllerUnsubscribeEventing();
        }
    }
}


void
Controller::stopDeviceContainerTimers()
{
    for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
        (*it)->stopSsdpTimer();
    }
}


void
Controller::deleteDeviceContainers()
{
    _deviceContainers.deepClear();
}


void
Controller::handleSsdpMessage(const Poco::AutoPtr<SsdpMessage>& pMessage)
{
    if (getState() == Transitioning) {
        return;
    }
    // we load all device descriptions, regardless of service types contained in the device

    // get UUID from USN
    std::string usn = pMessage->getUniqueServiceName();
    std::string::size_type left = usn.find(":") + 1;
    std::string uuid = usn.substr(left, usn.find("::") - left);
    LOG(ssdp, debug, "controller received message:" + Poco::LineEnding::NEWLINE_DEFAULT + pMessage->toString());

    switch(pMessage->getRequestMethod()) {
    case SsdpMessage::REQUEST_NOTIFY:
        LOG(ssdp, debug, "identified notification request");
        switch(pMessage->getNotificationSubtype()) {
        case SsdpMessage::SUBTYPE_ALIVE:
            LOG(ssdp, debug, "identified alive message");
            // TODO: handle other notification types than upnp:rootdevice
            if (pMessage->getNotificationType() == "upnp:rootdevice" && !_deviceContainers.contains(uuid)) {
                LOG(ssdp, debug, "identified alive message, attempting to add device container");
                discoverDeviceContainer(pMessage->getLocation());
            }
            if (pMessage->getNotificationType() == "upnp:rootdevice") {
                _deviceContainers.get(uuid).restartSsdpTimer(pMessage->getCacheControl());
            }
            break;
        case SsdpMessage::SUBTYPE_BYEBYE:
            LOG(ssdp, debug, "identified byebye message");
            // TODO: handle other notification types than upnp:rootdevice
            if (pMessage->getNotificationType() == "upnp:rootdevice" && _deviceContainers.contains(uuid)) {
                LOG(ssdp, debug, "identified byebye message, attempting to remove device container");
                _deviceContainers.get(uuid).stopSsdpTimer();
                removeDeviceContainer(&_deviceContainers.get(uuid));
            }
            break;
        }
    break;
    case SsdpMessage::REQUEST_RESPONSE:
        LOG(ssdp, debug, "identified m-search response");
        if (!_deviceContainers.contains(uuid)) {
            discoverDeviceContainer(pMessage->getLocation());
        }
        _deviceContainers.get(uuid).restartSsdpTimer(pMessage->getCacheControl());
        break;
    }
}


void
Controller::addDeviceContainer(DeviceContainer* pDeviceContainer)
{
    std::string uuid = pDeviceContainer->getRootDevice()->getUuid();
    LOG(upnp, debug, "controller add device container with uuid: " + uuid);
    if (!_deviceContainers.contains(uuid)) {
        addDeviceContainer(pDeviceContainer, _deviceContainers.size(), true);
        DeviceManager::addDeviceContainer(pDeviceContainer);
        pDeviceContainer->_pController = this;

        for (DeviceContainer::DeviceIterator it = pDeviceContainer->beginDevice(); it != pDeviceContainer->endDevice(); ++it) {
            Device* pDevice = *it;
            LOG(upnp, debug, "controller discovers device of type: " + pDevice->getDeviceTypeFullString() + ", friendly name: " + pDevice->getFriendlyName() + ", uuid: " + pDevice->getUuid());
            DeviceGroup* pDeviceGroup = getDeviceGroup(pDevice->getDeviceTypeFullString());
            if (pDeviceGroup) {
                Device* pTypedDevice = pDeviceGroup->createDevice();
                pDeviceContainer->replaceDevice(pDevice, pTypedDevice);

                LOG(upnp, information, "controller adds device, friendly name: " + pTypedDevice->getFriendlyName() + ", uuid: " + pTypedDevice->getUuid());
                pTypedDevice->addCtlDeviceCode();
//                if (_featureSubscribeToEvents) {
//                    pTypedDevice->controllerSubscribeEventing();
//                }
                pTypedDevice->initController();

                pDeviceGroup->addDevice(pTypedDevice);
                LOG(upnp, debug, "controller add device finished, friendly name: " + pTypedDevice->getFriendlyName() + ", uuid: " + pTypedDevice->getUuid());
            }
        }

        addDeviceContainer(pDeviceContainer, _deviceContainers.size() - 1, false);
        subscribeAllDevicesInContainer(pDeviceContainer);
    }
    else {
        LOG(upnp, debug, "controller already knows device (ignoring): " + uuid);
    }
}


void
Controller::removeDeviceContainer(DeviceContainer* pDeviceContainer)
{
    LOG(upnp, debug, "remove device container");
    std::string uuid = pDeviceContainer->getRootDevice()->getUuid();
    LOG(upnp, debug, "remove device container with root device uuid: " + uuid);
    if (_deviceContainers.contains(uuid)) {
        DeviceContainer* pDeviceContainer = &_deviceContainers.get(uuid);
        int position = _deviceContainers.position(uuid);
        removeDeviceContainer(pDeviceContainer, position, true);
        DeviceManager::removeDeviceContainer(pDeviceContainer);

        for (DeviceContainer::DeviceIterator it = pDeviceContainer->beginDevice(); it != pDeviceContainer->endDevice(); ++it) {
            Device* pDevice = *it;
            pDevice->stopSubscriptionTimer();
            DeviceGroup* pDeviceGroup = getDeviceGroup(pDevice->getDeviceTypeFullString());
            if (pDeviceGroup) {
                LOG(upnp, information, "controller removes device, friendly name: " + pDevice->getFriendlyName() + ", uuid: " + pDevice->getUuid());
                pDeviceGroup->removeDevice(pDevice);
            }
        }

        removeDeviceContainer(pDeviceContainer, position, false);
        pDeviceContainer->_pController = 0;
    }
}


//void
//Controller::update()
//{
//    // TODO: do a more carefull controller update and don't remove servers that are still active.
//    for (Container<DeviceContainer>::KeyIterator it = _deviceContainers.beginKey(); it != _deviceContainers.endKey(); ++it) {
////        removeDeviceContainer((*it).first);
//    }
//    sendMSearch();
//}


DeviceServer::DeviceServer() :
DeviceManager(new Socket)
{
}


DeviceServer::~DeviceServer()
{
}


void
DeviceServer::init()
{
    DeviceManager::init();
    for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
        (*it)->initDevice();
    }
}


void
DeviceServer::setState(State newState)
{
    LOG(upnp, debug, "device server state change: " + _state + " -> " + newState);
    if (_state == newState) {
        LOG(upnp, debug, "new state equal to old state, ignoring");
        return;
    }

    if (newState == Stopped) {
        for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
            for(DeviceContainer::DeviceIterator d = (*it)->beginDevice(); d != (*it)->endDevice(); ++d) {
                (*d)->stopSubscriptionTimer();
                (*d)->stop();
            }
        }
        DeviceManager::setState(Stopped);
    }
    else {
        if (_state != Stopped) {
            for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
                for(DeviceContainer::DeviceIterator d = (*it)->beginDevice(); d != (*it)->endDevice(); ++d) {
                    (*d)->stopSubscriptionTimer();
                    (*d)->stop();
                }
            }
        }
        if (newState == Public) {
            DeviceManager::setState(Public);
        }
//        else if (newState == Bus) {
//            DeviceManager::setState(Bus);
//        }
        else if (newState == Local) {
            DeviceManager::setState(Local);
        }
        for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
            for(DeviceContainer::DeviceIterator d = (*it)->beginDevice(); d != (*it)->endDevice(); ++d) {
                (*d)->start();
            }
        }
    }
    LOG(upnp, debug, "device server state change finished");
}


void
DeviceServer::startSsdp()
{
    LOG(ssdp, debug, "device server start sending ssdp messages");
    DeviceManager::startSsdp();
    for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
        // send advertisements once delayed for up to 100ms and continuously in random intervals
        // of max half the expiraton time (CACHE-CONTROL header)
        _pSocket->startSendSsdpMessageSet(*(*it)->_pSsdpNotifyAliveMessages);
    }
}


void
DeviceServer::stopSsdp()
{
    LOG(ssdp, debug, "device server stop sending ssdp messages");
    for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
        _pSocket->stopSendSsdpMessageSet(*(*it)->_pSsdpNotifyAliveMessages);
        // send device unavailable messages once with no delay
        _pSocket->sendSsdpMessageSet(*(*it)->_pSsdpNotifyByebyeMessages, 1, 0);
    }
    DeviceManager::stopSsdp();
}


void
DeviceServer::handleSsdpMessage(const Poco::AutoPtr<SsdpMessage>& pMessage)
{
    if (pMessage->getRequestMethod() == SsdpMessage::REQUEST_SEARCH) {
        LOG(ssdp, debug, "device server reply to m-search");
        for (DeviceContainerIterator it = beginDeviceContainer(); it != endDeviceContainer(); ++it) {
            std::string descriptionUri = (*it)->getDescriptionUri();
            std::string searchTarget = pMessage->getSearchTarget();
            Poco::StringTokenizer searchTargetFields(searchTarget, ":");
            bool respond = false;
            SsdpMessageSet responseMessageSet;

            if (searchTarget == "ssdp:all") {
                respond = true;
                SsdpMessageSetWriter msw(&responseMessageSet, SsdpMessageSetWriter::SearchAllResponse);
                msw.deviceContainer(*it);
                for(DeviceContainer::DeviceIterator d = (*it)->beginDevice(); d != (*it)->endDevice(); ++d) {
                    msw.device(*d);
                    for(Device::ServiceIterator s = (*d)->beginService(); s != (*d)->endService(); ++s) {
                        msw.service(*s);
                    }
                }
            }
            else {
//                Poco::AutoPtr<SsdpMessage> pM = new SsdpMessage(SsdpMessage::REQUEST_RESPONSE);
                SsdpMessage* pM = new SsdpMessage(SsdpMessage::REQUEST_RESPONSE);
                pM->setSearchTarget(searchTarget);
                pM->setLocation(descriptionUri);

                if (searchTarget == "upnp:rootdevice") {
                    respond = true;
                    pM->setUniqueServiceName("uuid:" + (*it)->getRootDevice()->getUuid() + "::" + searchTarget);
                }
                else if (searchTargetFields.count() == 2 && searchTargetFields[0] == "uuid") {
                    for (DeviceContainer::DeviceIterator dit = (*it)->beginDevice(); dit != (*it)->endDevice(); ++dit) {
                        if ((*dit)->getUuid() == searchTargetFields[1]) {
                            respond = true;
                            pM->setUniqueServiceName("uuid:" + (*dit)->getUuid());
                        }
                    }
                }
                else if (searchTargetFields.count() == 5 && searchTargetFields[0] == "urn" && searchTargetFields[1] == "schemas-upnp-org") {
                    if (searchTargetFields[2] == "device") {
                        for (DeviceContainer::DeviceIterator dit = (*it)->beginDevice(); dit != (*it)->endDevice(); ++dit) {
                            if ((*dit)->getDeviceTypeFullString() == searchTarget) {
                                respond = true;
                                pM->setUniqueServiceName("uuid:" + (*dit)->getUuid() + "::" + (*dit)->getDeviceTypeFullString());
                            }
                        }
                    }
                    else if (searchTargetFields[2] == "service") {
                        for (DeviceContainer::DeviceIterator dit = (*it)->beginDevice(); dit != (*it)->endDevice(); ++dit) {
                            Service* pService = (*dit)->getServiceForTypeFullString(searchTarget);
                            if (pService) {
                                respond = true;
                                pM->setUniqueServiceName("uuid:" + (*dit)->getUuid() + "::" + pService->getServiceTypeFullString());
                            }
                        }
                    }
                }
                if (respond) {
                    responseMessageSet.addMessage(pM);
                }
                else {
                    pM->release();
                }
            }
            if (respond) {
                // TODO: fill in the correct value for CacheControl
                //       -> _ssdpNotifyAliveMessages._sendTimer
                //       -> need to know the elapsed time ... (though initial timer val isn't so wrong)
                int mx = pMessage->getMaximumWaitTime();
                // MX greater than SSDP_MAX_WAIT_TIME then cut down to SSDP_MAX_WAIT_TIME
                mx = (mx > SSDP_MAX_WAIT_TIME) ? SSDP_MAX_WAIT_TIME : mx;
                // send out response delayed according to MX field of msearch request
                _pSocket->sendSsdpMessageSet(responseMessageSet, 1, mx * 1000, pMessage->getSender());
            }
        }
    }
}


void
SsdpMessageSetWriter::deviceContainer(const DeviceContainer* pDeviceContainer)
{
    // root device first message
    SsdpMessage* pM1 = createMessage();
    if (_type == Alive || _type == SearchAllResponse) {
        pM1->setLocation(pDeviceContainer->getDescriptionUri());    // location of UPnP description of the root device
    }
    if (_type == Alive || _type == ByeBye) {
        pM1->setNotificationType("upnp:rootdevice");  // once for root device
    }
    if (_type == SearchAllResponse) {
        pM1->setSearchTarget("upnp:rootdevice");  // once for root device
    }
    pM1->setUniqueServiceName("uuid:" + pDeviceContainer->getRootDevice()->getUuid() + "::upnp:rootdevice");
    _res->addMessage(pM1);
}


void
SsdpMessageSetWriter::device(const Device* pDevice)
{
     // device first and second message (root device second and third message)
    SsdpMessage* pM2 = createMessage();
    SsdpMessage* pM3 = createMessage();
    if (_type == Alive || _type == SearchAllResponse) {
        pM2->setLocation(pDevice->getDeviceContainer()->getDescriptionUri());    // location of UPnP description of the root device
        pM3->setLocation(pDevice->getDeviceContainer()->getDescriptionUri());    // location of UPnP description of the root device
    }
    if (_type == Alive || _type == ByeBye) {
        pM2->setNotificationType("uuid:" + pDevice->getUuid());
        pM3->setNotificationType(pDevice->getDeviceTypeFullString());
    }
    if (_type == SearchAllResponse) {
        pM2->setSearchTarget("uuid:" + pDevice->getUuid());
        pM3->setSearchTarget(pDevice->getDeviceTypeFullString());
    }
    pM2->setUniqueServiceName("uuid:" + pDevice->getUuid());
    pM3->setUniqueServiceName("uuid:" + pDevice->getUuid() + "::" + pDevice->getDeviceTypeFullString());
    _res->addMessage(pM2);
    _res->addMessage(pM3);
}


void
SsdpMessageSetWriter::service(const Service* pService)
{
    // service first (and only) message
    SsdpMessage* pM1 = createMessage();
    if (_type == Alive || _type == SearchAllResponse) {
        pM1->setLocation(pService->getDevice()->getDeviceContainer()->getDescriptionUri());    // location of UPnP description of the root device
    }
    if (_type == Alive || _type == ByeBye) {
        pM1->setNotificationType(pService->getServiceTypeFullString());
    }
    if (_type == SearchAllResponse) {
        pM1->setSearchTarget(pService->getServiceTypeFullString());
    }
    pM1->setUniqueServiceName("uuid:" + pService->getDevice()->getUuid() + "::" +  pService->getServiceTypeFullString());
    _res->addMessage(pM1);
}


SsdpMessage*
SsdpMessageSetWriter::createMessage()
{
    SsdpMessage* pRes = 0;
    switch (_type) {
        case Alive:
            pRes = new SsdpMessage(SsdpMessage::REQUEST_NOTIFY_ALIVE);
            break;
        case ByeBye:
            pRes = new SsdpMessage(SsdpMessage::REQUEST_NOTIFY_BYEBYE);
            break;
        case SearchAllResponse:
            pRes = new SsdpMessage(SsdpMessage::REQUEST_RESPONSE);
            break;
    }
    return pRes;
}


SsdpMessage::SsdpMessage(TRequestMethod requestMethod) :
_requestMethod(requestMethod)
{
    initMessageMap();

    switch (requestMethod) {
        case REQUEST_NOTIFY_BYEBYE:
            _requestMethod = REQUEST_NOTIFY;
            setNotificationSubtype(SsdpMessage::SUBTYPE_BYEBYE);
            setHost();
            break;
        case REQUEST_NOTIFY_ALIVE:
            _requestMethod = REQUEST_NOTIFY;
            setNotificationSubtype(SsdpMessage::SUBTYPE_ALIVE);
            setServer("Omm/" + OMM_VERSION);
            setCacheControl();
            setHost();
            break;
        case REQUEST_RESPONSE:
            setServer("Omm/" + OMM_VERSION);
            setHttpExtensionConfirmed();
            setCacheControl();
            setDate();
            break;
        case REQUEST_SEARCH:
            setHost();
            setHttpExtensionNamespace();
            setMaximumWaitTime();
            break;
        }
}


SsdpMessage::SsdpMessage(const std::string& buf, const Poco::Net::SocketAddress& sender)
{
    initMessageMap();

    // FIXME: what about Poco::Message header for retrieving the request method?
    std::istringstream is(buf);
    // get first line and check request method type
    std::string line;
    getline(is, line);
    Poco::trimInPlace(line);

    // TODO: must be case-insensitive map?!
    _requestMethod = _messageConstMap[line];
    _sender = sender;

    while(getline(is, line)) {
        std::string::size_type col = line.find(":");
        if (col != std::string::npos) {
            _messageHeader[Poco::toUpper(line.substr(0, col))] = Poco::trim(line.substr(col + 1));
        }
    }

//     if (getNotificationSubtype() == SsdpMessage::SUBTYPE_ALIVE) {
//             _requestMethod = REQUEST_NOTIFY_ALIVE;
//     }
//     else if (getNotificationSubtype() == SsdpMessage::SUBTYPE_BYEBYE) {
//         _requestMethod = REQUEST_NOTIFY_BYEBYE;
//     }
//     try {
//         _messageHeader.read(is);
//     } catch (Poco::Net::MessageException) {
//         std::clog << "Error in sdpMessage::SsdpMessage(): malformed header" << std::endl;
//     }
    // FIXME: workaround for bug in Poco: empty values are mistreated
//     if (_messageHeader.has("EXT")) {
//         _messageHeader.set("EXT", "fix bug in poco");
//         _messageHeader.erase("EXT");
//     }
}


void
SsdpMessage::initMessageMap()
{
    _messageMap[REQUEST_UNKNOWN]           = "";
    _messageMap[REQUEST_NOTIFY]            = "NOTIFY * HTTP/1.1";
    _messageMap[REQUEST_NOTIFY_ALIVE]      = "dummy1";
    _messageMap[REQUEST_NOTIFY_BYEBYE]     = "dummy2";
    _messageMap[REQUEST_SEARCH]            = "M-SEARCH * HTTP/1.1";
    _messageMap[REQUEST_RESPONSE]          = "HTTP/1.1 200 OK";
    _messageMap[SUBTYPE_ALIVE]             = "ssdp:alive";
    _messageMap[SUBTYPE_BYEBYE]            = "ssdp:byebye";
    _messageMap[SSDP_ALL]                  = "ssdp:all";
    _messageMap[UPNP_ROOT_DEVICES]         = "upnp:rootdevice";

    for (std::map<TRequestMethod,std::string>::iterator i = _messageMap.begin(); i != _messageMap.end(); i++) {
        _messageConstMap[(*i).second] = (*i).first;
    }
}


std::string
SsdpMessage::getTypeHeader(const TRequestMethod method) const
{
    std::map<TRequestMethod,std::string>::const_iterator pos = _messageMap.find(method);
    if (pos != _messageMap.end()) {
        return pos->second;
    }
    else {
        return "";
    }
}


SsdpMessage::TRequestMethod
SsdpMessage::getTypeHeaderConst(const std::string& method) const
{
    std::map<std::string,TRequestMethod>::const_iterator pos = _messageConstMap.find(method);
    if (pos != _messageConstMap.end()) {
        return pos->second;
    }
    else {
        return REQUEST_UNKNOWN;
    }
}


std::string
SsdpMessage::getHeader(const std::string& header) const
{
    std::map<std::string,std::string>::const_iterator pos = _messageHeader.find(header);
    if (pos != _messageHeader.end()) {
        return pos->second;
    }
    else {
        return "";
    }
}


SsdpMessage::~SsdpMessage()
{
}


std::string
SsdpMessage::toString() const
{
    std::string res = getTypeHeader(_requestMethod) + Poco::LineEnding::NEWLINE_DEFAULT;
    for (std::map<std::string,std::string>::const_iterator i = _messageHeader.begin(); i != _messageHeader.end(); ++i) {
        res += (*i).first + ": " + (*i).second + Poco::LineEnding::NEWLINE_DEFAULT;
    }
    return res + Poco::LineEnding::NEWLINE_DEFAULT;
}


SsdpMessage::TRequestMethod
SsdpMessage::getRequestMethod() const
{
    return _requestMethod;
}


void
SsdpMessage::setCacheControl(int duration)
{
    _messageHeader["CACHE-CONTROL"] = "max-age = " + Poco::NumberFormatter::format(duration);
}


int
SsdpMessage::getCacheControl() const
{
    int res = SSDP_CACHE_DURATION; // if no valid number is given, return minimum required value
    try {
        std::string value = getHeader("CACHE-CONTROL");
        res = Poco::NumberParser::parse(value.substr(value.find('=')+1));
    }
    catch (Poco::NotFoundException) {
        LOG(ssdp, error, "missing CACHE-CONTROL in ssdp header");
    }
    catch (Poco::SyntaxException) {
        LOG(ssdp, error, "wrong number format of CACHE-CONTROL in ssdp header");
    }
    return res;
}


void
SsdpMessage::setNotificationType(const std::string& searchTarget)
{
    _messageHeader["NT"] =  searchTarget;
}


std::string
SsdpMessage::getNotificationType() const
{
    return getHeader("NT");
}


void
SsdpMessage::setNotificationSubtype(TRequestMethod notificationSubtype)
{
    _messageHeader["NTS"] = _messageMap[notificationSubtype];
    _notificationSubtype = notificationSubtype;
}


SsdpMessage::TRequestMethod
SsdpMessage::getNotificationSubtype() const
{
    return getTypeHeaderConst(getHeader("NTS"));
}


void
SsdpMessage::setSearchTarget(const std::string& searchTarget)
{
    _messageHeader["ST"] = searchTarget;
}


std::string
SsdpMessage::getSearchTarget() const
{
    return getHeader("ST");
}


void
SsdpMessage::setUniqueServiceName(const std::string& serviceName)
{
    _messageHeader["USN"] = serviceName;
}


std::string
SsdpMessage::getUniqueServiceName() const
{
    return getHeader("USN");
}


void
SsdpMessage::setLocation(const std::string& location)
{
//     _messageHeader.set("LOCATION", location.toString());
    _messageHeader["LOCATION"] = location;
}


std::string
SsdpMessage::getLocation() const
{
    try {
        return getHeader("LOCATION");
    }
    catch (Poco::NotFoundException) {
        LOG(ssdp, error, "LOCATION field not found");
        return "";
    }
}


void
SsdpMessage::setHost()
{
//     _messageHeader.set("HOST", SSDP_FULL_ADDRESS);
    _messageHeader["HOST"] = SSDP_FULL_ADDRESS;
}


void
SsdpMessage::setHttpExtensionNamespace()
{
//     _messageHeader.set("MAN", "\"ssdp:discover\"");
    _messageHeader["MAN"] = "\"ssdp:discover\"";
}


void
SsdpMessage::setHttpExtensionConfirmed()
{
//     _messageHeader.set("EXT", "");
    _messageHeader["EXT"] = "";
}


bool
SsdpMessage::getHttpExtensionConfirmed() const
{
//     return _messageHeader.has("EXT");
    return true;
}


void
SsdpMessage::setServer(const std::string& productNameVersion)
{
//     _messageHeader.set("SERVER",
//                         Environment::osName() + "/"
//                         + Environment::osVersion() + ", "
//                         + "UPnP/" + UPNP_VERSION + ", "
//                         + productNameVersion);
    _messageHeader["SERVER"] =
        Poco::Environment::osName() + "/"
        + Poco::Environment::osVersion() + ", "
        + "UPnP/" + UPNP_VERSION + ", "
        + productNameVersion;
}


std::string
SsdpMessage::getServerOperatingSystem() const
{
    std::string value = getHeader("SERVER");
    std::vector<std::string> elements;
    Poco::Net::MessageHeader::splitElements(value, elements);
    return elements[0];
}


std::string
SsdpMessage::getServerUpnpVersion() const
{
    std::string value = getHeader("SERVER");
    std::vector<std::string> elements;
    Poco::Net::MessageHeader::splitElements(value, elements);
    std::string upnpVersion = elements[1];
    return upnpVersion.substr(upnpVersion.find('/')+1);
}


std::string
SsdpMessage::getServerProductNameVersion() const
{
    std::string value = getHeader("SERVER");
    std::vector<std::string> elements;
    Poco::Net::MessageHeader::splitElements(value, elements);
    return elements[2];
}


void
SsdpMessage::setMaximumWaitTime(int waitTime)
{
//     _messageHeader.set("MX", NumberFormatter::format(waitTime));
    _messageHeader["MX"] = Poco::NumberFormatter::format(waitTime);
}


int
SsdpMessage::getMaximumWaitTime() const
{
    try {
        return Poco::NumberParser::parse(getHeader("MX"));
    }
    catch (Poco::NotFoundException) {
        LOG(ssdp, error, "missing MX in SSDP header");

    }
    catch (Poco::SyntaxException) {
        LOG(ssdp, error, "wrong number format of MX in SSDP header");
    }
    return SSDP_MIN_WAIT_TIME;  // if no valid number is given, return minimum required value
}


void
SsdpMessage::setDate()
{
    Poco::Timestamp t;
//     _messageHeader.set("DATE", DateTimeFormatter::format(t, DateTimeFormat::HTTP_FORMAT));
    _messageHeader["DATE"] = Poco::DateTimeFormatter::format(t, Poco::DateTimeFormat::HTTP_FORMAT);
}


Poco::DateTime
SsdpMessage::getDate() const
{
    std::string value = getHeader("DATE");
    int timeZoneDifferentail;
    try {
        return Poco::DateTimeParser::parse(Poco::DateTimeFormat::HTTP_FORMAT, value, timeZoneDifferentail);
    }
    catch (Poco::SyntaxException) {
        LOG(ssdp, error, "wrong date format of DATE in SSDP header, replacing by current date");
        return Poco::DateTime();
    }
}


Poco::Net::SocketAddress
SsdpMessage::getSender() const
{
    return _sender;
}


//int
//DeviceGroupDelegate::getDeviceCount() const
//{
//    return _pDeviceGroup->getDeviceCount();
//}
//
//
//Device*
//DeviceGroupDelegate::getDevice(int index) const
//{
//    return _pDeviceGroup->getDevice(index);
//}
//
//
//void
//DeviceGroupDelegate::selectDevice(Device* pDevice)
//{
//    _pDeviceGroup->selectDevice(pDevice);
//}


//DeviceGroup::DeviceGroup(DeviceGroupDelegate* pDeviceGroupDelegate) :
//_pDeviceGroupDelegate(pDeviceGroupDelegate),
//_pSelectedDevice(0),
//_preferredDeviceUuid("")
//{
//    pDeviceGroupDelegate->_pDeviceGroup = this;
//}


DeviceGroup::DeviceGroup(const std::string& deviceType, const std::string& shortName) :
_pDeviceGroupDelegate(0),
_pController(0),
_deviceType(deviceType),
_shortName(shortName),
_pSelectedDevice(0),
_preferredDeviceUuid("")
{

}


DeviceGroup::DeviceGroup(DeviceGroupDelegate* pDeviceGroupDelegate) :
_pDeviceGroupDelegate(pDeviceGroupDelegate),
_pController(0),
_pSelectedDevice(0),
_preferredDeviceUuid("")
{
    _pDeviceGroupDelegate->setDeviceGroup(this);
}


int
DeviceGroup::getDeviceCount() const
{
    return _devices.size();
}


Device*
DeviceGroup::getDevice(int index) const
{

    return &_devices.get(index);
}


Device*
DeviceGroup::getDevice(const std::string& uuid)
{
    return &_devices.get(uuid);
}


Controller*
DeviceGroup::getController() const
{
    return _pController;
}


std::string
DeviceGroup::getDeviceType()
{
    if (_pDeviceGroupDelegate) {
        return _pDeviceGroupDelegate->getDeviceType();
    }
    else {
        return _deviceType;
    }
}


std::string
DeviceGroup::shortName()
{
    if (_pDeviceGroupDelegate) {
        return _pDeviceGroupDelegate->shortName();
    }
    else {
        return _shortName;
    }
}


Icon*
DeviceGroup::groupIcon()
{
    return 0;
}


Device*
DeviceGroup::createDevice()
{
    LOG(upnp, warning, "factory method virtual Device* DeviceGroup::createDevice() possibly not implemented in subclass");
    return new Device;
}


void
DeviceGroup::addDevice(Device* pDevice)
{
    addDevice(pDevice, _devices.size(), true);
    _devices.append(pDevice->getUuid(), pDevice);
    addDevice(pDevice, _devices.size() - 1, false);
}


void
DeviceGroup::removeDevice(Device* pDevice)
{
    std::string uuid = pDevice->getUuid();
    int position = _devices.position(uuid);
    LOG(upnp, debug, "device group remove device: " + pDevice->getFriendlyName() + ", uuid: " + uuid + ", position: " + Poco::NumberFormatter::format(position) + " ...");
    removeDevice(pDevice, position, true);
    _devices.remove(uuid);
    removeDevice(pDevice, position, false);
    LOG(upnp, debug, "device group removed device: " + pDevice->getFriendlyName());
//    delete pDevice;
}


void
DeviceGroup::initDelegate()
{
    if (_pDeviceGroupDelegate) {
        _pDeviceGroupDelegate->init();
    }
}


void
DeviceGroup::selectDevice(Device* pDevice)
{
    if (pDevice) {
        LOG(upnp, debug, "selected device: " + pDevice->getFriendlyName());
        _pSelectedDevice = pDevice;
        pDevice->selected();
        selectDevice(pDevice, _devices.position(pDevice->getUuid()));
    }
    else {
        LOG(upnp, warning, "can not select null device");
    }
}


Device*
DeviceGroup::getSelectedDevice() const
{
    return _pSelectedDevice;
}


void
DeviceGroup::clearDevices()
{
    _devices.clear();
    _pSelectedDevice = 0;
}


DeviceGroupDelegate::DeviceGroupDelegate() :
_pDeviceGroup(0)
{
}


void
DeviceGroupDelegate::setDeviceGroup(DeviceGroup* pDeviceGroup)
{
    _pDeviceGroup = pDeviceGroup;
}

} // namespace Omm
