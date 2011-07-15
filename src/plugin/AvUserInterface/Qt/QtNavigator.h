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

#ifndef QtBrowser_INCLUDED
#define QtBrowser_INCLUDED

#include <stack>
#include <QtGui>

class QtNavigable
{
public:
    virtual QString getBrowserTitle() { return ""; }
    virtual QWidget* getWidget() { return 0; }
    /// If getWidget() returns not null but a valid widget, the widget
    /// is pushed on QtNavigator::_pStackedWidget.
    virtual void show() {}
    /// Additionally, show() can be implemented if for example no widget
    /// is pushed but some other action is necessary to show the correct view.
};


class QtNavigatorPanelButton : public QPushButton
{
    Q_OBJECT

public:
    QtNavigatorPanelButton(QtNavigable* pNavigable);

    QtNavigable* getNavigable();

private:
    QtNavigable*    _pNavigable;
};


class QtNavigatorPanel : public QWidget
{
    Q_OBJECT

public:
    QtNavigatorPanel(QWidget* pParent = 0);

    void push(QtNavigable* pNavigable);
    void pop(QtNavigable* pNavigable);

signals:
    void selectedNavigable(QtNavigable* pNavigable);

private slots:
    void buttonPushed();

private:
    std::stack<QtNavigatorPanelButton*>     _buttonStack;
    QHBoxLayout*                            _pButtonLayout;
    QSignalMapper*                          _pSignalMapper;
};


class QtNavigator : public QWidget
{
    Q_OBJECT

public:
    QtNavigator(QWidget* pParent = 0);
    ~QtNavigator();
    
    void push(QtNavigable* pNavigable);

private slots:
    void expose(QtNavigable* pNavigable);

private:
    QtNavigatorPanel*           _pNavigatorPanel;
    QStackedWidget*             _pStackedWidget;
    QVBoxLayout*                _pNavigatorLayout;
    std::stack<QtNavigable*>    _navigableStack;
};





#endif

