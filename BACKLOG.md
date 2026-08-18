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
- [x] `report_build`-Tool: Reports aus einzelnen Feldern bauen statt Python-Einwegskripte
      (2026-08-18, `session` als echtes Feld in `Interface2Report` ergänzt, per Pipe mit
      `report_dump`/`report_send` kombinierbar)
- [ ] PCAP-Parser/Wireshark-Auswertung reproduzierbar machen
- [ ] IO-Center-Web-Aktionen den HID-Paketen zuordnen
- [x] Prüfen ob Web-Capture Per-Key-Kommandos enthält (2026-08-18: bestätigt NEIN — IO
      Center Web bietet nur vorgeformte Effekte, keine Einzeltasten-UI, Nutzer hat UI
      direkt geprüft; siehe `PROTOCOL.md`)
- [ ] Windows-Client-Capture-Plan (VM + USB-Passthrough) für Per-Key-Adressierung
      ausarbeiten — großer Schritt, nicht ohne Rücksprache beginnen (Master-Prompt
      Phase 1)
- [x] Mountain- vs. Light-Mount-Pakete strukturell vergleichen (2026-08-18, siehe Phase 0, widerlegt)
- [x] Offline-Tests mit gespeicherten Paketen (ohne Hardware) (2026-08-18, `tests/test_protocol.cpp`, 20/20 Fixtures)

## Phase 2 (RGB-MVP)

- [x] Geräteerkennung/-öffnen (2026-08-18, `report_send`, exakt Interface 2 via sysfs-ermitteltem `hidraw`-Pfad)
- [x] Statische Gesamtfarbe (2026-08-18, Frame 2747 live getestet: statisches Orange,
      aber feste Preset-Farbe, keine frei wählbare RGB-Eingabe bestätigt — siehe `PROTOCOL.md`)
- [ ] Off-Kommando identifizieren/verifizieren (USB-Replug ist KEIN verlässlicher Reset,
      physischer Hotkey ist der bisher einzige bekannte verlässliche Rückfall — siehe `SECURITY.md`)
- [x] Farbcodierung für frei wählbare (nicht Preset-)Farben identifizieren (2026-08-18,
      eigener Capture, exakter Treffer gegen `#1FB4FF`, siehe `PROTOCOL.md`)
- [x] Static-Color-Kommando (Länge 15, Subcmd `0x06`) real auf Hardware getestet
      (2026-08-18, `#00FF00` selbst konstruiert und erfolgreich angewendet — siehe
      `docs/evidence/sequence-number-validation-test.md`)
- [ ] Sequenznummer-Akzeptanzregel klären (Toleranzfenster? exakte Fortsetzung? nur
      diese Kommandofamilie betroffen?) — kritisch für Phase 3 (`DECISIONS.md`)
- [ ] Bedeutung von Payload-Byte 11 (`0x32`) im Static-Color-Kommando klären
- [ ] Push-Frame-Mechanismus (Subcmd `0x02`, Tastendruck-Spiegelung) weiter untersuchen (Flags-Byte-Bedeutung)
- [ ] Ablehnungs-Antwortformat (Byte 3 = `0x0a` bei Fehler) an weiteren Fällen verifizieren
- [ ] Direct Mode
- [ ] ≥2 einzelne Tasten unabhängig einfärben (blockiert: kein Per-Key-Kommando im bisherigen Capture identifiziert)
- [ ] Vollständige Tastenmatrix/LED-Reihenfolge bestimmen
- [ ] Obere/seitliche Leisten getrennt adressieren
- [ ] Reconnect nach USB-Reset (noch nicht getestet — beim bisherigen Test kein Reset aufgetreten)

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
