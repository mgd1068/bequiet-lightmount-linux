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
- [ ] Verbleibende unentschlüsselte usbmon3-Kommandos (15/18/7 Byte) weiter eingrenzen,
      siehe `docs/evidence/usbmon3_decoded_commands.txt` (29-Byte-Kommando als
      Matrix-Effekt identifiziert, 2026-08-18, siehe `PROTOCOL.md`)
- [ ] Weitere Effekte (Tornado, ...) gezielt capturen und Payload-Byte-Bedeutung für
      Matrix genauer eingrenzen (Farben/Geschwindigkeit/Richtung vermutet, nicht bestätigt)
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
- [x] Windows-App statisch analysiert (2026-08-18, ohne VM/Ausführung) — vollständiges
      Gerätemanifest + 168-LED-Tabelle gefunden, siehe `PROTOCOL.md`/
      `docs/evidence/windows-app-static-analysis.md`. VM-Capture-Plan damit vorerst
      nicht mehr nötig — falls das Wire-Kommando für Per-Key doch noch per Live-Capture
      gebraucht wird, bleibt das als Option offen.
- [ ] Wire-Kommando für Per-Key-Adressierung finden (Manifest kennt nur die 168
      LED-Namen/Indizes, nicht das Byte-Protokoll dafür — Interface 3 Report ID 4
      als Kandidat identifiziert, aber `GET_FEATURE` liefert nur Nullen, siehe
      `PROTOCOL.md`; noch kein Schreibversuch, kein bekanntes Kommando als Basis)
- [ ] Bedeutung der Interface-3-Report-ID-1/3-Telemetriewerte klären (2026-08-18
      per `GET_FEATURE` ausgelesen, nicht interpretiert)
- [x] `light_mount_main_iso.json` (Pixel-Koordinaten pro Taste) mit den LED-Indizes
      verknüpft (2026-08-18, `docs/evidence/light_mount_led_layout_iso.json`, 166
      Einträge, 111 mit Geometrie)
- [ ] Geometrie für die 55 Leisten-LEDs (Top/Left/Right, aktuell `null`) aus den noch
      unausgewerteten `boundingRect`-Dateien ergänzen
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

- [x] Idiomatischer OpenRGB-Controller-Grundgerüst (2026-08-18,
      `openrgb-integration/Controllers/LightMountController/`) — bewusst minimal:
      nur statische Vollflächenfarbe (einzige mit selbst gewähltem Wert verifizierte
      Funktion), Detection über Interface/UsagePage/Usage, ein Zone/ein LED
- [x] Lokaler Build gegen echten OpenRGB-Quellstand verifiziert (2026-08-18, fehlerfrei
      kompiliert, `./openrgb --list-devices` erkennt "be quiet! Light Mount" korrekt
      mit richtigem `hidraw10`, Seriennummer, Static-Modus, Keyboard-Zone/LED)
- [x] Hardwaretest des neuen Controller-Codes (2026-08-18): Sequenznummer-Start-bei-1
      **widerlegt** — genauso abgelehnt wie `0x2000`. Static-Modus funktioniert noch
      nicht zuverlässig, siehe `PROTOCOL.md`/`openrgb-integration/README.md`.
- [ ] **Kritisch, weiterhin ungelöst:** Herausfinden, wie ein frisch startender Client
      die vom Gerät erwartete Sequenznummer lernen kann — ohne Lösung ist der
      OpenRGB-Controller nicht praktisch nutzbar. Interface-3-Report-ID-1-Telemetrie
      als Quelle geprüft und verworfen (2026-08-18, sieht nach Uptime-Zählern statt
      Sequenznummer aus, siehe `PROTOCOL.md`). Nächste Ideen, keine davon verfolgt:
      andere Report-IDs (2,4,5,6 lieferten nur Nullen, evtl. nach einer echten
      IO-Center-Web-Session neu prüfen), oder grundsätzlich andere Bootstrap-Strategie
      (z. B. erst einen bekannten harmlosen Kommando-Typ mit beliebiger Sequenz senden
      und aus dessen Annahme/Ablehnung lernen — spekulativ, nicht bewertet).
- [x] `Shutdown()`-Aufruf im `RGBController_LightMount`-Destruktor nachgerüstet
      (2026-08-18, verhinderte OpenRGB-Warnung beim Beenden)
- [ ] Per-Key-Adressierung ergänzen, sobald Wire-Kommando bekannt ist
- [ ] Effekte (Matrix, Tornado, ColorWave, Breathing, Reactive) ergänzen, sobald
      Byte-Parameter bekannt sind
- [ ] udev-Regeln mit minimalen Rechten
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
