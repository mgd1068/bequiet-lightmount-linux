# STATE

## Aktuelle Phase

Phase 0 — Bestand und Reproduzierbarkeit.

## Letzter Stand

2026-08-18: Repository angelegt (privat), Pflichtdokumentation (`README.md`, `LICENSE`,
`SPEC.md`, `ARCHITECTURE.md`, `PROTOCOL.md`, `SECURITY.md`, `STATE.md`, `BACKLOG.md`,
`DECISIONS.md`, `docs/research-sources.md`) initial befüllt aus dem Master-Prompt
(`LIGHTMOUNT_AGENT_LOOP_PROMPT.md`, nicht Teil dieses Repos). Noch keine Hardware
angefasst, noch kein Code, noch kein Capture heruntergeladen/analysiert.

## Nächster konkreter Schritt

1. System/Umgebung read-only erfassen: Kernel, installierte OpenRGB-Version (falls
   vorhanden), USB-Topologie (`lsusb -t`), ob die Light Mount aktuell angeschlossen ist.
2. `lsusb -v` für `373f:0002` (nur lesen, keine `hidraw`-Zugriffe) und sysfs-Deskriptoren
   für die vier HID-Interfaces sichern.
3. OpenRGB Issue #4950 auf neue Aktivität/Code prüfen, vorhandenen Capture-Anhang aus der
   Originalquelle laden, offline (nicht committet) ablegen unter `captures-private/`.
4. Aktuellen OpenRGB-Quellstand holen (lokaler Clone/Submodule-Entscheidung noch offen,
   siehe `DECISIONS.md`), Mountain-Everest-Controller lesen.
5. Erste überprüfbare Protokollhypothese formulieren, sicheres Offline-Testgerüst (Dry-Run
   CLI/Testbibliothek) beginnen.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Interface 2 (laut Ausgangsdeskriptor `0x83`/`0x04`) ist der von
  IO Center Web genutzte Konfigurationskanal.
- **Erwartetes Ergebnis:** sysfs-Deskriptoren bestätigen Interface-Anzahl, Endpoints und
  Usage Page, ohne dass ein Gerätezugriff nötig ist.
- **Sicherheitsrisiko:** keins bei reinem sysfs-/`lsusb`-Lesen; **kein** `hidraw`-Zugriff
  in diesem Schritt.
- **Rückfall:** falls Gerät nicht angeschlossen ist — Schritt anhand des im Prompt
  dokumentierten Ausgangsdeskriptors vorbereiten, echten Test verschieben und als
  externen Blocker (fehlender Hardwarezugriff) in diesem Dokument vermerken.

## Blocker

Keiner. Warte auf Entscheidung des Nutzers, ob/wie der 30-Iterationen-Loop jetzt
gestartet werden soll (manuell in dieser Session vs. `/ralph-loop`-Plugin, siehe
Master-Prompt, Abschnitt "Aufrufhinweise").
