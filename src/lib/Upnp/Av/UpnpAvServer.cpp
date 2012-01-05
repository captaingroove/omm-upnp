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

#include <sstream>
#include <fstream>

#include <Poco/File.h>
#include <Poco/StreamCopier.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Exception.h>
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/RecordSet.h"

#include <map>

#include "UpnpAvServer.h"
#include "UpnpInternal.h"
#include "UpnpAvDevices.h"
#include "UpnpAvDescriptions.h"
//#include "UpnpAvServerPrivate.h"
#include "UpnpAvServerImpl.h"

namespace Omm {
namespace Av {


MediaServer::MediaServer(int port) :
_socket(Poco::Net::ServerSocket(port))
{
    MediaServerDescriptions mediaServerDescriptions;
    MemoryDescriptionReader descriptionReader(mediaServerDescriptions);
    descriptionReader.getDeviceDescription();
    setDeviceData(descriptionReader.rootDeviceData());

    // service implementations are owned by DevMediaServer (DevDeviceCode)
    // DevDeviceCode should be owned by super class Device

    // store a pointer to content directory implementation, so that the
    // root object can be set later.
    _pDevContentDirectoryServerImpl = new DevContentDirectoryServerImpl;

    setDevDeviceCode(new DevMediaServer(
        _pDevContentDirectoryServerImpl,
        new DevConnectionManagerServerImpl,
        new DevAVTransportServerImpl)
    );
}


MediaServer::~MediaServer()
{
    Log::instance()->upnpav().information("stopping media server ...");
    _pHttpServer->stop();
    delete _pHttpServer;
    Log::instance()->upnpav().information("done");
}


void
MediaServer::setRoot(ServerContainer* pRoot)
{
    _pDevContentDirectoryServerImpl->_pRoot = pRoot;
    start();
}


void
MediaServer::start()
{
    Poco::Net::HTTPServerParams* pParams = new Poco::Net::HTTPServerParams;
    _pHttpServer = new Poco::Net::HTTPServer(new ItemRequestHandlerFactory(this), _socket, pParams);
    _pHttpServer->start();
    Log::instance()->upnpav().information("media server listening on: " + _socket.address().toString());
}


void
MediaServer::stop()
{
    _pHttpServer->stop();
}


Poco::UInt16
MediaServer::getServerPort() const
{
    return _socket.address().port();
}


std::string
MediaServer::getServerAddress()
{
    std::string address = Net::NetworkInterfaceManager::instance()->getValidIpAddress().toString();
    return "http://" + address + ":" + Poco::NumberFormatter::format(getServerPort());
}


std::string
MediaServer::getServerProtocol()
{
    return "http-get:*";
}


ItemRequestHandlerFactory::ItemRequestHandlerFactory(MediaServer* pItemServer) :
_pServer(pItemServer)
{
}


Poco::Net::HTTPRequestHandler*
ItemRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
    return new ItemRequestHandler(_pServer);
}


ItemRequestHandler::ItemRequestHandler(MediaServer* pItemServer) :
_bufferSize(8192),
_pServer(pItemServer)
{
}


void
ItemRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    Log::instance()->upnpav().debug("handle media item request: " + request.getURI());

    response.setDate(Poco::Timestamp());

    std::ostringstream requestHeader;
    request.write(requestHeader);
    Log::instance()->upnpav().debug("request method: " + request.getMethod());
    Log::instance()->upnpav().debug("request header:\n" + requestHeader.str());

    Poco::StringTokenizer uri(request.getURI(), "$");
    std::string objectId = uri[0].substr(1);
    Log::instance()->upnpav().debug("objectId: " + objectId + ", resourceId: " + uri[1]);
    ServerObject* pItem = _pServer->_pServerContainer->getDescendant(objectId);
    if (!pItem) {
        Log::instance()->upnpav().error("item request handler could not find object with id: " + objectId);
        return;
    }

