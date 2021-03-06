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

#ifndef ViewImpl_INCLUDED
#define ViewImpl_INCLUDED

#include "Gui/View.h"
#include "Gui/Controller.h"

namespace Omm {
namespace Gui {

class Drag;

class ViewImpl
{
    friend class View;

public:
    virtual ~ViewImpl();

    virtual void init() {}
    View* getView();
    void* getNativeView();
#ifdef __WINDOWS__
    void* getNativeWindowId();
#else
    uint32_t getNativeWindowId();
#endif
    void setNativeView(void* pView);
    virtual void setParent(View* pView);
    virtual void showView(bool async);
    virtual void hideView(bool async);
    virtual void raise(bool async);
    bool isVisible();
    virtual int posXView();
    virtual int posYView();
    virtual int widthView();
    virtual int heightView();
    virtual void resizeView(int width, int height);
    virtual void setSizeConstraint(int width, int height, View::SizeConstraint size);
    virtual float getFontSize();
    virtual void setFontSize(float fontSize);
    virtual void moveView(int x, int y);
    virtual void setHighlighted(bool highlighted);
    virtual void setBackgroundColor(const Color& color);
    bool getEnableRedraw();
    void setEnableRedraw(bool enable);
    void setAcceptDrops(bool accept);
    int getDragMode();
    void setEnableHover(bool enable);

protected:
    void initViewImpl(View* pView, void* pNative);
    void triggerViewSync();

    View*                       _pView;
    void*                       _pNativeView;

private:
    void shown();
    void hidden();
    void resized(int width, int height);
    void selected();
    void released();
    void activated();
//    void keyPressed(int key);
    bool keyPressed(int key, int modifiers);
    Controller::KeyCode keyTranslateNativeCode(int key);
    Controller::Modifiers keyTranslateNativeModifiers(int modifiers);
    void mouseHovered(const Position& pos);
    void mouseMoved(const Position& pos);
    void dragStarted();
    void dragEntered(const Position& pos, Drag* pDrag, bool& accept);
    void dragMoved(const Position& pos, Drag* pDrag, bool& accept);
    void dragLeft();
    void dropped(const Position& pos, Drag* pDrag, bool& accept);
};


class PlainViewImpl : public ViewImpl
{
public:
    PlainViewImpl(View* pView);
};


}  // namespace Omm
}  // namespace Gui

#endif

