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

#import <UIKit/UIKit.h>

#include <Poco/NumberFormatter.h>

#include "ViewImpl.h"
#include "Gui/View.h"
#include "Gui/GuiLogger.h"

namespace Omm {
namespace Gui {


ViewImpl::~ViewImpl()
{
    Omm::Gui::Log::instance()->gui().debug("view impl dtor");
//    delete _pNativeView;
}


void
ViewImpl::init(View* pView, void* pNative)
{
    Omm::Gui::Log::instance()->gui().debug("init view impl view: " + Poco::NumberFormatter::format(pView) + ", native: " + Poco::NumberFormatter::format(pNative));

    _pView = pView;

    UIView* pNativeView;
    UIViewController* pNativeViewController;
    if ([static_cast<NSObject*>(pNative) isKindOfClass:[UIView class]]) {
        Omm::Gui::Log::instance()->gui().debug("init view impl native is of type UIView");
        pNativeView = static_cast<UIView*>(pNative);
        pNativeViewController = [[UIViewController alloc] init];
        pNativeViewController.view = pNativeView;
        _pNativeView = pNative;
        _pNativeViewController = pNativeViewController;
    }
    else if ([static_cast<NSObject*>(pNative) isKindOfClass:[UIViewController class]]) {
        Omm::Gui::Log::instance()->gui().debug("init view impl native is of type UIViewController");
        pNativeViewController = static_cast<UIViewController*>(pNative);
        _pNativeView = pNativeViewController.view;
        _pNativeViewController = pNative;
    }
//    _pNativeView = pNativeViewController.view;
//    _pNativeViewController = pNativeViewController;

    if (pView->getParent()) {
        UIView* pParentView = static_cast<UIView*>(pView->getParent()->getNativeView());
        [pParentView addSubview:pNativeView];
    }
}


View*
ViewImpl::getView()
{
    Omm::Gui::Log::instance()->gui().debug("view impl get view: " + Poco::NumberFormatter::format(_pView));
    return _pView;
}


void*
ViewImpl::getNativeView()
{
    Omm::Gui::Log::instance()->gui().debug("view impl get native view: " + Poco::NumberFormatter::format(_pNativeView));

    return _pNativeView;
}


void*
ViewImpl::getNativeViewController()
{
    Omm::Gui::Log::instance()->gui().debug("view impl get native view controller: " + Poco::NumberFormatter::format(_pNativeViewController));

    return _pNativeViewController;
}


void
ViewImpl::setNativeView(void* pView)
{
    Omm::Gui::Log::instance()->gui().debug("view impl set native view: " + Poco::NumberFormatter::format(pView));
    _pNativeView = pView;
}


void
ViewImpl::showView()
{
    Omm::Gui::Log::instance()->gui().debug("view impl show _pNativeView: " + Poco::NumberFormatter::format(_pNativeView) + " ...");
//    static_cast<UIView*>(_pNativeView).hidden = NO;
    Omm::Gui::Log::instance()->gui().debug("view impl show finished.");
}


void
ViewImpl::hideView()
{
    Omm::Gui::Log::instance()->gui().debug("view impl hide _pNativeView: " + Poco::NumberFormatter::format(_pNativeView) + " ...");
//    static_cast<UIView*>(_pNativeView).hidden = YES;
    Omm::Gui::Log::instance()->gui().debug("view impl hide finished.");
}


int
ViewImpl::widthView()
{
    Omm::Gui::Log::instance()->gui().debug("view impl width.");

    return static_cast<UIView*>(getNativeView()).frame.size.width;
}


int
ViewImpl::heightView()
{
    Omm::Gui::Log::instance()->gui().debug("view impl height.");
    return static_cast<UIView*>(getNativeView()).frame.size.height;
}


void
ViewImpl::resizeView(int width, int height)
{
    Omm::Gui::Log::instance()->gui().debug("view impl resize.");
    CGRect frame = static_cast<UIView*>(getNativeView()).frame;
    frame.size.width = width;
    frame.size.height = height;
    static_cast<UIView*>(getNativeView()).frame = frame;
}


void
ViewImpl::moveView(int x, int y)
{
    Omm::Gui::Log::instance()->gui().debug("view impl move.");
    CGRect frame = static_cast<UIView*>(getNativeView()).frame;
    frame.origin.x = x;
    frame.origin.y = y;
    static_cast<UIView*>(getNativeView()).frame = frame;
}


void
ViewImpl::selected()
{
    Omm::Gui::Log::instance()->gui().debug("view impl selected event.");
    IMPL_NOTIFY_CONTROLLER(Controller, selected);
}


PlainViewImpl::PlainViewImpl(View* pView)
{
    Omm::Gui::Log::instance()->gui().debug("plain view impl ctor.");
    UIView* pNativeView = [[UIView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    pNativeView.backgroundColor = [UIColor grayColor];

    init(pView, pNativeView);
}


//void
//NativeView::mousePressEvent(QMouseEvent* pMouseEvent)
//{
//    _pViewImpl->selected();
//    UIView::mousePressEvent(pMouseEvent);
//}


}  // namespace Omm
}  // namespace Gui
