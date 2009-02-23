/***************************************************************************
 *   Copyright (C) 2009 by Jörg Bakker   				   *
 *   joerg<at>hakker<dot>de   						   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 (not   *
 *   v2.2 or v3.x or other) as published by the Free Software Foundation.  *
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
#include "signode.h"

using namespace Jamm;

JSignal::JSignal()
{
}


void
JSignal::connectNodes(JSignal* sender, JSlot* receiver)
{
//     sender->registerReceiver(receiver);
    sender->m_receiverList.push_back(receiver);
}


void
JSignal::disconnectNodes(JSignal* /*sender*/, JSlot* /*receiver*/)
{
    //TODO: implement deleting of receivers
}


void
JSignal::emitSignal()
{
    for (vector<JSlot*>::iterator i = m_receiverList.begin(); i != m_receiverList.end() ;++i) {
        (*i)->onSignalReceived();
    }
}