# Draft comment for GitLab issue #4950 (NOT posted yet — needs your review/go-ahead)

Context: issue is still fully open, checklist unchecked, no maintainer engagement,
last real update was the original 2025-07-27 post (checked 2026-08-24 via the GitLab
API). Repo referenced below (`bequiet-lightmount-linux`) is still **private** per
`SECURITY.md` — this draft deliberately doesn't link it, only summarizes findings
inline. If you want to link it, it needs to go public first (and you'd want to
sanitize/confirm `captures-private/` etc. stay out of git history, which they already
do via `.gitignore`).

---

Hi, I've been reverse-engineering this device on Linux over the past week or so and
wanted to share some findings that should make proper OpenRGB support fairly
straightforward — possibly without even needing a device-specific controller.

**TL;DR: Interface 3 is a standard HID LampArray device (Usage Page 0x59, Usage 0x01),
and it already works with OpenRGB's existing generic `HIDLampArrayController`.**

- 135 individually addressable lamps, `ArrayKind = Keyboard`, `MinUpdateInterval =
  33333µs`.
- I've live-verified a near-complete lamp-ID → physical-key mapping against real
  hardware (full main matrix including a numpad, 5 macro keys M1-M5, the volume dial,
  and all 23 segments of the top light bar). Happy to share the mapping table
  (`lamp_id → key name`, JSON) if useful for a proper controller/detector.
- My device's `bcdDevice` is `23.00` with a real iSerial string (`QUK...`), vs. the
  `15.00`/no-serial reported in the original issue — looks like a newer firmware
  revision, but the LampArray interface itself is unaffected.

**One real blocker I hit, which may be relevant beyond just this device:** while
prototyping a minimal vendor-protocol-only controller for Interface 2 (see below), I
found that `DetectionManager` sets `skip_generic_detectors = true` for *every*
interface of a VID/PID once any vendor-specific detector for that VID/PID activates —
even on interfaces where that vendor detector's own interface/usage match fails. In my
case, an interface-2-only vendor detector was silently blocking the generic
Page-0x59/Usage-0x01 LampArray detector from ever running on interface 3 of the *same*
device. Worked around locally with a detector config that disables the vendor
detector, but that's obviously not a real fix. Given `HIDLampArrayController` already
exists generically, I suspect other devices with a partial/legacy vendor detector
alongside a real LampArray interface could hit the same thing — might be worth a
narrower `skip_generic_detectors` scope (per-interface rather than per-VID/PID) as a
separate fix.

**Also FYI, not blocking anything:** there's a second vendor channel on Interface 2
(the same one `iocenter.bequiet.com`'s WebHID config page uses) that supports a
handful of global effects (Static, ColorWave, Matrix confirmed working; Tornado,
Breathing, Reactive not yet tested) plus two small RGB elements (side/underside
accent lights) that live *outside* the LampArray's 135-lamp address space and, as far
as I can tell after fairly thorough testing, aren't individually addressable at all —
only settable as part of the same global command that colors everything else on that
channel. Not needed for basic per-key operation via LampArray, just noting it in case
someone wants full effect parity later.

Happy to test patches/detector changes against real hardware, or share the full
protocol writeup/mapping data — let me know what'd be useful.

---

**Vor dem Posten bitte prüfen:**
- Ton/Detailgrad okay so, oder kürzer?
- Soll das Mapping (`docs/evidence/lamp_id_key_mapping.json`) tatsächlich angeboten
  werden (dann müsste zumindest diese eine Datei früher oder später öffentlich
  einsehbar sein, nicht das ganze Repo)?
- GitLab-Account zum Posten (eigener Account? oder gar nicht selbst posten, nur als
  Grundlage für einen Maintainer-DM/E-Mail an be quiet! Support nutzen?)
