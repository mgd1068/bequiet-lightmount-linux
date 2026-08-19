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

## Bekannte Einschränkung: Zähler-Kaltstart ungelöst

**Status (2026-08-19): Build und Geräteerkennung funktionieren. Das Feld, das bis
2026-08-18 als "16-Bit-Sequenznummer" bezeichnet wurde, ist tatsächlich zwei getrennte
Bytes — ein 1-Byte-Zähler (Byte 4) und ein separates Marker-Byte (Byte 5, konstant
`0x10` für Static-Color). Sobald der Zähler korrekt an den echten Gerätezustand
anschließt, funktioniert einfaches Weiterzählen zuverlässig (bestätigt mit zwei
aufeinanderfolgenden echten Hardwaretests, siehe `PROTOCOL.md`
„Verbindungsaufbau-Capture").**

- `LightMountController` verlangt jetzt explizit `SetCounter()`, bevor
  `SendStaticColor()` überhaupt schreibt (`IsCounterPrimed()` davor false) — der
  Controller **rät den Startwert nicht mehr**, weil das nachweislich nicht funktioniert
  (weder `1` noch `0x2000` wurden vom Gerät akzeptiert).
- **Weiterhin ungelöst:** wie `SetCounter()` ohne einen frischen Live-Capture-Wert
  automatisch befüllt werden kann. Der Zähler scheint einen längerfristigen, aber nicht
  unbegrenzt persistenten Gerätezustand fortzusetzen (Details, inkl. einer
  vielversprechenden, aber ungetesteten Hypothese zu einem möglichen
  Timeout-/Reset-Verhalten: siehe `PROTOCOL.md`).
- Bis das gelöst ist, ist dieser Controller **nur mit manuell eingespeistem
  Startzähler nutzbar** (z. B. aus einem frischen `usbmon`-Mitschnitt) — er dient als
  verifiziertes, korrektes Grundgerüst, nicht als für Endnutzer fertige Lösung.
