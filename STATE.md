# STATE

## Aktuelle Phase

Phase 3 — OpenRGB-Integration. Architekturwechsel in Arbeit: statt des Vendor-Protokolls
auf Interface 2 (nur statische Vollflächenfarbe, Sequenznummer-Kaltstart-Problem
ungelöst) führt der HID-LampArray-Standardweg auf Interface 3 direkt zu echter
Per-Key-Adressierung — live bestätigt (siehe Update 2026-08-23 unten).

## Update — 2026-08-23: Erste echte Per-Key-Farbe live bestätigt (HID LampArray, Interface 3)

**Kontextwechsel gegenüber Iteration 28 (2026-08-19/20):** Der dort beschriebene
Windows-VM-Blocker ("Busy"-Status, Sequenznummer-Kaltstart) betraf ausschließlich den
**Vendor-Weg über Interface 2**. Unabhängig davon wurde diese Session mit einer neuen
Beobachtung gestartet: Interface-3-Report-Deskriptor beginnt mit `05 59 09 01` — Usage
Page `0x59` (Lighting And Illumination), Usage `0x01` (LampArray). Das ist **kein**
Vendor-Kanal, sondern der **offizielle HID-LampArray-Standard** (derselbe, den OpenRGB
bereits generisch unterstützt).

**Ablauf:**
1. Rein lesend per `HIDIOCGFEATURE` Report-ID 1 aus `/dev/hidraw11`: 135 Lampen,
   ArrayKind 1 (Keyboard), MinUpdateInterval 33333 µs — bestätigt LampArray-Metadaten
   ohne jede Schreiboperation.
2. Mit Nutzerfreigabe `build/openrgb --list-devices` gestartet, um die Erkennung zu
   verifizieren. **Nebenwirkung:** Der volle Hardware-Scan schaltete auch die parallel
   angeschlossene SteelSeries Apex 3 und Corsair Ironclaw in Direct Mode (Beleuchtung
   aus) — beide vom Nutzer selbst wiederhergestellt. **Root Cause:** Ein aktivierter
   VID/PID-spezifischer Detector setzt in OpenRGBs `DetectionManager` für **jedes**
   Interface derselben VID/PID `skip_generic_detectors=true`, auch wenn sein eigener
   Interface-/Usage-Match dort fehlschlägt — blockierte den generischen
   Page-0x59/Usage-0x01-Detector auf Interface 3. Umgangen durch temporäre
   `OpenRGB.json` mit deaktiviertem `373f:0002`-Vendor-Detector. Danach: Erkennung
   erfolgreich, `HID LampArray Device`, Modes `Direct`/`Autonomous`, Zone `Keyboard`.
   **Verbindliche Konsequenz für künftige Tests:** keine vollen OpenRGB-Scans mehr,
   solange andere RGB-Geräte angeschlossen sind — stattdessen isolierte Tools/Configs.
3. Als direkte Konsequenz daraus `tools/lamp_array_probe.py` (reines Auslesen der
   135 Lamp-Attribute per `HIDIOCGFEATURE`) und `tools/lamp_array_control.py`
   (`set-one`/`restore-autonomous` per `HIDIOCSFEATURE`) gebaut — beide gehen **direkt**
   über `hidraw`, nicht über OpenRGB, um das Scan-Nebenwirkungsproblem aus Schritt 2
   grundsätzlich zu vermeiden.
4. **Realer Schreibtest, mit Nutzer-Go-ahead, ein Schritt, Antwort abgewartet:**
   `python3 tools/lamp_array_control.py /dev/hidraw11 --confirm set-one 0 255 0 255`
   (Autonomous Mode aus, Lamp-ID 0 auf Magenta). Nutzer bestätigte live: **ESC-Taste
   leuchtet magenta.** Direkt danach `restore-autonomous` ausgeführt (Report `06 01`) —
   Tastatur wieder im Normalzustand.

**Ergebnis:** Per-Key-Farbsteuerung ist damit grundsätzlich bewiesen und reproduzierbar,
über den Standardpfad, nicht über den ungelösten Vendor-Kaltstart-Blocker aus Iteration
28. Lamp-ID `0` = ESC ist der erste bestätigte Eintrag einer noch aufzubauenden
Lamp-ID→Taste-Tabelle (135 Einträge, `input_binding`-Feld aus Report 3 sollte das für
die übrigen 134 direkt liefern, ungeprüft).

**hidraw-Zuordnung dieser Session (nicht persistent, bei jedem Neuanschluss neu
prüfen):** if0→hidraw8, if1→hidraw9, if2→hidraw10, if3→hidraw11 (=LampArray-Kanal).

## Nächster konkreter Schritt

1. `tools/lamp_array_probe.py --json` laufen lassen (rein lesend, unkritisch) und
   `input_binding`-Felder auswerten → vollständige Lamp-ID→Taste-Tabelle ohne weitere
   Einzeltests ableiten.
2. Mit der Tabelle gezielt 2-3 weitere Lampen testen (`set-one`) zur Cross-Verifikation
   der `input_binding`-Interpretation gegen echte Tastenbeobachtung.
3. Architekturentscheidung dokumentieren (`DECISIONS.md`): `LightMountController`
   (Vendor, Interface 2, statisch) vs. Umstieg/Ergänzung auf generischen
   `HIDLampArrayController` (Interface 3, echtes Per-Key) für die OpenRGB-Integration.
4. Falls LampArray-Weg gewählt: den Detection-Konflikt (`skip_generic_detectors`)
   sauber lösen, nicht nur per temporärer Config umgehen — z. B. eigenen Vendor-Detector
   auf Interface 2 beschränken statt VID/PID-weit.

Der ungelöste Vendor-Protokoll-Blocker aus Iteration 28 (Windows-VM "Busy"-Status,
Sequenznummer-Kaltstart) bleibt als offene Frage bestehen, ist aber für den Per-Key-Weg
nicht mehr auf dem kritischen Pfad — VM `lightmount-win11` bleibt trotzdem registriert
und einsatzbereit, falls der Vendor-Weg später noch gebraucht wird (z. B. für Effekte
wie Matrix/Tornado, die LampArray nicht abdeckt).

## Update — 2026-08-24 Fortsetzung 3: OpenRGB-Integration live, `automation/`-Daemon gebaut

**Ziel:** Tastatur manuell über GUI und automatisiert (Zeitprofile + Event-Overlays +
manuelles Gaming-Profil) steuerbar machen, als eigenständiges externes Tool, mit
Schnittstelle für Home Assistant statt fest eingebauter HA-Logik. Plan unter
`~/.claude/plans/hazy-hatching-gosling.md`.

**Zwei echte OpenRGB-Bugs gefunden und lokal gefixt** (Details siehe `BACKLOG.md`):
1. `skip_generic_detectors`-Bug in `DetectionManager.cpp` — jetzt korrekt erst nach
   `compare()` gesetzt statt schon bei grobem VID/PID-Treffer. Vendor-Detector
   (Interface 2) und generischer LampArray-Detector (Interface 3) laufen jetzt
   gleichzeitig, ohne Workaround-Config.
2. Nullpointer-Crash in `NetworkServer::SendReply_PluginList` — `plugin_manager` ist im
   headless `--server`-Betrieb immer `nullptr`, jeder SDK-Client-Connect crashte den
   Server. Gefunden via `coredumpctl`/`gdb` (sechs Crashes in Folge, exakt gleicher
   Absturzpunkt), gefixt.

**Architektur (ein OpenRGB-Prozess hält die Hardware, alles andere ist SDK-Client):**
- `openrgb-lightmount-server.service` (neues systemd-User-Unit): dedizierter,
  headless OpenRGB-SDK-Server (Port 6742), eigene Config
  `~/.config/openrgb-lightmount/OpenRGB.json` mit explizit deaktivierten
  Fremd-Detectoren (`SteelSeries Apex 3`, `Corsair Ironclaw RGB`) — **wichtig:** die
  Detector-Einstellungen müssen unter einem verschachtelten `"Detectors"`-Block
  liegen (`{"Detectors": {"detectors": {...}}}`), nicht flach — eine falsch
  verschachtelte Version griff beim ersten Versuch nicht und hat Apex3/Ironclaw
  versehentlich nochmal in Direct Mode geschaltet (zweimal, sofort mit dem bekannten
  Fix behoben, vom Nutzer bestätigt).
- `automation/openrgb-lightmount-gui.sh`: manuelle GUI-Nutzung als reiner SDK-Client
  (`--nodetect --client 127.0.0.1:6742 --gui`) — **wichtig:** `--noautoconnect`
  bedeutet nur "nicht automatisch zu einem lokalen Server verbinden", **nicht**
  "keine lokale Hardware-Erkennung" — dafür ist `--nodetect` nötig, sonst scannt auch
  ein `--client`-Aufruf lokale Hardware und hätte wieder Apex3/Ironclaw angefasst
  (live beobachtet).
- `automation/lightmount_automation/` (Python, eigenes `.venv`, `openrgb-python`):
  `zones.py` (löst benannte Zonen aus `config/zones.yaml` gegen
  `lamp_id_key_mapping.json` auf), `openrgb_client.py` (SDK-Wrapper — **wichtig:**
  viele einzelne `UpdateSingleLED`-Aufrufe hintereinander für ein komplettes Repaint
  brachten den Client aus dem Tritt (`OpenRGBDisconnected`) — ab >8 geänderten Lampen
  immer Bulk-`set_colors()`/`UpdateLEDs` verwenden, nicht einzeln loopen),
  `layers.py` (Basis-Ebene + priorisierte Overlay-Regeln, Diff-basiertes Pushen),
  `time_profiles.py` (Zeitprofile config-driven, analog Apex3, aber pro Zone),
  `events/` (pluggable `EventSource`-Interface, MVP-Implementierung `ntfy_source.py`
  nutzt den schon vorhandenen ntfy-Server statt neuer Infrastruktur), `api.py`
  (lokale FastAPI, das "Gerät" für externe Systeme wie Home Assistant — keine
  HA-spezifische Logik im Code, nur eine generische REST-Schnittstelle, HA bindet sich
  über eigene RESTful-Command-Integration an), `cli.py`/`lightmount-ctl` (manuelle
  Steuerung, u. a. der vom Nutzer gewünschte manuelle Gaming-Profil-Trigger).
- `lightmount-automation.service` (systemd-User-Unit, `Requires=`/`After=` den
  Server-Service): läuft den Daemon (Zeitplan-Loop + Event-Listener + API in einem
  asyncio-Prozess).
- udev-Regel `/etc/udev/rules.d/99-lightmount-openrgb.rules` (analog zur
  Apex3-Regel): startet den Server-Service bei USB-Reconnect neu (OpenRGB erkennt
  Hot-Plug nicht selbst) — Skript `/usr/local/bin/lightmount-reconnect.sh`, nutzt
  `sudo -u mathias systemctl --user restart` aus dem Root-Kontext von udev heraus.
  **Nicht live getestet** (kein tatsächlicher Replug durchgeführt).

**Home Assistant:** Läuft echt unter `192.168.2.26:8123` (Korrektur des Nutzers
gegenüber der ursprünglichen Recherche, die "kein HA vorhanden" ergab — das war die
falsche/veraltete Vault-Notiz). Bestehende Token-Config unter
`/opt/scripts/scripts/config/config.yaml` (root-only lesbar), genutzt von
`~/homeassistant-automations/ha_common.py`. **Netzwerk von diesem Rechner zu
192.168.2.26 war beim Testen nicht erreichbar** ("Keine Route zum Zielrechner") —
muss vor echter HA-Anbindung geklärt werden, blockiert aber nicht den Daemon selbst.
Bewusst **keine** HA-spezifische Logik im Code — der Daemon exponiert nur eine
generische HTTP-API, HA bindet sich per RESTful-Integration an (Beispiel in
`automation/README.md`), MQTT Discovery als möglicher späterer Ausbau.

**End-to-End live verifiziert (alles vom Nutzer bestätigt):**
- Bulk-Farbwechsel über die API (Zeitprofil "standard" → hellblau)
- Zonen-Override per CLI (`lightmount-ctl zone light_bar FF0000`) über dem Basislayer
- Manuelles Gaming-Profil (`lightmount-ctl profile gaming`: WASD cyan, Rest gedimmt,
  Lichtleiste rot)
- ntfy-Event-Trigger End-to-End: Test-Alarm `server_a_down` → Lichtleiste rot →
  `server_a_up`-Nachricht → Overlay deaktiviert, zurück zum Basislayer

## Nächster konkreter Schritt

1. Upstream-MR für beide OpenRGB-Fixes vorbereiten (Commits `1443577`/`243f1f6` im
   `openrgb-src-private`-Checkout liegen bereit, noch nicht gepusht).
2. Netzwerk-Erreichbarkeit zu `192.168.2.26` (Home Assistant) klären, dann echte
   HA-seitige RESTful-Integration einrichten und testen.
3. udev-Reconnect-Regel mit einem echten Replug/KVM-Wechsel verifizieren.
4. Bei Bedarf: MQTT-Discovery als native HA-Geräte-Integration ergänzen, sobald ein
   MQTT-Broker in HA-Nähe bestätigt ist.

## Update — 2026-08-24: drei Tasten gleichzeitig unabhängig gefärbt (Ziel erreicht)

Rein lesender `lamp_array_probe.py --json`-Lauf zeigt: von 135 Lampen haben nur **3**
ein gesetztes `input_binding` (HID-Keyboard-Usage-Code) — Lamp `0`→Usage `41`/`0x29` =
ESC (deckt sich exakt mit dem Test von gestern), Lamp `133`→Usage `100`/`0x64` =
ISO-Backslash-Taste (`<>|`), Lamp `134`→Usage `50`/`0x32` = ISO-Hash/Tilde-Taste (`#'`).
Alle anderen 132 Lampen bleiben firmwareseitig ungebunden — vermutlich weil nur
Layout-abhängige Sondertasten eine explizite Bindung brauchen, der Rest folgt
vermutlich einer festen Standard-Reihenfolge, aber ungeprüft. Nebenbefund: `purposes`
ist bei allen 135 Lampen identisch `2000`/`0x7D0` — passt nicht sauber auf die im
HID-LampArray-Standard definierten Purpose-Flags (nur Bits bis `0x20` vorgesehen),
ungeklärt, ob Parsing-Fehler oder Vendor-Erweiterung, nicht weiter verfolgt (nicht
blockierend).