    ServerObjectResource* pResource;
    ServerObjectProperty* pProperty;
    std::streamsize resSize = 0;
    if (uri[1] == "i") {
        // object icon requested by controller
        Log::instance()->upnpav().debug("icon request: server creates icon property");
        pProperty = static_cast<ServerObjectProperty*>(pItem->getProperty(AvProperty::ICON));
        Log::instance()->upnpav().debug("icon request: server icon property created");
        // deliver icon
        if (pProperty) {
            // icon requested (handle other streamable properties as well, such as album art ...)
            try {
                std::istream* pIstr = pProperty->getStream();
                if (pIstr) {
                    Log::instance()->upnpav().debug("sending icon ...");
                    std::ostream& ostr = response.send();
                    std::streamsize numBytes = Poco::StreamCopier::copyStream(*pIstr, ostr);
                    Log::instance()->upnpav().debug("icon sent (" + Poco::NumberFormatter::format(numBytes) + " bytes transfered).");
                    delete pIstr;
                }
            }
            catch(Poco::Exception& e) {
                Log::instance()->upnpav().debug("delivering icon failed: " + e.displayText());
            }
        }
        return;
    }
    else {
        // resource requested by controller
        int resourceId = Poco::NumberParser::parse(uri[1]);
        pResource = static_cast<ServerObjectResource*>(pItem->getResource(resourceId));
        resSize = pResource->getSize();

        std::string protInfoString = pResource->getProtInfo();
        ProtocolInfo protInfo(protInfoString);
        std::string mime = protInfo.getMimeString();
        std::string dlna = protInfo.getDlnaString();

        response.setContentType(mime);
        response.set("Mime-Version", "1.0");
        response.set("transferMode.dlna.org", "Streaming");
        response.set("EXT", "");
        response.set("Server", Poco::Environment::osName() + "/"
                     + Poco::Environment::osVersion()
                     + " UPnP/" + UPNP_VERSION + " Omm/" + OMM_VERSION);

        if (pResource->isSeekable()) {
            response.set("Accept-Ranges", "bytes");
        }
        if (request.has("getcontentFeatures.dlna.org")) {
            response.set("contentFeatures.dlna.org", dlna);
        }
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
            std::istream* pIstr;
            // deliver resource
            pIstr = pResource->getStream();
            if (pIstr) {
                Log::instance()->upnpav().debug("sending stream ...");
                std::streamsize numBytes = copyStream(*pIstr, ostr, start, end);
                Log::instance()->upnpav().debug("stream sent (" + Poco::NumberFormatter::format(numBytes) + " bytes transfered).");
                delete pIstr;
            }
            else {
                throw Poco::Exception("no stream available");
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


ServerObjectProperty::ServerObjectProperty(ServerItem* pItem, AbstractDataModel* pDataModel) :
_pItem(pItem),
_pDataModel(pDataModel)
{
}


std::istream*
ServerObjectProperty::getStream()
{
    std::string path = _pDataModel->getPath(_pItem->getIndex());
//    if (getName() == AvProperty::RES) {
//        return _pDataModel->getStream(path);
//    }
    if (getName() == AvProperty::ICON) {
        return _pDataModel->getIconStream(path);
    }
    else {
        return 0;
    }
}


ServerObjectResource::ServerObjectResource(ServerItem* pItem, AbstractDataModel* pDataModel) :
_pItem(pItem),
_pDataModel(pDataModel),
_id(0)
{
}


bool
ServerObjectResource::isSeekable()
{
    return _pDataModel->isSeekable(_pDataModel->getPath(_pItem->getIndex()));
}


std::streamsize
ServerObjectResource::getSize()
{
    return _pDataModel->getSize(_pDataModel->getPath(_pItem->getIndex()));
}


std::istream*
ServerObjectResource::getStream()
{
    return _pDataModel->getStream(_pDataModel->getPath(_pItem->getIndex()));
}


ServerObject::ServerObject(MediaServer* pServer) :
_index(0),
_pParent(0),
_pServer(pServer)
{
}


ServerObject::~ServerObject()
{
//    if (_pItemServer) {
//        _pItemServer->stop();
//        delete _pItemServer;
//        _pItemServer = 0;
//    }
}


std::string
ServerObject::getId()
{
//    Log::instance()->upnpav().debug("ServerObject::getObjectId()");

    ServerObject* pParent = getParent();
    if (pParent == 0) {
        return "0";
    }
    else {
        return pParent->getId() + "/" + Poco::NumberFormatter::format(getIndex());
    }
}


std::string
ServerObject::getParentId()
{
//    Log::instance()->upnpav().debug("ServerObject::getParentObjectId()");

    AbstractMediaObject* pParent = getParent();
    if (pParent) {
        return pParent->getId();
    }
    else {
        return "";
    }
}


ui4
ServerObject::getIndex()
{
//    Log::instance()->upnpav().debug("ServerObject::getIndex index: " + Poco::NumberFormatter::format(_index));
    return _index;
}


void
ServerObject::setIndex(ui4 index)
{
//    Log::instance()->upnpav().debug("ServerObject::setObjectNumber() objectId: " + Poco::NumberFormatter::format(id));
    _index = index;
}


void
ServerObject::setIndex(const std::string& index)
{
//    Log::instance()->upnpav().debug("ServerObject::setObjectId() from string: " + id);
    _index = Poco::NumberParser::parseUnsigned(index);
}


ui4
ServerObject::getParentIndex()
{
//    Log::instance()->upnpav().debug("ServerObject::getIndex index: " + Poco::NumberFormatter::format(_index));
    return _parentIndex;
}


ServerObject*
ServerObject::getParent()
{
//    Log::instance()->upnpav().debug("ServerObject::getParent()");
    return _pParent;
}


void
ServerObject::setParentIndex(ui4 index)
{
//    Log::instance()->upnpav().debug("ServerObject::getIndex index: " + Poco::NumberFormatter::format(_index));
    _parentIndex = index;
}


void
ServerObject::setParent(ServerObject* pParent)
{
//    Log::instance()->upnpav().debug("ServerObject::setParent()");
    _pParent = pParent;
}


AbstractMediaObject*
ServerObject::createChildObject()
{
    ServerObject* pObject = new ServerObject(_pServer);
    pObject->setParent(this);
    return pObject;
}


ServerItem::ServerItem(MediaServer* pServer, ServerContainer* pContainer) :
ServerObject(pServer),
_pContainer(pContainer)
{
}


ServerItem::~ServerItem()
{
}


ServerObjectResource*
ServerItem::createResource()
{
    Log::instance()->upnpav().debug("server item create resource");

    return new ServerObjectResource(this, _pContainer->getDataModel());
}


ServerContainer::ServerContainer(MediaServer* pServer) :
ServerObject(pServer),
_pDataModel(0),
_pObjectCache(0),
_layout(Flat),
_updateCacheThreadRunnable(*this, &ServerContainer::updateCacheThread),
_updateCacheThreadRunning(false)
{
    setIsContainer(true);
    _pServer->_pServerContainer = this;

    _searchCaps.append(AvProperty::CLASS);
    _searchCaps.append(AvProperty::TITLE);
    _searchCaps.append(AvProperty::ARTIST);
    _searchCaps.append(AvProperty::ALBUM);
    _searchCaps.append(AvProperty::ORIGINAL_TRACK_NUMBER);

    _sortCaps.append(AvProperty::CLASS);
    _sortCaps.append(AvProperty::TITLE);
    _sortCaps.append(AvProperty::ARTIST);
    _sortCaps.append(AvProperty::ALBUM);
    _sortCaps.append(AvProperty::ORIGINAL_TRACK_NUMBER);
}


AbstractDataModel*
ServerContainer::getDataModel()
{
    return _pDataModel;
}


void
ServerContainer::setDataModel(AbstractDataModel* pDataModel)
{
    _pDataModel = pDataModel;
    _pDataModel->setServerContainer(this);
    if (_pDataModel->useObjectCache()) {
        _pObjectCache = new DatabaseCache;
        _pObjectCache->_pServerContainer = this;
    }
}


ServerObjectCache*
ServerContainer::getObjectCache()
{
    return _pObjectCache;
}


void
ServerContainer::setObjectCache(ServerObjectCache* pObjectCache)
{
    _pObjectCache = pObjectCache;
}


ServerContainer*
ServerContainer::createMediaContainer()
{
    Log::instance()->upnpav().debug("server container create media container");

    ServerContainer* pContainer = new ServerContainer(_pServer);
    pContainer->_pServer = _pServer;

    pContainer->setIsContainer(true);
    pContainer->setClass(AvClass::className(AvClass::CONTAINER));

    return pContainer;
}


ServerItem*
ServerContainer::createMediaItem()
{
    Log::instance()->upnpav().debug("server container create media item");

    ServerItem* pItem = new ServerItem(_pServer, this);
    pItem->setParent(this);
    return pItem;
}


ServerObject*
ServerContainer::createChildObject()
{
    Log::instance()->upnpav().debug("server container create child object");

    return createMediaItem();
}


CsvList*
ServerContainer::getSearchCaps()
{
    if (_searchCaps.getSize()) {
        return &_searchCaps;
    }
    else {
        return 0;
    }
}


CsvList*
ServerContainer::getSortCaps()
{
    if (_sortCaps.getSize()) {
        return &_sortCaps;
    }
    else {
        return 0;
    }
}


void
ServerContainer::setBasePath(const std::string& basePath)
{
    Log::instance()->upnpav().debug("server container, set base path to: " + basePath);
    _pDataModel->setBasePath(basePath);
    if (_pObjectCache) {
        _pObjectCache->setCacheFilePath(Util::Home::instance()->getCacheDirPath(basePath) + _pDataModel->getServerClass() + "/objects");
        updateCache();
    }
}


void
ServerContainer::updateCache(bool on)
{
    _updateCacheThreadLock.lock();
    _updateCacheThreadRunning = on;
    _updateCacheThreadLock.unlock();
    if (on) {
        _updateCacheThread.start(_updateCacheThreadRunnable);
    }
    else {
        _updateCacheThread.join();
    }
}


void
ServerContainer::updateCacheThread()
{
    Log::instance()->upnpav().debug("server container, update cache thread started ...");
    if (!cacheNeedsUpdate()) {
        Log::instance()->upnpav().debug("database cache is current, nothing to do");
    }
    else {
        for (AbstractDataModel::IndexIterator it = _pDataModel->beginIndex(); it != _pDataModel->endIndex(); ++it) {
            if (!updateCacheThreadIsRunning()) {
                Log::instance()->upnpav().debug("stopping scan thread");
                break;
            }
            _pObjectCache->insertMediaObject(ServerContainer::getChildForIndex((*it).first, false));
        }
    }
    _updateCacheThreadLock.lock();
    _updateCacheThreadRunning = false;
    _updateCacheThreadLock.unlock();
    Log::instance()->upnpav().debug("server container, update cache thread finished.");
}


bool
ServerContainer::updateCacheThreadIsRunning()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_updateCacheThreadLock);
    Log::instance()->upnpav().debug("server container, update cache thread is running: " + std::string(_updateCacheThreadRunning ? "yes" : "no"));
    return _updateCacheThreadRunning;
}


bool
ServerContainer::cacheNeedsUpdate()
{
    if (!_pObjectCache) {
        return false;
    }
    ui4 rows = _pObjectCache->rowCount();
    if (rows > _pDataModel->getIndexCount()) {
        Log::instance()->upnpav().error("server container, database cache not coherent.");
        return true;
    }
    else if (rows == _pDataModel->getIndexCount()) {
        Log::instance()->upnpav().debug("server container, database cache is current.");
        return false;
    }
    else {
        Log::instance()->upnpav().debug("server container, database cache needs update.");
        return true;
    }
}


void
ServerContainer::appendChild(AbstractMediaObject* pChild)
{
// TODO: server implementation of appendChild()
////    pChild->setIndex(getChildCount());
////    pChild->setIndex(index);
//    pChild->setParent(this);
//    appendChildImpl(pChild);
}


void
ServerContainer::appendChildWithAutoIndex(AbstractMediaObject* pChild)
{
// TODO: server implementation of appendChild(), replace appendChildWithAutoIndex()
//    pChild->setIndex(getChildCount());
//    appendChild(pChild);
}


ui4
ServerContainer::getChildCount()
{
    if (!_pDataModel) {
        return 0;
    }
    return _pDataModel->getIndexCount();
}


ServerObject*
ServerContainer::getDescendant(const std::string& objectId)
{
    Log::instance()->upnpav().debug("server container get descendant with object id relative to this container's id (in most cases \"0\"): " + objectId);

    std::string::size_type slashPos = objectId.find('/');
    if (slashPos != std::string::npos) {
        // child is a container
        ServerContainer* pChild = static_cast<ServerContainer*>(getChildForIndex(objectId.substr(0, slashPos)));
        if (pChild == 0) {
            // child container is not a child of this container, we try the full path
            pChild = static_cast<ServerContainer*>(getChildForIndex(objectId));
            if (pChild == 0) {
                // child container is not a child of this container
                Log::instance()->upnpav().error("retrieving child objectId of container, but no child container found");
                return 0;
            }
            else {
                // object id of child contains slashes (subtree's implementation is a faked tree with only item objects).
                return pChild;
            }
        }
        else {
            // recurse into child container
            return pChild->getDescendant(objectId.substr(slashPos + 1));
        }
    }
    else {
        // child is an item
        Log::instance()->upnpav().debug("server container get descendant is a child with index: " + objectId);
        ServerItem* pChild = static_cast<ServerItem*>(getChildForIndex(objectId));
        if (pChild == 0) {
            // child item is not a child of this container
            Log::instance()->upnpav().error("no child item found");
            return 0;
        }
        else {
            // return child item and stop recursion
            return pChild;
        }
    }
}


ServerObject*
ServerContainer::getChildForIndex(const std::string& index)
{
    return getChildForIndex(Poco::NumberParser::parseUnsigned(index));
}


ServerObject*
ServerContainer::getChildForIndex(ui4 index, bool init)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_serverLock);

    if (!_pDataModel) {
        // currently, only server containers with data models are handled
        Log::instance()->upnpav().error("server container without data model, can not retrieve child objects");
        return 0;
    }

    ServerObject* pObject = 0;
    if (_pObjectCache && !updateCacheThreadIsRunning() && !cacheNeedsUpdate()) {
        // get media object out of data base cache (column xml)
         pObject = _pObjectCache->getMediaObjectForIndex(index);
    }
    else {
        std::string path = _pDataModel->getPath(index);
        Log::instance()->upnpav().debug("server container, get child from data model with index: " + Poco::NumberFormatter::format(index) + ", path: " + path);
        pObject = _pDataModel->getMediaObject(path);
    }
    if (pObject) {
        initChild(pObject, index, init);
    }
    return pObject;
}


