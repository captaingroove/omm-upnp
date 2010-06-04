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
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

#include <Poco/Exception.h>
#include <Poco/Thread.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/ClassLoader.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/SplitterChannel.h>
#include <Poco/ConsoleChannel.h>

#include <fstream>

#include "AvStream.h"

using namespace Omm;
using namespace Omm::AvStream;


#define PRINT_LIB_VERSION(outstream,libname,LIBNAME,indent) \
version= libname##_version(); \
fprintf(outstream, "%slib%-10s %2d.%2d.%2d / %2d.%2d.%2d\n", indent? "  " : "", #libname, \
LIB##LIBNAME##_VERSION_MAJOR, LIB##LIBNAME##_VERSION_MINOR, LIB##LIBNAME##_VERSION_MICRO, \
version >> 16, version >> 8 & 0xff, version & 0xff);

static void print_all_lib_versions(FILE* outstream, int indent)
{
    unsigned int version;
    PRINT_LIB_VERSION(outstream, avutil,   AVUTIL,   indent);
    PRINT_LIB_VERSION(outstream, avcodec,  AVCODEC,  indent);
    PRINT_LIB_VERSION(outstream, avformat, AVFORMAT, indent);
//     PRINT_LIB_VERSION(outstream, avdevice, AVDEVICE, indent);
// #if CONFIG_AVFILTER
//     PRINT_LIB_VERSION(outstream, avfilter, AVFILTER, indent);
// #endif
// #if CONFIG_SWSCALE
//     PRINT_LIB_VERSION(outstream, swscale,  SWSCALE,  indent);
// #endif
// #if CONFIG_POSTPROC
//     PRINT_LIB_VERSION(outstream, postproc, POSTPROC, indent);
// #endif
}

void show_banner(void)
{
    std::cout <<  "libommavstream Copyright (c) 2010 Joerg Bakker." << std::endl;
    print_all_lib_versions(stdout, 1);
    std::cout << "  built on " __DATE__ " " __TIME__ ;
#ifdef __GNUC__
    std::cout << ", gcc: " __VERSION__  << std::endl;
#else
    std::cout << ", using a non-gcc compiler" << std::endl;
#endif
}


Log* Log::_pInstance = 0;

// possible log levels: trace, debug, information, notice, warning, error, critical, fatal

Log::Log()
{
    Poco::FormattingChannel* pFormatLogger = new Poco::FormattingChannel(new Poco::PatternFormatter("%H:%M:%S.%i %N[%P,%I] %q %s %t"));
    Poco::SplitterChannel* pSplitterChannel = new Poco::SplitterChannel;
    Poco::ConsoleChannel* pConsoleChannel = new Poco::ConsoleChannel;
    pSplitterChannel->addChannel(pConsoleChannel);
//     Poco::FileChannel* pFileChannel = new Poco::FileChannel("ommavstream.log");
//     pSplitterChannel->addChannel(pFileChannel);
    pFormatLogger->setChannel(pSplitterChannel);
    pFormatLogger->open();
    _pAvStreamLogger = &Poco::Logger::create("AVSTREAM", pFormatLogger, Poco::Message::PRIO_TRACE);
//     _pAvStreamLogger = &Poco::Logger::create("AVSTREAM", pFormatLogger, Poco::Message::PRIO_DEBUG);
//     _pAvStreamLogger = &Poco::Logger::create("AVSTREAM", pFormatLogger, Poco::Message::PRIO_INFORMATION);
//     _pAvStreamLogger = &Poco::Logger::create("AVSTREAM", pFormatLogger, Poco::Message::PRIO_ERROR);
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
Log::avstream()
{
    return *_pAvStreamLogger;
}


ByteQueue::ByteQueue(int size) :
_size(size),
_level(0),
_writeSemaphore(1, 1),
_readSemaphore(0, 1)
{
//     _bytestream.open("bytestream");
}


void
ByteQueue::read(char* buffer, int num)
{
    int bytesRead = 0;
    while (bytesRead < num) {
        bytesRead += readSome(buffer, num - bytesRead);
        buffer += bytesRead;
    }
}


void
ByteQueue::write(char* buffer, int num)
{
    int bytesWritten = 0;
    while (bytesWritten < num) {
        bytesWritten += writeSome(buffer, num - bytesWritten);
        buffer += bytesWritten;
    }
}


int
ByteQueue::readSomeRwLock(char* buffer, int num)
{
    // block byte queue for further reading
    Log::instance()->avstream().trace("byte queue readSome() read lock wait ...");
    _readLock.writeLock();
    Log::instance()->avstream().trace("byte queue readSome() read lock success.");
    
    _lock.lock();
    
    Log::instance()->avstream().trace(Poco::format("byte queue readSome() trying to read %s bytes, level: %s",
        Poco::NumberFormatter::format(num), Poco::NumberFormatter::format(_level)));
    
    int bytesRead = (_level < num) ? _level : num;
    _bytestream.read(buffer, bytesRead);
    _level -= bytesRead;
    
    Log::instance()->avstream().trace(Poco::format("byte queue readSome() read %s bytes, level: %s",
        Poco::NumberFormatter::format(bytesRead), Poco::NumberFormatter::format(_level)));
    
    // queue is not empty, we can go on reading
    if (_level > 0) {
        Log::instance()->avstream().trace("byte queue readSome() trying to unlock read lock ...");
        _readLock.unlock();
        Log::instance()->avstream().trace("byte queue readSome() read lock unlocked");
    }
    
    _lock.unlock();
    // we've read some bytes, so we can put something in, again
    if (num > 0) {
        Log::instance()->avstream().trace("byte queue readSome() trying to unlock write lock ...");
        _writeLock.unlock();
        Log::instance()->avstream().trace("byte queue readSome() write lock unlocked");
    }  
    
    Log::instance()->avstream().trace(Poco::format("byte queue readSome() returns: %s",
        Poco::NumberFormatter::format(bytesRead)));
    
    return bytesRead;
}


int
ByteQueue::writeSomeRwLock(char* buffer, int num)
{
    // block byte queue for further writing
    Log::instance()->avstream().trace("byte queue writeSome() write lock wait ...");
    _writeLock.writeLock();
    Log::instance()->avstream().trace("byte queue writeSome() write lock success.");
    
    _lock.lock();
    
    Log::instance()->avstream().trace(Poco::format("byte queue writeSome() trying to write %s bytes, level: %s",
        Poco::NumberFormatter::format(num), Poco::NumberFormatter::format(_level)));
    
    int bytesWritten = (_size - _level < num) ? (_size - _level) : num;
    _bytestream.write(buffer, bytesWritten);
    _level += bytesWritten;
    
    Log::instance()->avstream().trace(Poco::format("byte queue writeSome() wrote %s bytes, level: %s",
        Poco::NumberFormatter::format(bytesWritten), Poco::NumberFormatter::format(_level)));
    
    // queue is not full, we can go on writing
    if (_level < _size) {
        Log::instance()->avstream().trace("byte queue writeSome() trying to unlock write lock ...");
        _writeLock.unlock();
        Log::instance()->avstream().trace("byte queue writeSome() write lock unlocked");
    }
    
    _lock.unlock();
    // we've written some bytes, so we can get something out, again
    if (num > 0/* && !_readSemaphoreSet*/) {
        Log::instance()->avstream().trace("byte queue writeSome() trying to unlock read lock ...");
        _readLock.unlock();
        Log::instance()->avstream().trace("byte queue writeSome() read lock unlocked");
    }  
    
    Log::instance()->avstream().trace(Poco::format("byte queue writeSome() returns: %s",
        Poco::NumberFormatter::format(bytesWritten)));
    
    return bytesWritten;
}


int
ByteQueue::readSome(char* buffer, int num)
{
    // block byte queue for further reading
    _readSemaphore.wait();
    
    _lock.lock();
    
//     Log::instance()->avstream().trace(Poco::format("byte queue readSome() trying to read %s bytes, level: %s",
//         Poco::NumberFormatter::format(num), Poco::NumberFormatter::format(_level)));
    
    int bytesRead = (_level < num) ? _level : num;
    _bytestream.read(buffer, bytesRead);
    _level -= bytesRead;
    
    Log::instance()->avstream().trace(Poco::format("byte queue readSome() read %s bytes, level: %s",
        Poco::NumberFormatter::format(bytesRead), Poco::NumberFormatter::format(_level)));
    
    // queue is not empty, we can go on reading
    if (_level > 0) {
        try {
            _readSemaphore.set();
        }
        catch (Poco::SystemException) {
        }
    }
    
    _lock.unlock();
    // we've read some bytes, so we can put something in, again
    if (num > 0) {
        try {
            _writeSemaphore.set();
        }
        catch (Poco::SystemException) {
        }
    }  
    
    return bytesRead;
}


int
ByteQueue::writeSome(char* buffer, int num)
{
    // block byte queue for further writing
    _writeSemaphore.wait();
    _lock.lock();
    
//     Log::instance()->avstream().trace(Poco::format("byte queue writeSome() trying to write %s bytes, level: %s",
//         Poco::NumberFormatter::format(num), Poco::NumberFormatter::format(_level)));
    
    int bytesWritten = (_size - _level < num) ? (_size - _level) : num;
    _bytestream.write(buffer, bytesWritten);
    _level += bytesWritten;
    
    Log::instance()->avstream().trace(Poco::format("byte queue writeSome() wrote %s bytes, level: %s",
        Poco::NumberFormatter::format(bytesWritten), Poco::NumberFormatter::format(_level)));
    
    // queue is not full, we can go on writing
    if (_level < _size) {
        try {
            _writeSemaphore.set();
        }
        catch (Poco::SystemException) {
        }
    }
    
    _lock.unlock();
    // we've written some bytes, so we can get something out, again
    if (num > 0) {
        try {
            _readSemaphore.set();
        }
        catch (Poco::SystemException) {
        }
    }  
    
    return bytesWritten;
}


int
ByteQueue::level()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_lock);
    return _level;
}


