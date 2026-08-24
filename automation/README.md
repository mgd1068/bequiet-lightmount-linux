# Light Mount automation

External daemon that drives the be quiet! Light Mount "from outside" via
OpenRGB's SDK — time-of-day base colors, event-triggered per-zone overlays,
and a manually-toggled gaming profile. Manual GUI control and this daemon
never fight over the hardware: one dedicated OpenRGB process owns the
device, everything else (GUI, this daemon) is just an SDK client of it.

See `~/.claude/plans/hazy-hatching-gosling.md` for the full design writeup.

## Pieces

| Component | What |
|---|---|
| `openrgb-lightmount-server.service` | Dedicated OpenRGB SDK server (port 6742), owns the hardware. Detectors for other RGB devices (Apex 3, Corsair Ironclaw) are explicitly disabled in `~/.config/openrgb-lightmount/OpenRGB.json` so this instance can never touch them. |
| `openrgb-lightmount-gui.sh` | Manual GUI, connects as a pure SDK client (`--nodetect --client`) - never scans hardware itself. |
| `lightmount-automation.service` | This daemon: time profiles + event overlays + local HTTP API. |
| `lightmount-ctl` | CLI: `lightmount-ctl zone light_bar FF0000`, `lightmount-ctl profile gaming`, `lightmount-ctl status`. |
| `config/{zones,profiles,events}.yaml` | All user-editable, no hardcoded backend logic. |

## Zones

Defined in `config/zones.yaml`, resolved against the live-verified
`docs/evidence/lamp_id_key_mapping.json`. Add a zone by name/key list/lamp
range/lamp IDs - no code changes needed.

## Events

`lightmount_automation/events/` is a pluggable interface
(`EventSource` in `base.py`). The only concrete source right now is
`ntfy_source.py`, reusing the existing self-hosted ntfy instance
(`192.168.103.59:8090`) rather than new infrastructure - see
`config/events.yaml`. Writing a new source (webhook receiver, MQTT, etc.)
means implementing `async def run(self)` and calling `self.activate(rule)`/
`self.deactivate(rule)`; nothing else in the daemon needs to change.

## Integrating with Home Assistant

This daemon does **not** talk to Home Assistant directly - it exposes a
small local HTTP API (`http://127.0.0.1:8420` by default) that HA (running
at `192.168.2.26:8123` in this environment) can call using its generic
integrations. Example `configuration.yaml` snippet for HA (adjust the host
running this daemon):

```yaml
rest_command:
  lightmount_zone_color:
    url: "http://<daemon-host>:8420/zones/{{ zone }}/color"
    method: POST
    content_type: "application/json"
    payload: '{"r": {{ r }}, "g": {{ g }}, "b": {{ b }}}'

  lightmount_zone_off:
    url: "http://<daemon-host>:8420/zones/{{ zone }}/off"
    method: POST
```

Then a normal HA automation ("door sensor opens -> call
`lightmount_zone_color` with zone=light_bar, r=0,g=255,b=0") does the rest.
No plugin, no MQTT broker needed for this MVP. MQTT Discovery (making the
Light Mount show up as a proper HA light entity) is a natural next step
once an MQTT broker near the HA instance is confirmed to exist - not
verified this session (this machine couldn't reach `192.168.2.26` when
checked, "no route to host" - check routing/VLAN first).

## Operations

```bash
systemctl --user status openrgb-lightmount-server.service lightmount-automation.service
journalctl --user -u lightmount-automation.service -f
./lightmount-ctl status
```

The dedicated server's own config lives in `~/.config/openrgb-lightmount/`
(separate from `~/.config/OpenRGB/`, which the system package + Apex 3
timers keep using unmodified).
