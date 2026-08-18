# PROTOCOL

Bestätigte Fakten und Hypothesen werden strikt getrennt. Nichts aus diesem Dokument
gilt als belegt, ohne dass die Quelle (Deskriptor, Capture, realer Test) genannt ist.

## Bestätigte Fakten

| Fakt | Quelle |
|---|---|
| USB VID:PID = `373f:0002` | Bekannter Ausgangsstand / OpenRGB-Ticket #4950 |
| Gerätebezeichnung "be quiet! Light Mount" | Herstellerangabe |
| USB 1.1 Full Speed, max. 64 Byte/Endpoint-Paket | Bekannter Ausgangsstand |
| Vier HID-Interfaces | Bekannter Ausgangsstand |
| Interface 2 laut vorhandenem Deskriptor: `0x83` IN (64B), `0x04` OUT (64B) | Bekannter Ausgangsstand — **lokal noch nicht neu verifiziert** |

## Hypothesen (unbestätigt)

- Interface 2 ist der Konfigurationskanal, über den IO Center Web zugreift — muss lokal
  über sysfs, HID Usage Page, Interface-Nummer und kontrollierte Traces verifiziert werden.
- Protokoll ist strukturell mit Mountain Everest verwandt (65-Byte-Reports inkl. Report-ID,
  Kommandofamilien `0x14 0x2c` / `0x14 0x2d` / `0x14 0xa0`, `0x13 ... 0x55`) — unbewiesen bis
  Light-Mount-Capture strukturell verglichen wurde.
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
