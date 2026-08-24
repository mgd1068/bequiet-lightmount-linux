"""Pluggable event-source interface. An EventSource watches something
external (ntfy, a webhook, a future Home Assistant/MQTT connector, ...)
and calls back into the LayerEngine to activate/deactivate named overlay
rules. Nothing in this base module or the LayerEngine knows anything
about ntfy, Home Assistant, or any other specific backend - that all
lives in the concrete source implementation.
"""
from __future__ import annotations

import abc

from ..layers import LayerEngine


class EventSource(abc.ABC):
    def __init__(self, engine: LayerEngine, rules: dict):
        self.engine = engine
        self.rules = rules

    def activate(self, rule_name: str) -> None:
        rule = self.rules[rule_name]
        self.engine.activate_overlay(
            name=rule_name,
            zone=rule["zone"],
            color=tuple(rule["color"]),
            priority=rule.get("priority", 0),
            ttl_seconds=rule.get("ttl_seconds"),
        )

    def deactivate(self, rule_name: str) -> None:
        self.engine.deactivate_overlay(rule_name)

    @abc.abstractmethod
    async def run(self) -> None:
        """Long-running coroutine; should keep listening until cancelled."""
