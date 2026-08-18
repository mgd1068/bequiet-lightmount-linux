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

## Update — Iteration 3 (2026-08-18, ralph-loop)

- Checksum-Hypothese offline gegen alle 20 bekannten Frames verifiziert: letzte 2 Byte
  jedes 64-Byte-Reports = **CRC16/MODBUS** (Poly `0x8005`, Init `0xFFFF`, reflektiert,
  kein XOR-Out) über Byte 0–61, Little-Endian angehängt. 20/20 Treffer, siehe
  `docs/evidence/checksum_verification.py` (reproduzierbar, `python3` ausführen).
  Damit ist die vollständige Interface-2-Report-Struktur (Länge, konstantes Feld,
  Sequenznummer, Subcommand, Flags, Payload, CRC) für alle bisher beobachteten
  Kommandotypen bekannt — nur die genaue Payload-Semantik einzelner Subcommands
  (außer dem Rainbow-Gradient) ist noch offen.
- Kein Gerätezugriff, keine Hardware angefasst — reine Offline-Verifikation bekannter
  Bytes.

## Nächster konkreter Schritt

1. Sicheres Offline-Testgerüst in C++ beginnen (funktionaler Stil laut `DECISIONS.md`):
   freie Funktionen für CRC16/MODBUS-Berechnung, Report-Aufbau (Header+Payload+CRC) und
   Report-Parsing; als erste Tests die 20 hier bekannten Frames aus
   `docs/evidence/checksum_verification.py` als Fixtures nachbilden und die CRC-Funktion
   dagegen verifizieren (Portierung der bereits verifizierten Python-Logik).
2. Build-Grundgerüst anlegen (CMake, minimal — kein OpenRGB-Abhängigkeit in Phase 1,
   das kommt erst in Phase 3), Testframework-Entscheidung treffen (klein/header-only,
   passend zum funktionalen Stil, keine schwere Abhängigkeit ohne Not).
3. Danach: verbleibende unentschlüsselte Kommandos (29/15/18/7 Byte) mit dem neuen
   Report-Parser strukturiert ausgeben, um Muster leichter zu erkennen als per Hand.
4. Erst danach (weiterhin Phase 1, kein Hardwarezugriff): Testplan für einen ersten,
   risikoarmen realen Schreibtest ausarbeiten, inklusive Timeout/Reconnect-Verhalten —
   noch nicht ausführen.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Eine C++-Portierung der bereits in Python verifizierten CRC16/MODBUS-
  Funktion reproduziert exakt dieselben 20/20 Treffer gegen die bekannten Fixtures.
- **Erwartetes Ergebnis:** Kompilierender, laufender Offline-Test ohne Gerätezugriff,
  der als Fundament für den späteren Report-Builder dient.
- **Sicherheitsrisiko:** keins — reiner Code/Build/Test-Schritt, keine Hardware beteiligt.
- **Rückfall:** falls CMake/Testframework-Wahl Reibung erzeugt — minimal mit einer
  einzelnen `main.cpp` und `assert()`-basierten Tests starten, Testframework erst
  ergänzen, wenn der Umfang es rechtfertigt (kein Overengineering laut `DECISIONS.md`).

## Blocker

Keiner.
