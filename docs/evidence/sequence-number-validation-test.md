# Diagnose: Sequenznummer-Validierung bei Static-Color-Kommandos (2026-08-18)

Nach der Entschlüsselung der RGB-Kodierung (`own_capture_iocenter_decoded.md`) erster
Versuch, ein **selbst konstruiertes** (nicht byteidentisch aus einem Capture kopiertes)
Kommando zu senden: Static-Color-Report, Subcmd `0x06`, RGB `#00FF00`, mit frei gewählter
Sequenznummer `0x2000`. Alle Schritte mit Nutzerfreigabe, ein Schreibvorgang pro Versuch,
`report_dump`/`crc_valid` vor jedem Senden geprüft.

## Verlauf

1. **Erster Versuch** (Session `0x02`, Seq `0x2000`, RGB `00 ff 00`): `report_send`
   meldete `write completed`, aber **keine sichtbare Farbänderung**. Response zunächst
   nicht ausgelesen.
2. Antwort nachträglich ausgelesen (direkter `hidraw`-Read nach erneutem Senden):
   Statt der bei allen bisherigen Kommandos beobachteten kurzen 6-Byte-Bestätigung kam
   eine **vollständige Echo-Antwort** (Länge = Anfragelänge, hier 15) mit einem
   geänderten Byte 3: gesendet `0x00`, empfangen `0x0a`. CRC der Antwort selbst gültig
   (kein Übertragungsfehler).
3. **Hypothese „Sitzungs-ID falsch" getestet und widerlegt:** Sowohl Session `0x02`
   als auch `0x01` (letztere real aus dem Live-Capture bekannt) ergaben identisch
   `byte3=0x0a`. Browser-Tab wurde zusätzlich geschlossen — keine Änderung.
4. **Hypothese „fehlendes Commit-Kommando" getestet und widerlegt:** Grün-Kommando
   gefolgt vom bisher unzugeordneten Subcmd-`0x0a`-Kommando (Frame 2997 aus dem alten
   Capture) gesendet — auch das Commit-Kommando selbst bekam `byte3=0x0a` als Antwort.
   Keine Verbesserung.
5. **Kontrolltest:** Das bereits bestätigt funktionierende Orange-Preset-Kommando
   (Frame 2747, byteidentisch) über exakt dieselbe Sende-Methode (rohes `open()`/
   `write()`/`read()` auf `hidraw`) erneut gesendet → **normale kurze Bestätigung**
   (`byte3=0x00`), identisch zur historischen Aufzeichnung. Schließt aus, dass unsere
   Sendemethode selbst die Ursache ist.
6. **Entscheidender Test:** Gleiches Grün-Kommando, aber mit der **echten, live
   beobachteten Sequenznummer `0x10ad`** (statt frei erfundener `0x2000`er-Werte) und
   Session `0x01`. Ergebnis: **normale kurze Bestätigung** (`byte3=0x00`) UND vom
   Nutzer bestätigt sichtbar **Grün**.

## Bestätigter Fakt

Das Gerät validiert die Sequenznummer bei Static-Color-Kommandos (mindestens bei
Subcmd `0x06` dieser Form) gegen einen internen Zustand. Frei erfundene, weit entfernte
Werte (`0x2000`+) werden mit einer abweichenden Antwortform (voller Echo statt kurzer
Bestätigung, Byte 3 = `0x0a` statt `0x00`) zurückgewiesen und **keine sichtbare Wirkung**
entfaltet. Ein Wert nahe am zuletzt real beobachteten Stand (`0x10ad`) wird akzeptiert
und wirkt sichtbar.

**Nicht bestätigt:** die genaue Regel (Toleranzfenster? exakte Fortsetzung des internen
Zählers? Verfällt nach Zeit oder nach Trennung?). Nicht raten — für eine spätere
OpenRGB-Integration muss die Sequenznummer vermutlich aus einem beobachteten
Ausgangspunkt fortgeführt werden, nicht frei gewählt werden. Betrifft möglicherweise nur
diese eine Kommandofamilie — Rainbow- und Preset-Kommandos (Subcmd `0x06`, andere
Payload-Form) akzeptierten in früheren Tests beliebige alte Sequenznummern
klaglos, d. h. die Validierung ist nicht global für alle Subcmd-`0x06`-Kommandos gleich
streng, oder betrifft nur bestimmte Payload-Formen.

## Neu identifiziert: Ablehnungs-/Fehlerantwortformat

Bei Ablehnung antwortet das Gerät nicht mit der üblichen 6-Byte-Bestätigung, sondern mit
einem vollständigen Echo der Anfrage (gleiche Länge, gleicher Inhalt) und Byte 3 auf
`0x0a` gesetzt (statt `0x00`). Bedeutung von `0x0a` als konkretem Fehlercode nicht
verifiziert (nur ein einziger abweichender Wert beobachtet) — als Muster „Antwortlänge
= Anfragelänge UND Byte 3 ≠ 0x00" festgehalten, nicht als universeller Fehlercode `0x0a`
missverstehen.

## Konsequenz für die Architektur (Phase 3)

Eine künftige Implementierung darf Sequenznummern nicht frei/beliebig vergeben. Nötig:
entweder (a) den aktuellen Zählerstand vom Gerät ablesen/lernen, bevor eigene Kommandos
gesendet werden, oder (b) ausschließlich fortlaufend ab einer beobachteten realen
Sequenznummer inkrementieren, nicht bei einem willkürlichen Wert neu beginnen. Siehe
`BACKLOG.md`.