ui4
ServerContainer::getChildrenAtRowOffset(std::vector<ServerObject*>& children, ui4 offset, ui4 count, const std::string& sort, const std::string& search)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_serverLock);

     if (!_pDataModel) {
        // currently, only server containers with data models are handled
        Log::instance()->upnpav().error("server container without data model, can not retrieve child objects");
        return 0;
    }

    ui4 childCount = 0;
    bool updateCache = cacheNeedsUpdate();
    if (_pObjectCache && !updateCache) {
        childCount = _pObjectCache->getBlockAtRow(children, offset, count, sort, search);
    }
    else {
        childCount = _pDataModel->getBlockAtRow(children, offset, count, sort, search);
    }
    for (std::vector<ServerObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        initChild(*it, (*it)->getIndex());
    }
    return childCount;
}


void
ServerContainer::initChild(ServerObject* pObject, ui4 index, bool init)
{
    Log::instance()->upnpav().debug("server container, init child with title: " + pObject->getTitle());

    pObject->setIndex(index);
    std::string path = _pDataModel->getPath(index);
    std::string parentPath = _pDataModel->getParentPath(path);
    if (parentPath == "") {
        pObject->setParentIndex(AbstractDataModel::INVALID_INDEX);
    }
    else {
        pObject->setParentIndex(_pDataModel->getIndex(parentPath));
    }

    if (!init) {
        return;
    }

//    ServerObject* pParent;
//    if (parentPath == "") {
//        pParent = this;
//    }
//    else {
//        pParent = _pDataModel->getMediaObject(parentPath);
//    }
//    pObject->setParent(pParent);

    if (!pObject->isContainer()) {
        std::string serverAddress = _pServer->getServerAddress();
        std::string relativeObjectId = Poco::NumberFormatter::format(index);
        std::string resourceUri = serverAddress + "/" + relativeObjectId;
        for (int r = 0; r < pObject->getResourceCount(); r++) {
            AbstractResource* pResource = pObject->getResource(r);
            if (pResource) {
                pResource->setUri(resourceUri + "$" + Poco::NumberFormatter::format(r));
            }
        }
    }
}


