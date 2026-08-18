# bequiet-lightmount-linux

Linux-Unterstützung für die kabelgebundene Tastatur **be quiet! Light Mount**
(DE ISO, Silent-Linear-Switches), mit dem Ziel, den Funktionsumfang der
Windows-/Web-Hersteller-Software (IO Center) unter Linux nachzubilden —
bevorzugt als Controller in [OpenRGB](https://openrgb.org/), nicht als
eigene RGB-Plattform.

Status: **Phase 0 — Bestand und Reproduzierbarkeit.** Siehe [`STATE.md`](STATE.md)
für den aktuellen Loop-Zustand und den nächsten konkreten Schritt.

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
