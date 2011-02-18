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
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Exception.h>

#include "UpnpAvServer.h"
#include "UpnpAvServerPrivate.h"
#include "UpnpAvServerImpl.h"

namespace Omm {
namespace Av {


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


std::string
MediaItemServer::getProtocol()
{
    return "http-get:*";
}


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
_bufferSize(8192),
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
    Log::instance()->upnpav().debug("objectId: " + objectId + ", resourceId: " + uri[1]);
    
    AbstractMediaObject* pItem = _pItemServer->_pServerContainer->getObject(objectId);
    StreamingResource* pResource = static_cast<StreamingResource*>(pItem->getResource(resourceId));

    std::string protInfoString = pResource->getProtInfo();
    ProtocolInfo protInfo(protInfoString);
    std::string mime = protInfo.getMimeString();
    std::string dlna = protInfo.getDlnaString();
    std::streamsize resSize = pResource->getSize();
    
    response.setContentType(mime);
    response.set("Mime-Version", "1.0");
    response.set("transferMode.dlna.org", "Streaming");
    response.set("EXT", "");
    response.set("Server", Poco::Environment::osName() + "/"
                 + Poco::Environment::osVersion()
                 + " UPnP/" + UPNP_VERSION + " Omm/" + OMM_VERSION);
    response.setDate(Poco::Timestamp());
    
    if (pResource->isSeekable()) {
        response.set("Accept-Ranges", "bytes");
    }
    if (request.has("getcontentFeatures.dlna.org")) {
        response.set("contentFeatures.dlna.org", dlna);
    }
    
    if (request.getMethod() == "GET") {
        std::streamoff start = 0;
        std::streamoff end = -1;
        std::streamsize len = resSize;
        if (request.has("Range") && pResource->isSeekable()) {
            response.setStatusAndReason(Poco::Net::HTTPResponse::HTTP_PARTIAL_CONTENT, "Partial Content");
            std::string rangeValue = request.get("Range");
            parseRange(rangeValue, start, end);
            response.set("Content-Range", "bytes " 
                        + Poco::NumberFormatter::format((long)start) + "-"
                        + (end == -1 ? "" : Poco::NumberFormatter::format((long)end)) + "/"
                        + Poco::NumberFormatter::format(pResource->getSize()));
            if (end >= 0) {
                len = end - start + 1;
            }
            else {
                len = resSize - start;
            }
            response.setContentLength(len);
        }
        std::ostringstream responseHeader;
        response.write(responseHeader);
        Log::instance()->upnpav().debug("response header:\n" + responseHeader.str());
        std::ostream& ostr = response.send();
#ifdef __DARWIN__
        signal(SIGPIPE, SIG_IGN); // fixes abort with "Program received signal SIGPIPE, Broken pipe." on Mac OSX when renderer stops the stream.
#endif
        try {
            std::istream* pIstr = pResource->getStream();
            if (pIstr) {
                Log::instance()->upnpav().debug("sending stream ...");
                std::streamsize numBytes = copyStream(*pIstr, ostr, start, end);
                Log::instance()->upnpav().debug("stream sent (" + Poco::NumberFormatter::format(numBytes) + " bytes transfered).");
                delete pIstr;
            }
        }
        catch(Poco::Exception& e) {
            Log::instance()->upnpav().debug("streaming aborted: " + e.displayText());
        }
    }
    else if (request.getMethod() == "HEAD") {
        if (resSize > 0 ) {
            response.setContentLength(resSize);
        }
        std::ostringstream responseHeader;
        response.write(responseHeader);
        Log::instance()->upnpav().debug("response header:\n" + responseHeader.str());
        response.send();
    }
    