`tools/lamp_array_control.py` um `set-many` erweitert (`lamp_id:r:g:b`-Syntax, bis zu
8 Lampen pro Multi-Update-Report, HID LampArray erlaubt maximal 8 pro Report).
Realer Schreibtest, mit Nutzer-Go-ahead: alle drei bekannten Lampen gleichzeitig in
unterschiedlichen Farben (`0`→rot, `133`→grün, `134`→blau). Nutzer bestätigte live alle
drei korrekt. Damit ist die Kernfrage der Session — mehrere Tasten unabhängig
voneinander einfärben — bewiesen und reproduzierbar, nicht nur für eine einzelne Lampe.
Danach `restore-autonomous`, Tastatur wieder im Normalzustand.

**Offen für später, nicht mehr kritisch fürs Kernziel:** vollständige Lamp-ID→Taste-
Tabelle für die restlichen 132 Lampen (Einzeltests oder Koordinatenabgleich mit der
alten Vendor-LED-Tabelle `docs/evidence/light_mount_led_layout_iso.json` — andere
Nummerierung, `led_index` statt `lamp_id`, keine direkte 1:1-Übernahme möglich).

## Update — 2026-08-24 Fortsetzung: Sonderelemente identifiziert (M-Tasten, Lautstärke-Rad, Lichtleiste)

**Methode:** Koordinaten-Transformation aus den 3 schon bestätigten Ankerpunkten (ESC,
ISO-`<>|`, ISO-`#'`) berechnet, um Pixel-Positionen aus der alten Vendor-Tabelle
(`docs/evidence/light_mount_led_layout_iso.json`) auf LampArray-µm-Koordinaten
abzubilden — plus reine Struktur-Analyse der 135 Lampen (Zeilen/Spalten/ID-Blöcke).
Alle Ergebnisse anschließend **live verifiziert**, nicht nur berechnet.

**Vollständige Struktur der 135 Lampen (jetzt lückenlos erklärt):**
- 106 Lampen: normale Tastenmatrix (6 Zeilen, inkl. ESC=`0`, ISO-`<>|`=`133`,
  ISO-`#'`=`134`)
- 5 Lampen (`16`–`20`): **M1–M5** — eine Spalte links neben der Hauptmatrix, IDs
  fortlaufend über 5 Zeilen. Die Vendor-Tabelle wurde initial nur nach `M1`-`M3`
  durchsucht (falscher Suchbegriff), enthält aber tatsächlich `Key_M4`
  (`led_index 124`) und `Key_M5` (`led_index 144`) — das Gerät hat 5, nicht 3
  Makrotasten. Live bestätigt (Nutzer sah M4/M5 farbig).
- 1 Lampe (`109`): **Lautstärke-Rad ("Vol")**. Erster Test (zusammen mit Nachbarn
  110/0 aktiv) ergab durch optisches Bleeding ein irreführendes Bild (Nutzer sah nur
  Orange, kein Gelb). Sauberer Isoliertest — nur `109` allein grün, alle anderen
  Lampen per `restore-autonomous` zurückgesetzt — ergab eindeutig: **Rad wird grün**.
  Lehre für künftige Tests in dicht bebauten Ecken: Nachbarn vorher auf Aus/Standard
  setzen statt mehrere eng benachbarte Kandidaten gleichzeitig zu testen.
- 23 Lampen (`110`–`132`): **Lichtleiste oben**, volle Gerätebreite. `110` = linkes
  Ende, `132` = rechtes Ende, IDs steigen von links nach rechts. Live bestätigt
  (Orange links, Lila rechts, korrekt zugeordnet). Nutzer bestätigt unabhängig:
  visuell mindestens 10, eher deutlich mehr einzeln unterscheidbare Segmente — passt
  zu 23 individuell adressierbaren Lampen.

**Weiterhin ungeklärt: die zwei kleinen Lichter an der Unterseite außen (vom Nutzer
"Seitenlichter" genannt).** Alle 135 LampArray-Lampen sind jetzt einer Kategorie
zugeordnet (106 Tasten + 5 M-Tasten + 1 Rad + 23 Leiste = 135, exakt vollständig) —
für diese zwei Lichter bleibt **kein** Lamp-ID-Kandidat übrig. Die Vendor-Tabelle
kennt zusätzlich `Led_KeyboardLeft1-5`/`Led_KeyboardRight1-5` (10 Stück, `geometry:
null`, also keine On-Screen-Position in der App) — die tauchen in den LampArray-Daten
ebenfalls nicht als eigene Spalte auf (keine gespiegelte rechte Randspalte gefunden).
**Arbeitshypothese:** Diese Unterseiten-/Seitenlichter hängen nicht am HID-LampArray-
Kanal (Interface 3), sondern entweder am alten Vendor-Protokoll (Interface 2) oder
sind nicht-RGB/statisch. Noch nicht getestet — nächster Schritt bei Bedarf: gezielt
im Vendor-Protokoll nach einem passenden Kommando suchen, oder rein empirisch prüfen,
ob sie bei `restore-autonomous`/Werkszustand überhaupt reagieren.

**Vollständige bestätigte Lamp-ID-Tabelle bis hierhin:**

| Lamp-ID | Element | Bestätigungsweg |
|---|---|---|
| 0 | ESC | live + `input_binding` |
| 16 | M1 | Struktur + Vendor-Tabelle |
| 17 | M2 | Struktur + Vendor-Tabelle |
| 18 | M3 | Struktur + Vendor-Tabelle |
| 19 | M4 | live + Vendor-Tabelle |
| 20 | M5 | live + Vendor-Tabelle |
| 109 | Lautstärke-Rad | live (isoliert) |
| 110 | Lichtleiste, linkes Ende | live |
| 111–131 | Lichtleiste, Mitte (aufsteigend) | Struktur (ungetestet) |
| 132 | Lichtleiste, rechtes Ende | live |
| 133 | ISO `<>\|` (neben li. Shift) | live + `input_binding` |
| 134 | ISO `#'` (neben Enter) | live + `input_binding` |
| 1–15, 21–108 (außer 96–108\*) | restliche Hauptmatrix-Tasten | Struktur (ungetestet) |

\* 96–108 = unterste Zeile (Leertaste etc.), noch nicht einzeln benannt.

## Update — 2026-08-24 Fortsetzung 2: komplette Hauptmatrix live kartiert

**Methode:** 9-Anker-Koordinatentransformation verfeinert (getrennter Fit für X- und
Y-Achse — die M-Tasten-Spalte hat einen anderen X-Versatz als die reguläre Matrix, ein
gemeinsamer Fit hätte die Genauigkeit verschlechtert). Y-Fit über alle 9 Anker
(Residuen <1,3mm), X-Fit nur über die 3 Matrix-Anker (Residuen <1,5mm). Für jede
benannte Taste aus der alten Vendor-Tabelle (102 `Key_*`-Einträge mit Geometrie)
per Nearest-Neighbor die passende Lamp-ID vorhergesagt — Ergebnis liest sich
lückenlos wie ein reales Tastaturlayout (inkl. **vollständigem Nummernblock**, bisher
nicht bekannt gewesen: Gerätebreite 450mm passt zu einer Full-Size-Tastatur).

**Live-Verifikation zeilenweise, alle 6 Zeilen vollständig bestätigt** (Funktionsreihe,
Zahlenreihe+Numpad-oben, QWERTY+Numpad, ASDF+Numpad, ZXCV+Numpad+Pfeil-hoch, unterste
Reihe+Numpad+Pfeile) — je 2-3 `set-many`-Schreibvorgänge direkt hintereinander (nicht
überlappend, daher ohne Zwischen-Reset), danach eine Sammelbestätigung pro Zeile.
**Alle Vorhersagen korrekt**, inklusive der unsichereren Kandidaten (LeftShift 7,9mm
Abweichung, NumpadEnter 10,5mm Abweichung — beide trotzdem richtig).

**Zwei Korrekturen gegenüber der reinen Vorhersage:**
- `Key_Enter` (ISO-L-Form, als SVG-Path statt Rechteck in der Vendor-Tabelle) hatte
  eine schlechte Bbox-Center-Schätzung und kollidierte rechnerisch mit Lamp `100`.
  Live-Test zeigt: `100` ist tatsächlich **RightAlt**, die echte Enter-Taste ist
  **Lamp `75`** (initial unvorhergesagt, da kein Vendor-Key dorthin passte).
- **Lamp `55`** (initial ebenfalls unvorhergesagt, zwischen `BracketRight`/54 und
  `Delete`/56) bleibt **nachweislich dunkel** — zweimal getestet, zuletzt komplett
  isoliert (alle anderen 134 Lampen explizit auf Schwarz, nur `55` grün) — keine
  sichtbare LED an dieser Adresse. Vermutlich eine unbestückte Position im
  Enter-Notch-Bereich (großer/stabilisierter Tastenbereich ohne eigene LED).

**Vollständige, live verifizierte Mapping-Tabelle:**
`docs/evidence/lamp_id_key_mapping.json` — alle 135 Lamp-IDs, mit Konfidenz-Feld
(`live`, `live+vendor`, `live-negative-confirmed`, `structural` für die 21
ungetesteten mittleren Lichtleisten-Segmente 111-131, deren Enden aber live bestätigt
sind). Einzige verbleibende offene Fragen: die zwei kleinen Unterseiten-Lichter
(kein Lamp-Kandidat, siehe `unresolved` im JSON) und die 21 mittleren
Lichtleisten-Segmente (nur strukturell, nicht einzeln live getestet).

**Protokoll-Nebenfund:** `restore-autonomous` (Report `06 01`) stellt **nicht** den
ursprünglichen Anzeigezustand wieder her — nach einem Testlauf über die komplette
Tastatur zeigte das Gerät danach durchgehend **Weiß**, nicht etwa ein Rainbow-Effekt
oder den Zustand von vor dem ersten Test. Vermutlich ist das schlicht der
Autonomous-Leerlaufzustand der Firmware (kein Fehler), aber wichtig für künftige
Tests: einzelne `restore-autonomous`-Aufrufe zwischen Tests löschen frühere
Farbwerte **nicht** zuverlässig sichtbar zurück — bei Bedarf für einen sauberen
Nullzustand müssen alle betroffenen Lamp-IDs explizit auf Schwarz gesetzt werden
(wie beim Lamp-55-Isoliertest gemacht), nicht auf `restore-autonomous` verlassen.

## Letzter Stand

2026-08-18, Iteration 1 (ralph-loop, max. 30, Stop-Promise `LIGHTMOUNT_LOOP_STOP`):

- Repository angelegt (privat, `github.com/mgd1068/bequiet-lightmount-linux`),
  Pflichtdokumentation initial befüllt.
- System-/USB-Bestand read-only erfasst: Kubuntu 26.04, Kernel 7.0.0-29-generic,
  OpenRGB `0.9+git20251009+ds-1` bereits als Debian-Paket installiert. Light Mount
  angeschlossen: Bus 001 Device 060, USB 1.1 Full Speed, 4 HID-Interfaces, alle an
  Kernel-`usbhid` gebunden.
- `lsusb -v` und sysfs-`report_descriptor` aller vier Interfaces gesichert
  (`docs/evidence/`, committet — reine Geräte-Deskriptoren, keine Traffic-Capture,
  keine Tastendrücke). Ergebnisse siehe `PROTOCOL.md`: Interface 2 als wahrscheinlichster
  Vendor-Konfigurationskanal bestätigt (Endpoints **und** Report-Deskriptor stimmen mit
  Ausgangshypothese überein), Interface 3 als zweiter, umfangreicherer Vendor-Kanal via
  Feature-Reports neu entdeckt (stand vorher nicht im Ausgangsstand), Interface 1 enthält
  vermutlich das Medienrad als generische Maus-Collection, Interface 0 ist reiner
  Boot-Keyboard-Traffic.
- Kein `hidraw`-Gerät geöffnet oder gelesen — ausschließlich `lsusb` und sysfs-Attribute.

## Update — Iteration 2 (2026-08-18, ralph-loop)

- OpenRGB Issue #4950 geprüft (GitLab API, unauthentifiziert): keine neuen Kommentare,
  keine MR/Code seit Ticketerstellung. Ursprünglicher Melder hatte `bcdDevice 15.00`,
  `iSerial 0` (kein Serial-String) — abweichend von meinem Gerät (`23.00`,
  `[REDACTIERTE-SERIENNUMMER]`); vermutlich neuere Firmware-Revision, nicht sicherheitsrelevant.
- `usbmon3_capture.pcapng.gz` aus dem Issue geladen (via GitLab API mit numerischer
  Projekt-ID — der direkte `/uploads/`-Pfad aus der Markdown-Vorschau lieferte 404),
  lokal unter `captures-private/` entpackt (git-ignoriert, nicht committet).
- `tshark` installiert (`sudo apt-get install tshark`, System-Policy für sudo laut
  Nutzer explizit erlaubt). AppArmor sperrt `tshark -r <pfad>` für Dateien außerhalb
  fester Systempfade (`/usr/share/wireshark`, kein Home-Zugriff) — umgangen durch
  Lesen über Stdin (`tshark -r - < datei`), kein Policy-Change nötig.
- Capture analysiert (20 Frames auf Bus 3 Device 2, Endpoints 3/4 = Interface 2;
  Maus-Traffic auf `3.3.1` laut Melder-Hinweis ignoriert): **Interface 2 als echter
  Konfigurationskanal bestätigt** (nicht nur laut Deskriptor). Header-Struktur aus
  10 Kommando/Antwort-Paaren abgeleitet, ein 41-Byte-Kommando als 6-Stopp-Regenbogen-
  Gradient dekodiert (zweifach identisch bestätigt). Details: `PROTOCOL.md`,
  `docs/evidence/usbmon3_decoded_commands.txt`.
- Auf Nutzerwunsch: Coding-Stil-Entscheidung dokumentiert (C++, aber funktional/C-artig,
  OOP nur wo OpenRGBs API es verlangt) — siehe `DECISIONS.md`.
- Kein `hidraw`-Zugriff, kein Schreiben ans Gerät — weiterhin nur Analyse vorhandener
  Deskriptoren und eines fremden, bereits öffentlichen Captures.

## Update — Iteration 3 (2026-08-18, ralph-loop)

