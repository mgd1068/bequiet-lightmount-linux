# BACKLOG

Offene Arbeitspakete, grob nach Phase. Nicht priorisiert innerhalb einer Phase — siehe
`STATE.md` für den jeweils nächsten konkreten Schritt.

## Phase 0

- [x] System/Kernel/OpenRGB-Version/USB-Topologie erfassen (2026-08-18)
- [x] sysfs-HID-Report-Deskriptoren der vier Interfaces sichern (2026-08-18, `docs/evidence/`)
- [x] OpenRGB Issue #4950 auf neue Aktivität prüfen, Capture-Anhang laden (2026-08-18, keine neue Aktivität, Capture analysiert)
- [ ] Baseline dokumentieren: aktuelles Profil, sichtbare Beleuchtung, Verhalten nach USB-Reconnect
- [ ] Interface-3-Report-Struktur (Vendor Page 0x59, 6 Report-IDs) genauer aufschlüsseln
- [x] Checksum-Algorithmus der letzten 2 Report-Byte offline gegen alle 20 bekannten usbmon3-Frames verifizieren (2026-08-18, CRC16/MODBUS bestätigt, 20/20)
- [ ] Verbleibende unentschlüsselte usbmon3-Kommandos (15/18/7 Byte) weiter eingrenzen,
      siehe `docs/evidence/usbmon3_decoded_commands.txt` (29-Byte-Kommando als
      Matrix-Effekt identifiziert, 2026-08-18, siehe `PROTOCOL.md`)
- [ ] Weitere Effekte (Tornado, ...) gezielt capturen und Payload-Byte-Bedeutung für
      Matrix genauer eingrenzen (Farben/Geschwindigkeit/Richtung vermutet, nicht bestätigt)
- [x] Mountain-Everest-Referenzcode besorgt und strukturell verglichen (2026-08-18, Protokollverwandtschaft widerlegt, siehe `PROTOCOL.md`)

## Phase 1

- [x] CRC16/MODBUS + Report-Parsing/-Aufbau in C++ implementiert und gegen alle 20
      bekannten usbmon3-Fixtures getestet (2026-08-18, `src/protocol/`, `tests/`)
- [x] Dry-Run-CLI (`report_dump`) mit strukturierten Hex-Dumps, kein `hidraw`-Zugriff (2026-08-18)
- [x] `report_build`-Tool: Reports aus einzelnen Feldern bauen statt Python-Einwegskripte
      (2026-08-18, `session` als echtes Feld in `Interface2Report` ergänzt, per Pipe mit
      `report_dump`/`report_send` kombinierbar)
- [ ] PCAP-Parser/Wireshark-Auswertung reproduzierbar machen
- [ ] IO-Center-Web-Aktionen den HID-Paketen zuordnen
- [x] Prüfen ob Web-Capture Per-Key-Kommandos enthält (2026-08-18: bestätigt NEIN — IO
      Center Web bietet nur vorgeformte Effekte, keine Einzeltasten-UI, Nutzer hat UI
      direkt geprüft; siehe `PROTOCOL.md`)
- [x] Windows-App statisch analysiert (2026-08-18, ohne VM/Ausführung) — vollständiges
      Gerätemanifest + 168-LED-Tabelle gefunden, siehe `PROTOCOL.md`/
      `docs/evidence/windows-app-static-analysis.md`. VM-Capture-Plan damit vorerst
      nicht mehr nötig — falls das Wire-Kommando für Per-Key doch noch per Live-Capture
      gebraucht wird, bleibt das als Option offen.
- [ ] Wire-Kommando für Per-Key-Adressierung finden (Manifest kennt nur die 168
      LED-Namen/Indizes, nicht das Byte-Protokoll dafür — Interface 3 Report ID 4
      als Kandidat identifiziert, aber `GET_FEATURE` liefert nur Nullen, siehe
      `PROTOCOL.md`; noch kein Schreibversuch, kein bekanntes Kommando als Basis)
