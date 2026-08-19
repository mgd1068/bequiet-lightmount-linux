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
|   bytes[2:4]  "session", u16 LE - observed 0x0000/0x0001/0x0002 across |
|                different phases of a single connection, meaning        |
|                unclear. This driver uses 0x0001, matching the phase in |
|                which our own successful static-color writes happened. |
|   byte[4]     counter - a single-byte running counter that must        |
|                continue by exactly +1 from the device's real current   |
|                value (confirmed with two consecutive real hardware     |
|                writes, see PROTOCOL.md "Verbindungsaufbau-Capture").   |
|                There is currently NO KNOWN WAY to discover this value  |
|                without a live capture of real traffic - a fixed        |
|                starting value (tried: 1, and an arbitrary large value) |
|                is reliably rejected. This driver therefore REQUIRES    |
|                the caller to prime it via SetCounter() before the      |
|                first write; SendStaticColor() refuses to write until   |
|                then. See BACKLOG.md - this is the main open blocker.   |
|   byte[5]     marker - constant 0x10 for every static-color command    |
|                observed so far (third-party and our own captures).    |
|                NOT the counter's high byte (corrected 2026-08-19,      |
|                see PROTOCOL.md).                                       |
|   byte[6]     subcmd                                                  |
|   byte[7]     flags                                                   |
|   bytes[8:62] payload, zero-padded                                    |
|   bytes[62:64] crc16_modbus(bytes[0:62]), u16 LE                      |
\*-----------------------------------------------------------------------*/
#define LIGHT_MOUNT_SESSION                   0x0001
#define LIGHT_MOUNT_MARKER_STATIC_COLOR       0x10
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
    | Primes the running counter with a value known (from a real, live   |
    | capture) to be the device's current state, or one more than the    |
    | last value this controller itself successfully sent. Required      |
    | before SendStaticColor() will do anything - see the counter field  |
    | note above. There is no automatic way to obtain this value yet.    |
    \*-------------------------------------------------------------------*/
    void SetCounter(uint8_t counter);
    bool IsCounterPrimed() const;

    /*-------------------------------------------------------------------*\
    | Sets a single static color across the entire keyboard (confirmed   |
    | working: PROTOCOL.md "Erster selbst konstruierter Hardwaretest").  |
    | Returns false without writing anything if the counter has not been |
    | primed via SetCounter(), or if the write did not complete (device  |
    | busy/detached).                                                    |
    \*-------------------------------------------------------------------*/
    bool SendStaticColor(uint8_t r, uint8_t g, uint8_t b);

private:
    hid_device*     dev;
    std::string     location;
    std::string     name;
    uint8_t         next_counter;
    bool            counter_primed;
};
