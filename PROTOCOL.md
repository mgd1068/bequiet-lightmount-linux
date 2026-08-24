# PROTOCOL

Bestätigte Fakten und Hypothesen werden strikt getrennt. Nichts aus diesem Dokument
gilt als belegt, ohne dass die Quelle (Deskriptor, Capture, realer Test) genannt ist.

## Bestätigte Fakten

| Fakt | Quelle |
|---|---|
| USB VID:PID = `373f:0002` | Bekannter Ausgangsstand / OpenRGB-Ticket #4950 — lokal per `lsusb -d 373f:0002` bestätigt (2026-08-18, `docs/evidence/lsusb_v.txt`) |
| Gerätebezeichnung "be quiet! Light Mount", bcdDevice 23.00 | `lsusb -v`, lokal verifiziert |
| USB 1.1 Full Speed (12 Mbps negotiated), max. 64 Byte/Endpoint-Paket | Bekannter Ausgangsstand, lokal bestätigt |
| Vier HID-Interfaces (0–3), alle Klasse HID, alle vom Kernel-`usbhid`-Treiber gebunden | Lokal per `lsusb -t` + sysfs bestätigt |
| Interface 2: EP `0x83` IN (64B) + EP `0x04` OUT (64B), Interrupt | Bekannter Ausgangsstand — **jetzt lokal per `lsusb -v` bestätigt** |
| Interface 2 Report Descriptor: Usage Page **Vendor Defined (0xFF00)**, Usage 0x01, Collection Application, **kein Report-ID-Byte**, Input/Output je 64 Byte (Usage 0x02/0x03), zusätzlich ein 64-Byte **Feature Report** (Usage 0xFF00, via Control Transfer) | `docs/evidence/report_descriptor_if2.hex`, sysfs `report_descriptor`, 2026-08-18 |
| Interface 0: Standard USB-HID-**Boot-Keyboard**-Deskriptor (Usage Page Generic Desktop/Keyboard, Modifier-Byte, 3 LED-Output-Bits, 6-Byte-Key-Array) — normaler Tastatur-Traffic, nicht der Konfigurationskanal | `docs/evidence/report_descriptor_if0.hex`, sysfs, 2026-08-18 |
| Interface 1: Composite-Deskriptor mit mehreren Report-IDs: ID 1 = Keyboard (Modifier + 160-Bit-NKRO-Bitmap), ID 3/4 = Consumer-Control-Seite (Medientasten, u.a. bis Usage 0x023c), ID 0x0a = eigene Collection mit Buttons + Wheel/X/Y (Generic Desktop, Usage 0x38/0x30/0x31) | `docs/evidence/report_descriptor_if1.hex`, sysfs, 2026-08-18 |
| Interface 3: Vendor Usage Page `0x59`, sechs Report-IDs (1–6), überwiegend **Feature Reports** (Control Transfer, kein passender Output-Endpoint — Interface hat laut `lsusb -v` nur einen IN-Interrupt-EP `0x85`/4B) | `docs/evidence/report_descriptor_if3.hex`, sysfs, 2026-08-18 |
| **Korrektur (2026-08-18):** die wiederkehrenden Usage-Werte `0x51 0x52 0x53 0x54` gehören zu **Report ID 4** (nicht 3, frühere Notiz war falsch) und wiederholen sich **8×** (32 Usage-Tags gesamt, nicht 128), gefolgt von einem **32-Byte**-Feature-Segment (Report Count 32, Report Size 8). Report ID 4 enthält davor zusätzlich ein 2-Byte- (Usage `0x03`/`0x55`, Logical Max 8) und ein 16-Byte-Segment (Usage `0x21`, 8×16-Bit) — insgesamt 51 Byte Feature-Daten (inkl. Report-ID-Byte) | erneute manuelle Dekodierung von `report_descriptor_if3.hex`, 2026-08-18 |
| Jedes Interface hat ein eigenes `hidraw`-Device: if0→hidraw8, if1→hidraw9, if2→hidraw10, if3→hidraw11 (Nummern hostabhängig, nicht stabil) | sysfs `.../hidraw/`, 2026-08-18, **nicht geöffnet**, nur Pfad gelesen |

## Bestätigt durch Traffic-Capture (usbmon3, OpenRGB Issue #4950)

Fremder, öffentlicher Capture aus dem OpenRGB-Ticket (Nutzer klickt in IO Center Web
verschiedene Optionen an), offline analysiert am 2026-08-18 mit `tshark` (Filter auf
Bus 3 Device 2, Light-Mount-Adresse laut Capture-Metadaten). Details, vollständige
Rohdaten und Zuordnung: `docs/evidence/usbmon3_decoded_commands.txt`.

