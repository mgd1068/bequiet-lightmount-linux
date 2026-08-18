# LED-Layout-Tabelle: Indizes + Pixel-Koordinaten zusammengeführt (2026-08-18)

`light_mount_led_layout_iso.json` ist eine **eigene Zusammenführung** (kein Vendor-
Original) zweier aus der Windows-App extrahierter Fakten (siehe
`windows-app-static-analysis.md`):

1. Die LED-Index-Tabelle (`light_mount_leds_mapping.json`, `base` + `diff.ISO`
   gemerged für unser DE-ISO-Gerät) — Name → LED-Index (0–167).
2. Die Tasten-Layout-Datei für ISO (`light_mount_main_iso.json`, per Inhalt
   identifiziert: enthält `"UK"/"DE"/"FR"` in `layoutImages` sowie die
   ISO-spezifischen Tasten `Key_NonUsTilde`/`Key_NonUsBackslash`) — Name →
   Pixel-Geometrie (x/y/Breite/Höhe, für die UI-Darstellung gedacht).

Zusammengeführt per Name (Skript nicht Teil dieses Repos — Einweg-Analyse, siehe
`DECISIONS.md` zum Umgang mit Vendor-Material). Ergebnis: 166 Einträge (168
LED-Indizes minus der ANSI-exklusiven `Key_Backslash`, die für unser ISO-Gerät nicht
zutrifft — ein Index bleibt in der Zählung unbelegt, nicht abschließend geklärt warum
genau 166 statt 167, keine weitere Zeit investiert).

## Struktur

```json
{
  "name": "Key_Escape",
  "led_index": 46,
  "geometry": { "type": "Rect", "x": 118, "y": 209, "width": 28, "height": 31, "rx": 3 }
}
```

- 111 Einträge (Einzeltasten) haben eine `geometry` (Pixel-Position für UI-Rendering,
  vermutlich in etwa proportional zur echten physischen Anordnung, aber **nicht**
  verifiziert als exakte physische Millimeter-Maße).
- 55 Einträge (`Led_KeyboardTop1-45`, `Led_KeyboardLeft1-5`, `Led_KeyboardRight1-5`)
  haben `geometry: null` — das sind die Leisten-LEDs ohne eigene Taste, ihre Geometrie
  steckt vermutlich in separaten `boundingRect`-Dateien (im Windows-App-Material
  gefunden, aber noch nicht ausgewertet, siehe `BACKLOG.md`).

## Verwendungszweck

Grundlage für ein künftiges OpenRGB-Controller-Layout (Phase 3): `RGBController`-Zonen
und LED-Namen können direkt aus dieser Tabelle abgeleitet werden, sobald das
Wire-Kommando für Per-Key-Adressierung bekannt ist (siehe `BACKLOG.md`, weiterhin offen).
Die Pixel-Koordinaten sind nur für ein grobes visuelles Layout gedacht, nicht für die
Bestimmung der physischen LED-Reihenfolge auf dem Draht (die kommt aus dem noch
unbekannten Protokoll selbst).
