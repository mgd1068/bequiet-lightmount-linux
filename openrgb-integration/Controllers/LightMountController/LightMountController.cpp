/*---------------------------------------------------------*\
| LightMountController.cpp                                   |
|                                                             |
|   Driver for be quiet! Light Mount                         |
|                                                             |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#include <cstring>
#include "LightMountController.h"
#include "StringUtils.h"

namespace
{
    /*---------------------------------------------------------------*\
    | CRC16/MODBUS (poly 0x8005, init 0xFFFF, reflected, no xorout).  |
    | Verified against 20 real captured reports in the               |
    | bequiet-lightmount-linux project (100% match) before this      |
    | controller was written - see that project's PROTOCOL.md and    |
    | docs/evidence/checksum_verification.py.                        |
    \*---------------------------------------------------------------*/
    uint16_t Crc16Modbus(const uint8_t* data, size_t len)
    {
        uint16_t crc = 0xFFFF;
        for(size_t i = 0; i < len; i++)
        {
            crc ^= data[i];
            for(int bit = 0; bit < 8; bit++)
            {
                if(crc & 0x0001)
                {
                    crc = static_cast<uint16_t>((crc >> 1) ^ 0xA001);
                }
                else
                {
                    crc = static_cast<uint16_t>(crc >> 1);
                }
            }
        }
        return crc;
    }
}

LightMountController::LightMountController(hid_device* dev_handle, const char* path, std::string dev_name)
{
    dev             = dev_handle;
    location        = path;
    name            = dev_name;
    next_counter    = 0;
    counter_primed  = false;
}

LightMountController::~LightMountController()
{
    hid_close(dev);
}

std::string LightMountController::GetDeviceLocation()
{
    return("HID: " + location);
}

std::string LightMountController::GetNameString()
{
    return(name);
}

std::string LightMountController::GetSerialString()
{
    wchar_t serial_string[128];
    int ret = hid_get_serial_number_string(dev, serial_string, 128);

    if(ret != 0)
    {
        return("");
    }

    return(StringUtils::wstring_to_string(serial_string));
}

void LightMountController::SetCounter(uint8_t counter)
{
    next_counter    = counter;
    counter_primed  = true;
}

bool LightMountController::IsCounterPrimed() const
{
    return(counter_primed);
}

bool LightMountController::SendReport(uint8_t marker, uint8_t subcmd, const std::vector<uint8_t>& payload)
{
    if(!counter_primed)
    {
        return(false);
    }

    if(payload.size() > (LIGHT_MOUNT_REPORT_SIZE - 8 - 2))
    {
        return(false);
    }

    uint8_t report[LIGHT_MOUNT_REPORT_SIZE] = { 0 };

    /*-----------------------------------------------------------------*\
    | Header. The length field's derivation rule was unknown until      |
    | today: confirmed against 4 independent real captures of different |
    | payload sizes (8/11/22/34 payload bytes -> length 15/18/29/41),   |
    | always exactly payload_bytes + 7. See PROTOCOL.md.                 |
    \*-----------------------------------------------------------------*/
    uint16_t length = static_cast<uint16_t>(payload.size() + 7);
    report[0] = static_cast<uint8_t>(length & 0xFF);
    report[1] = static_cast<uint8_t>(length >> 8);
    report[2] = static_cast<uint8_t>(LIGHT_MOUNT_SESSION & 0xFF);
    report[3] = static_cast<uint8_t>(LIGHT_MOUNT_SESSION >> 8);
    report[4] = next_counter;
    report[5] = marker;
    report[6] = subcmd;
    report[7] = 0x00;
    next_counter++;

    std::memcpy(&report[8], payload.data(), payload.size());

    uint16_t crc = Crc16Modbus(report, LIGHT_MOUNT_REPORT_SIZE - 2);
    report[62] = static_cast<uint8_t>(crc & 0xFF);
    report[63] = static_cast<uint8_t>(crc >> 8);

    int written = hid_write(dev, report, LIGHT_MOUNT_REPORT_SIZE);
    return(written == LIGHT_MOUNT_REPORT_SIZE);
}

bool LightMountController::SendStaticColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    /*-----------------------------------------------------------------*\
    | Payload: 00 00 <brightness> <unknown=0x32> 00 <R> <G> <B>         |
    | Confirmed byte-for-byte against a user-verified real color       |
    | (#1FB4FF) - see PROTOCOL.md "Bestätigt: RGB-Kodierung...".       |
    | Brightness position confirmed live 2026-08-24 with two           |
    | independent values (100 and 80). Byte 3 (0x32) meaning is        |
    | unconfirmed; kept at the only value ever observed to work rather |
    | than guessed.                                                     |
    \*-----------------------------------------------------------------*/
    std::vector<uint8_t> payload =
    {
        0x00, 0x00, brightness, 0x32, 0x00, r, g, b
    };

    return(SendReport(LIGHT_MOUNT_MARKER_LIGHTING, LIGHT_MOUNT_SUBCMD_LIGHTING, payload));
}

bool LightMountController::SendDynamicEffect(LightMountEffect effect, uint8_t direction, uint8_t brightness,
                                              uint8_t speed, const std::vector<RGBColor>& colors)
{
    if(colors.empty())
    {
        return(false);
    }

    std::vector<uint8_t> payload =
    {
        static_cast<uint8_t>(effect), direction, brightness, speed
    };

    if(colors.size() == 1)
    {
        payload.push_back(0x00);
        payload.push_back(RGBGetRValue(colors[0]));
        payload.push_back(RGBGetGValue(colors[0]));
        payload.push_back(RGBGetBValue(colors[0]));
    }
    else if(colors.size() == 2)
    {
        payload.push_back(0x01);
        for(const RGBColor& color : colors)
        {
            payload.push_back(RGBGetRValue(color));
            payload.push_back(RGBGetGValue(color));
            payload.push_back(RGBGetBValue(color));
        }
    }
    else
    {
        /*---------------------------------------------------------*\
        | 3+ colors: interpolated keyframe form. Percent-per-        |
        | keyframe formula confirmed live 2026-08-24 against a       |
        | real 7-keyframe rainbow capture (0/17/33/50/67/83/100%)    |
        | and a real 4-keyframe capture - round(i*100/(N-1)) matches |
        | both.                                                       |
        \*---------------------------------------------------------*/
        uint8_t count = static_cast<uint8_t>(colors.size());
        payload.push_back(0x02);
        payload.push_back(count);

        for(uint8_t i = 0; i < count; i++)
        {
            uint8_t percent = static_cast<uint8_t>((i * 100 + (count - 1) / 2) / (count - 1));
            payload.push_back(RGBGetRValue(colors[i]));
            payload.push_back(RGBGetGValue(colors[i]));
            payload.push_back(RGBGetBValue(colors[i]));
            payload.push_back(percent);
        }
    }

    return(SendReport(LIGHT_MOUNT_MARKER_LIGHTING, LIGHT_MOUNT_SUBCMD_LIGHTING, payload));
}
