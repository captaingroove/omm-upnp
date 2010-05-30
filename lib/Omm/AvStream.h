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
#include <deque>

#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Runnable.h>
#include <Poco/Semaphore.h>
#include <Poco/Mutex.h>
#include <Poco/Timer.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libpostproc/postprocess.h>

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


template<typename T>
class Queue
{
public:
    Queue(const std::string& name, int size, int putTimeout, int getTimeout) :
        _name(name),
        _size(size),
        _putTimeout(putTimeout),
        _getTimeout(getTimeout),
        _putSemaphore(size, size),
        _getSemaphore(0, size)
    {
    }
    
    
    bool put(T element)
    {
        Log::instance()->avstream().trace(Poco::format("%s queue put waiting in thread %s ...",
            getName(), Poco::Thread::current()->name()));
        
//         if (!_putSemaphore.tryWait(_putTimeout)) {
//             Log::instance()->avstream().warning(Poco::format("%s queue put timed out in thread %s.",
//                 getName(), Poco::Thread::current()->name()));
//             return false;
//         }
        _putSemaphore.wait();
        
        _lock.lock();
        _queue.push(element);
        _lock.unlock();
        
        _getSemaphore.set();
        
        Log::instance()->avstream().trace(Poco::format("%s queue put success in thread %s, size: %s",
            getName(), Poco::Thread::current()->name(), Poco::NumberFormatter::format(_queue.size())));
        return true;
    }
    
    
    T get()
    {
        Log::instance()->avstream().trace(Poco::format("%s queue get waiting in thread %s ...",
            getName(), Poco::Thread::current()->name()));
        
//         if (!_getSemaphore.tryWait(_getTimeout)) {
//             Log::instance()->avstream().warning(Poco::format("%s queue get timed out in thread %s.",
//                 getName(), Poco::Thread::current()->name()));
//             // FIXME: Queue::get() what to return if timed out ...? 
//             // -> throw an exception ... use Poco::Semaphore::tryWait() with exception ...
//             // -> bool get(T& element)
//             return false;
//         }
        
        _getSemaphore.wait();
            
        _lock.lock();
        T ret = _queue.front();
        _queue.pop();
        _lock.unlock();
        
        _putSemaphore.set();
        
        Log::instance()->avstream().trace(Poco::format("%s queue get success in thread %s, size: %s",
            getName(), Poco::Thread::current()->name(), Poco::NumberFormatter::format(_queue.size())));
        return ret;
    }
    
    
    T front()
    {
        Poco::ScopedLock<Poco::FastMutex> lock(_lock);
        return _queue.front();
    }
    
    
    std::string getName()
    {
        return _name;
    }
    
private:
    std::string             _name;
    std::queue<T>           _queue;
    int                     _size;
    // wait _packetQueuePutTimeout ms for data to be put into the queue
    int                     _putTimeout;
    // wait _packetQueueGetTimeout ms for data to be fetched from the queue
    int                     _getTimeout;
    
    Poco::Semaphore         _putSemaphore;
    Poco::Semaphore         _getSemaphore;
    Poco::FastMutex         _lock;
};


class StreamQueue : public Queue<Frame*>
{
public:
    StreamQueue(Node* pNode, int size = 20, int putTimeout = 500, int getTimeout = 500);
    
    Node* getNode();
    
private:
    Node*       _pNode;
};


// FIXME: access to StreamInfo may be locked here and there
class StreamInfo
{
    friend class Stream;
    friend class Demuxer;
    friend class Frame;
    
public:
    StreamInfo();
    
    bool isAudio();
    bool isVideo();
    
    bool findCodec();
    
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
    
private:
    std::string             _streamName;
    AVStream*               _pAvStream;
    AVCodecContext*         _pAvCodecContext;
    AVCodec*                _pAvCodec;
    int64_t               _newFrameNumber;
};


class Stream
{
    friend class Demuxer;
    friend class Frame;
    
public:
    // TODO: in and out stream ctors (only in streams allocate a queue)
    Stream(Node* node, const std::string name = "avstream");
    ~Stream();
    
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
    
    // needed by C callback functions (see SdlAudioSink)
    bool doStop();
    Stream* getInStream(int inStreamNumber);
    
protected:
    virtual bool init() {}
//     virtual bool prepareStart();
    virtual void run() {}
    
//     void setStop(bool stop);

    
    std::string                     _name;
    std::vector<Stream*>            _inStreams;
    std::vector<Stream*>            _outStreams;
    Poco::Thread                    _thread;
    Poco::FastMutex                 _lock;
    bool                            _quit;
};


class Demuxer : public Node
{
public:
    Demuxer();
    
    void set(std::istream& istr);
    void reset();
    
    int firstAudioStream();
    int firstVideoStream();
    
private:
    bool init();
    
    virtual void run();
    
    Meta*       _pMeta;
    int         _firstAudioStream;
    int         _firstVideoStream;
};