| Fakt | Beleg |
|---|---|
| **Interface 2 ist der tatsächlich genutzte Konfigurationskanal** — alle 10 beobachteten Kommando/Antwort-Paare laufen über EP4 OUT / EP3 IN (Interrupt), keine Control-Transfer-Aktivität auf EP0 während der Interaktion | `usbmon3_capture.pcapng`, 20 Frames, 2026-08-18 |
| Kommando-Header-Struktur: `[len u16 LE][0x0002][seq u16 LE][subcmd][flags]…[checksum u16, letzte 2 Byte]`; Sequenzzähler steigt über alle Kommandos hinweg monoton (0x103e…0x1047); Antworten sind immer 6 Byte lang und echoen Sequenz/Subcmd exakt | 10/10 Kommandos, 10/10 Antworten konsistent, `docs/evidence/usbmon3_decoded_commands.txt` |
| Ein konkretes 41-Byte-Kommando (zweimal byteidentisch, 11,4s auseinander gesendet, `subcmd=0x06`) enthält ab Byte 17 sechs `[Zyklus-Zeitpunkt 0–100%][R][G][B]`-Keyframes, die exakt einen Regenbogen-Farbkreis ergeben: 0%=Gelb, 17%=Grün, 33%=Cyan, 50%=Blau, 67%=Magenta, 83%=Rot — **auf echter Hardware bestätigt als zeitlicher Farbwechsel alle LEDs gleichzeitig (Rainbow-Cycle-Effekt), NICHT als räumlicher Verlauf über die Tasten** (siehe Hardwaretest-Abschnitt unten; ursprüngliche „Positions"-Lesart war falsch) | Frames 1453 & 3531, `docs/evidence/usbmon3_decoded_commands.txt`; Hardware-Korrektur 2026-08-18 |
| **Prüfsumme identifiziert:** die letzten 2 Byte jedes 64-Byte-Reports (Kommando UND Antwort) sind **CRC16/MODBUS** (Polynom `0x8005`, Init `0xFFFF`, reflektiert ein/aus, kein XOR-Out) über die ersten 62 Byte, Little-Endian angehängt. Exakter Treffer bei 20/20 bekannten Frames (10 Kommandos + 10 Antworten) — bei einer Trefferwahrscheinlichkeit von 1/65536 pro Frame praktisch ausgeschlossen, dass das Zufall ist | `docs/evidence/checksum_verification.py`, 2026-08-18, offline gegen `usbmon3_capture.pcapng` verifiziert |

**Einschränkung:** Dies ist eine Offline-Analyse eines fremden Captures, keine eigene
Hardwareverifikation. Die Rainbow-Interpretation ist plausibel und durch doppelte,
identische Übertragung gestützt, gilt aber erst nach eigenem `--dry-run`/Hardwaretest
(Phase 1/2) als bestätigt im Sinne von `SPEC.md`.

## Interface 3 als Per-Key-Kandidat (2026-08-18, rein statisch, kein Test)

Da Interface 3 in **keinem** bisherigen Capture (weder fremd noch eigen) jemals Traffic
zeigte — IO Center Web nutzt es nie — bleibt nur die erneute Deskriptor-Analyse (siehe
Korrektur oben). Report ID 4 (51 Byte Feature-Daten: 2+16+32 Byte in drei Segmenten) ist
ein plausibler Kandidat für einen Per-Key-Kanal: 32 Byte reichen aber bei weitem nicht
für 168 LEDs × 3 Byte RGB (504 Byte) — ein echtes Per-Key-Update müsste also (falls
dieser Kanal überhaupt dafür zuständig ist) über mehrere aufeinanderfolgende
`SET_FEATURE`-Aufrufe verteilt werden, ähnlich Mountains `SendColorPacketCmd`-Chunking.
**Reine Hypothese, nicht verifiziert.**

**Bewusst nicht (blind) geschrieben:** Anders als bei allen bisherigen Hardwaretests gibt
es hier kein bekanntes reales Kommando als Ausgangspunkt. Stattdessen zuerst ein rein
**lesender** Test durchgeführt (`HIDIOCGFEATURE`-ioctl auf `/dev/hidraw11`, alle sechs
Report-IDs, kein Schreiben, mit Nutzerfreigabe).

**Ergebnis (2026-08-18):** Alle sechs `GET_FEATURE`-Aufrufe erfolgreich, kein Fehler,
kein USB-Reset, Gerät danach unverändert erreichbar.

- Report-ID 1 und 3 liefern **von Null verschiedene** Daten zurück (passend zur
  Deskriptor-Struktur: mehrere 16-/32-/8-Bit-Felder). Rohwerte (little-endian dekodiert):
  - Report 1: 16-bit=135, dann 5×32-bit = 452788, 157418, 42110, 1, 33333
  - Report 3: 16-bit=0, dann 5×32-bit = 18970, 17390, 2890, 1, 2000, dann 6×8-bit =
    255,255,255,255,1,41
  - Bedeutung **nicht bekannt** — sehen nach Zählern/Telemetrie/Diagnosewerten aus
    (z. B. Uptime, Firmware-Build, interne Statistik), reine Spekulation, nicht als Fakt
    behandelt.
- Report-ID 2, 4, 5, 6 liefern **durchgehend Null** zurück. Für Report-ID 4 (der
  Per-Key-Kandidat) ist das **nicht eindeutig**: könnte bedeuten, dass der Kanal
  ungenutzt/leer ist, oder dass es sich um einen reinen Schreibkanal ohne persistenten
  Lesezustand handelt (typisch für einen Bulk-Upload-Slot). Weder bestätigt noch
  widerlegt die Per-Key-Hypothese.

Kein weiterer Test (insbesondere kein Schreiben) auf Interface 3 in dieser Iteration.

**Ergänzung (2026-08-19):** Zwei weitere risikofreie Tests durchgeführt:
- Reiner Lesetest auf den Interrupt-IN-Endpunkt (EP `0x85`, ohne `GET_FEATURE`): keine
  unaufgeforderten Daten wartend.
- Minimaler No-Op-`SET_FEATURE`-Test auf Report-ID 4: exakt die bereits per
  `GET_FEATURE` gelesenen Nullen zurückgeschrieben (kein geratener Inhalt). **Ergebnis:**
  vom Gerät akzeptiert (kein USB-Fehler, kein Reset), nachfolgendes `GET_FEATURE` zeigt
  weiterhin Nullen — kein erkennbarer Nebeneffekt. Bestätigt, dass Schreibzugriffe auf
  Interface 3 grundsätzlich funktionieren, sagt aber nichts über die Bedeutung eines
  echten (nicht-Null-)Inhalts aus.
- Kein Test mit echtem/geratenem Inhalt unternommen — ohne jeden bekannten realen
  Referenzwert wäre das reines Raten (`SECURITY.md` Regel 10). Interface-3-Spur an
  diesem Punkt ausgereizt, bis neue Daten (z. B. ein echter Per-Key-Capture) vorliegen.

## Erster eigener Hardwaretest (2026-08-18) — bestätigt UND eine Hypothese korrigiert

Durchgeführt nach `docs/first-write-test-plan.md`, mit expliziter Nutzerfreigabe. Werkzeug:
`report_send /dev/hidraw10 <bekannte 64 Byte aus Frame 1453> --confirm`. Genau ein
Schreibvorgang, kein Retry, `crc_valid=yes` vor dem Senden per `report_dump` geprüft.

- **Bestätigt:** Das Kommando aus Frame 1453/3531 (Interface 2, EP4 OUT, Subcmd `0x06`,
  CRC16/MODBUS korrekt) erzeugt auf echter Hardware eine sichtbare Beleuchtungsänderung.
  Kein USB-Reset, kein Disconnect (`lsusb` und `dmesg` direkt danach geprüft, Gerät blieb
  durchgehend erreichbar).
- **Hypothese korrigiert:** Der Nutzer beobachtete live *keinen* räumlichen Verlauf über
  die Tasten (nicht "links blau, nach rechts wechselnd"), sondern **alle Tasten
  gleichzeitig, Farbe wechselt über die Zeit** durch den vollen Regenbogen. Die 6
  `[Wert][R][G][B]`-Gruppen sind also **Keyframes eines zeitlichen Zyklus** (z. B.
  Position im Animationsloop), nicht Positionen entlang der Tastenreihen. Der frühere
  Begriff „Farbverlauf/Gradient" in den Iterationen 2–4 ist in diesem Sinn präzisiert;
  betroffene Doku-Stellen oben entsprechend markiert.
- Das erste Abnahmekriterium aus `SPEC.md` („Light Mount wird ausschließlich über exakte
  Geräte-/Interfaceidentität geöffnet") ist damit für einen realen Schreibpfad erstmals
  belegt. Die Kriterien zu einzeln ansteuerbaren Tasten sind davon **nicht** berührt —
  dieses Kommando steuert alle LEDs gemeinsam, keine Einzeltasten-Adressierung.
- **Zusatzbefund (USB-Replug ist kein Reset):** Nutzer hat danach die Light Mount kurz
  vom USB getrennt und wieder verbunden, um den Zyklus-Effekt zu beenden — der Effekt
  lief unverändert weiter. Der in `docs/first-write-test-plan.md` genannte Replug-
  Fallback ist damit für laufende Effekte **nicht zuverlässig**. Vermutlich behält der
  Light-Mount-Controller seinen Zustand über einen reinen Host-seitigen USB-Replug hinweg
  (z. B. weiterhin mit Strom versorgt, oder Effekt läuft MCU-seitig unabhängig weiter).
  Ein echtes, verifiziertes "Off"/Static-Kommando fehlt weiterhin — siehe `SECURITY.md`
  Regel 7 und `BACKLOG.md`. **Tatsächlich funktionierender Rückfall:** eine physische
  Hotkey-Kombination auf der Tastatur selbst setzt zuverlässig auf eine gleichförmige
  statische Farbe zurück (vom Nutzer bestätigt, benutzt) — siehe `SECURITY.md`.

### Zweiter Hardwaretest: Frame 2747 (2026-08-18)

Gleiches Verfahren wie beim ersten Test (bekanntes, byteidentisches Kommando aus dem
Capture, `report_send --confirm`, ein einziger Schreibvorgang, `crc_valid=yes` vorab
geprüft). Payload: `04 00 64 0a 01 00 ff 00 ff 37` + Nullpadding (18 Byte Gesamtlänge,
deutlich kürzer als das Rainbow-Kommando mit 41 Byte).

- **Bestätigt:** Erzeugt eine **statische** (nicht zeitlich veränderliche) einheitliche
  Farbe auf allen Tasten — vom Nutzer als **Orange** beschrieben. Kein USB-Reset.
- **Bytehypothese explizit widerlegt:** Die naheliegende Lesart „Byte 14-16 = RGB"
  (`ff,00,ff` = Magenta) passt **nicht** zur beobachteten Farbe Orange. Damit ist diese
  kurze Payload-Form vermutlich **kein** direkter RGB-Träger.
- **Neue, besser gestützte Hypothese:** Byte 8 fungiert (zumindest teilweise) als
  **Preset-/Effekt-Index**: `0x03` = Rainbow-Zyklus (voller Farbkreis über Zeit),
  `0x04` = dieses feste Orange-Preset. Die genaue Farbcodierung für „beliebige eigene
  Farbe setzen" ist damit weiterhin **nicht** bekannt — möglich, dass sehr kurze
  Kommandos wie dieses aus einer festen Presetliste wählen, statt Rohfarben zu
  übertragen. Nicht weiter geraten, bis ein gezielter Einzeltest das klärt.
- Damit ist das `SPEC.md`-Kriterium „statische Gesamtfarbe … sicher schalten" im Kern
  erstmals demonstriert (eine statische Farbe lässt sich setzen) — aber noch nicht im
  Sinne von „beliebige selbst gewählte Farbe", da die Farbe hier fest im Kommando steckt
  und nicht durch uns kontrolliert wurde.

## OpenRGB-Controller-Test: Sequenznummer-Start-bei-1 widerlegt (2026-08-18)

Nach dem erfolgreichen Build- und Detection-Test (siehe unten) ein echter Farbtest über
`./openrgb --device 0 --mode static --color 8000FF` (Nutzerfreigabe vorab). **Keine
sichtbare Wirkung.** Zur Diagnose per `report_send`/direktem `hidraw`-Zugriff exakt das
Kommando nachgebaut, das der Controller gesendet hätte (Session `0x0002`, Seq `0x0001`)
— **identisches Ablehnungsmuster** wie beim `0x2000`-Fehlschlag aus Iteration 12 (volle
Echo-Antwort, Byte 3 = `0x0a` statt `0x00`).

**Damit widerlegt:** Die Hypothese „ein bei 1 startender, monoton steigender Zähler
funktioniert, weil das Gerät nach einem Reset auch bei 0/1 anfängt" ist **falsch** —
sowohl sehr niedrige als auch sehr hohe frei gewählte Sequenzwerte werden abgelehnt. Nur
der eine tatsächlich live beobachtete Wert (`0x10ad`) hat je funktioniert. Das deutet
darauf hin, dass das Gerät einen **echten internen Zustand** verfolgt (z. B. den zuletzt
gesehenen realen Wert), der nicht einfach durch einen frischen Client-Zähler
nachgebildet werden kann, ohne diesen Zustand vorher zu kennen. Die
Interface-3-Report-ID-1-Telemetriewerte (siehe eigener Abschnitt oben) sind ein noch
nicht verifizierter Kandidat dafür, woher ein Client diesen Zustand lernen könnte.

**Konsequenz:** Der OpenRGB-Controller (`openrgb-integration/`) baut fehlerfrei und wird
korrekt erkannt (siehe unten), aber sein einziger Modus (`Static`) funktioniert **noch
nicht zuverlässig** auf frisch gestarteten Sessions. Als bekannte Einschränkung in
`openrgb-integration/README.md` und `BACKLOG.md` festgehalten, nicht stillschweigend
übergangen.

**Interface-3-Telemetrie als Sequenznummer-Quelle geprüft und verworfen (2026-08-18):**
Report-ID 1 zweimal im Abstand einiger Minuten gelesen (rein lesend, `GET_FEATURE`):
zwei der fünf 32-Bit-Werte hatten sich verändert (450740→452788-Bereich,
157418→157930-Bereich), **ohne dass in der Zwischenzeit etwas geschrieben wurde** — sehen
nach frei laufenden Zählern/Timern (z. B. Uptime) aus, nicht nach einem
"nächste-erwartete-Sequenznummer"-Feld. Die drei konstant gebliebenen Werte (42110, 1,
33333) sind vermutlich feste Geräte-Kennungen. Keiner der Werte liegt plausibel nahe an
der einzigen bekannten funktionierenden Sequenznummer (`0x10ad`=4269). **Hypothese
verworfen, kein Schreibversuch mit einem dieser Werte unternommen** — zu spekulativ für
einen weiteren realen Hardwarezugriff (`SECURITY.md` Regel 10).

## Strukturvergleich mit Mountain Everest — Protokollverwandtschaft widerlegt

Quelle: `MountainKeyboardController.{h,cpp}` aus dem OpenRGB-Hauptzweig, per `curl` auf
die Raw-Dateien geladen (2026-08-18, siehe `docs/research-sources.md`).

Mountain Everest (Referenzcode, tatsächlicher Aufbau — nicht nur die im Master-Prompt
genannten Vermutungen):

- 65-Byte-USB-Buffer, Byte 0 = **HID-Report-ID** (bei `hid_write` von hidapi automatisch
  als erstes Byte erwartet; im Code fast immer implizit `0x00` via `memset`).
- Byte 1 = Kommandoklasse: `0x14` (SEND) oder `0x13` (SAVE) — feste Werte an fester Position.
- Byte 2 = Subkommando (`0x2C` Farbdaten, `0x2D` Edge-Farbdaten, `0x00` Moduswahl, `0xA0` Bestätigung).
- Byte 3 = Modus-Nachricht (z. B. `0x00` static, `0x04` wave, `0x0A` custom) bzw. Paketindex.
- Byte 4–6 = modusabhängige Parameter (oft `0x01`/pkt_no, speed, brightness).
- Byte 7+ = Payload (Farben, Wave-Parameter).
- **Keine Prüfsumme, keine Sequenznummer, kein Längenfeld** — der Rest des Buffers wird
  einfach mit `0x00` (bzw. bei `SendColorStartPacketCmd` mit `0xFF`) vorbelegt und roh
  gesendet.

Light Mount Interface 2 (siehe oben, eigene Messung):

- 64-Byte-Reports, **kein** Report-ID-Byte (Report Descriptor hat kein Report-ID-Tag,
  siehe Abschnitt „Bestätigte Fakten“) — die Kommandoklasse `0x14`/`0x13` an fester
  Byte-1-Position aus Mountain kommt in keinem der 20 beobachteten Frames vor.
- Byte 0–1 = Längenfeld, Byte 2–3 = konstant `0x0002`, Byte 4–5 = monoton steigende
  Sequenznummer, Byte 6 = Subkommando, Byte 7 = Flags, Byte 62–63 = CRC16/MODBUS.
- Die Subkommando-Werte, die bisher beobachtet wurden (`0x02`, `0x06`, `0x0a`), haben
  keine erkennbare Entsprechung zu Mountains `0x00/0x2C/0x2D/0xA0`.

**Ergebnis:** Die im Master-Prompt genannte Hypothese einer strukturellen Verwandtschaft
(gleiche Kommandofamilien, 65-Byte-Reports mit Report-ID) ist **widerlegt**. Das Light-
Mount-Protokoll auf Interface 2 ist ein eigenständiges Format mit Längenfeld, Sequenz-
zähler und CRC16/MODBUS-Absicherung — keines davon existiert im Mountain-Protokoll.
Einzige echte Gemeinsamkeit: beide verwenden ein ca. 64-Byte-HID-Interrupt-Interface mit
einem kurzen Header vor der eigentlichen Nutzlast, was eher an eine übliche USB-HID-
Konvention als an geteilten Code erinnert. Für Phase 3 bleibt der Mountain-Controller
dennoch als **Vorlage für die OpenRGB-Integrationsform** (Klassenstruktur, Aufbau der
Farbverlauf-/Wave-Parameter, `RGBController`-Anbindung) relevant — nicht als
Byte-Protokoll-Vorlage.

## Interpretierte Hypothesen (aus Deskriptoren bzw. Capture-Struktur abgeleitet, noch nicht durch eigenen Hardwaretest bestätigt)

- Interface 1, Report-ID 0x0a (Buttons + Wheel + X/Y) ist vermutlich das **Medienrad** —
  wird als generische Mausbewegung/-Scrollrad an den Kernel gemeldet, nicht als eigenes
  Vendor-Kommando. Reine Deskriptor-Interpretation, kein realer Drehtest durchgeführt.
- Interface 3 (Vendor Page 0x59, viele Feature-Reports, Report-ID-Struktur mit
  wiederkehrenden 4er-Usage-Gruppen) ist ein zweiter, vermutlich **umfangreicherer**
  Konfigurationskanal (evtl. Per-Key-Daten oder Profile), der über Control-Transfer
  (`SET_REPORT`/`GET_REPORT`) statt über Interrupt-Endpoints läuft. Noch keine Zuordnung
  zu konkreten Funktionen.
- ~~Protokoll ist strukturell mit Mountain Everest verwandt~~ — **widerlegt** (siehe
  eigener Abschnitt unten): Byte-Layout, Kommandokennung und Prüfsumme unterscheiden
  sich grundlegend. Als Referenzcode für die Phase-3-OpenRGB-Integration (Klassenform,
  `RGBController`-Anbindung, Farbverlauf-Aufbau) bleibt der Mountain-Controller trotzdem
  nützlich — nur eben nicht als Byte-Protokoll-Vorlage.
- ~~IO Center Web deckt möglicherweise keine Per-Key-Beleuchtung ab~~ — **bestätigt**
  (2026-08-18, Nutzer hat die UI direkt geprüft): IO Center Web bietet ausschließlich
  vorgeformte Effekte/„Figuren" mit Animation (u. a. **Matrix**, **Tornado**) an, keine
  Einzeltasten-Konfiguration. Bemerkenswert: „Matrix" und „Tornado" sind exakt dieselben
  Effektnamen wie im Mountain-Everest-Referenzcode (`MOUNTAIN_KEYBOARD_MATRIX_MSG`,
  `MOUNTAIN_KEYBOARD_TORNADO_MSG`) — stützt die gemeinsame Produktherkunft (be quiet! hat
  Mountain übernommen) auf Konzept-/Namensebene, obwohl das Byte-Protokoll nachweislich
  unterschiedlich ist (siehe Abschnitt „Strukturvergleich mit Mountain Everest"). Für
  Einzeltasten-Adressierung wäre laut Master-Prompt nur der Windows-Client eine mögliche
  Quelle (VM mit USB-Passthrough, deutlich größerer Aufwand) — nicht ohne Rücksprache
  begonnen, siehe `BACKLOG.md`.

## Eigener Capture: RGB-Kodierung bestätigt (2026-08-18)

Eigener, gezielter Mitschnitt (nicht aus dem OpenRGB-Ticket) einer einzelnen freien
Farbwahl in IO Center Web — vollständige Rohdaten und Analyse in
`docs/evidence/own_capture_iocenter_decoded.md`. Kernergebnisse:

| Fakt | Beleg |
|---|---|
| **RGB-Kodierung für frei wählbare Farben bestätigt:** bei einer Static-Color-Kommandofamilie (Länge 15, Subcmd `0x06`, Byte 8-12 = `00 00 64 32 00`) tragen Byte 13-15 direkt Rot/Grün/Blau. Exakter Treffer gegen vom Nutzer aus der UI abgelesenen Hex-Wert `#1FB4FF` | `docs/evidence/own_capture_iocenter_decoded.md`, 2026-08-18 |
| Byte 10 (`0x64`=100) = Helligkeit, vom Nutzer bestätigt (UI stand auf 100%) | dito |
| Neues Subkommando `0x03` = periodisches Keepalive (~1,5s Intervall, leerer Payload, Länge 6), unabhängig von Nutzeraktionen | dito |
| **Korrektur:** Byte 2-3 ("konstant 0x0002") ist **kein** universeller Protokollwert, sondern session-/verbindungsabhängig — eigener Capture zeigte `0x0001` statt `0x0002` | dito, `report.h` entsprechend präzisiert |
| Unaufgeforderte Geräte→Host-Push-Frames (Subcmd `0x02`) spiegeln Tastendrücke (Pfeiltasten/Enter) vom Boot-Keyboard-Interface fast zeitgleich über den Vendor-Kanal — Hypothese: Umgehung der WebHID-Tastatur-Zugriffssperre im Browser | dito, siehe auch `SECURITY.md` (Privacy-Hinweis) |

**Methodischer Hinweis:** `tshark`/`dumpcap` waren für `/dev/usbmon*` durch AppArmor
gesperrt (siehe `SECURITY.md`); Mitschnitt erfolgte über das debugfs-Textinterface
(`/sys/kernel/debug/usb/usbmon/1u`), das Payloads auf 32 Byte kürzt — für die hier
beobachteten kurzen Kommandos ausreichend.

## Erster selbst konstruierter Hardwaretest: Sequenznummer wird validiert (2026-08-18)

Vollständige Diagnose in `docs/evidence/sequence-number-validation-test.md`. Kernergebnis:

| Fakt | Beleg |
|---|---|
| **RGB-Kodierung erneut bestätigt** mit einem zweiten, unabhängigen Farbwert (`#00FF00`, selbst konstruiert, nicht aus einem Capture kopiert) | Nutzerbestätigung "jetzt grün", 2026-08-18 |
| **Gerät validiert die Sequenznummer** bei Static-Color-Kommandos: frei erfundene Werte (`0x2000`+) werden abgelehnt (keine sichtbare Wirkung, abweichendes Antwortformat), ein Wert nahe am zuletzt real beobachteten Stand (`0x10ad`) wird akzeptiert | 6 kontrollierte Teilversuche, siehe Diagnose-Dokument |
| **Widerlegt:** Sitzungs-ID-Mismatch (Byte 2) als Ursache — Session `0x01` und `0x02` beide gleichermaßen abgelehnt bei falscher Sequenznummer, beide gleichermaßen akzeptiert bei richtiger | dito |
| **Widerlegt:** fehlendes „Commit"-Kommando (Subcmd `0x0a`) als Ursache | dito |
| **Neu:** Ablehnungs-Antwortformat identifiziert — volles Echo der Anfrage (Länge = Anfragelänge) mit Byte 3 = `0x0a` statt der üblichen 6-Byte-Bestätigung mit Byte 3 = `0x00` | dito |

**Einschränkung:** Nur ein akzeptierter und mehrere abgelehnte Sequenzwerte getestet —
die genaue Akzeptanzregel (Toleranzfenster? exakte Fortsetzung?) ist nicht bestimmt.
Auch nicht geklärt, ob diese Validierung nur diese eine Kommandofamilie betrifft (Rainbow-
und Preset-Kommandos akzeptierten in früheren Tests beliebige alte Sequenznummern).

## Frame 2341/3109 identifiziert: Matrix-Effekt (2026-08-18)

Gezielter eigener Capture (Nutzer wählt explizit "Matrix" in IO Center Web) ergab ein
Kommando, das **byteidentisch** mit dem seit Iteration 2 unentschlüsselten Frame
2341/3109 aus dem alten Fremd-Capture ist. Details: `docs/evidence/own_capture_matrix_effect.md`.
Damit ist der Matrix-Effekt als Subcmd `0x06`, Länge 29, mit einer deutlich komplexeren
Payload-Struktur als Static-Color/Rainbow identifiziert — die genaue Bedeutung der
einzelnen Payload-Bytes (vermutlich mehrere Parameter: Farben, Geschwindigkeit,
Richtung) ist weiterhin nicht vollständig geklärt, um nicht zu raten.

## Statische Analyse der Windows-App: vollständiges Gerätemanifest (2026-08-18)

Rein statische Analyse (kein Ausführen, kein Wine/VM) des vom Nutzer bereitgestellten
Windows-Installers. Vollständige Details: `docs/evidence/windows-app-static-analysis.md`.
Rohdateien lokal unter `vendor-extracts-private/` (git-ignoriert, proprietäres
Herstellermaterial).

| Fakt | Beleg |
|---|---|
| **Genau 6 Effekte bestätigt:** Static, ColorWave, Tornado, Breathing, Reactive, Matrix — identische Namen/Reihenfolge wie im Mountain-Referenzcode-Enum | offizielles Gerätemanifest, `light_mount_device_full.json` |
| **168 individuell adressierbare LEDs, vollständig durchnummeriert** (45 obere Leiste + 113 Einzeltasten inkl. ISO-Varianten + 10 seitliche Leiste) | `light_mount_leds_mapping.json`, siehe Evidence-Dokument |
| VID `0x373F`/PID `0x0002` exakt bestätigt, Bootloader-PID `0x000A` (separate USB-Identität, **kein** Zusammenhang mit Subcmd `0x0a`) | dito |
| Macro-Limits (10 Makros, 58 Events), Polling-Rate 1000Hz, 2 Keybinding-Layer (Common/Fn1) | dito |

**Bedeutung für Per-Key-Adressierung (seit Iteration 1 offene Frage):** Per-Key-Steuerung
**existiert in der Firmware** (168 benannte, durchnummerierte LEDs) — sie ist nur in IO
Center Web nicht freigeschaltet. Das konkrete Wire-Kommando dafür ist weiterhin
**nicht bekannt** (kein solches Kommando in einem Capture beobachtet, da die Web-UI es
nie sendet) — die Analyse liefert das Ziel-Schema (welche 168 LEDs), nicht das Protokoll
dafür.

**LED-Index-Tabelle mit Pixel-Koordinaten zusammengeführt:** `docs/evidence/light_mount_led_layout_iso.json`
(eigene Zusammenführung, kein Vendor-Original, siehe `docs/evidence/led-layout-merge.md`)
— 166 Einträge, 111 davon mit UI-Pixel-Geometrie. Grundlage für ein künftiges
OpenRGB-Layout (Phase 3).

## Verbindungsaufbau-Capture: Zähler-Feld neu verstanden, Kontinuität bestätigt (2026-08-19)

Vollständige Analyse in `docs/evidence/connection-handshake-analysis.md`. Kernergebnisse:

| Fakt | Beleg |
|---|---|
| **Korrektur:** Was bisher als "16-Bit-Sequenznummer" (Byte 4-5) dokumentiert war, ist tatsächlich zwei getrennte Felder — Byte 4 ist ein eigenständiger, **fortlaufender 1-Byte-Zähler** (steigt exakt +1 pro Kommando, über alle "Session"-Wechsel hinweg), Byte 5 ist ein separates, kontextabhängiges Feld (konstant `0x10` für Static-Color, `0x01` für Keepalive) | gezielter Verbindungsaufbau-Capture, 20 Kommandos chronologisch dekodiert |
| **Bestätigt (echter Hardwaretest):** Wird der Zähler exakt an den zuletzt beobachteten echten Gerätestand angeschlossen, werden **mehrere aufeinanderfolgende, selbst konstruierte** Kommandos zuverlässig akzeptiert (zwei Farben nacheinander gesetzt und vom Nutzer live bestätigt: Lila `#8000FF`, dann Gelb `#FFFF00`) | 2026-08-19, `docs/evidence/connection-handshake-analysis.md` |
| Byte 2-3 ("Session") nimmt mindestens drei Werte innerhalb **einer** Verbindung an (`0x0000` Handshake, `0x0002` Einstellungs-Batch, `0x0001` Laufzeit/Keepalive) — der Zähler läuft davon unbeeinflusst durch | dito |
| Neuer, teilweise entschlüsselter Identifikations-Handshake gefunden (Geräte-Seriennummer-String, Fähigkeitenliste unterstützter Subcmd-IDs) | dito |

**Konsequenz für Code:** `src/protocol/report.h`/`.cpp` und der OpenRGB-Controller
(`openrgb-integration/`) wurden entsprechend korrigiert (`Interface2Report.seq` →
`counter`/`marker` als getrennte Felder). **Weiterhin ungelöst:** wie ein frisch
startender Client den aktuell gültigen Zählerstand ohne Live-Capture lernt — siehe
`BACKLOG.md`.

**Vertiefung (2026-08-19):** Byte 5 ("marker") ist vermutlich eine **Attribut-/
Fähigkeits-ID**, keine beliebige Kennung — eine im Handshake übertragene 17-Byte-Liste
(Subcmd `0x04`, Paare `[ID][Version=1]`) zählt genau die Marker-Werte auf, die auch
sonst in echten Kommandos verwendet werden (u. a. `0x10` für Static-Color, `0x01` für
Keepalive). Vollständige Tabelle und weitere, noch nicht gedeutete Antwortstrukturen
(u. a. ein möglicher Zonen-/Gruppen-Block) in
`docs/evidence/connection-handshake-analysis.md`.

## Bekannter Firmwarefehler

Ungezielter Lesezugriff auf ein herstellerspezifisches `hidraw`-Interface kann die
USB-Verbindung zurücksetzen (community-reproduziert, u.a. durch generisches HID-Polling
von Heroic/Lutris/Steam). Siehe `SECURITY.md`.

## Kaltstart-Problem des Zählers gelöst (2026-08-24)

Bisher ungelöst: wie ein frisch startender Client den aktuell gültigen Zählerstand
(Byte 4, siehe oben) ohne Live-Capture lernt. Getestet mit einer kontrollierten,
seriellen Sweep-Sequenz (ein Schreibvorgang, Antwort lesen, bei Ablehnung nächsten Wert
versuchen, bei Erfolg sofort stoppen — keine parallele/endlose Schleife, siehe
`SECURITY.md` Regel 8/9) unter der Bedingung: **frische Verbindung, keine andere
Software (IO Center, Browser) hatte vorher mit Interface 2 gesprochen** (Voraussetzung
der bereits früher aufgestellten, bis dahin ungetesteten Hypothese).

**Ergebnis: Zählerwert `0x00` wurde beim allerersten Versuch akzeptiert** (kurze
6-Byte-Bestätigung, `byte3=0x00`, kein Ablehnungs-Echo). Bestätigt damit die Hypothese
aus Iteration ~19: nach einem echten Reset/Kaltstart (keine aktive Fremd-Verbindung)
akzeptiert das Gerät `0x00` als neuen Ausgangspunkt. Nachfolgendes Kommando mit
`counter=0x01` (korrekte Fortsetzung) ebenfalls akzeptiert und sichtbar wirksam
(Farbwechsel Rot→Magenta, vom Nutzer live bestätigt).

**Konsequenz für Phase 3/OpenRGB-Integration:** Ein frisch verbindender Client kann
`counter=0x00` als ersten Versuch nutzen, sofern keine andere Software aktuell verbunden
ist (nicht zuverlässig prüfbar) — bei Ablehnung (Byte 3 = `0x0a`-Antwortform) mit
inkrementierenden Werten weitersuchen, bis Akzeptanz, dann fortlaufend `+1` je Kommando.
Nicht getestet: ob `0x00` auch bei einer noch aktiven Fremd-Verbindung fälschlich
akzeptiert würde (Kollisionsrisiko) — dieser Test lief bewusst nur im nachweislich
verbindungsfreien Zustand.

## Unterseiten-/Seiten-Lichter: über Interface 2 erreichbar, nicht individuell (2026-08-24)

Die vom Nutzer beobachteten zwei kleinen Lichter an der Unterseite außen (vermutlich
Teil der im Windows-Manifest dokumentierten `Led_KeyboardLeft/Right1-5`, 10 LEDs ohne
UI-Geometrie) sind **nicht** Teil der 135 HID-LampArray-Lampen auf Interface 3 (dort
sind alle 135 Adressen anderen Elementen zugeordnet, siehe
`docs/evidence/lamp_id_key_mapping.json`). Live getestet: Sie reagieren auf das globale
Static-Color-Kommando auf Interface 2 (Rot, dann Magenta, beides vom Nutzer inklusive
der Unterseiten-Lichter bestätigt) — sind also RGB-fähig und softwaregesteuert, aber
**nicht individuell** adressierbar, solange kein Per-LED-Wire-Kommando für Interface 2
bekannt ist (weiterhin ungelöst, siehe `BACKLOG.md`).

## Offene Fragen

- Welche der vier HID-Interfaces entspricht welcher Usage Page / welchem Zweck?
- Welche Aktionen deckt der vorhandene Web-Capture tatsächlich ab (fehlt Per-Key)?
- Matrix-/LED-Reihenfolge der Tasten sowie obere/seitliche Leisten als getrennte Zonen?

## Quellen

Siehe `docs/research-sources.md`.
