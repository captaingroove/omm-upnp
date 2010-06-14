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
#ifndef AvStream_INCLUDED
#define AvStream_INCLUDED

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <queue>

#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Runnable.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/Semaphore.h>
#include <Poco/Mutex.h>
#include <Poco/RWLock.h>
#include <Poco/Timer.h>
#include <Poco/DateTime.h>

// FIXME: make clear if you need to set -malign-double when linking with libommavstream and ffmpeg is compiled with -malign-double
// FIXME: find a build check for ffmpeg, if it is compiled with -malign-double

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
// #include <libavfilter/avfilter.h>
// #include <libavdevice/avdevice.h>
// #include <libpostproc/postprocess.h>

//Forward declaration of ffmpeg structs
struct AVInputFormat;
struct AVFormatContext;
struct AVCodecContext;
struct AVRational;
struct SwsContext;
struct ReSampleContext;
struct AVPicture;
struct AVPacket;
struct AVPacketList;
struct AVStream;
}

namespace Omm {
namespace AvStream {

class Meta;
class Node;
class Frame;
class Stream;
class StreamInfo;
class StreamQueue;
class Overlay;
class Sink;
class PresentationTimer;


class Log
{
public:
    static Log* instance();
    
    Poco::Logger& avstream();
    
private:
    Log();
    
    static Log*     _pInstance;
    Poco::Logger*   _pAvStreamLogger;
};


class RingBuffer
{
public:
    RingBuffer(int size);
    ~RingBuffer();
    
    /**
    NOTE: this is no generic implemenation of a ring buffer:
    1. read() and write() don't check if num > size
    2. it is not thread safe
    this is all done in the customer ByteQueue
    **/
    void read(char* buffer, int num);
    void write(const char* buffer, int num);
    
private:
    char*                   _ringBuffer;
    char*                   _readPos;
    char*                   _writePos;
    int                     _size;
};


/**
class ByteQueue - a blocking byte stream with a fixed size
**/
class ByteQueue
{
public:
    ByteQueue(int size);
    
    /**
    read() and write() block until num bytes have been read or written
    **/
    void read(char* buffer, int num);
    void write(const char* buffer, int num);
    
    /**
    readSome() and writeSome() read upto num bytes, return the number of bytes read
    and block if queue is empty / full
    **/
    int readSome(char* buffer, int num);
    int writeSome(const char* buffer, int num);
    
    int level();
    
private:
//     int readSomeSemaphore(char* buffer, int num);
//     int writeSomeSemaphore(char* buffer, int num);
//     int readSomeRwLock(char* buffer, int num);
//     int writeSomeRwLock(char* buffer, int num);
    
    
    RingBuffer              _ringBuffer;
    int                     _size;
    volatile int            _level;
    // TODO: this should also work with a read lock and a write lock and a condition. Use:
    // Poco::FastMutex, Poco::Condition
    // TODO: alternatively use a Poco::SharedObject
    Poco::Semaphore         _writeSemaphore;
    Poco::Semaphore         _readSemaphore;
    Poco::FastMutex         _lock;
//     Poco::RWLock            _writeLock;
//     Poco::RWLock            _readLock;
};


template<typename T>
class Queue
{
public:
    Queue(const std::string& name, int size) :
        _name(name),
        _size(size),
        _putSemaphore(size, size),
        _getSemaphore(0, size)
        // FIXME: this should also work, but crashes when allocating sink plugin
//         _putSemaphore(size),
//         _getSemaphore(0)
    {
    }
    
    
    void put(T element)
    {
//         Log::instance()->avstream().debug("Queue::put() wait ...");
        _putSemaphore.wait();
//         Log::instance()->avstream().debug("Queue::put()");
        _queueLock.lock();
        
        _queue.push(element);
        
        _queueLock.unlock();
        _getSemaphore.set();
//         Log::instance()->avstream().debug("Queue::put() finished");
    }
    
    
    T get()
    {
//         Log::instance()->avstream().debug("Queue::get() wait ...");
        _getSemaphore.wait();
//         Log::instance()->avstream().debug("Queue::get()");
        _queueLock.lock();
        
        T ret = _queue.front();
        _queue.pop();
        
        _queueLock.unlock();
        _putSemaphore.set();
//         Log::instance()->avstream().debug("Queue::put() returning");
        
        return ret;
    }
    
    
    T front()
    {
//         Log::instance()->avstream().debug("Queue::front()");
        Poco::ScopedLock<Poco::FastMutex> queueLock(_queueLock);
        Poco::ScopedLock<Poco::FastMutex> sizeLock(_sizeLock);
        if (_queue.size() == 0) {
            return T();
        }
        return _queue.front();
    }
    
    
    const int count()
    {
//         Log::instance()->avstream().debug("Queue::count()");
        Poco::ScopedLock<Poco::FastMutex> lock(_sizeLock);
        return _queue.size();
    }
    
    
    const std::string& getName()
    {
//         Log::instance()->avstream().debug("Queue::getName()");
        Poco::ScopedLock<Poco::FastMutex> lock(_nameLock);
        return _name;
    }
    
private:
    std::string             _name;
    Poco::FastMutex         _nameLock;
    std::queue<T>           _queue;
    Poco::FastMutex         _queueLock;
    int                     _size;
    Poco::FastMutex         _sizeLock;
    
