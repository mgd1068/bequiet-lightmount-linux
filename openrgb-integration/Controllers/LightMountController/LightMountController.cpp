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

bool LightMountController::SendStaticColor(uint8_t r, uint8_t g, uint8_t b)
{
    if(!counter_primed)
    {
        return(false);
    }

    uint8_t report[LIGHT_MOUNT_REPORT_SIZE] = { 0 };

    /*-----------------------------------------------------------------*\
    | Header. length=0x0f matches the only static-color report shape   |
    | this project has confirmed on hardware - not derived from        |
    | payload size (no confirmed derivation rule exists).              |
    \*-----------------------------------------------------------------*/
    report[0] = 0x0F;
    report[1] = 0x00;
    report[2] = static_cast<uint8_t>(LIGHT_MOUNT_SESSION & 0xFF);
    report[3] = static_cast<uint8_t>(LIGHT_MOUNT_SESSION >> 8);
    report[4] = next_counter;
    report[5] = LIGHT_MOUNT_MARKER_STATIC_COLOR;
    report[6] = LIGHT_MOUNT_SUBCMD_STATIC_COLOR;
    report[7] = 0x00;
    next_counter++;

    /*-----------------------------------------------------------------*\
    | Payload: 00 00 <brightness=100> <unknown=0x32> 00 <R> <G> <B>     |
    | Confirmed byte-for-byte against a user-verified real color       |
    | (#1FB4FF) - see PROTOCOL.md "Bestätigt: RGB-Kodierung...".       |
    | Byte 11 (0x32) meaning is unconfirmed; kept at the only value    |
    | ever observed to work rather than guessed.                       |
    \*-----------------------------------------------------------------*/
    report[8]  = 0x00;
    report[9]  = 0x00;
    report[10] = 0x64;
    report[11] = 0x32;
    report[12] = 0x00;
    report[13] = r;
    report[14] = g;
    report[15] = b;

    uint16_t crc = Crc16Modbus(report, LIGHT_MOUNT_REPORT_SIZE - 2);
    report[62] = static_cast<uint8_t>(crc & 0xFF);
    report[63] = static_cast<uint8_t>(crc >> 8);

    int written = hid_write(dev, report, LIGHT_MOUNT_REPORT_SIZE);
    return(written == LIGHT_MOUNT_REPORT_SIZE);
}
