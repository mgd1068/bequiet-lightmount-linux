"""Read-only health probe for the dedicated Light Mount OpenRGB server."""
from __future__ import annotations

from openrgb import OpenRGBClient
from openrgb.utils import DeviceType

from .openrgb_client import LAMP_ARRAY_DEVICE_NAME


def main() -> int:
    client = OpenRGBClient(
        address="127.0.0.1",
        port=6743,
        name="lightmount-watchdog",
    )
    try:
        devices = client.get_devices_by_type(DeviceType.KEYBOARD)
        healthy = any(
            device.name == LAMP_ARRAY_DEVICE_NAME and len(device.leds) >= 100
            for device in devices
        )
        return 0 if healthy else 1
    finally:
        client.disconnect()


if __name__ == "__main__":
    raise SystemExit(main())
