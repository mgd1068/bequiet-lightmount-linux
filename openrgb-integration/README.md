# OpenRGB-Integration

Dieser Ordner enthält die Dateien für den OpenRGB-Controller (`Controllers/LightMountController/`),
die eines Tages als Merge Request an [OpenRGB](https://gitlab.com/CalcProgrammer1/OpenRGB)
gehen sollen. Sie liegen hier im Hauptrepo (GPL-2.0-or-later, wie OpenRGB selbst),
**nicht** als Git-Submodule des OpenRGB-Quellstands — siehe `DECISIONS.md`.

## Lokal bauen und testen

1. OpenRGB-Quellstand klonen (nicht Teil dieses Repos, git-ignoriert):
   ```
   git clone https://gitlab.com/CalcProgrammer1/OpenRGB.git openrgb-src-private
   ```
2. Diese Controller-Dateien hineinkopieren:
   ```
   cp -r openrgb-integration/Controllers/LightMountController openrgb-src-private/Controllers/
   ```
3. Bauen (siehe `openrgb-src-private/Documentation/Compiling.md` für Abhängigkeiten,
   Debian/Ubuntu: `qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libusb-1.0-0-dev
   libhidapi-dev pkgconf libmbedtls-dev qttools5-dev-tools`):
   ```
   cd openrgb-src-private && mkdir -p build && cd build
   qmake ../OpenRGB.pro && make -j$(nproc)
   ```
   OpenRGB entdeckt neue Controller automatisch per Glob (`Controllers/*.cpp`,
   `Controllers/*.h` in `OpenRGB.pro`) — kein manueller Eintrag nötig.

## Umfang (bewusst minimal)

Nur **eine statische Vollflächenfarbe** ist implementiert — das ist die einzige
Funktion, die dieses Projekt mit einem selbst gewählten (nicht nur aus einem Capture
kopierten) Wert auf echter Hardware verifiziert hat (siehe `PROTOCOL.md`, „Erster
selbst konstruierter Hardwaretest"). Per-Key-Adressierung, Helligkeit und die
eingebauten Effekte (Matrix, Tornado, ColorWave, Breathing, Reactive) existieren laut
Gerätemanifest in der Firmware, aber ihr Byte-Protokoll ist nicht bestätigt — sie werden
bewusst **nicht** geraten implementiert. Siehe `BACKLOG.md` für den Ausbaupfad.

## Bekannte Einschränkung: Sequenznummer funktioniert noch nicht zuverlässig

**Status (2026-08-18): Build und Geräteerkennung funktionieren, der `Static`-Modus
selbst noch nicht zuverlässig.**

- Build getestet: `./openrgb --list-devices` erkennt "be quiet! Light Mount" korrekt
  (richtiger `hidraw`-Pfad, Seriennummer, Modus, Zone).
- Farbtest getestet (`./openrgb --device 0 --mode static --color 8000FF`): **keine
  Wirkung.** Das Gerät validiert Sequenznummern gegen einen internen Zustand (siehe
  `PROTOCOL.md`) — der hier verwendete, bei 1 startende Zähler wird genauso abgelehnt
  wie beliebige hohe Werte (`0x2000`, siehe frühere Tests). Nur eine tatsächlich real
  beobachtete Sequenznummer hat je funktioniert.
- **Nicht gelöst:** wie ein frisch startender Client die aktuell vom Gerät erwartete
  Sequenznummer herausfindet, ohne vorher einen echten Session-Mitschnitt zu haben. Ein
  möglicher, noch nicht verifizierter Ansatz: die Telemetriewerte aus Interface 3
  Report-ID 1 (siehe `PROTOCOL.md`) vor dem ersten Schreibversuch auslesen.
- Bis das gelöst ist, ist dieser Controller **nicht praktisch nutzbar** — er dient als
  verifiziertes Grundgerüst (Detection, Report-Aufbau, CRC), nicht als fertige Lösung.
