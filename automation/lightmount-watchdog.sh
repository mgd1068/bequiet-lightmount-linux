#!/bin/bash
set -u

PROJECT_DIR=/home/mathias/bequiet-lightmount-linux
HEALTHCHECK=${PROJECT_DIR}/automation/.venv/bin/python3

# Do nothing while the KBD switch presents the keyboard to another host.
device_present=false
for vendor_file in /sys/bus/usb/devices/*/idVendor; do
    device_dir=${vendor_file%/idVendor}
    if [[ $(<"${vendor_file}") == 373f ]] && \
       [[ -r "${device_dir}/idProduct" ]] && \
       [[ $(<"${device_dir}/idProduct") == 0002 ]]; then
        device_present=true
        break
    fi
done

if [[ ${device_present} != true ]]; then
    exit 0
fi

# The SDK probe is deliberately read-only. It verifies much more than an open
# TCP port: OpenRGB must answer the protocol handshake and expose the 135-lamp
# keyboard. The HTTP check confirms that the profile daemon is alive as well.
if timeout 8 "${HEALTHCHECK}" -m lightmount_automation.healthcheck && \
   curl --fail --silent --show-error --max-time 3 \
       http://127.0.0.1:8420/status >/dev/null; then
    exit 0
fi

logger -t lightmount-watchdog \
    'health check failed; restarting OpenRGB and profile automation'

systemctl --user restart openrgb-lightmount-server.service

# PartOf= propagates a server restart to an active automation service, but it
# intentionally does not start a service that was already inactive. Starting
# it explicitly covers both cases and reapplies the current profile.
systemctl --user restart lightmount-automation.service
