# Verbindungsaufbau-Capture: Zähler-Feld neu verstanden (2026-08-19)

Gezielter Mitschnitt des **Verbindungsaufbaus** (nicht einer laufenden Session): Nutzer
hatte IO Center Web vollständig geschlossen, Mitschnitt gestartet, dann frisch verbunden.
Aufnahme über `/sys/kernel/debug/usb/usbmon/1u`, gefiltert auf Device 061.

## Zentrale Korrektur: Byte 4 und Byte 5 sind KEIN gemeinsames 16-Bit-Feld

Bisherige Annahme (seit Iteration 2, 2026-08-18): Byte 4-5 = eine 16-Bit-Sequenznummer
(Little Endian). **Das ist falsch.** Die tatsächliche Struktur:

- **Byte 4** ist ein **eigenständiger 1-Byte-Zähler**, der bei jedem einzelnen Kommando
  exakt um 1 steigt — durchgehend, auch über Wechsel des Byte-2/3-Felds hinweg. Im
  Handshake: `02, 03, 04, 05, 06, 07`, dann nahtlos weiter `08, 09, 0a, 0b, 0c, 0d, 0e,
  0f, 10, 11` (kein Reset beim Wechsel von Byte2/3 `0000`→`0002`).
- **Byte 5** ist **kein Zähler-Hochbyte**, sondern variiert unabhängig (vermutlich eine
  Art Attribut-/Abfrage-Kennung je Kommando) — z. B. bei Subcmd `0x01` innerhalb desselben
  Handshakes: Byte 5 = `03`, dann `03`, dann `01`, dann `07` für vier verschiedene
  Aufrufe desselben Subcmd. Für die uns bekannte Static-Color-Kommandofamilie
  (Subcmd `0x06`) ist Byte 5 aber durchgehend konstant `0x10` (bestätigt in jedem
  bisher beobachteten Fall, alt und neu).

## Byte 2-3 ("Session"): mindestens drei verschiedene Werte im selben Verbindungsaufbau

- `0x0000` — erste 6 Kommandos: Identifikations-/Fähigkeiten-Austausch (siehe unten).
- `0x0002` — nächste 10 Kommandos: Einstellungs-/Zustands-Batch direkt nach der
  Identifikation (verschiedene Subcmds: 01, 02, 04, 05, 06 in wechselnder Kombination).
- `0x0001` — nach einer ca. 13-Sekunden-Pause: Tastendruck-Spiegelung (siehe Iteration 11)
  und danach das erste echte periodische Keepalive (Subcmd `0x03`).

Die genaue Bedeutung von Byte 2-3 bleibt unklar (Modul-/Ziel-ID? Verbindungsphase?) —
nicht weiter geraten. Wichtig: der Byte-4-Zähler läuft **unabhängig davon durchgehend
weiter**, wie oben gezeigt.

## Identifikations-Handshake (Byte 2-3 = `0x0000`), erste 6 Kommandos

| Byte4 | Subcmd | Antwort-Payload (Auszug) | Interpretation (Hypothese) |
|---|---|---|---|
| 02 | 02 | ASCII `"004H5380007453"` | Geräte-Identifikations-/Seriennummer-artiger String |
| 03 | 05 | 1 Byte `17` | unklar |
| 04 | 01 | `00 01 01 00 00 03` | evtl. Firmware-/Protokollversion |
| 05 | 01 | identisch zu Byte4=04 | Wiederholung derselben Abfrage |
| 06 | 04 | 17 Byte: `01 01 02 01 03 01 04 01 06 01 07 01 10 01 11 01 12` | sieht nach einer **Liste unterstützter Subcmd-IDs mit Versionsnummer** aus (Paare `XX 01`) — Werte 01,02,03,04,06,07,10,11,12 |
| 07 | 01 | 5 Byte `db 00 00 02 00` | unklar |

Nicht vollständig entschlüsselt — festgehalten für spätere Vertiefung, nicht weiter
geraten.

## Vertiefung (2026-08-19): vollständige Byte-für-Byte-Tabelle, neuer Fund zu Byte 5

Alle 20 Kommando/Antwort-Paare des Verbindungsaufbaus systematisch mit
Länge/Session/Zähler/Marker/Subcmd/Flags + Payload dekodiert (Skript nicht Teil dieses
Repos, siehe `DECISIONS.md` zum Umgang mit Analyse-Einwegskripten).

**Neuer Fund:** Die 17-Byte-„Fähigkeitenliste" aus der Antwort auf Subcmd `0x04`
(`01 01 02 01 03 01 04 01 06 01 07 01 10 01 11 01 12`, gelesen als Paare `[ID] [Version=1]`
mit IDs `01,02,03,04,06,07,10,11,12`) **korreliert direkt mit den Marker-Werten (Byte 5)**,
die in den unmittelbar folgenden Setup-Kommandos verwendet werden (`07,07,06,10,10,11,11,
07,01,01`) — alle liegen innerhalb dieser Liste. Das stützt eine präzisere Deutung: Byte 5
ist eine **Attribut-/Fähigkeits-ID**, und diese Antwort zählt auf, welche Attribut-IDs
das Gerät unterstützt (mit Versionsnummer `1` je Attribut). Passt zur bereits bekannten
Beobachtung, dass Byte 5 für Static-Color-Kommandos konstant `0x10` ist (= eine der
gelisteten Attribut-IDs) und für Keepalive konstant `0x01` (= eine andere gelistete ID).

