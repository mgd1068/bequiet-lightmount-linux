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

- [x] Repository sicher angelegt, privat, nachvollziehbare Historie
- [x] Quellen, Lizenzen, Architektur, Sicherheitsregeln dokumentiert
- [x] Light Mount wird ausschließlich über exakte Geräte-/Interfaceidentität geöffnet
- [ ] Offline-Tests für Paketaufbau und Tasten-/LED-Mapping bestehen (Paketaufbau: ja,
      `tests/test_protocol.cpp`; Tasten-/LED-Mapping: nein, Matrix noch unbekannt)
- [ ] Mindestens zwei einzelne reale Tasten unabhängig in verschiedenen Farben setzbar
      (bisher nur Vollflächen-/einheitliche Farben getestet; Ziel-LED-Tabelle jetzt
      bekannt — 168 LEDs, siehe `PROTOCOL.md` „Statische Analyse der Windows-App" —
      aber das Wire-Kommando dafür weiterhin unbekannt)
- [ ] Vollständige statische Tastenbelegung auf echter Hardware darstellbar
- [ ] Obere/seitliche Lichtleisten als getrennte Zonen steuerbar (oder Einschränkung mit Capture-Belegen dokumentiert)
- [ ] USB-Disconnect/Reset führt nicht zu Absturz oder unbegrenzter Reconnect-Schleife
      (Timeout/No-Retry-Logik implementiert, aber noch kein echter Reset im Test aufgetreten)
- [ ] Funktionierender OpenRGB-Controller, oder begründeter Nachweis für vorgelagerte Protokollbibliothek
- [ ] Build, Tests, Bedienung auf Zielsystem reproduzierbar dokumentiert (für den bisherigen
      Umfang ja, siehe README — für den vollen Funktionsumfang noch nicht anwendbar)
- [ ] Vorheriger Beleuchtungszustand bzw. sicherer Fallback wiederherstellbar (USB-Replug
      erwiesenermaßen unzuverlässig, nur physischer Hotkey verifiziert, kein Software-Fallback)
- [x] Keine Rohmitschnitte oder Geheimnisse veröffentlicht (Repo bleibt ohnehin privat)

**Teilfortschritt (2026-08-18, noch kein MVP-Abschluss):** Statische Gesamtfarbe frei
wählbar setzen ist auf echter Hardware verifiziert (zwei unabhängige Farbwerte,
`#1FB4FF` und `#00FF00`) — deckt aber nur einen Teil der obigen Kriterien ab. Per-Key-
Adressierung, Zonentrennung, OpenRGB-Controller und Fallback-Wiederherstellung fehlen
weiterhin. Siehe `STATE.md`/`BACKLOG.md` für Details.

„Fast fertig“, kompilierender Code ohne Hardwarenachweis oder ausschließlich gemockte
Tests reichen nicht.

## Nicht im Scope (bis gesonderte Freigabe)

- Firmwareupdates, Schreiben von Firmwareblöcken, Bootloader-Kommandos
- Veröffentlichung des Repos (bleibt privat, bis Captures sanitisiert und User freigibt)
- Automatische Upstream-Veröffentlichung eines OpenRGB-Patches ohne dokumentierte Hardwaretests
