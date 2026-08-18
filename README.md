# bequiet-lightmount-linux

Linux-Unterstützung für die kabelgebundene Tastatur **be quiet! Light Mount**
(DE ISO, Silent-Linear-Switches), mit dem Ziel, den Funktionsumfang der
Windows-/Web-Hersteller-Software (IO Center) unter Linux nachzubilden —
bevorzugt als Controller in [OpenRGB](https://openrgb.org/), nicht als
eigene RGB-Plattform.

Status: **Phase 3 — OpenRGB-Integration (Grundgerüst).** Siehe [`STATE.md`](STATE.md)
für den aktuellen Loop-Zustand und den nächsten konkreten Schritt.

## Bauen und Testen

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Reine Offline-Tests (CRC16/MODBUS, Report-Parsing/-Aufbau) gegen bekannte,
aus dem öffentlichen usbmon3-Capture stammende Fixtures — kein Gerätezugriff.

## Dry-Run-CLI

`report_dump` liest einen 64-Byte-Interface-2-Report als 128-stelligen Hex-String
(Argument oder stdin) und gibt die dekodierten Felder aus. Öffnet **niemals** `hidraw`,
fasst keine Hardware an:

```
./build/report_dump "290002003e100600030064320207ff000000ffff001100ff002100ffff320000ff43ff00ff53ff0000640000000000000000000000000000000000000000f3d0"
```

`report_build` baut einen Report aus einzelnen Feldern (inkl. CRC) und gibt ihn als
Hex-String auf stdout aus — kombinierbar per Pipe mit `report_dump`/`report_send`.
`--length` muss explizit angegeben werden (keine bestätigte Ableitungsformel aus dem
Payload, siehe `PROTOCOL.md`):

```
./build/report_build --subcmd 06 --seq 10ad --session 01 --length 0f --payload 000064320000ff00
```

`report_send <hidraw-Pfad> <Hex>` schreibt einen Report tatsächlich ans Gerät — nur mit
explizitem `--confirm`-Flag, sonst reiner Dry-Run (kein `open()`). Siehe
`docs/first-write-test-plan.md` für das Vorgehen bei echten Hardwaretests.

## OpenRGB-Integration

`openrgb-integration/Controllers/LightMountController/` enthält den (bewusst minimalen)
OpenRGB-Controller — siehe `openrgb-integration/README.md` zum lokalen Bauen/Testen.

## Zielumfang

1. Jede Tasten-LED einzeln adressieren
2. Obere und seitliche ARGB-Leisten separat steuern
3. Helligkeit, statische Farben, Hardwareeffekte, Direct Mode
4. Profile lesen/schreiben/wechseln, kontrolliertes Onboard-Speichern
5. Tasten-/Makro-Remapping, Medienrad
6. Stabile lokale Automatisierungsschnittstelle (OpenRGB SDK / D-Bus)
7. Subtile Statusdarstellung (Uhrzeit, Systemzustand, Benachrichtigungen) über einzelne LEDs

## Dokumentation

| Datei | Zweck |
|---|---|
| [`SPEC.md`](SPEC.md) | Verbindliche Anforderungen und Abnahmekriterien |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Zielarchitektur (OpenRGB-Integration) |
| [`PROTOCOL.md`](PROTOCOL.md) | Bestätigte Fakten vs. Hypothesen zum HID-Protokoll |
| [`SECURITY.md`](SECURITY.md) | Verbindliche Sicherheitsregeln im Umgang mit der Hardware |
| [`STATE.md`](STATE.md) | Aktueller Loop-Zustand, nächster Schritt |
| [`BACKLOG.md`](BACKLOG.md) | Offene Arbeitspakete |
| [`DECISIONS.md`](DECISIONS.md) | Architecture Decision Records |
| [`docs/research-sources.md`](docs/research-sources.md) | Primärquellen |

## Hardware

- USB VID/PID: `373f:0002`, USB 1.1 Full Speed, 4 HID-Interfaces
- Referenzprotokoll: OpenRGB-Controller für Mountain Everest (vermutete Verwandtschaft, unbestätigt)
- Bekannter Firmwarefehler: ungezielter `hidraw`-Lesezugriff kann die USB-Verbindung zurücksetzen — siehe [`SECURITY.md`](SECURITY.md)

## Lizenz

[GPL-2.0-or-later](LICENSE) — erforderlich für eine mögliche Integration in/Ableitung aus OpenRGB.

## Sicherheit

Bevor du dieses Repository mit angeschlossener Hardware nutzt: lies [`SECURITY.md`](SECURITY.md)
vollständig. Insbesondere: niemals `cat /dev/hidraw*`, niemals unbekannte HID-Geräte
probeweise öffnen.
