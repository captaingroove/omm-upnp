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
#ifndef AlsaSink_INCLUDED
#define AlsaSink_INCLUDED

#include <Omm/AvStream.h>

#include <alsa/asoundlib.h>
#include <alsa/pcm.h>

class SinkPlugin : public Omm::Av::Sink {
public:
    SinkPlugin();
    ~SinkPlugin();
    void open();
    void open(const std::string& device);
    void close();
    // Writes blocking
    void writeFrame(Omm::Av::Frame *pFrame);
//     void pause();
//     void resume();
//     int latency();
    
private:
    void         initDevice();
    
    snd_pcm_t* pcm_playback;
    snd_pcm_hw_params_t *hw;
    std::string device;
    snd_pcm_format_t format;
    unsigned int rate;
    int channels;
    int periods;       // Number of periods
    snd_pcm_uframes_t periodsize; // Periodsize (bytes)
    char* buffer;
    char* bufferPos;
    snd_pcm_uframes_t frames;
};

#endif