- [ ] Bedeutung der Interface-3-Report-ID-1/3-Telemetriewerte klären (2026-08-18
      per `GET_FEATURE` ausgelesen, nicht interpretiert)
- [x] `light_mount_main_iso.json` (Pixel-Koordinaten pro Taste) mit den LED-Indizes
      verknüpft (2026-08-18, `docs/evidence/light_mount_led_layout_iso.json`, 166
      Einträge, 111 mit Geometrie)
- [ ] Geometrie für die 55 Leisten-LEDs (Top/Left/Right, aktuell `null`) aus den noch
      unausgewerteten `boundingRect`-Dateien ergänzen
- [x] Mountain- vs. Light-Mount-Pakete strukturell vergleichen (2026-08-18, siehe Phase 0, widerlegt)
- [x] Offline-Tests mit gespeicherten Paketen (ohne Hardware) (2026-08-18, `tests/test_protocol.cpp`, 20/20 Fixtures)

## Phase 2 (RGB-MVP)

- [x] Geräteerkennung/-öffnen (2026-08-18, `report_send`, exakt Interface 2 via sysfs-ermitteltem `hidraw`-Pfad)
- [x] Statische Gesamtfarbe (2026-08-18, Frame 2747 live getestet: statisches Orange,
      aber feste Preset-Farbe, keine frei wählbare RGB-Eingabe bestätigt — siehe `PROTOCOL.md`)
- [ ] Off-Kommando identifizieren/verifizieren (USB-Replug ist KEIN verlässlicher Reset,
      physischer Hotkey ist der bisher einzige bekannte verlässliche Rückfall — siehe `SECURITY.md`)
- [x] Farbcodierung für frei wählbare (nicht Preset-)Farben identifizieren (2026-08-18,
      eigener Capture, exakter Treffer gegen `#1FB4FF`, siehe `PROTOCOL.md`)
- [x] Static-Color-Kommando (Länge 15, Subcmd `0x06`) real auf Hardware getestet
      (2026-08-18, `#00FF00` selbst konstruiert und erfolgreich angewendet — siehe
      `docs/evidence/sequence-number-validation-test.md`)
- [ ] Sequenznummer-Akzeptanzregel klären (Toleranzfenster? exakte Fortsetzung? nur
      diese Kommandofamilie betroffen?) — kritisch für Phase 3 (`DECISIONS.md`)
- [ ] Bedeutung von Payload-Byte 11 (`0x32`) im Static-Color-Kommando klären
- [ ] Push-Frame-Mechanismus (Subcmd `0x02`, Tastendruck-Spiegelung) weiter untersuchen (Flags-Byte-Bedeutung)
- [ ] Ablehnungs-Antwortformat (Byte 3 = `0x0a` bei Fehler) an weiteren Fällen verifizieren
- [x] Interface 3: Interrupt-IN-Lesetest + minimaler No-Op-`SET_FEATURE`-Test (2026-08-19,
      beide risikofrei, keine neuen Erkenntnisse zur Bedeutung — siehe `PROTOCOL.md`)
- [ ] Direct Mode
- [x] Erste einzelne Taste unabhängig einfärben (2026-08-23, HID-LampArray-Weg über
      Interface 3, nicht der Vendor-Weg über Interface 2 — siehe Phase-3-Eintrag unten.
      `tools/lamp_array_control.py set-one 0 255 0 255` live getestet: Lamp-ID `0` =
      ESC-Taste, magenta, vom Nutzer visuell bestätigt. Autonomous Mode danach
      wiederhergestellt.)
- [x] ≥2 Tasten gleichzeitig unabhängig einfärben (2026-08-24, ein Multi-Update-Report
      mit 3 Lampen: ID 0=ESC rot, ID 133=ISO-`<>|`-Taste grün, ID 134=ISO-`#'`-Taste
      blau — alle drei vom Nutzer live und korrekt bestätigt, per Cross-Check gegen die
      `input_binding`-Werte aus dem Probe. `tools/lamp_array_control.py set-many` neu
      dafür gebaut, `lamp_id:r:g:b`-Syntax, bis zu 8 Lampen pro Report.)
