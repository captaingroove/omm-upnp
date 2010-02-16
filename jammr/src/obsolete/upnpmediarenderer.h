/***************************************************************************|
|  Jamm - Just another multimedia ...                                       |
|         ... set of applications and libraries based on the UPnP-AV specs  |
|                                                                           |
|  Copyright (C) 2009                                                       |
|  Jörg Bakker (joerg'at'hakker'dot'de)                                     |
|                                                                           |
|  This file is part of Jamm.                                               |
|                                                                           |
|  Jamm is free software: you can redistribute it and/or modify             |
|  it under the terms of the GNU General Public License as published by     |
|  the Free Software Foundation version 3 of the License.                   |
|                                                                           |
|  Jamm is distributed in the hope that it will be useful,                  |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of           |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            |
|  GNU General Public License for more details.                             |
|                                                                           |
|  You should have received a copy of the GNU General Public License        |
|  along with this program.  If not, see <http://www.gnu.org/licenses/>.    |
 ***************************************************************************/

#ifndef UPNPMEDIARENDERER_H
#define UPNPMEDIARENDERER_H

// #include "enginemplayer.h"
#include "engine.h"

#include <platinum/PltMediaRenderer.h>
#include <jamm/thread.h>
#include <jamm/signode.h>

using namespace Jamm;

class UpnpMediaRenderer : public PLT_MediaRenderer
{
public:
    UpnpMediaRenderer(Engine*              engine,
                      const char*          friendly_name,
                      bool                 show_ip = false,
                      const char*          uuid = NULL,
                      unsigned int         port = 0);

    ~UpnpMediaRenderer();
    
    class PollSlot : public JSlot {
    public:
        PollSlot(void* slotUser) { s = static_cast<UpnpMediaRenderer*>(slotUser); }
        virtual void onSignalReceived();
    private:
        UpnpMediaRenderer* s;
    };
    friend class PollSlot;
    
    class EndOfTrackSlot : public JSlot {
    public:
        EndOfTrackSlot(void* slotUser) { s = static_cast<UpnpMediaRenderer*>(slotUser); }
        virtual void onSignalReceived();
    private:
        UpnpMediaRenderer* s;
    };
    friend class EndOfTrackSlot;
    
    
protected:
    // ConnectionManager
//     virtual NPT_Result OnGetCurrentConnectionInfo(PLT_ActionReference& action);
    
    // AVTransport
    virtual NPT_Result OnSetAVTransportURI(PLT_ActionReference& action);
    virtual NPT_Result OnPlay(PLT_ActionReference& action);
    virtual NPT_Result OnStop(PLT_ActionReference& action);
    virtual NPT_Result OnPause(PLT_ActionReference& action);
    virtual NPT_Result OnSeek(PLT_ActionReference& action);
    virtual NPT_Result OnNext(PLT_ActionReference& action);
    virtual NPT_Result OnPrevious(PLT_ActionReference& action);
//     virtual NPT_Result OnSetPlayMode(PLT_ActionReference& action);
    
    // RenderingControl
//     virtual NPT_Result OnGetVolume(PLT_ActionReference& action);
    virtual NPT_Result OnSetVolume(PLT_ActionReference& action);
    virtual NPT_Result OnSetMute(PLT_ActionReference& action);

    // TODO: add required missing actions:
    //       OnGetMediaInfo (?)
    //       OnGetTransportInfo (?)
    //       OnGetPositionInfo (?) -> already implemented in PLT_MediaController
    //       OnGetDeviceCapabilities (?)
    //       OnGetTransportSettings (?)
    
    // TODO: add optional missing actions:
    //       OnSetNextAVTransportURI
    //       OnRecord
    //       OnSetPlayMode
    //       OnSetRecordQualityMode
    //       OnGetCurrentTransportActions
    
    // TODO: lock all actions
    
private:
    Engine*         m_engine;
    PLT_Service*    m_AvTransport;
    PLT_Service*    m_AvRenderingControl;
    
    NPT_String      m_lastCurrentTrackUri;
    JTimer          m_pollPositionTimer;
};

#endif