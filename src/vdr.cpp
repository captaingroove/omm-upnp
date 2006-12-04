/***************************************************************************
 *   Copyright (C) 2006 by Jörg Bakker   				   *
 *   joerg@hakker.de   							   *
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
#include "vdr.h"
#include "vdrrequest.h"


Vdr::Vdr()
 : ListManager()
{
}


Vdr::~Vdr()
{
}


void
Vdr::fill(List *list, Title::TitleT type)
{
    QString vdrRequest = "";

    switch (type) {
    case Title::TvTimerT:
        vdrRequest = "LSTT";
        break;
    case Title::TvChannelT:
        vdrRequest = "LSTC";
        break;
    case Title::TvProgramT:
        vdrRequest = "LSTE";
        break;
    case Title::TvRecT:
        vdrRequest = "LSTR";
        break;
    default:
        break;
    }
    qDebug("Vdr::fill() starting request: %s", vdrRequest.latin1());

    VdrRequest request(this, list, vdrRequest);
    request.startRequest();
    // TODO: check the answer of the backend.
}


void
Vdr::addProxyTitle(Title *title)
{
    QString vdrRequest = "";

    switch (title->getType()) {
    case Title::TvTimerT: {
        QString active = title->getText("Active");
        QString channel = title->getText("Channel");
        QString day = title->getText("Day");
        QString start = title->getText("Time Start");
        QString end = title->getText("Time End");
        QString prio = title->getText("Priority");
        QString resist = title->getText("Resistance");
        QString name = title->getText("Name");
        vdrRequest = "NEWT " + active + ":" + channel + ":" + day + ":" + 
                    start + ":" + end + ":" + prio + ":" + resist + ":" + name + ":";
        break;
        }
    default:
        break;
    }
    qDebug("Vdr::addProxyTitle() starting request: %s", vdrRequest.latin1());

    VdrRequest request(this, 0, vdrRequest);
    request.startRequest();
    // TODO: check the answer of the backend.
}


void
Vdr::delProxyTitle(Title *title)
{
    QString vdrRequest = "";

    switch (title->getType()) {
    case Title::TvTimerT:
        // TODO: check again, if Id hasn't changed and do this check and deletion in one request.
        vdrRequest = "DELT " + title->getText("Id");
        // TODO: update ListBrowser to reflect the changes (emit signal, that ListProxy has changed).
        break;
    case Title::TvRecT:
        // TODO: delete selected recording.
        break;
    default:
        break;
    }
    qDebug("Vdr::delProxyTitle() starting request: %s", vdrRequest.latin1());

    VdrRequest request(this, 0, vdrRequest);
    request.startRequest();
   // TODO: check the answer of the backend.
}