void
ServerObjectWriter::writeChildren(std::string& meta, const std::vector<ServerObject*>& children, const std::string& filter)
{
    Log::instance()->upnpav().debug("ServerObjectWriter::writeChildren()");
    writeMetaDataHeader();
    for (std::vector<ServerObject*>::const_iterator it = children.begin(); it != children.end(); ++it) {
        ServerObjectWriter writer;
        writer.writeMetaData(_pDidl, *it);
    }
    writeMetaDataClose(meta);
}


DatabaseCache::DatabaseCache() :
_pSession(0)
{
    Log::instance()->upnpav().debug("database cache ctor");
    Poco::Data::SQLite::Connector::registerConnector();
}


DatabaseCache::~DatabaseCache()
{
    if (_pSession) {
        delete _pSession;
    }
    Poco::Data::SQLite::Connector::unregisterConnector();
}


void
DatabaseCache::setCacheFilePath(const std::string& cacheFilePath)
{
    Log::instance()->upnpav().debug("database cache set cache file path: " + cacheFilePath);
    _cacheFilePath = cacheFilePath;
    _pSession = new Poco::Data::Session("SQLite", cacheFilePath);
    try {
//        *_pSession << "CREATE TABLE objcache (idx UNSIGNED INT(4), paridx UNSIGNED INT(4), class VARCHAR(30), title VARCHAR, artist VARCHAR, album VARCHAR, track INTEGER(2), xml VARCHAR)",
        *_pSession << "CREATE TABLE objcache (idx INTEGER(8), paridx INTEGER(8), class VARCHAR(30), title VARCHAR, artist VARCHAR, album VARCHAR, track INTEGER(2), xml VARCHAR)",
                Poco::Data::now;
    }
    catch (Poco::Exception& e) {
        Log::instance()->upnpav().warning("database cache creating object cache table failed: " + e.displayText());
    }
    try {
        *_pSession << "CREATE UNIQUE INDEX idx ON objcache (idx)", Poco::Data::now;
    }
    catch (Poco::Exception& e) {
        Log::instance()->upnpav().warning("database cache creating index on object cache table failed: " + e.displayText());
    }
}


