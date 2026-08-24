#!/bin/bash
# Runs (as root, via udev) whenever the Light Mount's Interface 3
# (HID LampArray) hidraw node reappears - USB replug or KVM switch. OpenRGB
# does not re-detect hot-plugged devices on its own, so we restart the
# dedicated server to force a fresh detection pass. See STATE.md 2026-08-24
# and the Apex 3 precedent (openrgb-time-profile.sh /
# 99-openrgb-keyboard.rules) for why this pattern is needed at all.
sleep 1
sudo -u mathias XDG_RUNTIME_DIR=/run/user/1000 systemctl --user restart openrgb-lightmount-server.service >> /tmp/lightmount-udev.log 2>&1
