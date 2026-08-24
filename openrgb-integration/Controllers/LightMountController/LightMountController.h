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
#include <vector>
#include <hidapi.h>
#include "RGBControllerInterface.h"

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
#define LIGHT_MOUNT_SESSION                   0x0002
#define LIGHT_MOUNT_MARKER_LIGHTING            0x10
#define LIGHT_MOUNT_SUBCMD_LIGHTING            0x06

/*-----------------------------------------------------------------------*\
| All 6 firmware effects, fully decoded and live-confirmed on real       |
| hardware 2026-08-24 via a real usbmon capture of the official          |
| iocenter.bequiet.com web client (see PROTOCOL.md "Alle 6 Effekte       |
| entschlüsselt..." and the two "...-Parameter..." sections). Static     |
| keeps its own separate short payload shape; the other 5 share one      |
| payload layout:                                                        |
|   payload[0] effect type (matches the enum below)                     |
|   payload[1] direction - meaning/range is effect-specific:             |
|                ColorWave: 0=up,1=down,2=left,3=right (full compass)    |
|                Tornado:   4=clockwise,5=counterclockwise (2 values     |
|                           only - NOT the same encoding as ColorWave)   |
|                Breathing/Reactive: always 0 (no spatial direction)     |
|                Matrix: always 1 - only value ever observed, not        |
|                        exposed as a user-controllable parameter here   |
|   payload[2] brightness, 0-100 direct value                           |
|   payload[3] tempo/speed, 0-100 direct value (for Reactive this is     |
|              actually the fade-back/decay time, confirmed live)       |
|   payload[4] color-count mode: 0x00=1 color (short form, RGB follows  |
|              directly, no percent byte), 0x01=2 colors (medium form,  |
|              two direct RGB triplets, no percent byte), 0x02=3+       |
|              colors (long form: payload[5]=keyframe count N, then     |
|              N x [R,G,B,percent] - percent = round(i*100/(N-1)))      |
| The report length field for all of the above was NEVER derived from   |
| payload size before today - it now is, via a formula confirmed        |
| against 4 independent real captures: length = payload_bytes + 7.      |
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
    | brightness is 0-100, confirmed live 2026-08-24 (was hardcoded to   |
    | 100 before that). Returns false without writing anything if the    |
    | counter has not been primed via SetCounter(), or if the write did  |
    | not complete (device busy/detached).                               |
    \*-------------------------------------------------------------------*/
    bool SendStaticColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

    /*-------------------------------------------------------------------*\
    | Sets one of the 5 dynamic effects (everything except Static) with  |
    | its confirmed parameters. `direction`/`speed` are effect-specific  |
    | (see the LightMountEffect/payload comment above) - callers must    |
    | pass the raw device-level value already, this function does not    |
    | re-interpret them per effect. `colors` selects the color-count     |
    | mode: 1, 2, or 3+ entries switch between the three confirmed       |
    | payload shapes automatically. `colors[0]` becomes the FIRST wire   |
    | color (confirmed 2026-08-24 live capture: for Reactive this is the |
    | base/resting color, `colors[1]` the keypress-trigger color) -      |
    | matches the device's own byte order exactly. Observed once via     |
    | this OpenRGB build's own `--color A,B` CLI flag that the resulting |
    | visual base/trigger roles came out swapped versus what was typed;  |
    | not chased further since it wasn't confirmed whether that's a CLI  |
    | list-parsing quirk or applies to the GUI's colors[] too - the      |
    | payload byte order itself is the device-confirmed fact and is not  |
    | changed here to compensate for an unconfirmed client-side quirk.   |
    | Same counter-priming requirement and return value semantics as     |
    | SendStaticColor().                                                  |
    \*-------------------------------------------------------------------*/
    bool SendDynamicEffect(LightMountEffect effect, uint8_t direction, uint8_t brightness,
                            uint8_t speed, const std::vector<RGBColor>& colors);

private:
    /*-------------------------------------------------------------------*\
    | Shared report assembly: header (incl. counter increment) + given   |
    | payload + length field (payload_bytes + 7, see header comment) +   |
    | CRC16/MODBUS trailer, then a single hid_write(). Used by both      |
    | SendStaticColor() and SendDynamicEffect() so the header/CRC/length |
    | logic exists in exactly one place.                                 |
    \*-------------------------------------------------------------------*/
    bool SendReport(uint8_t marker, uint8_t subcmd, const std::vector<uint8_t>& payload);

    hid_device*     dev;
    std::string     location;
    std::string     name;
    uint8_t         next_counter;
    bool            counter_primed;
};