ui4
DatabaseCache::rowCount()
{
    Poco::Data::Statement select(*_pSession);
    std::string statement = "SELECT idx FROM objcache";
    select << statement;
    Poco::Data::RecordSet recordSet(select);
    try {
        select.execute();
    }
    catch (Poco::Exception& e) {
        Log::instance()->upnpav().warning("database cache get row count failed: " + e.displayText());
    }
    return recordSet.rowCount();
}


ServerObject*
DatabaseCache::getMediaObjectForIndex(ui4 index)
{
    Log::instance()->upnpav().debug("database cache get object for index: " + Poco::NumberFormatter::format(index));

    std::vector<std::string> xml;
    try {
        *_pSession << "SELECT xml FROM objcache WHERE idx = :index", Poco::Data::use(index), Poco::Data::into(xml), Poco::Data::now;
    }
    catch (Poco::Exception& e) {
        Log::instance()->upnpav().warning("database cache get object for index failed: " + e.displayText());
    }
    Log::instance()->upnpav().debug("database cache get xml for object with index: " + Poco::NumberFormatter::format(index));
    if (xml.size() == 1) {
        Log::instance()->upnpav().debug("database cache got xml: " + xml[0]);
        ServerObject* pObject = _pServerContainer->createChildObject();
        MediaObjectReader xmlReader;
        xmlReader.read(pObject, xml[0]);
        return pObject;
    }
    else {
        Log::instance()->upnpav().warning("database cache get object for index reading meta data failed.");
        return 0;
    }
}