// TODO: queue name should contain the stream name, too.
StreamQueue::StreamQueue(Node* pNode, int size, int putTimeout, int getTimeout) :
Queue<Frame*>(pNode->getName() + " stream", size, putTimeout, getTimeout),
_pNode(pNode)
{
}


Node*
StreamQueue::getNode()
{
    return _pNode;
}


StreamInfo::StreamInfo() :
_streamName("avstream"),
_pAvStream(0),
_pAvCodecContext(0),
_pAvCodec(0),
_newFrameNumber(0)
{
}


bool
StreamInfo::findCodec()
{
    if (!_pAvStream || !_pAvStream->codec) {
        Log::instance()->avstream().error(Poco::format("missing stream info in %s while trying to find decoder", _streamName));
        return false;
    }
    _pAvCodecContext = _pAvStream->codec;
    
    //////////// find decoders for audio and video stream ////////////
    Log::instance()->avstream().debug(Poco::format("searching codec with codec id: %s",
        Poco::NumberFormatter::format(_pAvCodecContext->codec_id)));
    
    Log::instance()->avstream().trace("ffmpeg::avcodec_find_decoder() ...");
    _pAvCodec = avcodec_find_decoder(_pAvCodecContext->codec_id);
    
    if(!_pAvCodec) {
        Log::instance()->avstream().error(Poco::format("could not find decoder for codec id: %s",
            Poco::NumberFormatter::format(_pAvCodecContext->codec_id)));
        return false;
    }
    
    // Inform the codec that we can handle truncated bitstreams -- i.e.,
    // bitstreams where frame boundaries can fall in the middle of packets
//     if(_pAvCodec->capabilities & CODEC_CAP_TRUNCATED) {
//         _pAvCodecContext->flags |= CODEC_FLAG_TRUNCATED;
//     }
    
    Log::instance()->avstream().trace("ffmpeg::avcodec_open() ...");
    if(avcodec_open(_pAvCodecContext, _pAvCodec) < 0) {
        Log::instance()->avstream().error(Poco::format("could not open decoder for codec id: %s",
            Poco::NumberFormatter::format(_pAvCodecContext->codec_id)));
        return false;
    }
    Log::instance()->avstream().information(Poco::format("found codec: %s (%s)",
        std::string(_pAvCodec->name), std::string(_pAvCodec->long_name)));
    Log::instance()->avstream().information(Poco::format("start time: %s, duration: %s",
        Poco::NumberFormatter::format(_pAvStream->start_time),
        Poco::NumberFormatter::format(_pAvStream->duration)));
    
    // time_base: fundamental unit of time (in seconds) in terms of which frame timestamps are represented. 
    // This is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. For fixed-fps content,
    // time base should be 1/framerate and timestamp increments should be 1.
    Log::instance()->avstream().information(Poco::format("time base numerator: %s, denominator: %s",
        Poco::NumberFormatter::format(_pAvStream->time_base.num),
        Poco::NumberFormatter::format(_pAvStream->time_base.den)));
    
    // r_frame_rate: Real base framerate of the stream. 
    Log::instance()->avstream().information(Poco::format("base frame rate numerator: %s, denominator: %s",
        Poco::NumberFormatter::format(_pAvStream->r_frame_rate.num),
        Poco::NumberFormatter::format(_pAvStream->r_frame_rate.den)));
    
//     Log::instance()->avstream().information(Poco::format("average frame rate numerator: %s, denominator: %s",
//         Poco::NumberFormatter::format(_pAvStream->avg_frame_rate.num),
//         Poco::NumberFormatter::format(_pAvStream->avg_frame_rate.den)));
    
    // reference dts (for timestamp generation): Timestamp corresponding to the last dts sync point
    // Initialized when AVCodecParserContext.dts_sync_point >= 0 and
    // a DTS is received from the underlying container. Otherwise set to
    // AV_NOPTS_VALUE by default.
    Log::instance()->avstream().information(Poco::format("first dts: %s, current dts: %s, reference dts: %s, last IP pts: %s, last IP duration: %s",
        Poco::NumberFormatter::format(_pAvStream->first_dts),
        Poco::NumberFormatter::format(_pAvStream->cur_dts),
        Poco::NumberFormatter::format(_pAvStream->reference_dts),
        Poco::NumberFormatter::format(_pAvStream->last_IP_pts),
        Poco::NumberFormatter::format(_pAvStream->last_IP_duration)));
    
//     Log::instance()->avstream().trace(Poco::format("_pStreamInfo->_pAvCodecContext->codec_id %s",
//         Poco::NumberFormatter::format(_pAvCodecContext->codec_id)));
    return true;
    
    
//     if(_pVideoCodec->frame_rate > 1000 && _pVideoCodec->frame_rate_base == 1) {
//         _pVideoCodec->frame_rate_base = 1000;
//     }
}


bool
StreamInfo::isAudio()
{
    return _pAvStream->codec->codec_type == CODEC_TYPE_AUDIO;
}


bool
StreamInfo::isVideo()
{   
    return _pAvStream->codec->codec_type == CODEC_TYPE_VIDEO;
}


void
StreamInfo::printInfo()
{
    Log::instance()->avstream().debug(Poco::format("stream width: %s, height: %s",
        Poco::NumberFormatter::format(width()),
        Poco::NumberFormatter::format(height())));
}


int
StreamInfo::width()
{
    if (_pAvCodecContext) {
        return _pAvCodecContext->width;
    }
    else {
        return 0;
    }
}


int
StreamInfo::height()
{
    if (_pAvCodecContext) {
        return _pAvCodecContext->height;
    }
    else {
        return 0;
    }
}


PixelFormat
StreamInfo::pixelFormat()
{
    return _pAvCodecContext->pix_fmt;
}


int
StreamInfo::pictureSize()
{
    return avpicture_get_size(pixelFormat(), width(), height());
}


int
StreamInfo::sampleWidth()
{
    Log::instance()->avstream().debug(Poco::format("stream sample format: %s",
        Poco::NumberFormatter::format((int)_pAvStream->codec->sample_fmt)));
    
    // avcodec.h says sample_fmt is not used (always S16?)
    switch(_pAvStream->codec->sample_fmt) {
    case SAMPLE_FMT_U8:
        return 8; // beware unsigned!
        break;
    case SAMPLE_FMT_S16:
        return 16;
        break;
    case SAMPLE_FMT_S32:
        return 32;
        break;
    case SAMPLE_FMT_FLT:
        return -32;
        break;
    default:
        return 0;
    }
}


unsigned int
StreamInfo::sampleRate()
{
    Log::instance()->avstream().debug(Poco::format("stream sample rate: %s",
        Poco::NumberFormatter::format((int)_pAvStream->codec->sample_rate)));
    
    return _pAvStream->codec->sample_rate;
}


int
StreamInfo::channels()
{
    Log::instance()->avstream().debug(Poco::format("stream channels: %s",
        Poco::NumberFormatter::format((int)_pAvStream->codec->channels)));
    
    return _pAvStream->codec->channels;
}


int64_t
StreamInfo::newFrameNumber()
{
    return ++_newFrameNumber;
}


Stream::Stream(Node* pNode, const std::string name) :
_pNode(pNode),
_pStreamInfo(new StreamInfo),
_pStreamQueue(0)
{
    setName(name);
}


Stream::~Stream()
{
    // StreamInfo and StreamQueue belong to the nodes that are connected
    // so they are not deleted here
}


Frame*
Stream::firstFrame()
{
    if (!_pStreamQueue) {
        Log::instance()->avstream().warning(Poco::format("node %s [%s] get frame no queue attached, ignoring",
            getNode()->getName(), getName()));
        return 0;
    }
    else {
        Frame* pRes = _pStreamQueue->front();
        if (!pRes) {
            Log::instance()->avstream().warning(Poco::format("node %s [%s] first frame is null frame.",
                getNode()->getName(), getName()));
        }
        else {
            Log::instance()->avstream().trace(Poco::format("node %s [%s] first frame %s.",
                getNode()->getName(), getName(), pRes->getName()));
        }
        return pRes;
    }
}


Frame*
Stream::getFrame()
{
    if (!_pStreamQueue) {
        Log::instance()->avstream().warning(Poco::format("node %s [%s] get frame no queue attached, ignoring",
            getNode()->getName(), getName()));
        return 0;
    }
    else {
        Frame* pRes = _pStreamQueue->get();
        if (!pRes) {
            Log::instance()->avstream().warning(Poco::format("node %s [%s] get frame returns null frame.",
                getNode()->getName(), getName()));
        }
        else {
            Log::instance()->avstream().trace(Poco::format("node %s [%s] get frame %s, queue size: %s",
                getNode()->getName(), getName(), pRes->getName(),
                Poco::NumberFormatter::format(_pStreamQueue->count())));
        }
        return pRes;
    }
}


void
Stream::putFrame(Frame* pFrame)
{
    if (!pFrame) { 
        Log::instance()->avstream().warning(Poco::format("node %s [%s] put null frame, discarding frame",
            getNode()->getName(), getName()));
    }
    else if (!_pStreamQueue) {
        Log::instance()->avstream().warning(Poco::format("node %s [%s] put frame %s no queue attached, discarding frame",
            getNode()->getName(), getName(), pFrame->getName()));
    }
    else {
        pFrame->_pStream = this;
        bool success = _pStreamQueue->put(pFrame);
        if (!success) {
            Log::instance()->avstream().warning(Poco::format("node %s [%s] put frame %s failed, queue blocked.",
                getNode()->getName(), getName(), pFrame->getName()));
        }
        else {
            Log::instance()->avstream().trace(Poco::format("node %s [%s] put frame %s, queue size: %s",
                getNode()->getName(), getName(), pFrame->getName(),
                Poco::NumberFormatter::format(_pStreamQueue->count())));
        }
    }
}


StreamInfo*
Stream::getInfo()
{
    return _pStreamInfo;
}


