/*---------------------------------------------------------*\
| RGBController_LightMount.h                                 |
|                                                             |
|   RGBController for be quiet! Light Mount                  |
|                                                             |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController.h"
#include "LightMountController.h"

enum
{
    LIGHT_MOUNT_MODE_STATIC = 0,
};

/*-------------------------------------------------------------------*\
| Only "Static" (single color, whole keyboard) is exposed. Per-key    |
| control, brightness, and the built-in effects (Matrix, Tornado,     |
| ColorWave, Breathing, Reactive) exist on the device but their wire  |
| parameters are not confirmed by this project - see BACKLOG.md.      |
| Do not add modes here without a corresponding confirmed protocol    |
| fact in PROTOCOL.md; guessed byte layouts do not belong in code.    |
\*-------------------------------------------------------------------*/
class RGBController_LightMount : public RGBController
{
public:
    RGBController_LightMount(LightMountController* controller_ptr);
    ~RGBController_LightMount();

    void        SetupZones();

    void        DeviceUpdateLEDs();
    void        DeviceUpdateZoneLEDs(int zone);
    void        DeviceUpdateSingleLED(int led);

    void        DeviceUpdateMode();
    void        DeviceSaveMode();

private:
    LightMountController* controller;
};
