/*---------------------------------------------------------*\
| LightMountController.h                                    |
|                                                             |
|   Driver for be quiet! Light Mount                         |
|                                                             |
|   This file is part of the OpenRGB project                 |
|   SPDX-License-Identifier: GPL-2.0-or-later                |
\*---------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <string>
#include <hidapi.h>

/*-----------------------------------------------------------------------*\
| be quiet! vendor ID / Light Mount product ID                            |
\*-----------------------------------------------------------------------*/
#define LIGHT_MOUNT_VID                     0x373F
#define LIGHT_MOUNT_PID                     0x0002

/*-----------------------------------------------------------------------*\
| Interface 2 is the vendor config channel this controller uses           |
| (EP 0x83 IN / 0x04 OUT). Confirmed via sysfs report descriptor read and |
| offline capture analysis - see the bequiet-lightmount-linux project's   |
| PROTOCOL.md for the full derivation. Interface 0/1 carry normal         |
| keyboard/consumer HID traffic and must not be touched by this driver.  |
\*-----------------------------------------------------------------------*/
#define LIGHT_MOUNT_INTERFACE               2
#define LIGHT_MOUNT_USAGE_PAGE               0xFF00
#define LIGHT_MOUNT_USAGE                    0x01

#define LIGHT_MOUNT_REPORT_SIZE               64

/*-----------------------------------------------------------------------*\
| Confirmed protocol facts (bequiet-lightmount-linux PROTOCOL.md):        |
|   bytes[0:2]  length, u16 LE (exact derivation rule NOT confirmed -     |
|                only known-good values for known command shapes are     |
|                used here, never derived from payload size)             |
|   bytes[2:4]  session id, u16 LE (NOT a fixed constant - observed       |
|                0x0002 and 0x0001 in different sessions; this driver    |
|                uses 0x0002, matching the values used in every real     |
|                hardware test performed so far)                        |
|   bytes[4:6]  seq, u16 LE - the device REJECTS sequence numbers that   |
|                are far from its last-seen value for at least the      |
|                static-color command family (confirmed by hardware     |
|                test, see PROTOCOL.md "Sequenznummer-Validierung").     |
|                The exact acceptance rule is unknown. This driver uses  |
|                a small monotonically increasing counter starting at 1  |
|                as its best-effort strategy - UNVERIFIED against real  |
|                hardware for a value this low, see DECISIONS.md.        |
|   byte[6]     subcmd                                                  |
|   byte[7]     flags                                                   |
|   bytes[8:62] payload, zero-padded                                    |
|   bytes[62:64] crc16_modbus(bytes[0:62]), u16 LE                      |
\*-----------------------------------------------------------------------*/
#define LIGHT_MOUNT_SESSION                   0x0002
#define LIGHT_MOUNT_SUBCMD_STATIC_COLOR       0x06

/*-----------------------------------------------------------------------*\
| Only the "set a single static color for the whole keyboard" command is |
| implemented, because that is the only command this project has        |
| actually confirmed on real hardware with a *chosen* (not just replayed |
| verbatim) color. Per-key addressing, Matrix/Tornado/etc. effects, and  |
| brightness control are NOT implemented - their exact byte-level        |
| parameters are not confirmed, and this project does not guess at      |
| protocol details in code. See BACKLOG.md for what would be needed.    |
\*-----------------------------------------------------------------------*/

class LightMountController
{
public:
    LightMountController(hid_device* dev_handle, const char* path, std::string dev_name);
    ~LightMountController();

    std::string     GetDeviceLocation();
    std::string     GetNameString();
    std::string     GetSerialString();

    /*-------------------------------------------------------------------*\
    | Sets a single static color across the entire keyboard (confirmed   |
    | working: PROTOCOL.md "Erster selbst konstruierter Hardwaretest").  |
    | Returns false if the write did not complete (device busy/detached).|
    \*-------------------------------------------------------------------*/
    bool SendStaticColor(uint8_t r, uint8_t g, uint8_t b);

private:
    hid_device*     dev;
    std::string     location;
    std::string     name;
    uint16_t        next_seq;
};
