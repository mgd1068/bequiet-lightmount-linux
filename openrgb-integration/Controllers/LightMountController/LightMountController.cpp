/*---------------------------------------------------------*\
| LightMountController.cpp                                  |
|                                                           |
|   Driver for be quiet! Light Mount                         |
|                                                           |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#include <chrono>
#include <cstring>

#include "LightMountController.h"
#include "LogManager.h"

LightMountController::LightMountController(hid_device* lamp_array_handle,
                                           const char* lamp_array_path,
                                           hid_device* vendor_handle,
                                           std::string dev_name) :
    HIDLampArrayController(lamp_array_handle, lamp_array_path),
    vendor_dev(vendor_handle),
    name(dev_name),
    next_counter(0),
    counter_primed(false)
{
}

LightMountController::~LightMountController()
{
    hid_close(vendor_dev);
}

std::string LightMountController::GetNameString()
{
    return(name);
}

void LightMountController::SetCounter(uint8_t counter)
{
    std::lock_guard<std::mutex> lock(vendor_mutex);
    next_counter   = counter;
    counter_primed = true;
}

bool LightMountController::IsCounterPrimed() const
{
    std::lock_guard<std::mutex> lock(vendor_mutex);
    return(counter_primed);
}

uint16_t LightMountController::Crc16Modbus(const uint8_t* data, std::size_t length)
{
    uint16_t crc = 0xFFFF;

    for(std::size_t byte_idx = 0; byte_idx < length; byte_idx++)
    {
        crc ^= data[byte_idx];

        for(unsigned int bit_idx = 0; bit_idx < 8; bit_idx++)
        {
            crc = (crc & 0x0001) ? static_cast<uint16_t>((crc >> 1) ^ 0xA001)
                                 : static_cast<uint16_t>(crc >> 1);
        }
    }

    return(crc);
}

bool LightMountController::SendReport(uint8_t marker, uint8_t subcmd, const std::vector<uint8_t>& payload)
{
    std::lock_guard<std::mutex> lock(vendor_mutex);

    if(!counter_primed || payload.size() > (LIGHT_MOUNT_REPORT_SIZE - 10))
    {
        return(false);
    }

    uint8_t report[LIGHT_MOUNT_REPORT_SIZE] = { 0 };
    const uint16_t length = static_cast<uint16_t>(payload.size() + 7);
    const uint8_t sent_counter = next_counter;

    report[0] = static_cast<uint8_t>(length & 0xFF);
    report[1] = static_cast<uint8_t>(length >> 8);
    report[2] = static_cast<uint8_t>(LIGHT_MOUNT_SESSION & 0xFF);
    report[3] = static_cast<uint8_t>(LIGHT_MOUNT_SESSION >> 8);
    report[4] = sent_counter;
    report[5] = marker;
    report[6] = subcmd;
    report[7] = 0x00;

    std::memcpy(&report[8], payload.data(), payload.size());

    const uint16_t crc = Crc16Modbus(report, LIGHT_MOUNT_REPORT_SIZE - 2);
    report[62] = static_cast<uint8_t>(crc & 0xFF);
    report[63] = static_cast<uint8_t>(crc >> 8);

    const int written = hid_write(vendor_dev, report, LIGHT_MOUNT_REPORT_SIZE);

    if(written != LIGHT_MOUNT_REPORT_SIZE)
    {
        LOG_WARNING("[Light Mount] Vendor report write failed (%d of %d bytes)", written, LIGHT_MOUNT_REPORT_SIZE);
        return(false);
    }

    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now()
                                                          + std::chrono::milliseconds(LIGHT_MOUNT_REPLY_TIMEOUT_MS);

    while(std::chrono::steady_clock::now() < deadline)
    {
        uint8_t reply[LIGHT_MOUNT_REPORT_SIZE] = { 0 };
        const std::chrono::milliseconds remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                        deadline - std::chrono::steady_clock::now());
        const int received = hid_read_timeout(vendor_dev,
                                              reply,
                                              sizeof(reply),
                                              static_cast<int>(remaining.count()));

        if(received <= 0)
        {
            break;
        }

        /*-------------------------------------------------------------*\
        | Interface 2 can also emit key-event telemetry. Never log that |
        | payload and keep waiting when it does not belong to our       |
        | command.                                                       |
        \*-------------------------------------------------------------*/
        if(received != LIGHT_MOUNT_REPORT_SIZE
        || reply[4] != sent_counter
        || reply[5] != marker
        || reply[6] != subcmd)
        {
            continue;
        }

        const uint16_t reply_crc = static_cast<uint16_t>(reply[62] | (reply[63] << 8));
        const bool valid_reply = (Crc16Modbus(reply, LIGHT_MOUNT_REPORT_SIZE - 2) == reply_crc)
                              && (reply[0] == 0x06)
                              && (reply[1] == 0x00)
                              && (reply[2] == static_cast<uint8_t>(LIGHT_MOUNT_SESSION & 0xFF))
                              && (reply[3] == 0x00);

        if(valid_reply)
        {
            next_counter++;
            return(true);
        }

        counter_primed = false;
        LOG_WARNING("[Light Mount] Vendor command rejected or reply invalid; counter desynchronized");
        return(false);
    }

    /*-----------------------------------------------------------------*\
    | The write may have reached the device. Its counter state is       |
    | therefore unknown; never retry using a guessed value.             |
    \*-----------------------------------------------------------------*/
    counter_primed = false;
    LOG_WARNING("[Light Mount] Vendor reply timed out; counter desynchronized");
    return(false);
}

bool LightMountController::SendStaticColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    const std::vector<uint8_t> payload =
    {
        0x00, 0x00, brightness, 0x32, 0x00, r, g, b
    };

    return(SendReport(LIGHT_MOUNT_MARKER_LIGHTING, LIGHT_MOUNT_SUBCMD_LIGHTING, payload));
}

bool LightMountController::SendDynamicEffect(LightMountEffect effect,
                                              uint8_t direction,
                                              uint8_t brightness,
                                              uint8_t speed,
                                              const std::vector<RGBColor>& colors)
{
    if(colors.empty() || colors.size() > 12)
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

        for(const RGBColor color : colors)
        {
            payload.push_back(RGBGetRValue(color));
            payload.push_back(RGBGetGValue(color));
            payload.push_back(RGBGetBValue(color));
        }
    }
    else
    {
        const uint8_t count = static_cast<uint8_t>(colors.size());
        payload.push_back(0x02);
        payload.push_back(count);

        for(uint8_t idx = 0; idx < count; idx++)
        {
            const uint8_t percent = static_cast<uint8_t>((idx * 100 + (count - 1) / 2) / (count - 1));
            payload.push_back(RGBGetRValue(colors[idx]));
            payload.push_back(RGBGetGValue(colors[idx]));
            payload.push_back(RGBGetBValue(colors[idx]));
            payload.push_back(percent);
        }
    }

    return(SendReport(LIGHT_MOUNT_MARKER_LIGHTING, LIGHT_MOUNT_SUBCMD_LIGHTING, payload));
}
