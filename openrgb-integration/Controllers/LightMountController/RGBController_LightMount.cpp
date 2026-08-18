/*---------------------------------------------------------*\
| RGBController_LightMount.cpp                               |
|                                                             |
|   RGBController for be quiet! Light Mount                  |
|                                                             |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#include "RGBController_LightMount.h"

/**------------------------------------------------------------------*\
    @name be quiet! Light Mount
    @category Keyboard
    @type USB
    @save :x:
    @direct :white_check_mark:
    @effects :x:
    @detectors DetectLightMountControllers
    @comment Only a single, whole-keyboard static color is supported.
             Per-key addressing and the device's built-in effects
             (Matrix, Tornado, ColorWave, Breathing, Reactive) exist
             in firmware but their wire protocol is not yet confirmed
             - see the bequiet-lightmount-linux project's BACKLOG.md.
\*-------------------------------------------------------------------*/

RGBController_LightMount::RGBController_LightMount(LightMountController* controller_ptr)
{
    controller                          = controller_ptr;

    name                                = controller->GetNameString();
    vendor                              = "be quiet!";
    type                                = DEVICE_TYPE_KEYBOARD;
    description                         = "be quiet! Light Mount";
    location                            = controller->GetDeviceLocation();
    serial                              = controller->GetSerialString();

    mode Static;
    Static.name                         = "Static";
    Static.value                        = LIGHT_MOUNT_MODE_STATIC;
    Static.flags                        = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_MANUAL_SAVE;
    Static.color_mode                   = MODE_COLORS_MODE_SPECIFIC;
    Static.colors_min                   = 1;
    Static.colors_max                   = 1;
    Static.colors.resize(1);
    modes.push_back(Static);

    active_mode = 0;

    SetupZones();
}

RGBController_LightMount::~RGBController_LightMount()
{
    Shutdown();

    delete controller;
}

void RGBController_LightMount::SetupZones()
{
    /*-------------------------------------------------------------------*\
    | Single zone, single LED representing the whole keyboard - the only |
    | addressing granularity this project has confirmed on hardware.     |
    | The device's own manifest lists 168 individually named/indexed     |
    | LEDs (see docs/evidence/light_mount_led_layout_iso.json in the     |
    | bequiet-lightmount-linux project), but no wire command to address  |
    | them individually has been confirmed - do not expand this zone     |
    | until that protocol fact exists.                                   |
    \*-------------------------------------------------------------------*/
    zone whole_keyboard_zone;
    whole_keyboard_zone.name            = "Keyboard";
    whole_keyboard_zone.type            = ZONE_TYPE_SINGLE;
    whole_keyboard_zone.leds_min        = 1;
    whole_keyboard_zone.leds_max        = 1;
    whole_keyboard_zone.leds_count      = 1;
    zones.push_back(whole_keyboard_zone);

    led whole_keyboard_led;
    whole_keyboard_led.name             = "Keyboard";
    leds.push_back(whole_keyboard_led);

    SetupColors();
}

void RGBController_LightMount::DeviceUpdateLEDs()
{
    if(colors.empty())
    {
        return;
    }

    controller->SendStaticColor(RGBGetRValue(colors[0]), RGBGetGValue(colors[0]), RGBGetBValue(colors[0]));
}

void RGBController_LightMount::DeviceUpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_LightMount::DeviceUpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_LightMount::DeviceUpdateMode()
{
    controller->SendStaticColor(RGBGetRValue(modes[active_mode].colors[0]),
                                 RGBGetGValue(modes[active_mode].colors[0]),
                                 RGBGetBValue(modes[active_mode].colors[0]));
}

void RGBController_LightMount::DeviceSaveMode()
{
    /*-------------------------------------------------------------------*\
    | No onboard-save command has been confirmed for this device - see   |
    | SECURITY.md/BACKLOG.md in the bequiet-lightmount-linux project.    |
    | Intentionally a no-op rather than guessing a save command.         |
    \*-------------------------------------------------------------------*/
}
