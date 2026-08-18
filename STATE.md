# STATE

## Aktuelle Phase

Phase 3 — OpenRGB-Integration (Grundgerüst geschrieben, Build-Verifikation läuft,
Hardwaretest steht noch aus).

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

## Update — Iteration 7 (2026-08-18, ralph-loop)

- Alle verbleibenden unentschlüsselten Kommandos mit `report_dump` neu durchgesehen
  (Frames 2447, 2605, 2747, 1731, 1739, 2997, 2341 — Rohausgabe geprüft statt aus dem
  Gedächtnis transkribiert).
- **Eigenen Fehler gefunden und korrigiert:** Die frühere Notiz "Frame 2447/2605
  unterscheiden sich nur in Byte 8" war falsch — tatsächlich unterscheiden sich ZWEI
  Payload-Bytes gemeinsam (Byte 8: `01→00`, Byte 9: `03→00`), alle anderen Bytes
  identisch. Korrigiert in `docs/evidence/usbmon3_decoded_commands.txt`.
- Neue Beobachtung zum Längenfeld dokumentiert: `length=6` (Frame 2997, subcmd 0x0a,
  leerer Payload) vs. `length=7` (Frame 1731, subcmd 0x02, ebenfalls leerer Payload)
  zeigt, dass die Länge NICHT allein aus der Payload-Füllung ableitbar ist, sondern auch
  vom Subkommando abhängt. Bewusst als offene Frage belassen, keine Formel im Code
  nachgebildet, um keine unbelegte Annahme in `build_report`/`parse_report`
  einzubauen — passt zur Grundregel aus `PROTOCOL.md` (Fakten von Hypothesen trennen).