StreamQueue*
Stream::getQueue()
{
    return _pStreamQueue;
}


void
Stream::setInfo(StreamInfo* pInfo)
{
    _pStreamInfo = pInfo;
}


void
Stream::setQueue(StreamQueue* pQueue)
{
    _pStreamQueue = pQueue;
}


Node*
Stream::getNode()
{
    return _pNode;
}


std::string
Stream::getName()
{
    if (_pStreamInfo) {
        return _pStreamInfo->_streamName;
    }
    else {
        return "<unknown stream name>";
    }
}


void
Stream::setName(const std::string& name)
{
    if (_pStreamInfo) {
        _pStreamInfo->_streamName = name;
    }
    else {
        Log::instance()->avstream().warning(Poco::format("could not set stream name to: %s",
            name));
    }
}


Frame*
Stream::allocateVideoFrame(PixelFormat targetFormat)
{
    // Determine required buffer size and allocate buffer
    Log::instance()->avstream().trace("ffmpeg::avpicture_get_size() ...");
    int numBytes = avpicture_get_size(targetFormat, getInfo()->width(), getInfo()->height());
    Log::instance()->avstream().trace("ffmpeg::av_malloc() ...");
    uint8_t* buffer = (uint8_t *)av_malloc(numBytes);
    Log::instance()->avstream().trace("ffmpeg::avcodec_alloc_frame() ...");
    AVFrame* pRes = avcodec_alloc_frame();
    Log::instance()->avstream().trace("ffmpeg::avpicture_fill() ...");
    avpicture_fill((AVPicture *)pRes, buffer, targetFormat, getInfo()->width(), getInfo()->height());
    
    return new Frame(getInfo()->newFrameNumber(), this, pRes);
}


Frame*
Stream::decodeFrame(Frame* pFrame)
{
    if (!pFrame || !pFrame->data()) {
        Log::instance()->avstream().warning(Poco::format("input frame broken while decoding stream %s, discarding frame.", getName()));
        return 0;
    }
    if (!_pStreamInfo || !_pStreamInfo->_pAvStream || !_pStreamInfo->_pAvStream->codec) {
        Log::instance()->avstream().warning(Poco::format("missing stream info while decoding stream %s, discarding frame.", getName()));
        return 0;
    }
    if (_pStreamInfo->isAudio()) {
        return decodeAudioFrame(pFrame);
    }
    else if (_pStreamInfo->isVideo()) {
        return decodeVideoFrame(pFrame);
    }
    return 0;
}


Frame*
Stream::decodeAudioFrame(Frame* pFrame)
{
    const int outBufferSize = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
    Frame* pOutFrame = new Frame(pFrame->getNumber(), this, outBufferSize);
    pOutFrame->_data[outBufferSize-1] = 0;
    
    uint8_t* inBuffer = (uint8_t*)pFrame->data();
    int inBufferSize = pFrame->size();
    // TODO: does this while loop work for uncompressed packets ?
    // (it's only needed for e.g. .wav, where on packet contains multiple uncompressed frames
    while(inBufferSize > 0) {
        Log::instance()->avstream().trace("ffmpeg::avcodec_decode_audio2() ...");
        int bytesConsumed = avcodec_decode_audio2(_pStreamInfo->_pAvStream->codec,
            (int16_t*)pOutFrame->_data, &pOutFrame->_size, 
            inBuffer, inBufferSize);
        
        Log::instance()->avstream().debug(Poco::format("ffmpeg::avcodec_decode_audio2() frame size: %s, bytes consumed: %s, decoded size: %s",
            Poco::NumberFormatter::format(inBufferSize),
            Poco::NumberFormatter::format(bytesConsumed),
            Poco::NumberFormatter::format(pOutFrame->_size)
            ));
        
        if (bytesConsumed < 0 || pOutFrame->_size == 0) {
            Log::instance()->avstream().warning(Poco::format("decoding audio frame in stream %s failed, discarding frame.", getName()));
            return 0;
        }
        inBuffer += bytesConsumed;
        inBufferSize -= bytesConsumed;
    }
    return pOutFrame;
}


Frame*
Stream::decodeVideoFrame(Frame* pFrame)
{
    AVFrame outPic;
    int decodeSuccess;
    Log::instance()->avstream().trace("ffmpeg::avcodec_decode_video() ...");
    int bytesConsumed = avcodec_decode_video(_pStreamInfo->_pAvCodecContext,
                                             &outPic, &decodeSuccess,
                                             (uint8_t*)pFrame->data(), pFrame->paddedSize());
    std::string success(decodeSuccess ? "success" : "failed");
    Log::instance()->avstream().debug(Poco::format("ffmpeg::avcodec_decode_video() %s, bytes consumed: %s, linesize[0..2]: %s, %s, %s",
        success,
        Poco::NumberFormatter::format(bytesConsumed),
        Poco::NumberFormatter::format(outPic.linesize[0]),
        Poco::NumberFormatter::format(outPic.linesize[1]),
        Poco::NumberFormatter::format(outPic.linesize[2])));
    
    if (decodeSuccess <= 0 || bytesConsumed <= 0) {
        Log::instance()->avstream().warning(Poco::format("decoding video frame in stream %s failed, discarding frame.", getName()));
        return 0;
    }
    
    Frame* pOutFrame = new Frame(pFrame->getNumber(), this, &outPic);
    return pOutFrame;
}


Node::Node(const std::string& name) :
_name(name),
_quit(false)
{
    _thread.setName(name);
}


std::string
Node::getName()
{
    return _name;
}


void
Node::setName(const std::string& name)
{
    _name = name;
    _thread.setName(name);
}


// bool
// Node::prepareStart()
// {
//     return true;
// }


void
Node::start()
{
    Log::instance()->avstream().debug(Poco::format("starting node %s ...", getName()));

    // first start all nodes downstream, so they can begin sucking frames ...
    int outStreamNumber = 0;
    for (std::vector<Stream*>::iterator it = _outStreams.begin(); it != _outStreams.end(); ++it, ++outStreamNumber) {
        if (!(*it)) {
            Log::instance()->avstream().debug(Poco::format("node %s [%s] could not start downstream node, no stream attached",
                getName(), Poco::NumberFormatter::format(outStreamNumber)));
            continue;
        }
        Node* downstreamNode = getDownstreamNode(outStreamNumber);
        if (!downstreamNode) {
            Log::instance()->avstream().debug(Poco::format("node %s [%s] could not start downstream node, no node attached",
                getName(), Poco::NumberFormatter::format(outStreamNumber)));
            continue;
        }
        downstreamNode->start();
    }
    // then start this node. If this node would be started before the downstream node,
    // all frames processed in this node would be discarded before the downstream nodes could process them.
    if (!_thread.isRunning()) {
        _thread.start(*this);
    }
}


void
Node::stop()
{
    Log::instance()->avstream().debug(Poco::format("stopping node %s ...", getName()));
    // first stop this node by setting the _quit flag ...
    _lock.lock();
    _quit = true;
    _lock.unlock();
    // then stop all nodes downstream
    int outStreamNumber = 0;
    for (std::vector<Stream*>::iterator it = _outStreams.begin(); it != _outStreams.end(); ++it, ++outStreamNumber) {
        if (!(*it)) {
            Log::instance()->avstream().debug(Poco::format("node %s [%s] could not stop downstream node, no stream attached",
                getName(), Poco::NumberFormatter::format(outStreamNumber)));
            continue;
        }
        Node* downstreamNode = getDownstreamNode(outStreamNumber);
        if (!downstreamNode) {
            Log::instance()->avstream().debug(Poco::format("node %s [%s] could not stop downstream node, no node attached",
                getName(), Poco::NumberFormatter::format(outStreamNumber)));
            continue;
        }
        downstreamNode->stop();
    }
    
    // TODO: join all downstream threads after setting _quit in all nodes. Otherwise they would be shut down brutally.
}


void
Node::attach(Node* node, int outStreamNumber, int inStreamNumber)
{
    if (!node) {
        Log::instance()->avstream().warning(Poco::format("node %s: could not attach null node", getName()));
        return;
    }
    Poco::ScopedLock<Poco::FastMutex>lock(_lock);
    
    // entwine StreamInfo and StreamQueue of inStream and outStream
    // TODO: improve warning messages
    if (!_outStreams[outStreamNumber]) {
        Log::instance()->avstream().warning(Poco::format("node %s: could not attach ..., output stream invalid",
            getName()));
        return;
    }
    if (!_outStreams[outStreamNumber]->getInfo()) {
        Log::instance()->avstream().warning(Poco::format("node %s: could not attach ..., output stream has no info",
            getName()));
        return;
    }
    if (!node->_inStreams[inStreamNumber]) {
        Log::instance()->avstream().warning(Poco::format("node %s: could not attach ..., downstream node input stream invalid",
            getName()));
        return;
    }
    if (!node->_inStreams[inStreamNumber]->getQueue()) {
        Log::instance()->avstream().warning(Poco::format("node %s: could not attach ..., downstream node input stream has no queue",
            getName()));
        return;
    }
    
    _outStreams[outStreamNumber]->setQueue(node->_inStreams[inStreamNumber]->getQueue());
    node->_inStreams[inStreamNumber]->setInfo(_outStreams[outStreamNumber]->getInfo());
    
    Log::instance()->avstream().debug(Poco::format("node %s [%s] attached node %s [%s]",
        getName(),
        Poco::NumberFormatter::format(outStreamNumber),
        node->getName(),
        Poco::NumberFormatter::format(inStreamNumber)));
    
    Log::instance()->avstream().debug(Poco::format("calling init() of %s", node->getName()));
    bool initSuccess = node->init();
    if (initSuccess) {
        Log::instance()->avstream().debug(Poco::format("%s init success.", node->getName()));
    }
    else {
        Log::instance()->avstream().warning(Poco::format("%s init failed, start of node canceled.", node->getName()));
        _quit = true;
    }
}


