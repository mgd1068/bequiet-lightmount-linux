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
| Ein konkretes 41-Byte-Kommando (zweimal byteidentisch, 11,4s auseinander gesendet, `subcmd=0x06`) enthält ab Byte 17 sechs `[Position 0–100][R][G][B]`-Gruppen, die exakt einen Regenbogen-Farbkreis ergeben: 0%=Gelb, 17%=Grün, 33%=Cyan, 50%=Blau, 67%=Magenta, 83%=Rot | Frames 1453 & 3531, `docs/evidence/usbmon3_decoded_commands.txt` |

**Einschränkung:** Dies ist eine Offline-Analyse eines fremden Captures, keine eigene
Hardwareverifikation. Die Rainbow-Interpretation ist plausibel und durch doppelte,
identische Übertragung gestützt, gilt aber erst nach eigenem `--dry-run`/Hardwaretest
(Phase 1/2) als bestätigt im Sinne von `SPEC.md`.

## Interpretierte Hypothesen (aus Deskriptoren bzw. Capture-Struktur abgeleitet, noch nicht durch eigenen Hardwaretest bestätigt)

- Interface 1, Report-ID 0x0a (Buttons + Wheel + X/Y) ist vermutlich das **Medienrad** —
  wird als generische Mausbewegung/-Scrollrad an den Kernel gemeldet, nicht als eigenes
  Vendor-Kommando. Reine Deskriptor-Interpretation, kein realer Drehtest durchgeführt.
- Interface 3 (Vendor Page 0x59, viele Feature-Reports, Report-ID-Struktur mit
  wiederkehrenden 4er-Usage-Gruppen) ist ein zweiter, vermutlich **umfangreicherer**
  Konfigurationskanal (evtl. Per-Key-Daten oder Profile), der über Control-Transfer
  (`SET_REPORT`/`GET_REPORT`) statt über Interrupt-Endpoints läuft. Noch keine Zuordnung
  zu konkreten Funktionen.
- Protokoll ist strukturell mit Mountain Everest verwandt (65-Byte-Reports inkl. Report-ID,
  Kommandofamilien `0x14 0x2c` / `0x14 0x2d` / `0x14 0xa0`, `0x13 ... 0x55`) — unbewiesen bis
  Light-Mount-Capture strukturell verglichen wurde. Auffällig: Mountain-Referenz nennt
  65-Byte-Reports **mit** Report-ID, während Interface 2 hier **ohne** Report-ID auskommt —
  mögliche Abweichung vom Mountain-Protokoll, noch nicht abschließend bewertet.
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
