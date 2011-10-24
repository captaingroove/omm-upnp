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

#include "ImageImpl.h"
#include "Gui/Image.h"
#include "Gui/GuiLogger.h"


@interface OmmGuiImage : UIImage
{
    Omm::Gui::ImageViewImpl* _pImageViewImpl;
}

@end


@implementation OmmGuiImage

- (void)setImpl:(Omm::Gui::ImageViewImpl*)pImpl
{
    _pImageViewImpl = pImpl;
}


//- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
//{
////    Omm::Gui::Log::instance()->gui().debug("Label view impl touch began");
//    _pLabelViewImpl->pushed();
//    [super touchesBegan:touches withEvent:event];
//}

@end


namespace Omm {
namespace Gui {


ImageViewImpl::ImageViewImpl(View* pView)
{
//    Omm::Gui::Log::instance()->gui().debug("image view impl ctor");
    OmmGuiImage* pNativeView = [[OmmGuiImage alloc] init];
    [pNativeView setImpl:this];

    initViewImpl(pView, pNativeView);
}


ImageViewImpl::~ImageViewImpl()
{
}


void
ImageViewImpl::setData(const std::string& data)
{
    // FIXME: implement setData() with UIImage
//    QPixmap* pImage = static_cast<QPixmap*>(_pImage);
//    pImage->loadFromData((const uchar*)data.data(), data.size(), 0);
//    static_cast<QLabel*>(_pNativeView)->setPixmap(*pImage);
}


void
ImageViewImpl::setAlignment(View::Alignment alignment)
{
    // FIXME: implement setAlignment() with UIImage
    switch(alignment) {
        case View::AlignLeft:
//            static_cast<QLabel*>(_pNativeView)->setAlignment(Qt::AlignLeft);
            break;
        case View::AlignCenter:
//            static_cast<QLabel*>(_pNativeView)->setAlignment(Qt::AlignCenter);
            break;
        case View::AlignRight:
//            static_cast<QLabel*>(_pNativeView)->setAlignment(Qt::AlignRight);
            break;
    }
}


}  // namespace Omm
}  // namespace Gui