- [x] Sonderelemente identifiziert (2026-08-24, live + Koordinaten-Abgleich mit
      Vendor-Tabelle): M1-M5 = Lamp 16-20 (Gerät hat 5 Makrotasten, nicht 3 wie erst
      angenommen), Lautstärke-Rad = Lamp 109, Lichtleiste oben = Lamp 110-132 (23
      Segmente, 110=links, 132=rechts). Siehe `STATE.md` für die vollständige Tabelle.
- [x] Lamp-ID→Taste-Mapping für die komplette Hauptmatrix (2026-08-24, alle 6 Zeilen
      live zeilenweise verifiziert, inkl. Numpad — Gerät hat einen vollständigen
      Nummernblock, war vorher nicht bekannt). Vollständige Tabelle:
      `docs/evidence/lamp_id_key_mapping.json`. Lamp `55` bleibt nachweislich ohne
      sichtbare LED (isoliert zweifach getestet), Lamp `100` korrigiert von "Enter"
      (Vorhersagefehler) zu "RightAlt", echtes Enter ist Lamp `75`.
- [x] 21 mittlere Lichtleisten-Segmente (Lamp `111`-`131`) einzeln live getestet
      (2026-08-24, alle 23 Segmente 110-132 gleichzeitig unterschiedlich eingefärbt) —
      keine toten Segmente, volle Breite bunt, aber optisch verschwimmen die Farben zu
      einem Verlauf (Diffusor) statt scharf getrennt zu bleiben. Elektrisch pro Segment
      adressierbar bestätigt.
- [x] M1/M2/M3 (Lamp `16`-`18`) tatsächlich live getestet (2026-08-24) — vorher nur aus
      dem M4/M5-Muster abgeleitet, jetzt direkt bestätigt.
- [x] Die zwei kleinen Lichter an der Unterseite außen (2026-08-24): kein Lamp-ID-
      Kandidat unter den 135 LampArray-Lampen (Interface 3) — aber live bestätigt, dass
      sie auf das globale Static-Color-Kommando auf Interface 2 reagieren (Rot, dann
      Magenta, beides vom Nutzer inkl. dieser Lichter bestätigt). RGB-fähig, aber
      aktuell nur global mitgesteuert, nicht individuell adressierbar.
- [x] **Kaltstart-Problem des Sequenzzählers gelöst (2026-08-24):** bei nachweislich
      verbindungsfreiem Zustand wird `counter=0x00` als erster Versuch akzeptiert.
      Siehe `PROTOCOL.md`. Löst den seit Iteration 19 offenen Blocker für Phase 3.
- [ ] Per-LED-Wire-Kommando für Interface 2 (Einzelansteuerung der Unterseiten-/
      Seiten-Lichter sowie generell Per-Key über den Vendor-Kanal) — nach gründlicher
      Prüfung (2026-08-24) aktuell keine Evidenz, dass es überhaupt existiert: IO Center
      Web hat nie eine Einzelelement-UI, kein Capture zeigt je ein indiziertes
      Kommando, und der Interface-3-Deskriptor hat nachweislich keine zusätzlichen
      Lampen außerhalb der bekannten 135. Vermutlich echte Firmware-Grenze, nicht nur
      Wissenslücke. Nicht weiter raten (Projektprinzip) — nur bei neuer Evidenz (z. B.
      Firmware-Update, Hersteller-Support-Auskunft) wieder aufgreifen.
- [x] **Alle 6 Effekte entschlüsselt und live bestätigt (2026-08-24, zwei Etappen).**
      Byte0/Byte1: Static=`00`, ColorWave=`01`/`03`, Tornado=`02`/`04`,
      Breathing=`03`/`00`, Reactive=`04`, Matrix=`05`. Etappe 1 (Zeit-Zyklus über 7
      Keyframes, alle LEDs synchron) war fälschlich als "ColorWave" dokumentiert —
      per echtem Web-Client-Live-Capture (WebHID, ohne VM) korrigiert: das ist
      Breathing. Tornado (räumlich rotierend) und Reactive (Tastendruck-getriggert,
      erklärt rückwirkend Frame 2747 aus Iteration 1) neu entschlüsselt. Siehe
      `PROTOCOL.md`, Rohauszug
      `docs/evidence/own_capture_tornado_breathing_colorwave_reactive_raw.txt`.
