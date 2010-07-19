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
#include <Poco/ClassLibrary.h>

#include "WebRadioSimple.h"

WebradiosimpleServer::WebradiosimpleServer()
// MediaContainer("Web Radio", "musicContainer")
{
    setTitle("Simple Web Radio");
    setIsContainer(true);
    setObjectId("0");
    
    std::string protInfoMp3 = "http-get:*:audio/mpeg:*";
    std::string subClass = "audioItem.audioBroadcast";
    
    
    /*----------- media object with meta data completely hold in memory, streaming directly from internet ------------*/
    
    Omm::Av::AbstractMediaObject* pGrooveSalad = new Omm::Av::MemoryMediaObject;
    pGrooveSalad->setIsContainer(false);
    pGrooveSalad->setObjectId("1");
    pGrooveSalad->setTitle("SOMA FM - Groove Salad (mp3)");
    
    Omm::Av::AbstractResource* pGrooveSaladRes = pGrooveSalad->createResource();
    pGrooveSaladRes->setProtInfo(protInfoMp3);
    pGrooveSaladRes->setSize(0);
    pGrooveSaladRes->setUri("http://streamer-dtc-aa04.somafm.com:80/stream/1018");
    pGrooveSalad->addResource(pGrooveSaladRes);
    appendChild(pGrooveSalad);
    
    
    /*----------- media object with meta data completely hold in memory, streaming through local proxy ------------*/

    Omm::Av::AbstractMediaObject* pLush = new Omm::Av::StreamingMemoryMediaObject;
//     Omm::Av::AbstractMediaObject* pLush = new Omm::Av::AbstractMediaObject<MemoryObject,MemoryProperty,WebResource>;
//     Omm::Av::AbstractMediaObject* pLush = new Omm::Av::AbstractMediaObject(new MemoryObject, new MemoryProperty, new WebResource);
    Omm::Av::AbstractMediaObject* pLush = new Omm::Av::AbstractMediaObject(new FileDirectory("/base/path"), new MemoryProperty, new FileResource("/base/path");
    pLush->setIsContainer(false);
    pLush->setObjectId("2");
    pLush->setTitle("SOMA FM - Lush (mp3)");

    Omm::Av::AbstractResource* pLushRes = pLush->createResource();
    pLushRes->setProtInfo(protInfoMp3);
    pLushRes->setSize(0);
    pLush->addResource(pLushRes);
    appendChild(pLush);
};

POCO_BEGIN_MANIFEST(Omm::Av::AbstractMediaObject)
POCO_EXPORT_CLASS(WebradiosimpleServer)
POCO_END_MANIFEST
