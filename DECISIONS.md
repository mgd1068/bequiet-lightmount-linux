# DECISIONS

Kurze Architecture Decision Records. Neueste zuerst.

## 2026-08-19 — Controller verlangt explizites Zähler-Priming statt Rätselei

**Kontext:** Verbindungsaufbau-Capture zeigt, dass Byte 4 ein eigenständiger,
gerätegeführter Zähler ist (nicht Teil einer 16-Bit-Sequenznummer, siehe `PROTOCOL.md`).
Weder ein fester niedriger (`1`) noch ein hoher (`0x2000`) Startwert wird vom Gerät
akzeptiert — nur ein an den echten Gerätestand angeschlossener Wert funktioniert
(bestätigt mit zwei aufeinanderfolgenden echten Schreibtests).

**Entscheidung:** `LightMountController` rät den Startwert nicht mehr. Stattdessen:
`SetCounter(uint8_t)` muss explizit aufgerufen werden, bevor `SendStaticColor()`
überhaupt einen Schreibversuch unternimmt (`IsCounterPrimed()` davor `false`,
`SendStaticColor()` gibt in diesem Zustand sofort `false` zurück, ohne zu schreiben).

**Konsequenz:** Der Controller ist bis zur Lösung des Kaltstart-Problems nicht ohne
externe Hilfe (aktuell: manueller Live-Capture-Wert) nutzbar — das ist ehrlicher als ein
Controller, der bei jedem Start unbemerkt einen abgelehnten Schreibversuch unternimmt.
Der erste Controller-Entwurf (Iteration 19) hatte trotz der Vorgabe unten (Eintrag vom
gleichen Tag, „nicht willkürlich/bei 0 beginnend") versehentlich doch bei 1 begonnen —
jetzt korrigiert durch explizites Priming statt eines Default-Werts.

## 2026-08-18 — OpenRGB-Anbindung: kein Submodule, Contribution-Dateien im Hauptrepo

**Kontext:** Phase 3 (OpenRGB-Controller) beginnt. Offene Frage aus Iteration 2
("Submodule-Entscheidung noch offen") muss geklärt werden.

**Entscheidung:**
- Der OpenRGB-Quellstand selbst ist **kein Git-Submodule** dieses Repos — er lebt lokal
  unter `openrgb-src-private/` (git-ignoriert), nur als Build-/Testumgebung, analog zu
  `vendor-extracts-private/`.
- Die eigentlichen Controller-Dateien (`LightMountController.{h,cpp}`,
  `RGBController_LightMount.{h,cpp}`, `LightMountControllerDetect.cpp`) sind **von uns
  verfasster Code** und leben getrackt in diesem Repo unter
  `openrgb-integration/Controllers/LightMountController/` — sie werden zum Bauen/Testen
  in den lokalen OpenRGB-Checkout kopiert, siehe `openrgb-integration/README.md`.

**Konsequenz:** Unser Repo bleibt schlank (kein Kopieren von OpenRGBs kompletter
Historie/Codebase), die Contribution-Dateien sind trotzdem versioniert und
nachvollziehbar, und ein künftiger Merge Request lässt sich direkt aus
`openrgb-integration/Controllers/LightMountController/` heraus vorbereiten.

## 2026-08-18 — Extrahiertes Herstellermaterial bleibt lokal, nicht im Repo

**Kontext:** Statische Analyse der Windows-App (`docs/evidence/windows-app-static-analysis.md`)
hat proprietäre JSON-Manifeste (Gerätedaten, LED-Tabellen, UI-Layouts) freigelegt.

**Entscheidung:** Rohe extrahierte Dateien liegen unter `vendor-extracts-private/`
(git-ignoriert), analog zu `captures-private/`. Ins Repo fließen nur destillierte,
in eigenen Worten zusammengefasste Fakten (Zahlen, Namen, Struktur) plus kurze,
punktuelle Zitate zur Beleglage — keine vollständigen Kopien der Hersteller-JSON-Dateien.

**Konsequenz:** Reverse Engineering zu Interoperabilitätszwecken bleibt der erklärte
Zweck (siehe `README.md`), aber ohne unnötige Vervielfältigung fremden proprietären
Materials im (später eventuell öffentlichen) Repository.

## 2026-08-18 — Sequenznummern nicht frei vergeben, sondern aus Gerätezustand fortführen

**Kontext:** Hardwaretest zeigte, dass das Gerät Sequenznummern bei Static-Color-
Kommandos validiert (siehe `PROTOCOL.md`, `docs/evidence/sequence-number-validation-test.md`).
Frei erfundene Werte wurden abgelehnt, ein Wert nahe am zuletzt real beobachteten Stand
wurde akzeptiert.

**Entscheidung:** Eine künftige Implementierung (Dry-Run-CLI-Erweiterung, später
OpenRGB-Controller) darf Sequenznummern nicht willkürlich/bei 0 beginnend vergeben.
Stattdessen entweder (a) den aktuellen Zählerstand vom Gerät lernen (z. B. aus der
Antwort eines beliebigen zuerst gesendeten, unkritischen Kommandos wie dem Keepalive
Subcmd `0x03`) und ab dort fortlaufend inkrementieren, oder (b) empirisch ein
akzeptiertes Toleranzfenster ermitteln, bevor darauf gebaut wird.

**Konsequenz:** Kein eigener Sequenznummerngenerator, der bei einem festen/konfigurierbaren
Startwert beginnt, ohne vorher den Gerätezustand zu berücksichtigen. Offene Frage
(genaue Akzeptanzregel) bleibt in `BACKLOG.md`, bis weitere Tests sie klären.

## 2026-08-18 — Coding-Stil: C++, aber möglichst funktional/C-artig statt OOP

**Kontext:** Nutzerwunsch (explizit): das Projekt soll für ihn leicht lesbar bleiben.
Sprache ist C++ zur OpenRGB-Kompatibilität (siehe Eintrag unten), aber OOP soll nicht
der Standardstil sein.

**Entscheidung:**
- Standardmäßig freie Funktionen, plain structs (POD, keine Kapselung um ihrer selbst
  willen), klare Datenflüsse statt Klassenhierarchien.
- Vererbung/Polymorphie/Klassen nur dort, wo sie einen klaren, konkreten Vorteil bringen
  — insbesondere wo OpenRGBs eigene API es verlangt: Controller müssen von `RGBController`
  erben, Detectoren folgen OpenRGBs Registrierungsmuster. Dieser API-Rand ist unvermeidbares
  OOP, alles darunter (Protokoll-Encoding, HID-I/O, Report-Aufbau, CLI/Testgerüst aus
  Phase 1) wird C-artig gehalten.
- Keine zusätzlichen Abstraktionsschichten, Interfaces oder Factory-Patterns ohne
  konkreten, aktuell bestehenden Bedarf.

**Konsequenz:** Bei jeder Iteration, die Code schreibt (ab Phase 1), gilt dieser Stil als
Review-Kriterium — vor dem Commit prüfen, ob unnötige Klassen/Vererbung eingeschlichen sind.

## 2026-08-18 — Projektstart, GPL-2.0-or-later, OpenRGB als Zielplattform

**Kontext:** Neues Projekt zur Linux-Steuerung der be quiet! Light Mount. Auftrag
verlangt Integration in OpenRGB statt einer eigenen RGB-Plattform, da GPL-kompatibler
Referenzcode (Mountain-Controller) potenziell wiederverwendet wird.

**Entscheidung:**
- Lizenz: `GPL-2.0-or-later`.
- Zielarchitektur: Hardwareprotokoll/LED-Topologie in OpenRGB; eigener Code nur für
  Automatisierung/Benachrichtigungen (dünner SDK-Client), siehe `ARCHITECTURE.md`.
- Repository bleibt privat, bis Captures sanitisiert und Nutzer Veröffentlichung freigibt.

**Konsequenz:** Jede Übernahme von OpenRGB-Code muss Herkunft und Änderungen dokumentieren.
Kein Fork/keine Kopie von OpenRGB als Ganzes ohne separate Prüfung, ob Submodule oder
eigenständiger Patch/MR der richtige Weg ist (offene Frage, siehe `STATE.md` Schritt 4).
