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

#include "menu.h"
#include "controler.h"
#include "globalkeyhandler.h"


Menu::Menu(QString name)
 : Page(name)
{
    m_list = new QListView(this);
    // install global event filter on m_list
    m_list->installEventFilter(GlobalKeyHandler::instance());
    m_list->addColumn(m_name);
    m_entryNumber = 0;

    connect(m_list, SIGNAL(returnPressed(QListViewItem*)), this, SLOT(selectEntry(QListViewItem*)));
}


Menu::~Menu()
{
}


void
Menu::addEntry(Page* page)
{
    m_entryNumber++;
    QString entryNumberStr = QString().setNum(m_entryNumber).rightJustify(2, '0') + ".  ";
    QListViewItem *i = new QListViewItem(m_list, entryNumberStr + page->getName(), "");
    m_entryDict.insert(i, page);

    // make the first entry the default selection.
    if (m_entryDict.count() == 1) {
        m_defaultEntry = i;
    }
}


void
Menu::setDefaultEntry(Page* page)
{
    qDebug("Menu::setDefaultEntry() for page: %p", page);
    // Search for page in QPtrList and set corresponding QListViewItem as m_defaultItem.
    // TODO: implement findItem()
    //m_defaultItem = findItem(page);
}


void
Menu::enterPage()
{
    qDebug("Menu::enterPage(), set default entry to: %p", m_defaultEntry);
    m_list->setCurrentItem(m_defaultEntry);
    m_list->setSelected(m_defaultEntry, true);
    m_list->setFocus();
}


void
Menu::selectEntry(QListViewItem* i)
{
    qDebug("Menu::selectEntry()");
    Page *s = m_entryDict[i];
    s->showUp();
}


QListViewItem*
Menu::findEntry(Page *page)
{
    qDebug("Menu::findEntry() with page: %p", page);
    // TODO: implement findItem()
/*
    QPtrDictIterator<Page> i(m_itemDict);
    for( ; i.current(); ++i ) {
        if (i.current() == page) {
            return i;
        }
    }
*/
    return 0;
}
