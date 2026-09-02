/*---------------------------------------------------------*\
| RGBController_LightMount.h                                |
|                                                           |
|   RGBController for be quiet! Light Mount                  |
|                                                           |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController_HIDLampArray.h"
#include "LightMountController.h"

enum
{
    LIGHT_MOUNT_MODE_DIRECT      = 0,
    LIGHT_MOUNT_MODE_STATIC      = 1,
    LIGHT_MOUNT_MODE_COLORWAVE   = 2,
    LIGHT_MOUNT_MODE_TORNADO     = 3,
    LIGHT_MOUNT_MODE_BREATHING   = 4,
    LIGHT_MOUNT_MODE_REACTIVE    = 5,
    LIGHT_MOUNT_MODE_MATRIX      = 6,
};

/*-------------------------------------------------------------------*\
| One RGBController represents the physical keyboard. Direct mode    |
| uses LampArray on interface 3; all other modes use the firmware     |
| effects on interface 2 and then return LampArray to Autonomous.     |
\*-------------------------------------------------------------------*/
class RGBController_LightMount : public RGBController_HIDLampArray
{
public:
    RGBController_LightMount(LightMountController* controller_ptr);
    ~RGBController_LightMount() override;

    void DeviceUpdateLEDs() override;
    void DeviceUpdateZoneLEDs(int zone) override;
    void DeviceUpdateSingleLED(int led) override;
    void DeviceUpdateMode() override;

private:
    LightMountController* controller;
};
