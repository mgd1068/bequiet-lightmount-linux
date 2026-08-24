"""Time-of-day base profile scheduler - same concept as the SteelSeries
Apex 3's Standard/Late/Night systemd timers, but config-driven and
per-zone instead of whole-device-only, driven from inside this daemon
rather than separate systemd timers (one process, one clock check).
"""
from __future__ import annotations

import datetime as dt
import logging
from pathlib import Path

import yaml

from .layers import LayerEngine

logger = logging.getLogger(__name__)


def _parse_hm(value: str) -> dt.time:
    hours, minutes = value.split(":")
    return dt.time(int(hours) % 24, int(minutes))


class TimeProfileScheduler:
    def __init__(self, profiles_yaml_path: Path, engine: LayerEngine):
        data = yaml.safe_load(profiles_yaml_path.read_text())
        self.profiles = data["profiles"]
        self.manual_profiles = data.get("manual_profiles", {})
        self.engine = engine
        self._active_manual_profile: str | None = None
        self._current_profile_name: str | None = None

    def _profile_for_time(self, now: dt.time) -> tuple[str, dict]:
        for name, profile in self.profiles.items():
            start = _parse_hm(profile["start"])
            end = _parse_hm(profile["end"])
            if start <= end:
                if start <= now < end:
                    return name, profile
            else:  # wraps midnight, e.g. 20:00-24:00 handled as 20:00-00:00
                if now >= start or now < end:
                    return name, profile
        raise RuntimeError("no time profile covers the current time - check profiles.yaml")

    def tick(self) -> None:
        """Call periodically (e.g. every 60s). Applies the manual profile
        if one is active, otherwise the time-of-day profile - but only
        pushes to hardware when the effective profile actually changes.
        """
        if self._active_manual_profile:
            name = self._active_manual_profile
            zones = self.manual_profiles[name]["zones"]
        else:
            name, profile = self._profile_for_time(dt.datetime.now().time())
            zones = profile["zones"]

        if name == self._current_profile_name:
            return
        logger.info("switching base profile: %s", name)
        self._current_profile_name = name
        self.engine.set_base({zone: tuple(color) for zone, color in zones.items()})

    def set_manual_profile(self, name: str | None) -> None:
        """name=None returns to the time-of-day schedule."""
        if name is not None and name not in self.manual_profiles:
            raise ValueError(f"unknown manual profile: {name}")
        self._active_manual_profile = name
        self._current_profile_name = None  # force re-apply on next tick
        self.tick()
