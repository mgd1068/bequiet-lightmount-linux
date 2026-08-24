"""Composition engine: a base layer (time profile or manual profile) plus
named, prioritized overlay rules, recomputed into a single lamp_id -> RGB
map and pushed to the hardware as a diff (only changed lamps are sent).
"""
from __future__ import annotations

import logging
import time
from dataclasses import dataclass, field

from .openrgb_client import LightMountClient
from .zones import ZoneRegistry

logger = logging.getLogger(__name__)

RGB = tuple[int, int, int]


@dataclass
class Overlay:
    name: str
    zone: str
    color: RGB
    priority: int
    expires_at: float | None  # time.monotonic() timestamp, None = no expiry


DEFAULT_COLOR: RGB = (0, 0, 0)

# Above this many changed lamps, push a single bulk RGBCONTROLLER_UPDATELEDS
# packet instead of looping individual RGBCONTROLLER_UPDATESINGLELED
# packets - looping was found to overwhelm the SDK connection
# (OpenRGBDisconnected) for whole-device repaints (100+ lamps).
BULK_THRESHOLD = 8


class LayerEngine:
    def __init__(self, client: LightMountClient, zones: ZoneRegistry):
        self.client = client
        self.zones = zones
        self.base_zone_colors: dict[str, RGB] = {}
        self.overlays: dict[str, Overlay] = {}
        self._last_applied: dict[int, RGB] = {}

    def set_base(self, zone_colors: dict[str, RGB]) -> None:
        self.base_zone_colors = zone_colors
        self.recompute_and_push()

    def activate_overlay(
        self, name: str, zone: str, color: RGB, priority: int, ttl_seconds: int | None
    ) -> None:
        expires_at = time.monotonic() + ttl_seconds if ttl_seconds else None
        self.overlays[name] = Overlay(name, zone, color, priority, expires_at)
        logger.info("overlay activated: %s -> zone=%s color=%s", name, zone, color)
        self.recompute_and_push()

    def deactivate_overlay(self, name: str) -> None:
        if self.overlays.pop(name, None) is not None:
            logger.info("overlay deactivated: %s", name)
            self.recompute_and_push()

    def _expire_overlays(self) -> None:
        now = time.monotonic()
        expired = [n for n, o in self.overlays.items() if o.expires_at and o.expires_at <= now]
        for name in expired:
            logger.info("overlay expired (ttl): %s", name)
            del self.overlays[name]

    def recompute_and_push(self) -> None:
        self._expire_overlays()

        lamp_colors: dict[int, RGB] = {}
        for zone_name, color in self.base_zone_colors.items():
            for lamp_id in self.zones.lamp_ids(zone_name):
                lamp_colors[lamp_id] = color

        # Apply overlays in priority order (lowest first, so highest wins
        # when two overlays target the same zone).
        for overlay in sorted(self.overlays.values(), key=lambda o: o.priority):
            for lamp_id in self.zones.lamp_ids(overlay.zone):
                lamp_colors[lamp_id] = overlay.color

        diff = {
            lamp_id: color
            for lamp_id, color in lamp_colors.items()
            if self._last_applied.get(lamp_id) != color
        }
        if not diff:
            return

        if len(diff) > BULK_THRESHOLD:
            logger.debug("pushing bulk update (%d changed lamp(s))", len(diff))
            full = dict(self._last_applied)
            full.update(diff)
            ordered = [full.get(i, DEFAULT_COLOR) for i in range(self.client.lamp_count)]
            self.client.set_all(ordered)
            self._last_applied = {i: ordered[i] for i in range(len(ordered))}
        else:
            logger.debug("pushing %d changed lamp(s) individually", len(diff))
            for lamp_id, color in diff.items():
                self.client.set_lamp(lamp_id, color)
            self._last_applied.update(diff)
