/***************************************************************************
 *   Copyright (C) 2006 by Jörg Bakker   				   *
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
#include "widgetfactory.h"
#include "qtpagestack.h"
#include "qtpage.h"
#include "qtmenu.h"
#include "qtlistbrowser.h"
#include "qtpopupmenu.h"

// TODO: this looks a bit like too much similar code, ... (templates somehow?)

WidgetFactory *WidgetFactory::m_instance = 0;

WidgetFactory::WidgetFactory()
{
}


WidgetFactory::~WidgetFactory()
{
}


WidgetFactory*
WidgetFactory::instance()
{
    if (m_instance == 0) {
        m_instance = new WidgetFactory();
    }
    return m_instance;
}


PageStack* 
WidgetFactory::createPageStack(PageStack *pageStackLogic)
{
    qDebug("WidgetFactory::createPageStack()");
    switch(m_toolkit) {
    case ToolkitQt:
        qDebug("WidgetFactory::createPageStack() for toolkit Qt");
        return new QtPageStack(pageStackLogic);
    default:
        return 0;
    }
}


Page* 
WidgetFactory::createPage(Page *pageLogic)
{
    qDebug("WidgetFactory::createPage()");
    switch(m_toolkit) {
    case ToolkitQt:
        qDebug("WidgetFactory::createPage() for toolkit Qt");
        return new QtPage(pageLogic);
    default:
        return 0;
    }
}


Menu* 
WidgetFactory::createMenu(Menu *menuLogic)
{
    qDebug("WidgetFactory::createMenu()");
    switch(m_toolkit) {
    case ToolkitQt:
        qDebug("WidgetFactory::createMenu() for toolkit Qt");
        return new QtMenu(menuLogic);
    default:
        return 0;
    }
}


ListBrowser* 
WidgetFactory::createListBrowser(ListBrowser *listBrowserLogic)
{
    qDebug("WidgetFactory::createListBrowser()");
    switch(m_toolkit) {
    case ToolkitQt:
        qDebug("WidgetFactory::createListBrowser() for toolkit Qt");
        return new QtListBrowser(listBrowserLogic);
    default:
        return 0;
    }
}


PopupMenu* 
WidgetFactory::createPopupMenu(PopupMenu *popupMenuLogic)
{
    qDebug("WidgetFactory::createPopupMenu()");
    switch(m_toolkit) {
    case ToolkitQt:
        qDebug("WidgetFactory::createPopupMenu() for toolkit Qt");
        return new QtPopupMenu(popupMenuLogic);
    default:
        return 0;
    }
}
