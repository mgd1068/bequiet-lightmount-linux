# SECURITY

Verbindliche Regeln im Umgang mit der Light-Mount-Hardware. Diese Regeln sind nicht
verhandelbar und gelten für jede Iteration des Entwicklungsloops.

1. **Niemals** `cat /dev/hidraw*`, niemals alle HID-Geräte probeweise öffnen oder lesen.
2. Zielgerät ausschließlich über VID `0x373f`, PID `0x0002`, Interface-Nummer, Usage Page
   und Usage auswählen.
3. Reihenfolge: zuerst sysfs und Deskriptoren lesen; danach bekannte Captures offline
   analysieren; erst anschließend kontrollierte Gerätezugriffe.
4. Zunächst nur flüchtige Beleuchtung testen. Schreiben in den Onboard-Speicher erst nach
   verstandenem Protokoll und explizitem Testplan.
5. **Keine** Firmwareupdates, kein Schreiben von Firmwareblöcken, keine Bootloader-Kommandos.
   Außerhalb des Scopes bis zu gesonderter, dokumentierter Freigabe.
6. Jeder Hardwarebefehl braucht Timeout, Fehlerbehandlung, Reconnect und nachvollziehbare
   Hex-Dump-Protokollierung ohne sensible Nutzdaten.
7. Immer einen bekannten sicheren Zustand bereitstellen (Beleuchtung aus oder vorhandenes
   Profil reaktivieren).
8. USB-Reset ist ein erwartbarer Testfehler: erkennen, Handle schließen, Gerät neu
   enumerieren, nur nach begrenztem Retry fortsetzen. Keine Endlosschleife gegen ein
   resettendes Gerät.
9. Hardwaretests seriell ausführen. Keine parallelen Writer auf demselben HID-Interface.
10. Vor persistenten oder noch unbekannten Kommandos: `--dry-run` bzw. explizite
    Testfreigabe im Programm verlangen.

## Umgang mit Captures

- PCAP-Dateien gelten als potenziell sensibel (Tastendrücke, Gerätepfade, Seriennummern,
  Tokens, Benutzerdaten möglich).
- Rohe Captures werden **niemals automatisch** in ein öffentliches oder dieses Repository
  gepusht — sie gehören lokal in `captures-private/` (git-ignoriert).
- Nur sanitisiertes, minimales Testmaterial wird committet.
- Vor jeder Veröffentlichung (auch nur des Repos) explizit prüfen, ob Captures/Logs
  sensible Daten enthalten.

## Repository-Sichtbarkeit

Bleibt **privat**, bis Captures/Protokolldaten sanitisiert sind und der Benutzer eine
Veröffentlichung ausdrücklich freigibt.
