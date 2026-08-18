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
| Interface 3: Vendor Usage Page `0x59`, sechs Report-IDs (1–6), überwiegend **Feature Reports** (Control Transfer, kein passender Output-Endpoint — Interface hat laut `lsusb -v` nur einen IN-Interrupt-EP `0x85`/4B); Report ID 3 enthält 128× wiederkehrende Usage-Werte `0x51 0x52 0x53 0x54` gefolgt von einem 32-Byte-Feature-Block | `docs/evidence/report_descriptor_if3.hex`, sysfs, 2026-08-18 |
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
- IO Center Web deckt möglicherweise keine Per-Key-Beleuchtung ab (nur der Windows-Client) —
  der usbmon3-Capture zeigt in den ~20s beobachteter Interaktion keine Kommandos, die nach
  Einzeltasten-Adressierung aussehen (keine 60+ Byte langen, sich klar in Tastenanzahl
  wiederholenden Muster) — schwacher, indirekter Hinweis, keine sichere Widerlegung, da
  der Capture nur einen kleinen Ausschnitt der UI abdeckt.

## Bekannter Firmwarefehler

Ungezielter Lesezugriff auf ein herstellerspezifisches `hidraw`-Interface kann die
USB-Verbindung zurücksetzen (community-reproduziert, u.a. durch generisches HID-Polling
von Heroic/Lutris/Steam). Siehe `SECURITY.md`.

## Offene Fragen

- Welche der vier HID-Interfaces entspricht welcher Usage Page / welchem Zweck?
- Welche Aktionen deckt der vorhandene Web-Capture tatsächlich ab (fehlt Per-Key)?
- Matrix-/LED-Reihenfolge der Tasten sowie obere/seitliche Leisten als getrennte Zonen?

## Quellen

Siehe `docs/research-sources.md`.