- [x] **Effekt-Parameter vollständig entschlüsselt (2026-08-24, gezielter
      Ein-Parameter-Test).** ColorWave komplett live durchgetestet (Richtung 0-3,
      Tempo, Farbanzahl-Modi 1/2/3+), Tornado/Breathing/Reactive/Matrix je per
      Stichprobe bestätigt — ein gemeinsames Payload-Schema für alle 5
      nicht-statischen Effekte (Byte1=Richtung, Byte2=Helligkeit, Byte3=Tempo,
      Byte4=Farbanzahl-Modus, Byte5=Keyframe-Anzahl). Bei Reactive ist Byte3 die
      Abklingzeit, nicht die reine "Geschwindigkeit". Siehe `PROTOCOL.md`,
      Rohauszüge `docs/evidence/own_capture_static_colorwave_parameters_raw.txt` +
      `docs/evidence/own_capture_tornado_breathing_reactive_matrix_parameters_raw.txt`.
      Damit ist die Byte-Parameter-Grundlage für eine vollständige OpenRGB-
      Effekt-Integration gelegt (nächster Schritt: `LightMountController`
      implementieren, siehe Eintrag oben).
- [x] **Architektur-Erkenntnis (2026-08-24):** Interface 2 (global) und Interface 3
      (LampArray) bilden einen sauberen Zwei-Schichten-Anzeigeumschalter über
      LampArrays Autonomous-Flag, keine chaotische gegenseitige Überschreibung — siehe
      `PROTOCOL.md`. Interface-2-Kommandos werden immer gespeichert, aber nur sichtbar
      im Autonomous Mode; LampArray-Direct-Mode überdeckt alles (inkl. Seiten-Lichter,
      die dabei aus sind). Damit sind "alle Tasten einzeln" und "Seiten-Lichter an"
      weiterhin nicht gleichzeitig sichtbar, aber der Mechanismus ist jetzt klar
      verstanden und für Phase-3-Architekturentscheidung nutzbar (`DECISIONS.md`).
- [ ] Vollständige Tastenmatrix/LED-Reihenfolge bestimmen
- [ ] Obere/seitliche Leisten getrennt adressieren
- [ ] Reconnect nach USB-Reset (noch nicht getestet — beim bisherigen Test kein Reset aufgetreten)

## Phase 3 (OpenRGB-Integration)

- [x] Idiomatischer OpenRGB-Controller-Grundgerüst (2026-08-18,
      `openrgb-integration/Controllers/LightMountController/`) — bewusst minimal:
      nur statische Vollflächenfarbe (einzige mit selbst gewähltem Wert verifizierte
      Funktion), Detection über Interface/UsagePage/Usage, ein Zone/ein LED
- [x] Lokaler Build gegen echten OpenRGB-Quellstand verifiziert (2026-08-18, fehlerfrei
      kompiliert, `./openrgb --list-devices` erkennt "be quiet! Light Mount" korrekt
      mit richtigem `hidraw10`, Seriennummer, Static-Modus, Keyboard-Zone/LED)
- [x] Hardwaretest des neuen Controller-Codes (2026-08-18): Sequenznummer-Start-bei-1
      **widerlegt** — genauso abgelehnt wie `0x2000`. Static-Modus funktioniert noch
      nicht zuverlässig, siehe `PROTOCOL.md`/`openrgb-integration/README.md`.
