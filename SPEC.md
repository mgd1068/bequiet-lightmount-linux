# SPEC

Verbindliche Anforderungen und Abnahmekriterien. Quelle: `LIGHTMOUNT_AGENT_LOOP_PROMPT.md`
(privater Auftrags-Master-Prompt, nicht Teil dieses Repos).

## Zielgerät

be quiet! Light Mount, DE ISO, Silent-Linear-Switches. USB VID `0x373f`, PID `0x0002`.

## Funktionsumfang (Zielzustand, siehe README für Kurzfassung)

1. Einzeladressierung jeder Tasten-LED
2. Getrennte Steuerung oberer/seitlicher ARGB-Leisten
3. Helligkeit, statische Farben, Hardwareeffekte, Direct Mode
4. Profile lesen/schreiben/wechseln, kontrolliertes Onboard-Speichern
5. Tasten-/Makro-Remapping, Medienrad
6. Stabile lokale Automatisierungsschnittstelle
7. Subtile Statusdarstellung über einzelne LEDs

Keine unnötige zweite RGB-Plattform: Integration in OpenRGB hat Vorrang vor eigenem Daemon/eigener GUI.

## Abnahmekriterien RGB-MVP

Der Loop darf den RGB-MVP nur als abgeschlossen markieren, wenn alle folgenden Punkte
erfüllt **und mit realen Testbelegen dokumentiert** sind:

- [ ] Repository sicher angelegt, privat, nachvollziehbare Historie
- [ ] Quellen, Lizenzen, Architektur, Sicherheitsregeln dokumentiert
- [ ] Light Mount wird ausschließlich über exakte Geräte-/Interfaceidentität geöffnet
- [ ] Offline-Tests für Paketaufbau und Tasten-/LED-Mapping bestehen
- [ ] Mindestens zwei einzelne reale Tasten unabhängig in verschiedenen Farben setzbar
- [ ] Vollständige statische Tastenbelegung auf echter Hardware darstellbar
- [ ] Obere/seitliche Lichtleisten als getrennte Zonen steuerbar (oder Einschränkung mit Capture-Belegen dokumentiert)
- [ ] USB-Disconnect/Reset führt nicht zu Absturz oder unbegrenzter Reconnect-Schleife
- [ ] Funktionierender OpenRGB-Controller, oder begründeter Nachweis für vorgelagerte Protokollbibliothek
- [ ] Build, Tests, Bedienung auf Zielsystem reproduzierbar dokumentiert
- [ ] Vorheriger Beleuchtungszustand bzw. sicherer Fallback wiederherstellbar
- [ ] Keine Rohmitschnitte oder Geheimnisse veröffentlicht

„Fast fertig“, kompilierender Code ohne Hardwarenachweis oder ausschließlich gemockte
Tests reichen nicht.

## Nicht im Scope (bis gesonderte Freigabe)

- Firmwareupdates, Schreiben von Firmwareblöcken, Bootloader-Kommandos
- Veröffentlichung des Repos (bleibt privat, bis Captures sanitisiert und User freigibt)
- Automatische Upstream-Veröffentlichung eines OpenRGB-Patches ohne dokumentierte Hardwaretests
