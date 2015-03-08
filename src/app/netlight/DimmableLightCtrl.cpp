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

/***************************************************************************|
|                               WARNING                                     |
|        This file is generated by the omm stub generator ommgen.           |
|       Don't edit, it will be overriden at the next run of ommgen.         |
****************************************************************************/

#include <Omm/UpnpInternal.h>

#include "DimmableLightCtrl.h"

void
DimmingController::SetLoadLevelTarget(const Omm::ui1& newLoadlevelTarget)
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("SetLoadLevelTarget")->clone();
    pAction->setArgument<Omm::ui1>("newLoadlevelTarget", newLoadlevelTarget);
    _pService->sendAction(pAction);
}

void
DimmingController::GetLoadLevelTarget(Omm::ui1& retLoadlevelTarget)
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("GetLoadLevelTarget")->clone();
    _pService->sendAction(pAction);
    retLoadlevelTarget = pAction->getArgument<Omm::ui1>("retLoadlevelTarget");
}

void
DimmingController::GetLoadLevelStatus(Omm::ui1& retLoadlevelStatus)
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("GetLoadLevelStatus")->clone();
    _pService->sendAction(pAction);
    retLoadlevelStatus = pAction->getArgument<Omm::ui1>("retLoadlevelStatus");
}

Omm::ui1
DimmingController::_getLoadLevelStatus()
{
    return _pService->getStateVar<Omm::ui1>("LoadLevelStatus");
}


void
DimmingController::_reqSetLoadLevelTarget(const Omm::ui1& newLoadlevelTarget)
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("SetLoadLevelTarget")->clone();
    pAction->setArgument<Omm::ui1>("newLoadlevelTarget", newLoadlevelTarget);
    Omm::ActionThread<DimmingController> thread(this, &DimmingController::_threadSetLoadLevelTarget, pAction);
    thread.start();
}

void
DimmingController::_reqGetLoadLevelTarget()
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("GetLoadLevelTarget")->clone();
    Omm::ActionThread<DimmingController> thread(this, &DimmingController::_threadGetLoadLevelTarget, pAction);
    thread.start();
}

void
DimmingController::_reqGetLoadLevelStatus()
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("GetLoadLevelStatus")->clone();
    Omm::ActionThread<DimmingController> thread(this, &DimmingController::_threadGetLoadLevelStatus, pAction);
    thread.start();
}

void
DimmingController::_threadSetLoadLevelTarget(Poco::AutoPtr<Omm::Action> pAction)
{
    _pService->sendAction(pAction);
    Omm::ui1 newLoadlevelTarget = pAction->getArgument<Omm::ui1>("newLoadlevelTarget");
    _ansSetLoadLevelTarget(newLoadlevelTarget);
}

void
DimmingController::_threadGetLoadLevelTarget(Poco::AutoPtr<Omm::Action> pAction)
{
    _pService->sendAction(pAction);
    Omm::ui1 retLoadlevelTarget = pAction->getArgument<Omm::ui1>("retLoadlevelTarget");
    _ansGetLoadLevelTarget(retLoadlevelTarget);
}

void
DimmingController::_threadGetLoadLevelStatus(Poco::AutoPtr<Omm::Action> pAction)
{
    _pService->sendAction(pAction);
    Omm::ui1 retLoadlevelStatus = pAction->getArgument<Omm::ui1>("retLoadlevelStatus");
    _ansGetLoadLevelStatus(retLoadlevelStatus);
}



void
SwitchPowerController::SetTarget(const bool& NewTargetValue)
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("SetTarget")->clone();
    pAction->setArgument<bool>("NewTargetValue", NewTargetValue);
    _pService->sendAction(pAction);
}

void
SwitchPowerController::GetTarget(bool& RetTargetValue)
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("GetTarget")->clone();
    _pService->sendAction(pAction);
    RetTargetValue = pAction->getArgument<bool>("RetTargetValue");
}

void
SwitchPowerController::GetStatus(bool& ResultStatus)
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("GetStatus")->clone();
    _pService->sendAction(pAction);
    ResultStatus = pAction->getArgument<bool>("ResultStatus");
}

bool
SwitchPowerController::_getStatus()
{
    return _pService->getStateVar<bool>("Status");
}


void
SwitchPowerController::_reqSetTarget(const bool& NewTargetValue)
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("SetTarget")->clone();
    pAction->setArgument<bool>("NewTargetValue", NewTargetValue);
    Omm::ActionThread<SwitchPowerController> thread(this, &SwitchPowerController::_threadSetTarget, pAction);
    thread.start();
}

void
SwitchPowerController::_reqGetTarget()
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("GetTarget")->clone();
    Omm::ActionThread<SwitchPowerController> thread(this, &SwitchPowerController::_threadGetTarget, pAction);
    thread.start();
}

void
SwitchPowerController::_reqGetStatus()
{
    Poco::AutoPtr<Omm::Action> pAction = _pService->getAction("GetStatus")->clone();
    Omm::ActionThread<SwitchPowerController> thread(this, &SwitchPowerController::_threadGetStatus, pAction);
    thread.start();
}

void
SwitchPowerController::_threadSetTarget(Poco::AutoPtr<Omm::Action> pAction)
{
    _pService->sendAction(pAction);
    bool NewTargetValue = pAction->getArgument<bool>("NewTargetValue");
    _ansSetTarget(NewTargetValue);
}

void
SwitchPowerController::_threadGetTarget(Poco::AutoPtr<Omm::Action> pAction)
{
    _pService->sendAction(pAction);
    bool RetTargetValue = pAction->getArgument<bool>("RetTargetValue");
    _ansGetTarget(RetTargetValue);
}

void
SwitchPowerController::_threadGetStatus(Poco::AutoPtr<Omm::Action> pAction)
{
    _pService->sendAction(pAction);
    bool ResultStatus = pAction->getArgument<bool>("ResultStatus");
    _ansGetStatus(ResultStatus);
}



void
DimmableLightController::eventHandler(Omm::StateVar* pStateVar)
{
    if (pStateVar->getName() == "LoadLevelStatus") {
        Omm::ui1 val;
        pStateVar->getValue(val);
        _pDimmingController->_changedLoadLevelStatus(val);
    }
    else if (pStateVar->getName() == "Status") {
        bool val;
        pStateVar->getValue(val);
        _pSwitchPowerController->_changedStatus(val);
    }
}


DimmableLightController::DimmableLightController(Omm::Device* pDevice, SwitchPowerController* pSwitchPowerController, DimmingController* pDimmingController) :
CtlDeviceCode(pDevice),
_pDevice(pDevice),
_pSwitchPowerController(pSwitchPowerController),
_pDimmingController(pDimmingController)
{
    _pSwitchPowerController->_pService = _pDevice->getServiceForTypeFullString("urn:schemas-upnp-org:service:SwitchPower:1");
    _pDimmingController->_pService = _pDevice->getServiceForTypeFullString("urn:schemas-upnp-org:service:Dimming:1");

    init();
}

