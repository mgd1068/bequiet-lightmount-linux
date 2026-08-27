"""Entry point: wires up the OpenRGB client, zones, layer engine, time
profile scheduler, event sources, and the HTTP API into one asyncio
process. Run via `python -m lightmount_automation.daemon` (see the
lightmount-automation.service systemd unit).
"""
from __future__ import annotations

import asyncio
import logging
from pathlib import Path

import uvicorn
import yaml

from .api import create_app
from .events.ntfy_source import NtfySource
from .events.wireguard_source import WireguardSource
from .layers import LayerEngine
from .openrgb_client import LightMountClient
from .time_profiles import TimeProfileScheduler
from .zones import ZoneRegistry

CONFIG_DIR = Path(__file__).resolve().parents[1] / "config"
API_PORT = 8420

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s: %(message)s")
logger = logging.getLogger(__name__)


async def _time_profile_loop(scheduler: TimeProfileScheduler) -> None:
    while True:
        scheduler.tick()
        await asyncio.sleep(60)


async def main() -> None:
    zones = ZoneRegistry(CONFIG_DIR / "zones.yaml")
    client = LightMountClient()
    engine = LayerEngine(client, zones)
    scheduler = TimeProfileScheduler(CONFIG_DIR / "profiles.yaml", engine)
    scheduler.tick()  # apply the correct base profile immediately on start

    events_config = yaml.safe_load((CONFIG_DIR / "events.yaml").read_text())
    ntfy_cfg = events_config.get("ntfy")
    tasks = [asyncio.create_task(_time_profile_loop(scheduler))]
    if ntfy_cfg:
        ntfy_source = NtfySource(
            engine,
            rules=events_config["rules"],
            base_url=ntfy_cfg["base_url"],
            topic=ntfy_cfg["topic"],
            matches=ntfy_cfg["matches"],
        )
        tasks.append(asyncio.create_task(ntfy_source.run()))

    wireguard_cfg = events_config.get("wireguard")
    if wireguard_cfg:
        wireguard_source = WireguardSource(
            engine,
            tunnels=wireguard_cfg["tunnels"],
            poll_interval_seconds=wireguard_cfg.get("poll_interval_seconds", 15),
            ping_timeout_seconds=wireguard_cfg.get("ping_timeout_seconds", 2),
            priority=wireguard_cfg.get("priority", 20),
        )
        tasks.append(asyncio.create_task(wireguard_source.run()))

    app = create_app(engine, zones, scheduler)
    config = uvicorn.Config(app, host="127.0.0.1", port=API_PORT, log_level="info")
    server = uvicorn.Server(config)
    tasks.append(asyncio.create_task(server.serve()))

    logger.info("lightmount-automation started, API on http://127.0.0.1:%d", API_PORT)
    await asyncio.gather(*tasks)


if __name__ == "__main__":
    asyncio.run(main())
