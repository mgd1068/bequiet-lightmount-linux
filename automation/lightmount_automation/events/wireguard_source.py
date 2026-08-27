"""Polls local WireGuard tunnel liveness (one ICMP echo bound to the
tunnel interface, via `ping -I <interface>`, at a host known reachable
through that tunnel) and keeps one zone permanently colored green
(reachable) or red (not) per configured tunnel. This is a continuous
status display, not a transient alert: the overlay is always present for
a configured tunnel, just recolored on each poll - unlike the
ttl_seconds/deactivate pattern used for ntfy-style alerts.

Deliberately probe-based rather than handshake-timestamp based: these
tunnels have no PersistentKeepalive configured, so an idle-but-healthy
tunnel can show an arbitrarily old `wg show ... latest-handshakes` value -
an active probe is the only way to distinguish "idle" from "actually
down" without guessing at a staleness threshold.
"""
from __future__ import annotations

import asyncio
import logging
import subprocess

from .base import EventSource

logger = logging.getLogger(__name__)

CONNECTED_COLOR = (0, 255, 0)
DISCONNECTED_COLOR = (255, 0, 0)


class WireguardSource(EventSource):
    def __init__(
        self,
        engine,
        tunnels: list[dict],
        poll_interval_seconds: int = 15,
        ping_timeout_seconds: int = 2,
        priority: int = 20,
    ):
        super().__init__(engine, rules={})
        self.tunnels = tunnels
        self.poll_interval_seconds = poll_interval_seconds
        self.ping_timeout_seconds = ping_timeout_seconds
        self.priority = priority

    def _is_reachable(self, interface: str, probe_target: str) -> bool:
        try:
            result = subprocess.run(
                [
                    "ping",
                    "-I", interface,
                    "-c", "1",
                    "-W", str(self.ping_timeout_seconds),
                    probe_target,
                ],
                capture_output=True,
                timeout=self.ping_timeout_seconds + 2,
            )
        except (subprocess.TimeoutExpired, FileNotFoundError):
            return False
        return result.returncode == 0

    async def run(self) -> None:
        while True:
            for tunnel in self.tunnels:
                interface = tunnel["interface"]
                reachable = await asyncio.to_thread(
                    self._is_reachable, interface, tunnel["probe_target"]
                )
                color = CONNECTED_COLOR if reachable else DISCONNECTED_COLOR
                self.engine.activate_overlay(
                    name=f"wireguard-{interface}",
                    zone=tunnel["zone"],
                    color=color,
                    priority=self.priority,
                    ttl_seconds=None,
                )
                logger.debug("wireguard: %s -> %s", interface, "up" if reachable else "down")
            await asyncio.sleep(self.poll_interval_seconds)
