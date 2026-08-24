"""Local HTTP API - this is the generic "device" surface external systems
(Home Assistant's RESTful Command/Switch integrations, or anything else)
call to control the Light Mount. Nothing here is Home-Assistant-specific;
it is a small, generic REST interface that happens to be easy for HA to
consume. See automation/README.md for an example HA-side wiring.
"""
from __future__ import annotations

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

from .layers import LayerEngine
from .time_profiles import TimeProfileScheduler
from .zones import ZoneRegistry


class ColorRequest(BaseModel):
    r: int
    g: int
    b: int


class ProfileRequest(BaseModel):
    name: str | None = None  # None = return to time-of-day schedule


def create_app(engine: LayerEngine, zones: ZoneRegistry, scheduler: TimeProfileScheduler) -> FastAPI:
    app = FastAPI(title="Light Mount automation API")

    @app.get("/status")
    def status():
        return {
            "zones": zones.names(),
            "active_overlays": list(engine.overlays.keys()),
        }

    @app.get("/zones")
    def list_zones():
        return {name: zones.lamp_ids(name) for name in zones.names()}

    @app.post("/zones/{name}/color")
    def set_zone_color(name: str, body: ColorRequest):
        if name not in zones.names():
            raise HTTPException(404, f"unknown zone: {name}")
        engine.activate_overlay(
            name=f"manual:{name}",
            zone=name,
            color=(body.r, body.g, body.b),
            priority=100,
            ttl_seconds=None,
        )
        return {"ok": True}

    @app.post("/zones/{name}/off")
    def clear_zone_override(name: str):
        engine.deactivate_overlay(f"manual:{name}")
        return {"ok": True}

    @app.post("/profile")
    def set_profile(body: ProfileRequest):
        try:
            scheduler.set_manual_profile(body.name)
        except ValueError as exc:
            raise HTTPException(400, str(exc)) from exc
        return {"ok": True, "active_profile": body.name or "time-of-day"}

    return app
