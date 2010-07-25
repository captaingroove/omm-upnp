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

#include <Poco/File.h>

#include "UpnpAvServer.h"
#include "UpnpAvServerImpl.h"

namespace Omm {
namespace Av {


ServerResource::ServerResource(const std::string& resourceId, const std::string& protInfo, ui4 size) :
Resource("", protInfo, size),
_resourceId(resourceId)
{
}


const std::string&
ServerResource::getResourceId()
{
    return _resourceId;
}


// FileResource::FileResource(const std::string& resourceId, const std::string& protInfo, ui4 size, const std::string& privateUri) :
// ServerResource(resourceId, protInfo, size),
// _privateUri(privateUri)
// {
// }
// 
// 
// std::streamsize
// FileResource::stream(std::ostream& ostr, std::iostream::pos_type seek)
// {
// //     std::string path = _pFileObjectSource->_basePath + "/" + _privateUri;
//     std::string path = _privateUri;
//     
//     std::ifstream istr(path.c_str());
//     if (seek > 0) {
//         istr.seekg(seek);
//     }
//     return Poco::StreamCopier::copyStream(istr, ostr);
// }


WebResource::WebResource(const std::string& resourceId, const std::string& protInfo, const std::string& privateUri) :
ServerResource(resourceId, protInfo, 0),
_privateUri(privateUri)
{
}


// FIXME: vdr streamdev-server does this frequently:
// 21:19:16.627 arthur[3581,12] D UPNP.AV sending stream: http://192.168.178.23:3000/TS/S19.2E-1-1107-17500 ...
// 21:19:16.628 arthur[3581,12] I UPNP.AV HTTP 409 Channel not available
// 21:19:16.628 arthur[3581,12] D UPNP.AV proxy response header:
// HTTP/1.0 409 Channel not available
// (409 = HTTP_CONFLICT)


std::streamsize
WebResource::stream(std::ostream& ostr, std::iostream::pos_type seek)
{
    Poco::URI uri(_privateUri);
    Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
    session.setKeepAlive(true);
    session.setKeepAliveTimeout(Poco::Timespan(3, 0));
    Poco::Timespan timeout = session.getKeepAliveTimeout();
    Log::instance()->upnpav().debug("web resource server proxy timeout is: " + Poco::NumberFormatter::format(timeout.seconds() + "sec"));
    Poco::Net::HTTPRequest proxyRequest("GET", uri.getPath());
    proxyRequest.setKeepAlive(true);
    session.sendRequest(proxyRequest);
    std::stringstream requestHeader;
    proxyRequest.write(requestHeader);
    Log::instance()->upnpav().debug("proxy request header:\n" + requestHeader.str());
    
    Poco::Net::HTTPResponse proxyResponse;
    std::istream& istr = session.receiveResponse(proxyResponse);
    
    if (istr.peek() == EOF) {
        Log::instance()->upnpav().error("error reading data from web resource");
    }
    else {
        Log::instance()->upnpav().debug("success reading data from web resource");
    }
    
    Log::instance()->upnpav().information("HTTP " + Poco::NumberFormatter::format(proxyResponse.getStatus()) + " " + proxyResponse.getReason());
    std::stringstream responseHeader;
    proxyResponse.write(responseHeader);
    Log::instance()->upnpav().debug("proxy response header:\n" + responseHeader.str());
    
    return Poco::StreamCopier::copyStream(istr, ostr);
}


MediaItemServer::MediaItemServer(int port) :
_socket(Poco::Net::ServerSocket(port))
{
}


MediaItemServer::~MediaItemServer()
{
    Log::instance()->upnpav().information("stopping media item server ...");
    _pHttpServer->stop();
    delete _pHttpServer;
    Log::instance()->upnpav().information("done");
}


void
MediaItemServer::start()
{
    Poco::Net::HTTPServerParams* pParams = new Poco::Net::HTTPServerParams;
    _pHttpServer = new Poco::Net::HTTPServer(new ItemRequestHandlerFactory(this), _socket, pParams);
    _pHttpServer->start();
    Log::instance()->upnpav().information("media item server listening on: " + _socket.address().toString());
}


void
MediaItemServer::stop()
{
    _pHttpServer->stop();
}


Poco::UInt16
MediaItemServer::getPort() const
{
    return _socket.address().port();
}


// MediaServerContainer::MediaServerContainer(const std::string& title, const std::string& subClass, int port) :
// MediaContainer(title, subClass)
// {
//     _pItemServer = new MediaItemServer(port);
//     _pItemServer->_pServerContainer = this;
//     _pItemServer->start();
//     _port = _pItemServer->_socket.address().port();
//     _address =  Omm::NetworkInterfaceManager::instance()->getValidInterfaceAddress().toString();
// }
// 
// 
// MediaServerContainer::~MediaServerContainer()
// {
//     _pItemServer->stop();
//     delete _pItemServer;
// }
// 
// 
// // void
// // MediaServerContainer::appendChild(ServerObject* pChild)
// // {
// //     ServerObject::appendChild(pChild);
// //     
// //     // FIXME: writing the public uri should be moved somewhere else, probably into the XML writer of ServerObject
// //     // then appendChild() can be removed from MediaServerContainer
// //     for (MediaObject::ResourceIterator it = pChild->beginResource(); it != pChild->endResource(); ++it) {
// //         (*it)->setUri(getServerAddress() + "/" + (*it)->getUri());
// //     }
// // }
// 
// 
// std::string
// MediaServerContainer::getServerAddress()
// {
//     return "http://" + _address + ":" + Poco::NumberFormatter::format(_port);
// }


// ServerObject::ServerObject() //:
// // MediaObject()
// {
// }


// void
// ServerObject::appendChild(ServerObject* pChild)
// {
//     MediaObject::appendChild(pChild);
//     _childrenMap[pChild->_objectId] = pChild;
// }


// ServerObject*
// ServerObject::getObject(const std::string& objectId)
// {
// //     std::clog << "ServerObject::getObject() objectId: " << objectId << std::endl;
//     std::string::size_type slashPos = objectId.find('/');
//     ServerObject* pChild;
//     if (slashPos != std::string::npos) {
// //         std::clog << "container id: " << objectId.substr(0, slashPos - 1) << std::endl;
//         pChild = _childrenMap[objectId.substr(0, slashPos)];
//         if (pChild == 0) {
//             Log::instance()->upnpav().error("child objectId of container, but no child container found");
//             return 0;
//         }
//         else {
//             return pChild->getObject(objectId.substr(slashPos + 1));
//         }
//     }
//     else {
// //         std::clog << "item id: " << objectId << std::endl;
//         pChild = _childrenMap[objectId];
//         if (pChild == 0) {
//             Log::instance()->upnpav().error("no child item found");
//             return 0;
//         }
//         else {
//             return pChild;
//         }
//     }
// }


// void
// ServerObject::addResource(ServerResource* pResource)
// {
//     MediaObject::addResource(pResource);
//     _resourceMap[pResource->getResourceId()] = pResource;
//     pResource->setUri(_objectId + "$" + pResource->getResourceId());
//     pResource->setProtInfo("http-get:*:" + pResource->getProtInfo());
// }


// ServerResource*
// AbstractResource*
// ServerObject::getResource(const std::string& resourceId)
// {
//     return _resourceMap[resourceId];
// }


// ui4
// ServerObject::getChildCount()
// {
//     return _children.size();
// }


// MediaContainer::MediaContainer() :
// ServerObject()
// {
//     _isContainer = true;
// }


// MediaContainer::MediaContainer(const std::string& title, const std::string& subClass) :
// ServerObject()
// {
//     setTitle(title);
//     setClass(std::string("object.container" + (subClass == "" ? "" : "." + subClass)));
// }
// 
// 
// MediaItem::MediaItem() :
// ServerObject()
// {
// }
// 
// 
// MediaItem::MediaItem(const std::string& objectId, const std::string& title, const std::string& subClass)
// {
//     setObjectNumber(objectId);
//     setTitle(title);
//     setClass(std::string("object.item" + (subClass == "" ? "" : "." + subClass)));
// }


ItemRequestHandlerFactory::ItemRequestHandlerFactory(MediaItemServer* pItemServer) :
_pItemServer(pItemServer)
{
}


Poco::Net::HTTPRequestHandler*
ItemRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
    return new ItemRequestHandler(_pItemServer);
}


ItemRequestHandler::ItemRequestHandler(MediaItemServer* pItemServer) :
_pItemServer(pItemServer)
{
}


void
ItemRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    Log::instance()->upnpav().debug("handle media item request: " + request.getURI());
    