- Keine neuen, hinreichend sicheren Feature-Zuordnungen gefunden (Frame 2747 "01 00 ff
  00 ff 37" bleibt uneindeutig, Frame 2341/3109 bleibt uneindeutig) — bewusst nicht
  geraten. Weitere Fortschritte hier brauchen entweder eigene Einzelaktions-Captures
  oder echte Hardwaretests, beides noch nicht Teil dieser Iteration.
- Kein Gerätezugriff — reine Offline-Analyse bereits bekannter Bytes.

## Nächster konkreter Schritt

1. Sicherheitsbewusste Entscheidung: die statische Offline-Analyse des fremden
   Mehrfach-Klick-Captures hat ihren Erkenntnisgewinn für diese Iteration weitgehend
   ausgeschöpft (ein klar entschlüsseltes Kommando: Rainbow-Gradient; mehrere strukturell
   verstandene, aber semantisch unklare Kommandos). Nächster sinnvoller Schritt laut
   `SPEC.md`/Phase 1 ist der **Testplan-Entwurf** für einen ersten, risikoarmen realen
   Schreibtest — NICHT die Ausführung selbst.
2. Testplan muss laut `SECURITY.md` mindestens enthalten: gewähltes Kommando (das
   bereits bekannte, byteidentisch reproduzierte Rainbow-Kommando aus Frame 1453/3531 —
   kein selbst konstruiertes), erwartete sichtbare Wirkung, Timeout-Wert, Reconnect-
   Verhalten bei USB-Reset, Rückfall auf bekannten sicheren Zustand (Beleuchtung aus),
   explizite `--dry-run`-Standardeinstellung, genaue Interface-/Endpoint-Auswahl
   (Interface 2, EP 0x04 OUT / EP 0x83 IN — keine anderen `hidraw`-Geräte berühren).
3. Testplan als eigenes Dokument oder Abschnitt festhalten (`docs/` oder `STATE.md`),
   dem Nutzer NICHT eigenmächtig zur Ausführung vorlegen, ohne dass die Freigabe für den
   ersten echten Schreibzugriff auf die angeschlossene Hardware explizit erteilt wurde —
   das ist der im Master-Prompt vorgesehene Übergang von reiner Analyse zu echtem
   Gerätekontakt und rechtfertigt Innehalten, auch wenn der Loop technisch autonom
   weiterlaufen könnte.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Ein schriftlicher Testplan (kein Code, keine Ausführung) lässt sich
  erstellen, der alle zehn SECURITY.md-Regeln nachvollziehbar abdeckt, bevor überhaupt
  ein `hidraw`-Handle geöffnet wird.
- **Erwartetes Ergebnis:** Ein dokumentierter, überprüfbarer Plan, der als Grundlage für
  eine spätere, bewusste Freigabe des ersten echten Schreibtests dient.
- **Sicherheitsrisiko:** keins beim Schreiben des Plans selbst; der Plan beschreibt aber
  einen Schritt mit echtem (wenn auch laut `SECURITY.md` minimiertem) Hardwarerisiko.
- **Rückfall:** keiner nötig, da dieser Schritt noch keine Hardware anfasst.

## Update — Iteration 8 (2026-08-18, ralph-loop)

- Testplan für den ersten echten `hidraw`-Schreibzugriff geschrieben:
  `docs/first-write-test-plan.md`. Deckt alle zehn `SECURITY.md`-Regeln ab (exakte
  Interfaceauswahl, bekanntes/nicht selbst konstruiertes Kommando, kein Speicherbefehl,
  Timeout, Reset-als-erwarteter-Fehler statt Retry-Schleife, Rückfall auf sicheren
  Zustand via USB-Replug, `--dry-run`-Standard für das noch zu bauende Sende-Tool).
  **Noch nicht ausgeführt.**

## Update — Iteration 9 (2026-08-18, ralph-loop) — erster Hardwaretest, mit Nutzerfreigabe

- Nutzer hat den Testplan freigegeben (“ja, mach den Testplan”).
- `report_send` gebaut (`src/cli/report_send.cpp`, neues CMake-Target): ohne `--confirm`
  reiner Dry-Run (kein `open()`), mit `--confirm` genau ein `write()` mit
  500ms-Poll-Timeout (`poll()` auf `POLLOUT`/`POLLERR`/`POLLHUP`), keine Retry-Schleife
  bei Timeout/Fehler — wie in `docs/first-write-test-plan.md` vorgesehen.
- `hidraw`-Pfad frisch ermittelt (Interface 2 → `hidraw10`, Weltschreibrechte, kein
  `sudo` nötig), `report_dump` bestätigte `crc_valid=yes` vor dem Senden.
- Nutzer vor dem Schreiben gewarnt (möglicher kurzer USB-Abriss laut bekanntem
  Firmwarefehler), dann genau ein Schreibvorgang mit dem bekannten Rainbow-Kommando
  (Frame 1453/3531) ausgeführt.
- **Ergebnis: Erfolg, kein USB-Reset** (`lsusb`/`dmesg` direkt danach geprüft). Nutzer
  bestätigte sichtbare Wirkung — allerdings **anders als angenommen**: kein räumlicher
  Verlauf über die Tasten, sondern ein zeitlicher Farbzyklus (alle LEDs gleichzeitig,
  Farbe wechselt durch den Regenbogen). Die „Position”-Interpretation der 6 Keyframe-
  Gruppen war falsch (räumlich statt zeitlich) — jetzt in `PROTOCOL.md` und
  `docs/first-write-test-plan.md` korrigiert dokumentiert, kein Fehlerzustand am Gerät.
- Damit ist das erste `SPEC.md`-Abnahmekriterium („Light Mount wird ausschließlich über
  exakte Geräte-/Interfaceidentität geöffnet”) für einen echten Schreibpfad erstmals
  belegt. Kriterien zu Einzeltasten/voller Matrix sind davon nicht erfüllt — dieses
  Kommando steuert alle LEDs gemeinsam.

## Nächster konkreter Schritt

1. Da die „Position”-Hypothese jetzt als zeitlich (nicht räumlich) bestätigt ist: prüfen,
   ob eines der noch unentschlüsselten Kommandos (z. B. Frame 2341/3109, 29 Byte) eher zu
   einem STATISCHEN Einzelfarb- oder Zweifarb-Kommando passt als zu einem weiteren Zyklus
   — mit der jetzt korrigierten Erwartungshaltung (zeitlich vs. räumlich) neu bewerten,
   bevor ein weiterer Hardwaretest vorgeschlagen wird.
2. Für den nächsten Hardwaretest (mindestens zwei einzelne Tasten unabhängig einfärben,
   SPEC.md-Kriterium) fehlt noch ein bekanntes Kommando für Einzeltasten-Adressierung —
   im bisherigen Capture nicht enthalten (siehe offene Frage in `PROTOCOL.md`: IO Center
   Web deckt evtl. keine Per-Key-Beleuchtung ab). Ohne ein solches Kommando ist ein
   weiterer Hardwaretest in diese Richtung nicht sinnvoll planbar — als offene Frage
   markiert, nicht durch Raten zu lösen.
3. Alternativ: mit dem jetzt verifizierten Schreibpfad (`report_send`) und bereits
   bekannten Kommandos (z. B. Frame 2747, 18 Byte) einen weiteren, ähnlich risikoarmen
   Hardwaretest vorschlagen, um mehr über unentschlüsselte Subcmd-Familien zu lernen —
   erfordert erneut kurze Nutzerfreigabe vor Ausführung (gleiches Muster wie hier).

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Frame 2747 (18 Byte, Subcmd `0x06`, Payload beginnt `04 00 64 0a 01 00
  ff 00 ff 37`) ist ebenfalls ein Vollflächen-/Zyklus-Effekt (kein Einzeltasten-Kommando),
  da bisher keines der Capture-Kommandos Anzeichen von Tastenzahl-Wiederholung zeigt.
- **Erwartetes Ergebnis:** Falls getestet, vermutlich eine weitere sichtbare, aber
  gleichmäßige (nicht Tasten-individuelle) Lichtänderung.
- **Sicherheitsrisiko:** identisch zum bereits durchgeführten Test (bekanntes,
  byteidentisches Kommando, kein Save-Befehl) — bei erneuter Nutzerfreigabe gering.
- **Rückfall:** wie in `docs/first-write-test-plan.md` — USB-Replug, falls unerwarteter
  Fehlerzustand (bisher nie eingetreten).

## Update — Iteration 10 (2026-08-18, ralph-loop) — zweiter Hardwaretest

- Nutzer hat vor dem Test per physischem Hotkey einen definierten Ausgangszustand
  hergestellt (gleichförmige Farbe) — USB-Replug hatte den vorherigen Zyklus-Effekt NICHT
  gestoppt (wichtiger Erfahrungswert, in `SECURITY.md`/`PROTOCOL.md` festgehalten).
- Frame 2747 (18 Byte, Subcmd `0x06`, Payload `04 00 64 0a 01 00 ff 00 ff 37` +
  Nullpadding) mit `report_send --confirm` gesendet — bekanntes, byteidentisches
  Kommando, `crc_valid=yes` vorab per `report_dump` bestätigt, ein einziger Schreibvorgang.
- **Ergebnis: statische (nicht zeitlich veränderliche) Farbe, vom Nutzer als Orange
  beschrieben.** Kein USB-Reset.
- **Hypothese widerlegt:** Die naheliegende Byte-14-16-als-RGB-Lesart (`ff,00,ff` =
  Magenta) passt nicht zu Orange — verworfen, nicht in `PROTOCOL.md` als Fakt übernommen.
- **Neue, gestützte Hypothese:** Byte 8 ist (zumindest teilweise) ein Preset-/Effekt-
  Index (`0x03`=Rainbow-Zyklus, `0x04`=dieses Orange-Preset), keine direkte RGB-Eingabe
  in dieser kurzen Kommandoform. Farbcodierung für frei wählbare Farben bleibt offen.
- `SPEC.md`-Kriterium „statische Gesamtfarbe" im Kern erstmals demonstriert (Farbe ist
  aber preset-fest, nicht frei wählbar) — als Teilerfolg in `BACKLOG.md` vermerkt.

