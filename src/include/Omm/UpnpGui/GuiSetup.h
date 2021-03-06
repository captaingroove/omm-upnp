/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2013                                                       |
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

#ifndef GuiSetup_INCLUDED
#define GuiSetup_INCLUDED

#include "../Gui/Navigator.h"
#include "../Gui/Selector.h"
#include "../Gui/List.h"
#include "../Gui/ScrollArea.h"

namespace Omm {

class ServerListModel;
class UpnpApplication;
class ControllerWidget;


class GuiSetup : public Gui::NavigatorView
{
    friend class RendererItemController;
    friend class RendererDoneButton;
    friend class RendererConfView;
    friend class ServerItemController;
    friend class ServerListController;
    friend class ServerListModel;
    friend class ServerDoneButton;
    friend class ServerNewButton;
    friend class ServerConfView;
    friend class ServerConfModel;

public:
    GuiSetup(UpnpApplication* pApp, Gui::View* pParent = 0);
    virtual ~GuiSetup();

private:
//    void writeConf();

    UpnpApplication*    _pApp;

    Gui::ScrollAreaView*    _pSetupArea;
    Gui::View*              _pSetupView;
    Gui::Selector*          _pAppStateSelector;
    Gui::View*              _pRendererConfig;
    Gui::ListItemModel*     _pRendererItemModel;
    Gui::ListItemView*      _pRendererItem;
    Gui::ListItemView*      _pServerItem;
    ServerListModel*        _pServerListModel;
    Gui::ListView*          _pServerList;
};


}  // namespace Omm


#endif
