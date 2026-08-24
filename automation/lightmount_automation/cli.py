"""Manual control CLI - talks to the running daemon's local HTTP API, the
same one Home Assistant or anything else would use. Requires
lightmount-automation.service to be running.

Usage:
    lightmount-ctl zone light_bar FF0000
    lightmount-ctl zone light_bar off
    lightmount-ctl profile gaming
    lightmount-ctl profile off
    lightmount-ctl status
"""
from __future__ import annotations

import sys

import httpx

API_BASE = "http://127.0.0.1:8420"


def _hex_to_rgb(hex_color: str) -> tuple[int, int, int]:
    hex_color = hex_color.lstrip("#")
    return tuple(int(hex_color[i : i + 2], 16) for i in (0, 2, 4))


def main(argv: list[str]) -> int:
    if not argv:
        print(__doc__)
        return 1

    command = argv[0]

    if command == "zone" and len(argv) == 3:
        zone, value = argv[1], argv[2]
        if value.lower() == "off":
            resp = httpx.post(f"{API_BASE}/zones/{zone}/off")
        else:
            r, g, b = _hex_to_rgb(value)
            resp = httpx.post(f"{API_BASE}/zones/{zone}/color", json={"r": r, "g": g, "b": b})
        resp.raise_for_status()
        print(resp.json())
        return 0

    if command == "profile" and len(argv) == 2:
        name = None if argv[1] == "off" else argv[1]
        resp = httpx.post(f"{API_BASE}/profile", json={"name": name})
        resp.raise_for_status()
        print(resp.json())
        return 0

    if command == "status":
        resp = httpx.get(f"{API_BASE}/status")
        resp.raise_for_status()
        print(resp.json())
        return 0

    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
