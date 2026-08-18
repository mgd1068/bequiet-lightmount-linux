# DECISIONS

Kurze Architecture Decision Records. Neueste zuerst.

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