void
Node::detach(int outStreamNumber)
{
    if (outStreamNumber >= _outStreams.size()) {
        Log::instance()->avstream().error(Poco::format("node %s [%s] could not detach node, stream number to high",
            getName(), Poco::NumberFormatter::format(outStreamNumber)));
        return;
    }
    Log::instance()->avstream().debug(Poco::format("node %s [%s] detached node %s",
        _name,
        Poco::NumberFormatter::format(outStreamNumber),
        _outStreams[outStreamNumber]->getQueue()->getNode()->getName()
        ));
    
    Poco::ScopedLock<Poco::FastMutex>lock(_lock);
    // lazy strategie: only set queue of outStream to 0, so no frames are put into the queue anymore
    // the attached node empties the queue with a valid stream context (StreamInfo) and blocks
    // without being connected to anything
    _outStreams[outStreamNumber]->setQueue(0);
}


Node*
Node::getDownstreamNode(int outStreamNumber)
{
    if (outStreamNumber >= _outStreams.size()) {
        Log::instance()->avstream().error(Poco::format("node %s [%s] could not get downstream node, stream number to high",
            getName(), Poco::NumberFormatter::format(outStreamNumber)));
        return 0;
    }
    Stream* outStream = _outStreams[outStreamNumber];
    if (!outStream || !outStream->getQueue()) {
        Log::instance()->avstream().warning(Poco::format("node %s [%s] could not get downstream node, no node attached to output stream.",         getName(), Poco::NumberFormatter::format(outStreamNumber)));
        return 0;
    }
    return outStream->getQueue()->getNode();
}


// void
// Node::setStop(bool stop)
// {
//     _lock.lock();
//     _quit = stop;
//     _lock.unlock();
// }
// 
// 
bool
Node::doStop()
{
//     Poco::ScopedLock<Poco::FastMutex> lock(_lock);
    return _quit;
}


Stream*
Node::getInStream(int inStreamNumber)
{
    return _inStreams[inStreamNumber];
}


// Frame::Frame(const Frame& frame) :
// _size(frame._size),
// _pStream(frame._pStream)
// {
//     if (frame._pAvFrame) {
//         uint8_t* buffer = (uint8_t*)av_malloc(_size);
//         _pAvFrame = avcodec_alloc_frame();
//         avpicture_fill((AVPicture*)_pAvFrame, buffer,
//                        _pStream->_pStreamInfo->pixelFormat(),
//                        _pStream->_pStreamInfo->width(),
//                        _pStream->_pStreamInfo->height());
//         
//         for (int plane = 0; plane < 4; plane++) {
//             memcpy(_pAvFrame->data[plane],
//                    frame._pAvFrame->data[plane],
//                    _pStream->_pStreamInfo->height() * frame._pAvFrame->linesize[plane]);
//         }
//         _data = (char*)_pAvFrame->data[0];
//     }
//     else if (frame._pAvPacket) {
//         // TODO: copy ctor for AVPacket
//     }
//     else {
//         // TODO: only _data needs to be copied
//     }
// }


Frame::Frame(int64_t number, Stream* pStream) :
_number(number),
_pts(AV_NOPTS_VALUE),
_data(0),
_size(0),
_pAvPacket(0),
_pAvFrame(0),
_pStream(pStream)
{
}


Frame::Frame(int64_t number, Stream* pStream, int dataSize) :
_number(number),
_pts(AV_NOPTS_VALUE),
_data(0),
_size(0),
_pAvPacket(0),
_pAvFrame(0),
_pStream(pStream)
{
    _data = new char[dataSize];
    _size = dataSize;
}


Frame::Frame(int64_t number, Stream* pStream, char* data, int dataSize) :
_number(number),
_pts(AV_NOPTS_VALUE),
_data(0),
_size(0),
_pAvPacket(0),
_pAvFrame(0),
_pStream(pStream)
{
    _data = new char[dataSize];
    _size = dataSize;
    memcpy(_data, data, dataSize);
}


Frame::Frame(int64_t number, Stream* pStream, AVPacket* pAvPacket) :
_number(number),
_pts(AV_NOPTS_VALUE),
_data(0),
_size(0),
_pAvPacket(0),
_pAvFrame(0),
_pStream(pStream)
{
    // NOTE: seems like we need to copy and free the demuxed packets.
    // copying the reference to the data results in garbled decoded frames
    // NOTE: make all packets a bit larger, not only the video packets
    // this doesn't hurt, as they will be deleted shortly after
    _pAvPacket = copyPacket(pAvPacket, FF_INPUT_BUFFER_PADDING_SIZE);
    _data = (char*)_pAvPacket->data;
    _size = _pAvPacket->size;
    _paddedSize = _pAvPacket->size + FF_INPUT_BUFFER_PADDING_SIZE;
    Log::instance()->avstream().trace("ffmpeg::av_free_packet() ...");
    av_free_packet(pAvPacket);
}


// Frame::Frame(Stream* pStream, AVPacket* pAvPacket) :
// _data(0),
// _size(0),
// _pAvPacket(0),
// _pAvFrame(0),
// _pStream(pStream)
// {
//     // NOTE: without copying the payload of the packets, the resulting decoded frames are somewhat garbled
//     // ... still don't completely understand ffmpeg's memory management ...
//     _pAvPacket = new AVPacket;
//     *_pAvPacket = *pAvPacket;
//     _data = (char*)_pAvPacket->data;
//     _size = _pAvPacket->size;
//     _paddedSize = _pAvPacket->size;
// }


Frame::Frame(int64_t number, Stream* pStream, AVFrame* pAvFrame) :
_number(number),
_pts(AV_NOPTS_VALUE),
_data(0),
_size(0),
_pAvPacket(0),
_pAvFrame(0),
_pStream(pStream)
{
    // NOTE: seems like ffmpeg holds one GOP in memory.
    // the pointers to the picture data are copied in struc AVFrame into AvStream::Frame
    
    // NOTE: if we don't copy the frame data after decoding a frame,
    // it is deleted / overriden by libavcodec (for example: only the last three frames of 
    // the stream are stored)
    
    // NOTE: why these two lines are needed, dunno exactly ...
    _pAvFrame = new AVFrame;
    *_pAvFrame = *pAvFrame;
//     _pAvFrame = copyFrame(pAvFrame);
    
    // NOTE: this only points to the correct data in planeless picture formats
    _size = _pStream->_pStreamInfo->pictureSize();
    _data = (char*)_pAvFrame->data[0];
    
    // FIXME: need to copy the data, too.
    
    // FIXME: then free pAvFrame
}


Frame::~Frame()
{
    if (_data) {
        delete _data;
        _data = 0;
    }
    if (_pAvPacket) {
        delete _pAvPacket;
        _pAvPacket = 0;
    }
    if (_pAvFrame) {
        delete _pAvFrame;
        _pAvFrame = 0;
    }
}


AVPacket*
Frame::copyPacket(AVPacket* pAvPacket, int padSize)
{
    // allocate AVPacket struc
    AVPacket* pRes = new AVPacket;
    // copy fields of AVPacket struc
    *pRes = *pAvPacket;
    // allocate payload
    pRes->data = new uint8_t[pAvPacket->size + padSize];
    // copy payload
    memcpy(pRes->data, pAvPacket->data, pAvPacket->size);
    
    return pRes;
}


AVFrame*
Frame::allocateFrame(PixelFormat format)
{
    AVFrame* pRes = avcodec_alloc_frame();
    Log::instance()->avstream().trace("ffmpeg::avpicture_fill() ...");
    
    // use int avpicture_alloc(AVPicture *picture, int pix_fmt, int width, int height) instead of avpicture_fill?
    uint8_t* buffer = (uint8_t *)av_malloc(_pStream->_pStreamInfo->pictureSize());
    Log::instance()->avstream().trace("ffmpeg::avcodec_alloc_frame() ...");
    avpicture_fill((AVPicture *)pRes,
                   buffer,
                   format,
                   _pStream->_pStreamInfo->width(),
                   _pStream->_pStreamInfo->height());
    
    return pRes;
}


AVFrame*
Frame::copyFrame(AVFrame* pAvFrame)
{
    // allocate AVFrame in the format of this stream
    AVFrame* pRes = allocateFrame(_pStream->getInfo()->pixelFormat());
    
    // copy payload
//     for (int planes = 0; planes < 4; planes++) {
//         if (pRes->linesize[planes] != pAvFrame->linesize[planes]) {
//             Log::instance()->avstream().error("copy AVFrame failed, format missmatch linesizes of source and target don't fit");
//         }
//         memcpy(pRes->data[planes], pAvFrame->data[planes], pAvFrame->linesize[planes]);
//     }
    
    // copy payload
    av_picture_copy((AVPicture*)pRes, (AVPicture*) pAvFrame,
                    _pStream->getInfo()->pixelFormat(),
                    _pStream->getInfo()->width(),
                    _pStream->getInfo()->height());
    
    return pRes;
}


char*
Frame::data()
{
    return _data;
}


int
Frame::size()
{
    return _size;
}


int
Frame::paddedSize()
{
    return _paddedSize;
}


char*
Frame::planeData(int plane)
{
    return (char*)_pAvFrame->data[plane];
}


int
Frame::planeSize(int plane)
{
    return _pAvFrame->linesize[plane];
}


void
Frame::printInfo()
{
    Log::instance()->avstream().debug(Poco::format("frame linesize[0..2]: %s, %s, %s",
        Poco::NumberFormatter::format(_pAvFrame->linesize[0]),
        Poco::NumberFormatter::format(_pAvFrame->linesize[1]),
        Poco::NumberFormatter::format(_pAvFrame->linesize[2]))
        );
    
    Log::instance()->avstream().debug(Poco::format("frame data[0..2]: %s, %s, %s",
        Poco::NumberFormatter::format((unsigned int)_pAvFrame->data[0]),
        Poco::NumberFormatter::format((unsigned int)_pAvFrame->data[1]),
        Poco::NumberFormatter::format((unsigned int)_pAvFrame->data[2]))
        );
}


