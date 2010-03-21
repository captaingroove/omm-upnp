/***************************************************************************|
|  Jamm - Just another multimedia ...                                       |
|         ... set of applications and libraries based on the UPnP-AV specs  |
|                                                                           |
|  Copyright (C) 2009                                                       |
|  Jörg Bakker (joerg'at'hakker'dot'de)                                     |
|                                                                           |
|  This file is part of Jamm.                                               |
|                                                                           |
|  Jamm is free software: you can redistribute it and/or modify             |
|  it under the terms of the GNU General Public License as published by     |
|  the Free Software Foundation version 3 of the License.                   |
|                                                                           |
|  Jamm is distributed in the hope that it will be useful,                  |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of           |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            |
|  GNU General Public License for more details.                             |
|                                                                           |
|  You should have received a copy of the GNU General Public License        |
|  along with this program.  If not, see <http://www.gnu.org/licenses/>.    |
 ***************************************************************************/
#ifndef JAMMUPNPAV_SERVER_H
#define JAMMUPNPAV_SERVER_H

#include "UpnpAvTypes.h"
#include "UpnpAvDevices.h"


namespace Jamm {
namespace Av {

class MediaItemServer
{
    friend class ItemRequestHandler;
    friend class MediaServerContainer;
    
public:
    MediaItemServer();
    ~MediaItemServer();
    
    void start();
    void stop();
    Poco::UInt16 getPort() const;
    
    void registerMediaItem(const std::string& relObjectId, MediaItem* pMediaItem, const std::string& privateUri = "");
    
private:
    Poco::Net::ServerSocket                     m_socket;
    Poco::Net::HTTPServer*                      m_pHttpServer;
    std::map<std::string,std::string>           m_privateUriMap;
    std::map<std::string,MediaItem*>            m_itemMap;
};


class ItemRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    ItemRequestHandler(MediaItemServer* pItemServer);
    
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
    
private:
    MediaItemServer*  m_pItemServer;
};


class MediaServerContainer : public MediaContainer
{
public:
    MediaServerContainer(const std::string& title, const std::string& subClass = "");
    ~MediaServerContainer();
    
    void appendChild(const std::string& objectId, MediaItem* pMediaItem);
    
private:
    MediaItemServer*    m_pItemServer;
    int                 m_port;
    std::string         m_address;
};


class ItemRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
    ItemRequestHandlerFactory(MediaItemServer* pItemServer);
    
    
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request);
    
private:
    MediaItemServer*  m_pItemServer;
};


class UpnpAvServer : public MediaServer
{
public:
    UpnpAvServer();

    void setRoot(MediaObject* pRoot);
    MediaObject* getRoot();

private:
    MediaObject* m_pRoot;
};

} // namespace Av
} // namespace Jamm

#endif
