/***************************************************************************
 *   Copyright (C) 2006 by Jörg Bakker                                     *
 *   joerg@hakker.de                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <qdir.h>
#include <iostream>

#include "jamstreamplayerxine.h"


JAMStreamPlayerXine::JAMStreamPlayerXine(QWidget *parent, const char *name)
 : JAMStreamPlayer(parent, name)
{
    QPoint g = mapToGlobal(QPoint(0,0));
    globX = g.x();
    globY = g.y();

    XInitThreads ();
    
    //xineDisplay = XOpenDisplay( getenv("DISPLAY") );
    xineDisplay = XOpenDisplay(NULL);
    xineScreen = DefaultScreen(xineDisplay);
    xineWindow = winId();

    XLockDisplay( xineDisplay );
//    XSelectInput( xineDisplay, xineWindow, ExposureMask );
    res_h = (DisplayWidth(xineDisplay, xineScreen) * 1000 / DisplayWidthMM(xineDisplay, xineScreen));
    res_v = (DisplayHeight(xineDisplay, xineScreen) * 1000 / DisplayHeightMM(xineDisplay, xineScreen));
    pixel_aspect = res_v / res_h;
    XSync(xineDisplay, False);
    XUnlockDisplay( xineDisplay );

    xineEngine = xine_new();

    //xine_engine_set_param(xineEngine, XINE_ENGINE_PARAM_VERBOSITY, 99);
    
    QString configFile = QDir::homeDirPath();
    configFile.append("/.jam/xineconfig");
    
    QString videoDriverName = "xv";
    QString audioDriverName = "auto";

    if (QFile::exists(configFile))
    {
        xine_config_load(xineEngine, configFile);
    }
    
    xine_init(xineEngine);

    /* init video driver */
    visual.display = xineDisplay;
    visual.screen = xineScreen;
    visual.d = xineWindow;
    visual.dest_size_cb = DestSizeCallback;
    visual.frame_output_cb = FrameOutputCallback;
    visual.user_data = (void*)this;

    videoDriver = xine_open_video_driver(xineEngine,
        videoDriverName,  XINE_VISUAL_TYPE_X11,
        (void *) &(visual));
    
    if (!videoDriver)
    {
        std::cout << "JAM: Can't init Video Driver! (" << videoDriverName << ")\n";
    }

    audioDriver = xine_open_audio_driver(xineEngine, audioDriverName, NULL);

    std::cout << "JAM: Creating new xine stream.\n";
    xineStream = xine_stream_new(xineEngine, audioDriver, videoDriver);
    
    m_OSD = NULL;
    connect(&m_OSDTimer, SIGNAL(timeout()), this, SLOT(hideOSD()));
}


JAMStreamPlayerXine::~JAMStreamPlayerXine()
{
    std::cout << "JAM: closing xine streamplayer\n";
    xine_close(xineStream);
    xine_dispose(xineStream);
    xine_close_audio_driver(xineEngine, audioDriver);
    xine_close_video_driver(xineEngine, videoDriver);
    xine_exit(xineEngine);
    if (xineDisplay)
        XCloseDisplay (xineDisplay);
    xineDisplay = NULL;
}


void
JAMStreamPlayerXine::showOSD(QString text, uint duration)
{
    initOSD();
    qDebug("JAMStreamPlayerXine::showOSD(), with text: %s", text.latin1());
    xine_osd_draw_text(m_OSD, 0, 0, text, XINE_OSD_TEXT1);
    //xine_osd_draw_rect(m_OSD, 0, 0, 500, 100, 200, 1 );

    xine_osd_show(m_OSD, 0 );
    m_OSDTimer.start(duration, TRUE);
}


void
JAMStreamPlayerXine::hideOSD()
{
    qDebug("JAMStreamPlayerXine::hideOSD()");
    xine_osd_hide(m_OSD, 0);
    xine_osd_free(m_OSD);
    m_OSD = NULL;
}


void
JAMStreamPlayerXine::initOSD()
{
    qDebug("JAMStreamPlayerXine::initOSD()");
    if (m_OSD != NULL)
        hideOSD();
    if (m_OSDTimer.isActive())
        m_OSDTimer.stop();

    m_marginOSD = this->height() / 10;
    m_OSD = xine_osd_new(xineStream, m_marginOSD, (int)(this->height() * 0.75), this->width() - 2*m_marginOSD, (int)((this->height() * 0.25)) - m_marginOSD);
    if (!m_OSD)
        qDebug("JAMStreamPlayerXine::initOSD(), initialization of OSD failed");
    if (!xine_osd_set_font(m_OSD, "sans", 24))
        qDebug("JAMStreamPlayerXine::initOSD(), initialization of OSD font failed");
    //xine_osd_set_text_palette(m_OSD, XINE_TEXTPALETTE_WHITE_BLACK_TRANSPARENT, XINE_OSD_TEXT1);
}


void
JAMStreamPlayerXine::DestSizeCallback(void* p, int /*video_width*/, int /*video_height*/, double /*video_aspect*/,
                       int* dest_width, int* dest_height, double* dest_aspect)
{
    if (p == NULL) return;
    JAMStreamPlayerXine* vw = (JAMStreamPlayerXine*) p;

    *dest_width = vw->width();
    *dest_height = vw->height();
    *dest_aspect = vw->pixel_aspect;
}


void
JAMStreamPlayerXine::FrameOutputCallback(void* p, int video_width, int video_height, double video_aspect,
                          int* dest_x, int* dest_y, int* dest_width, int* dest_height,
                          double* dest_aspect, int* win_x, int* win_y)

{
    if (p == NULL) return;
    JAMStreamPlayerXine* vw = (JAMStreamPlayerXine*) p;

    *dest_x = 0;
    *dest_y = 0 ;
    *dest_width = vw->width();
    *dest_height = vw->height();
    *dest_aspect = vw->pixel_aspect;
    *win_x = vw->globX;
    *win_y = vw->globY;
}


void
JAMStreamPlayerXine::play(QString mrl)
{
    std::cout << "JAM: Opening xine stream.\n";
    xine_open(xineStream, mrl);
    std::cout << "JAM: Playing xine stream...\n";
    xine_play(xineStream, 0, 0);
}

void
JAMStreamPlayerXine::stop()
{
    std::cout << "JAM: stopping xine stream.\n";
    xine_stop(xineStream);
    std::cout << "JAM: closing xine stream...\n";
    xine_close(xineStream);
}


QString
JAMStreamPlayerXine::tvMRL(QString channelId)
{
    return QString("http://tristan:3000/PES/" + channelId + "#demux:mpeg_pes");
}
