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
#include <string>

#include <Poco/ClassLibrary.h>
#include <Poco/Observer.h>

#include "Omm/Util.h"
#include "AvStreamEngine.h"


AvStreamEngine::AvStreamEngine() :
_isPlaying(false),
_pTagger(0),
_pClock(0),
_pDemuxer(0),
_pAudioSink(0),
_pVideoSink(0),
_pSession(0)
{
    _engineId = "OMM AvStream engine " + Omm::OMM_VERSION;
}


AvStreamEngine::~AvStreamEngine()
{
    destructPlayer();
}


void
AvStreamEngine::createPlayer()
{
    _pClock = new Omm::AvStream::Clock;
    _pDemuxer = new Omm::AvStream::Demuxer;

    std::string taggerPlugin("tagger-ffmpeg");
    Omm::Util::PluginLoader<Omm::AvStream::Tagger> taggerPluginLoader;
    try {
        _pTagger = taggerPluginLoader.load(taggerPlugin, "Tagger", "FFmpeg");
    }
    catch(Poco::NotFoundException) {
        LOGNS(Omm::AvStream, avstream, error, "could not find avstream tagger plugin: " + taggerPlugin);
        return;
    }
#ifdef __LINUX__
    std::string audioPlugin("audiosink-alsa");
#else
    std::string audioPlugin("audiosink-sdl");
#endif
    Omm::Util::PluginLoader<Omm::AvStream::AudioSink> audioPluginLoader;
    try {
        _pAudioSink = audioPluginLoader.load(audioPlugin, "AudioSink");
    }
    catch(Poco::NotFoundException) {
        LOGNS(Omm::AvStream, avstream, error, "Error could not find avstream audio plugin: " + audioPlugin);
        return;
    }
    std::string videoPlugin("videosink-sdl");
    Omm::Util::PluginLoader<Omm::AvStream::VideoSink> videoPluginLoader;
    try {
        _pVideoSink = videoPluginLoader.load(videoPlugin, "VideoSink");
    }
    catch(Poco::NotFoundException) {
        LOGNS(Omm::AvStream, avstream, error, "Error could not find avstream video plugin: " + videoPlugin);
        return;
    }
    // FIXME: set a Sys::Visual here.
    _pVideoSink->openWindow(false, 1020, 576);
//    _pVideoSink->openWindow(_fullscreen, _width, _height);

    _pAudioSink->registerStreamEventObserver(new Poco::Observer<AvStreamEngine, Omm::AvStream::Sink::EndOfStream>(*this, &AvStreamEngine::endOfStream));
}


void
AvStreamEngine::destructPlayer()
{
    delete _pTagger;
    _pTagger = 0;
    delete _pClock;
    _pClock = 0;
    delete _pDemuxer;
    _pDemuxer = 0;
    delete _pAudioSink;
    _pAudioSink = 0;
    delete _pVideoSink;
    _pVideoSink = 0;
}


void
AvStreamEngine::setFullscreen(bool on)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


bool
AvStreamEngine::preferStdStream()
{
    return true;
//     return false;
}


void
AvStreamEngine::setUri(const std::string& uri, const Omm::Av::ProtocolInfo& protInfo)
{
    if (_isPlaying) {
        stop();
    }

    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
    LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE SET ... >>>>>>>>>>>>");
    _pDemuxer->set(_pTagger->tag(uri));
}


// void
// AvStreamEngine::setUri(std::string mrl)
// {
//     if (_isPlaying) {
//         stop();
//     }
//     Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
//     LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE SET ... >>>>>>>>>>>>");
//
//     _uri = mrl;
//     Poco::URI uri(mrl);
//     LOGNS(Omm::AvStream, avstream, debug, "getting stream of type: " + uri.getScheme());
//     if (uri.getScheme() == "http") {
//         _isFile = false;
//         _pSession = new Poco::Net::HTTPClientSession(uri.getHost(), uri.getPort());
//         Poco::Net::HTTPRequest request("GET", uri.getPathEtc());
//         _pSession->sendRequest(request);
//         std::stringstream requestHeader;
//         request.write(requestHeader);
//         LOGNS(Omm::AvStream, avstream, debug, "request header:\n" + requestHeader.str());
//
//         Poco::Net::HTTPResponse response;
//         std::istream& istr = _pSession->receiveResponse(response);
//
//         LOGNS(Omm::AvStream, avstream, information, "HTTP " + Poco::NumberFormatter::format(response.getStatus()) + " " + response.getReason());
//         std::stringstream responseHeader;
//         response.write(responseHeader);
//         LOGNS(Omm::AvStream, avstream, debug, "response header:\n" + responseHeader.str());
//         _pDemuxer->set(_pTagger->tag(istr));
//     }
//     else if (uri.getScheme() == "file" || uri.getScheme() == "") {
//         _isFile = true;
//         _file.open(uri.getPath().c_str());
//         _pDemuxer->set(_pTagger->tag(_file));
//     }
// }


