/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2011                                                       |
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

#ifndef QtDeviceGroup_INCLUDED
#define QtDeviceGroup_INCLUDED

#include <QtGui>
#include <Omm/Upnp.h>

class QtCrumbButton;
class QtCrumbPanel;
class QtBrowserModel;
class QtListItem;


class QtDeviceGroup : public QWidget, public Omm::DeviceGroupInterface
{
    Q_OBJECT
public:
    QtDeviceGroup();
    
private:

    QVBoxLayout*                    _pLayout;
//    QtBrowserModel*                 _pBrowserModel;
    QtCrumbPanel*                   _pCrumbPanel;
    QtCrumbButton*                  _pCrumbButton;
    QTreeView*                      _pBrowserView;
    QtListItem*                     _pListItem;
};

#endif

