#!/usr/bin/env python3
"""Minimal, explicitly confirmed HID LampArray control utility."""

import argparse
import fcntl
import os
import struct


def _hid_set_feature_ioctl(report_size: int) -> int:
    return 0xC0000000 | (report_size << 16) | (ord("H") << 8) | 0x06


def send_feature(fd: int, payload: bytes) -> None:
    data = bytearray(payload)
    returned = fcntl.ioctl(fd, _hid_set_feature_ioctl(len(data)), data, True)
    if returned != len(data):
        raise RuntimeError(
            f"report {data[0]}: expected to send {len(data)} bytes, sent {returned}"
        )


def autonomous_report(enabled: bool) -> bytes:
    return bytes((6, int(enabled)))


def one_lamp_report(lamp_id: int, red: int, green: int, blue: int) -> bytes:
    return many_lamp_report([(lamp_id, red, green, blue)])


def many_lamp_report(lamps: list[tuple[int, int, int, int]]) -> bytes:
    if not 1 <= len(lamps) <= 8:
        raise ValueError("a Multi Update report holds between 1 and 8 lamps")
    for lamp_id, red, green, blue in lamps:
        if not 0 <= lamp_id <= 0xFFFF:
            raise ValueError("lamp ID must be between 0 and 65535")
        for name, value in (("red", red), ("green", green), ("blue", blue)):
            if not 0 <= value <= 255:
                raise ValueError(f"{name} must be between 0 and 255")

    padding = 8 - len(lamps)
    lamp_ids = [lamp_id for lamp_id, _, _, _ in lamps] + [0] * padding
    colors = [(red, green, blue, 255) for _, red, green, blue in lamps] + [(0, 0, 0, 0)] * padding
    return (
        bytes((4, len(lamps), 1))
        + struct.pack("<8H", *lamp_ids)
        + b"".join(struct.pack("<4B", *color) for color in colors)
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("device", help="LampArray hidraw device")
    parser.add_argument("--confirm", action="store_true", help="perform the write")
    subparsers = parser.add_subparsers(dest="command", required=True)

    set_one = subparsers.add_parser("set-one")
    set_one.add_argument("lamp_id", type=int)
    set_one.add_argument("red", type=int)
    set_one.add_argument("green", type=int)
    set_one.add_argument("blue", type=int)

    set_many = subparsers.add_parser("set-many")
    set_many.add_argument(
        "lamp",
        nargs="+",
        help="lamp_id:red:green:blue, up to 8 (e.g. 0:255:0:0 133:0:255:0)",
    )

    subparsers.add_parser("restore-autonomous")
    args = parser.parse_args()

    reports = []
    if args.command == "set-one":
        reports.append(autonomous_report(False))
        reports.append(one_lamp_report(args.lamp_id, args.red, args.green, args.blue))
    elif args.command == "set-many":
        lamps = []
        for spec in args.lamp:
            lamp_id, red, green, blue = (int(part) for part in spec.split(":"))
            lamps.append((lamp_id, red, green, blue))
        reports.append(autonomous_report(False))
        reports.append(many_lamp_report(lamps))
    else:
        reports.append(autonomous_report(True))

    for report in reports:
        print(f"report_{report[0]}={report.hex()}")

    if not args.confirm:
        print("dry_run=yes (pass --confirm before the subcommand to write)")
        return

    fd = os.open(args.device, os.O_RDWR | os.O_NONBLOCK)
    try:
        for report in reports:
            send_feature(fd, report)
    finally:
        os.close(fd)
    print("write_complete=yes")


if __name__ == "__main__":
    main()