void
AvStreamEngine::setUri(std::istream& istr, const Omm::Av::ProtocolInfo& protInfo)
{
    if (_isPlaying) {
        stop();
    }

    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
    LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE SET ... >>>>>>>>>>>>");
    _pDemuxer->set(_pTagger->tag(istr));
}


void
AvStreamEngine::play()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);

    if (_pDemuxer->firstAudioStream() < 0 && _pDemuxer->firstVideoStream() < 0) {
        LOGNS(Omm::AvStream, avstream, error, "no audio or video stream found, exiting");;
        return;
    }

    //////////// load and attach audio Sink ////////////
    if (_pDemuxer->firstAudioStream() >= 0) {
        _pDemuxer->attach(_pAudioSink, _pDemuxer->firstAudioStream());
        _pClock->attachAudioSink(_pAudioSink);
    }


    //////////// load and attach video sink ////////////
    if (_pDemuxer->firstVideoStream() >= 0) {
        _pDemuxer->attach(_pVideoSink, _pDemuxer->firstVideoStream());
        _pClock->attachVideoSink(_pVideoSink);
    }

    LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE START ... >>>>>>>>>>>>");

    _pDemuxer->start();
//     _pClock->setStartTime(true);
    _pClock->setStartTime(false);

    LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE RUN ... >>>>>>>>>>>>");

    _pClock->start();
    _isPlaying = true;
}


void
AvStreamEngine::stop()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);

    LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE HALT. >>>>>>>>>>>>");

    _pDemuxer->stop();
    _pClock->stop();

    LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE STOP. >>>>>>>>>>>>");

    ////////// deallocate meta data and packet queues ////////////
    if (_pDemuxer->firstAudioStream() >= 0) {
        _pDemuxer->detach(_pDemuxer->firstAudioStream());
    }

    if (_pDemuxer->firstVideoStream() >= 0) {
        _pDemuxer->detach(_pDemuxer->firstVideoStream());
    }
    LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE RESET. >>>>>>>>>>>>");
    _pClock->reset();

    if (_pDemuxer->firstAudioStream() >= 0) {
        _pAudioSink->reset();
    }

    if (_pDemuxer->firstVideoStream() >= 0) {
        _pVideoSink->reset();
    }
    // demuxer is last node to reset, because StreamInfo belongs to it and is refered to by downstream nodes.
    _pDemuxer->reset();

    if (_isFile) {
        _file.close();
    }

    _isPlaying = false;

    if (_pSession) {
        delete _pSession;
        _pSession = 0;
    }
    LOGNS(Omm::AvStream, avstream, debug, "<<<<<<<<<<<< ENGINE OFF. >>>>>>>>>>>>");
}


void
AvStreamEngine::next()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);

}


void
AvStreamEngine::previous()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


void
AvStreamEngine::pause()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


void
AvStreamEngine::seekByte(Poco::UInt64 byte)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


void
AvStreamEngine::seekPercentage(float percentage)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


void
AvStreamEngine::seekSecond(Omm::r8 second)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


bool
AvStreamEngine::isSeekable()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);

    // TODO: depends on http header received from server
    return false;
}


void
AvStreamEngine::savePosition()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


void
AvStreamEngine::setSpeed(int nom, int denom)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


Poco::UInt64
AvStreamEngine::getPositionByte()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


float
AvStreamEngine::getPositionPercentage()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


Omm::r8
AvStreamEngine::getPositionSecond()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


Omm::r8
AvStreamEngine::getLengthSeconds()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


AvStreamEngine::TransportState
AvStreamEngine::getTransportState()
{
    // TODO: implement getTransportState in AvStreamEngine.
}


void
AvStreamEngine::setVolume(const std::string& channel, float vol)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
    _pAudioSink->setVolume(vol);
//     _pAudioSink->setVolume(channel, vol);
}


float
AvStreamEngine::getVolume(const std::string& channel)
{
    Poco::ScopedLock<Poco::FastMutex> lock(_actionLock);
}


void
AvStreamEngine::endOfStream(Omm::AvStream::Sink::EndOfStream* eof)
{
    stop();
//    Engine::endOfStream();
}


#ifdef OMMPLUGIN
POCO_BEGIN_MANIFEST(Omm::Av::Engine)
POCO_EXPORT_CLASS(AvStreamEngine)
POCO_END_MANIFEST
#endif

