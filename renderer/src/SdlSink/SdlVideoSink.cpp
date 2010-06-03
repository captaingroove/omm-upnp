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
#include <Poco/Thread.h>
#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include "SdlVideoSink.h"


SdlOverlay::SdlOverlay(Omm::AvStream::VideoSink* pVideoSink) :
Overlay(pVideoSink)
{
}


SdlVideoSink::SdlVideoSink() :
// reserve 5 overlays for SdlVideoSink
VideoSink("sdl video sink", 720, 576, PIX_FMT_YUV420P, 5)
{
}


SdlVideoSink::~SdlVideoSink()
{
}


bool
SdlVideoSink::initDevice()
{
    Omm::AvStream::Log::instance()->avstream().debug("opening SDL video sink ...");
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        Omm::AvStream::Log::instance()->avstream().error(Poco::format("failed to init SDL:  %s", std::string(SDL_GetError())));
        return false;
    }
    
    SDL_Surface* pSdlScreen = SDL_SetVideoMode(getWidth(), getHeight(), 0, SDL_HWSURFACE | SDL_RESIZABLE | SDL_ASYNCBLIT | SDL_HWACCEL);
    
    if (pSdlScreen == 0) {
        Omm::AvStream::Log::instance()->avstream().error(Poco::format("could not open SDL window: %s", std::string(SDL_GetError())));
        return false;
    }
    
    for (int numOverlay = 0; numOverlay < _overlayCount; numOverlay++) {
        // SDL_YV12_OVERLAY corresponds to ffmpeg::PixelFormat == PIX_FMT_YUV420P
        // TODO: catch, if SDL_Overlay could not be created (video card has to few memory)
        SDL_Overlay* pSDLOverlay = SDL_CreateYUVOverlay(getWidth(), getHeight(), SDL_YV12_OVERLAY, pSdlScreen);
        SdlOverlay* pOverlay = new SdlOverlay(this);
        
        pOverlay->_pSDLOverlay = pSDLOverlay;
        
        pOverlay->_data[0] = pSDLOverlay->pixels[0];
        pOverlay->_data[1] = pSDLOverlay->pixels[2];
        pOverlay->_data[2] = pSDLOverlay->pixels[1];
        
        pOverlay->_pitch[0] = pSDLOverlay->pitches[0];
        pOverlay->_pitch[1] = pSDLOverlay->pitches[2];
        pOverlay->_pitch[2] = pSDLOverlay->pitches[1];
        
        _overlayVector[numOverlay] = pOverlay;
    }

    Omm::AvStream::Log::instance()->avstream().debug(Poco::format("%s opened.", getName()));
    return true;
}


void
SdlVideoSink::displayFrame(Omm::AvStream::Overlay* pOverlay)
{
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = getWidth();
    rect.h = getHeight();
    
    Omm::AvStream::Log::instance()->avstream().debug(Poco::format("%s display frame %s, width: %s, height: %s",
        getName(),
        pOverlay->_pFrame->getName(),
        Poco::NumberFormatter::format(rect.w),
        Poco::NumberFormatter::format(rect.h)));
    
    SDL_DisplayYUVOverlay(static_cast<SdlOverlay*>(pOverlay)->_pSDLOverlay, &rect);
}


int
SdlVideoSink::eventLoop()
{
    Omm::AvStream::Log::instance()->avstream().debug("event loop ...");
//     Poco::Thread::sleep(10000);
    
}



POCO_BEGIN_MANIFEST(Omm::AvStream::VideoSink)
POCO_EXPORT_CLASS(SdlVideoSink)
POCO_END_MANIFEST
