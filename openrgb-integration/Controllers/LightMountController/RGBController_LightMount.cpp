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
    @effects :white_check_mark:
    @detectors DetectLightMountControllers
    @comment All 6 firmware effects (Static, ColorWave, Tornado,
             Breathing, Reactive, Matrix) are supported, whole-keyboard
             only - this is the vendor config channel (Interface 2),
             confirmed to be a genuinely global/synchronized channel.
             Per-key control is the separate HIDLampArrayController on
             this device's Interface 3. See PROTOCOL.md.
\*-------------------------------------------------------------------*/

namespace
{
    /*-------------------------------------------------------------------*\
    | Colors_max=8 is a documented cap, not a confirmed device limit -   |
    | only 4 and 7 keyframes have actually been observed live. The       |
    | payload buffer (54 bytes) allows up to 12 for the long keyframe    |
    | form; 8 was chosen as a reasonable, untested-beyond headroom.      |
    | See PROTOCOL.md.                                                    |
    \*-------------------------------------------------------------------*/
    constexpr unsigned int kColorsMax = 8;

    mode MakeDynamicMode(const std::string& name, int value, unsigned int extra_flags)
    {
        mode m;
        m.name                          = name;
        m.value                         = value;
        m.flags                         = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR
                                         | MODE_FLAG_HAS_BRIGHTNESS
                                         | MODE_FLAG_HAS_SPEED
                                         | MODE_FLAG_MANUAL_SAVE
                                         | extra_flags;
        m.color_mode                    = MODE_COLORS_MODE_SPECIFIC;
        m.colors_min                    = 1;
        m.colors_max                    = kColorsMax;
        m.colors.resize(1);
        m.brightness_min                = 0;
        m.brightness_max                = 100;
        m.brightness                    = 100;
        m.speed_min                     = 0;
        m.speed_max                     = 100;
        m.speed                         = 50;
        return(m);
    }
}

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
    Static.flags                        = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_MANUAL_SAVE;
    Static.color_mode                   = MODE_COLORS_MODE_SPECIFIC;
    Static.colors_min                   = 1;
    Static.colors_max                   = 1;
    Static.colors.resize(1);
    Static.brightness_min               = 0;
    Static.brightness_max               = 100;
    Static.brightness                   = 100;
    modes.push_back(Static);

    /*-------------------------------------------------------------------*\
    | ColorWave: only effect with the full 4-way compass direction       |
    | (confirmed live: 0=up,1=down,2=left,3=right). Combining LR+UD is   |
    | how OpenRGB conventionally offers a 4-direction choice.            |
    \*-------------------------------------------------------------------*/
    mode ColorWave = MakeDynamicMode("ColorWave", LIGHT_MOUNT_MODE_COLORWAVE,
                                      MODE_FLAG_HAS_DIRECTION_LR | MODE_FLAG_HAS_DIRECTION_UD);
    ColorWave.direction = MODE_DIRECTION_RIGHT;
    modes.push_back(ColorWave);

    /*-------------------------------------------------------------------*\
    | Tornado: only 2 rotation directions exist (confirmed live:         |
    | 4=clockwise,5=counterclockwise) - a different, smaller range than  |
    | ColorWave's. OpenRGB has no clockwise/counterclockwise flag, so    |
    | LR is reused with a documented arbitrary mapping (see              |
    | DeviceUpdateMode()).                                               |
    \*-------------------------------------------------------------------*/
    mode Tornado = MakeDynamicMode("Tornado", LIGHT_MOUNT_MODE_TORNADO, MODE_FLAG_HAS_DIRECTION_LR);
    Tornado.direction = MODE_DIRECTION_LEFT;
    modes.push_back(Tornado);

    /*-------------------------------------------------------------------*\
    | Breathing: no spatial direction (payload byte1 always 0 in every   |
    | live capture) - no direction flag exposed.                         |
    \*-------------------------------------------------------------------*/
    modes.push_back(MakeDynamicMode("Breathing", LIGHT_MOUNT_MODE_BREATHING, 0));

    /*-------------------------------------------------------------------*\
    | Reactive: "speed" is confirmed to actually be the fade-back/decay  |
    | time here, not a spatial-effect tempo - MODE_FLAG_HAS_SPEED is     |
    | still the correct flag (OpenRGB has no separate "decay" concept),  |
    | just documented for future readers. No direction.                  |
    \*-------------------------------------------------------------------*/
    modes.push_back(MakeDynamicMode("Reactive", LIGHT_MOUNT_MODE_REACTIVE, 0));

    /*-------------------------------------------------------------------*\
    | Matrix: direction byte was always 1 (down) in every live capture - |
    | plausible given the "falling code rain" look, but only one value   |
    | was ever observed, so it is hardcoded in DeviceUpdateMode() rather  |
    | than exposed as a user control (would be guessing the other        |
    | values' meaning).                                                   |
    \*-------------------------------------------------------------------*/
    modes.push_back(MakeDynamicMode("Matrix", LIGHT_MOUNT_MODE_MATRIX, 0));

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
    | Single zone, single LED representing the whole keyboard. Confirmed |
    | 2026-08-24 (PROTOCOL.md "Architektur-Erkenntnis") that this is a   |
    | genuinely global/synchronized channel by design, not just an       |
    | unconfirmed limitation - every effect here always applies to the   |
    | entire keyboard at once. Real per-key addressing exists on this    |
    | device via the separate, already-working generic                   |
    | HIDLampArrayController (Interface 3) - see                         |
    | docs/evidence/lamp_id_key_mapping.json in the                      |
    | bequiet-lightmount-linux project. Do not expand this zone; it      |
    | would misrepresent what this channel can actually do.              |
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
    /*-------------------------------------------------------------------*\
    | Raw/direct color path (e.g. OpenRGB's own "Direct" client mode,    |
    | distinct from the named modes below) - this device has no true     |
    | per-frame direct mode on this channel, so fall back to a plain     |
    | Static write at full brightness, matching the pre-2026-08-24        |
    | behaviour of this function.                                        |
    \*-------------------------------------------------------------------*/
    if(colors.empty())
    {
        return;
    }

    controller->SendStaticColor(RGBGetRValue(colors[0]), RGBGetGValue(colors[0]), RGBGetBValue(colors[0]), 100);
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
    mode& active = modes[active_mode];

    if(active.value == LIGHT_MOUNT_MODE_STATIC)
    {
        controller->SendStaticColor(RGBGetRValue(active.colors[0]),
                                     RGBGetGValue(active.colors[0]),
                                     RGBGetBValue(active.colors[0]),
                                     static_cast<uint8_t>(active.brightness));
        return;
    }

    uint8_t direction = 0x00;

    switch(active.value)
    {
        case LIGHT_MOUNT_MODE_COLORWAVE:
            /*-----------------------------------------------------*\
            | Confirmed live 2026-08-24: 0=up,1=down,2=left,3=right |
            \*-----------------------------------------------------*/
            switch(active.direction)
            {
                case MODE_DIRECTION_UP:    direction = 0x00; break;
                case MODE_DIRECTION_DOWN:  direction = 0x01; break;
                case MODE_DIRECTION_LEFT:  direction = 0x02; break;
                case MODE_DIRECTION_RIGHT: direction = 0x03; break;
                default:                   direction = 0x03; break;
            }
            break;

        case LIGHT_MOUNT_MODE_TORNADO:
            /*-----------------------------------------------------------*\
            | No clockwise/counterclockwise flag exists in OpenRGB;      |
            | LR is reused with this documented, arbitrary mapping.      |
            | Confirmed live 2026-08-24: 4=clockwise,5=counterclockwise. |
            \*-----------------------------------------------------------*/
            direction = (active.direction == MODE_DIRECTION_RIGHT) ? 0x05 : 0x04;
            break;

        case LIGHT_MOUNT_MODE_MATRIX:
            /*-----------------------------------------------------*\
            | Only value ever observed live - not user-controllable |
            \*-----------------------------------------------------*/
            direction = 0x01;
            break;

        default:
            /* Breathing, Reactive: always 0, confirmed live */
            direction = 0x00;
            break;
    }

    controller->SendDynamicEffect(static_cast<LightMountEffect>(active.value), direction,
                                   static_cast<uint8_t>(active.brightness),
                                   static_cast<uint8_t>(active.speed),
                                   active.colors);
}

void RGBController_LightMount::DeviceSaveMode()
{
    /*-------------------------------------------------------------------*\
    | No onboard-save command has been confirmed for this device - see   |
    | SECURITY.md/BACKLOG.md in the bequiet-lightmount-linux project.    |
    | Intentionally a no-op rather than guessing a save command.         |
    \*-------------------------------------------------------------------*/
}
