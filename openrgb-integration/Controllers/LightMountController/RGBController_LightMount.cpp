/*---------------------------------------------------------*\
| RGBController_LightMount.cpp                              |
|                                                           |
|   RGBController for be quiet! Light Mount                  |
|                                                           |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#include "RGBController_LightMount.h"
#include "LogManager.h"

/**------------------------------------------------------------------*\
    @name be quiet! Light Mount
    @category Keyboard
    @type USB
    @save :x:
    @direct :white_check_mark:
    @effects :white_check_mark:
    @detectors DetectLightMountControllers
    @comment Direct/per-key control and all six firmware effects are
             exposed by one controller. The firmware modes also cover
             the non-addressable underside lights.
\*-------------------------------------------------------------------*/

namespace
{
    constexpr unsigned int kColorsMax = 8;

    mode MakeDynamicMode(const std::string& name, int value, unsigned int extra_flags)
    {
        mode new_mode;
        new_mode.name           = name;
        new_mode.value          = value;
        new_mode.flags          = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR
                                | MODE_FLAG_HAS_BRIGHTNESS
                                | MODE_FLAG_HAS_SPEED
                                | extra_flags;
        new_mode.color_mode     = MODE_COLORS_MODE_SPECIFIC;
        new_mode.colors_min     = 1;
        new_mode.colors_max     = kColorsMax;
        new_mode.colors.resize(1);
        new_mode.brightness_min = 0;
        new_mode.brightness_max = 100;
        new_mode.brightness     = 100;
        new_mode.speed_min      = 0;
        new_mode.speed_max      = 100;
        new_mode.speed          = 50;
        return(new_mode);
    }
}

RGBController_LightMount::RGBController_LightMount(LightMountController* controller_ptr) :
    RGBController_HIDLampArray(controller_ptr),
    controller(controller_ptr)
{
    name        = controller->GetNameString();
    vendor      = "be quiet!";
    type        = DEVICE_TYPE_KEYBOARD;
    description = "be quiet! Light Mount";

    /*-------------------------------------------------------------*\
    | Replace the generic Direct/Autonomous list. Autonomous is an   |
    | implementation detail here; every named firmware effect uses  |
    | it after its vendor state has been accepted.                   |
    \*-------------------------------------------------------------*/
    modes.clear();

    mode direct;
    direct.name       = "Direct";
    direct.value      = LIGHT_MOUNT_MODE_DIRECT;
    direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR;
    direct.color_mode = MODE_COLORS_PER_LED;
    modes.push_back(direct);

    mode static_mode;
    static_mode.name           = "Static";
    static_mode.value          = LIGHT_MOUNT_MODE_STATIC;
    static_mode.flags          = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    static_mode.color_mode     = MODE_COLORS_MODE_SPECIFIC;
    static_mode.colors_min     = 1;
    static_mode.colors_max     = 1;
    static_mode.colors.resize(1);
    static_mode.brightness_min = 0;
    static_mode.brightness_max = 100;
    static_mode.brightness     = 100;
    modes.push_back(static_mode);

    mode color_wave = MakeDynamicMode("ColorWave", LIGHT_MOUNT_MODE_COLORWAVE,
                                      MODE_FLAG_HAS_DIRECTION_LR | MODE_FLAG_HAS_DIRECTION_UD);
    color_wave.direction = MODE_DIRECTION_RIGHT;
    modes.push_back(color_wave);

    mode tornado = MakeDynamicMode("Tornado", LIGHT_MOUNT_MODE_TORNADO, MODE_FLAG_HAS_DIRECTION_LR);
    tornado.direction = MODE_DIRECTION_LEFT;
    modes.push_back(tornado);

    modes.push_back(MakeDynamicMode("Breathing", LIGHT_MOUNT_MODE_BREATHING, 0));
    modes.push_back(MakeDynamicMode("Reactive", LIGHT_MOUNT_MODE_REACTIVE, 0));
    modes.push_back(MakeDynamicMode("Matrix", LIGHT_MOUNT_MODE_MATRIX, 0));

    active_mode = 0;
}

RGBController_LightMount::~RGBController_LightMount()
{
    /* The HIDLampArray base owns and deletes controller. */
}

void RGBController_LightMount::DeviceUpdateLEDs()
{
    if(modes[active_mode].value == LIGHT_MOUNT_MODE_DIRECT)
    {
        RGBController_HIDLampArray::DeviceUpdateLEDs();
    }
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

    if(active.value == LIGHT_MOUNT_MODE_DIRECT)
    {
        if(!controller->SetLampArrayControlReport(false))
        {
            LOG_WARNING("[Light Mount] Failed to enable LampArray Direct mode");
        }
        return;
    }

    bool vendor_mode_set = false;

    if(active.value == LIGHT_MOUNT_MODE_STATIC)
    {
        vendor_mode_set = controller->SendStaticColor(RGBGetRValue(active.colors[0]),
                                                       RGBGetGValue(active.colors[0]),
                                                       RGBGetBValue(active.colors[0]),
                                                       static_cast<uint8_t>(active.brightness));
    }
    else
    {
        uint8_t direction = 0x00;

        switch(active.value)
        {
            case LIGHT_MOUNT_MODE_COLORWAVE:
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
                direction = (active.direction == MODE_DIRECTION_RIGHT) ? 0x05 : 0x04;
                break;

            case LIGHT_MOUNT_MODE_MATRIX:
                direction = 0x01;
                break;

            default:
                direction = 0x00;
                break;
        }

        const LightMountEffect effect = static_cast<LightMountEffect>(active.value - 1);
        vendor_mode_set = controller->SendDynamicEffect(effect,
                                                         direction,
                                                         static_cast<uint8_t>(active.brightness),
                                                         static_cast<uint8_t>(active.speed),
                                                         active.colors);
    }

    /*-----------------------------------------------------------------*\
    | Only reveal the vendor layer after the device acknowledged it.    |
    | A failed vendor write therefore leaves the current Direct image   |
    | visible instead of switching to an unknown old firmware state.    |
    \*-----------------------------------------------------------------*/
    if(!vendor_mode_set)
    {
        LOG_WARNING("[Light Mount] Firmware effect was not accepted; keeping the current display layer");
        return;
    }

    if(!controller->SetLampArrayControlReport(true))
    {
        LOG_WARNING("[Light Mount] Firmware effect accepted, but enabling Autonomous mode failed");
    }
}
