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
#include <Poco/StreamCopier.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Timer.h>
#include <Poco/File.h>
#include <Poco/Observer.h>

#include <fstream>
#include <vector>

#include <Omm/Dvb/DvbLogger.h>
#include <Omm/Dvb/Transponder.h>
#include <Omm/Dvb/Service.h>

#include "DvbServer.h"


DvbModel::DvbModel()
{
    LOGNS(Omm::Dvb, dvb, debug, "dvb model ctor.");
}


void
DvbModel::init()
{
    LOGNS(Omm::Dvb, dvb, debug, "dvb model init ...");

    Poco::NotificationCenter::defaultCenter().addObserver(Poco::Observer<DvbModel, Omm::Dvb::ScanNotification>(*this, &DvbModel::onScanNotification));
    Poco::File xmlDeviceFile(getBasePath());

    Omm::Dvb::Device::instance()->detectAdapters();
    if (xmlDeviceFile.exists()) {
        std::ifstream xmlDevice(getBasePath().c_str());
        Omm::Dvb::Device::instance()->readXml(xmlDevice);
        Omm::Dvb::Device::instance()->open();
    }

    LOGNS(Omm::Dvb, dvb, debug, "dvb model init finished.");
}


void
DvbModel::deInit()
{
    LOGNS(Omm::Dvb, dvb, debug, "dvb model deInit ...");

    Omm::Dvb::Device::instance()->close();

    LOGNS(Omm::Dvb, dvb, debug, "dvb model deInit finished.");
}


std::string
DvbModel::getModelClass()
{
    return "DvbModel";
}


Poco::UInt64
DvbModel::getSystemUpdateId(bool checkMod)
{
    Poco::File xmlDeviceFile(getBasePath());
    if (xmlDeviceFile.exists()) {
        return xmlDeviceFile.getLastModified().epochTime();
    }
    else {
        return 0;
    }
}


void
DvbModel::scan()
{
    LOGNS(Omm::Dvb, dvb, debug, "dvb model scan ...");

    Poco::File xmlDeviceFile(getBasePath());
    if (xmlDeviceFile.exists()) {
        std::ifstream xmlDevice(getBasePath().c_str());
        Omm::Dvb::Device::instance()->readXml(xmlDevice);
        for (Omm::Dvb::Device::ServiceIterator it = Omm::Dvb::Device::instance()->serviceBegin(); it != Omm::Dvb::Device::instance()->serviceEnd(); ++it) {
            addPath(it->first);
        }
    }

    LOGNS(Omm::Dvb, dvb, debug, "dvb model scan finished.");
}


void
DvbModel::scanDeep()
{
    LOGNS(Omm::Dvb, dvb, debug, "dvb model deep scan ...");

    Omm::Dvb::Device::instance()->open();

    // do the big dvb transponder scan
    Omm::Dvb::Device::instance()->scan();

    for (Omm::Dvb::Device::ServiceIterator it = Omm::Dvb::Device::instance()->serviceBegin(); it != Omm::Dvb::Device::instance()->serviceEnd(); ++it) {
        addPath(it->first);
    }
    std::ofstream xmlDevice(getBasePath().c_str());
    Omm::Dvb::Device::instance()->writeXml(xmlDevice);

    LOGNS(Omm::Dvb, dvb, debug, "dvb model deep scan finished.");
}


std::string
DvbModel::getClass(const std::string& path)
{
    Omm::Dvb::Service* pService = Omm::Dvb::Device::instance()->getFirstTransponder(path)->getService(path);
    if (pService->isAudio()) {
        return Omm::Av::AvClass::className(Omm::Av::AvClass::ITEM, Omm::Av::AvClass::AUDIO_BROADCAST);
    }
    else if (pService->isSdVideo() || pService->isHdVideo()) {
        return Omm::Av::AvClass::className(Omm::Av::AvClass::ITEM, Omm::Av::AvClass::VIDEO_BROADCAST);
    }
    else {
        return Omm::Av::AvClass::className(Omm::Av::AvClass::ITEM);
    }
}


std::string
DvbModel::getTitle(const std::string& path)
{
    return path;
}


bool
DvbModel::isSeekable(const std::string& path, const std::string& resourcePath)
{
    return false;
}


std::istream*
DvbModel::getStream(const std::string& path, const std::string& resourcePath)
{
    return Omm::Dvb::Device::instance()->getStream(path);
}


void
DvbModel::freeStream(std::istream* pIstream)
{
    Omm::Dvb::Device::instance()->freeStream(pIstream);
}


std::string
DvbModel::getMime(const std::string& path)
{
    Omm::Dvb::Service* pService = Omm::Dvb::Device::instance()->getFirstTransponder(path)->getService(path);
    if (pService->isAudio()) {
        return Omm::Av::Mime::AUDIO_MPEG;
    }
    else if (pService->isSdVideo() || pService->isHdVideo()) {
        return Omm::Av::Mime::VIDEO_MPEG;
    }
    else {
        return "";
    }
}


std::string
DvbModel::getDlna(const std::string& path)
{
    // TODO: add DLNA string for mpeg audio streams
    return "DLNA.ORG_PN=MPEG_PS_PAL";
}


void
DvbModel::onScanNotification(Omm::Dvb::ScanNotification* pScanNotification)
{
    Omm::Dvb::Service* pService = pScanNotification->getService();

    if (pService && pService->getStatus() == Omm::Dvb::Service::StatusRunning
            && !pService->getScrambled()
            && (pService->isAudio() || pService->isSdVideo())) {
        addPath(pService->getName());
    }
}


#ifdef OMMPLUGIN
POCO_BEGIN_MANIFEST(Omm::Av::AbstractDataModel)
POCO_EXPORT_CLASS(DvbModel)
POCO_END_MANIFEST
#endif