    std::ostringstream requestHeader;
    request.write(requestHeader);
    Log::instance()->upnpav().debug("request method: " + request.getMethod());
    Log::instance()->upnpav().debug("request header:\n" + requestHeader.str());
    
    Poco::StringTokenizer uri(request.getURI(), "$");
    std::string objectId = uri[0].substr(1);
    int resourceId = Poco::NumberParser::parse(uri[1]);
//     std::string resourceId = uri[1];
    Log::instance()->upnpav().debug("objectId: " + objectId + ", resourceId: " + uri[1]);
    
    // TODO: check if pItem really is a MediaItem and not a MediaContainer?
//     ServerObject* pItem = _pItemServer->_pServerContainer->getObject(objectId);
    AbstractMediaObject* pItem = _pItemServer->_pServerContainer->getObject(objectId);
//     ServerResource* pResource = pItem->getResource(resourceId);
//     StreamingResource* pResource = static_cast<StreamingResource*>(pItem->getResource(resourceId));
    StreamingResource* pResource = static_cast<StreamingResource*>(pItem->getResource(resourceId));

    std::string resProtInfo = pResource->getProtInfo();
    Log::instance()->upnpav().debug("protInfo: " + resProtInfo);
    Poco::StringTokenizer prot(resProtInfo, ":");
    std::string mime = prot[2];
    std::string dlna = prot[3];
    Log::instance()->upnpav().debug("protInfo mime: " + mime + ", dlna: " + dlna);
    ui4 resSize = pResource->getSize();
    