- [x] Sequenznummer-Feld korrekt verstanden (2026-08-19): Byte 4 = fortlaufender
      1-Byte-Zähler, Byte 5 = separates Marker-Feld (nicht Zähler-Hochbyte). Kontinuität
      bestätigt mit zwei echten, aufeinanderfolgenden Hardwaretests. `report.h`/`.cpp`,
      CLI-Tools und OpenRGB-Controller entsprechend korrigiert.
- [ ] **Kritisch, weiterhin ungelöst:** Kaltstart-Problem — wie lernt ein frisch
      startender Client den aktuell gültigen Zählerstand ohne Live-Capture? Zähler
      scheint nach einiger Zeit/Inaktivität zurückgesetzt zu werden (heute bei `02`
      begonnen, obwohl gestern bei `~0xad`/`~0x11` endend, ohne erkennbare
      Neuenumerierung) — Trigger unklar (Timeout? Standby?). Vielversprechende, noch
      nicht getestete Hypothese: unmittelbar nach einem solchen Reset akzeptiert das
      Gerät evtl. (fast) jeden Startwert als neue Basis, d. h. das eigentliche Problem
      wäre "unsere eigenen Testversuche konkurrierten mit einer noch aktiven
      Browser-Session", nicht "der Zähler ist grundsätzlich unbekannbar" — siehe
      `docs/evidence/connection-handshake-analysis.md`. Interface-3-Report-ID-1-
      Telemetrie als Quelle bereits geprüft und verworfen (2026-08-18).
- [ ] Identifikations-Handshake (Byte2-3=`0x0000`, 6 Kommandos beim Verbindungsaufbau)
      weiter entschlüsseln — Geräte-Seriennummer-String und Fähigkeitenliste teilweise
      erkannt, nicht vollständig gedeutet, siehe `docs/evidence/connection-handshake-analysis.md`
- [x] `Shutdown()`-Aufruf im `RGBController_LightMount`-Destruktor nachgerüstet
      (2026-08-18, verhinderte OpenRGB-Warnung beim Beenden)
- [x] HID-LampArray-Weg (Interface 3) als eigentlicher Per-Key-Kanal identifiziert und
      live bestätigt (2026-08-23) — macht das gesuchte "Wire-Kommando" auf Interface 2
      obsolet, siehe `docs/evidence/`. OpenRGB bringt mit `HIDLampArrayController`
      bereits einen generischen Controller mit; Detection-Konflikt mit dem eigenen
      `373f:0002`-Vendor-Detector gefunden (skip_generic_detectors blockiert ihn) und
      per temporärer Detector-Config umgangen.
- [ ] Per-Key-Adressierung: `LightMountController` (Vendor/Interface 2) vs. generischer
      `HIDLampArrayController` (Interface 3) — Architekturentscheidung nötig, ob Vendor-
      Controller für Per-Key erweitert oder ganz auf LampArray umgestiegen wird. Falls
      LampArray: Detection-Konflikt (`skip_generic_detectors`) muss sauber gelöst werden,
      nicht nur per temporärer Config umgangen.
- [x] Vollständige Lamp-ID→Taste-Zuordnung — erledigt (2026-08-24), nicht wie ursprünglich
      geplant über `input_binding` (nur 3 von 135 Lampen gesetzt), sondern per
      Koordinaten-Transformation + zeilenweiser Live-Verifikation. Siehe
      `docs/evidence/lamp_id_key_mapping.json`.
- [ ] Effekte (Matrix, Tornado, ColorWave, Breathing, Reactive) in den OpenRGB-
      Controller integrieren — Byte-Parameter jetzt für alle 6 bekannt (2026-08-24,
      siehe `PROTOCOL.md`), Implementierung im `LightMountController` noch offen