    Poco::Semaphore         _putSemaphore;
    Poco::Semaphore         _getSemaphore;
};


class StreamQueue : public Queue<Frame*>
{
public:
    StreamQueue(Node* pNode, int size = 100);
    
    Node* getNode();
    
private:
    Node*       _pNode;
};


// FIXME: access to StreamInfo may be locked here and there
class StreamInfo
{
    friend class Stream;
    friend class Demuxer;
    friend class Muxer;
    friend class Frame;
    
public:
    StreamInfo();
    
    bool isAudio();
    bool isVideo();
    
    bool findCodec();
    bool findEncoder();
    
    void printInfo();
    
    int width();
    int height();
    
    // audio parameters
    int sampleWidth();
    unsigned int sampleRate();
    int channels();
    
    // video parameters
    PixelFormat pixelFormat();
    int pictureSize();
    
    int64_t newFrameNumber();
    
    StreamInfo* cloneOutStreamInfo(Meta* pMeta, int outStreamNumber);
    
private:
    std::string             _streamName;
    AVStream*               _pAvStream;
    AVCodecContext*         _pAvCodecContext;
    AVCodec*                _pAvCodec;
    int64_t                 _newFrameNumber;
};


class Stream
{
    friend class Demuxer;
    friend class Frame;
    
public:
    // TODO: in and out stream ctors (only in streams allocate a queue)
    Stream(Node* node, const std::string name = "avstream");
    ~Stream();
    
    Frame* firstFrame();
    Frame* getFrame();
    void putFrame(Frame* pFrame);
    
    Node* getNode();
    
    // TODO: extend this to handle more than one queue (tee stream)
    // getQueue(int queueNumber = 0)
    // addQueue(StreamQueue* pQueue, int queueNumber = 0)
    // removeQueue(int queueNumber = 0)
    // _pStreamQueue -> std::vector<StreamQueue*>
    std::string getName();
    StreamInfo* getInfo();
    StreamQueue* getQueue();
    void setName(const std::string& name);
    void setInfo(StreamInfo* pInfo);
    void setQueue(StreamQueue* pQueue);
    
    Frame* allocateVideoFrame(PixelFormat targetFormat);
    Frame* decodeFrame(Frame* pFrame);
    
private:
    Frame* decodeAudioFrame(Frame* pFrame);
    Frame* decodeVideoFrame(Frame* pFrame);
    
    // _pNode of a Stream and of a StreamQueue can differ, as StreamQueue is shared between in and out streams
    Node*               _pNode;
    // _pStreamInfo and _pStreamQueue can be shared between in and out streams or are 0 if not attached
    StreamInfo*         _pStreamInfo;
    // _pStreamQueue always belongs to the input stream of a node
    StreamQueue*        _pStreamQueue;
    Poco::FastMutex     _lock;
};


class Node : Poco::Runnable
{
public:
    Node(const std::string& name = "avstream node");
    
    std::string getName();
    void setName(const std::string& name);
    
    void start();
    void stop();
    
    void attach(Node* node, int outStreamNumber = 0, int inStreamNumber = 0);
    void detach(int outStreamNumber = 0);
    
    Node* getDownstreamNode(int outStreamNumber = 0);
    
    bool doStop();
    
    Stream* getInStream(int inStreamNumber);
    Stream* getOutStream(int outStreamNumber);
    
protected:
    virtual bool init() { return true; }
    virtual void run() {}
    
    
    std::string                     _name;
    Poco::FastMutex                 _nameLock;
    std::vector<Stream*>            _inStreams;
    Poco::FastMutex                 _inStreamsLock;
    std::vector<Stream*>            _outStreams;
    Poco::FastMutex                 _outStreamsLock;
    bool                            _quit;
    Poco::FastMutex                 _quitLock;
    Poco::Thread                    _thread;
};


class Demuxer : public Node
{
public:
    Demuxer();
    
