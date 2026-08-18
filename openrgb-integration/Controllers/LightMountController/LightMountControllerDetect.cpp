/*---------------------------------------------------------*\
| LightMountControllerDetect.cpp                             |
|                                                             |
|   Detector for be quiet! Light Mount                       |
|                                                             |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#include <hidapi.h>
#include "DetectionManager.h"
#include "LightMountController.h"
#include "RGBController_LightMount.h"

DetectedControllers DetectLightMountControllers(hid_device_info* info, const std::string& name)
{
    DetectedControllers detected_controllers;
    hid_device*         dev;

    dev = hid_open_path(info->path);

    if(dev)
    {
        LightMountController*     controller     = new LightMountController(dev, info->path, name);
        RGBController_LightMount* rgb_controller = new RGBController_LightMount(controller);

        detected_controllers.push_back(rgb_controller);
    }

    return(detected_controllers);
}

/*-----------------------------------------------------------------------*\
| Interface 2, Usage Page 0xFF00, Usage 0x01 - confirmed via sysfs report |
| descriptor read (docs/evidence/report_descriptor_if2.hex) in the        |
| bequiet-lightmount-linux project, not assumed.                          |
\*-----------------------------------------------------------------------*/
REGISTER_HID_DETECTOR_IPU("be quiet! Light Mount", DetectLightMountControllers, LIGHT_MOUNT_VID, LIGHT_MOUNT_PID, LIGHT_MOUNT_INTERFACE, LIGHT_MOUNT_USAGE_PAGE, LIGHT_MOUNT_USAGE);
