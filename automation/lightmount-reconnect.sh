#!/bin/bash
set -u

# Runs (as root, via udev) whenever the Light Mount's Interface 3
# (HID LampArray) hidraw node reappears - USB replug or KVM switch. OpenRGB
# does not re-detect hot-plugged devices on its own, so we restart the
# dedicated server to force a fresh detection pass. The automation service
# has PartOf=openrgb-lightmount-server.service, so the same restart also
# reconnects it and reapplies the current profile.
sleep 1

LOG_FILE=/tmp/lightmount-udev.log
RUNTIME_DIR=/run/user/1000
BUS_ADDRESS=unix:path=${RUNTIME_DIR}/bus

exec >>"${LOG_FILE}" 2>&1
echo "$(date --iso-8601=seconds) Light Mount interface 3 appeared; restarting services"

if [[ ! -S "${RUNTIME_DIR}/bus" ]]; then
    echo "$(date --iso-8601=seconds) user bus missing at ${RUNTIME_DIR}/bus"
    exit 1
fi

sudo -u mathias \
    XDG_RUNTIME_DIR="${RUNTIME_DIR}" \
    DBUS_SESSION_BUS_ADDRESS="${BUS_ADDRESS}" \
    systemctl --user restart openrgb-lightmount-server.service

echo "$(date --iso-8601=seconds) restart completed"
