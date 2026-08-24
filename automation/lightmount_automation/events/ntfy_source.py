"""MVP event source: subscribes to an ntfy topic's JSON stream and matches
incoming messages against config/events.yaml's `ntfy.matches` list by a
simple substring match against title+message+tags. Reuses the existing
self-hosted ntfy instance (see STATE.md/BACKLOG.md 2026-08-24) rather than
standing up new alerting infrastructure.
"""
from __future__ import annotations

import asyncio
import json
import logging

import httpx

from .base import EventSource

logger = logging.getLogger(__name__)


class NtfySource(EventSource):
    def __init__(self, engine, rules: dict, base_url: str, topic: str, matches: list[dict]):
        super().__init__(engine, rules)
        self.base_url = base_url.rstrip("/")
        self.topic = topic
        self.matches = matches

    def _text_of(self, message: dict) -> str:
        parts = [message.get("title", ""), message.get("message", "")]
        parts.extend(message.get("tags", []))
        return " ".join(parts).lower()

    def _handle_message(self, message: dict) -> None:
        if message.get("event") != "message":
            return  # ntfy also sends "open"/"keepalive" events on the stream
        text = self._text_of(message)
        for match in self.matches:
            rule_name = match["rule"]
            if match["contains"].lower() in text:
                self.activate(rule_name)
            elif "resolved_contains" in match and match["resolved_contains"].lower() in text:
                self.deactivate(rule_name)

    async def run(self) -> None:
        url = f"{self.base_url}/{self.topic}/json"
        while True:
            try:
                async with httpx.AsyncClient(timeout=None) as client:
                    async with client.stream("GET", url) as response:
                        logger.info("ntfy: subscribed to %s", url)
                        async for line in response.aiter_lines():
                            if not line:
                                continue
                            try:
                                self._handle_message(json.loads(line))
                            except json.JSONDecodeError:
                                logger.warning("ntfy: non-JSON line skipped: %r", line)
            except Exception:
                logger.exception("ntfy: stream error, reconnecting in 10s")
                await asyncio.sleep(10)