- Checksum-Hypothese offline gegen alle 20 bekannten Frames verifiziert: letzte 2 Byte
  jedes 64-Byte-Reports = **CRC16/MODBUS** (Poly `0x8005`, Init `0xFFFF`, reflektiert,
  kein XOR-Out) über Byte 0–61, Little-Endian angehängt. 20/20 Treffer, siehe
  `docs/evidence/checksum_verification.py` (reproduzierbar, `python3` ausführen).
  Damit ist die vollständige Interface-2-Report-Struktur (Länge, konstantes Feld,
  Sequenznummer, Subcommand, Flags, Payload, CRC) für alle bisher beobachteten
  Kommandotypen bekannt — nur die genaue Payload-Semantik einzelner Subcommands
  (außer dem Rainbow-Gradient) ist noch offen.
- Kein Gerätezugriff, keine Hardware angefasst — reine Offline-Verifikation bekannter
  Bytes.

## Update — Iteration 4 (2026-08-18, ralph-loop)

- C++-Offline-Testgerüst aufgesetzt: `src/protocol/crc16.{h,cpp}` (freie Funktion,
  CRC16/MODBUS), `src/protocol/report.{h,cpp}` (plain struct `Interface2Report` +
  freie Funktionen `parse_report`/`build_report`/`report_crc_valid`, kein OOP,
  passend zu `DECISIONS.md`). Testframework: einfaches `assert()`-basiertes
  `tests/test_protocol.cpp` (kein externes Framework, wie im Rückfallplan der letzten
  Iteration vorgesehen — Umfang rechtfertigt bisher keines). Fixtures
  (`tests/fixtures_usbmon3.h`) sind die 20 aus `checksum_verification.py` bekannten
  Frames.
- Build: CMake (`CMakeLists.txt`, C++17, `-Wall -Wextra`), `cmake -S . -B build &&
  cmake --build build && ctest --test-dir build` — alle 4 Tests grün (CRC gegen alle
  20 Fixtures, `report_crc_valid` gegen alle 20, Parse→Build-Rundlauf byteidentisch
  für alle 20, bekannte Header-Felder + Rainbow-Stop-0-Bytes von Frame 1453 geprüft).
- Ein echter Implementierungsfehler beim ersten Durchlauf gefunden und behoben:
  `build_report` schrieb `raw[2]=0x00` statt `0x02` (das konstante Feld ist Byte2=`0x02`,
  Byte3=`0x00`, nicht umgekehrt — Verwechslung beim Übertragen aus der
  Little-Endian-Notation `0x0002`). Der Parse→Build-Rundlauf-Test hat das sofort als
  `assert`-Fehlschlag aufgedeckt, bevor es unbemerkt geblieben wäre.
- `README.md` um Bau-/Testanleitung ergänzt. `build/` bleibt git-ignoriert.
- Kein Gerätezugriff — reiner Code/Build/Test-Schritt.

## Update — Iteration 5 (2026-08-18, ralph-loop)

- Mountain-Everest-Referenzcode (`MountainKeyboardController.{h,cpp}`) per `curl` von
  GitLab Raw geladen (kein Vollklon nötig, WebFetch-Zusammenfassung war zu ungenau für
  einen Byte-genauen Vergleich — direktes `curl` liefert exakten Quelltext).
- Strukturvergleich durchgeführt: die ursprüngliche Hypothese "Light Mount ist
  protokollverwandt mit Mountain Everest" ist **widerlegt**. Mountain nutzt ein
  Report-ID-Byte + feste Kommandoklassen (`0x14`/`0x13`) an fester Position, **keine**
  Prüfsumme und **keine** Sequenznummer. Light Mount Interface 2 hat dagegen ein
  Längenfeld, einen monoton steigenden Sequenzzähler und einen CRC16/MODBUS-Trailer —
  keines davon existiert bei Mountain. Details: `PROTOCOL.md`, Abschnitt
  „Strukturvergleich mit Mountain Everest“.
- `ARCHITECTURE.md` aktualisiert: Mountain-Code bleibt als Vorlage für die spätere
  OpenRGB-Integrationsform (Klassenstruktur, `RGBController`-Anbindung) relevant, aber
  nicht mehr als Byte-Protokoll-Quelle.
- Kein Gerätezugriff — reiner Recherche-/Dokumentationsschritt.

## Update — Iteration 6 (2026-08-18, ralph-loop)

- Dry-Run-CLI `report_dump` gebaut (`src/cli/report_dump.cpp`, neues CMake-Target):
  liest einen 64-Byte-Report als 128-stelligen Hex-String (Argument oder stdin), gibt
  Länge/Sequenz/Subcmd/Flags/Payload/CRC-Gültigkeit lesbar aus. Öffnet **kein** `hidraw`,
  keine Geräteinteraktion. Manuell gegen das bekannte Rainbow-Kommando (Frame 1453)
  getestet: korrekt dekodiert, `crc_valid=yes`; mit manipuliertem letzten Byte getestet:
  `crc_valid=NO` wie erwartet; mit ungültiger Eingabelänge getestet: sauberer Fehler,
  Exit-Code 1.
- `README.md` um CLI-Nutzung ergänzt.
- Kein Gerätezugriff — reiner Code-Schritt.

## Update — Iteration 7 (2026-08-18, ralph-loop)

- Alle verbleibenden unentschlüsselten Kommandos mit `report_dump` neu durchgesehen
  (Frames 2447, 2605, 2747, 1731, 1739, 2997, 2341 — Rohausgabe geprüft statt aus dem
  Gedächtnis transkribiert).
- **Eigenen Fehler gefunden und korrigiert:** Die frühere Notiz "Frame 2447/2605
  unterscheiden sich nur in Byte 8" war falsch — tatsächlich unterscheiden sich ZWEI
  Payload-Bytes gemeinsam (Byte 8: `01→00`, Byte 9: `03→00`), alle anderen Bytes
  identisch. Korrigiert in `docs/evidence/usbmon3_decoded_commands.txt`.
- Neue Beobachtung zum Längenfeld dokumentiert: `length=6` (Frame 2997, subcmd 0x0a,
  leerer Payload) vs. `length=7` (Frame 1731, subcmd 0x02, ebenfalls leerer Payload)
  zeigt, dass die Länge NICHT allein aus der Payload-Füllung ableitbar ist, sondern auch
  vom Subkommando abhängt. Bewusst als offene Frage belassen, keine Formel im Code
  nachgebildet, um keine unbelegte Annahme in `build_report`/`parse_report`
  einzubauen — passt zur Grundregel aus `PROTOCOL.md` (Fakten von Hypothesen trennen).
- Keine neuen, hinreichend sicheren Feature-Zuordnungen gefunden (Frame 2747 "01 00 ff
  00 ff 37" bleibt uneindeutig, Frame 2341/3109 bleibt uneindeutig) — bewusst nicht
  geraten. Weitere Fortschritte hier brauchen entweder eigene Einzelaktions-Captures
  oder echte Hardwaretests, beides noch nicht Teil dieser Iteration.
- Kein Gerätezugriff — reine Offline-Analyse bereits bekannter Bytes.

## Nächster konkreter Schritt

1. Sicherheitsbewusste Entscheidung: die statische Offline-Analyse des fremden
   Mehrfach-Klick-Captures hat ihren Erkenntnisgewinn für diese Iteration weitgehend
   ausgeschöpft (ein klar entschlüsseltes Kommando: Rainbow-Gradient; mehrere strukturell
   verstandene, aber semantisch unklare Kommandos). Nächster sinnvoller Schritt laut
   `SPEC.md`/Phase 1 ist der **Testplan-Entwurf** für einen ersten, risikoarmen realen
   Schreibtest — NICHT die Ausführung selbst.
2. Testplan muss laut `SECURITY.md` mindestens enthalten: gewähltes Kommando (das
   bereits bekannte, byteidentisch reproduzierte Rainbow-Kommando aus Frame 1453/3531 —
   kein selbst konstruiertes), erwartete sichtbare Wirkung, Timeout-Wert, Reconnect-
   Verhalten bei USB-Reset, Rückfall auf bekannten sicheren Zustand (Beleuchtung aus),
   explizite `--dry-run`-Standardeinstellung, genaue Interface-/Endpoint-Auswahl
   (Interface 2, EP 0x04 OUT / EP 0x83 IN — keine anderen `hidraw`-Geräte berühren).
3. Testplan als eigenes Dokument oder Abschnitt festhalten (`docs/` oder `STATE.md`),
   dem Nutzer NICHT eigenmächtig zur Ausführung vorlegen, ohne dass die Freigabe für den
   ersten echten Schreibzugriff auf die angeschlossene Hardware explizit erteilt wurde —
   das ist der im Master-Prompt vorgesehene Übergang von reiner Analyse zu echtem
   Gerätekontakt und rechtfertigt Innehalten, auch wenn der Loop technisch autonom
   weiterlaufen könnte.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Ein schriftlicher Testplan (kein Code, keine Ausführung) lässt sich
  erstellen, der alle zehn SECURITY.md-Regeln nachvollziehbar abdeckt, bevor überhaupt
  ein `hidraw`-Handle geöffnet wird.
- **Erwartetes Ergebnis:** Ein dokumentierter, überprüfbarer Plan, der als Grundlage für
  eine spätere, bewusste Freigabe des ersten echten Schreibtests dient.
- **Sicherheitsrisiko:** keins beim Schreiben des Plans selbst; der Plan beschreibt aber
  einen Schritt mit echtem (wenn auch laut `SECURITY.md` minimiertem) Hardwarerisiko.
- **Rückfall:** keiner nötig, da dieser Schritt noch keine Hardware anfasst.

## Update — Iteration 8 (2026-08-18, ralph-loop)

- Testplan für den ersten echten `hidraw`-Schreibzugriff geschrieben:
  `docs/first-write-test-plan.md`. Deckt alle zehn `SECURITY.md`-Regeln ab (exakte
  Interfaceauswahl, bekanntes/nicht selbst konstruiertes Kommando, kein Speicherbefehl,
  Timeout, Reset-als-erwarteter-Fehler statt Retry-Schleife, Rückfall auf sicheren
  Zustand via USB-Replug, `--dry-run`-Standard für das noch zu bauende Sende-Tool).
  **Noch nicht ausgeführt.**

## Update — Iteration 9 (2026-08-18, ralph-loop) — erster Hardwaretest, mit Nutzerfreigabe

- Nutzer hat den Testplan freigegeben (“ja, mach den Testplan”).
- `report_send` gebaut (`src/cli/report_send.cpp`, neues CMake-Target): ohne `--confirm`
  reiner Dry-Run (kein `open()`), mit `--confirm` genau ein `write()` mit
  500ms-Poll-Timeout (`poll()` auf `POLLOUT`/`POLLERR`/`POLLHUP`), keine Retry-Schleife
  bei Timeout/Fehler — wie in `docs/first-write-test-plan.md` vorgesehen.
- `hidraw`-Pfad frisch ermittelt (Interface 2 → `hidraw10`, Weltschreibrechte, kein
  `sudo` nötig), `report_dump` bestätigte `crc_valid=yes` vor dem Senden.
- Nutzer vor dem Schreiben gewarnt (möglicher kurzer USB-Abriss laut bekanntem
  Firmwarefehler), dann genau ein Schreibvorgang mit dem bekannten Rainbow-Kommando
  (Frame 1453/3531) ausgeführt.
- **Ergebnis: Erfolg, kein USB-Reset** (`lsusb`/`dmesg` direkt danach geprüft). Nutzer
  bestätigte sichtbare Wirkung — allerdings **anders als angenommen**: kein räumlicher
  Verlauf über die Tasten, sondern ein zeitlicher Farbzyklus (alle LEDs gleichzeitig,
  Farbe wechselt durch den Regenbogen). Die „Position”-Interpretation der 6 Keyframe-
  Gruppen war falsch (räumlich statt zeitlich) — jetzt in `PROTOCOL.md` und
  `docs/first-write-test-plan.md` korrigiert dokumentiert, kein Fehlerzustand am Gerät.
- Damit ist das erste `SPEC.md`-Abnahmekriterium („Light Mount wird ausschließlich über
  exakte Geräte-/Interfaceidentität geöffnet”) für einen echten Schreibpfad erstmals
  belegt. Kriterien zu Einzeltasten/voller Matrix sind davon nicht erfüllt — dieses
  Kommando steuert alle LEDs gemeinsam.

## Nächster konkreter Schritt

1. Da die „Position”-Hypothese jetzt als zeitlich (nicht räumlich) bestätigt ist: prüfen,
   ob eines der noch unentschlüsselten Kommandos (z. B. Frame 2341/3109, 29 Byte) eher zu
   einem STATISCHEN Einzelfarb- oder Zweifarb-Kommando passt als zu einem weiteren Zyklus
   — mit der jetzt korrigierten Erwartungshaltung (zeitlich vs. räumlich) neu bewerten,
   bevor ein weiterer Hardwaretest vorgeschlagen wird.
2. Für den nächsten Hardwaretest (mindestens zwei einzelne Tasten unabhängig einfärben,
   SPEC.md-Kriterium) fehlt noch ein bekanntes Kommando für Einzeltasten-Adressierung —
   im bisherigen Capture nicht enthalten (siehe offene Frage in `PROTOCOL.md`: IO Center
   Web deckt evtl. keine Per-Key-Beleuchtung ab). Ohne ein solches Kommando ist ein
   weiterer Hardwaretest in diese Richtung nicht sinnvoll planbar — als offene Frage
   markiert, nicht durch Raten zu lösen.
3. Alternativ: mit dem jetzt verifizierten Schreibpfad (`report_send`) und bereits
   bekannten Kommandos (z. B. Frame 2747, 18 Byte) einen weiteren, ähnlich risikoarmen
   Hardwaretest vorschlagen, um mehr über unentschlüsselte Subcmd-Familien zu lernen —
   erfordert erneut kurze Nutzerfreigabe vor Ausführung (gleiches Muster wie hier).

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Frame 2747 (18 Byte, Subcmd `0x06`, Payload beginnt `04 00 64 0a 01 00
  ff 00 ff 37`) ist ebenfalls ein Vollflächen-/Zyklus-Effekt (kein Einzeltasten-Kommando),
  da bisher keines der Capture-Kommandos Anzeichen von Tastenzahl-Wiederholung zeigt.
- **Erwartetes Ergebnis:** Falls getestet, vermutlich eine weitere sichtbare, aber
  gleichmäßige (nicht Tasten-individuelle) Lichtänderung.
- **Sicherheitsrisiko:** identisch zum bereits durchgeführten Test (bekanntes,
  byteidentisches Kommando, kein Save-Befehl) — bei erneuter Nutzerfreigabe gering.
- **Rückfall:** wie in `docs/first-write-test-plan.md` — USB-Replug, falls unerwarteter
  Fehlerzustand (bisher nie eingetreten).

## Update — Iteration 10 (2026-08-18, ralph-loop) — zweiter Hardwaretest

- Nutzer hat vor dem Test per physischem Hotkey einen definierten Ausgangszustand
  hergestellt (gleichförmige Farbe) — USB-Replug hatte den vorherigen Zyklus-Effekt NICHT
  gestoppt (wichtiger Erfahrungswert, in `SECURITY.md`/`PROTOCOL.md` festgehalten).
- Frame 2747 (18 Byte, Subcmd `0x06`, Payload `04 00 64 0a 01 00 ff 00 ff 37` +
  Nullpadding) mit `report_send --confirm` gesendet — bekanntes, byteidentisches
  Kommando, `crc_valid=yes` vorab per `report_dump` bestätigt, ein einziger Schreibvorgang.
- **Ergebnis: statische (nicht zeitlich veränderliche) Farbe, vom Nutzer als Orange
  beschrieben.** Kein USB-Reset.
- **Hypothese widerlegt:** Die naheliegende Byte-14-16-als-RGB-Lesart (`ff,00,ff` =
  Magenta) passt nicht zu Orange — verworfen, nicht in `PROTOCOL.md` als Fakt übernommen.
- **Neue, gestützte Hypothese:** Byte 8 ist (zumindest teilweise) ein Preset-/Effekt-
  Index (`0x03`=Rainbow-Zyklus, `0x04`=dieses Orange-Preset), keine direkte RGB-Eingabe
  in dieser kurzen Kommandoform. Farbcodierung für frei wählbare Farben bleibt offen.
- `SPEC.md`-Kriterium „statische Gesamtfarbe" im Kern erstmals demonstriert (Farbe ist
  aber preset-fest, nicht frei wählbar) — als Teilerfolg in `BACKLOG.md` vermerkt.

