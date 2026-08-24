# Draft comment for GitLab issue #4950 (NOT posted yet — needs your review/go-ahead)

Context: issue is still fully open, checklist unchecked, no maintainer engagement,
last real update was the original 2025-07-27 post (checked 2026-08-24 via the GitLab
API). Repo referenced below (`bequiet-lightmount-linux`) is still **private** per
`SECURITY.md` — this draft deliberately doesn't link it, only summarizes findings
inline. If you want to link it, it needs to go public first (and you'd want to
sanitize/confirm `captures-private/` etc. stay out of git history, which they already
do via `.gitignore`).

---

Hi, I've been reverse-engineering this device on Linux and have a finding that should
make OpenRGB support fairly straightforward — possibly without a device-specific
controller at all.

**Interface 3 is a standard HID LampArray device (Usage Page 0x59, Usage 0x01) and
already works with OpenRGB's existing generic `HIDLampArrayController`.** 135
individually addressable lamps, `ArrayKind = Keyboard`. I've live-verified a
near-complete lamp-ID → key mapping against real hardware (full matrix incl. numpad, 5
macro keys, volume dial, 23-segment top light bar) — happy to share the JSON if
useful. (My device's `bcdDevice`/iSerial differ from the original report — newer
firmware, LampArray interface unaffected.)

**One blocker that may matter beyond this device:** `DetectionManager` sets
`skip_generic_detectors = true` for *every* interface of a VID/PID once any
vendor-specific detector for that VID/PID activates, even on interfaces where that
detector's own match fails. Here, an interface-2-only vendor detector silently blocked
the generic LampArray detector from ever running on interface 3 of the same device.
Worked around locally by disabling the vendor detector; a real fix would probably
scope `skip_generic_detectors` per-interface instead of per-VID/PID.

There's also a separate vendor channel (Interface 2, same one iocenter.bequiet.com
uses) with a few global effects and two small accent lights outside the LampArray's
address space, not individually controllable as far as I can tell — not needed for
per-key operation, just FYI.

Happy to test patches against real hardware, or share the protocol notes.

---

**Vor dem Posten bitte prüfen:**
- GitLab-Account zum Posten (eigener Account? oder gar nicht selbst posten, nur als
  Grundlage für einen Maintainer-DM/E-Mail an be quiet! Support nutzen?)

**Zur Mapping-Datei:** `docs/evidence/lamp_id_key_mapping.json` ist mit 561 Zeilen zu
lang, um sie im Kommentar selbst einzufügen. Praktikabelster Weg, ohne das ganze
(noch private) Repo zu veröffentlichen: als eigenständiges **GitLab Snippet**
anhängen (Datei hochladen unter "Create new snippet", Link dann in den Kommentar
einfügen) — das ist ein bewusster, separater Veröffentlichungsschritt, den nur du
machen kannst (eigener GitLab-Account nötig), nicht etwas, das automatisch mit diesem
Kommentar-Entwurf mitpassiert. Datei liegt bereit unter
`~/bequiet-lightmount-linux/docs/evidence/lamp_id_key_mapping.json`.
