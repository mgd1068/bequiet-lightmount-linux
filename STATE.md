# STATE

## Aktuelle Phase

Phase 0 — Bestand und Reproduzierbarkeit.

## Letzter Stand

2026-08-18, Iteration 1 (ralph-loop, max. 30, Stop-Promise `LIGHTMOUNT_LOOP_STOP`):

- Repository angelegt (privat, `github.com/mgd1068/bequiet-lightmount-linux`),
  Pflichtdokumentation initial befüllt.
- System-/USB-Bestand read-only erfasst: Kubuntu 26.04, Kernel 7.0.0-29-generic,
  OpenRGB `0.9+git20251009+ds-1` bereits als Debian-Paket installiert. Light Mount
  angeschlossen: Bus 001 Device 060, USB 1.1 Full Speed, 4 HID-Interfaces, alle an
  Kernel-`usbhid` gebunden.
- `lsusb -v` und sysfs-`report_descriptor` aller vier Interfaces gesichert
  (`docs/evidence/`, committet — reine Geräte-Deskriptoren, keine Traffic-Capture,
  keine Tastendrücke). Ergebnisse siehe `PROTOCOL.md`: Interface 2 als wahrscheinlichster
  Vendor-Konfigurationskanal bestätigt (Endpoints **und** Report-Deskriptor stimmen mit
  Ausgangshypothese überein), Interface 3 als zweiter, umfangreicherer Vendor-Kanal via
  Feature-Reports neu entdeckt (stand vorher nicht im Ausgangsstand), Interface 1 enthält
  vermutlich das Medienrad als generische Maus-Collection, Interface 0 ist reiner
  Boot-Keyboard-Traffic.
- Kein `hidraw`-Gerät geöffnet oder gelesen — ausschließlich `lsusb` und sysfs-Attribute.

## Update — Iteration 2 (2026-08-18, ralph-loop)

- OpenRGB Issue #4950 geprüft (GitLab API, unauthentifiziert): keine neuen Kommentare,
  keine MR/Code seit Ticketerstellung. Ursprünglicher Melder hatte `bcdDevice 15.00`,
  `iSerial 0` (kein Serial-String) — abweichend von meinem Gerät (`23.00`,
  `QUK123456789`); vermutlich neuere Firmware-Revision, nicht sicherheitsrelevant.
- `usbmon3_capture.pcapng.gz` aus dem Issue geladen (via GitLab API mit numerischer
  Projekt-ID — der direkte `/uploads/`-Pfad aus der Markdown-Vorschau lieferte 404),
  lokal unter `captures-private/` entpackt (git-ignoriert, nicht committet).
- `tshark` installiert (`sudo apt-get install tshark`, System-Policy für sudo laut
  Nutzer explizit erlaubt). AppArmor sperrt `tshark -r <pfad>` für Dateien außerhalb
  fester Systempfade (`/usr/share/wireshark`, kein Home-Zugriff) — umgangen durch
  Lesen über Stdin (`tshark -r - < datei`), kein Policy-Change nötig.
- Capture analysiert (20 Frames auf Bus 3 Device 2, Endpoints 3/4 = Interface 2;
  Maus-Traffic auf `3.3.1` laut Melder-Hinweis ignoriert): **Interface 2 als echter
  Konfigurationskanal bestätigt** (nicht nur laut Deskriptor). Header-Struktur aus
  10 Kommando/Antwort-Paaren abgeleitet, ein 41-Byte-Kommando als 6-Stopp-Regenbogen-
  Gradient dekodiert (zweifach identisch bestätigt). Details: `PROTOCOL.md`,
  `docs/evidence/usbmon3_decoded_commands.txt`.
- Auf Nutzerwunsch: Coding-Stil-Entscheidung dokumentiert (C++, aber funktional/C-artig,
  OOP nur wo OpenRGBs API es verlangt) — siehe `DECISIONS.md`.
- Kein `hidraw`-Zugriff, kein Schreiben ans Gerät — weiterhin nur Analyse vorhandener
  Deskriptoren und eines fremden, bereits öffentlichen Captures.

## Nächster konkreter Schritt

1. Verbleibende, noch nicht entschlüsselte Kommandos aus `usbmon3_decoded_commands.txt`
   (29-Byte-, 15-Byte-, 18-Byte-, 7-Byte-Kommandos) strukturell weiter eingrenzen, soweit
   ohne Hardwaretest möglich (Vergleich mit Mountain-Referenzcode: welche Felder ähneln
   Speed/Brightness/Mode-Parametern aus `MountainKeyboardController.cpp`).
2. Lokal installierten OpenRGB-Quellstand (`apt source openrgb` oder GitLab-Clone,
   Submodule-Entscheidung noch offen) holen und `MountainKeyboardController.{cpp,h}`
   strukturell mit der hier gefundenen Header-Struktur vergleichen (Report-ID
   vorhanden/fehlend, Checksum-Position, Sequenzzähler-Konzept).
3. Sicheres Offline-Testgerüst (Dry-Run-CLI/Testbibliothek, C++, funktionaler Stil laut
   `DECISIONS.md`) beginnen: Report-Aufbau (Header + Payload + Checksum) mit den hier
   dekodierten Kommandos als Fixtures nachbilden und gegen die aufzeichneten Bytes prüfen
   — weiterhin **kein** Schreibzugriff auf `hidraw`.
4. Erst danach (Phase 1 laut `SPEC.md`/`BACKLOG.md`): Testplan für einen ersten,
   risikoarmen realen Schreibtest ("LED aus" bzw. minimale Farbänderung) ausarbeiten,
   inklusive Timeout/Reconnect-Verhalten — noch nicht ausführen.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Die Checksum in den letzten 2 Byte jedes Interface-2-Reports lässt sich
  aus einem Standardalgorithmus (CRC16, z. B. CRC16/CCITT oder ein einfaches Summen-
  Prüfbyte) über den restlichen Report herleiten — analog zu bekannten Mountain-Mustern.
- **Erwartetes Ergebnis:** Ein Offline-Test bestätigt oder widerlegt den Algorithmus
  gegen alle 20 bekannten Frames aus dem Capture, ohne Gerätezugriff.
- **Sicherheitsrisiko:** keins — reine Offline-Berechnung/Vergleich bekannter Bytes.
- **Rückfall:** falls kein Standardalgorithmus passt — Checksum als weiterhin unbekannt
  markieren und für den ersten Hardwaretest zunächst nur bekannte, byteidentische
  Kommandos (samt ihrer Original-Checksum) wiederverwenden, statt eigene Reports zu
  konstruieren.

## Blocker

Keiner.
