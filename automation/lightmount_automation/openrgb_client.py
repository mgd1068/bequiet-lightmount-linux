"""Thin wrapper around the openrgb-python SDK client, scoped to the Light
Mount's HID LampArray device on the dedicated server
(openrgb-lightmount-server.service, 127.0.0.1:6742). Never touches other
RGB devices - the server itself is configured to never detect them
(~/.config/openrgb-lightmount/OpenRGB.json), so there is nothing else to
accidentally address here.
"""
from __future__ import annotations

import logging

from openrgb import OpenRGBClient
from openrgb.utils import DeviceType, RGBColor

logger = logging.getLogger(__name__)

LAMP_ARRAY_DEVICE_NAME = "be quiet! Light Mount"


class LightMountClient:
    def __init__(self, address: str = "127.0.0.1", port: int = 6742):
        self._client = OpenRGBClient(address=address, port=port, name="lightmount-automation")
        self._device = self._select_lamp_array_device()

    def _select_lamp_array_device(self):
        candidates = [
            d
            for d in self._client.get_devices_by_type(DeviceType.KEYBOARD)
            if d.name == LAMP_ARRAY_DEVICE_NAME and len(d.leds) > 10
        ]
        if not candidates:
            raise RuntimeError(
                "no HID LampArray 'be quiet! Light Mount' device found on the SDK "
                "server - is openrgb-lightmount-server.service running?"
            )
        # Prefer the one with the most LEDs (135) in case the vendor
        # Interface-2 controller (1 LED) also matched the name filter.
        return max(candidates, key=lambda d: len(d.leds))

    @property
    def lamp_count(self) -> int:
        return len(self._device.leds)

    def set_lamp(self, lamp_id: int, rgb: tuple[int, int, int]) -> None:
        """Single-LED update (RGBCONTROLLER_UPDATESINGLELED). Fine for small,
        targeted changes (a zone overlay); do not loop this for large diffs -
        see set_all().
        """
        self._device.leds[lamp_id].set_color(RGBColor(*rgb))

    def set_all(self, ordered_colors: list[tuple[int, int, int]]) -> None:
        """Bulk update (RGBCONTROLLER_UPDATELEDS), one packet for the whole
        device. Must supply a color for every LED, in lamp-ID order.
        Looping set_lamp() for a large number of LEDs instead of this was
        found to overwhelm the SDK connection (OpenRGBDisconnected) - always
        use this for whole/near-whole-device repaints.
        """
        self._device.set_colors([RGBColor(*c) for c in ordered_colors])

    def close(self) -> None:
        self._client.disconnect()
