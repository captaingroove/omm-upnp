/***************************************************************************|
|                               WARNING                                     |
|        This file is generated by the jamm stub generator jammgen.         |
|                   RENAME THIS FILE BEFORE EDITING!                        |
|           It will be overriden at the next run of jammgen.                |
***************************************************************************/

#ifndef DIMMABLELIGHT_CTRL_IMPL_H
#define DIMMABLELIGHT_CTRL_IMPL_H

#include <Jamm/Upnp.h>
#include "DimmableLightCtrl.h"

class DimmingControllerImpl : public DimmingController
{
private:
    virtual void _ansSetLoadLevelTarget(const Jamm::ui1& newLoadlevelTarget);
    virtual void _ansGetLoadLevelTarget(const Jamm::ui1& retLoadlevelTarget);
    virtual void _ansGetLoadLevelStatus(const Jamm::ui1& retLoadlevelStatus);

    virtual void _changedLoadLevelStatus(const Jamm::ui1& val);
};

class SwitchPowerControllerImpl : public SwitchPowerController
{
private:
    virtual void _ansSetTarget(const bool& NewTargetValue);
    virtual void _ansGetTarget(const bool& RetTargetValue);
    virtual void _ansGetStatus(const bool& ResultStatus);

    virtual void _changedStatus(const bool& val);
};

#endif