    void set(const std::string& uri);
    void set(std::istream& istr);
    void reset();
    
    int firstAudioStream();
    int firstVideoStream();
    
private:
    virtual bool init();
    virtual void run();
    
    int64_t correctPts(int64_t pts, int64_t lastPts, int lastDuration);
    
    Meta*       _pMeta;
    int         _firstAudioStream;
    int         _firstVideoStream;
};


/**
    @class Muxer
    mux one audio and one video stream
    container format is the same as input stream
*/
class Muxer : public Node
{
public:
    Muxer();
    ~Muxer();
    
    void set(const std::string& uri = "mux.ts", const std::string& format = "mpegts");
    void set(std::ostream& ostr, const std::string& format = "mpegts");
    void reset();
    
private:
    virtual bool init();
    virtual void run();
    
    bool prepareOutStream(int streamNumber);
    
    Meta*           _pMeta;
    std::string     _uri;
};


class Decoder : public Node
{
public:
    Decoder();
    
private:
    virtual bool init();
    virtual void run();
};


// class FrameInfo
// {
// public:
//     
// };


class Frame
{
    friend class Stream;
    friend class Muxer;
    
public:
//     Frame(int64_t number, const Frame& frame);
    Frame(int64_t number, Stream* pStream);
    Frame(int64_t number, Stream* pStream, int dataSize);
//     Frame(int64_t number, Stream* pStream, char* data, int dataSize);
    
    // copies *pAvPacket (and it's payload) into Frame and deletes *pAvPacket (and it's payload)
    Frame(int64_t number, Stream* pStream, AVPacket* pAvPacket);
    // copies *pAvFrame (and it's payload) into Frame and deletes *pAvFrame (and it's payload)
    Frame(int64_t number, Stream* pStream, AVFrame* pAvFrame);
    ~Frame();
    
    const char* data();
    const int size();
    int paddedSize();
    
    char* planeData(int plane);
    int planeSize(int plane);
    
    void printInfo();
    std::string getName();
    const int64_t getNumber();
    const int64_t getPts();
    void setPts(int64_t pts);
    
    const Stream* getStream();
    
    Frame* decode();
    
    // width and height = -1 means, leave the size of the frame as it is
    Frame* convert(PixelFormat targetFormat, int targetWidth = -1, int targetHeight = -1);
    
    void writePpm(const std::string& fileName);
    void write(Overlay* overlay);
    
    AVPacket* copyPacket(AVPacket* pAvPacket, int padSize);
    AVFrame* allocateFrame(PixelFormat format);
    AVFrame* copyFrame(AVFrame* pAvFrame);
    
private:
    // Frame must be a dynamic structure with three different "faces", determined at runtime.
    // face 1: simple buffer
    int64_t             _number;
    Poco::Mutex         _numberLock;
    int64_t             _pts;
    Poco::Mutex         _ptsLock;
    char*               _data;
    int                 _size;
    Poco::Mutex         _sizeLock;
    int                 _paddedSize;
    // face 2: packet coming from the demuxer
    AVPacket*           _pAvPacket;
    // face 3: decoded frame
    AVFrame*            _pAvFrame;
    
    // reference to the stream this frame is contained in
    Stream*             _pStream;
    Poco::FastMutex     _nameLock;
};


class Meta
{
    friend class Tagger;
    friend class StreamInfo;
    friend class Demuxer;
    friend class Muxer;
    
public:
    Meta();
    ~Meta();
    
    void print(bool isOutFormat = false);
    
private:
    AVFormatContext*    _pFormatContext; // contains pointers to ByteIOContext, AVInputFormat, codec ids, array of streams with AVCodecContext
    ByteIOContext*      _pIoContext;
    AVInputFormat*      _pInputFormat;  // contains the io access callbacks
};


class Tagger
{
public:
    Tagger();
    ~Tagger();
    
    Meta* tag(const std::string& uri);
    // TODO: istr is currently ignored (hardcoded av_open_input_file())
    Meta* tag(std::istream& istr);
    
private:
    AVInputFormat* probeInputFormat(std::istream& istr);
    ByteIOContext* initIo(std::istream& istr);
    
