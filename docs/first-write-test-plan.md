# Testplan: erster realer Schreibzugriff auf die Light Mount

Status: **Ausgeführt am 2026-08-18, mit expliziter Nutzerfreigabe.** Ergebnis: Kommando
erfolgreich angewendet, kein USB-Reset, sichtbare Wirkung bestätigt (mit einer
Korrektur der ursprünglichen Interpretation — siehe `PROTOCOL.md`, Abschnitt „Erster
eigener Hardwaretest"). Dieses Dokument beschreibt den Plan, wie er vor der Ausführung
formuliert und eingehalten wurde.

## Ziel

Bestätigen, dass ein bereits aus dem öffentlichen usbmon3-Capture bekanntes,
byteidentisch reproduziertes Kommando (Frame 1453/3531, Interface 2, Subcmd `0x06`,
6-Stopp-Regenbogen-Gradient) auf echter Hardware eine sichtbare, flüchtige
Beleuchtungsänderung auslöst — ohne Onboard-Speicherung, ohne selbst konstruierte Bytes.

## Warum genau dieses Kommando (Risikominimierung)

- **Bereits bekannt, nicht neu konstruiert:** exakt dieselben 64 Byte wie im Capture,
  inklusive der dort beobachteten (gültigen) CRC — kein Risiko durch einen eigenen
  Fehler im Report-Aufbau.
- **Kein Speicherbefehl:** Subcmd `0x06` über Interface 2 EP4 OUT, nicht der (bisher nur
  aus dem Master-Prompt vermuteten, nie bestätigten) Save-Befehlsfamilie. Flüchtig laut
  `SECURITY.md` Regel 4.
- **Reversibel:** falls das Ergebnis unerwartet ist, ist der einzige Rückfall nötig, den
  dieser Plan ohnehin vorsieht (siehe unten).

## Zielinterface (exakt, keine anderen Geräte berühren)

- VID `0x373f`, PID `0x0002`, **Interface 2 ausschließlich**.
- Auswahl über sysfs-Pfad `/sys/bus/usb/devices/1-2:1.2/0003:373F:0002.*/hidraw/hidrawN`
  (Nummer `N` vorher frisch ermitteln, nicht die aus Iteration 1 dokumentierte Nummer
  fest verdrahten — sie ist laut `PROTOCOL.md` hostabhängig/nicht stabil).
- Kein `cat`/blindes Öffnen anderer `hidraw`-Geräte. Die SteelSeries Apex 3 (angeschlossene
  Zweittastatur, siehe Nutzerhinweis) wird nicht berührt.

## Ablauf

1. `report_dump` erneut auf das Zielkommando anwenden, `crc_valid=yes` bestätigen
   (bereits erledigt, siehe Iteration 6 — hier nur als Vorbedingung wiederholt).
2. Aktuellen `hidraw`-Pfad für Interface 2 frisch aus sysfs ermitteln (read-only).
3. Baseline sichern: aktuellen sichtbaren Beleuchtungszustand der Light Mount kurz
   notieren (manuelle Beobachtung durch den Nutzer, kein Code nötig).
4. Timeout setzen: `hid_write`/`write()` auf das `hidraw`-Device mit Timeout (z. B. 500ms)
   — kein blockierender Aufruf ohne Zeitlimit.
5. Genau ein Schreibvorgang: die 64 bekannten Bytes senden.
6. Ergebnis beobachten:
   - Erwartung: Light Mount zeigt den Regenbogen-Farbverlauf.
   - Bei USB-Reset/Disconnect (bekannter Firmwarefehler, siehe `SECURITY.md`): als
     erwarteten Testfehler behandeln, Handle schließen, **nicht** in einer Schleife
     erneut versuchen. Einmaliger Re-Enumerationsversuch nach kurzer Wartezeit erlaubt,
     danach abbrechen und Ergebnis dokumentieren.
7. Rückfall auf bekannten sicheren Zustand: falls das Ergebnis unerwartet ist oder die
   Beleuchtung in einem unklaren Zustand hängen bleibt — Gerät kurz vom USB trennen und
   neu verbinden (Power-Cycle über USB-Replug), das ist laut `SECURITY.md` Regel 7 der
   einfachste verfügbare sichere Zustand für ein rein flüchtiges Kommando.
8. Keine Wiederholung mit variierten/eigenen Bytes in diesem ersten Test — nur exakte
   Reproduktion des bekannten Kommandos.

## Werkzeug

Kleine Erweiterung von `report_dump` (oder ein neues, klar benanntes Executable, z. B.
`report_send --confirm`) mit explizitem Bestätigungs-Flag, das ohne dieses Flag
ausschließlich im Dry-Run-Modus (nur Parsen/Drucken, kein `open()` auf `hidraw`) läuft —
siehe `SECURITY.md` Regel 10. Wird erst nach Freigabe dieses Plans implementiert.

## Nicht Teil dieses ersten Tests

- Keine Onboard-Speicherung (kein Save-Kommando).
- Keine Per-Key-Adressierung, keine Interface-3-Kommandos.
- Keine Firmware-Interaktion jeglicher Art.
- Keine automatisierte Wiederholung/Schleife.

## Freigabe

Dieser Plan erfordert eine explizite Nutzerfreigabe vor Ausführung, da es sich um den
ersten echten Schreibzugriff auf ein aktuell am Nutzersystem angeschlossenes,
möglicherweise aktiv genutztes Eingabegerät handelt (siehe bekannter Firmwarefehler:
möglicher kurzer USB-Reset/Verbindungsabriss während der Nutzung).

## Ergebnis (2026-08-18)

- Werkzeug `report_send` gebaut: ohne `--confirm` reiner Dry-Run (kein `open()`), mit
  `--confirm` genau ein `write()` mit 500ms-Poll-Timeout, keine Retry-Schleife.
- Ablaufschritte 1–5 wie geplant durchgeführt: `report_dump` bestätigte `crc_valid=yes`
  vor dem Senden, `hidraw`-Pfad frisch aus sysfs ermittelt (`hidraw10`, Interface 2),
  Nutzer vor dem Schreiben auf die Tastatur hingewiesen, ein einziger Schreibvorgang.
- Ergebnis: `write completed`, kein USB-Reset (`lsusb`/`dmesg` direkt danach geprüft,
  Gerät blieb durchgehend erreichbar). Sichtbare Wirkung vom Nutzer bestätigt.
- **Abweichung von der Erwartung, kein Fehler:** Die Wirkung war ein zeitlicher
  Farbzyklus (alle Tasten gleichzeitig, Farbe wechselt über die Zeit durch den
  Regenbogen), nicht der ursprünglich angenommene räumliche Verlauf über die Tasten.
  Kein Rückfall/USB-Replug nötig, da kein Fehlerzustand vorlag — nur eine falsche
  Interpretation der Payload-Semantik, jetzt in `PROTOCOL.md` korrigiert.
- Schritt 8 (keine Wiederholung mit eigenen/veränderten Bytes) eingehalten — es wurde
  ausschließlich das exakt bekannte Kommando einmalig gesendet.
