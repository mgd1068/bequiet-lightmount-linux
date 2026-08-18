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

## Update — Iteration 5 (2026-08-18, ralph-loop)

- Mountain-Everest-Referenzcode (`MountainKeyboardController.{h,cpp}`) per `curl` von
  GitLab Raw geladen (kein Vollklon nötig, WebFetch-Zusammenfassung war zu ungenau für
  einen Byte-genauen Vergleich — direktes `curl` liefert exakten Quelltext).
- Strukturvergleich durchgeführt: die ursprüngliche Hypothese "Light Mount ist
  protokollverwandt mit Mountain Everest" ist **widerlegt**. Mountain nutzt ein
  Report-ID-Byte + feste Kommandoklassen (`0x14`/`0x13`) an fester Position, **keine**
  Prüfsumme und **keine** Sequenznummer. Light Mount Interface 2 hat dagegen ein
  Längenfeld, einen monoton steigenden Sequenzzähler und einen CRC16/MODBUS-Trailer —
  keines davon existiert bei Mountain. Details: `PROTOCOL.md`, Abschnitt
  „Strukturvergleich mit Mountain Everest“.
- `ARCHITECTURE.md` aktualisiert: Mountain-Code bleibt als Vorlage für die spätere
  OpenRGB-Integrationsform (Klassenstruktur, `RGBController`-Anbindung) relevant, aber
  nicht mehr als Byte-Protokoll-Quelle.
- Kein Gerätezugriff — reiner Recherche-/Dokumentationsschritt.

## Update — Iteration 6 (2026-08-18, ralph-loop)

- Dry-Run-CLI `report_dump` gebaut (`src/cli/report_dump.cpp`, neues CMake-Target):
  liest einen 64-Byte-Report als 128-stelligen Hex-String (Argument oder stdin), gibt
  Länge/Sequenz/Subcmd/Flags/Payload/CRC-Gültigkeit lesbar aus. Öffnet **kein** `hidraw`,
  keine Geräteinteraktion. Manuell gegen das bekannte Rainbow-Kommando (Frame 1453)
  getestet: korrekt dekodiert, `crc_valid=yes`; mit manipuliertem letzten Byte getestet:
  `crc_valid=NO` wie erwartet; mit ungültiger Eingabelänge getestet: sauberer Fehler,
  Exit-Code 1.
- `README.md` um CLI-Nutzung ergänzt.
- Kein Gerätezugriff — reiner Code-Schritt.

## Nächster konkreter Schritt

1. Verbleibende unentschlüsselte Kommandos (29/15/18/7 Byte, siehe
   `docs/evidence/usbmon3_decoded_commands.txt`) mit `report_dump` durchgehen und ohne
   Mountain-Analogie (siehe Iteration 5, widerlegt) rein aus den Light-Mount-eigenen
   Daten neu interpretieren — insbesondere die beiden sehr ähnlichen 15-Byte-Kommandos
   (Frame 2447 vs. 2605, nur Byte 8 unterscheidet sich: `01` vs `00`) und die kurzen
   7-Byte-Toggle-Kommandos (Frame 1731/1739).
2. Sobald ein weiteres Kommando mit vertretbarer Sicherheit einem konkreten Feature
   zugeordnet werden kann: Testplan-Entwurf für einen ersten, risikoarmen realen
   Schreibtest beginnen (SECURITY.md-Regeln 1–10 explizit durchgehen, insbesondere
   Timeout/Reconnect-Verhalten und Rückfall auf bekannten sicheren Zustand) — **noch
   nicht ausführen**, nur Plan.
3. Erst nach explizitem Testplan (nicht in dieser oder der nächsten Iteration ohne
   weiteres): erster tatsächlicher `hidraw`-Schreibzugriff, ausschließlich mit einem der
   hier bereits bekannten, byteidentischen Kommandos (keine selbst konstruierten Bytes),
   um das Risiko unbekannter Nebenwirkungen zu minimieren.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Die beiden 15-Byte-Kommandos (Frame 2447/2605) unterscheiden sich nur
  in einem einzigen Payload-Byte (`01` vs `00`) und sind damit ein einfacher binärer
  Toggle innerhalb desselben Features (z. B. ein Ein/Aus-Schalter oder eine Auswahl aus
  zwei Optionen), nicht zwei unabhängige Kommandos.
- **Erwartetes Ergebnis:** `report_dump` bestätigt den Byte-für-Byte-Unterschied exakt;
  falls sich ein plausibles Feature (z. B. anhand der UI-Screenshots im OpenRGB-Ticket)
  zuordnen lässt, wird das in `PROTOCOL.md` als neue Hypothese ergänzt.
- **Sicherheitsrisiko:** keins — reine Offline-Analyse bekannter, bereits gesendeter Bytes.
- **Rückfall:** falls keine plausible Zuordnung möglich ist — als weiterhin offen markiert
  lassen, nicht raten und als Fakt hinstellen (siehe `PROTOCOL.md`-Grundregel).

## Blocker

Keiner.
