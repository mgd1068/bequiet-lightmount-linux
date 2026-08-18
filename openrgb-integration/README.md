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

## Bekannte Einschränkung: Sequenznummer

Das Gerät validiert Sequenznummern für die Static-Color-Kommandofamilie gegen einen
internen Zustand (siehe `PROTOCOL.md`/`DECISIONS.md`). Dieser Controller startet bei
einem kleinen, monoton steigenden Zähler (1, 2, 3, …) — das ist **nicht** gegen echte
Hardware verifiziert (unser einziger erfolgreicher Test nutzte eine reale, zuvor
beobachtete Sequenznummer, keinen frischen Zähler). Ein Hardwaretest mit diesem
Controller-Code steht noch aus, siehe `BACKLOG.md`.