std::string
Frame::getName()
{
    return "["
        + _pStream->getName() + "] "
        + "#" + Poco::NumberFormatter::format(_number) + ", pts: "
        + (_pts == AV_NOPTS_VALUE ? "AV_NOPTS_VALUE" : Poco::NumberFormatter::format(_pts));
//         + ", duration: " + Poco::NumberFormatter::format(_duration);
}


int64_t
Frame::getNumber()
{
    return _number;
}


int64_t
Frame::getPts()
{
    return _pts;
}


void
Frame::setPts(int64_t pts)
{
    _pts = pts;
}


Stream*
Frame::getStream()
{
    return _pStream;
}


Frame*
Frame::decode()
{
    Frame* pRes = _pStream->decodeFrame(this);
    if (pRes) {
        pRes->setPts(getPts());
    }
    return pRes;
}


Frame*
Frame::convert(PixelFormat targetFormat, int targetWidth, int targetHeight)
{
    _pStream->getInfo()->printInfo();
    int width = _pStream->getInfo()->width();
    int height = _pStream->getInfo()->height();
    if (targetWidth == -1) {
        targetWidth = width;
    }
    if (targetHeight == -1) {
        targetHeight = height;
    }
    PixelFormat inPixFormat = _pStream->getInfo()->pixelFormat();
    
    Log::instance()->avstream().debug(Poco::format("source pixelFormat: %s, target pixelFormat: %s",
        Poco::NumberFormatter::format(inPixFormat),
        Poco::NumberFormatter::format(targetFormat)));
    
    int scaleAlgo = SWS_BICUBIC;
    struct SwsContext *pImgConvertContext = 0;
    
    Log::instance()->avstream().trace("ffmpeg::sws_getCachedContext() ...");
    pImgConvertContext = sws_getCachedContext(pImgConvertContext,
                                              width, height, inPixFormat,
                                              width, height, targetFormat,
                                              scaleAlgo, NULL, NULL, NULL);
    
    if (pImgConvertContext == 0) {
        Log::instance()->avstream().warning("cannot initialize image conversion context");
        return 0;
    }
    else {
        Log::instance()->avstream().debug("success: image conversion context set up.");
    }
    
    // FIXME: _pStream->pCodecContext is wrong with pOutFrame, because e.g. pix_fmt changed
    Frame* pRes = _pStream->allocateVideoFrame(targetFormat);
    
    printInfo();
    Log::instance()->avstream().trace("ffmpeg::sws_scale() ...");
    sws_scale(pImgConvertContext,
              _pAvFrame->data, _pAvFrame->linesize,
              0, height,
              pRes->_pAvFrame->data, pRes->_pAvFrame->linesize);
    
    pRes->printInfo();
    
    return pRes;
}


void
Frame::writePpm(const std::string& fileName)
{
    Log::instance()->avstream().debug(Poco::format("write video frame to PPM file name: %s", fileName));
    
    Frame* pRes = convert(PIX_FMT_RGB24);
    
    std::ofstream ppmFile(fileName.c_str());
    // write PPM header
    ppmFile << "P6\n" << _pStream->_pStreamInfo->width() << " " << _pStream->_pStreamInfo->height() << "\n" << 255 << "\n";
    // write RGB pixel data
    ppmFile.write((const char*)pRes->_pAvFrame->data[0], _pStream->_pStreamInfo->width() * _pStream->_pStreamInfo->height() * 3);
}


void
Frame::write(Overlay* pOverlay)
{
    Log::instance()->avstream().debug("convert video frame to YUV ... ");
    
    _pStream->getInfo()->printInfo();
    int streamWidth = _pStream->getInfo()->width();
    int streamHeight = _pStream->getInfo()->height();
    PixelFormat streamPixelFormat = _pStream->getInfo()->pixelFormat();
    
    int targetWidth = pOverlay->getWidth();
    int targetHeight = pOverlay->getHeight();
    PixelFormat targetPixelFormat = pOverlay->getFormat();
    
    Log::instance()->avstream().debug(Poco::format("stream pixelFormat: %s, target pixelFormat: %s",
        Poco::NumberFormatter::format(streamPixelFormat),
        Poco::NumberFormatter::format(targetPixelFormat)));
    
    int scaleAlgo = SWS_BICUBIC;
    struct SwsContext *pImgConvertContext = 0;
    
    Log::instance()->avstream().trace("ffmpeg::sws_getCachedContext() ...");
    pImgConvertContext = sws_getCachedContext(pImgConvertContext,
                                              streamWidth, streamHeight, streamPixelFormat,
                                              targetWidth, targetHeight, targetPixelFormat,
                                              scaleAlgo, NULL, NULL, NULL);
    
    if (pImgConvertContext == 0) {
        Log::instance()->avstream().warning("cannot initialize image conversion context");
        return;
    }
    else {
        Log::instance()->avstream().debug("success: image conversion context set up.");
    }
    
    printInfo();
    Log::instance()->avstream().trace("ffmpeg::sws_scale() ...");
    int outSlizeHeight = sws_scale(pImgConvertContext,
                                   _pAvFrame->data, _pAvFrame->linesize,
                                   0, streamHeight,
                                   pOverlay->_data, pOverlay->_pitch);
    Log::instance()->avstream().debug("ffmpeg::sws_scale() frame written to overlay.");
}


Demuxer::Demuxer() :
Node("demuxer"),
_pMeta(0),
_firstAudioStream(-1),
_firstVideoStream(-1)
{
    // Initialize libavformat and register all the muxers, demuxers and protocols
    Log::instance()->avstream().trace("ffmpeg::av_register_all() ...");
    av_register_all();
}


void
Demuxer::set(const std::string& uri)
{
    Tagger tagger;
    _pMeta = tagger.tag(uri);
    if (!_pMeta) {
        Log::instance()->avstream().warning(Poco::format("demuxer could not tag input stream with uri %s, giving up.", uri));
        _quit = true;
    }
    else {
        _pMeta->print();
        if (!init()) {
            Log::instance()->avstream().warning("demuxer init failed, giving up.");
            _quit = true;
        }
    }
}


void
Demuxer::set(std::istream& istr)
{
    Tagger tagger;
    _pMeta = tagger.tag(istr);
    if (!_pMeta) {
        Log::instance()->avstream().warning("demuxer could not tag input stream, giving up.");
        _quit = true;
    }
    else {
        _pMeta->print();
        if (!init()) {
            Log::instance()->avstream().warning("demuxer init failed, giving up.");
            _quit = true;
        }
    }
}


bool
Demuxer::init()
{
    AVFormatContext* pFormatContext = _pMeta->_pFormatContext;
    int audioStreamCount = 0;
    int videoStreamCount = 0;
    for(int streamNr = 0; streamNr < pFormatContext->nb_streams; streamNr++) {
        //////////// allocate streams ////////////
        Stream* pStream = new Stream(this);
        _outStreams.push_back(pStream);
        
        StreamInfo* pStreamInfo = new StreamInfo;
        pStreamInfo->_pAvStream = pFormatContext->streams[streamNr];
        pStream->setInfo(pStreamInfo);
        pStream->setQueue(0);
        
        //////////// find first audio and video stream ////////////
        if (pStream->getInfo()->isAudio()) {
            Log::instance()->avstream().information(Poco::format("found audio stream #%s", Poco::NumberFormatter::format(streamNr)));
            pStream->setName("audio" + Poco::NumberFormatter::format(audioStreamCount++));
            if (_firstAudioStream < 0) {
                _firstAudioStream = streamNr;
            }
        }
        else if (pStream->getInfo()->isVideo()) {
            Log::instance()->avstream().information(Poco::format("found video stream #%s", Poco::NumberFormatter::format(streamNr)));
            pStream->setName("video" + Poco::NumberFormatter::format(videoStreamCount++));
            if (_firstVideoStream < 0) {
                _firstVideoStream = streamNr;
            }
        }
        else {
            Log::instance()->avstream().information(Poco::format("found unknown stream #%s", Poco::NumberFormatter::format(streamNr)));
        }
    }
    return true;
}


void
Demuxer::reset()
{
    if (_pMeta) {
        delete _pMeta;
    }
    _pMeta = 0;
    
    for(std::vector<Stream*>::iterator it = _outStreams.begin(); it != _outStreams.end(); ++it) {
        delete *it;
    }
    _outStreams.clear();
}


