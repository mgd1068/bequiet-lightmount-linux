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
   Profil reaktivieren). **Erfahrungswert (2026-08-18):** Ein einfacher USB-Replug ist
   bei dieser Hardware **kein zuverlässiger Reset** für einen laufenden Effekt — nach
   Aus-/Wiedereinstecken lief ein zuvor gesendeter Zeitzyklus-Effekt unverändert weiter.
   Ein echtes, per Software gesendetes "sicherer Zustand"-Kommando (z. B. Off) ist noch
   nicht bekannt/verifiziert. **Tatsächlich funktionierender Rückfall:** die Tastatur hat
   eine physische Hotkey-Kombination, die den Effekt zuverlässig auf eine gleichförmige
   statische Farbe zurücksetzt (vom Nutzer bestätigt) — bis ein Software-Off-Kommando
   verifiziert ist, gilt dieser physische Hotkey als der verlässliche Rückfall für
   Hardwaretests, nicht der USB-Replug.
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
- Für Live-Mitschnitte: `tshark`/`dumpcap` sind auf diesem System per AppArmor auf
  Netzwerk-Interfaces beschränkt, `/dev/usbmon*` ist nicht freigegeben. Mitschnitte
  laufen stattdessen über `/sys/kernel/debug/usb/usbmon/<bus><u|t>` (debugfs, `sudo`
  nötig) — bewusste, dokumentierte Abweichung, keine AppArmor-Policy verändert.

## Privacy-Hinweis: Vendor-Kanal spiegelt Tastendrücke (2026-08-18)

Eigener Capture hat gezeigt, dass die Firmware ausgewählte Tastendrücke (beobachtet:
Pfeiltasten, Enter) zusätzlich zum normalen Boot-Keyboard-Interface (Interface 0) auch
unaufgefordert über den Vendor-Kanal (Interface 2, denselben, den WebHID im Browser
nutzt) sendet — siehe `docs/evidence/own_capture_iocenter_decoded.md`. Das bedeutet:
**jede Website mit WebHID-Zugriff auf Interface 2 kann potenziell einzelne Tastendrücke
mitlesen**, obwohl Browser den direkten WebHID-Zugriff auf echte Tastatur-Interfaces aus
Keylogging-Schutzgründen normalerweise verweigern. Für dieses Projekt selbst ändert das
nichts an den obigen Regeln (weiterhin keine Protokollierung sensibler Nutzdaten), ist
aber als eigenständiger, für den Nutzer relevanter Befund festgehalten — kein Bug in
diesem Projekt, sondern ein Verhalten der Hersteller-Firmware.

## Repository-Sichtbarkeit

Bleibt **privat**, bis Captures/Protokolldaten sanitisiert sind und der Benutzer eine
Veröffentlichung ausdrücklich freigibt.
