/***************************************************************************|
|  Omm - Just another multimedia ...                                       |
|         ... set of applications and libraries based on the UPnP-AV specs  |
|                                                                           |
|  Copyright (C) 2009                                                       |
|  Jörg Bakker (joerg'at'hakker'dot'de)                                     |
|                                                                           |
|  This file is part of Omm.                                               |
|                                                                           |
|  Omm is free software: you can redistribute it and/or modify             |
|  it under the terms of the GNU General Public License as published by     |
|  the Free Software Foundation version 3 of the License.                   |
|                                                                           |
|  Omm is distributed in the hope that it will be useful,                  |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of           |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            |
|  GNU General Public License for more details.                             |
|                                                                           |
|  You should have received a copy of the GNU General Public License        |
|  along with this program.  If not, see <http://www.gnu.org/licenses/>.    |
***************************************************************************/

/***************************************************************************|
|                               WARNING                                     |
|        This file is generated by the omm stub generator ommgen.         |
|       Don't edit, it will be overriden at the next run of ommgen.        |
***************************************************************************/

#ifndef DIMMABLELIGHT_CTRL_H
#define DIMMABLELIGHT_CTRL_H

#include <Omm/Upnp.h>

class DimmingController
{
    friend class DimmableLightController;

public:
    void SetLoadLevelTarget(const Omm::ui1& newLoadlevelTarget);
    void GetLoadLevelTarget(Omm::ui1& retLoadlevelTarget);
    void GetLoadLevelStatus(Omm::ui1& retLoadlevelStatus);

    void _reqSetLoadLevelTarget(const Omm::ui1& newLoadlevelTarget);
    void _reqGetLoadLevelTarget();
    void _reqGetLoadLevelStatus();

    Omm::ui1 _getLoadLevelStatus();

protected:
    virtual void _ansSetLoadLevelTarget(const Omm::ui1& newLoadlevelTarget) = 0;
    virtual void _ansGetLoadLevelTarget(const Omm::ui1& retLoadlevelTarget) = 0;
    virtual void _ansGetLoadLevelStatus(const Omm::ui1& retLoadlevelStatus) = 0;

    virtual void _changedLoadLevelStatus(const Omm::ui1& val) = 0;

private:
    void _threadSetLoadLevelTarget(Omm::Action* pAction);
    void _threadGetLoadLevelTarget(Omm::Action* pAction);
    void _threadGetLoadLevelStatus(Omm::Action* pAction);

    Omm::Service* _pService;
};

class SwitchPowerController
{
    friend class DimmableLightController;

public:
    void SetTarget(const bool& NewTargetValue);
    void GetTarget(bool& RetTargetValue);
    void GetStatus(bool& ResultStatus);

    void _reqSetTarget(const bool& NewTargetValue);
    void _reqGetTarget();
    void _reqGetStatus();

    bool _getStatus();

protected:
    virtual void _ansSetTarget(const bool& NewTargetValue) = 0;
    virtual void _ansGetTarget(const bool& RetTargetValue) = 0;
    virtual void _ansGetStatus(const bool& ResultStatus) = 0;

    virtual void _changedStatus(const bool& val) = 0;

private:
    void _threadSetTarget(Omm::Action* pAction);
    void _threadGetTarget(Omm::Action* pAction);
    void _threadGetStatus(Omm::Action* pAction);

    Omm::Service* _pService;
};


class DimmableLightController : public Omm::ControllerImplAdapter
{
public:
    DimmableLightController(Omm::Device* pDevice, SwitchPowerController* pSwitchPowerController, DimmingController* pDimmingController);

    SwitchPowerController* SwitchPower() { return _pSwitchPowerController; }
    DimmingController* Dimming() { return _pDimmingController; }

private:
    virtual void eventHandler(Omm::StateVar* pStateVar);

    Omm::Device* _pDevice;
    SwitchPowerController* _pSwitchPowerController;
    DimmingController* _pDimmingController;
};

#endif

