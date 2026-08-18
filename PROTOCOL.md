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

## Interpretierte Hypothesen (aus Deskriptoren abgeleitet, noch nicht durch echten Traffic bestätigt)

- Interface 2 ist der wahrscheinlichste **Konfigurationskanal** für IO Center: einziges
  Interface mit reinem Vendor-Usage-Page, keinem Report-ID-Overhead, vollen 64-Byte
  Nutzlasten in beide Richtungen plus Feature-Report — passt am besten zu einem generischen
  Kommando-/Antwort-Kanal. **Noch nicht durch Traffic-Capture bestätigt.**
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
  noch zu prüfen anhand des vorhandenen Captures.

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