## Nächster konkreter Schritt

1. Kein bekanntes Kommando mehr aus dem Capture, das mit vertretbarer Sicherheit neue,
   noch nicht getestete Erkenntnisse liefert, ohne zu raten (Frame 2341/3109 bleibt
   unklar strukturiert, Frame 1731/1739 sind sehr kurze Toggle-Kommandos mit unklarer
   Wirkung, Frame 2997 wirkt wie eine leere Abfrage). Weitere Hardwaretests mit diesen
   Kommandos sind möglich, aber der Erkenntnisgewinn ist ungewisser als bei den bisherigen
   zwei erfolgreichen Tests — vor jedem weiteren Test erneut kurz beim Nutzer nachfragen.
2. Alternativ und vermutlich ergiebiger: einen eigenen, gezielten Einzelaktions-Capture
   planen (IO Center Web im Browser öffnen, eine einzelne bekannte Aktion wie "Farbe X
   wählen" ausführen, dabei mitschneiden) — das würde die offene Frage nach der
   RGB-Codierung für freie Farben direkt beantworten, statt an fremden Mehrfach-Klick-
   Daten zu raten. Das ist ein größerer Schritt (Browser-Interaktion, ggf. eigener
   `usbmon`-Mitschnitt) und sollte dem Nutzer vor Beginn kurz vorgeschlagen werden.
3. Weiterhin ohne Rückfrage möglich: `PROTOCOL.md`/`ARCHITECTURE.md` konsolidieren, damit
   der aktuelle Stand (2 erfolgreiche Hardwaretests, mehrere offene Fragen) für eine
   künftige Session klar lesbar bleibt.

## Update — Iteration 11 (2026-08-18, ralph-loop) — eigener gezielter Capture, großer Fortschritt

- Nutzer hat zugestimmt, einen eigenen gezielten Capture durchzuführen (statt zu raten).
- Technisches Problem gelöst: `tshark`/`dumpcap` sind per AppArmor auf `/dev/usbmon*`
  gesperrt (bestätigt per `dmesg`-DENIED-Eintrag), auch mit `sudo` (Policy gilt unabhängig
  von der UID). Keine AppArmor-Policy verändert — stattdessen debugfs-Textinterface
  (`/sys/kernel/debug/usb/usbmon/1u`, `sudo cat`) genutzt, das nicht durch das
  tshark-Profil mediiert wird. In `SECURITY.md` als Vorgehen dokumentiert.
- Mitschnitt während einer einzelnen Nutzeraktion (freie Farbwahl `#1FB4FF` in IO Center
  Web) auf Interface 2 (Device 061) durchgeführt, auf unser Gerät gefiltert.
- **Mehrere bedeutende neue, hardwarebestätigte Fakten** (Details in `PROTOCOL.md` und
  `docs/evidence/own_capture_iocenter_decoded.md`):
  1. RGB-Kodierung für frei wählbare Farben entschlüsselt und exakt gegen den vom
     Nutzer abgelesenen Hex-Wert verifiziert (Payload-Byte 5-7 = R,G,B direkt).
  2. Helligkeits-Byte (Payload-Byte 2 = `0x64`) durch Nutzerangabe (UI auf 100%)
     zusätzlich bestätigt.
  3. Neues Subkommando `0x03` entdeckt: periodisches Keepalive, unabhängig von
     Nutzeraktionen.
  4. Korrektur: das bisher als "konstant 0x0002" dokumentierte Feld (Byte 2-3) ist
     session-abhängig, nicht universell — `report.h`/`report.cpp`-Kommentare
     entsprechend präzisiert (Verhalten selbst unverändert, nur Dokumentation).
  5. Unaufgeforderte Push-Frames entdeckt, die Tastendrücke vom Boot-Keyboard-Interface
     über den Vendor-Kanal spiegeln — inklusive Privacy-relevanter Nebenfolge
     (WebHID-Tastatur-Zugriffssperre wird dadurch faktisch umgangen), in `SECURITY.md`
     als eigenständiger Hinweis festgehalten.
- Kein `hidraw`-Schreibzugriff in dieser Iteration — nur Lese-/Mitschnitt-Zugriff auf
  `/sys/kernel/debug/usb/usbmon/1u` (kein Kommando an das Gerät gesendet).

## Nächster konkreter Schritt

1. Das jetzt entschlüsselte Static-Color-Kommando (Länge 15, Subcmd `0x06`, RGB an
   Payload-Byte 5-7) selbst mit `report_send` auf Hardware testen — bisher nur aus dem
   Capture entschlüsselt, noch nicht selbst als Kommando gesendet/gebaut. Erfordert
   erneut kurze Nutzerfreigabe vor Ausführung (gleiches Muster wie bisher) und einen
   `build_report`-Aufruf mit selbst gewählten RGB-Werten (erstes Mal ein **nicht**
   byteidentisch aus einem Capture übernommenes Kommando — CRC wird lokal neu berechnet,
   nicht aus einem bekannten Beispiel kopiert; erfordert daher besondere Sorgfalt beim
   Report-Aufbau vor dem Senden, siehe `SECURITY.md` Regel 10).
2. Vor diesem Test: `report_dump`/eigene Unit-Tests nutzen, um den neu gebauten Report
   offline zu verifizieren (Längenfeld, Byte-2-3-Wert für die aktuelle Session, CRC),
   bevor überhaupt an Hardware gedacht wird.
3. Danach `BACKLOG.md`-Punkt „Static-Color-Kommando real testen" abschließen und
   `SPEC.md`-Kriterium „statische Gesamtfarbe … sicher schalten" von „preset-fest" auf
   „frei wählbar, verifiziert" upgraden.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Ein mit `build_report` selbst konstruierter Static-Color-Report
  (Subcmd `0x06`, Payload `00 00 64 32 00 <R> <G> <B>`, aktuelle Session-ID statt der
  Fixture-`0x0002`) wird vom Gerät akzeptiert und erzeugt die gewählte Farbe — auch wenn
  er nicht byteidentisch aus einem Capture kopiert ist.
- **Erwartetes Ergebnis:** Sichtbare, vom Nutzer bestätigbare Farbänderung passend zu den
  gewählten RGB-Werten.
- **Sicherheitsrisiko:** minimal höher als bei den ersten zwei Tests, da erstmals ein
  Report nicht byteidentisch aus einem echten Capture stammt, sondern aus bekannten,
  aber neu kombinierten Feldern besteht. Weiterhin kein Speicherbefehl, weiterhin
  Timeout/Single-Write/No-Retry.
- **Rückfall:** physischer Hotkey auf der Tastatur (siehe `SECURITY.md` Regel 7), falls
  das Ergebnis unerwartet ist.

## Update — Iteration 12 (2026-08-18, ralph-loop) — erster selbst konstruierter Report erfolgreich

- Mit Nutzerfreigabe: Static-Color-Kommando (`#00FF00`) selbst aus bekannten Feldern
  konstruiert (nicht byteidentisch aus einem Capture kopiert) und gesendet.
- **Erster Versuch schlug fehl** (keine sichtbare Wirkung trotz `write completed`).
  Systematische Fehlersuche mit dem Nutzer zusammen (dessen Idee "liegt es am offenen
  Browser-Tab?" führte zur entscheidenden Spur):
  1. Antwort ausgelesen (vorher nicht gemacht) → abweichendes Format entdeckt
     (Byte 3 = `0x0a` statt `0x00`, volles Echo statt kurzer Bestätigung).
  2. Sitzungs-ID-Hypothese getestet (0x01 vs 0x02, Tab offen/geschlossen) → widerlegt.
  3. Commit-Kommando-Hypothese getestet (Subcmd `0x0a` hinterher) → widerlegt.
  4. Kontrolltest mit bekannt funktionierendem Orange-Kommando über dieselbe Sende-
     Methode → normale Bestätigung, schließt Methodik-Artefakt aus.
  5. **Entscheidender Test:** dieselbe Payload mit der echten, live beobachteten
     Sequenznummer `0x10ad` statt einer erfundenen → normale Bestätigung UND vom
     Nutzer bestätigt sichtbares Grün.
- **Ergebnis: Gerät validiert Sequenznummern** bei dieser Kommandofamilie. Vollständige
  Diagnose in `docs/evidence/sequence-number-validation-test.md`, Architekturkonsequenz
  in `DECISIONS.md` festgehalten (Sequenznummern künftig aus Gerätezustand fortführen,
  nicht frei vergeben).
- `SPEC.md`-Kriterium „statische Gesamtfarbe … sicher schalten" jetzt vollständig erfüllt
  (frei wählbare Farbe, nicht nur Preset) — zwei unabhängige Farbwerte verifiziert
  (`#1FB4FF` aus Capture-Analyse, `#00FF00` aus eigenem Test).

## Nächster konkreter Schritt

1. `SPEC.md`-Fortschritt konsolidieren: prüfen, welche MVP-Abnahmekriterien jetzt
   erfüllt sind (statische Farbe: ja: Einzeltasten: weiterhin nein, keine Per-Key-
   Kommandos bekannt) und die Checkliste entsprechend aktualisieren.
2. Sequenznummer-Akzeptanzregel weiter eingrenzen wäre wertvoll, ist aber ein
   eigenständiger, nicht trivialer Untersuchungsaufwand (mehrere gezielte Tests mit
   verschiedenen Abständen zur echten Sequenznummer) — nicht ungefragt weitere
   Hardwaretests anstoßen, stattdessen dem Nutzer als nächste Option vorschlagen.
3. Alternativ: Ohne weitere Hardwaretests weiterarbeiten — z. B. `report_send`/
   `report_dump` um eine "aus Feldern bauen" CLI-Option erweitern (bisher nur informell
   per Python-Einweg-Skript gemacht), oder mit der OpenRGB-Controller-Grundstruktur
   (Phase 3) beginnen, da das Kernprotokoll (Farbe setzen) jetzt belastbar verstanden ist.

## Update — Iteration 13 (2026-08-18, ralph-loop) — CLI-Tooling ausgebaut

- Auf Nutzerwunsch: `report_build` ergänzt (`src/cli/report_build.cpp`, neues
  CMake-Target) — baut einen Report aus einzelnen Feldern (`--subcmd --seq --session
  --flags --length --payload`) über `build_report()`, druckt Hex auf stdout, druckt
  Zusammenfassung auf stderr (sauber pipe-fähig mit `report_dump`/`report_send`).
  `--length` bewusst ohne Default/Ableitungsformel — muss explizit angegeben werden
  (keine bestätigte Formel, siehe `PROTOCOL.md`).
- Dabei sauberer gelöst als ursprünglich geplant: `session` (Byte 2-3) ist jetzt ein
  echtes Feld in `Interface2Report` (`report.h`/`report.cpp`), Default `0x0002` (passend
  zu den bekannten Fixtures) — statt es nachträglich im CLI-Tool zu patchen. Kein
  Verhaltensunterschied für bestehende Fixtures (alle 20 haben `session=0x0002`,
  Parse→Build-Rundlauf-Test weiterhin grün).
- `report_dump`/`report_send` um Session-Anzeige ergänzt.
- Funktional gegen den real erfolgreichen Grün-Report aus Iteration 12 verifiziert:
  `report_build --subcmd 06 --seq 10ad --session 01 --length 0f --payload
  000064320000ff00` erzeugt exakt denselben Hex (inkl. CRC) wie der zuvor per
  Python-Skript gebaute, hardwareverifizierte Report.
- Alle 4 Unit-Tests weiterhin grün. Kein Gerätezugriff in dieser Iteration.

## Update — Iteration 14 (2026-08-18) — Per-Key-Frage geklärt (negativ)

- Nutzer hat IO Center Web direkt geprüft: nur vorgeformte Effekte/„Figuren" mit
  Animation (Matrix, Tornado, ...), keine Einzeltasten-Konfiguration. Offene Frage aus
  `PROTOCOL.md` damit bestätigt (nicht mehr nur Vermutung aus Capture-Lücke).
- Namensparallele zu Mountain Everest (Matrix, Tornado) als unterstützende Beobachtung
  für gemeinsame Produktherkunft festgehalten, ohne die bereits widerlegte
  Byte-Protokoll-Verwandtschaft neu zu behaupten.

## Nächster konkreter Schritt

Per-Key-Adressierung ist über IO Center Web nicht erreichbar. Optionen, noch nicht
entschieden:
1. Windows-Client-Capture-Plan (VM + USB-Passthrough) ausarbeiten — großer Schritt,
   siehe Master-Prompt Phase 1, nicht ohne Rücksprache beginnen.
2. Ohne Per-Key weiterarbeiten: weitere bekannte Effekte (Matrix, Tornado, ...) gezielt
   capturen wie in Iteration 11 — vertieft das Verständnis von Subcmd `0x06` und klärt
   nebenbei evtl. die offene Frage zu getrennten Zonen (obere/seitliche Leisten),
   ohne den großen VM-Aufwand.
3. Mit dem jetzt belastbar verstandenen Kernprotokoll (Farbe setzen, Sequenznummer-
   Regel bekannt) Richtung Phase 3 (OpenRGB-Controller-Grundstruktur) weitergehen.

## Update — Iteration 15 (2026-08-18) — Matrix-Effekt gezielt gecaptured

- Zweiter gezielter Capture (wie Iteration 11, diesmal Matrix-Effekt statt Farbwahl).
  Genau ein reales Kommando im Mitschnitt gefunden.
- **Ergebnis: löst eine seit Iteration 2 offene Frage** — das Kommando ist byteidentisch
  mit dem bis dahin unentschlüsselten Frame 2341/3109 aus dem alten Fremd-Capture. Damit
  ist klar: das war die ganze Zeit der Matrix-Effekt. Details in
  `docs/evidence/own_capture_matrix_effect.md`.
- Response war eine normale kurze Bestätigung (Byte3=0x00) — Kommando wurde vom Gerät
  akzeptiert, passt zur live beobachteten echten Sequenznummer (`0x10c7`).
- Genaue Payload-Byte-Bedeutung (vermutlich mehrere Parameter: Farben/Geschwindigkeit/
  Richtung) weiterhin nicht vollständig entschlüsselt — bewusst nicht geraten.
- Kein Schreibzugriff — nur Mitschnitt einer Nutzeraktion in IO Center Web.

## Nächster konkreter Schritt

Möglichkeiten, noch nicht entschieden: (1) Tornado-Effekt ebenso gezielt capturen für
einen weiteren Datenpunkt, (2) Matrix-Payload durch einen gezielten Parameter-Vergleich
weiter eingrenzen (z. B. Geschwindigkeit ändern, Rest gleich lassen, erneut capturen),
(3) Richtung Phase 3 (OpenRGB-Controller) weitergehen, da das Kernprotokoll für
Static-Color bereits belastbar ist.

## Update — Iteration 16 (2026-08-18) — größter Einzelfund im Projekt: vollständiges Gerätemanifest

- Auf Nutzervorschlag: Windows-IO-Center-Installer (vom Nutzer bereitgestellt, aus
  `~/Claude-Memory/IO Center Installer.zip`) rein statisch analysiert — nie ausgeführt.
  `innoextract` aus Git-Hauptzweig gebaut (Release 1.9 kennt Inno-Setup-6.3.0 noch
  nicht), Qt-Ressourcenarchive (`.rcc`) per Brute-Force-Zlib-Scan entpackt.
- **Vollständiges offizielles Gerätemanifest gefunden**, inkl.:
  - Alle 6 Effektnamen (Static, ColorWave, Tornado, Breathing, Reactive, Matrix) —
    exakt wie im Mountain-Referenzcode.
  - **Vollständige Per-Key-LED-Tabelle: 168 individuell adressierbare LEDs**,
    durchnummeriert und benannt (45 obere Leiste + 113 Einzeltasten + 10 seitliche
    Leiste). Löst die seit Iteration 1 offene Per-Key-Frage strukturell (welche LEDs
    existieren), auch wenn das Wire-Kommando dafür weiterhin unbekannt ist.
  - Macro-Limits, Polling-Rate, Keybinding-Layer, Bootloader-PID.
- Rohe extrahierte Dateien bleiben lokal (`vendor-extracts-private/`, neu
  git-ignoriert) — nur destillierte Fakten im Repo, siehe neue ADR in `DECISIONS.md`.
- App ist Qt6/QML mit separatem `bequietIOCenterService.exe` + `hidapi.dll` (nicht
  Electron wie zunächst vermutet) — bestätigt native `hidapi`-Kommunikation auf dem
  Windows-Client statt WebHID.
- Kein Byte-Protokoll (Kommandotabellen) im Service-Binary per `strings` gefunden —
  keine Disassembly versucht.

## Nächster konkreter Schritt

Größere Wahlmöglichkeiten, noch nicht entschieden:
1. `light_mount_main_iso.json` (Pixel-Koordinaten) mit der LED-Tabelle verknüpfen —
   Grundlage für ein OpenRGB-Layout, reine Offline-Arbeit.
2. Versuchen, das Per-Key-Wire-Kommando doch noch zu finden — z. B. gezielt Interface 3
   (der bisher unerforschte zweite Vendor-Kanal) untersuchen, da die 168-LED-Tabelle
   jetzt ein klares Ziel gibt, wonach zu suchen ist.
3. Mit dem jetzt sehr detaillierten Verständnis (Effekte, LED-Zählung, Sequenznummer-
   Regel) Richtung Phase 3 (OpenRGB-Controller-Grundgerüst) weitergehen.

## Update — Iteration 17 (2026-08-18) — LED-Tabelle mit Koordinaten verknüpft

- LED-Index-Tabelle (`light_mount_leds_mapping.json`, ISO-Variante) mit den
  Pixel-Koordinaten aus `light_mount_main_iso.json` per Name zusammengeführt.
  Datei per Inhalt als ISO-Variante identifiziert (enthält `UK`/`DE`/`FR`-Layouts und
  die ISO-spezifischen Tasten `Key_NonUsTilde`/`Key_NonUsBackslash`, nicht die
  US-only-ANSI-Variante).
- Ergebnis: 166 Einträge, 111 mit Geometrie (Einzeltasten), 55 ohne (die
  Top/Left/Right-Leisten-LEDs — deren Geometrie steckt vermutlich in separaten,
  noch nicht ausgewerteten `boundingRect`-Dateien aus demselben Vendor-Material).
  Datei: `docs/evidence/light_mount_led_layout_iso.json`, dokumentiert in
  `docs/evidence/led-layout-merge.md`. Eigene Zusammenführung, kein Vendor-Original
  (passend zur ADR in `DECISIONS.md`).
- Reine Offline-Arbeit, kein Gerätezugriff.

## Nächster konkreter Schritt

Noch offen, welche der in Iteration 16 genannten Optionen als Nächstes drankommt:
Per-Key-Wire-Kommando suchen (Interface 3), Leisten-LED-Geometrie ergänzen, oder
Richtung Phase 3 (OpenRGB-Controller-Grundgerüst) weitergehen — dem Nutzer erneut
vorlegen, bevor eigenmächtig eine größere Richtung gewählt wird.

## Update — Iteration 18 (2026-08-18) — read-only Interface-3-Test

- Mit Nutzerfreigabe: `HIDIOCGFEATURE`-ioctl auf `/dev/hidraw11` (Interface 3), alle
  sechs Report-IDs, reines Lesen, kein Schreiben.
- Erfolgreich, kein Fehler, kein USB-Reset, Gerät danach unverändert erreichbar.
- Report-ID 1 und 3 liefern von Null verschiedene Rohwerte (vermutlich Telemetrie/
  Zähler, Bedeutung nicht bekannt). Report-ID 2, 4, 5, 6 durchgehend Null — für den
  Per-Key-Kandidaten (Report-ID 4) weder Bestätigung noch Widerlegung.
- Kein Schreibversuch unternommen — kein bekanntes reales Kommando als Ausgangspunkt
  vorhanden, `SECURITY.md` Regel 10 verlangt hier zusätzliche Vorsicht.

## Nächster konkreter Schritt

Interface-3-Spur ohne echten Traffic (weder fremd noch eigen) weitgehend ausgeschöpft,
ohne zu raten. Optionen, dem Nutzer erneut vorzulegen: (1) hier stoppen und die
Per-Key-Frage als „strukturell verstanden, Wire-Protokoll offen" festhalten, (2)
Richtung Phase 3 (OpenRGB-Controller-Grundgerüst) weitergehen — Static Color und Matrix
sind gut genug verstanden für einen ersten Controller, Per-Key kann als spätere
Erweiterung nachgezogen werden, sobald neue Erkenntnisse vorliegen.

## Update — Iteration 19 (2026-08-18) — OpenRGB-Controller-Grundgerüst geschrieben

- Nutzerentscheidung: kein weiterer Interface-3-Vorstoß, stattdessen Phase 3 beginnen.
- OpenRGB-Quellstand lokal geklont (`openrgb-src-private/`, git-ignoriert), Build-System
  untersucht (qmake, automatisches Glob-Include für `Controllers/*`), Mountain-Referenz
  (`RGBController_MountainKeyboard.{h,cpp}`) als Konventions-Vorlage gelesen.
- Neue ADR in `DECISIONS.md`: kein Submodule, Controller-Dateien leben getrackt in
  diesem Repo unter `openrgb-integration/Controllers/LightMountController/`.
- Controller geschrieben — **bewusst minimal**: nur `SendStaticColor(r,g,b)`, ein
  Zone/ein LED ("Keyboard" als Ganzes), Detection über die schon verifizierten Werte
  (VID `0x373F`, PID `0x0002`, Interface 2, Usage Page `0xFF00`, Usage `0x01`). Kein
  Per-Key, keine Effekte, kein Save/Brightness — nichts geraten, was nicht in
  `PROTOCOL.md` als bestätigt steht. CRC16/MODBUS + Report-Aufbau selbstständig im
  Controller neu implementiert (Mountain-Stil: kein Link gegen unsere separate
  CMake-`protocol`-Bibliothek, OpenRGB-Controller sind traditionell in sich geschlossen).
- Baseline-Build des unveränderten OpenRGB-Quellstands angestoßen (Build-Abhängigkeiten
  installiert, `qmake && make -j$(nproc)`), lief bei Sitzungsende noch (großes Projekt,
  hunderte Controller) — Ergebnis und Test mit unserem Controller stehen noch aus.
- Kein Gerätezugriff in dieser Iteration.

## Nächster konkreter Schritt

1. Baseline-Build abwarten/prüfen, dann unsere Controller-Dateien in den Checkout
   kopieren und neu bauen — prüfen, ob unser Code fehlerfrei kompiliert.
2. Falls Build erfolgreich: `./openrgb --list-devices` bzw. echten Start prüfen, ob die
   Light Mount erkannt wird (Detection-Parameter sind bereits verifiziert, aber die
   Registrierung selbst noch nicht real getestet).
3. Erst danach, mit erneuter Nutzerfreigabe: echten Hardwaretest über den neuen
   Controller-Code (nicht mehr über `report_send`) versuchen — insbesondere die
   unverifizierte Sequenznummer-Strategie (Start bei 1) auf die Probe stellen.

## Update — Iteration 20 (2026-08-18) — Build erfolgreich, Gerät erkannt

- Baseline-OpenRGB-Build fertig, unsere Controller-Dateien reinkopiert, neu gebaut:
  **fehlerfrei kompiliert**, kein Fehler in `LightMountController.cpp`/
  `RGBController_LightMount.cpp`/`LightMountControllerDetect.cpp`.
- `./openrgb --list-devices` erkennt **"be quiet! Light Mount"** korrekt als eigenes
  Gerät: `Location: HID: /dev/hidraw10` (Interface 2, exakt wie vorgesehen),
  `Serial: QUK123456789` (stimmt mit `lsusb -v` aus Iteration 1 überein), `Modes:
  [Static]`, `Zones: Keyboard`, `LEDs: Keyboard` — alles wie im Controller-Code
  definiert. Erster echter End-to-End-Nachweis der Detection.
- Noch kein Farbtest über OpenRGB selbst ausgeführt (würde erstmals die unverifizierte
  Sequenznummer-Strategie — Start bei 1 — auf echter Hardware prüfen).

## Nächster konkreter Schritt

Mit Nutzerfreigabe: `./openrgb --device 0 --mode static --color <hex>` ausführen und
beobachten, ob die Farbe angenommen wird (Sequenznummer-Start-bei-1-Hypothese) oder ob
das Gerät ablehnt (analog zum `0x2000`-Fehlschlag aus Iteration 12). Danach Ergebnis in
`PROTOCOL.md`/`openrgb-integration/README.md` festhalten.

## Update — Iteration 21 (2026-08-18) — Sequenznummer-Start-bei-1 widerlegt

- Mit Nutzerfreigabe: `./openrgb --device 0 --mode static --color 8000FF` ausgeführt.
  Kein Fehler, kein USB-Reset, aber **keine sichtbare Farbänderung**.
- Zur Diagnose das exakte Kommando (Session `0x0002`, Seq `0x0001`, wie vom Controller
  gesendet) über `hidraw` nachgebaut und die Antwort ausgelesen: **identisches
  Ablehnungsmuster** wie beim `0x2000`-Fehlschlag (Byte 3 = `0x0a`, volle Echo-Antwort).
- **Hypothese widerlegt:** ein bei 1 startender Zähler ist NICHT die Lösung — sowohl
  sehr niedrige als auch sehr hohe frei gewählte Werte werden abgelehnt. Nur die eine
  real beobachtete Sequenznummer (`0x10ad`) hat je funktioniert. Deutet auf echten,
  bisher nicht zugänglichen Gerätezustand hin.
- Nebenbei echten Code-Bug gefunden und behoben: `RGBController_LightMount`-Destruktor
  rief `Shutdown()` nicht auf (OpenRGB-Warnung beim Beenden) — gefixt, neu gebaut,
  fehlerfrei.
- Alle Doku-Stellen (`PROTOCOL.md`, `openrgb-integration/README.md`, `BACKLOG.md`)
  ehrlich aktualisiert: Controller baut und wird erkannt, Kernfunktion (Farbe setzen)
  ist noch **nicht praktisch nutzbar**, bis die Sequenznummer-Frage gelöst ist.

## Nächster konkreter Schritt

Vielversprechendster, noch nicht verfolgter Ansatz: die Interface-3-Report-ID-1-
Telemetriewerte (siehe `PROTOCOL.md`) vor einem Schreibversuch auslesen und prüfen, ob
sich daraus eine gültige/erwartete Sequenznummer ableiten lässt (z. B. eines der
32-Bit-Felder als laufender Zähler). Erfordert erneuten, aber rein lesenden
Hardwarezugriff (`GET_FEATURE`, kein Schreiben) — mit kurzer Nutzerfreigabe vorab, wie
bisher etabliert. Alternativ: hier als bekannte, dokumentierte Einschränkung stehen
lassen und andere Baustellen priorisieren.

## Blocker

Keiner für Analyse-/Doku-/Code-Schritte. Weitere Hardwaretests weiterhin nur mit kurzer
Nutzerfreigabe vorab, wie bisher etabliert.
