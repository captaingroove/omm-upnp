/***************************************************************************|
|  Jamm - Just another multimedia ...                                       |
|         ... set of applications and libraries based on the UPnP-AV specs  |
|                                                                           |
|  Copyright (C) 2009                                                       |
|  Jörg Bakker (joerg'at'hakker'dot'de)                                     |
|                                                                           |
|  This file is part of Jamm.                                               |
|                                                                           |
|  Jamm is free software: you can redistribute it and/or modify             |
|  it under the terms of the GNU General Public License as published by     |
|  the Free Software Foundation version 3 of the License.                   |
|                                                                           |
|  Jamm is distributed in the hope that it will be useful,                  |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of           |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            |
|  GNU General Public License for more details.                             |
|                                                                           |
|  You should have received a copy of the GNU General Public License        |
|  along with this program.  If not, see <http://www.gnu.org/licenses/>.    |
***************************************************************************/

/***************************************************************************|
|                               WARNING                                     |
|        This file is generated by the jamm stub generator jammgen.         |
|       Don't edit, it will be overriden at the next run of jammgen.        |
***************************************************************************/

#ifndef DIMMABLELIGHT_H
#define DIMMABLELIGHT_H

#include <Jamm/Upnp.h>

using Jamm::DeviceRootImplAdapter;
using Jamm::Service;
using Jamm::Action;

class DimmableLight;

class Dimming
{
    friend class DimmableLight;

protected:
    virtual void SetLoadLevelTarget(const Jamm::ui1& newLoadlevelTarget) = 0;
    virtual void GetLoadLevelTarget(Jamm::ui1& retLoadlevelTarget) = 0;
    virtual void GetLoadLevelStatus(Jamm::ui1& retLoadlevelStatus) = 0;

    virtual void initStateVars() = 0;

    void _setLoadLevelTarget(const Jamm::ui1& val);
    Jamm::ui1 _getLoadLevelTarget();
    void _setLoadLevelStatus(const Jamm::ui1& val);
    Jamm::ui1 _getLoadLevelStatus();

private:
    static std::string  m_description;
    Service* m_pService;
};


class SwitchPower
{
    friend class DimmableLight;

protected:
    virtual void SetTarget(const bool& NewTargetValue) = 0;
    virtual void GetTarget(bool& RetTargetValue) = 0;
    virtual void GetStatus(bool& ResultStatus) = 0;

    virtual void initStateVars() = 0;

    void _setTarget(const bool& val);
    bool _getTarget();
    void _setStatus(const bool& val);
    bool _getStatus();

private:
    static std::string  m_description;
    Service* m_pService;
};


class DimmableLight : public DeviceRootImplAdapter
{
public:
    DimmableLight(SwitchPower* pSwitchPowerImpl, Dimming* pDimmingImpl);


private:
    virtual void actionHandler(Action* action);
    virtual void initStateVars(const std::string& serviceType, Service* pThis);

    static std::string m_deviceDescription;
    SwitchPower* m_pSwitchPowerImpl;
    Dimming* m_pDimmingImpl;
};

#endif

