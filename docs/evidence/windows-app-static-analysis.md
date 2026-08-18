# Statische Analyse der Windows-IO-Center-App (2026-08-18)

Auf Nutzervorschlag: der Windows-Installer ("IO Center Installer.exe", von
bequiet.com, vom Nutzer manuell heruntergeladen und bereitgestellt) wurde rein
**statisch** analysiert — zu keinem Zeitpunkt ausgeführt, keine VM, kein Wine.
Vorgehen:

1. `innoextract` (Git-Hauptzweig gebaut, da Release 1.9 nur bis Inno-Setup-6.0.5
   unterstützt, dieser Installer ist 6.3.0) entpackt den Installer statisch.
2. App ist **Qt6/QML** (nicht Electron, anders als zunächst vermutet) mit einem
   separaten Hintergrunddienst `bequietIOCenterService.exe` und `hidapi.dll` (dieselbe
   Bibliothek, die auch OpenRGB nutzt) — bestätigt native `hidapi`-Kommunikation statt
   WebHID auf dem Windows-Client.
3. Zwei Qt-Ressourcenarchive (`.rcc`-Format) enthalten die eigentlichen Gerätedaten:
   `device_components.bin` (Lüfter-Zubehör, nicht relevant) und
   `devices_manifests.bin` (7,2 MB, alle unterstützten Geräte inkl. Light Mount).
4. `.rcc`-Format brute-force entpackt (zlib-Streams gesucht + dekomprimiert, kein
   vollständiger Tree/Name-Table-Parser nötig) sowie unkomprimierte JSON-Blöcke direkt
   im Rohbinary gefunden. Rohdateien liegen lokal unter `vendor-extracts-private/`
   (git-ignoriert — proprietäres Herstellermaterial, nicht Teil dieses Repos, analog zu
   `captures-private/`).

## Bestätigt: vollständiges offizielles Gerätemanifest (`light_mount_device_full.json`)

VID `14143` (`0x373F`) / PID `2` (`0x0002`) exakt bestätigt. Kernfelder:

```json
"OnDeviceLightings": {
  "availableGeneralEffects": ["Static", "ColorWave", "Tornado", "Breathing", "Reactive", "Matrix"],
  "availableCustomEffects":  ["Static", "ColorWave", "Tornado", "Breathing", "Reactive", "Matrix"],
  "mainColor": "#e63000"
},
"Macros": { "maxMacrosCount": 10, "maxEventsCount": 58 },
"PollingRate": { "pollingRates": [1000] },
"KeyBinding": { "availableBindingLayers": ["Common", "Fn1"] },
"FwUpdate": { "bootloaderDeviceId": { "vid": 14143, "pid": 10 } }
```

**Bestätigt genau 6 Effekte** — exakt dieselben Namen/Reihenfolge wie im
Mountain-Everest-Referenzcode-Enum (`STATIC, COLOR_WAVE, TORNADO, BREATHING, REACTIVE,
MATRIX`). Stützt die gemeinsame Produktherkunft weiter (siehe `PROTOCOL.md`), ohne die
bereits widerlegte Byte-Protokoll-Identität neu zu behaupten. Zwei der sechs Effekte
(Matrix bestätigt per Hardwaretest, vermutlich auch der noch nicht getestete
"ColorWave"/Rainbow-Zyklus) decken sich mit unseren bisherigen Funden.

**Bootloader-PID `10` (`0x000A`) ist eine separate USB-Geräteidentität** für den
Firmware-Update-Modus — **kein** Zusammenhang mit dem bereits beobachteten Subcmd
`0x0a` auf dem normalen Interface (unterschiedliche Namensräume, reiner Ziffern-Zufall,
bewusst nicht verknüpft).

## Bestätigt: vollständige Per-Key-LED-Tabelle (`light_mount_leds_mapping.json`)

**168 individuell adressierbare LEDs**, exakt durchnummeriert 0–167:

- Index 0–44: `Led_KeyboardTop1`…`Led_KeyboardTop45` (obere Leiste, 45 LEDs)
- Index 45–157: jede einzelne Taste (`Key_Escape`, `Key_A`, `Key_Enter`, `Key_Numpad0`, …)
  — inklusive ISO-spezifischer Tasten (`Key_NonUsTilde`=119, `Key_NonUsBackslash`=126,
  passend zu unserem DE-ISO-Gerät) bzw. ANSI-Variante (`Key_Backslash`=98)
- Index 158–167: `Led_KeyboardLeft1..5` + `Led_KeyboardRight5..1` (seitliche Leisten,
  10 LEDs)

**Das beantwortet die seit Iteration 1 offene Frage endgültig:** Per-Key-Adressierung
**existiert in der Firmware** — sie ist nur in IO Center Web nicht freigeschaltet
(bestätigt der Nutzer-UI-Check aus Iteration 14). Für Phase 2/3 (Einzeltasten-Kriterium
aus `SPEC.md`) ist damit klar, WELCHE 168 LEDs adressiert werden müssen — nur das
konkrete Wire-Kommando dafür ist noch unbekannt (kein solches Kommando bisher in einem
Capture beobachtet, da IO Center Web es nie sendet).

## Zusätzlich gefunden (noch nicht ausgewertet)

- `light_mount_main_iso.json`/`_ansi.json`: Pixel-Koordinaten (x/y/Breite/Höhe) jeder
  Taste für die UI-Darstellung — verknüpfbar mit den LED-Indizes über den gemeinsamen
  `Key_*`-Namen. Nützlich für ein künftiges OpenRGB-Layout (Phase 3).
- `light_mount_keys_mapping.json`: eine **andere**, kleinere Durchnummerierung
  (`Key_Escape`=1, `Key_M1`=2, …) — vermutlich Matrix-Scan-Positionen für Remapping,
  nicht dieselbe wie die LED-Indizes. Nicht verwechseln.
- `light_mount_media_wheel_utility.json`: separate Datei für das Medienrad.
- Kein Klartext-Byte-Protokoll (Kommandotabellen, Report-Aufbau) im Service-Binary
  gefunden (gezielte `strings`-Suche nach Effekt-/Kommandonamen ergebnislos) — die
  eigentliche Wire-Protokoll-Logik ist vermutlich kompiliert/nicht als Strings
  vorhanden, keine weitere Tiefenanalyse (Disassembly) durchgeführt.

## Lizenz-/Vorsichtshinweis

Die extrahierten Dateien sind proprietäres Herstellermaterial (be quiet!/Listan).
Reverse Engineering zu Interoperabilitätszwecken ist der erklärte Zweck dieses Projekts
(siehe `README.md`), aber die Rohdateien werden bewusst **nicht** in dieses Repository
committet (liegen lokal unter `vendor-extracts-private/`, git-ignoriert) — nur die hier
dokumentierten, in eigenen Worten zusammengefassten Fakten fließen ins Repo ein, analog
zum Umgang mit rohen USB-Captures.