void
Demuxer::run()
{
    // TODO: save copying of AVPacket packet into Frame and read it directly into frame with av_read_frame()
    // TODO: introduce Frame::readFrame(Meta* pMeta) and while(pFrame* = readFrame(_pMeta))
    int64_t frameNumber = 0;
    
    std::vector<int64_t> lastPtsVec(_outStreams.size(), AV_NOPTS_VALUE);
    std::vector<int> lastDurationVec(_outStreams.size(), 0);
    
    AVPacket packet;
    while (!_quit) {
        Log::instance()->avstream().trace("ffmpeg::av_init_packet() ...");
        av_init_packet(&packet);
        Log::instance()->avstream().trace("ffmpeg::av_read_frame() ...");
        int ret = av_read_frame(_pMeta->_pFormatContext, &packet);
        if (ret < 0) {
            Log::instance()->avstream().debug(Poco::format("ffmpeg::av_read_frame() returns: %s (%s)",
                Poco::NumberFormatter::format(ret),
                (ret == AVERROR_EOF ? "eof reached." : ".")));
            break;
        }
        frameNumber++;
        Log::instance()->avstream().debug(Poco::format("ffmpeg::av_read_frame() returns packet #%s, type: %s, pts: %s, size: %s, duration: %s",
            Poco::NumberFormatter::format0(frameNumber, 8),
            _outStreams[packet.stream_index]->getName(),
            Poco::NumberFormatter::format(packet.pts),
            Poco::NumberFormatter::format(packet.size),
            Poco::NumberFormatter::format(packet.duration)));
        
        if (!_outStreams[packet.stream_index]->getQueue()) {
            Log::instance()->avstream().warning(Poco::format("%s stream %s not connected, discarding packet",
                getName(), Poco::NumberFormatter::format(packet.stream_index)));
            av_free_packet(&packet);
            continue;
        }
        // pass a reference to the stream to where this frame belongs (for further use of the codec context when decoding)
        // create a new frame out of the AVPacket that we extracted from the stream
        // create means: make a copy of the data buffer and delete the packet
        Frame* pFrame = new Frame(frameNumber, _outStreams[packet.stream_index], &packet);
        pFrame->setPts(packet.pts);
        
        // try to correct wrong packet.pts
        // this relies on a correct packet.duration
        // (pts or duration, one of them must be right, otherwise we are lost, especially with variable frame rate)
        int64_t currentPts = correctPts(packet.pts, lastPtsVec[packet.stream_index], lastDurationVec[packet.stream_index]);
        if (currentPts != packet.pts) {
            Log::instance()->avstream().warning(Poco::format("%s frame %s, correct pts %s -> %s",
                getName(),
                pFrame->getName(),
                Poco::NumberFormatter::format(packet.pts),
                Poco::NumberFormatter::format(currentPts)));
        }
        pFrame->setPts(currentPts);
        lastPtsVec[packet.stream_index] = currentPts;
        lastDurationVec[packet.stream_index] = packet.duration;
        
        _outStreams[packet.stream_index]->putFrame(pFrame);
    }
    // if no frames could be read from input stream, stop all nodes
    stop();
    // TODO: clock should be stopped, too (if connected to a sink node)
}


int
Demuxer::firstAudioStream()
{
    return _firstAudioStream;
}


int
Demuxer::firstVideoStream()
{
    return _firstVideoStream;
}


int64_t
Demuxer::correctPts(int64_t pts, int64_t lastPts, int lastDuration)
{
    // OK, this is the place for black magic. 
    // Everything would be fine, if pts and duration
    // would be in perfect harmony and pts would always be monotone ...
    // but this isn't the case, and we rely on two assumptions here:
    // 1. all frames are in correct order
    // 2. duration of frames is correct
    // 
    //     In general, there seem to be "seven symptoms of sick streams"
    //     
    //     pts related (all corrected in synchronizer):
    //     1. pts number overflow
    //     -> set flag Frame::numberOverflow = true
    //     2. pts cicle (pts systematic not monotone)
    //     -> reorder pts
    //     3. pts sprite (pts local not monotone)
    //     -> "tear" sprite pts linear
    //     4. packet shift
    //     -> discard packets until max_of_streams(pts first packet of stream)
    //     5. packet loss (pts not linear)
    //     -> inject blank frame (set Frame::_display = false)
    //     
    //     not pts related:
    //     6. decoding error
    //     -> set flag Frame::_display = false
    //     7. clock drift
    //     -> correct drift or one clock source (watch for cumulating drift when using timers)
    //     
    
    int64_t correctedPts;
    
    if (lastPts != AV_NOPTS_VALUE && pts != AV_NOPTS_VALUE) {
        if (lastDuration > 0 && pts != lastPts + lastDuration) {
            correctedPts = lastPts + lastDuration;
        }
        else {
            correctedPts = pts;
        }
        
    }
    else if (lastPts == AV_NOPTS_VALUE && pts != AV_NOPTS_VALUE) {
        correctedPts = pts;
    }
    else if (pts == AV_NOPTS_VALUE && lastPts != AV_NOPTS_VALUE) {
        if (lastDuration > 0) {
            correctedPts = lastPts + lastDuration;
        }
        else {
            correctedPts = pts;
        }
    }
    else { // lastPts && packet.pts == AV_NOPTS_VALUE
        correctedPts = pts;
    }
    
    return correctedPts;
}



Decoder::Decoder() :
Node("decoder")
{
    // decoder has one input stream
    _inStreams.push_back(new Stream(this));
    _inStreams[0]->setInfo(0);
    _inStreams[0]->setQueue(new StreamQueue(this));
    
    // and one output stream
    _outStreams.push_back(new Stream(this));
    _outStreams[0]->setInfo(0);
    _outStreams[0]->setQueue(0);
}


bool
Decoder::init()
{
    // init() is called on attach() so _inStreams[0]->_pStreamInfo should be available
    if (!_inStreams[0]->getInfo()) {
        Log::instance()->avstream().warning(Poco::format("%s init failed, input stream info not allocated", getName()));
        return false;
    }
    if (!_inStreams[0]->getInfo()->findCodec()) {
        Log::instance()->avstream().warning(Poco::format("%s init failed, could not find codec", getName()));
        return false;
    }
    if (_inStreams[0]->getInfo()->isAudio()) {
        setName("audio decoder");
    }
    else if (_inStreams[0]->getInfo()->isVideo()) {
        setName("video decoder");
    }
    // output stream has same parameters after decoding as before decoding (NOTE: check, if that's right)
    _outStreams[0]->setInfo(_inStreams[0]->getInfo());
    return true;
}


void
Decoder::run()
{
    Frame* pFrame;
    if (!_inStreams[0]) {
        Log::instance()->avstream().warning(Poco::format("%s no in stream attached, stopping.", getName()));
        return;
    }
    while (!_quit && (pFrame = _inStreams[0]->getFrame()))
    {
        if (!_outStreams[0]) {
            Log::instance()->avstream().warning(Poco::format("%s no out stream attached, discarding packet.", getName()));
            continue;
        }
        Log::instance()->avstream().debug(Poco::format("%s decode frame %s",
            getName(), pFrame->getName()));
        
        Frame* pFrameDecoded = pFrame->decode();
        _outStreams[0]->putFrame(pFrameDecoded);
    }
    Log::instance()->avstream().warning(Poco::format("%s finished.", getName()));
}


Meta::Meta() :
_pFormatContext(0),
_pIoContext(0),
_pInputFormat(0)
{
}


Meta::~Meta()
{
    if (_pFormatContext) {
        Log::instance()->avstream().trace("ffmpeg::av_close_input_stream() ...");
        av_close_input_stream(_pFormatContext);
    }
}


void
Meta::print()
{
    dump_format(_pFormatContext, 0, _pFormatContext->filename, 0);
    
    AVMetadataTag* tag = 0;
    Log::instance()->avstream().trace("ffmpeg::av_metadata_get() ...");
    while ((tag = av_metadata_get(_pFormatContext->metadata, "", tag, AV_METADATA_IGNORE_SUFFIX))) {
        std::clog << tag->key << ", " << tag->value << std::endl;
    }
}

// FIXME: for correct buffer sizes see libavformat/utils.c: av_open_input_file()
Tagger::Tagger() :
_tagBufferSize(2048),
// _IoBufferSize(32768),
// _IoBufferSize(2048),
// _IoBufferSize(1024),
_IoBufferSize(8192),
_pIoBuffer(new unsigned char[_IoBufferSize])
{

}


Tagger::~Tagger()
{
    if (_pIoBuffer) {
        delete _pIoBuffer;
    }
}


AVInputFormat*
Tagger::probeInputFormat(std::istream& istr)
{
    AVInputFormat* pInputFormat = 0;
    
    AVProbeData probeData;
    probeData.filename = "";
    Log::instance()->avstream().debug("probing stream ...");
    
    probeData.buf_size = _tagBufferSize;
    unsigned char buffer[_tagBufferSize];
    probeData.buf = (unsigned char*)&buffer;
    istr.read((char*)probeData.buf, _tagBufferSize);
    if(istr.bad()) {
        std::cerr << "error reading probe data" << std::endl;
        return 0;
    }
//     istr.seekg(0);
    std::clog << "done." << std::endl;
    
    Log::instance()->avstream().debug("detecting format ...");
    // initialize _pInputFormat
    Log::instance()->avstream().trace("ffmpeg::av_probe_input_format() ...");
    pInputFormat = av_probe_input_format(&probeData, 1 /*int is_opened*/);
    if (pInputFormat) {
        Log::instance()->avstream().information(Poco::format("AV stream format: %s (%s)",
            std::string(pInputFormat->name), std::string(pInputFormat->long_name)));
    }
    else {
        Log::instance()->avstream().error("AV stream format unknown");
        return 0;
    }
    
    // TODO: reset the stream after probing
//     // Seek back to 0
//     // copied from url_fseek
//     long offset1 = 0 - (_pIoContext->pos - (_pIoContext->buf_end - _pIoContext->buffer));
//     if (offset1 >= 0 && offset1 <= (_pIoContext->buf_end - _pIoContext->buffer)) {
//             /* can do the seek inside the buffer */
//         _pIoContext->buf_ptr = _pIoContext->buffer + offset1;
//     } else {
//         if (!d->src->seek(0)) {
//             return false;
//         } else {
//             _pIoContext->pos = 0;
//             _pIoContext->buf_ptr = d->file_buffer;
//             _pIoContext->buf_end = d->file_buffer;
//         }
//     }
    
//     pInputFormat->flags |= AVFMT_NOFILE;
    
    return pInputFormat;
}


static long totalRead = 0;
static long totalReadCount = 0;

static int IORead( void *opaque, uint8_t *buf, int buf_size )
{
    Log::instance()->avstream().trace("IORead()");
    
    URLContext* pUrlContext = (URLContext*)opaque;
    Log::instance()->avstream().trace(Poco::format("IORead() pUrlContext pointer: %s", Poco::NumberFormatter::format(pUrlContext)));
    Log::instance()->avstream().trace(Poco::format("IORead() URLProtocol name: %s", std::string(pUrlContext->prot->name)));
    
    std::istream* pInputStream = (std::istream*)pUrlContext->priv_data;
    if (!pInputStream) {
        Log::instance()->avstream().error("IORead failed, std::istream not set");
        return -1;
    }
    
    int bytes = pInputStream->readsome((char*)buf, buf_size);
    if (!pInputStream->good()) {
        Log::instance()->avstream().error("IORead failed to read from std::istream");
        return -1;
    }
    
    totalRead += bytes;
    totalReadCount++;
    Log::instance()->avstream().trace(Poco::format("IORead() bytes read: %s, total: %s, read ops: %s",
        Poco::NumberFormatter::format(bytes),
        Poco::NumberFormatter::format(totalRead),
        Poco::NumberFormatter::format(totalReadCount)
        ));
    return bytes;
}


