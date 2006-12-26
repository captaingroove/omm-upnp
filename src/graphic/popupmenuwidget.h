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
#ifndef POPUPMENUWIDGET_H
#define POPUPMENUWIDGET_H

#include <string>
using namespace std;


/**
Interface for toolkit specific implementation of PopupMenuWidget.

	@author Jörg Bakker <joerg@hakker.de>
*/
class PopupMenuWidget
{
public:
    //virtual ~PopupMenuWidget();
    // TODO: for now, we need a synchronous implementation of a popup menu,
    //       popup() returns with the name of the selected item.
    virtual void popup() = 0;
    virtual void insertItem(string text) = 0;
};

#endif