ui4
DatabaseCache::getBlockAtRow(std::vector<ServerObject*>& block, ui4 offset, ui4 count, const std::string& sort, const std::string& search)
{
    Log::instance()->upnpav().debug("database cache get block at offset: " + Poco::NumberFormatter::format(offset) + ", count: " + Poco::NumberFormatter::format(count));

    ui4 index;
    std::string xml;
    Poco::Data::Statement select(*_pSession);
    std::string statement = "SELECT idx, xml FROM objcache";
    if (search != "*") {
        statement += " WHERE " + search;
    }
//    statement += " ORDER BY artist, album, track, title";
    select << statement;
    Poco::Data::RecordSet recordSet(select);
    try {
        select.execute();
        // move to offset
        recordSet.moveFirst();
        for (ui4 r = 0; r < offset; r++) {
            recordSet.moveNext();
        }
    }
    catch (Poco::Exception& e) {
        Log::instance()->upnpav().warning("database cache get block executing query and moving to offset failed: " + e.displayText());
    }
    for (ui4 r = 0; r < count; r++) {
        // get block
        try {
            index = recordSet["idx"].convert<ui4>();
            xml = recordSet["xml"].convert<std::string>();
            ServerObject* pObject = _pServerContainer->createChildObject();
            MediaObjectReader xmlReader;
            xmlReader.read(pObject, xml);
            pObject->setIndex(index);
            block.push_back(pObject);
            recordSet.moveNext();
        }
        catch (Poco::Exception& e) {
            Log::instance()->upnpav().warning("database cache get block data for row " + Poco::NumberFormatter::format(r) + " failed: " + e.displayText());
        }
    }
    return recordSet.rowCount();
}


void
DatabaseCache::insertMediaObject(ServerObject* pObject)
{
    Log::instance()->upnpav().debug("database cache inserting media object with index: " + Poco::NumberFormatter::format(pObject->getIndex()));
    std::string xml;
    MediaObjectWriter2 xmlWriter;
    xmlWriter.write(xml, pObject);
    std::string artist;
    AbstractProperty* pProperty = pObject->getProperty(AvProperty::ARTIST);
    if (pProperty) {
        artist = pProperty->getValue();
    }
    std::string album;
    pProperty = pObject->getProperty(AvProperty::ALBUM);
    if (pProperty) {
        album = pProperty->getValue();
    }
    std::string track;
    pProperty = pObject->getProperty(AvProperty::ORIGINAL_TRACK_NUMBER);
    if (pProperty) {
        track = pProperty->getValue();
    }
    try {
        *_pSession << "INSERT INTO objcache (idx, paridx, class, title, artist, album, track, xml) VALUES(:idx, :paridx, :class, :title, :artist, :album, :track, :xml)",
                Poco::Data::use(pObject->getIndex()),
                Poco::Data::use(pObject->getParentIndex()),
                Poco::Data::use(pObject->getClass()),
                Poco::Data::use(pObject->getTitle()),
                Poco::Data::use(artist),
                Poco::Data::use(album),
                Poco::Data::use(track),
                Poco::Data::use(xml),
                Poco::Data::now;
    }
    catch (Poco::Exception& e) {
        Log::instance()->upnpav().debug("database cache inserting media object failed: " + e.displayText());
    }
}


