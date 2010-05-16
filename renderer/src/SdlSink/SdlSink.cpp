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

#include "SdlSink.h"


SinkPlugin::SinkPlugin() :
_pOverlay(new Omm::Av::Overlay)
{
}


SinkPlugin::~SinkPlugin()
{
    delete _pOverlay;
}


void
SinkPlugin::open()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        std::cerr << "error: initializing SDL: " << SDL_GetError() << std::endl;
        return;
    }
    
    _pSdlScreen = SDL_SetVideoMode(720, 576, 0, SDL_HWSURFACE|SDL_RESIZABLE|SDL_ASYNCBLIT|SDL_HWACCEL);
    if (_pSdlScreen == 0) {
        std::cerr << "error: could not open SDL window" << SDL_GetError() << std::endl;
        return;
    }

    _pSdlOverlay = SDL_CreateYUVOverlay(720, 576, SDL_YV12_OVERLAY, _pSdlScreen);
    _pOverlay->_data[0] = _pSdlOverlay->pixels[0];
    _pOverlay->_data[1] = _pSdlOverlay->pixels[2];
    _pOverlay->_data[2] = _pSdlOverlay->pixels[1];
    
    _pOverlay->_pitch[0] = _pSdlOverlay->pitches[0];
    _pOverlay->_pitch[1] = _pSdlOverlay->pitches[2];
    _pOverlay->_pitch[2] = _pSdlOverlay->pitches[1];
}


void
SinkPlugin::close()
{
    std::clog << "SinkPlugin::close()" << std::endl;
    
}


void
SinkPlugin::writeFrame(Omm::Av::Frame* pFrame)
{
    int width = pFrame->getStream()->width();
    int height = pFrame->getStream()->height();
    std::clog << "SDL video sink::writeFrame() width: " << width << " height: " << height << std::endl;
    
    SDL_LockYUVOverlay(_pSdlOverlay);
    pFrame->write(_pOverlay);
    SDL_UnlockYUVOverlay(_pSdlOverlay);
    
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    SDL_DisplayYUVOverlay(_pSdlOverlay, &rect);
}


int
SinkPlugin::eventLoop()
{
    std::clog << "event loop ..." << std::endl;
    Poco::Thread::sleep(5000);
    
}



POCO_BEGIN_MANIFEST(Omm::Av::Sink)
POCO_EXPORT_CLASS(SinkPlugin)
POCO_END_MANIFEST