- [ ] udev-Regeln mit minimalen Rechten
- [x] **Repo veröffentlicht (2026-08-24):** GitHub-Repo auf public gestellt
      (https://github.com/mgd1068/bequiet-lightmount-linux), nachdem zwei echte
      Geräte-Seriennummern in committeten Evidence-Dateien redigiert wurden
      (`docs/evidence/own_capture_connect_raw.txt`,
      `connection-handshake-analysis.md`, `lsusb_v.txt`, `STATE.md`). Mapping-Datei
      zusätzlich als öffentliches GitLab-Snippet geteilt
      (https://gitlab.com/-/snippets/6041483). Kommentar mit den Kernfunden (LampArray-
      Weg, `skip_generic_detectors`-Bug) auf GitLab-Issue #4950 gepostet.
- [x] **`skip_generic_detectors`-Bug echt gefixt und lokal verifiziert (2026-08-24):**
      `openrgb-src-private/DetectionManager.cpp`, `RunHIDDetector` +
      `RunHIDWrappedDetector` — `skip_generic_detectors=true` wird jetzt erst nach
      erfolgreichem `compare()` gesetzt, nicht mehr schon bei grobem
      `matching_id()`-Treffer. Verifiziert: unser Vendor-Detector (Interface 2) UND
      der generische LampArray-Detector (Interface 3) laufen jetzt gleichzeitig ohne
      Workaround-Config. Als eigener Commit `1443577` im `openrgb-src-private`-Checkout
      (eigenes Git, `origin` = echtes OpenRGB-Repo) — noch nicht gepusht/als MR
      eingereicht, das ist der nächste Schritt.
- [x] **Zweiter, unabhängig gefundener Bug: `NetworkServer::SendReply_PluginList`
      Nullpointer-Crash (2026-08-24).** `plugin_manager` ist im headless
      `--server`-Betrieb (ohne `--gui`) immer `nullptr` — jeder SDK-Client-Connect
      (u. a. `openrgb-python`) crashte den kompletten Server (SIGSEGV) beim
      Plugin-Listen-Request. Gefunden via `coredumpctl`/`gdb`-Backtrace, gefixt (0
      Plugins statt Crash bei fehlendem `plugin_manager`), verifiziert (Server bleibt
      stabil, Bulk-Update von 135 Lampen in <1ms). Commit `243f1f6`.
- [x] **Fixes propagiert (2026-08-24):** Patch-Dateien im GitHub-Repo
      (`openrgb-patches/`), Branch `fix/lightmount-lamparray-detection` auf GitLab-
      Fork `gitlab.com/mgd681/OpenRGB`, **MR gegen `CalcProgrammer1/OpenRGB` offen:
      https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/3509**. Ähnliche
      offene Issues zur Beobachtung: #5505 (ASUS TUF A18), #2811 (ASUS ROG Strix
      Multi-Device).
- [x] **Dedizierter Server zeigt jetzt alle Geräte (2026-08-24):** Detector-Sperre für
      Apex3/Ironclaw aus `~/.config/openrgb-lightmount/OpenRGB.json` entfernt (Datei
      wird von OpenRGB selbst beim Start auf alle ~2000 bekannten Detectoren
      expandiert — nur die zwei `false`-Einträge mussten geändert werden). GUI zeigt
      jetzt Light Mount + Apex3 + Ironclaw gemeinsam. Neustart mit voller Erkennung
      hat Apex3/Ironclaw erwartungsgemäß kurz in Direct Mode geschaltet, sofort mit
      dem bekannten Fix behoben, vom Nutzer bestätigt.

## Phase 4

- [ ] Hardwareeffekte, Helligkeit, Profilwechsel
- [ ] Kontrolliertes Onboard-Speichern
- [ ] Medienrad, Makrotasten, Key Remapping
- [ ] Rücklesen vorhandener Konfiguration (falls sicher möglich)

## Phase 5

- [ ] OpenRGB-SDK-Client / kleiner User-Daemon
- [ ] CLI + stabile lokale API (D-Bus/Unix-Socket)
- [ ] Zeitgesteuerte Profile, Systemzustände
- [ ] Freedesktop/KDE-Benachrichtigungsbeobachtung ohne Inhaltsspeicherung

## Phase 6

- [ ] GUI nur für nicht durch OpenRGB abgedeckte Bedienung
- [ ] Debian-Paket / reproduzierbare Installation
- [ ] systemd-User-Service, vollständige Deinstallation
