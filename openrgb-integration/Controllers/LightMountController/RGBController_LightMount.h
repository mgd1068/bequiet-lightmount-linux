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

/*-------------------------------------------------------------------*\
| Mode values match LightMountEffect in LightMountController.h        |
| directly (both are the device's own effect-type byte) so            |
| DeviceUpdateMode()/DeviceUpdateLEDs() can switch on modes[].value    |
| without a separate translation table.                               |
\*-------------------------------------------------------------------*/
enum
{
    LIGHT_MOUNT_MODE_STATIC     = LIGHT_MOUNT_EFFECT_STATIC,
    LIGHT_MOUNT_MODE_COLORWAVE  = LIGHT_MOUNT_EFFECT_COLORWAVE,
    LIGHT_MOUNT_MODE_TORNADO    = LIGHT_MOUNT_EFFECT_TORNADO,
    LIGHT_MOUNT_MODE_BREATHING  = LIGHT_MOUNT_EFFECT_BREATHING,
    LIGHT_MOUNT_MODE_REACTIVE   = LIGHT_MOUNT_EFFECT_REACTIVE,
    LIGHT_MOUNT_MODE_MATRIX     = LIGHT_MOUNT_EFFECT_MATRIX,
};

/*-------------------------------------------------------------------*\
| All 6 firmware effects, fully decoded and live-confirmed 2026-08-24 |
| - see PROTOCOL.md. Per-key control is intentionally still not       |
| exposed here: it belongs to the separate, already-working generic   |
| HIDLampArrayController on this device's Interface 3, not this       |
| Interface-2 vendor channel, which is confirmed to be a genuinely    |
| global/synchronized channel (see PROTOCOL.md "Architektur-          |
| Erkenntnis" - the two channels cannot show independent state at     |
| the same time anyway).                                              |
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
