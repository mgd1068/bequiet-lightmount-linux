"""Resolves named zones (from config/zones.yaml) to lamp-ID lists, using the
live-verified lamp_id -> key mapping from docs/evidence/lamp_id_key_mapping.json.
"""
from __future__ import annotations

import json
from pathlib import Path

import yaml

REPO_ROOT = Path(__file__).resolve().parents[2]
MAPPING_PATH = REPO_ROOT / "docs" / "evidence" / "lamp_id_key_mapping.json"


def load_lamp_mapping() -> dict[int, str]:
    data = json.loads(MAPPING_PATH.read_text())
    return {
        int(lamp_id): entry["key"]
        for lamp_id, entry in data["lamps"].items()
        if entry.get("key") is not None
    }


def _lamps_for_keys(key_names: list[str], lamp_to_key: dict[int, str]) -> list[int]:
    key_to_lamp = {key: lamp_id for lamp_id, key in lamp_to_key.items()}
    missing = [k for k in key_names if k not in key_to_lamp]
    if missing:
        raise ValueError(f"unknown key name(s) in zones.yaml: {missing}")
    return [key_to_lamp[k] for k in key_names]


class ZoneRegistry:
    """Resolves every zone in config/zones.yaml to a concrete lamp-ID list.

    Resolution order matters for `all_except` zones: they need every other
    zone already resolved, so those are done in a second pass.
    """

    def __init__(self, zones_yaml_path: Path, lamp_count: int = 135):
        self.lamp_to_key = load_lamp_mapping()
        self.lamp_count = lamp_count
        raw = yaml.safe_load(zones_yaml_path.read_text())["zones"]

        self.zones: dict[str, list[int]] = {}
        deferred: dict[str, dict] = {}

        for name, spec in raw.items():
            if "all_except" in spec:
                deferred[name] = spec
                continue
            self.zones[name] = self._resolve(spec)

        for name, spec in deferred.items():
            excluded: set[int] = set()
            for other in spec["all_except"]:
                excluded.update(self.zones[other])
            self.zones[name] = [i for i in range(lamp_count) if i not in excluded]

    def _resolve(self, spec: dict) -> list[int]:
        if "lamp_ids" in spec:
            return list(spec["lamp_ids"])
        if "lamp_range" in spec:
            start, end = spec["lamp_range"]
            return list(range(start, end + 1))
        if "keys" in spec:
            return _lamps_for_keys(spec["keys"], self.lamp_to_key)
        raise ValueError(f"zone spec has no recognized selector: {spec}")

    def lamp_ids(self, zone_name: str) -> list[int]:
        return self.zones[zone_name]

    def names(self) -> list[str]:
        return list(self.zones.keys())
