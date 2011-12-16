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

#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/SplitterChannel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/Environment.h>
#include <Poco/NumberFormatter.h>

#include "Util.h"

namespace Omm {
namespace Util {


Log* Log::_pInstance = 0;

// possible log levels: trace, debug, information, notice, warning, error, critical, fatal

Log::Log()
{
    Poco::FormattingChannel* pFormatLogger = new Poco::FormattingChannel(new Poco::PatternFormatter("%H:%M:%S.%i %N[%P,%I] %q %s %t"));
    Poco::SplitterChannel* pSplitterChannel = new Poco::SplitterChannel;
    Poco::ConsoleChannel* pConsoleChannel = new Poco::ConsoleChannel;
//     Poco::FileChannel* pFileChannel = new Poco::FileChannel("omm.log");
    pSplitterChannel->addChannel(pConsoleChannel);
//     pSplitterChannel->addChannel(pFileChannel);
    pFormatLogger->setChannel(pSplitterChannel);
    pFormatLogger->open();
#ifdef NDEBUG
    _pUtilLogger = &Poco::Logger::create("UTIL", pFormatLogger, 0);
    _pPluginLogger = &Poco::Logger::create("PLUGIN", pFormatLogger, 0);
#else
    _pUtilLogger = &Poco::Logger::create("UTIL", pFormatLogger, Poco::Message::PRIO_DEBUG);
    _pPluginLogger = &Poco::Logger::create("PLUGIN", pFormatLogger, Poco::Message::PRIO_DEBUG);
#endif
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
Log::util()
{
    return *_pUtilLogger;
}


Poco::Logger&
Log::plugin()
{
    return *_pPluginLogger;
}


std::string Home::_home = "";
const std::string Home::_defaultHome("/var");
Poco::FastMutex Home::_lock;

const std::string
Home::getHomePath()
{
    Poco::FastMutex::ScopedLock lock(_lock);

    if (_home == "") {
        _home = Poco::Environment::get("OMM_HOME", Poco::Environment::get("HOME", _defaultHome));
        Log::instance()->util().information("OMM HOME: " + _home);
    }
    if (_home == Poco::Environment::get("HOME")) {
        _home += ".omm";
    }
    else {
        _home += "omm";
    }
    return _home;
}


const std::string
Home::getCachePath()
{
    Poco::FastMutex::ScopedLock lock(_lock);

    return Poco::Environment::get("OMM_CACHE", getHomePath() + "/" + "cache");
}


const std::string
Home::getConfigPath()
{
    Poco::FastMutex::ScopedLock lock(_lock);

    return Poco::Environment::get("OMM_CONFIG", getHomePath() + "/" + "config");
}


void
Startable::startAsThread()
{
#if (POCO_VERSION & 0xFFFFFFFF) >= 0x01350000
    _thread.setOSPriority(Poco::Thread::getMinOSPriority());
#endif
    _thread.start(*this);
}


void
Startable::stopThread()
{
    stop();
    _thread.join();
}


void
Startable::run()
{
    start();
}


} // namespace Util
} // namespace Omm
