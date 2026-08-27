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
from openrgb.utils import DeviceType, OpenRGBDisconnected, RGBColor

logger = logging.getLogger(__name__)

LAMP_ARRAY_DEVICE_NAME = "be quiet! Light Mount"
DIRECT_MODE = "Direct"


class LightMountClient:
    """Note: openrgb-python's LED.set_color()/Device.set_colors() default to
    `fast=False`, which re-fetches the full device state after every write
    (RGBCONTROLLER_UPDATESINGLELED/UPDATELEDS followed by a full
    requestDeviceData round trip). That refetch was found to intermittently
    exceed the client socket's read timeout even for a single non-looped
    call - something the earlier bulk-vs-individual-update fix (see
    set_all()) didn't cover, only exposed once a caller (the WireGuard
    status source) started writing every ~15s instead of rarely. Both write
    methods below pass `fast=True` (we never read LED state back through
    this client, so the refetch buys nothing) and additionally retry once
    through a fresh reconnect on OpenRGBDisconnected, as defense in depth
    against a genuine connection drop.

    Also: OpenRGB's generic HIDLampArrayController only sends the
    HID LampArray "disable Autonomous mode" control report from
    DeviceUpdateMode() - i.e. only in response to an explicit SDK mode-set
    call - never from DeviceUpdateLEDs() (what set_lamp()/set_all() below
    actually trigger). The RGBController object's in-memory `active_mode`
    defaults to Direct (0) on construction, but that is a software default,
    not a query of the device's real state - so if the device's own
    Autonomous flag is ever true when a fresh server (re-)detects it (e.g.
    real-world observed: after a KVM switch power-cycles the device), every
    subsequent single-LED/bulk write is silently accepted over USB and
    rendered nowhere, because the device is showing the vendor channel's
    state instead. Live-confirmed 2026-08-24: the daemon reported successful
    writes throughout, keyboard stayed dark, and explicitly disabling
    Autonomous via a raw HID write (tools/lamp_array_control.py, which does
    this before every write) made the already-buffered colors appear
    instantly. Fixed by explicitly forcing Direct mode after every (re)connect
    below, rather than trusting the device's power-on default.
    """

    def __init__(self, address: str = "127.0.0.1", port: int = 6742):
        self._address = address
        self._port = port
        self._client = OpenRGBClient(address=address, port=port, name="lightmount-automation")
        self._device = self._select_lamp_array_device()
        self._device.set_mode(DIRECT_MODE)

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

    def _reconnect(self) -> None:
        logger.warning("SDK connection dropped, reconnecting to %s:%d", self._address, self._port)
        self._client = OpenRGBClient(address=self._address, port=self._port, name="lightmount-automation")
        self._device = self._select_lamp_array_device()
        self._device.set_mode(DIRECT_MODE)

    def set_lamp(self, lamp_id: int, rgb: tuple[int, int, int]) -> None:
        """Single-LED update (RGBCONTROLLER_UPDATESINGLELED). Fine for small,
        targeted changes (a zone overlay); do not loop this for large diffs -
        see set_all().
        """
        try:
            self._device.leds[lamp_id].set_color(RGBColor(*rgb), fast=True)
        except OpenRGBDisconnected:
            self._reconnect()
            self._device.leds[lamp_id].set_color(RGBColor(*rgb), fast=True)

    def set_all(self, ordered_colors: list[tuple[int, int, int]]) -> None:
        """Bulk update (RGBCONTROLLER_UPDATELEDS), one packet for the whole
        device. Must supply a color for every LED, in lamp-ID order.
        Looping set_lamp() for a large number of LEDs instead of this was
        found to overwhelm the SDK connection (OpenRGBDisconnected) - always
        use this for whole/near-whole-device repaints.
        """
        try:
            self._device.set_colors([RGBColor(*c) for c in ordered_colors], fast=True)
        except OpenRGBDisconnected:
            self._reconnect()
            self._device.set_colors([RGBColor(*c) for c in ordered_colors], fast=True)

    def close(self) -> None:
        self._client.disconnect()
