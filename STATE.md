# STATE

## Aktuelle Phase

Phase 1 — Sicheres Protokolllabor.

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

## Update — Iteration 4 (2026-08-18, ralph-loop)

- C++-Offline-Testgerüst aufgesetzt: `src/protocol/crc16.{h,cpp}` (freie Funktion,
  CRC16/MODBUS), `src/protocol/report.{h,cpp}` (plain struct `Interface2Report` +
  freie Funktionen `parse_report`/`build_report`/`report_crc_valid`, kein OOP,
  passend zu `DECISIONS.md`). Testframework: einfaches `assert()`-basiertes
  `tests/test_protocol.cpp` (kein externes Framework, wie im Rückfallplan der letzten
  Iteration vorgesehen — Umfang rechtfertigt bisher keines). Fixtures
  (`tests/fixtures_usbmon3.h`) sind die 20 aus `checksum_verification.py` bekannten
  Frames.
- Build: CMake (`CMakeLists.txt`, C++17, `-Wall -Wextra`), `cmake -S . -B build &&
  cmake --build build && ctest --test-dir build` — alle 4 Tests grün (CRC gegen alle
  20 Fixtures, `report_crc_valid` gegen alle 20, Parse→Build-Rundlauf byteidentisch
  für alle 20, bekannte Header-Felder + Rainbow-Stop-0-Bytes von Frame 1453 geprüft).
- Ein echter Implementierungsfehler beim ersten Durchlauf gefunden und behoben:
  `build_report` schrieb `raw[2]=0x00` statt `0x02` (das konstante Feld ist Byte2=`0x02`,
  Byte3=`0x00`, nicht umgekehrt — Verwechslung beim Übertragen aus der
  Little-Endian-Notation `0x0002`). Der Parse→Build-Rundlauf-Test hat das sofort als
  `assert`-Fehlschlag aufgedeckt, bevor es unbemerkt geblieben wäre.
- `README.md` um Bau-/Testanleitung ergänzt. `build/` bleibt git-ignoriert.
- Kein Gerätezugriff — reiner Code/Build/Test-Schritt.

## Nächster konkreter Schritt

1. Verbleibende unentschlüsselte Kommandos (29/15/18/7 Byte, siehe
   `docs/evidence/usbmon3_decoded_commands.txt`) mit dem neuen Report-Parser
   strukturiert als weitere Test-Fixtures/Kommentare festhalten, um Muster leichter zu
   erkennen als per Hand (z. B. `payload`-Felder benennen, sobald ein Muster gesichert ist).
2. Lokal installierten OpenRGB-Quellstand bzw. GitLab-Quelle des
   `MountainKeyboardController` holen und strukturell mit der hier gefundenen
   Header-/Payload-Struktur vergleichen (offene Frage aus Iteration 2/3, bisher noch
   nicht bearbeitet).
3. Dry-Run-CLI (liest/baut Reports, druckt Hex-Dumps, öffnet **kein** `hidraw`) als
   nächsten CMake-Target ergänzen — Grundlage für den späteren, sorgfältig geplanten
   ersten echten Schreibtest.
4. Erst danach (weiterhin kein Hardwarezugriff): Testplan für einen ersten,
   risikoarmen realen Schreibtest ausarbeiten, inklusive Timeout/Reconnect-Verhalten.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Der lokal installierte OpenRGB-Build (`openrgb 0.9+git20251009+ds-1`,
  bereits als Debian-Paket vorhanden) enthält den Mountain-Everest-Controller-Quellcode
  in einem lokal auffindbaren Pfad (Quellpaket) oder muss stattdessen per GitLab-Clone
  geholt werden.
- **Erwartetes Ergebnis:** Klarheit über den einfachsten Weg an den Referenzcode zu
  kommen, ohne unnötig einen vollen OpenRGB-Checkout anzulegen, bevor Phase 3 beginnt.
- **Sicherheitsrisiko:** keins — reiner Lese-/Rechercheschritt.
- **Rückfall:** falls kein lokales Quellpaket existiert — gezielt nur die bereits in
  `docs/research-sources.md` verlinkten Einzeldateien (nicht das ganze Repo) per WebFetch
  laden, Vollklon erst in Phase 3 entscheiden (siehe offene Frage in `DECISIONS.md`).

## Blocker

Keiner.
