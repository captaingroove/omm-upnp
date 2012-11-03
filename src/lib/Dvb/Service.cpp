/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2009, 2010, 2011, 2012                                     |
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

#include <Poco/NumberParser.h>
#include <Poco/DOM/AbstractContainerNode.h>
#include <Poco/DOM/DOMException.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/AttrMap.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/NumberFormatter.h>

#include "Log.h"
#include "DvbLogger.h"
#include "Stream.h"
#include "Service.h"
#include "Transponder.h"


namespace Omm {
namespace Dvb {


const std::string Service::TypeDigitalTelevision("DigitalTelevision");
const std::string Service::TypeDigitalRadioSound("DigitalRadioSound");
const std::string Service::TypeTeletext("Teletext");
const std::string Service::TypeNvodReference("NvodReference");
const std::string Service::TypeNodTimeShifted("NodTimeShifted");
const std::string Service::TypeMosaic("Mosaic");
const std::string Service::TypeFmRadio("FmRadio");
const std::string Service::TypeDvbSrm("DvbSrm");
const std::string Service::TypeAdvancedCodecDigitalRadioSound("AdvancedCodecDigitalRadioSound");
const std::string Service::TypeAdvancedCodecMosaic("AdvancedCodecMosaic");
const std::string Service::TypeDataBroadcastService("DataBroadcastService");
const std::string Service::TypeRcsMap("RcsMap");
const std::string Service::TypeRcsFls("RcsFls");
const std::string Service::TypeDvbMhp("DvbMhp");
const std::string Service::TypeMpeg2HdDigitalTelevision("Mpeg2HdDigitalTelevision");
const std::string Service::TypeAdvancedCodecSdDigitalTelevision("AdvancedCodecSdDigitalTelevision");
const std::string Service::TypeAdvancedCodecSdNvodTimeShifted("AdvancedCodecSdNvodTimeShifted");
const std::string Service::TypeAdvancedCodecSdNvodReference("AdvancedCodecSdNvodReference");
const std::string Service::TypeAdvancedCodecHdDigitalTelevision("AdvancedCodecHdDigitalTelevision");
const std::string Service::TypeAdvancedCodecHdNvodTimeShifted("AdvancedCodecHdNvodTimeShifted");
const std::string Service::TypeAdvancedCodecHdNvodReference("AdvancedCodecHdNvodReference");
const std::string Service::TypeAdvancedCodecFrameCompatiblePlanoStereoscopicHdTelevision("TypeAdvancedCodecFrameCompatiblePlanoStereoscopicHdTelevision");
const std::string Service::TypeAdvancedCodecFrameCompatiblePlanoStereoscopicTimeShifted("TypeAdvancedCodecFrameCompatiblePlanoStereoscopicTimeShifted");
const std::string Service::TypeAdvancedCodecFrameCompatiblePlanoStereoscopicReference("TypeAdvancedCodecFrameCompatiblePlanoStereoscopicReference");

const unsigned int Service::InvalidPcrPid(0);

const std::string Service::StatusUndefined("Undefined");
const std::string Service::StatusNotRunning("NotRunning");
const std::string Service::StatusStartsShortly("StartsShortly");
const std::string Service::StatusPausing("Pausing");
const std::string Service::StatusRunning("Running");
const std::string Service::StatusOffAir("OffAir");

Service::Service(Transponder* pTransponder, const std::string& name, unsigned int sid, unsigned int pmtid) :
_pTransponder(pTransponder),
_name(name),
_sid(sid),
_pmtPid(pmtid),
_pcrPid(InvalidPcrPid),
_status(StatusUndefined),
_scrambled(false)
{
}


void
Service::addStream(Stream* pStream)
{
    _streams.push_back(pStream);
}


void
Service::readXml(Poco::XML::Node* pXmlService)
{
    LOG(dvb, debug, "read service ...");

    if (pXmlService->hasChildNodes()) {
        Poco::XML::Node* pXmlParam = pXmlService->firstChild();
        while (pXmlParam) {
            if (pXmlParam->nodeName() == "stream") {
                std::string type = static_cast<Poco::XML::Element*>(pXmlParam)->getAttribute("type");
                int pid = Poco::NumberParser::parse(static_cast<Poco::XML::Element*>(pXmlParam)->getAttribute("pid"));
                Stream* pStream = new Stream(type, pid);
                addStream(pStream);
            }
            else if (pXmlParam->nodeName() == "status") {
                Poco::XML::Node* pVal = pXmlParam->firstChild();
                if (!pVal) {
                    LOG(dvb, error, "dvb service status has no value");
                }
                else {
                    _status = pVal->innerText();
                }
            }
            else if (pXmlParam->nodeName() == "scrambled") {
                Poco::XML::Node* pVal = pXmlParam->firstChild();
                if (!pVal) {
                    LOG(dvb, error, "dvb service scrambled has no value");
                }
                else {
                    _scrambled = pVal->innerText() == "true" ? true : false;
                }
            }
            else if (pXmlParam->nodeName() == "providerName") {
                Poco::XML::Node* pVal = pXmlParam->firstChild();
                if (!pVal) {
                    LOG(dvb, error, "dvb service providerName has no value");
                }
                else {
                    _providerName = pVal->innerText();
                }
            }
            else if (pXmlParam->nodeName() == "type") {
                Poco::XML::Node* pVal = pXmlParam->firstChild();
                if (!pVal) {
                    LOG(dvb, error, "dvb service type has no value");
                }
                else {
                    _type = pVal->innerText();
                }
            }
            pXmlParam = pXmlParam->nextSibling();
        }
    }
    else {
        LOG(dvb, error, "dvb service description contains no stream");
        return;
    }

    LOG(dvb, debug, "read service.");
}


void
Service::writeXml(Poco::XML::Element* pTransponder)
{
    LOG(dvb, debug, "write service ...");

    Poco::XML::Document* pDoc = pTransponder->ownerDocument();
    Poco::AutoPtr<Poco::XML::Element> pService = pDoc->createElement("service");
    pTransponder->appendChild(pService);
//    pService->setAttribute("name", _name);
    pService->setAttribute("name", getName());
    pService->setAttribute("sid", Poco::NumberFormatter::format(_sid));
    pService->setAttribute("pmtid", Poco::NumberFormatter::format(_pmtPid));

    for (std::vector<Stream*>::iterator it = _streams.begin(); it != _streams.end(); ++it) {
        (*it)->writeXml(pService);
    }

    Poco::AutoPtr<Poco::XML::Element> pStatus = pDoc->createElement("status");
    Poco::AutoPtr<Poco::XML::Text> pStatusVal = pDoc->createTextNode(_status);
    pStatus->appendChild(pStatusVal);
    pService->appendChild(pStatus);

    Poco::AutoPtr<Poco::XML::Element> pScrambled = pDoc->createElement("scrambled");
    Poco::AutoPtr<Poco::XML::Text> pScrambledVal = pDoc->createTextNode(std::string(_scrambled ? "true" : "false"));
    pScrambled->appendChild(pScrambledVal);
    pService->appendChild(pScrambled);

    Poco::AutoPtr<Poco::XML::Element> pProviderName = pDoc->createElement("providerName");
    Poco::AutoPtr<Poco::XML::Text> pProviderNameVal = pDoc->createTextNode(_providerName);
    pProviderName->appendChild(pProviderNameVal);
    pService->appendChild(pProviderName);

    Poco::AutoPtr<Poco::XML::Element> pType = pDoc->createElement("type");
    Poco::AutoPtr<Poco::XML::Text> pTypeVal = pDoc->createTextNode(_type);
    pType->appendChild(pTypeVal);
    pService->appendChild(pType);

    LOG(dvb, debug, "wrote service.");
}


std::string
Service::getType()
{
    return _type;
}


bool
Service::isAudio()
{
    return (_type == TypeDigitalRadioSound || _type == TypeFmRadio || _type == TypeAdvancedCodecDigitalRadioSound);
}


bool
Service::isSdVideo()
{
    return (_type == TypeDigitalTelevision || _type == TypeAdvancedCodecSdDigitalTelevision);
}


bool
Service::isHdVideo()
{
    return (_type == TypeMpeg2HdDigitalTelevision || _type == TypeAdvancedCodecHdDigitalTelevision);
}


std::string
Service::getName()
{
    if (_name == "" || _name == ".") {
        return "Service " + Poco::NumberFormatter::format(_sid) + " (TS " + Poco::NumberFormatter::format(_pTransponder->_transportStreamId) + ")";
    }
    else {
        return _name;
    }
}


std::string
Service::getStatus()
{
    return _status;
}


bool
Service::getScrambled()
{
    return _scrambled;
}


Transponder*
Service::getTransponder()
{
    return _pTransponder;
}


Stream*
Service::getFirstAudioStream()
{
    for (std::vector<Stream*>::iterator it = _streams.begin(); it != _streams.end(); ++it) {
        if ((*it)->isAudio()) {
            return *it;
        }
    }
    return 0;
}


Stream*
Service::getFirstVideoStream()
{
    for (std::vector<Stream*>::iterator it = _streams.begin(); it != _streams.end(); ++it) {
        if ((*it)->isVideo()) {
            return *it;
        }
    }
    return 0;
}


std::string
Service::typeToString(Poco::UInt8 status)
{
    switch (status) {
        case 0x01:
            return TypeDigitalTelevision;
        case 0x02:
            return TypeDigitalRadioSound;
        case 0x03:
            return TypeTeletext;
        case 0x04:
            return TypeNvodReference;
        case 0x05:
            return TypeNodTimeShifted;
        case 0x06:
            return TypeMosaic;
        case 0x07:
            return TypeFmRadio;
        case 0x08:
            return TypeDvbSrm;
        case 0x0A:
            return TypeAdvancedCodecDigitalRadioSound;
        case 0x0B:
            return TypeAdvancedCodecMosaic;
        case 0x0C:
            return TypeDataBroadcastService;
        case 0x0E:
            return TypeRcsMap;
        case 0x0F:
            return TypeRcsFls;
        case 0x10:
            return TypeDvbMhp;
        case 0x11:
            return TypeMpeg2HdDigitalTelevision;
        case 0x16:
            return TypeAdvancedCodecSdDigitalTelevision;
        case 0x17:
            return TypeAdvancedCodecSdNvodTimeShifted;
        case 0x18:
            return TypeAdvancedCodecSdNvodReference;
        case 0x19:
            return TypeAdvancedCodecHdDigitalTelevision;
        case 0x1A:
            return TypeAdvancedCodecHdNvodTimeShifted;
        case 0x1B:
            return TypeAdvancedCodecHdNvodReference;
        case 0x1C:
            return TypeAdvancedCodecFrameCompatiblePlanoStereoscopicHdTelevision;
        case 0x1D:
            return TypeAdvancedCodecFrameCompatiblePlanoStereoscopicTimeShifted;
        case 0x1E:
            return TypeAdvancedCodecFrameCompatiblePlanoStereoscopicReference;
        default:
            return "";
    }
}


std::string
Service::statusToString(Poco::UInt8 status)
{
    switch (status) {
        case 0x00:
            return StatusUndefined;
        case 0x01:
            return StatusNotRunning;
        case 0x02:
            return StatusStartsShortly;
        case 0x03:
            return StatusPausing;
        case 0x04:
            return StatusRunning;
        case 0x05:
            return StatusOffAir;
        default:
            return "";
    }
}


}  // namespace Omm
}  // namespace Dvb
