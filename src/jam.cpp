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


#include <qapplication.h>
#include <qlayout.h>
#include "jam.h"

JAM::JAM()
    : QWidget(0, "JAM")
{

    QHBoxLayout *l = new QHBoxLayout(this);
    l->setAutoAdd(TRUE);

    m_screen = new QWidgetStack(this);
    m_keyh = new GlobalKeyHandler(this);
    installEventFilter(m_keyh);

    m_menu = new MenuMain(this, m_keyh, m_screen);
    m_screen->addWidget(m_menu);

    m_tv = new JAMTV(m_keyh, m_screen);
    registerMenu(m_tv, true);

    m_proGuide = new MenuProGuide(m_tv, m_keyh, m_screen);
    registerMenu(m_proGuide);

    resize(720, 576);

    showMainMenu();
}


JAM::~JAM()
{
//    delete m_screen;
//    delete m_menu;
//    delete streamplayer;
//    delete tv;
}


void
JAM::registerMenu(Menu* menu, bool isDefaultMenu)
{
    m_screen->addWidget(menu);
    m_menu->registerMenuItem(menu, isDefaultMenu);
}


void
JAM::showMainMenu()
{
    showMenu(m_menu);
}


void
JAM::showMenu(Menu *m)
{
    if (m != 0) // some menu entries don't have real menu screens (for example "quit")
    {
        m_screen->raiseWidget(m);
        m->action();
        m->selectDefault();
    }
}