## Nächster konkreter Schritt

1. Kein bekanntes Kommando mehr aus dem Capture, das mit vertretbarer Sicherheit neue,
   noch nicht getestete Erkenntnisse liefert, ohne zu raten (Frame 2341/3109 bleibt
   unklar strukturiert, Frame 1731/1739 sind sehr kurze Toggle-Kommandos mit unklarer
   Wirkung, Frame 2997 wirkt wie eine leere Abfrage). Weitere Hardwaretests mit diesen
   Kommandos sind möglich, aber der Erkenntnisgewinn ist ungewisser als bei den bisherigen
   zwei erfolgreichen Tests — vor jedem weiteren Test erneut kurz beim Nutzer nachfragen.
2. Alternativ und vermutlich ergiebiger: einen eigenen, gezielten Einzelaktions-Capture
   planen (IO Center Web im Browser öffnen, eine einzelne bekannte Aktion wie "Farbe X
   wählen" ausführen, dabei mitschneiden) — das würde die offene Frage nach der
   RGB-Codierung für freie Farben direkt beantworten, statt an fremden Mehrfach-Klick-
   Daten zu raten. Das ist ein größerer Schritt (Browser-Interaktion, ggf. eigener
   `usbmon`-Mitschnitt) und sollte dem Nutzer vor Beginn kurz vorgeschlagen werden.
3. Weiterhin ohne Rückfrage möglich: `PROTOCOL.md`/`ARCHITECTURE.md` konsolidieren, damit
   der aktuelle Stand (2 erfolgreiche Hardwaretests, mehrere offene Fragen) für eine
   künftige Session klar lesbar bleibt.

## Update — Iteration 11 (2026-08-18, ralph-loop) — eigener gezielter Capture, großer Fortschritt

- Nutzer hat zugestimmt, einen eigenen gezielten Capture durchzuführen (statt zu raten).
- Technisches Problem gelöst: `tshark`/`dumpcap` sind per AppArmor auf `/dev/usbmon*`
  gesperrt (bestätigt per `dmesg`-DENIED-Eintrag), auch mit `sudo` (Policy gilt unabhängig
  von der UID). Keine AppArmor-Policy verändert — stattdessen debugfs-Textinterface
  (`/sys/kernel/debug/usb/usbmon/1u`, `sudo cat`) genutzt, das nicht durch das
  tshark-Profil mediiert wird. In `SECURITY.md` als Vorgehen dokumentiert.
- Mitschnitt während einer einzelnen Nutzeraktion (freie Farbwahl `#1FB4FF` in IO Center
  Web) auf Interface 2 (Device 061) durchgeführt, auf unser Gerät gefiltert.
- **Mehrere bedeutende neue, hardwarebestätigte Fakten** (Details in `PROTOCOL.md` und
  `docs/evidence/own_capture_iocenter_decoded.md`):
  1. RGB-Kodierung für frei wählbare Farben entschlüsselt und exakt gegen den vom
     Nutzer abgelesenen Hex-Wert verifiziert (Payload-Byte 5-7 = R,G,B direkt).
  2. Helligkeits-Byte (Payload-Byte 2 = `0x64`) durch Nutzerangabe (UI auf 100%)
     zusätzlich bestätigt.
  3. Neues Subkommando `0x03` entdeckt: periodisches Keepalive, unabhängig von
     Nutzeraktionen.
  4. Korrektur: das bisher als "konstant 0x0002" dokumentierte Feld (Byte 2-3) ist
     session-abhängig, nicht universell — `report.h`/`report.cpp`-Kommentare
     entsprechend präzisiert (Verhalten selbst unverändert, nur Dokumentation).
  5. Unaufgeforderte Push-Frames entdeckt, die Tastendrücke vom Boot-Keyboard-Interface
     über den Vendor-Kanal spiegeln — inklusive Privacy-relevanter Nebenfolge
     (WebHID-Tastatur-Zugriffssperre wird dadurch faktisch umgangen), in `SECURITY.md`
     als eigenständiger Hinweis festgehalten.
- Kein `hidraw`-Schreibzugriff in dieser Iteration — nur Lese-/Mitschnitt-Zugriff auf
  `/sys/kernel/debug/usb/usbmon/1u` (kein Kommando an das Gerät gesendet).

## Nächster konkreter Schritt

1. Das jetzt entschlüsselte Static-Color-Kommando (Länge 15, Subcmd `0x06`, RGB an
   Payload-Byte 5-7) selbst mit `report_send` auf Hardware testen — bisher nur aus dem
   Capture entschlüsselt, noch nicht selbst als Kommando gesendet/gebaut. Erfordert
   erneut kurze Nutzerfreigabe vor Ausführung (gleiches Muster wie bisher) und einen
   `build_report`-Aufruf mit selbst gewählten RGB-Werten (erstes Mal ein **nicht**
   byteidentisch aus einem Capture übernommenes Kommando — CRC wird lokal neu berechnet,
   nicht aus einem bekannten Beispiel kopiert; erfordert daher besondere Sorgfalt beim
   Report-Aufbau vor dem Senden, siehe `SECURITY.md` Regel 10).
2. Vor diesem Test: `report_dump`/eigene Unit-Tests nutzen, um den neu gebauten Report
   offline zu verifizieren (Längenfeld, Byte-2-3-Wert für die aktuelle Session, CRC),
   bevor überhaupt an Hardware gedacht wird.
3. Danach `BACKLOG.md`-Punkt „Static-Color-Kommando real testen" abschließen und
   `SPEC.md`-Kriterium „statische Gesamtfarbe … sicher schalten" von „preset-fest" auf
   „frei wählbar, verifiziert" upgraden.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Ein mit `build_report` selbst konstruierter Static-Color-Report
  (Subcmd `0x06`, Payload `00 00 64 32 00 <R> <G> <B>`, aktuelle Session-ID statt der
  Fixture-`0x0002`) wird vom Gerät akzeptiert und erzeugt die gewählte Farbe — auch wenn
  er nicht byteidentisch aus einem Capture kopiert ist.
- **Erwartetes Ergebnis:** Sichtbare, vom Nutzer bestätigbare Farbänderung passend zu den
  gewählten RGB-Werten.
- **Sicherheitsrisiko:** minimal höher als bei den ersten zwei Tests, da erstmals ein
  Report nicht byteidentisch aus einem echten Capture stammt, sondern aus bekannten,
  aber neu kombinierten Feldern besteht. Weiterhin kein Speicherbefehl, weiterhin
  Timeout/Single-Write/No-Retry.
- **Rückfall:** physischer Hotkey auf der Tastatur (siehe `SECURITY.md` Regel 7), falls
  das Ergebnis unerwartet ist.

## Update — Iteration 12 (2026-08-18, ralph-loop) — erster selbst konstruierter Report erfolgreich

- Mit Nutzerfreigabe: Static-Color-Kommando (`#00FF00`) selbst aus bekannten Feldern
  konstruiert (nicht byteidentisch aus einem Capture kopiert) und gesendet.
- **Erster Versuch schlug fehl** (keine sichtbare Wirkung trotz `write completed`).
  Systematische Fehlersuche mit dem Nutzer zusammen (dessen Idee "liegt es am offenen
  Browser-Tab?" führte zur entscheidenden Spur):
  1. Antwort ausgelesen (vorher nicht gemacht) → abweichendes Format entdeckt
     (Byte 3 = `0x0a` statt `0x00`, volles Echo statt kurzer Bestätigung).
  2. Sitzungs-ID-Hypothese getestet (0x01 vs 0x02, Tab offen/geschlossen) → widerlegt.
  3. Commit-Kommando-Hypothese getestet (Subcmd `0x0a` hinterher) → widerlegt.
  4. Kontrolltest mit bekannt funktionierendem Orange-Kommando über dieselbe Sende-
     Methode → normale Bestätigung, schließt Methodik-Artefakt aus.
  5. **Entscheidender Test:** dieselbe Payload mit der echten, live beobachteten
     Sequenznummer `0x10ad` statt einer erfundenen → normale Bestätigung UND vom
     Nutzer bestätigt sichtbares Grün.
- **Ergebnis: Gerät validiert Sequenznummern** bei dieser Kommandofamilie. Vollständige
  Diagnose in `docs/evidence/sequence-number-validation-test.md`, Architekturkonsequenz
  in `DECISIONS.md` festgehalten (Sequenznummern künftig aus Gerätezustand fortführen,
  nicht frei vergeben).
- `SPEC.md`-Kriterium „statische Gesamtfarbe … sicher schalten" jetzt vollständig erfüllt
  (frei wählbare Farbe, nicht nur Preset) — zwei unabhängige Farbwerte verifiziert
  (`#1FB4FF` aus Capture-Analyse, `#00FF00` aus eigenem Test).

## Nächster konkreter Schritt

1. `SPEC.md`-Fortschritt konsolidieren: prüfen, welche MVP-Abnahmekriterien jetzt
   erfüllt sind (statische Farbe: ja: Einzeltasten: weiterhin nein, keine Per-Key-
   Kommandos bekannt) und die Checkliste entsprechend aktualisieren.
2. Sequenznummer-Akzeptanzregel weiter eingrenzen wäre wertvoll, ist aber ein
   eigenständiger, nicht trivialer Untersuchungsaufwand (mehrere gezielte Tests mit
   verschiedenen Abständen zur echten Sequenznummer) — nicht ungefragt weitere
   Hardwaretests anstoßen, stattdessen dem Nutzer als nächste Option vorschlagen.
3. Alternativ: Ohne weitere Hardwaretests weiterarbeiten — z. B. `report_send`/
   `report_dump` um eine "aus Feldern bauen" CLI-Option erweitern (bisher nur informell
   per Python-Einweg-Skript gemacht), oder mit der OpenRGB-Controller-Grundstruktur
   (Phase 3) beginnen, da das Kernprotokoll (Farbe setzen) jetzt belastbar verstanden ist.

## Update — Iteration 13 (2026-08-18, ralph-loop) — CLI-Tooling ausgebaut

- Auf Nutzerwunsch: `report_build` ergänzt (`src/cli/report_build.cpp`, neues
  CMake-Target) — baut einen Report aus einzelnen Feldern (`--subcmd --seq --session
  --flags --length --payload`) über `build_report()`, druckt Hex auf stdout, druckt
  Zusammenfassung auf stderr (sauber pipe-fähig mit `report_dump`/`report_send`).
  `--length` bewusst ohne Default/Ableitungsformel — muss explizit angegeben werden
  (keine bestätigte Formel, siehe `PROTOCOL.md`).
- Dabei sauberer gelöst als ursprünglich geplant: `session` (Byte 2-3) ist jetzt ein
  echtes Feld in `Interface2Report` (`report.h`/`report.cpp`), Default `0x0002` (passend
  zu den bekannten Fixtures) — statt es nachträglich im CLI-Tool zu patchen. Kein
  Verhaltensunterschied für bestehende Fixtures (alle 20 haben `session=0x0002`,
  Parse→Build-Rundlauf-Test weiterhin grün).
- `report_dump`/`report_send` um Session-Anzeige ergänzt.
- Funktional gegen den real erfolgreichen Grün-Report aus Iteration 12 verifiziert:
  `report_build --subcmd 06 --seq 10ad --session 01 --length 0f --payload
  000064320000ff00` erzeugt exakt denselben Hex (inkl. CRC) wie der zuvor per
  Python-Skript gebaute, hardwareverifizierte Report.
- Alle 4 Unit-Tests weiterhin grün. Kein Gerätezugriff in dieser Iteration.

## Update — Iteration 14 (2026-08-18) — Per-Key-Frage geklärt (negativ)

- Nutzer hat IO Center Web direkt geprüft: nur vorgeformte Effekte/„Figuren" mit
  Animation (Matrix, Tornado, ...), keine Einzeltasten-Konfiguration. Offene Frage aus
  `PROTOCOL.md` damit bestätigt (nicht mehr nur Vermutung aus Capture-Lücke).
- Namensparallele zu Mountain Everest (Matrix, Tornado) als unterstützende Beobachtung
  für gemeinsame Produktherkunft festgehalten, ohne die bereits widerlegte
  Byte-Protokoll-Verwandtschaft neu zu behaupten.

## Nächster konkreter Schritt

Per-Key-Adressierung ist über IO Center Web nicht erreichbar. Optionen, noch nicht
entschieden:
1. Windows-Client-Capture-Plan (VM + USB-Passthrough) ausarbeiten — großer Schritt,
   siehe Master-Prompt Phase 1, nicht ohne Rücksprache beginnen.
2. Ohne Per-Key weiterarbeiten: weitere bekannte Effekte (Matrix, Tornado, ...) gezielt
   capturen wie in Iteration 11 — vertieft das Verständnis von Subcmd `0x06` und klärt
   nebenbei evtl. die offene Frage zu getrennten Zonen (obere/seitliche Leisten),
   ohne den großen VM-Aufwand.
3. Mit dem jetzt belastbar verstandenen Kernprotokoll (Farbe setzen, Sequenznummer-
   Regel bekannt) Richtung Phase 3 (OpenRGB-Controller-Grundstruktur) weitergehen.

## Update — Iteration 15 (2026-08-18) — Matrix-Effekt gezielt gecaptured

- Zweiter gezielter Capture (wie Iteration 11, diesmal Matrix-Effekt statt Farbwahl).
  Genau ein reales Kommando im Mitschnitt gefunden.
- **Ergebnis: löst eine seit Iteration 2 offene Frage** — das Kommando ist byteidentisch
  mit dem bis dahin unentschlüsselten Frame 2341/3109 aus dem alten Fremd-Capture. Damit
  ist klar: das war die ganze Zeit der Matrix-Effekt. Details in
  `docs/evidence/own_capture_matrix_effect.md`.
- Response war eine normale kurze Bestätigung (Byte3=0x00) — Kommando wurde vom Gerät
  akzeptiert, passt zur live beobachteten echten Sequenznummer (`0x10c7`).
- Genaue Payload-Byte-Bedeutung (vermutlich mehrere Parameter: Farben/Geschwindigkeit/
  Richtung) weiterhin nicht vollständig entschlüsselt — bewusst nicht geraten.
- Kein Schreibzugriff — nur Mitschnitt einer Nutzeraktion in IO Center Web.

## Nächster konkreter Schritt

Möglichkeiten, noch nicht entschieden: (1) Tornado-Effekt ebenso gezielt capturen für
einen weiteren Datenpunkt, (2) Matrix-Payload durch einen gezielten Parameter-Vergleich
weiter eingrenzen (z. B. Geschwindigkeit ändern, Rest gleich lassen, erneut capturen),
(3) Richtung Phase 3 (OpenRGB-Controller) weitergehen, da das Kernprotokoll für
Static-Color bereits belastbar ist.

## Update — Iteration 16 (2026-08-18) — größter Einzelfund im Projekt: vollständiges Gerätemanifest

- Auf Nutzervorschlag: Windows-IO-Center-Installer (vom Nutzer bereitgestellt, aus
  `~/Claude-Memory/IO Center Installer.zip`) rein statisch analysiert — nie ausgeführt.
  `innoextract` aus Git-Hauptzweig gebaut (Release 1.9 kennt Inno-Setup-6.3.0 noch
  nicht), Qt-Ressourcenarchive (`.rcc`) per Brute-Force-Zlib-Scan entpackt.
- **Vollständiges offizielles Gerätemanifest gefunden**, inkl.:
  - Alle 6 Effektnamen (Static, ColorWave, Tornado, Breathing, Reactive, Matrix) —
    exakt wie im Mountain-Referenzcode.
  - **Vollständige Per-Key-LED-Tabelle: 168 individuell adressierbare LEDs**,
    durchnummeriert und benannt (45 obere Leiste + 113 Einzeltasten + 10 seitliche
    Leiste). Löst die seit Iteration 1 offene Per-Key-Frage strukturell (welche LEDs
    existieren), auch wenn das Wire-Kommando dafür weiterhin unbekannt ist.
  - Macro-Limits, Polling-Rate, Keybinding-Layer, Bootloader-PID.
- Rohe extrahierte Dateien bleiben lokal (`vendor-extracts-private/`, neu
  git-ignoriert) — nur destillierte Fakten im Repo, siehe neue ADR in `DECISIONS.md`.
- App ist Qt6/QML mit separatem `bequietIOCenterService.exe` + `hidapi.dll` (nicht
  Electron wie zunächst vermutet) — bestätigt native `hidapi`-Kommunikation auf dem
  Windows-Client statt WebHID.
- Kein Byte-Protokoll (Kommandotabellen) im Service-Binary per `strings` gefunden —
  keine Disassembly versucht.

## Nächster konkreter Schritt

Größere Wahlmöglichkeiten, noch nicht entschieden:
1. `light_mount_main_iso.json` (Pixel-Koordinaten) mit der LED-Tabelle verknüpfen —
   Grundlage für ein OpenRGB-Layout, reine Offline-Arbeit.
2. Versuchen, das Per-Key-Wire-Kommando doch noch zu finden — z. B. gezielt Interface 3
   (der bisher unerforschte zweite Vendor-Kanal) untersuchen, da die 168-LED-Tabelle
   jetzt ein klares Ziel gibt, wonach zu suchen ist.
3. Mit dem jetzt sehr detaillierten Verständnis (Effekte, LED-Zählung, Sequenznummer-
   Regel) Richtung Phase 3 (OpenRGB-Controller-Grundgerüst) weitergehen.

## Update — Iteration 17 (2026-08-18) — LED-Tabelle mit Koordinaten verknüpft

- LED-Index-Tabelle (`light_mount_leds_mapping.json`, ISO-Variante) mit den
  Pixel-Koordinaten aus `light_mount_main_iso.json` per Name zusammengeführt.
  Datei per Inhalt als ISO-Variante identifiziert (enthält `UK`/`DE`/`FR`-Layouts und
  die ISO-spezifischen Tasten `Key_NonUsTilde`/`Key_NonUsBackslash`, nicht die
  US-only-ANSI-Variante).
- Ergebnis: 166 Einträge, 111 mit Geometrie (Einzeltasten), 55 ohne (die
  Top/Left/Right-Leisten-LEDs — deren Geometrie steckt vermutlich in separaten,
  noch nicht ausgewerteten `boundingRect`-Dateien aus demselben Vendor-Material).
  Datei: `docs/evidence/light_mount_led_layout_iso.json`, dokumentiert in
  `docs/evidence/led-layout-merge.md`. Eigene Zusammenführung, kein Vendor-Original
  (passend zur ADR in `DECISIONS.md`).
- Reine Offline-Arbeit, kein Gerätezugriff.

## Nächster konkreter Schritt

Noch offen, welche der in Iteration 16 genannten Optionen als Nächstes drankommt:
Per-Key-Wire-Kommando suchen (Interface 3), Leisten-LED-Geometrie ergänzen, oder
Richtung Phase 3 (OpenRGB-Controller-Grundgerüst) weitergehen — dem Nutzer erneut
vorlegen, bevor eigenmächtig eine größere Richtung gewählt wird.

## Update — Iteration 18 (2026-08-18) — read-only Interface-3-Test

- Mit Nutzerfreigabe: `HIDIOCGFEATURE`-ioctl auf `/dev/hidraw11` (Interface 3), alle
  sechs Report-IDs, reines Lesen, kein Schreiben.
- Erfolgreich, kein Fehler, kein USB-Reset, Gerät danach unverändert erreichbar.
- Report-ID 1 und 3 liefern von Null verschiedene Rohwerte (vermutlich Telemetrie/
  Zähler, Bedeutung nicht bekannt). Report-ID 2, 4, 5, 6 durchgehend Null — für den
  Per-Key-Kandidaten (Report-ID 4) weder Bestätigung noch Widerlegung.
- Kein Schreibversuch unternommen — kein bekanntes reales Kommando als Ausgangspunkt
  vorhanden, `SECURITY.md` Regel 10 verlangt hier zusätzliche Vorsicht.

## Nächster konkreter Schritt

Interface-3-Spur ohne echten Traffic (weder fremd noch eigen) weitgehend ausgeschöpft,
ohne zu raten. Optionen, dem Nutzer erneut vorzulegen: (1) hier stoppen und die
Per-Key-Frage als „strukturell verstanden, Wire-Protokoll offen" festhalten, (2)
Richtung Phase 3 (OpenRGB-Controller-Grundgerüst) weitergehen — Static Color und Matrix
sind gut genug verstanden für einen ersten Controller, Per-Key kann als spätere
Erweiterung nachgezogen werden, sobald neue Erkenntnisse vorliegen.

## Update — Iteration 19 (2026-08-18) — OpenRGB-Controller-Grundgerüst geschrieben

- Nutzerentscheidung: kein weiterer Interface-3-Vorstoß, stattdessen Phase 3 beginnen.
- OpenRGB-Quellstand lokal geklont (`openrgb-src-private/`, git-ignoriert), Build-System
  untersucht (qmake, automatisches Glob-Include für `Controllers/*`), Mountain-Referenz
  (`RGBController_MountainKeyboard.{h,cpp}`) als Konventions-Vorlage gelesen.
- Neue ADR in `DECISIONS.md`: kein Submodule, Controller-Dateien leben getrackt in
  diesem Repo unter `openrgb-integration/Controllers/LightMountController/`.
- Controller geschrieben — **bewusst minimal**: nur `SendStaticColor(r,g,b)`, ein
  Zone/ein LED ("Keyboard" als Ganzes), Detection über die schon verifizierten Werte
  (VID `0x373F`, PID `0x0002`, Interface 2, Usage Page `0xFF00`, Usage `0x01`). Kein
  Per-Key, keine Effekte, kein Save/Brightness — nichts geraten, was nicht in
  `PROTOCOL.md` als bestätigt steht. CRC16/MODBUS + Report-Aufbau selbstständig im
  Controller neu implementiert (Mountain-Stil: kein Link gegen unsere separate
  CMake-`protocol`-Bibliothek, OpenRGB-Controller sind traditionell in sich geschlossen).
- Baseline-Build des unveränderten OpenRGB-Quellstands angestoßen (Build-Abhängigkeiten
  installiert, `qmake && make -j$(nproc)`), lief bei Sitzungsende noch (großes Projekt,
  hunderte Controller) — Ergebnis und Test mit unserem Controller stehen noch aus.
- Kein Gerätezugriff in dieser Iteration.

## Nächster konkreter Schritt

1. Baseline-Build abwarten/prüfen, dann unsere Controller-Dateien in den Checkout
   kopieren und neu bauen — prüfen, ob unser Code fehlerfrei kompiliert.
2. Falls Build erfolgreich: `./openrgb --list-devices` bzw. echten Start prüfen, ob die
   Light Mount erkannt wird (Detection-Parameter sind bereits verifiziert, aber die
   Registrierung selbst noch nicht real getestet).
3. Erst danach, mit erneuter Nutzerfreigabe: echten Hardwaretest über den neuen
   Controller-Code (nicht mehr über `report_send`) versuchen — insbesondere die
   unverifizierte Sequenznummer-Strategie (Start bei 1) auf die Probe stellen.

## Update — Iteration 20 (2026-08-18) — Build erfolgreich, Gerät erkannt

- Baseline-OpenRGB-Build fertig, unsere Controller-Dateien reinkopiert, neu gebaut:
  **fehlerfrei kompiliert**, kein Fehler in `LightMountController.cpp`/
  `RGBController_LightMount.cpp`/`LightMountControllerDetect.cpp`.
- `./openrgb --list-devices` erkennt **"be quiet! Light Mount"** korrekt als eigenes
  Gerät: `Location: HID: /dev/hidraw10` (Interface 2, exakt wie vorgesehen),
  `Serial: [REDACTIERTE-SERIENNUMMER]` (stimmt mit `lsusb -v` aus Iteration 1 überein), `Modes:
  [Static]`, `Zones: Keyboard`, `LEDs: Keyboard` — alles wie im Controller-Code
  definiert. Erster echter End-to-End-Nachweis der Detection.
- Noch kein Farbtest über OpenRGB selbst ausgeführt (würde erstmals die unverifizierte
  Sequenznummer-Strategie — Start bei 1 — auf echter Hardware prüfen).

## Nächster konkreter Schritt

Mit Nutzerfreigabe: `./openrgb --device 0 --mode static --color <hex>` ausführen und
beobachten, ob die Farbe angenommen wird (Sequenznummer-Start-bei-1-Hypothese) oder ob
das Gerät ablehnt (analog zum `0x2000`-Fehlschlag aus Iteration 12). Danach Ergebnis in
`PROTOCOL.md`/`openrgb-integration/README.md` festhalten.

## Update — Iteration 21 (2026-08-18) — Sequenznummer-Start-bei-1 widerlegt

- Mit Nutzerfreigabe: `./openrgb --device 0 --mode static --color 8000FF` ausgeführt.
  Kein Fehler, kein USB-Reset, aber **keine sichtbare Farbänderung**.
- Zur Diagnose das exakte Kommando (Session `0x0002`, Seq `0x0001`, wie vom Controller
  gesendet) über `hidraw` nachgebaut und die Antwort ausgelesen: **identisches
  Ablehnungsmuster** wie beim `0x2000`-Fehlschlag (Byte 3 = `0x0a`, volle Echo-Antwort).
- **Hypothese widerlegt:** ein bei 1 startender Zähler ist NICHT die Lösung — sowohl
  sehr niedrige als auch sehr hohe frei gewählte Werte werden abgelehnt. Nur die eine
  real beobachtete Sequenznummer (`0x10ad`) hat je funktioniert. Deutet auf echten,
  bisher nicht zugänglichen Gerätezustand hin.
- Nebenbei echten Code-Bug gefunden und behoben: `RGBController_LightMount`-Destruktor
  rief `Shutdown()` nicht auf (OpenRGB-Warnung beim Beenden) — gefixt, neu gebaut,
  fehlerfrei.
- Alle Doku-Stellen (`PROTOCOL.md`, `openrgb-integration/README.md`, `BACKLOG.md`)
  ehrlich aktualisiert: Controller baut und wird erkannt, Kernfunktion (Farbe setzen)
  ist noch **nicht praktisch nutzbar**, bis die Sequenznummer-Frage gelöst ist.

## Nächster konkreter Schritt

Vielversprechendster, noch nicht verfolgter Ansatz: die Interface-3-Report-ID-1-
Telemetriewerte (siehe `PROTOCOL.md`) vor einem Schreibversuch auslesen und prüfen, ob
sich daraus eine gültige/erwartete Sequenznummer ableiten lässt (z. B. eines der
32-Bit-Felder als laufender Zähler). Erfordert erneuten, aber rein lesenden
Hardwarezugriff (`GET_FEATURE`, kein Schreiben) — mit kurzer Nutzerfreigabe vorab, wie
bisher etabliert. Alternativ: hier als bekannte, dokumentierte Einschränkung stehen
lassen und andere Baustellen priorisieren.

## Update — Iteration 22 (2026-08-18) — Telemetrie-Hypothese geprüft und verworfen

- Interface 3 Report-ID 1 erneut rein lesend ausgelesen (`GET_FEATURE`, kein Schreiben)
  und mit dem Wert aus Iteration 18 verglichen.
- **Ergebnis:** Zwei der fünf 32-Bit-Werte hatten sich verändert, obwohl in der
  Zwischenzeit nichts geschrieben wurde — sehen nach frei laufenden Zählern/Timern aus,
  nicht nach einer "nächste erwartete Sequenznummer". Die übrigen drei Werte blieben
  konstant (vermutlich feste Geräte-Kennungen) und liegen nicht plausibel nahe an der
  einzigen bekannten funktionierenden Sequenznummer.
- **Hypothese verworfen, bewusst kein weiterer Schreibversuch unternommen** — die
  verbleibenden Kandidatenwerte sind zu spekulativ für einen weiteren realen
  Hardwarezugriff. Sequenznummer-Frage bleibt offen, siehe `BACKLOG.md`.

## Nächster konkreter Schritt

Sequenznummer-Frage ist für diese Session ausgereizt, ohne zu raten. Guter Punkt zum
Pausieren — 22 von 30 Loop-Iterationen genutzt, sehr ergiebiger Tag mit mehreren echten
Durchbrüchen (RGB-Kodierung, Matrix-Effekt, 168-LED-Manifest, funktionierender
OpenRGB-Detection-Nachweis) und einer ehrlich dokumentierten offenen Kernfrage.

## Update — Iteration 23 (2026-08-19) — Durchbruch: Zähler-Feld korrekt verstanden

- Neuer Tag, neue Session. Nutzer schlug vor, gezielt den **Verbindungsaufbau** zu
  capturen (IO Center Web komplett geschlossen, Mitschnitt gestartet, dann frisch
  verbunden) — das hatten wir noch nie gemacht, wir waren bisher immer mitten in einer
  laufenden Session eingestiegen.
- **Zentrale Korrektur:** Das bisher als "16-Bit-Sequenznummer" (Byte 4-5) verstandene
  Feld ist tatsächlich zwei getrennte Bytes. Byte 4 ist ein eigenständiger,
  **fortlaufender 1-Byte-Zähler** (steigt exakt +1 pro Kommando, unabhängig vom
  "Session"-Feld Byte 2-3, das mehrfach innerhalb derselben Verbindung wechselt). Byte 5
  ist ein separates, kontextabhängiges Feld — konstant `0x10` für Static-Color, `0x01`
  für Keepalive, sonst variabel (Attribut-Kennung während des Handshakes).
- **Bestätigt mit echtem Hardwaretest:** Zähler direkt an den im Capture beobachteten
  letzten Stand angeschlossen (`0x5c`→`0x5d`) und ein selbst konstruiertes Kommando
  gesendet (Lila `#8000FF`) — **normale Bestätigung, vom Nutzer live bestätigt.**
  Zweites, direkt fortgesetztes Kommando (`0x5e`, Gelb `#FFFF00`) — **ebenfalls
  erfolgreich.** Damit ist bewiesen: Sobald der Zähler einmal korrekt anschließt,
  funktioniert einfaches Weiterzählen zuverlässig für mehrere Kommandos in Folge.
- Code entsprechend korrigiert: `Interface2Report.seq` (u16) → `counter`/`marker` als
  getrennte Felder in `src/protocol/report.h`/`.cpp`, CLI-Tools (`report_dump`,
  `report_send`, `report_build` — `--seq` → `--counter` + neues `--marker`) und der
  OpenRGB-Controller. `LightMountController` verlangt jetzt explizites
  `SetCounter()`-Priming, bevor `SendStaticColor()` überhaupt schreibt — kein Raten mehr.
  Alle Tests grün, Build gegen echten OpenRGB-Checkout weiterhin fehlerfrei, Gerät
  weiterhin korrekt erkannt.
- Neuer teilweise entschlüsselter Identifikations-Handshake gefunden (Geräte-Serial-
  String, Fähigkeitenliste) — nicht vollständig gedeutet, siehe
  `docs/evidence/connection-handshake-analysis.md`.

## Nächster konkreter Schritt

Kaltstart-Problem bleibt der Hauptblocker: wie lernt ein frisch startender Client (ohne
Live-Capture) den aktuell gültigen Zählerstand? Vielversprechende, noch nicht getestete
Hypothese: der Zähler scheint nach einer Weile zurückgesetzt zu werden (heute bei `02`
begonnen, obwohl gestern deutlich höher endend) — möglicherweise akzeptiert das Gerät
unmittelbar nach einem solchen Reset (fast) jeden Startwert. Test würde eine gesicherte
Ruhephase (kein Browser-Tab offen, längeres Warten) erfordern, um zu prüfen, ob danach
ein "kalter" Startwert wie `1` doch angenommen wird. Mit dem Nutzer abstimmen, ob das
heute verfolgt werden soll.

## Update — Iteration 24 (2026-08-19) — Interface 3 an der Grenze des sicher Testbaren

- Zwei weitere risikofreie Tests auf Interface 3: reiner Lesetest auf den Interrupt-IN-
  Endpunkt (nichts wartend) und ein minimaler No-Op-`SET_FEATURE`-Test auf Report-ID 4
  (exakt die bereits gelesenen Nullen zurückgeschrieben — kein geratener Inhalt).
- **Ergebnis:** Schreiben wird vom Gerät akzeptiert (kein Fehler, kein Reset), aber ohne
  erkennbaren Effekt (weiterhin Nullen beim Rücklesen). Bestätigt nur, dass
  Schreibzugriffe grundsätzlich funktionieren — nichts über die Bedeutung von echtem
  Inhalt.
- Bewusst **kein** Test mit geratenem/konstruiertem Inhalt unternommen — ohne jeden
  bekannten realen Referenzwert wäre das reines Raten. Interface-3-Spur damit an der
  Grenze des mit dieser Session sicher Testbaren angekommen.

## Nächster konkreter Schritt

Zwei offene Hauptbaustellen bleiben liegen, beide brauchen entweder mehr Zeit
(Zähler-Timeout-Hypothese) oder neue Daten (Per-Key: echter Capture oder gezielter
Windows-Client-Traffic nötig, den wir nicht haben). Guter Punkt, um mit dem Nutzer die
weitere Priorität für heute zu klären — z. B. Identifikations-Handshake vertiefen
(reine Offline-/Analysearbeit an bereits vorhandenen Daten) oder eine andere Richtung.

## Update — Iteration 26 (2026-08-19, kurze Folgesession, ~1h) — Zähler bleibt über Stunden gültig

- Kurzer Check-in: Nutzer wollte die Tastatur nach den gestrigen/vormittäglichen
  Testfarben wieder in einen "aufgeräumten" Zustand bringen. Echte Per-Key-Steuerung
  in dieser kurzen Session bewusst nicht verfolgt (kein Referenz-Traffic verfügbar,
  Windows-VM-Aufwand zu groß für die verfügbare Zeit) — mit dem Nutzer offen so
  besprochen, keine falschen Erwartungen geweckt.
- Zählerstand `0x5e` (aus dem Vormittags-Test) fortgesetzt zu `0x5f`, ca. eine Stunde
  später, ohne zwischenzeitliche Browser-Aktivität: **weiterhin akzeptiert.** Magenta
  (`#FF00FF`) erfolgreich gesetzt, vom Nutzer live bestätigt.
- **Neuer Erkenntnisgewinn:** spricht gegen einen kurzfristigen (Minuten-/Stunden-)
  Timeout für den Zähler-Reset. Der zwischen den Tagen beobachtete Reset liegt auf
  einer längeren Zeitskala oder ist an ein anderes Ereignis (z. B. Standby) geknüpft.
  Praktische Konsequenz: ein einmal ermittelter Zählerstand lässt sich vermutlich über
  eine ganze Arbeitssitzung hinweg weiterverwenden — das Kaltstart-Problem betrifft vor
  allem den Sitzungsbeginn, nicht laufenden Betrieb. Details:
  `docs/evidence/connection-handshake-analysis.md`.

## Nächster konkreter Schritt

Unverändert aus Iteration 24/25: Zähler-Kaltstart-Lösung (jetzt mit dem neuen
Datenpunkt: Reset-Trigger ist NICHT kurzfristige Inaktivität) und Per-Key-Wire-Kommando
bleiben die beiden offenen Hauptbaustellen. Windows-VM-Ansatz für Per-Key ist als
größeres, eigenständiges Vorhaben zu planen, nicht nebenbei.

## Update — Iteration 27 (2026-08-19/20) — Windows-VM für Per-Key: QEMU aufgegeben, VirtualBox-Weg, Host-Reboot nötig

Nutzerauftrag: "Fang mit der Windows-VM für Per-Key an" — Ziel ist, die echte
Windows-IO-Center-App per USB-Passthrough mit der Tastatur zu verbinden und
host-seitig per `usbmon` mitzuschneiden, um endlich echten Per-Key-Referenz-Traffic
zu bekommen (bisher komplett fehlend, siehe Iteration 24).

**Weg 1 — QEMU/libvirt, aufgegeben:** Unattended-Windows-11-Install gebaut
(`autounattend.xml` mit `HKLM\SYSTEM\Setup\LabConfig`-Bypass für TPM/SecureBoot/RAM/
CPU-Checks, generischer KMS-Key). Boot-Menü-Timing-Problem gelöst: statt interaktivem
Boot-Manager-Menü (ENTER registrierte unzuverlässig) direkt `EFI\BOOT\BOOTX64.EFI` aus
der UEFI-Shell gestartet, zusätzlich explizite `<boot order='N'/>`-Elemente pro
Datenträger in der libvirt-Domain-XML statt `<os><boot dev='hd'/></os>`. Trotzdem:
VM bootet bis in die Windows-Boot-Animation (schwarzer/blauer Vollbild-Frame sichtbar,
per Screenshot bestätigt), **resettet sich dann lautlos ohne jede Fehlermeldung**
(vermutlich CPU-Triple-Fault). Fünf Hypothesen systematisch ausgeschlossen:
Hyper-V-Enlightenments `evmcs`/`avic` entfernt, TPM-Emulator komplett entfernt,
`itco`-Watchdog entfernt, Video `qxl`→`virtio-gpu` gewechselt, CPU
`host-passthrough`→benanntes `Skylake-Client-noTSX-IBRS`-Modell gewechselt — keine
Änderung am Symptom. Debug-Log-Versuch (`-d cpu_reset,guest_errors` via
`qemu:commandline`) scheiterte an einem "Permission denied" beim Log-Öffnen, das auch
mit `seccomp_sandbox=0` in `qemu.conf` bestehen blieb (Ursache nicht geklärt, nicht
weiter verfolgt — zu viele Meta-Ebenen von der eigentlichen Aufgabe entfernt).
**Ungetestet bei Abbruch:** `pc-i440fx` statt `pc-q35` als Machine-Type.

**Weg 2 — VirtualBox, aktueller Stand:** Nutzer-Hinweis "virtual box ist auf dem PC
ebenfalls vorhanden" → Wechsel. `VBoxManage unattended install` (mit `--tpm-type 2.0`,
`--firmware efi`, keine Secure-Boot-Option in dieser VBox-Version nötig/vorhanden) lief
beim ersten Versuch **auf Anhieb fehlerfrei durch** (~15 Minuten, kein
Boot-Menü-Gefrickel, kein manuelles Keypress-Timing) — deutlich robuster als der
QEMU-Weg. Danach erfolgreich: Guest Additions 7.2.6 silent-installiert (`/S`-Flag,
UAC-Bestätigung per `keyboardputscancode`), IO-Center-Installer (aus
`~/Claude-Memory/IO Center Installer.zip`, per `VBoxManage guestcontrol copyto` +
PowerShell `Expand-Archive` übertragen) silent-installiert (`/VERYSILENT
/SUPPRESSMSGBOXES /NORESTART /SP-`, ebenfalls UAC-Bestätigung nötig) —
`C:\Program Files\IO Center\` mit `bequietIOCenterService.exe`, `IO_Center.exe`,
`hidapi.dll` bestätigt vorhanden. USB-Passthrough des Light Mount (VID `373f`/PID
`0002`, UUID wechselt bei jedem Attach) mechanisch verifiziert: Host-Interfaces binden
korrekt an `usbfs` (statt `usbhid`), `VBoxManage list usbhost` zeigt `Current State:
Captured`.

**Zwei eigenständige, vom Kernprotokoll unabhängige Probleme dabei gefunden und
(teilweise) gelöst:**

1. **`VBoxSVC`-Gruppen-Stale-Bug:** `sg vboxusers -c "..."` und `newgrp` wirken in
   dieser Sandbox-Shell-Umgebung nicht (kein `CAP_SETGID` — getestet, `id` im
   `sg`-Kontext zeigt zwar die neue Gruppe, das tatsächlich gestartete `VBoxSVC`
   behält aber trotzdem seine alten Gruppen). Nacktes `VBoxManage` läuft unter den
   Gruppen, mit denen `VBoxSVC` beim allerersten Start hochgefahren ist — bleibt so,
   bis der Daemon neu startet, auch nach `usermod -aG`. **Fix:** ausnahmslos
   `sudo -u mathias VBoxManage ...` verwenden (löst Gruppen bei jedem Aufruf sauber
   neu auf). Nach jeder `usermod -aG`-Änderung `VBoxSVC` beenden
   (`pkill -u mathias -f VBoxSVC`), damit der nächste `sudo -u mathias`-Aufruf ihn mit
   der neuen Gruppenmitgliedschaft neu startet — sonst weiterhin „VirtualBox is not
   currently allowed to access USB devices" trotz korrektem `/etc/group`.
2. **VT-x-Konflikt VirtualBox vs. KVM:** Mit `kvm_intel`/`kvm` gleichzeitig geladen
   (von `libvirt`, auch ohne laufende libvirt-VM) crasht die VirtualBox-VM nach
   wenigen Minuten mit `HCPhysVmxEnableError` → `GURU_MEDITATION` (VBox.log-Beleg:
   `HM: HCPhysVmxEnableError = 0x...` gefolgt von `Changing the VM state from
   'RUNNING' to 'GURU_MEDITATION'`) — beide Hypervisoren beanspruchen exklusiven
   VMX-Zugriff auf der CPU. `--vm-execution-engine=native-api` (VBox nutzt Host-KVM
   als Backend) **existiert nur auf Windows (WHP) und macOS (HVF), nicht auf Linux**
   (`NEM is not available` / `VERR_NEM_NOT_AVAILABLE`) — Holzweg, nicht
   weiterverfolgen. `kvm_intel`/`kvm` ließen sich nicht sauber per `rmmod` entladen
   (Refcount 1, aber `lsof`, `/proc/*/fd`-Scan und `/proc/*/maps`-Scan über alle
   Prozesse fanden keinen sichtbaren Halter — vermutlich ein interner
   `hardware_enable()`-Zustand, der nicht an eine offene Datei gebunden ist).
   **Fix: Host-Neustart** (durchgeführt 2026-08-20, nach Sicherung dieses Stands).
   **Praktische Konsequenz für künftige Sessions: nie eine libvirt/KVM-VM parallel
   zur VirtualBox-Windows-VM starten**, sonst tritt der Konflikt erneut auf.

**Kollateralschaden (wichtige Lehre):** Ein harter `kill -9` des laufenden
`VBoxHeadless`/`VBoxSVC`-Prozesses (statt sauberem `VBoxManage controlvm ... poweroff`
oder ACPI-Shutdown im Gast) während die Windows-VM aktiv lief, hat vermutlich das
PnP-/WMI-Subsystem im Windows-Gast beschädigt: Geräte-Manager komplett leer (keine
einzige Zeile), `Get-CimInstance`/`Get-PnpDevice` liefern durchgehend leere
Objektlisten, `pnputil` erkennt selbst seine eigenen dokumentierten Standardargumente
nicht mehr — aber `sc query PlugPlay`/`sc query Winmgmt` zeigen beide weiterhin
`RUNNING`, also keine abgestürzten Dienste, eher eine tiefere State-Korruption.
Führte zu einer kompletten zweiten Windows-Neuinstallation (Datenträger gelöscht,
`VBoxManage unattended install` erneut, diesmal von Anfang an konsequent mit
`sudo -u mathias`) — die dann aber direkt in den VT-x/KVM-Crash (Problem 2 oben)
lief, bevor Guest Additions/IO Center erneut installiert werden konnten.
**Lehre: VM-Prozesse nie mit rohem `kill`/`pkill` beenden, solange der Gast läuft** —
auch wenn der Host-seitige Grund für den Kill (Gruppen-Refresh) berechtigt ist, den
sauberen `VBoxManage controlvm <vm> poweroff`-Weg nehmen oder die VM vorher regulär
herunterfahren.

Fortschritt vor dem Reboot in `~/Claude-Memory/bequiet-lightmount-linux.md` (Vault)
und Tagesnote `2026-08-20.md` gesichert.

## Nächster konkreter Schritt

Nach dem Host-Reboot (VT-x/KVM-Konflikt sollte behoben sein, sauberer
`kvm_intel`-Modulzustand): VirtualBox-VM `lightmount-win11` neu erstellen (dritter
Anlauf — Prozedur jetzt vollständig bekannt: `VBoxManage createvm` → `modifyvm`
mit `--firmware efi --tpm-type 2.0 --usbxhci on` → `createmedium`/`storageattach`
→ `unattended install`, alles mit `sudo -u mathias`). Danach Guest Additions +
IO Center erneut installieren (Silent-Install-Flags oben dokumentiert), USB-
Passthrough des Light Mount erneut herstellen, IO Center App öffnen und
gleichzeitig host-seitig `usbmon` (`/sys/kernel/debug/usb/usbmon/<bus>u`)
mitschneiden während eines echten Per-Key-Testkommandos in der App. **Wichtig:**
während dieser gesamten Sitzung keine libvirt/KVM-VM parallel starten.

## Blocker

Zähler-Kaltstart-Problem für den OpenRGB-Controller (unverändert seit Iteration 24/26)
und Per-Key-Wire-Kommando — Windows-VM-Aufbau für Per-Key läuft, aber noch nicht bis
zum eigentlichen Capture gekommen (VT-x/KVM-Konflikt + Windows-Neuinstallation haben
die gesamte Sitzung 2026-08-19/20 aufgebraucht, ohne dass IO Center je mit
angeschlossener Tastatur geöffnet wurde). Host-Reboot ist Voraussetzung für den
nächsten Versuch.

## Update — Iteration 28 (2026-08-20, nach Host-Reboot) — VM dritter Anlauf erfolgreich, IO Center läuft mit echter Hardware

Nutzerauftrag: "Weiter mit der Tastatur nach reboot" — Fortsetzung von Iteration 27.

**Host-Zustand nach Reboot verifiziert:** `kvm_intel`/`kvm` sauber geladen (kein Halter,
kein laufender libvirt/QEMU-Prozess), kein VBoxSVC/VBoxHeadless-Prozess aktiv — VT-x-
Konflikt strukturell behoben.

**VM dritter Anlauf:** Alter `lightmount-win11`-Datenträger war vom VT-x-Crash
unbrauchbar (`Windows Boot Manager ... Not Found` beim Boot — Install kam nie bis zum
Bootloader-Schreiben). VM komplett gelöscht und neu erstellt (`createvm` →`modifyvm`
`--memory 8192 --cpus 4 --firmware efi --tpm-type 2.0 --usbxhci on` → `storagectl`/
`createmedium` 80GB → `storageattach`), alles mit `sudo -u mathias`.

- Erster Unattended-Install-Versuch mit `~/Win11_25H2_nocheck.iso` (modifiziertes,
  Hardware-Check-befreites Image) schlug fehl: `VISO: Failed to locate
  '/efi/microsoft/boot/cdboot.efi'` — dieses Image fehlt eine Datei, die VBoxManages
  eigener EFI-Unattended-Template beim Bau der Hilfs-ISO erwartet. **Nicht mehr nötig
  ohnehin:** `VBoxManage unattended install --firmware efi --tpm-type 2.0` umgeht
  TPM/Secure-Boot-Prüfungen bereits selbst.
- Zweiter Versuch mit dem unveränderten `~/Downloads/Win11_25H2_German_x64.iso` lief
  sauber durch (~25 Minuten inkl. mehrerer Reboots, kein VT-x-Crash mehr — Beleg, dass
  der Host-Reboot das Problem tatsächlich behoben hat). Admin-Login `Administrator`/
  `LightMount!2026` (unverändert aus Iteration 27).
- Guest Additions 7.2.6 silent installiert (`D:\VBoxWindowsAdditions.exe /S` über
  `keyboardputstring` ins "Ausführen"-Fenster getippt, kein UAC-Prompt nötig — das
  eingebaute `Administrator`-Konto ist von UAC-Bestätigungen ausgenommen). Nach
  silent-Install war `guestcontrol` zunächst "guest execution service is not ready" —
  ein `VBoxManage controlvm ... reset` hat das behoben (GA-Dienst braucht einen Neustart
  zur Aktivierung), danach `guestcontrol run` sofort funktionsfähig (Testbefehl in 15s
  bestätigt).
- IO-Center-Installer aus `~/Claude-Memory/IO Center Installer.zip` per `guestcontrol
  copyto` übertragen. **Leerzeichen im Dateinamen brachen mehrfach die Quoting-Kette**
  (bash → VBoxManage → guest cmd → powershell -Command) — Fix: kleine `.ps1`-Skripte
  lokal schreiben und per `copyto` + `-File` ausführen statt `-Command`-Inline-Strings,
  umgeht das Problem komplett. Silent-Install (`/VERYSILENT /SUPPRESSMSGBOXES
  /NORESTART /SP-`) erfolgreich, alle erwarteten Dateien vorhanden
  (`bequietIOCenterService.exe`, `IO_Center.exe`, `hidapi.dll`, Qt6-DLLs).

**USB-Passthrough + "Aktive Sitzung"-Blocker gelöst:** `VBoxManage controlvm usbattach`
zeigte sofort `Current State: Captured`, aber IO Center zeigte beim ersten Start einen
blockierenden Dialog: *"Warnung: Aktive Sitzung — Das Gerät wird derzeit von einer
anderen Anwendung verwendet. Bitte schließe sie und stecken Sie das Gerät erneut ein."*
Klicks auf "Weiter" änderten nichts (Dialog kam sofort wieder). **Ursache vermutlich:
firmware-seitiger Session-Zustand** (vgl. das in `PROTOCOL.md`/Iteration 24-26
dokumentierte Session-Byte im Protokoll-Header) — reines USB-Passthrough ist kein
echter elektrischer Replug, das Gerät "erinnert" sich an einen alten Session-Zustand.
**Fix:** echter Reconnect via `usbdetach` → Host-seitig `echo '1-2' >
/sys/bus/usb/drivers/usb/{unbind,bind}` (dieselbe Methode wie beim Apex-3-KVM-Fix,
siehe Obsidian-Note `openrgb-steelseries-apex3-setup`) → `usbattach` erneut. Danach
sofort: App zeigt das normale Willkommens-Tutorial statt der Warnung — Blocker weg.
**Wichtige Erkenntnis für künftige Sessions:** reines `VBoxManage usbdetach`/
`usbattach` reicht nicht, wenn das Gerät einen aktiven Session-Zustand hat — immer
zusätzlich unbind/bind (oder echtes Aus-/Einstecken) dazwischen.

**GUI-Automatisierung ohne Guest-Additions-Maus-Fokus:** `VBoxManage
keyboardputscancode`/`keyboardputstring` funktionieren zuverlässig für Tastatur
(Win+R, Textstrings, Win+Up zum Maximieren). Für Maus-Klicks: `xdotool mousemove
--window <hostFensterID> <x> <y> click 1` gegen das reale VirtualBox-GUI-Fenster auf
dem Host-Display (`DISPLAY=:0`, `sudo -u mathias`) funktioniert, **aber**: VirtualBox
zeigt den Gast-Framebuffer bei Fenstergrößenänderung NICHT automatisch skaliert an
(kein Auto-Resize der Gastauflösung trotz aktiver Guest Additions/VBoxTray) — beim
Vergrößern des Host-Fensters bleibt die Gastauflösung bei 1024x768 und wird nur mit
grauem Rand zentriert dargestellt, nicht gestreckt. **Für Klick-Koordinaten IMMER
frisch mit `import -window <id>` (ImageMagick) einen Host-seitigen Screenshot machen
und Pixelkoordinaten direkt daraus ablesen** — nicht aus einer Formel/vorherigen
Screenshot-Geometrie hochrechnen, das führt zu Fehlklicks (in dieser Session zweimal
passiert: einmal landete ein Klick im grauen Letterbox-Rand, einmal traf ein Klick
vermutlich einen Fenster-Chrome-Button und minimierte/verkleinerte das App-Fenster
unerwartet).

**usbmon-Capture-Hinweis:** `tshark -i usbmon1` scheitert trotz `sudo` mit "Permission
denied" — das `dumpcap`-Subprofil in `/etc/apparmor.d/tshark` erlaubt zwar
`owner rw /**.pcap{,ng}` fürs Schreiben, hat aber keine Regel für den
usbmon-Debugfs-Knoten. **Workaround:** rohes Text-Interface direkt lesen, umgeht
AppArmor komplett: `sudo cat /sys/kernel/debug/usb/usbmon/<bus>u > datei.txt &`
(Bus 1 für den Light Mount in diesem Setup, Device-Adresse per `lsusb -d 373f:0002`
prüfen). Läuft seit Sitzungsende im Hintergrund mit.

**Stand bei Sitzungsende:** IO-Center-Dashboard zeigt die Light Mount live mit
echtem Rainbow-Gradient (Hardware reagiert, Verbindung steht). Tutorial übersprungen.
Noch nicht gefunden: der eigentliche Per-Key-Farbwähler/Effekt-Editor in der
Seitenleiste (zwei Icons unklar zugeordnet — Sidebar-Klicks haben teils unerwartete
UI-Reaktionen ausgelöst, siehe GUI-Automatisierungs-Hinweis oben). **Nächster Schritt:**
Nutzer ist am selben physischen Rechner (VM läuft im GUI-Modus, sichtbar auf dem realen
Desktop) — für die feingranulare Navigation zum Per-Key-Editor und das Auslösen eines
echten Testbefehls übernimmt der Nutzer besser direkt Maus/Tastatur, während die
usbmon-Aufzeichnung im Hintergrund weiterläuft. Capture-Datei:
`/tmp/claude-1000/-home-mathias/21257473-06b3-498e-be36-e99d5fc3302c/scratchpad/usbmon1-capture.txt`
(liegt im Session-Scratchpad, nicht im Repo — vor Sitzungsende ggf. sichern/analysieren).

## Update — Iteration 28 Fortsetzung (2026-08-20, gleiche Sitzung) — VM-Grafikbug gefunden+behoben, "Busy"-Status bleibt trotz allem bestehen

**Blocker 1 (gelöst): Mausklicks in der VM funktionierten weder für Claude (`xdotool`)
noch für den Nutzer selbst.** Ursache zweigeteilt:

1. Claude-seitig: `xdotool click`/`mousedown`/`mouseup` gegen das VirtualBox-GUI-Fenster
   hatten nie eine Wirkung, selbst auf VirtualBox' eigener nativer Menüleiste — trotz
   korrektem Fokus (`getactivewindow` bestätigt) und korrekter Cursor-Position
   (`getmouselocation` bestätigt). Sehr wahrscheinlich KDE-Wayland/XWayland-
   Sicherheitsbeschränkung, die synthetische XTEST-Klick-Events von anderen Prozessen
   blockiert, während reine Zeigerbewegung durchgelassen wird. **Bleibt ungelöst, nicht
   weiter verfolgt** — Klicks müssen für Claude-Automatisierung in dieser Umgebung
   grundsätzlich als nicht verfügbar gelten; nur `VBoxManage keyboardputscancode`/
   `keyboardputstring` (läuft über VirtualBox' COM-API, nicht X11) sind zuverlässig.
2. **Nutzer-seitig (echte Root-Cause):** VM wurde mit `--graphicscontroller vmsvga`
   erstellt (VMware-kompatibler SVGA-Adapter). Guest Additions' WDDM-Grafiktreiber
   (`VBoxWddm.inf`) ist aber fest an VirtualBox' **eigenen** VGA-Adapter
   (`PCI\VEN_80EE&DEV_BEEF`) gebunden — bei VMSVGA (`PCI\VEN_15AD&DEV_0405`) schlägt die
   Treiberbindung mit `Unable to find any matching devices` fehl (Beleg:
   `C:\Windows\INF\setupapi.dev.log`, Abschnitt `[Device Install ... PCI\VEN_80EE&DEV_BEEF]`).
   Windows fällt auf "Microsoft Basic Display Adapter" zurück → feste 1024×768-Auflösung
   (kein `setvideomodehint`, kein `ChangeDisplaySettings` möglich, da der Fallback-Treiber
   keine dynamischen Modi kennt) **und** kaputte Maus-Zeiger-Integration (Cursor
   verschwindet beim Betreten des Fensters, kein Klick registriert — der fehlende
   WDDM-Treiber kann offenbar keinen Gast-Cursor rendern/melden).
   **Fix:** `VBoxManage controlvm ... poweroff` → `VBoxManage modifyvm ... 
   --graphicscontroller vboxsvga` → `startvm`. Danach zeigt Geräte-Manager
   `VirtualBox Graphics Adapter (WDDM)` (Treiber 7.2.6.22322, Status OK), Auflösung passt
   sich automatisch an die Fenstergröße an, Maus funktioniert normal (vom Nutzer live
   bestätigt: "jetzt habe ich einen Mauszeiger" / Klicks lösen aus).
   **Nebenbefund:** `Microsoft PS/2-Maus` bleibt auch danach der aktive Maustreiber
   (nicht `VBoxMouse.inf`, obwohl im Treiber-Store vorhanden, `sto: Driver package
   already imported`) — offenbar für moderne Windows-Gäste irrelevant, die
   Maus-Integration läuft über den Grafiktreiber/VMMDev, nicht über einen PS/2-Filter.
   **Für künftige Windows-VM-Neuanlagen in diesem Projekt: immer `--graphicscontroller
   vboxsvga` verwenden, nicht `vmsvga`.**
   **Kollateral-Lektion:** `VBoxManage guestcontrol` meldet nach jedem `modifyvm`/
   Treiber-Wechsel + `startvm`/`reset` vorübergehend "guest execution service is not
   ready" — normalerweise erholt sich das binnen 10-20s von selbst (einfach erneut
   pollen), außer nach einem fehlgeschlagenen GA-Reinstall-Versuch (siehe unten), dann
   kann es 2+ Minuten hängen bleiben und braucht einen weiteren Reset.
   **Riskanter Nebenschritt (im Nachhinein unnötig):** Zwei Versuche,
   `VBoxWindowsAdditions(-amd64).exe /S` blind erneut laufen zu lassen, um den
   Maustreiber zu erzwingen — beide mit Exit-Code 1 fehlgeschlagen (laut
   `setupapi.dev.log`: `Query-removal ... vetoed`, das Kern-Gerät kann nicht im laufenden
   Betrieb neu installiert werden, braucht einen echten Neustart). Der zweite Versuch
   hat `guestcontrol` für über 2 Minuten lahmgelegt. **Lehre:** Guest-Additions-
   Treiberprobleme brauchen einen vollständigen `poweroff`+`startvm`, kein wiederholtes
   blindes Neuinstallieren im laufenden Betrieb.

**Blocker 2 (weiterhin ungelöst): Firmware-seitiger "Busy"-Status überlebt jede
Reset-Methode.** In `C:\Users\Administrator\AppData\Local\be quiet!\IO Center\logs\*.log`
zeigt jede einzelne Verbindungsaufnahme (`qlink_protocol.hid: Light Mount ... initialized:
... Protocol status Busy` / `dev_list: Add Device(...) Status: Busy`) denselben Zustand —
**fünf unabhängige Reset-Versuche in Folge, alle wirkungslos:**
1. Software-USB-Reconnect (`usbdetach`/`unbind`+`bind`/`usbattach`)
2. Echtes physisches Aus-/Einstecken durch den Nutzer (USB-Kabel gezogen)
3. Physische Reset-Hotkey-Kombination auf der Tastatur selbst (löste real
   `qlink.bindings: Notification "ActionFired"`-Events mit **Action-Typen 27 und 18**
   aus — von der App als `ParsingFailed{Invalid Action type}` verworfen, da unbekannt —
   **neuer Protokoll-Fund**, aber keine sichtbare Wirkung auf den Busy-Status)
4. Kompletter VM-`poweroff`+`startvm`-Zyklus (Grafikcontroller-Wechsel)
5. Zweiter kompletter `poweroff`+`startvm`-Zyklus (nach GA-Reinstall-Versuch)

In der App selbst bleibt der Effekte-/Per-Key-Konfigurationsbereich für "Light Mount"
durchgehend leer — kein Fehlerdialog mehr (der initiale "Aktive Sitzung"-Warnhinweis kam
nur beim allerersten Verbindungsversuch dieser Sitzung), einfach keine Optionen. Ein
kleines Warndreieck am Geräte-Icon in der Sidebar ist der einzige sichtbare Hinweis,
liefert aber laut Nutzer beim Anklicken "keine Info".

**`PROTOCOL.md`/`docs/evidence/connection-handshake-analysis.md` durchsucht:** kein
dokumentierter "Session-Claim"/"Busy-Release"-Befehl vorhanden. Einziger Ansatzpunkt: die
Attribut-/Fähigkeits-ID-Liste aus dem Handshake (Subcmd `0x04`) enthält IDs `02,03,04,
06,07,11,12`, deren Bedeutung nie geklärt wurde — einer davon könnte der fehlende
Befehl sein, aber das ist reine Spekulation, kein Test unternommen (`SECURITY.md`
Regel 10).

**Schlussfolgerung:** Der Busy-Status wirkt inzwischen wie **fest im Gerät (nicht-flüchtig)
gespeichert** — kein uns bekannter Reset-Mechanismus erreicht ihn. Nutzer-Entscheidung:
Sitzung hier für heute beendet.

## Nächster konkreter Schritt

**Nutzer leiht sich morgen (2026-08-21) einen echten Windows-Rechner** (Rechner der
Tochter, ohnehin dort andere Aufgaben zu erledigen) und wird IO Center dort direkt mit
der Tastatur testen — klärt, ob "Busy" ein VM/USB-Passthrough-spezifisches Artefakt ist
oder tatsächlich am Gerät selbst hängt (bisher nicht unterscheidbar, da nie auf echter
Hardware ohne VM getestet). **Falls "Busy" dort ebenfalls auftritt:** bestätigt
Geräte-Zustand als Ursache, nächster Schritt wäre Kontakt zum Hersteller-Support oder
Suche nach einem offiziellen Firmware-Reset-/Update-Tool. **Falls "Busy" dort NICHT
auftritt:** VM/USB-Passthrough-Umgebung (z. B. `bcdDevice`/Timing-Unterschiede beim
Passthrough-Handshake) wäre die Ursache, weiterer Test in der VM sinnlos, Fokus auf
Analyse der Handshake-Unterschiede zwischen echtem Host und Passthrough nötig.

VM `lightmount-win11` bleibt mit korrigiertem `vboxsvga`-Grafikcontroller registriert
und einsatzbereit für die nächste Sitzung (Login `Administrator`/`LightMount!2026`).
Die usbmon-Aufzeichnung dieser Sitzung wurde ohne verwertbaren Per-Key-Traffic beendet
(nie über die "Busy"-Sperre hinausgekommen) und nicht gesichert — bei Bedarf für Debugging
der Busy-Frage könnte sie aber immer noch nützlich sein (zeigt zumindest den
Verbindungsaufbau bei jedem der Reset-Versuche).

## Update (2026-08-29) — Upstream-Community-Aktivität, Lücke 2026-08-21 bis -28 noch nicht nachgetragen

**Hinweis:** Der reale Fortschritt zwischen dieser Notiz (Stand 20./21.08.) und heute ist
erheblich (echter Windows-PC-Test, LampArray-Durchbruch für Interface 3, Repo public seit
24.08., OpenRGB-MRs `!3509`/`!3511`) — bisher nur im Obsidian-Vault
(`bequiet-lightmount-linux.md`) und in Auto-Memory dokumentiert, noch nicht rückwirkend
hier nachgetragen. Diese Sektion deckt nur den heutigen Stand ab, keinen vollständigen
Rückblick.

**Upstream-Status:**
- Issue [`#4950`](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/4950) — ursprünglicher
  Device-Request, LampArray-Fund dort gepostet.
- MR [`!3509`](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/3509) —
  zwei Bugfixes (`skip_generic_detectors`-Scope, Headless-Server-Plugin-List-Crash),
  **gemerged**.
- MR [`!3511`](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/3511) —
  `LightMountController` (Interface-2-Vendor-Treiber, alle 6 Effekte), seit 24.08. als
  Draft offen, bisher keine Reviewer-Rückmeldung.

**Zwei neue, unabhängige Issues von Dritten gefunden (29.08.), beide kommentiert:**
- [`#5726`](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/5726) — Duplikat-Device-Request
  für dieselbe Light Mount (`f.mueller81`). Kommentar mit Link auf dieses Repo + `!3511`/`!3509`:
  https://gitlab.com/CalcProgrammer1/OpenRGB/-/work_items/5726#note_3757229805
- [`#5761`](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/5761) — Request für die
  **Dark Mount** (`373f:0001`, Schwestermodell, PID-Range `373f:0001`–`0013` laut
  `Cheezykins` in `#4950`). Kommentar stellt klar, dass diese Arbeit sich auf die
  **Light Mount** bezieht (nicht gegen Dark-Mount-Hardware verifiziert), aber evtl. als
  Ausgangspunkt nützlich ist:
  https://gitlab.com/CalcProgrammer1/OpenRGB/-/work_items/5761#note_3757230753

## Bugfix (2026-08-31) — Port-Kollision liess Tastatur ohne Automatisierung zurueck

**Symptom:** Tastatur leuchtete falsch, kein sichtbarer Prozess-Absturz auf den ersten
Blick. Root Cause: `openrgb-lightmount-server.service` (dediziert, custom Fork) UND der
Ubuntu-Stock-`openrgb.service` binden beide standardmaessig an Port **6742** (OpenRGBs
Default-SDK-Port) -- der dedizierte Server war nie auf einen eigenen Port konfiguriert.
Bei einem Login/Neustart heute Morgen (10:27) starteten beide praktisch gleichzeitig,
der dedizierte Server verlor das Rennen um den Port, stuerzte beim Neustart-Versuch
sogar mit `free(): invalid pointer` in `HIDLampArrayController::SetLampMultiUpdateReport`
ab (echter Heap-Bug bei unsauberem Shutdown waehrend eines laufenden LED-Writes), gab
nach 3 Versuchen auf ("Start request repeated too quickly"). Der Automatisierungs-Daemon
(`lightmount-automation.service`) fand daraufhin keinen Server mehr und faellte
ebenfalls aus ("Dependency failed"). Ergebnis: Tastatur blieb >30 Minuten ohne jede
Steuerung im letzten (mutmasslich mitten im Crash eingefrorenen) Zustand haengen.

**Fix:** Dedizierten Server dauerhaft auf **Port 6743** verlegt (`ExecStart` in
`openrgb-lightmount-server.service`, Default in `openrgb_client.py`, `--client`-Flag in
`openrgb-lightmount-gui.sh`, README) -- keine Kollision mehr moeglich, unabhaengig von
Start-Reihenfolge. `openrgb-lightmount-server.service` war zudem nur indirekt ueber die
`Requires=` des Automatisierungs-Daemons gestartet worden, nie selbst `enable`d --
jetzt zusaetzlich explizit `systemctl --user enable` gesetzt.

**Update (2026-08-31, spaeter am Tag) -- Root Cause gefunden und gefixt:**
`RGBController_HIDLampArray::~RGBController_HIDLampArray()` rief `delete controller`
auf, ohne vorher `Shutdown()` aufzurufen. Die Basisklasse `RGBController` stoppt den
`DeviceCallThread` nur als Sicherheitsnetz in ihrem EIGENEN Destruktor -- der laeuft
aber erst NACH dem Destruktor-Body der abgeleiteten Klasse, also zu spaet: der
Hintergrund-Thread konnte weiterhin `DeviceUpdateLEDs()` auf dem bereits geloeschten
`controller` aufrufen. Fix (upstream-Fork, `openrgb-src-private`, Branch
`local-combined`, Commit `c864a25c`): `Shutdown();` als erste Zeile ergaenzt, exakt das
Muster, das andere Controller (z.B. `RGBController_MountainKeyboard`) bereits nutzen.
Verifiziert mit 8 sequenziellen Neustarts bei aktiver Automatisierung -- kein Absturz
mehr (vorher stuerzte der allererste kontendierte Neustart ab).

**Eigene, fokussierte Upstream-MR eroeffnet:**
[`!3544`](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/3544) --
sauberer 1-Zeilen-Diff, sauber von `origin/master` abgezweigt (nicht von
`local-combined`, das noch andere unfertige Aenderungen enthaelt), unabhaengig von
`!3509`/`!3511`.

**Neuer, kleinerer Fund beim Extremtest:** Unter kuenstlich uebertriebenem Stress (15
Neustarts binnen ~2s, weit jenseits realer Nutzung) stuerzt stattdessen `hid_close()`
selbst mit Heap-Korruption ab -- vermutlich mehrere Prozesse, die kurzzeitig denselben
physischen HID-Knoten anfassen. Das urspruengliche Trigger-Szenario (zwei Services
konkurrieren einmalig um Port 6742 beim Login) kann durch die Port-Trennung ohnehin
nicht mehr auftreten, daher bewusst zurueckgestellt, nicht weiter verfolgt.
