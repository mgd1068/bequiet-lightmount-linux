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

## Umfang (Stand 2026-08-24: alle 6 Effekte implementiert)

Alle 6 Firmware-Effekte (Static, ColorWave, Tornado, Breathing, Reactive, Matrix) sind
implementiert und live gegen echte Hardware über den vollständigen OpenRGB-Stack
getestet (SDK-Server → Client → Controller → Gerät), inkl. Helligkeit, Tempo und
wählbarer Farben. Grundlage: vollständige Protokoll-Reverse-Engineering-Arbeit vom
2026-08-24, siehe `PROTOCOL.md` ("Alle 6 Effekte entschlüsselt...", "...Parameter
systematisch entschlüsselt..."). Nur **Whole-Keyboard**-Steuerung — dieser Kanal
(Interface 2) ist nachweislich global/synchron, kein Per-Key. Echtes Per-Key läuft
über den separaten, bereits fertigen generischen `HIDLampArrayController` auf
Interface 3 desselben Geräts (siehe `docs/evidence/lamp_id_key_mapping.json`).

**Byte-Layout** (alle Effekte außer Static teilen sich ein Schema): `[Effekt-Typ]
[Richtung][Helligkeit][Tempo][Farbanzahl-Modus]` gefolgt von 1/2 direkten RGB-Werten
oder (3+ Farben) `[Keyframe-Anzahl]` + N×`[R,G,B,Prozent]`. Die Report-Längenfeld-
Formel wurde am 2026-08-24 erstmals abgeleitet (vorher nie bekannt): `Länge =
Payload-Bytes + 7`, bestätigt gegen 4 unabhängige echte Captures.

**Bekannte, dokumentierte Design-Entscheidungen** (siehe Code-Kommentare in
`LightMountController.h`/`RGBController_LightMount.cpp` für Details):
- ColorWave hat 4 Richtungen (oben/unten/links/rechts, kombinierte
  `HAS_DIRECTION_LR|UD`-Flags), Tornado nur 2 (im/gegen Uhrzeigersinn, `LR`
  zweckentfremdet mangels passendem OpenRGB-Flag).
- Bei Reactive ist das "Tempo"-Byte tatsächlich die Abklingzeit — live bestätigt.
- Matrix' Richtungs-Byte ist auf den einzigen je beobachteten Wert (`0x01`) fest
  codiert, nicht nutzersteuerbar (keine anderen Werte je getestet).
- Farbanzahl-Obergrenze `8` ist ein dokumentiertes, ungetestetes Limit (nur 4 und 7
  Keyframes live beobachtet, Payload-Puffer erlaubt bis 12).
- **Offene Beobachtung, nicht behoben:** Beim eigenen `--color A,B`-CLI-Flag kamen
  Basis-/Trigger-Rolle bei Reactive vertauscht heraus gegenüber dem, was eingetippt
  wurde — nicht weiter verfolgt, da unklar ob CLI-spezifische Listen-Reihenfolge oder
  auch GUI-relevant. Der Payload-Byte-Order selbst folgt dem echten, bestätigten
  Geräteverhalten, wurde nicht zur Kompensation geändert.

## Bekannte Einschränkung: Zähler-Kaltstart

**Gelöst für den häufigsten Fall (2026-08-24):** Auf einer nachweislich
verbindungsfreien Verbindung (keine andere Software spricht mit Interface 2)
akzeptiert das Gerät `counter=0` beim allerersten Schreibversuch — bestätigt live,
siehe `PROTOCOL.md` "Kaltstart-Problem des Zählers gelöst". `LightMountControllerDetect.cpp`
ruft jetzt automatisch `SetCounter(0)` direkt nach der Geräte-Erkennung auf.
**Weiterhin nicht abgesichert:** falls beim Verbindungsaufbau bereits eine andere
Software (z. B. ein offener `iocenter.bequiet.com`-Browser-Tab) verbunden ist, könnte
`counter=0` fälschlich kollidieren — dafür gibt es weiterhin keine Erkennung ohne
Live-Capture. `SetCounter()`/`IsCounterPrimed()` bleiben öffentlich, falls ein Aufrufer
einen echten, beobachteten Zählerstand manuell einspeisen möchte.
