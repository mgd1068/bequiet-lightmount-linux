/*---------------------------------------------------------*\
| LightMountController.h                                    |
|                                                           |
|   Driver for be quiet! Light Mount                         |
|                                                           |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include <hidapi.h>

#include "HIDLampArrayController.h"
#include "RGBControllerInterface.h"

#define LIGHT_MOUNT_VID                       0x373F
#define LIGHT_MOUNT_PID                       0x0002

#define LIGHT_MOUNT_VENDOR_INTERFACE          2
#define LIGHT_MOUNT_VENDOR_USAGE_PAGE         0xFF00
#define LIGHT_MOUNT_VENDOR_USAGE              0x01

#define LIGHT_MOUNT_LAMPARRAY_INTERFACE       3
#define LIGHT_MOUNT_LAMPARRAY_USAGE_PAGE      0x59
#define LIGHT_MOUNT_LAMPARRAY_USAGE           0x01

#define LIGHT_MOUNT_REPORT_SIZE               64
#define LIGHT_MOUNT_REPLY_TIMEOUT_MS          500
#define LIGHT_MOUNT_SESSION                   0x0002
#define LIGHT_MOUNT_MARKER_LIGHTING           0x10
#define LIGHT_MOUNT_SUBCMD_LIGHTING           0x06

/*-----------------------------------------------------------------------*\
| The vendor endpoint provides the six firmware effects for the whole    |
| device. The LampArray endpoint provides Direct/per-key control. They   |
| are mutually exclusive display layers selected by AutonomousMode, so  |
| one controller owns both endpoints.                                    |
\*-----------------------------------------------------------------------*/
enum LightMountEffect
{
    LIGHT_MOUNT_EFFECT_STATIC      = 0x00,
    LIGHT_MOUNT_EFFECT_COLORWAVE   = 0x01,
    LIGHT_MOUNT_EFFECT_TORNADO     = 0x02,
    LIGHT_MOUNT_EFFECT_BREATHING   = 0x03,
    LIGHT_MOUNT_EFFECT_REACTIVE    = 0x04,
    LIGHT_MOUNT_EFFECT_MATRIX      = 0x05,
};

class LightMountController : public HIDLampArrayController
{
public:
    LightMountController(hid_device* lamp_array_handle,
                         const char* lamp_array_path,
                         hid_device* vendor_handle,
                         std::string dev_name);
    ~LightMountController() override;

    std::string GetNameString();

    void SetCounter(uint8_t counter);
    bool IsCounterPrimed() const;

    bool SendStaticColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    bool SendDynamicEffect(LightMountEffect effect,
                           uint8_t direction,
                           uint8_t brightness,
                           uint8_t speed,
                           const std::vector<RGBColor>& colors);

private:
    bool SendReport(uint8_t marker, uint8_t subcmd, const std::vector<uint8_t>& payload);
    static uint16_t Crc16Modbus(const uint8_t* data, std::size_t length);

    hid_device*     vendor_dev;
    std::string     name;
    uint8_t         next_counter;
    bool            counter_primed;
    mutable std::mutex vendor_mutex;
};
