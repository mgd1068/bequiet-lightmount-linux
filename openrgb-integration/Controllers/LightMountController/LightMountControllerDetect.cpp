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

        /*-------------------------------------------------------------*\
        | Primes the running counter to 0. Confirmed live 2026-08-24:   |
        | on a connection with demonstrably no prior traffic, counter=0 |
        | is accepted on the very first write attempt (see PROTOCOL.md  |
        | "Kaltstart-Problem des Zählers gelöst"). This closes the      |
        | counter-priming gap for the common case, but is NOT proven    |
        | safe if another client (e.g. an open iocenter.bequiet.com     |
        | browser tab) is already connected at detection time - there   |
        | is still no way to detect that condition without a live       |
        | capture. See BACKLOG.md.                                      |
        \*-------------------------------------------------------------*/
        controller->SetCounter(0);

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