**Antwort-Byte 7 ("Flags") trägt in Antworten offenbar echte Information, kein reines
Statusbit:** Werte wie `0e, 17, 02, 09, 2a, 1f, e8, 05` etc. traten bei durchweg
erfolgreichen Austauschen auf (keine Fehler, Verbindung lief normal weiter) — nur `0x0a`
ist als konkreter „abgelehnt"-Code anderweitig verifiziert (siehe Sequenznummer-Tests).
Die übrigen Werte sind vermutlich kleine Ergebniswerte der jeweiligen Abfrage, nicht
generische Fehler-Flags. Nicht einzeln gedeutet.

**Ein größerer Antwort-Block (Subcmd `0x01`, Marker `0x11`, 20 Byte Payload) zeigt eine
Struktur aus fünf 4-Byte-Gruppen:** `00 02 00 02` gefolgt von vier Paaren `[07 03] [05 04]
[06 05] [08 06]`, jeweils mit `00 02` aufgefüllt. Sieht nach einer Aufzählung interner
Zonen/Gruppen mit je zwei kleinen Zahlen aus (z. B. Start/Anzahl) — **spekulativ, nicht
verifiziert**, keine erkennbare direkte Entsprechung zu den bekannten 168 LEDs oder den
45/113/10-Aufteilungen aus dem Windows-Manifest. Nicht weiter interpretiert.

## Entscheidender Test: Zähler-Kontinuität bestätigt (echter Hardwaretest)

Mit dem jetzt korrekten Verständnis (Byte 4 = fortlaufender Zähler, Byte 5 = `0x10`
für Static-Color) wurden **zwei aufeinanderfolgende, selbst konstruierte** Kommandos
gesendet — direkt an den in diesem Capture beobachteten letzten Zählerstand (`0x5c`)
angeschlossen:

1. Byte4=`0x5d`, Farbe `#8000FF` (Lila) → **normale Bestätigung**, vom Nutzer live als
   Lila bestätigt.
2. Byte4=`0x5e`, Farbe `#FFFF00` (Gelb) → **normale Bestätigung**, vom Nutzer live als
   Gelb bestätigt.

**Bestätigt:** Sobald der Zähler am korrekten aktuellen Stand des Geräts anknüpft,
funktioniert einfaches Weiterzählen (+1 pro Kommando) zuverlässig — auch für mehrere
aufeinanderfolgende, selbst gewählte Kommandos. Das ist die Grundlage für einen im
Betrieb zuverlässig arbeitenden Controller, **sobald einmal synchronisiert**.

## Verbleibendes Problem: „Kaltstart" ungelöst

Der Zähler ist **nicht bei jeder Verbindung bei 0/1** (dieser „frische" Handshake
begann bei `02`, nicht bei `00`/`01`) — er scheint einen längerfristigen internen
Zustand fortzusetzen, der aber offenbar **irgendwann zurückgesetzt wird** (gestern
endete eine Session bei Zählerstand `~0xad`/`~0x11`-Bereich in unterschiedlichen
Kontexten, heute beginnt der Handshake wieder bei `02` — ohne dass das Gerät
zwischenzeitlich neu enumeriert wurde, siehe `lsusb`). Ob der Reset an einen Timeout,
einen Suspend/Resume-Zyklus des Rechners, oder etwas anderes gekoppelt ist, ist
**nicht geklärt**.

**Für einen praktisch nutzbaren Controller bleibt offen:** wie lernt ein frisch
startender Client (ohne Live-Capture) den aktuell gültigen Zählerstand? Naheliegende,
noch nicht getestete Hypothese: unmittelbar nach einem Reset/Timeout akzeptiert das
Gerät möglicherweise (fast) jeden Startwert als neue Basis (so wie der `02`-Start
dieses Handshakes ohne erkennbare vorherige Abstimmung akzeptiert wurde) — d. h. das
Problem wäre nicht "der Zähler ist unbekannbar", sondern "unser eigener Schreibversuch
konkurriert mit einer noch aktiven Session eines anderen Clients (Browser)". Nicht
verifiziert — würde einen Test mit gesichert langer Inaktivität (kein Browser-Tab
offen, längere Wartezeit) erfordern.

## Negativer Datenpunkt: kein kurzfristiger Timeout (2026-08-19, später am Tag)

Rund eine Stunde nach dem letzten erfolgreichen Test (Zählerstand `0x5e`, siehe oben)
und ohne zwischenzeitliche Aktivität (kein Browser-Tab offen) wurde der fortlaufende
Wert `0x5f` erneut getestet: **anstandslos akzeptiert**, Magenta (`#FF00FF`) erfolgreich
gesetzt und vom Nutzer live bestätigt.

**Schlussfolgerung:** Ein Reset auf einer Zeitskala von Minuten bis niedrigen Stunden
ist damit unwahrscheinlich. Der zwischen den Sessions vom 18. und 19. beobachtete
Reset (Zähler fiel von `~0xad`/`~0x11`-Bereich auf `02`) liegt entweder auf einer
deutlich längeren Zeitskala (viele Stunden, über Nacht) oder ist an ein anderes
Ereignis geknüpft (z. B. Rechner-Standby/Suspend zwischen den Sessions) statt an
reine kurzfristige Inaktivität. Für einen praktisch nutzbaren Controller bedeutet das:
**innerhalb einer Arbeitssitzung kann ein einmal ermittelter Zählerstand vermutlich
über Stunden hinweg zuverlässig weiterverwendet werden** — das Kaltstart-Problem
betrifft in erster Linie den Sitzungsbeginn (z. B. nach Neustart des Rechners), nicht
laufenden Betrieb.
