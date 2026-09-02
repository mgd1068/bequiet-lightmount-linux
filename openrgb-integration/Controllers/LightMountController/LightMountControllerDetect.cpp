/*---------------------------------------------------------*\
| LightMountControllerDetect.cpp                            |
|                                                           |
|   Detector for be quiet! Light Mount                       |
|                                                           |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#include <cwchar>
#include <string>
#include <vector>
#include <hidapi.h>

#include "DetectionManager.h"
#include "LightMountController.h"
#include "LogManager.h"
#include "RGBController_LightMount.h"

namespace
{
    bool SameSerial(const hid_device_info* lhs, const hid_device_info* rhs)
    {
        return(lhs->serial_number != nullptr
            && rhs->serial_number != nullptr
            && lhs->serial_number[0] != L'\0'
            && rhs->serial_number[0] != L'\0'
            && std::wcscmp(lhs->serial_number, rhs->serial_number) == 0);
    }
}

DetectedControllers DetectLightMountControllers(hid_device_info* info, const std::string& name)
{
    DetectedControllers detected_controllers;
    std::vector<std::string> vendor_paths;
    std::vector<std::string> serial_matched_paths;

    hid_device_info* enumeration = hid_enumerate(info->vendor_id, info->product_id);

    for(hid_device_info* candidate = enumeration; candidate != nullptr; candidate = candidate->next)
    {
        if(candidate->interface_number == LIGHT_MOUNT_VENDOR_INTERFACE
        && candidate->usage_page       == LIGHT_MOUNT_VENDOR_USAGE_PAGE
        && candidate->usage            == LIGHT_MOUNT_VENDOR_USAGE)
        {
            vendor_paths.push_back(candidate->path);

            if(SameSerial(info, candidate))
            {
                serial_matched_paths.push_back(candidate->path);
            }
        }
    }

    std::string vendor_path;

    if(serial_matched_paths.size() == 1)
    {
        vendor_path = serial_matched_paths[0];
    }
    else if(info->serial_number != nullptr && info->serial_number[0] != L'\0')
    {
        LOG_WARNING("[Light Mount] Could not uniquely match vendor interface by serial number");
    }
    else if(vendor_paths.size() == 1)
    {
        /*-------------------------------------------------------------*\
        | Safe fallback for old firmware without a serial number: only |
        | pair interfaces when exactly one physical candidate exists.  |
        \*-------------------------------------------------------------*/
        vendor_path = vendor_paths[0];
    }
    else
    {
        LOG_WARNING("[Light Mount] Multiple devices without serial numbers cannot be paired safely");
    }

    hid_free_enumeration(enumeration);

    if(vendor_path.empty())
    {
        return(detected_controllers);
    }

    hid_device* lamp_array_dev = hid_open_path(info->path);
    hid_device* vendor_dev     = hid_open_path(vendor_path.c_str());

    if(lamp_array_dev == nullptr || vendor_dev == nullptr)
    {
        if(lamp_array_dev != nullptr)
        {
            hid_close(lamp_array_dev);
        }

        if(vendor_dev != nullptr)
        {
            hid_close(vendor_dev);
        }

        LOG_WARNING("[Light Mount] Failed to open both HID interfaces");
        return(detected_controllers);
    }

    LightMountController* controller = new LightMountController(lamp_array_dev,
                                                                 info->path,
                                                                 vendor_dev,
                                                                 name);

    if(controller->GetLampCount() == 0)
    {
        delete controller;
        return(detected_controllers);
    }

    /* A traffic-free connection accepts counter zero as its first value. */
    controller->SetCounter(0);

    detected_controllers.push_back(new RGBController_LightMount(controller));
    return(detected_controllers);
}

/*-----------------------------------------------------------------------*\
| Register specifically on the LampArray endpoint. This suppresses the    |
| generic LampArray fallback for this interface while the detector opens  |
| and owns both interface 3 (LampArray) and interface 2 (vendor effects). |
\*-----------------------------------------------------------------------*/
REGISTER_HID_DETECTOR_IPU("be quiet! Light Mount",
                          DetectLightMountControllers,
                          LIGHT_MOUNT_VID,
                          LIGHT_MOUNT_PID,
                          LIGHT_MOUNT_LAMPARRAY_INTERFACE,
                          LIGHT_MOUNT_LAMPARRAY_USAGE_PAGE,
                          LIGHT_MOUNT_LAMPARRAY_USAGE);