    response.setContentType(mime);
    response.set("transferMode.dlna.org", "Streaming");
    response.set("EXT", "");
    response.set("Server", Poco::Environment::osName() + "/"
                 + Poco::Environment::osVersion()
                 + " UPnP/" + UPNP_VERSION + " Omm/" + OMM_VERSION);
    response.setDate(Poco::Timestamp());
    
    if (pResource->isSeekable()) {
        response.set("Accept-Ranges", "bytes");
    }
    if (resSize > 0 ) {
        response.setContentLength(resSize);
    }
    if (request.has("getMediaInfo.sec")) {
        response.setContentType(mime);
    }
    if (request.has("getcontentFeatures.dlna.org")) {
        response.set("contentFeatures.dlna.org", dlna);
    }
    
    std::ostringstream responseHeader;
    response.write(responseHeader);
    Log::instance()->upnpav().debug("response header:\n" + responseHeader.str());

    if (request.getMethod() == "GET") {
        Log::instance()->upnpav().debug("sending stream ...");
        std::ostream& ostr = response.send();
        std::iostream::pos_type start = 0;
        if (request.has("Range")) {
            std::string rangeVal = request.get("Range");
            std::string range = rangeVal.substr(rangeVal.find('=') + 1);
            
            std::string::size_type delim = range.find('-');
            start = Poco::NumberParser::parse(range.substr(0, delim));
            Log::instance()->upnpav().debug("range: " + range + " (start: " + Poco::NumberFormatter::format(start) + ")");
        }
        std::streamsize numBytes = pResource->stream(ostr, start);
        Log::instance()->upnpav().debug("stream sent (" + Poco::NumberFormatter::format(numBytes) + " bytes transfered).");
    }
    else if (request.getMethod() == "HEAD") {
        response.send();
    }
    