void
DatabaseCache::insertBlock(std::vector<ServerObject*>& block)
{
    Log::instance()->upnpav().debug("database cache inserting media object block");
    typedef Poco::Tuple<ui4, ui4, std::string, std::string, std::string, std::string, int, std::string> MediaObject;
    std::vector<MediaObject> tupleBlock;
    for (std::vector<ServerObject*>::iterator it = block.begin(); it != block.end(); ++it) {
        ServerObject* pObject = *it;
        std::string xml;
        MediaObjectWriter2 xmlWriter;
        xmlWriter.write(xml, pObject);
        std::string artist;
        AbstractProperty* pProperty = pObject->getProperty(AvProperty::ARTIST);
        if (pProperty) {
            artist = pProperty->getValue();
        }
        std::string album;
        pProperty = pObject->getProperty(AvProperty::ALBUM);
        if (pProperty) {
            album = pProperty->getValue();
        }
        Variant trackVariant;
        pProperty = pObject->getProperty(AvProperty::ORIGINAL_TRACK_NUMBER);
        if (pProperty) {
            trackVariant.setValue(pProperty->getValue());
        }
        int track;
        trackVariant.getValue(track);

        tupleBlock.push_back(MediaObject(pObject->getIndex(), pObject->getParentIndex(), pObject->getClass(), pObject->getTitle(), artist, album, track, xml));
    }

    try {
        *_pSession << "INSERT INTO objcache (idx, paridx, class, title, artist, album, track, xml) VALUES(:idx, :paridx, :class, :title, :artist, :album, :track, :xml)",
                Poco::Data::use(tupleBlock),
                Poco::Data::now;
    }
    catch (Poco::Exception& e) {
        Log::instance()->upnpav().debug("database cache inserting media object failed: " + e.displayText());
    }
}


const ui4 AbstractDataModel::INVALID_INDEX = 0xffffffff;

AbstractDataModel::AbstractDataModel() :
_pServerContainer(0),
_maxIndex(0)
//_indexBufferSize(50)
{
}


void
AbstractDataModel::setServerContainer(ServerContainer* pServerContainer)
{
    _pServerContainer = pServerContainer;
}


ServerContainer*
AbstractDataModel::getServerContainer()
{
    return _pServerContainer;
}


void
AbstractDataModel::setBasePath(const std::string& basePath)
{
    _basePath = basePath;
    _indexFilePath = Poco::Path(Util::Home::instance()->getMetaDirPath(basePath), "index");
    Omm::Av::Log::instance()->upnpav().debug("data model scan ...");
    scan(true);
    Omm::Av::Log::instance()->upnpav().debug("data model scan finished.");
}


std::string
AbstractDataModel::getBasePath()
{
    return _basePath.toString();
}


ui4
AbstractDataModel::getIndexCount()
{
    return _indexMap.size();
}


bool
AbstractDataModel::hasIndex(ui4 index)
{
    return _indexMap.find(index) != _indexMap.end();
}


ui4
AbstractDataModel::getIndex(const std::string& path)
{
    // check if path is not in the cache
    std::map<std::string, ui4>::const_iterator pos = _pathMap.find(path);
    if (pos  != _pathMap.end()) {
        return (*pos).second;
    }
    else {
        Log::instance()->upnpav().error("abstract data model, could not retrieve index from path: " + path);
        return INVALID_INDEX;
    }
}


std::string
AbstractDataModel::getPath(ui4 index)
{
    std::map<ui4, std::string>::const_iterator pos = _indexMap.find(index);
    if (pos  != _indexMap.end()) {
        return (*pos).second;
    }
    else {
        Log::instance()->upnpav().error("abstract data model, could not retrieve path from index: " + Poco::NumberFormatter::format(index));
        return "";
    }
}


AbstractDataModel::IndexIterator
AbstractDataModel::beginIndex()
{
    return _indexMap.begin();
}


AbstractDataModel::IndexIterator
AbstractDataModel::endIndex()
{
    return _indexMap.end();
}


void
AbstractDataModel::addPath(const std::string& path, const std::string& resourcePath)
{
    ui4 index;
    // got a free index lying around?
    if (!_freeIndices.empty()) {
        index = _freeIndices.top();
        _freeIndices.pop();
    }
    else {
        index = _maxIndex++;
        if (index == INVALID_INDEX) {
            Log::instance()->upnpav().error("abstract data model max index reached, can not add path: " + path);
            return;
        }
        Log::instance()->upnpav().debug("abstract data model add path: " + path + " with index: " + Poco::NumberFormatter::format(index));
    }
    // create a new index
    _pathMap[path] = index;
    _indexMap[index] = path;
    if (resourcePath != "") {
        _resourceMap.insert(std::pair<ui4, std::string>(index, resourcePath));
    }
}


void
AbstractDataModel::removePath(const std::string& path)
{
    std::map<std::string, ui4>::iterator pos = _pathMap.find(path);
    if (pos  != _pathMap.end()) {
        _indexMap.erase((*pos).second);
        _pathMap.erase(pos);
        _resourceMap.erase((*pos).second);
    }
    else {
        Log::instance()->upnpav().error("abstract data model, could not erase path from index cache: " + path);
    }
}


