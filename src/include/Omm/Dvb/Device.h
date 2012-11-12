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

#ifndef Device_INCLUDED
#define Device_INCLUDED

#include <cstring>

#include <Poco/Timestamp.h>
#include <Poco/Thread.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/Mutex.h>
#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/StringTokenizer.h>
#include <Poco/DOM/Document.h>

#include "../AvStream.h"

namespace Omm {
namespace Dvb {

class UnixFileIStream;
class Section;
class Stream;
class Service;
class Device;
class Frontend;
class Transponder;
class SignalCheckThread;
class Lnb;
class Frontend;
class Demux;
class Mux;
class Dvr;
class Adapter;


class Adapter
{
    friend class Device;
    friend class Frontend;
    friend class Demux;
    friend class Dvr;

public:
    Adapter(int num);
    ~Adapter();

    void addFrontend(Frontend* pFrontend);
    void openAdapter();
    void closeAdapter();

    void getDeviceInfo();  // TODO: implement DvbAdapter::getDeviceInfo()
//    Demux* getDemux();
//    Dvr* getDvr();

    void readXml(Poco::XML::Node* pXmlAdapter);
    void writeXml(Poco::XML::Element* pDvbDevice);

private:
    int                         _num;
    std::string                 _deviceName;
    std::vector<Frontend*>      _frontends;
    bool                        _recPsi;
};


class Device
{
    friend class Adapter;
    friend class SignalCheckThread;

public:
    typedef enum {ModeDvr, ModeMultiplex, ModeDvrMultiplex, ModeElementaryStreams} Mode;

    static Device* instance();

    typedef std::map<std::string, std::vector<Transponder*> >::iterator ServiceIterator;
    ServiceIterator serviceBegin();
    ServiceIterator serviceEnd();

    void open();
    void close();
    void scan(const std::string& initialTransponderData);
    void readXml(std::istream& istream);
    void writeXml(std::ostream& ostream);

    void addAdapter(Adapter* pAdapter);

    Transponder* getTransponder(const std::string& serviceName);

    std::istream* getStream(const std::string& serviceName);
    void freeStream(std::istream* pIstream);

    Mode getMode();
    bool blockDvrDevice();

private:
    Device();
    ~Device();

    void initServiceMap();
    void clearServiceMap();
    void clearAdapters();

    static Device*                                      _pInstance;

    std::vector<Adapter*>                               _adapters;
    std::map<std::string, std::vector<Transponder*> >   _serviceMap;
    std::map<std::istream*, Service*>                   _streamMap;

    Mode                                                _mode;
//    bool                                                _useDvrDevice;
    bool                                                _blockDvrDevice;
//    bool                                                _reopenDvrDevice;

    // TODO: move _pMux into Demux, because every Demuxer needs it's own Muxer
    Mux*                                                _pMux;
};

}  // namespace Omm
}  // namespace Dvb

#endif