    if (response.sent()) {
        Log::instance()->upnpav().debug("media item request finished: " + request.getURI());
    }
}


UpnpAvServer::UpnpAvServer() :
MediaServer(
new ContentDirectoryImplementation,
new ConnectionManagerImplementation,
new AVTransportImplementation
)
{
}

void
// UpnpAvServer::setRoot(ServerObject* pRoot)
UpnpAvServer::setRoot(AbstractMediaObject* pRoot)
{
    _pRoot = pRoot;
    static_cast<ContentDirectoryImplementation*>(_pContentDirectoryImpl)->_pRoot = _pRoot;
// //     _pRoot->setObjectId("0");
}


// ServerObject*
AbstractMediaObject*
UpnpAvServer::getRoot()
{
    return _pRoot;
}



// StreamingResourceImpl::StreamingResourceImpl(StreamingMediaObject* pServer, AbstractMediaObject* pItem) :
// _pServer(pServer),
// _pItem(pItem)
// {
//     std::clog << "StreamingResourceImpl::StreamingResourceImpl(pServer, pItem), pServer: " << pServer << ", pItem: " << pItem << std::endl;
// }
// 
// 
// std::string
// StreamingResourceImpl::getValue()
// {
//     std::clog << "StreamingResourceImpl::getValue()" << std::endl;
//     
//     std::string serverAddress = _pServer->getServerAddress();
//     std::string relativeObjectId = _pItem->getObjectId().substr(_pServer->getObjectId().length()+1);
// //     std::string resourceId = Poco::NumberFormatter::format(getResourceNumber());
//     return serverAddress + "/" + relativeObjectId + "$" /*+ resourceId*/;
//     
// //     return _pServer->getServerAddress() + "/" + _pItem->getObjectId();
// }


StreamingResource::StreamingResource(PropertyImpl* pPropertyImpl, StreamingMediaObject* pServer, AbstractMediaObject* pItem) :
// AbstractResource(new StreamingResourceImpl(pServer, pItem)),
AbstractResource(pPropertyImpl),
_pServer(pServer),
_pItem(pItem),
_id(0)
{
    std::clog << "StreamingResource::StreamingResource(pServer, pItem), pServer: " << pServer << ", pItem: " << pItem << std::endl;
}


std::string
StreamingResource::getValue()
{
    std::clog << "StreamingResourceImpl::getValue()" << std::endl;
    
    std::string serverAddress = _pServer->getServerAddress();
    std::string relativeObjectId = _pItem->getObjectId().substr(_pServer->getObjectId().length()+1);
    std::string resourceId = Poco::NumberFormatter::format(_id);
    return serverAddress + "/" + relativeObjectId + "$" + resourceId;
}


StreamingMediaObject::StreamingMediaObject(int port)
{
    std::clog << "StreamingMediaObject::StreamingMediaObject(port), port: " << port << std::endl;
    
    _pItemServer = new MediaItemServer(port);
    _pItemServer->_pServerContainer = this;
    _pItemServer->start();
}


StreamingMediaObject::~StreamingMediaObject()
{
    _pItemServer->stop();
    delete _pItemServer;
}


std::string
StreamingMediaObject::getServerAddress()
{
    std::clog << "StreamingMediaObject::getServerAddress()" << std::endl;
    
    std::string address = Omm::NetworkInterfaceManager::instance()->getValidInterfaceAddress().toString();
    int port = _pItemServer->_socket.address().port();
    return "http://" + address + ":" + Poco::NumberFormatter::format(port);
}



} // namespace Av
} // namespace Omm
