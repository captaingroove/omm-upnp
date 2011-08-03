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

#ifndef QtWidgetList_INCLUDED
#define QtWidgetList_INCLUDED

#include <stack>
#include <QtGui>

class QtWidgetListModel;
class QtWidgetListView;

class QtWidgetListModel
{
    friend class QtWidgetListView;
    
public:
    QtWidgetListModel();

    // item related
    virtual int totalItemCount() { return 0; }
    void insertItem(int row);
    /// Ask the view to show an item at row. No data is created (cached), as
    /// the whole data of the model is already present in the underlying implementation.
    /// The model is only an abstraction layer on top of the data.
    void removeItem(int row);
    /// See insertItem().

    // widget related
    virtual QWidget* createWidget() { return 0; }
    virtual QWidget* getWidget(int row) { return 0; }
    virtual void attachWidget(int row, QWidget* pWidget) {}
    virtual void detachWidget(int row) {}

private:
    QtWidgetListView*               _pView;
};


class QtWidgetListView
{
public:
    QtWidgetListView();
    
    void setModel(QtWidgetListModel* pModel);
    void insertItem(int row);
    void removeItem(int row);

protected:
    int widgetPoolSize();
    void extendWidgetPool(int n);
    /// The view has a widget pool which is large enough to fill the area of the view
    /// with widgets (created by the model).
    int visibleIndex(int row);
    int countVisibleWidgets();
    QWidget* visibleWidget(int index);

    virtual bool itemIsVisible(int row) { return false; }
    virtual void createProxyWidget(QWidget* pWidget) {}
    virtual void showItemWidget(int row, QWidget* pWidget) {}
    virtual void hideItemWidget(int row, QWidget* pWidget) {}

private:
    QtWidgetListModel*              _pModel;

    std::vector<QWidget*>           _widgetPool;
    std::vector<QWidget*>           _visibleWidgets;
    std::stack<QWidget*>            _freeWidgets;
};


class QtWidgetList : public QGraphicsView, public QtWidgetListView
{
    Q_OBJECT

public:
    QtWidgetList(QWidget* pParent = 0);
    virtual ~QtWidgetList();

protected:
    virtual bool itemIsVisible(int row);
    virtual void createProxyWidget(QWidget* pWidget);
    virtual void showItemWidget(int row, QWidget* pWidget);
    virtual void hideItemWidget(int row, QWidget* pWidget);

signals:
    void selectedWidget(int row);
//    void showWidget(QWidget* pWidget);
//    void hideWidget(QWidget* pWidget);
    void moveWidget(int targetRow, QWidget* pWidget);

private slots:
    void show(QWidget* pWidget);
    void hide(QWidget* pWidget);
    void move(int targetRow, QWidget* pWidget);

private:
    QGraphicsScene*                                 _pGraphicsScene;
    std::map<QWidget*, QGraphicsProxyWidget*>       _proxyWidgetPool;
    int                                             _widgetHeight;
};


#endif