// class Monotonizer : public Node
// {
//     /*
//     You should uses the timestamps given by the demuxer. Unless the encoder
// does B-frames this should work well enough (if not you'll have to
// reorder the demuxed timestamps in the same fashion as the encoder did).
// coded_frame->pts contains synthetic values.
// What you should do is rescale from the input stream's time base to the
// output stream's time base. Something like:
//     
// pkt.pts = av_rescale_q(inpkt.pts, inctx->streams[0]->time_base,
// ctx->streams[0]->time_base);
//     
// In the case of flv -> flv the time bases are probably the same (1 kHz),
// but it's a good idea to do this anyway.
//     
// The reason for this is that flv has variable frame rate. I've had files
// with initial time stamps like 0, 333, 666, 1000, 1033, 1066, 1100..
//     */
//     
//     /*
//     Seven Symptoms of Sick Streams
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
//     */
//     
// public:
//     Monotonizer();
//     
// private:
//     virtual bool init();
//     virtual void run();
//     
//     std::deque<Frame*>      _frameWindow;
// };


class Decoder : public Node
{
public:
    Decoder();
    
private:
    virtual bool init();
    virtual void run();
};


class Frame
{
    friend class Stream;
    
public:
    Frame(int64_t number, const Frame& frame);
    Frame(int64_t number, Stream* pStream);
    Frame(int64_t number, Stream* pStream, int dataSize);
    Frame(int64_t number, Stream* pStream, char* data, int dataSize);
    
    // copies *pAvPacket (and it's payload) into Frame and deletes *pAvPacket (and it's payload)
    Frame(int64_t number, Stream* pStream, AVPacket* pAvPacket);
    // copies *pAvFrame (and it's payload) into Frame and deletes *pAvFrame (and it's payload)
    Frame(int64_t number, Stream* pStream, AVFrame* pAvFrame);
    ~Frame();
    
    char* data();
    int size();
    int paddedSize();
    
    char* planeData(int plane);
    int planeSize(int plane);
    
    void printInfo();
    std::string getName();
    int64_t getNumber();
    int64_t getPts();
    void setPts(int64_t pts);
    
    Stream* getStream();
    
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
    int64_t             _pts;
    char*               _data;
    int                 _size;
    int                 _paddedSize;
    // face 2: packet coming from the demuxer
    AVPacket*           _pAvPacket;
    // face 3: decoded frame
    AVFrame*            _pAvFrame;
    
    // reference to the stream this frame is contained in
    Stream*             _pStream;
};


class Meta
{
    friend class Tagger;
    friend class Demuxer;
    
public:
    Meta();
    ~Meta();
    
    void print();
    
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
    
    // TODO: istr is currently ignored (hardcoded av_open_input_file())
    Meta* tag(std::istream& istr);
    
private:
    AVInputFormat* probeInputFormat(std::istream& istr);
    ByteIOContext* initIo(std::istream& istr);
    
    int                 _tagBufferSize;
    int                 _IoBufferSize;
    unsigned char*      _pIoBuffer;
};


class Overlay
{
public:
    Overlay(Sink* pSink);
    
    int getWidth();
    int getHeight();
    PixelFormat getFormat();
    
    uint8_t*        _data[4];
    int             _pitch[4];
    
    Sink*           _pSink;
    Frame*          _pFrame;
};


class Sink : public Node
{
    friend class Stream;
    friend class Clock;
    
public:
    Sink(const std::string& name = "sink",
         int width = 720, int height = 576, PixelFormat pixelFormat = PIX_FMT_YUV420P,
         int overlayCount = 1);
    virtual ~Sink() {}
    
    static Sink* loadPlugin(const std::string& libraryPath, const std::string& className = "SinkPlugin");
    virtual int eventLoop() { return 0; }
    
    void currentTime(int64_t time);
    void startPresentation();
    void stopPresentation();
    
    int getWidth();
    int getHeight();
    PixelFormat getFormat();
    
protected:
    virtual void onTick(int64_t time) {}
    virtual void beforeTimerStart() {}
    virtual void afterTimerStart() {}
    
    void startTimer();
    void stopTimer();
    
    int                     _overlayCount;
    std::vector<Overlay*>   _overlayVector;
    Queue<Overlay*>         _overlayQueue;
    
    bool                    _timerQuit;
    
private:
    void timerThread();
    
    Queue<int64_t>          _timerQueue;
    Poco::Thread            _timerThread;
    int64_t                 _currentTime;
    
    int                     _width;
    int                     _height;
    PixelFormat             _pixelFormat;
    
    Poco::FastMutex         _sinkLock;
};


class AudioSink : public Sink
{
    AudioSink(const std::string& name = "audio sink");
};


class VideoSink : public Sink
{
public:
    VideoSink(const std::string& name = "video sink");
};


class Clock
{
public:
    static Clock* instance();
    
    // attachSink()
    // pSink: pointer to sink, that receives the timing signals
    void attachSink(Sink* pSink);
    // setTime()
    // sets clock's current stream time to currentTime
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
    
    Poco::Timer             _clockTimer;
    long                    _clockTick;
    long                    _clockTickStreamBase;
    
    std::vector<Sink*>      _sinkVec;
    
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
