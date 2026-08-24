#!/usr/bin/env python3
"""Probe HID LampArray metadata without changing lighting state."""

import argparse
import fcntl
import json
import os
import struct


def _hid_feature_ioctl(report_size: int, request: int) -> int:
    # Linux _IOWR('H', request, report_size), as used by HIDIOCG/CSFEATURE.
    return 0xC0000000 | (report_size << 16) | (ord("H") << 8) | request


def get_feature(fd: int, report_id: int, report_size: int) -> bytes:
    data = bytearray(report_size)
    data[0] = report_id
    returned = fcntl.ioctl(
        fd, _hid_feature_ioctl(report_size, 0x07), data, True
    )
    if returned <= 0 or returned > report_size:
        raise RuntimeError(
            f"report {report_id}: invalid returned size {returned} for {report_size}-byte buffer"
        )
    return bytes(data[:returned])


def send_feature(fd: int, data: bytes) -> None:
    payload = bytearray(data)
    returned = fcntl.ioctl(
        fd, _hid_feature_ioctl(len(payload), 0x06), payload, True
    )
    if returned != len(payload):
        raise RuntimeError(
            f"report {payload[0]}: expected to send {len(payload)} bytes, sent {returned}"
        )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("device", help="LampArray hidraw device, for example /dev/hidraw11")
    parser.add_argument("--json", action="store_true", help="emit complete metadata as JSON")
    args = parser.parse_args()

    fd = os.open(args.device, os.O_RDWR | os.O_NONBLOCK)
    try:
        array_raw = get_feature(fd, 1, 23)
        lamp_count, width, height, depth, kind, min_interval = struct.unpack_from(
            "<HIIIII", array_raw, 1
        )

        # HID LampArray interrogation: selecting LampId 0 once makes each subsequent
        # LampAttributesResponse read advance to the next LampId.
        send_feature(fd, struct.pack("<BH", 2, 0))

        lamps = []
        for expected_id in range(lamp_count):
            # hidapi/OpenRGB requests the maximum 65-byte feature buffer here.  The
            # kernel returns the descriptor-defined 29 bytes for Report ID 3.
            raw = get_feature(fd, 3, 65)
            if len(raw) < 29:
                raise RuntimeError(
                    f"LampAttributesResponse is too short: expected at least 29, got {len(raw)}"
                )
            values = struct.unpack_from("<HIIIII6B", raw, 1)
            lamp = {
                "id": values[0],
                "x_um": values[1],
                "y_um": values[2],
                "z_um": values[3],
                "update_latency_us": values[4],
                "purposes": values[5],
                "red_levels": values[6],
                "green_levels": values[7],
                "blue_levels": values[8],
                "intensity_levels": values[9],
                "programmable": values[10],
                "input_binding": values[11],
            }
            if lamp["id"] != expected_id:
                raise RuntimeError(
                    f"LampId sequence mismatch: expected {expected_id}, got {lamp['id']}"
                )
            lamps.append(lamp)
    finally:
        os.close(fd)

    result = {
        "device": args.device,
        "lamp_count": lamp_count,
        "bounding_box_um": [width, height, depth],
        "array_kind": kind,
        "min_update_interval_us": min_interval,
        "programmable_count": sum(lamp["programmable"] != 0 for lamp in lamps),
        "bound_key_count": sum(lamp["input_binding"] != 0 for lamp in lamps),
        "lamps": lamps,
    }

    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"device={args.device}")
        print(f"lamp_count={lamp_count}")
        print(f"bounding_box_um={width}x{height}x{depth}")
        print(f"array_kind={kind}")
        print(f"min_update_interval_us={min_interval}")
        print(f"programmable_count={result['programmable_count']}")
        print(f"bound_key_count={result['bound_key_count']}")


if __name__ == "__main__":
    main()
