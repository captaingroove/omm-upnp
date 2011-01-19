/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2009, 2010                                                 |
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

#ifndef UpnpAvServerPrivate_INCLUDED
#define UpnpAvServerPrivate_INCLUDED

#include "UpnpAvServer.h"


namespace Omm {
namespace Av {


class TorchItemProperty : public Omm::Av::AbstractProperty
{
public:
    TorchItemProperty();
};


class TorchItemPropertyImpl : public Omm::Av::PropertyImpl
{
public:
    virtual void setName(const std::string& name);
    virtual void setValue(const std::string& value);
    virtual std::string getName();
    virtual std::string getValue();
    
private:
    std::string         _name;
    std::string         _value;
};


class TorchItemResource : public Omm::Av::StreamingResource
{
public:
    TorchItemResource(TorchServer* pServer, Omm::Av::AbstractMediaObject* pItem);
    
    virtual Omm::ui4 getSize();
    virtual std::string getMime();
    virtual std::string getDlna();
    
    virtual bool isSeekable();
    virtual std::streamsize stream(std::ostream& ostr, std::iostream::pos_type seek);
};


class TorchItem : public Omm::Av::StreamingMediaItem
{
    friend class TorchServer;
    
public:
    TorchItem(TorchServer* pServer);
    virtual ~TorchItem();
    
    virtual int getPropertyCount(const std::string& name = "");
    virtual Omm::Av::AbstractProperty* getProperty(int index);
    virtual Omm::Av::AbstractProperty* getProperty(const std::string& name, int index = 0);

private:
    TorchItemProperty*                   _pClassProp;
    TorchItemProperty*                   _pTitleProp;
    TorchItemResource*                   _pResource;
};

} // namespace Av
} // namespace Omm

#endif