    if (response.sent()) {
        Log::instance()->upnpav().debug("media item request finished: " + request.getURI());
    }
}


std::streamsize
ItemRequestHandler::copyStream(std::istream& istr, std::ostream& ostr, std::streamoff start, std::streamoff end)
{
    if (start > 0) {
        istr.seekg(start);
    }

    char buffer[_bufferSize];
    std::streamsize len = 0;
    std::streamsize left = end;
    if (end >= 0) {
        left -= start;
        left++;
    }
    if (left >= 0 && left < _bufferSize) {
        istr.read(buffer, left);
    }
    else {
        istr.read(buffer, _bufferSize);
    }
    std::streamsize n = istr.gcount();
    while (n > 0)
    {
        len += n;
        if (end >= 0) {
            left -= n;
        }
        ostr.write(buffer, n);
        if (istr && ostr)
        {
            if (left >= 0 && left < _bufferSize) {
                istr.read(buffer, left);
            }
            else {
                istr.read(buffer, _bufferSize);
            }
            n = istr.gcount();
        }
        else n = 0;
    }
    return len;
}


void
ItemRequestHandler::parseRange(const std::string& rangeValue, std::streamoff& start, std::streamoff& end)
{
   std::string range = rangeValue.substr(rangeValue.find('=') + 1);

    std::string::size_type delim = range.find('-');
    start = Poco::NumberParser::parse(range.substr(0, delim));
    try {
        end = Poco::NumberParser::parse(range.substr(delim + 1));
    }
    catch (Poco::Exception& e) {
        Log::instance()->upnpav().warning("range end parsing: " + e.displayText());
    }
    Log::instance()->upnpav().debug("range: " + range + " (start: " + Poco::NumberFormatter::format((long)start)
                                                      + ", end: " + (end == -1 ? "not defined" : Poco::NumberFormatter::format((long)end)) + ")");
}


AvServer::AvServer() :
MediaServer(
new ContentDirectoryImplementation,
new ConnectionManagerImplementation,
new AVTransportImplementation
)
{
}

void
AvServer::setRoot(AbstractMediaObject* pRoot)
{
    _pRoot = pRoot;
    static_cast<ContentDirectoryImplementation*>(_pContentDirectoryImpl)->_pRoot = _pRoot;
//     _pRoot->setObjectId("0");
}


AbstractMediaObject*
AvServer::getRoot()
{
    return _pRoot;
}


StreamingResource::StreamingResource(PropertyImpl* pPropertyImpl, StreamingMediaObject* pServer, AbstractMediaObject* pItem) :
AbstractResource(pPropertyImpl),
_pServer(pServer),
_pItem(pItem),
_id(0)
{
}


std::string
StreamingResource::getValue()
{
    std::string serverAddress = _pServer->getServerAddress();
    std::string relativeObjectId = _pItem->getObjectId().substr(_pServer->getObjectId().length()+1);
    std::string resourceId = Poco::NumberFormatter::format(_id);
    return serverAddress + "/" + relativeObjectId + "$" + resourceId;
}


std::string
StreamingResource::getAttributeName(int index)
{
    if (index == 0) {
        return AvProperty::PROTOCOL_INFO;
    }
    else if (index == 1) {
        return AvProperty::SIZE;
    }
}


std::string
StreamingResource::getAttributeValue(int index)
{
    if (index == 0) {
        return _pServer->getServerProtocol() + ":" + getMime() + ":" + getDlna();
    }
    else if (index == 1) {
        return Poco::NumberFormatter::format(getSize());
    }
}


std::string
StreamingResource::getAttributeValue(const std::string& name)
{
    if (name == AvProperty::PROTOCOL_INFO) {
        return _pServer->getServerProtocol() + ":" + getMime() + ":" + getDlna();
    }
    else if (name == AvProperty::SIZE) {
        return Poco::NumberFormatter::format(getSize());
    }
}


int
StreamingResource::getAttributeCount()
{
    // protocolInfo and size
    return 2;
}


StreamingMediaItem::StreamingMediaItem(StreamingMediaObject* pServer)
{
    _pServer = pServer;
}


StreamingMediaObject::StreamingMediaObject(int port)
{
    _pItemServer = new MediaItemServer(port);
    _pItemServer->_pServerContainer = this;
    _pItemServer->start();
}


StreamingMediaObject::~StreamingMediaObject()
{
    _pItemServer->stop();
    delete _pItemServer;
}


AbstractMediaObject*
StreamingMediaObject::createChildObject()
{
    return new StreamingMediaItem(this);
}


std::string
StreamingMediaObject::getServerAddress()
{
    std::string address = Sys::NetworkInterfaceManager::instance()->getValidIpAddress().toString();
    int port = _pItemServer->_socket.address().port();
    return "http://" + address + ":" + Poco::NumberFormatter::format(port);
}


std::string
StreamingMediaObject::getServerProtocol()
{
    return _pItemServer->getProtocol();
}


TorchServer::TorchServer(int port) :
StreamingMediaObject(port),
_pChild(new TorchItem(this))
{
    _pChild->setParent(this);
}


TorchServer::~TorchServer()
{
    delete _pDataModel;
    delete _pTitleProp;
    delete _pChild;
}


void
TorchServer::setDataModel(AbstractDataModel* pDataModel)
{
    _pDataModel = pDataModel;
}


AbstractMediaObject*
TorchServer::getChild(ui4 numChild)
{
    _pChild->setObjectNumber(numChild);
    TorchItem*_pTorchChild = static_cast<TorchItem*>(_pChild);
    _pTorchChild->_optionalProps.clear();

    // FIXME: title property of child item should get it's title based on item's object number
    _pTorchChild->_pClassProp->setValue(_pDataModel->getClass(numChild));
    _pTorchChild->_pTitleProp->setValue(_pDataModel->getTitle(numChild));
    std::string artist = _pDataModel->getOptionalProperty(numChild, AvProperty::ARTIST);
    if (artist != "") {
        _pTorchChild->_pArtistProp->setValue(artist);
        _pTorchChild->_optionalProps.push_back(_pTorchChild->_pArtistProp);
    }
    std::string album = _pDataModel->getOptionalProperty(numChild, AvProperty::ALBUM);
    if (album != "") {
        _pTorchChild->_pAlbumProp->setValue(album);
        _pTorchChild->_optionalProps.push_back(_pTorchChild->_pAlbumProp);
    }
    std::string track = _pDataModel->getOptionalProperty(numChild, AvProperty::ORIGINAL_TRACK_NUMBER);
    if (track != "") {
        _pTorchChild->_pTrackProp->setValue(track);
        _pTorchChild->_optionalProps.push_back(_pTorchChild->_pTrackProp);
    }
    std::string genre = _pDataModel->getOptionalProperty(numChild, AvProperty::GENRE);
    if (genre != "") {
        _pTorchChild->_pGenreProp->setValue(genre);
        _pTorchChild->_optionalProps.push_back(_pTorchChild->_pGenreProp);
    }
    
    return _pChild;
}


ui4
TorchServer::getChildCount()
{
    if (_pDataModel) {
        return _pDataModel->getChildCount();
    }
    else {
        return 0;
    }
}


bool
TorchServer::isContainer()
{
    return true;
}


int
TorchServer::getPropertyCount(const std::string& name)
{
    // the server itself (not the media items) has only one property overall and one title property in particular
    // TODO: add class property, it's required
    if (name == "" || name == AvProperty::TITLE) {
        return 1;
    }
    else {
        return 0;
    }
}


AbstractProperty*
TorchServer::getProperty(int index)
{
    return _pTitleProp;
}


AbstractProperty*
TorchServer::getProperty(const std::string& name, int index)
{
    if (name == AvProperty::TITLE) {
        return _pTitleProp;
    }
    else {
        return 0;
    }
}


void
TorchServer::addProperty(AbstractProperty* pProperty)
{
    _pTitleProp = pProperty;
}


AbstractProperty*
TorchServer::createProperty()
{
    return new MemoryProperty;
}


TorchItemResource::TorchItemResource(TorchServer* pServer, AbstractMediaObject* pItem) :
StreamingResource(new MemoryPropertyImpl, pServer, pItem)
{
}


bool
TorchItemResource::isSeekable()
{
    AbstractDataModel* pDataModel = static_cast<TorchServer*>(_pServer)->_pDataModel;
    if (pDataModel) {
        return pDataModel->isSeekable(_pItem->getObjectNumber());
    }
    else {
        return false;
    }
}


std::istream*
TorchItemResource::getStream()
{
    AbstractDataModel* pDataModel = static_cast<TorchServer*>(_pServer)->_pDataModel;
    if (pDataModel) {
        return pDataModel->getStream(_pItem->getObjectNumber());
    }
    else {
        return 0;
    }
}


std::streamsize
TorchItemResource::getSize()
{
    AbstractDataModel* pDataModel = static_cast<TorchServer*>(_pServer)->_pDataModel;
    if (pDataModel) {
        return pDataModel->getSize(_pItem->getObjectNumber());
    }
    else {
        return 0;
    }
}


std::string
TorchItemResource::getMime()
{
    AbstractDataModel* pDataModel = static_cast<TorchServer*>(_pServer)->_pDataModel;
    if (pDataModel) {
        return pDataModel->getMime(_pItem->getObjectNumber());
    }
    else {
        return "*";
    }
}


std::string
TorchItemResource::getDlna()
{
    AbstractDataModel* pDataModel = static_cast<TorchServer*>(_pServer)->_pDataModel;
    if (pDataModel) {
        return pDataModel->getDlna(_pItem->getObjectNumber());
    }
    else {
        return "*";
    }
}


void
TorchItemPropertyImpl::setName(const std::string& name)
{
    _name = name;
}


void
TorchItemPropertyImpl::setValue(const std::string& value)
{
    _value = value;
}


std::string
TorchItemPropertyImpl::getName()
{
    Log::instance()->upnpav().debug("TorchItemPropertyImpl::getName() returns: " + _name);

    return _name;
}


std::string
TorchItemPropertyImpl::getValue()
{
    Log::instance()->upnpav().debug("TorchItemPropertyImpl::getValue() returns: " + _value);

    if (_name == AvProperty::CLASS || _name == AvProperty::TITLE
        || _name == AvProperty::ARTIST || _name == AvProperty::ALBUM || _name == AvProperty::ORIGINAL_TRACK_NUMBER
        || _name == AvProperty::GENRE) {
        return _value;
    }
}


TorchItemProperty::TorchItemProperty() :
AbstractProperty(new TorchItemPropertyImpl)
{
}


TorchItem::TorchItem(TorchServer* pServer) :
StreamingMediaItem(pServer),
_pClassProp(new TorchItemProperty),
_pTitleProp(new TorchItemProperty),
_pResource(new TorchItemResource(pServer, this)),
_pArtistProp(new TorchItemProperty),
_pAlbumProp(new TorchItemProperty),
_pTrackProp(new TorchItemProperty),
_pGenreProp(new TorchItemProperty)
{
//     std::clog << "TorchItem::TorchItem(pServer), pServer: " << pServer << std::endl;
    _pClassProp->setName(AvProperty::CLASS);
    _pTitleProp->setName(AvProperty::TITLE);
    _pArtistProp->setName(AvProperty::ARTIST);
    _pAlbumProp->setName(AvProperty::ALBUM);
    _pTrackProp->setName(AvProperty::ORIGINAL_TRACK_NUMBER);
    _pGenreProp->setName(AvProperty::GENRE);
}


TorchItem::~TorchItem()
{
    delete _pClassProp;
    delete _pTitleProp;
}


int
TorchItem::getPropertyCount(const std::string& name)
{
    Log::instance()->upnpav().debug("TorchItem::getPropertyCount(), name: " + name);
    
    if (name == "") {
        return 3 + _optionalProps.size();
    }
    else if (name == AvProperty::CLASS || name == AvProperty::TITLE || name == AvProperty::RES
        || name == AvProperty::ARTIST || name == AvProperty::ALBUM || name == AvProperty::ORIGINAL_TRACK_NUMBER
        || name == AvProperty::GENRE) {
        return 1;
    }
    else {
        return 0;
    }
}


AbstractProperty*
TorchItem::getProperty(int index)
{
    Log::instance()->upnpav().debug("TorchItem::getProperty(index), index: " + Poco::NumberFormatter::format(index));
    
    if (index == 0) {
        return _pClassProp;
    }
    else if (index == 1) {
        return _pTitleProp;
    }
    else if (index == 2) {
        return _pResource;
    }
    else {
        return _optionalProps[index - 3];
    }
}


AbstractProperty*
TorchItem::getProperty(const std::string& name, int index)
{
    Log::instance()->upnpav().debug("TorchItem::getProperty(name, index), name: " + name + ", index: " + Poco::NumberFormatter::format(index));

    if (name == AvProperty::CLASS) {
        return _pClassProp;
    }
    else if (name == AvProperty::TITLE) {
        return _pTitleProp;
    }
    else if (name == AvProperty::RES) {
        return _pResource;
    }
    else {
        for (int i = 0; i < _optionalProps.size(); i++) {
            if (_optionalProps[i]->getName() == name) {
                return _optionalProps[i];
            }
        }
        return 0;
    }
}


} // namespace Av
} // namespace Omm