static int64_t IOSeek( void *opaque, int64_t offset, int whence )
{
    Log::instance()->avstream().trace(Poco::format("IOSeek() offset: %s", Poco::NumberFormatter::format(offset)));
    
    URLContext* pUrlContext = (URLContext*)opaque;
    Log::instance()->avstream().trace(Poco::format("IOSeek() pUrlContext pointer: %s", Poco::NumberFormatter::format(pUrlContext)));
    Log::instance()->avstream().trace(Poco::format("IOSeek() URLProtocol name: %s", std::string(pUrlContext->prot->name)));
    
    std::istream* pInputStream = (std::istream*)pUrlContext->priv_data;
    if (!pInputStream) {
        Log::instance()->avstream().error("IOSeek failed, std::istream not set");
        return 0;
    }
    
    pInputStream->seekg(offset);
    if (!pInputStream->good()) {
        Log::instance()->avstream().error("IOSeek failed to read from std::istream");
        return 0;
    }
    totalRead = offset;
    return totalRead;
}


ByteIOContext*
Tagger::initIo(std::istream& istr)
{
    static char streamName[] = "std::istream";
    
    URLContext* pUrlContext = new URLContext;
    pUrlContext->is_streamed = 1;
    pUrlContext->priv_data = 0;
    pUrlContext->filename = streamName;
    pUrlContext->prot = new URLProtocol;
    pUrlContext->prot->name = "OMM avio wrapper for std::iostream";
    pUrlContext->prot->next = 0;
    pUrlContext->prot->url_open = 0;
    pUrlContext->prot->url_read = (int (*) (URLContext *, unsigned char *, int))IORead;
    pUrlContext->prot->url_write = 0;
    pUrlContext->prot->url_seek = (int64_t (*) (URLContext *, int64_t, int))IOSeek;
    pUrlContext->prot->url_close = 0;
    pUrlContext->priv_data = &istr;
    
    // TODO: maybe IOSeek = 0 stops libavformat from doing seek operations
    Log::instance()->avstream().trace("ffmpeg::av_alloc_put_byte() ...");
    ByteIOContext* pIoContext = av_alloc_put_byte(_pIoBuffer, _IoBufferSize, 0, pUrlContext, IORead, 0, IOSeek);
    pIoContext->is_streamed = 1;
    pIoContext->max_packet_size = _IoBufferSize;
    
    return pIoContext;
}


Meta*
Tagger::tag(const std::string& uri)
{
    Meta* pMeta = new Meta;
    int error;
    
    Log::instance()->avstream().trace("ffmpeg::av_open_input_file() ...");
    error = av_open_input_file(&pMeta->_pFormatContext, uri.c_str(), 0, 0, 0);
    if (error) {
        Log::instance()->avstream().error("av_open_input_stream() failed");
        return 0;
    }
    
    Log::instance()->avstream().trace("ffmpeg::av_find_stream_info() ...");
    error = av_find_stream_info(pMeta->_pFormatContext);
    if (error) {
        Log::instance()->avstream().error("av_find_stream_info() failed, could not find codec parameters");
        return 0;
    }
    
    return pMeta;
}


Meta*
Tagger::tag(std::istream& istr)
{
    Meta* pMeta = new Meta;
    int error;
    
    pMeta->_pIoContext = initIo(istr);
    pMeta->_pInputFormat = probeInputFormat(istr);
    Log::instance()->avstream().trace("ffmpeg::av_open_input_stream() ...");
    error = av_open_input_stream(&pMeta->_pFormatContext, pMeta->_pIoContext, "std::istream", pMeta->_pInputFormat, 0);
    if (error) {
        Log::instance()->avstream().error("av_open_input_stream() failed");
        return 0;
    }
    
    Log::instance()->avstream().trace("ffmpeg::av_find_stream_info() ...");
    error = av_find_stream_info(pMeta->_pFormatContext);
    if (error) {
        Log::instance()->avstream().error("av_find_stream_info() failed, could not find codec parameters");
        return 0;
    }
    
//     pMeta->_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;
    
//     if (pMeta->_pFormatContext->flags & AVFMT_FLAG_NONBLOCK) {
//         std::clog << "frame reading is set to NONBLOCK" << std::endl;
//     }
//     else {
//         std::clog << "frame reading is set to BLOCK" << std::endl;
//     }
    
    return pMeta;
}


Sink::Sink(const std::string& name) :
Node(name),
_timeQueue(name + " timer", 10, 100, 100),
_firstDecodeSuccess(false)
{
    // sink has one input stream
    _inStreams.push_back(new Omm::AvStream::Stream(this));
    _inStreams[0]->setInfo(0);
    _inStreams[0]->setQueue(new Omm::AvStream::StreamQueue(this));
    
    // and no output stream
}


VideoSink::VideoSink(const std::string& name, int width, int height, PixelFormat pixelFormat, int overlayCount) :
Sink(name),
_overlayCount(overlayCount),
_overlayVector(overlayCount),
_overlayQueue(name + " overlay", overlayCount, 500, 500),
_timerThread(name + " timer thread"),
_timerQuit(false),
_width(width),
_height(height),
_pixelFormat(pixelFormat),
_writeOverlayNumber(0)
{
}


VideoSink*
VideoSink::loadPlugin(const std::string& libraryPath, const std::string& className)
{
    Poco::ClassLoader<VideoSink> sinkPluginLoader;
    if (sinkPluginLoader.isLibraryLoaded(libraryPath)) {
        Log::instance()->avstream().error(Poco::format("library %s already loaded", libraryPath));
        return 0;
    }
    try {
        sinkPluginLoader.loadLibrary(libraryPath);
    }
    catch (Poco::NotFoundException) {
        Log::instance()->avstream().error(Poco::format("could not find  %s sink plugin.", libraryPath));
        return 0;
    }
    catch (Poco::LibraryLoadException) {
        Log::instance()->avstream().error(Poco::format("could not load  %s sink plugin.", libraryPath));
        return 0;
    }
    Log::instance()->avstream().debug(Poco::format("%s plugin successfully loaded.", libraryPath));
    
    Log::instance()->avstream().debug(Poco::format("%s plugin to be allocated ...", className));
    VideoSink* pRes;
    try {
        pRes = sinkPluginLoader.create(className);
    }
    catch (Poco::NotFoundException) {
        Log::instance()->avstream().error(Poco::format("%s could not create instance of plugin.", className));
        return 0;
    }
    Log::instance()->avstream().debug(Poco::format("%s plugin successfully allocated.", className));
    return pRes;
}


bool
VideoSink::checkInStream()
{
    if (!_inStreams[0]->getInfo()->isVideo()) {
        Omm::AvStream::Log::instance()->avstream().warning(Poco::format("%s init failed, input stream is not a video stream", getName()));
        return false;
    }
    
    return true;
}


void
VideoSink::startPresentation()
{
    Log::instance()->avstream().debug(Poco::format("%s starting timer thread ...", getName()));
    
//     beforeTimerStart();
    
    Poco::RunnableAdapter<VideoSink> ra(*this, &VideoSink::timerThread);
    _timerThread.start(ra);
    
//     afterTimerStart();
}


void
VideoSink::stopPresentation()
{
    Log::instance()->avstream().debug(Poco::format("%s stopping timer thread ...", getName()));
    
    // FIXME: this doesn't really work, for example:
    // alsa audio sink write thread waits for frame from queue, then _timerQuit is set to false
    // -> write thread keeps waiting ...
    _timerQuit = true;
}


void
VideoSink::currentTime(int64_t time)
{
    Log::instance()->avstream().trace(Poco::format("%s current time: %s", getName(), Poco::NumberFormatter::format(time)));
    
    _timeQueue.put(time);
}


void
VideoSink::timerThread()
{
    // TODO: what if get() in Sink::timerThread() times out ...
    // what is returned?
    // is onTick() executed?
    while(!_timerQuit) {
        int64_t time = _timeQueue.get();
        Log::instance()->avstream().trace(Poco::format("%s on tick time: %s", getName(), Poco::NumberFormatter::format(time)));
        onTick(time);
    }
    Log::instance()->avstream().debug(Poco::format("%s timer thread finished.", getName()));
}


int
VideoSink::getWidth()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_sinkLock);
    return _width;
}


int
VideoSink::getHeight()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_sinkLock);
    return _height;
}


PixelFormat
VideoSink::getFormat()
{
    Poco::ScopedLock<Poco::FastMutex> lock(_sinkLock);
    return _pixelFormat;
}


void
VideoSink::writeDecodedFrame(Omm::AvStream::Frame* pDecodedFrame)
{
    Overlay* pWriteOverlay = _overlayVector[_writeOverlayNumber];
    pDecodedFrame->write(pWriteOverlay);
    pWriteOverlay->_pFrame = pDecodedFrame;
    
    _overlayQueue.put(pWriteOverlay);
    
    // increment modulo _overlayCount
    _writeOverlayNumber++;
    if (_writeOverlayNumber >= _overlayCount) {
        _writeOverlayNumber = 0;
    }
}


bool
Sink::init()
{
    if (!_inStreams[0]->getInfo()) {
        Omm::AvStream::Log::instance()->avstream().warning(Poco::format("%s init failed, input stream info not allocated", getName()));
        return false;
    }
    if (!_inStreams[0]->getInfo()->findCodec()) {
        Omm::AvStream::Log::instance()->avstream().warning(Poco::format("%s init failed, could not find codec", getName()));
        return false;
    }
    
    return checkInStream() && initDevice();
}