    int                 _tagBufferSize;
    int                 _IoBufferSize;
    unsigned char*      _pIoBuffer;
};


/*
sinks in warmup phase:
1. audio and video set start time to first succesfully decoded frame
2. clock waits until all registered sinks set their start time
3. clock decides wether to start with min(start times) or max(start times)
-> min(start times): set clock
-> max(start times): tell all sinks to discard old frames and set clock

sinks in running phase:
1. don't start before warmup phase is finished
...
*/

class Sink : public Node
{
    friend class Clock;
    
public:
    Sink(const std::string& name = "sink");
    virtual ~Sink() {}
    
    virtual void startPresentation() {}
    virtual void stopPresentation() {}
    
    void currentTime(int64_t time);

protected:
    virtual bool checkInStream() {}
    virtual bool initDevice() {}
    virtual void writeDecodedFrame(Frame* pDecodedFrame) {}

    Queue<int64_t>          _timeQueue;
    bool                    _firstDecodeSuccess;
    int64_t                 _startTime;
    
private:
    virtual bool init();
    virtual void run();
};


class AudioSink : public Sink
{
public:
    AudioSink(const std::string& name = "audio sink");
    virtual ~AudioSink();
    
    void setStartTime(int64_t startTime);
    
    // methods for the audio driver (public, so it can be called form C callbacks)
    bool audioAvailable();
    int audioRead(char* buffer, int size);
    void audioReadBlocking(char* buffer, int size);
    void initSilence(char* buffer, int size);

protected:
    int channels();
    unsigned int sampleRate();
    int silence();
    
private:
    virtual bool checkInStream();
    virtual void writeDecodedFrame(Frame* pDecodedFrame);
    int64_t audioLength(int64_t bytes);
    
    Omm::AvStream::ByteQueue    _byteQueue;
    int64_t                     _audioTime;
};


class Overlay;

class VideoSink : public Sink
{
public:
    VideoSink(const std::string& name = "video sink",
         int width = 720, int height = 576, PixelFormat pixelFormat = PIX_FMT_YUV420P,
         int overlayCount = 5);
    
    virtual int eventLoop() { return 0; }
    
    void setStartTime(int64_t startTime);
    
    virtual void startPresentation();
    virtual void stopPresentation();
    
    int getWidth();
    int getHeight();
    PixelFormat getFormat();
    
protected:
    virtual void displayFrame(Overlay* pOverlay) {}
    
    int                         _overlayCount;
    std::vector<Overlay*>       _overlayVector;
    Queue<Overlay*>             _overlayQueue;
    
    bool                        _timerQuit;
    
private:
    virtual bool checkInStream();
    virtual void writeDecodedFrame(Frame* pDecodedFrame);
    
    void onTick(int64_t time);
    void timerThread();
    
    Poco::Thread                        _timerThread;
    Poco::RunnableAdapter<VideoSink>    _timerThreadRunnable;
    Poco::FastMutex                     _sinkLock;
    
    int                                 _width;
    int                                 _height;
    PixelFormat                         _pixelFormat;
    int                                 _writeOverlayNumber;
};


class Overlay
{
public:
    Overlay(VideoSink* pVideoSink);
    
    int getWidth();
    int getHeight();
    PixelFormat getFormat();
    
    uint8_t*        _data[4];
    int             _pitch[4];
    
    VideoSink*      _pVideoSink;
    Frame*          _pFrame;
};


class Clock
{
public:
    static Clock* instance();
    
    /** attachAudioSink() / attachVideoSink()
        pSink: pointer to sink, that receives the timing signals
    **/
    void attachAudioSink(AudioSink* pAudioSink);
    void attachVideoSink(VideoSink* pVideoSink);
    
    void setStartTime(bool toFirstFrame = false);
    
    /** setTime()
        sets clock's current stream time to currentTime and notifies sinks
    **/
    void setTime(int64_t currentTime);
    
    // start clock
    void start();
    void stop();
    
private:
    Clock();
    
    void clockTick(Poco::Timer& timer);
    
    static Clock*           _pInstance;
    // TODO: should Clock be able to sync more than one stream?
    // -> store a stream time for each stream
    int64_t                 _streamTime;  // stream current time in stream time base units [sec]
    double                  _streamBase;  // stream refresh rate in kHz ( = 1 / (time base * 1000))
    Poco::Timestamp         _systemTime;
    
    Poco::Timer             _clockTimer;
    long                    _clockTick;
    long                    _clockTickStreamBase;
    
    std::vector<AudioSink*> _audioSinkVec;
    std::vector<VideoSink*> _videoSinkVec;
    
    Poco::FastMutex         _clockLock;
};


class Scanner
{
};


class Transcoder
{
};



} // namespace AvStream
} // namespace Omm

#endif
