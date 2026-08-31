#!/bin/bash
# Launches the OpenRGB GUI as a pure SDK client against the dedicated
# Light Mount server (openrgb-lightmount-server.service) - never touches
# hardware directly, never runs its own detection pass, so it can never
# accidentally flip other RGB devices (Apex 3, Corsair Ironclaw) into
# Direct mode. See STATE.md 2026-08-24 for why --nodetect is required.
exec /home/mathias/bequiet-lightmount-linux/openrgb-src-private/build/openrgb \
    --nodetect \
    --client 127.0.0.1:6743 \
    --gui