void
Sink::run()
{
    if (!_inStreams[0]->getQueue()) {
        Omm::AvStream::Log::instance()->avstream().warning("no in stream attached to video sink, stopping.");
        return;
    }
    Omm::AvStream::Frame* pFrame;
    while (!_quit && (pFrame = _inStreams[0]->getFrame())) {
        
        Omm::AvStream::Frame* pDecodedFrame = pFrame->decode();
        if (!pDecodedFrame) {
            Omm::AvStream::Log::instance()->avstream().warning(Poco::format("%s decoding failed, discarding frame %s",
                getName(), pFrame->getName()));
        }
        else {
            if (!_firstDecodeSuccess) {
                _firstDecodeSuccess = true;
                _timeQueue.put(pFrame->getPts());
            }
            writeDecodedFrame(pDecodedFrame);
//             putFrameInOverlayQueue(pDecodedFrame);
        }
    }
    
    Omm::AvStream::Log::instance()->avstream().debug(Poco::format("%s finished.", getName()));
}


void
VideoSink::onTick(int64_t time)
{
    Overlay* pOverlay = _overlayQueue.front();
    
    if (!pOverlay) {
        Omm::AvStream::Log::instance()->avstream().warning("null video frame in overlay queue, ignoring");
        return;
    }
    
    if (pOverlay->_pFrame->getPts() <= time) {
        pOverlay = _overlayQueue.get();
        if (pOverlay) {
//             Overlay* pOverlay = static_cast<SdlOverlay*>(pOverlay);
            displayFrame(pOverlay);
        }
    }
    
//     Omm::AvStream::Log::instance()->avstream().trace(Poco::format("%s stream time: %s, frame %s pts: %s.",
//         getName(),
//         Poco::NumberFormatter::format(time),
//         pOverlay->_pFrame->getName(),
//         Poco::NumberFormatter::format(pOverlay->_pFrame->getPts())));
    
    
}


AudioSink::AudioSink(const std::string& name) :
Sink(name),
// allocate byte queue for 20k s16-2chan-samples
_byteQueue(20 * 1024 * 2 * 2)
{
}


AudioSink::~AudioSink()
{
}


AudioSink*
AudioSink::loadPlugin(const std::string& libraryPath, const std::string& className)
{
    Poco::ClassLoader<AudioSink> sinkPluginLoader;
    if (sinkPluginLoader.isLibraryLoaded(libraryPath)) {
        Log::instance()->avstream().error(Poco::format("library %s already loaded", libraryPath));
        return 0;
    }
    try {
        sinkPluginLoader.loadLibrary(libraryPath);
    }
    catch (Poco::NotFoundException) {
        Log::instance()->avstream().error(Poco::format("could not find  %s sink plugin.", libraryPath));
        return 0;
    }
    catch (Poco::LibraryLoadException) {
        Log::instance()->avstream().error(Poco::format("could not load  %s sink plugin.", libraryPath));
        return 0;
    }
    Log::instance()->avstream().debug(Poco::format("%s plugin successfully loaded.", libraryPath));
    
    Log::instance()->avstream().debug(Poco::format("%s plugin to be allocated ...", className));
    AudioSink* pRes;
    try {
        pRes = sinkPluginLoader.create(className);
    }
    catch (Poco::NotFoundException) {
        Log::instance()->avstream().error(Poco::format("%s could not create instance of plugin.", className));
        return 0;
    }
    Log::instance()->avstream().debug(Poco::format("%s plugin successfully allocated.", className));
    return pRes;
}


bool
AudioSink::checkInStream()
{
    if (!_inStreams[0]->getInfo()->isAudio()) {
        Omm::AvStream::Log::instance()->avstream().warning(Poco::format("%s init failed, input stream is not an audio stream", getName()));
        return false;
    }
    
    return true;    
}


// void
// AudioSink::run()
// {
//     if (!_inStreams[0]->getQueue()) {
//         Omm::AvStream::Log::instance()->avstream().warning("no in stream attached to audio sink, stopping.");
//         return;
//     }
//     Omm::AvStream::Frame* pFrame;
//     while (!_quit && (pFrame = _inStreams[0]->getFrame())) {
//         
//         Omm::AvStream::Frame* pDecodedFrame = pFrame->decode();
//         if (!pDecodedFrame) {
//             Omm::AvStream::Log::instance()->avstream().warning(Poco::format("%s decoding failed, discarding frame %s",
//                 getName(), pFrame->getName()));
//         }
//         else {
//             if (!_firstDecodeSuccess) {
//                 _firstDecodeSuccess = true;
//                 _timeQueue.put(pFrame->getPts());
//             }
//             _byteQueue.write(pDecodedFrame->data(), pDecodedFrame->size());
//         }
//     }
//     Omm::AvStream::Log::instance()->avstream().debug(Poco::format("%s finished.", getName()));
// }


void
AudioSink::writeDecodedFrame(Frame* pDecodedFrame)
{
    _byteQueue.write(pDecodedFrame->data(), pDecodedFrame->size());
}


int
AudioSink::channels()
{
    return _inStreams[0]->getInfo()->channels();
}


unsigned int
AudioSink::sampleRate()
{
    return _inStreams[0]->getInfo()->sampleRate();
}


int
AudioSink::silence()
{
    return 0;
}


bool
AudioSink::audioAvailable()
{
    return _byteQueue.level();
}


int
AudioSink::audioRead(char* buffer, int size)
{
    return _byteQueue.readSome(buffer, size);
}


void
AudioSink::audioReadBlocking(char* buffer, int size)
{
    _byteQueue.read(buffer, size);
}


void
AudioSink::initSilence(char* buffer, int size)
{
    memset(buffer, silence(), size);
}


// bool
// VideoSink::init()
// {
//     if (!_inStreams[0]->getInfo()) {
//         Omm::AvStream::Log::instance()->avstream().warning(Poco::format("%s init failed, input stream info not allocated", getName()));
//         return false;
//     }
//     if (!_inStreams[0]->getInfo()->isVideo()) {
//         Omm::AvStream::Log::instance()->avstream().warning(Poco::format("%s init failed, input stream is not an video stream", getName()));
//         return false;
//     }
//     
//     return initDevice();
// }


Overlay::Overlay(VideoSink* pVideoSink) :
_pVideoSink(pVideoSink),
_pFrame(0)
{
}


int
Overlay::getWidth()
{
    return _pVideoSink->getWidth();
}


int
Overlay::getHeight()
{
    return _pVideoSink->getHeight();
}


PixelFormat
Overlay::getFormat()
{
    return _pVideoSink->getFormat();
}


// VideoSink::VideoSink(const std::string& name, int width, int height, PixelFormat pixelFormat, int overlayCount) :
// Sink(name)
// {
// }


Clock* Clock::_pInstance = 0;

Clock::Clock() :
_streamTime(AV_NOPTS_VALUE),
_clockTick(20),
_streamBase(90.0), // TODO: set _streamBase while loading the stream
_clockTickStreamBase(_clockTick * _streamBase)
{
}


Clock*
Clock::instance()
{
    if (!_pInstance) {
        _pInstance = new Clock;
    }
    return _pInstance;
}


void
Clock::attachAudioSink(AudioSink* pAudioSink)
{
    _audioSinkVec.push_back(pAudioSink);
    
    Log::instance()->avstream().debug(Poco::format("CLOCK attached %s",
        pAudioSink->getName()));
}


void
Clock::attachVideoSink(VideoSink* pVideoSink)
{
    _videoSinkVec.push_back(pVideoSink);
    
    Log::instance()->avstream().debug(Poco::format("CLOCK attached %s",
        pVideoSink->getName()));
}


void
Clock::setTime(int64_t currentTime)
{
    Log::instance()->avstream().debug(Poco::format("CLOCK set stream time to: %s.", Poco::NumberFormatter::format(currentTime)));
    
    _clockLock.lock();
    for (std::vector<VideoSink*>::iterator it = _videoSinkVec.begin(); it != _videoSinkVec.end(); ++it) {
        (*it)->currentTime(currentTime);
    }
    _streamTime = currentTime;
    _clockLock.unlock();
}


void
Clock::clockTick(Poco::Timer& timer)
{
    
    // _streamTime and _clockTickStreamBase is in time base units, clock tick is in ms
    _clockLock.lock();
    if (_streamTime != AV_NOPTS_VALUE) {
        Log::instance()->avstream().debug("CLOCK TICK.");
        
        _streamTime += _clockTickStreamBase;
    
        for (std::vector<VideoSink*>::iterator it = _videoSinkVec.begin(); it != _videoSinkVec.end(); ++it) {
            (*it)->currentTime(_streamTime);
        }
    }
    _clockLock.unlock();
}


void
Clock::start()
{
    Log::instance()->avstream().debug("CLOCK starting ...");
    
    for (std::vector<AudioSink*>::iterator it = _audioSinkVec.begin(); it != _audioSinkVec.end(); ++it) {
        (*it)->startPresentation();
    }
    for (std::vector<VideoSink*>::iterator it = _videoSinkVec.begin(); it != _videoSinkVec.end(); ++it) {
        (*it)->startPresentation();
    }
    _clockTimer.setPeriodicInterval(_clockTick);
    _clockTimer.start(Poco::TimerCallback<Clock>(*this, &Clock::clockTick));
}


void
Clock::stop()
{
    _clockTimer.stop();
    for (std::vector<VideoSink*>::iterator it = _videoSinkVec.begin(); it != _videoSinkVec.end(); ++it) {
        (*it)->stopPresentation();
    }
    for (std::vector<AudioSink*>::iterator it = _audioSinkVec.begin(); it != _audioSinkVec.end(); ++it) {
        (*it)->stopPresentation();
    }
    Log::instance()->avstream().debug("CLOCK stopped.");
}