ui4
AbstractDataModel::getBlockAtRow(std::vector<ServerObject*>& block, ui4 offset, ui4 count, const std::string& sort, const std::string& search)
{
    // TODO: should be faster with a method getIndexBlock(), implemented with an additional std::vector<ui4> as a sorted index list
    // TODO: implement building sort indices and row filtering in memory without data base, currently sort and search are ignored
    if (sort != "" || search != "*") {
        return 0;
    }
    ui4 r = 0;
    for (IndexIterator it = beginIndex(); (it != endIndex()) && (r < offset + count); ++it) {
        if (r >= offset) {
            ServerObject* pObject = getMediaObject((*it).second);
            block.push_back(pObject);
        }
        r++;
    }
    return getIndexCount();
}


void
AbstractDataModel::readIndexCache()
{
    if (!Poco::File(_indexFilePath).exists()) {
        Omm::Av::Log::instance()->upnpav().debug("index cache not present, not reading it");
        return;
    }
    Omm::Av::Log::instance()->upnpav().debug("index cache present, reading ...");
    std::ifstream indexCache(_indexFilePath.toString().c_str());
    std::string line;
    ui4 index = 0;
    ui4 lastIndex = 0;
    _maxIndex = 0;
    std::string path;
    while(getline(indexCache, line)) {
        std::string::size_type pos = line.find(' ');
        index = Poco::NumberParser::parse(line.substr(0, pos));
        path = line.substr(pos + 1);
        _indexMap[index] = path;
        _pathMap[path] = index;
        for (ui4 i = lastIndex + 1; i < index; i++) {
            _freeIndices.push(i);
        }
        lastIndex = index;
    }
    _maxIndex = index;
    // TODO: read resource map
    Omm::Av::Log::instance()->upnpav().debug("index cache reading finished.");
}


void
AbstractDataModel::writeIndexCache()
{
    if (Poco::File(_indexFilePath).exists()) {
        // TODO: check if index cache needs update
        return;
    }
    Log::instance()->upnpav().debug("abstract data model write index cache to: " + _indexFilePath.toString() + " ...");
    std::ofstream indexCache(_indexFilePath.toString().c_str());
    for (std::map<ui4, std::string>::iterator it = _indexMap.begin(); it != _indexMap.end(); ++it) {
//        Log::instance()->upnpav().debug("abstract data model write index: " + Poco::NumberFormatter::format((*it).first) + ", path: " + (*it).second);
        indexCache << (*it).first << ' ' << (*it).second << std::endl;
    }
    // TODO: write resource map
    Log::instance()->upnpav().debug("abstract data model write index cache finished.");
}


ServerObject*
SimpleDataModel::getMediaObject(const std::string& path)
{
//    Log::instance()->upnpav().debug("simple data model get media object for index: " + Poco::NumberFormatter::format(index) + " ...");
    Log::instance()->upnpav().debug("simple data model get media object for path: " + path + " ...");

//    Omm::Av::MemoryMediaObject* pItem = new Omm::Av::MemoryMediaObject;
//    std::string path = getPath(index);
    ServerItem* pItem = getServerContainer()->createMediaItem();

    pItem->setClass(getClass(path));
    pItem->setTitle(getTitle(path));

    std::string artist = getOptionalProperty(path, AvProperty::ARTIST);
    if (artist != "") {
        pItem->setUniqueProperty(AvProperty::ARTIST, artist);
    }
    std::string album = getOptionalProperty(path, AvProperty::ALBUM);
    if (album != "") {
        pItem->setUniqueProperty(AvProperty::ALBUM, album);
    }
    std::string track = getOptionalProperty(path, AvProperty::ORIGINAL_TRACK_NUMBER);
    if (track != "") {
        pItem->setUniqueProperty(AvProperty::ORIGINAL_TRACK_NUMBER, track);
    }
    std::string genre = getOptionalProperty(path, AvProperty::GENRE);
    if (genre != "") {
        pItem->setUniqueProperty(AvProperty::GENRE, genre);
    }

    ServerObjectResource* pResource = pItem->createResource();
//    Omm::Av::MemoryResource* pResource = new Omm::Av::MemoryResource;
    pResource->setSize(getSize(path));
    // FIXME: add some parts of protinfo in server container / media server.
    pResource->setProtInfo("http-get:*:" + getMime(path) + ":" + getDlna(path));
    pItem->addResource(pResource);
    // TODO: add icon property
//        pItem->_icon = pItem->_path;

    return pItem;
}


} // namespace Av
} // namespace Omm
