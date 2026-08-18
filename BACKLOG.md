# BACKLOG

Offene Arbeitspakete, grob nach Phase. Nicht priorisiert innerhalb einer Phase — siehe
`STATE.md` für den jeweils nächsten konkreten Schritt.

## Phase 0

- [x] System/Kernel/OpenRGB-Version/USB-Topologie erfassen (2026-08-18)
- [x] sysfs-HID-Report-Deskriptoren der vier Interfaces sichern (2026-08-18, `docs/evidence/`)
- [x] OpenRGB Issue #4950 auf neue Aktivität prüfen, Capture-Anhang laden (2026-08-18, keine neue Aktivität, Capture analysiert)
- [ ] Baseline dokumentieren: aktuelles Profil, sichtbare Beleuchtung, Verhalten nach USB-Reconnect
- [ ] Interface-3-Report-Struktur (Vendor Page 0x59, 6 Report-IDs) genauer aufschlüsseln
- [x] Checksum-Algorithmus der letzten 2 Report-Byte offline gegen alle 20 bekannten usbmon3-Frames verifizieren (2026-08-18, CRC16/MODBUS bestätigt, 20/20)
- [ ] Verbleibende unentschlüsselte usbmon3-Kommandos (29/15/18/7 Byte) weiter eingrenzen, siehe `docs/evidence/usbmon3_decoded_commands.txt`
- [x] Mountain-Everest-Referenzcode besorgt und strukturell verglichen (2026-08-18, Protokollverwandtschaft widerlegt, siehe `PROTOCOL.md`)

## Phase 1

- [x] CRC16/MODBUS + Report-Parsing/-Aufbau in C++ implementiert und gegen alle 20
      bekannten usbmon3-Fixtures getestet (2026-08-18, `src/protocol/`, `tests/`)
- [x] Dry-Run-CLI (`report_dump`) mit strukturierten Hex-Dumps, kein `hidraw`-Zugriff (2026-08-18)
- [ ] PCAP-Parser/Wireshark-Auswertung reproduzierbar machen
- [ ] IO-Center-Web-Aktionen den HID-Paketen zuordnen
- [ ] Prüfen ob Web-Capture Per-Key-Kommandos enthält; falls nicht: Capture-Plan für Windows-Client
- [ ] Mountain- vs. Light-Mount-Pakete strukturell vergleichen
- [ ] Offline-Tests mit gespeicherten Paketen (ohne Hardware)

## Phase 2 (RGB-MVP)

- [ ] Geräteerkennung/-öffnen
- [ ] Statische Gesamtfarbe + Off
- [ ] Direct Mode
- [ ] ≥2 einzelne Tasten unabhängig einfärben
- [ ] Vollständige Tastenmatrix/LED-Reihenfolge bestimmen
- [ ] Obere/seitliche Leisten getrennt adressieren
- [ ] Reconnect nach USB-Reset

## Phase 3 (OpenRGB-Integration)

- [ ] Idiomatischer OpenRGB-Controller (Detection, Zonen, LED-Namen, Layout, Direct Mode)
- [ ] udev-Regeln mit minimalen Rechten
- [ ] Lokaler Build + Regressionstests
- [ ] Upstream-tauglicher Patch/MR vorbereiten

## Phase 4

- [ ] Hardwareeffekte, Helligkeit, Profilwechsel
- [ ] Kontrolliertes Onboard-Speichern
- [ ] Medienrad, Makrotasten, Key Remapping
- [ ] Rücklesen vorhandener Konfiguration (falls sicher möglich)

## Phase 5

- [ ] OpenRGB-SDK-Client / kleiner User-Daemon
- [ ] CLI + stabile lokale API (D-Bus/Unix-Socket)
- [ ] Zeitgesteuerte Profile, Systemzustände
- [ ] Freedesktop/KDE-Benachrichtigungsbeobachtung ohne Inhaltsspeicherung

## Phase 6

- [ ] GUI nur für nicht durch OpenRGB abgedeckte Bedienung
- [ ] Debian-Paket / reproduzierbare Installation
- [ ] systemd-User-Service, vollständige Deinstallation
