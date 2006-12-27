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
#ifndef LISTBROWSER_H
#define LISTBROWSER_H

#include "page.h"
#include "title.h"
#include "list.h"
#include "popupmenu.h"
#include "event.h"
#include "listbrowserwidget.h"

#include <string>
using namespace std;

/**
Page for browsing lists of timers, recordings, files, channels, ...

	@author Jörg Bakker <joerg@hakker.de>
*/


class ListBrowser : public List, public Page
{
public:
    ListBrowser(string name, string cols, List *list);
    ListBrowser()                               {}
    ~ListBrowser();

    int cols()                                  { return m_cols.size(); }
    string colText(int i)                       { return m_cols[i]; }
    void setPopupMenu(PopupMenu *popupMenu);
    PopupMenu* getPopupMenu()                   { return m_popupMenu; }
    virtual void enterPage();

    virtual void addTitle(Title *title);
    virtual void delTitle(Title *title);
    virtual void clear();
    Title *getCurrent()                         { return m_listBrowserWidget->getCurrent(); }
    int getPosition(Title *title)               { return m_listBrowserWidget->getPosition(title); }
    void selectEntry(int number)                { return m_listBrowserWidget->selectEntry(number); }
    void selectEntry(Title *title)              { return m_listBrowserWidget->selectEntry(title); }
    virtual bool eventHandler(Event *e);

protected:
    virtual void addViewColumn(string colName, int colWidth)  { m_listBrowserWidget->addViewColumn(colName, colWidth); }

    static string       m_colSeperator;
    static string       m_colFormatSeperator;
    static string       m_colLabelSeperator;
    List               *m_list;
    vector<string>      m_cols;
    PopupMenu          *m_popupMenu;
    ListBrowserWidget  *m_listBrowserWidget;
};

#endif
