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
   cd openrgb-src-private
   git apply ../openrgb-integration/HIDLampArrayController-subclass.patch
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

## Umfang (Stand 2026-09-02: kombinierter Controller)

Ein einzelner gerätespezifischer `RGBController_LightMount` besitzt beide HID-Endpunkte:

- `Direct` nutzt HID LampArray auf Interface 3 für 135 einzeln adressierbare Lampen.
- `Static`, `ColorWave`, `Tornado`, `Breathing`, `Reactive` und `Matrix` nutzen das
  Vendor-Protokoll auf Interface 2 und schalten LampArray anschließend in Autonomous.
- Die nicht einzeln adressierbaren Unterseitenlichter sind in den Firmwareeffekten
  enthalten und im LampArray-Direct-Modus hardwarebedingt aus.

Der Detector ist gerätespezifisch auf dem LampArray-Endpunkt registriert, öffnet beide
Interfaces und verhindert damit den generischen zweiten LampArray-Geräteeintrag ohne
Änderung am `DetectionManager`. Mehrere Tastaturen werden über ihre Seriennummern
gepaart; ohne Seriennummer ist die sichere Ein-Gerät-Fallback-Erkennung erlaubt, eine
mehrdeutige Paarung dagegen nicht.

Alle sechs Vendor-Effekte wurden bereits am 2026-08-24 live bestätigt. Der kombinierte
Controller wurde am 2026-09-02 vollständig gegen den aktuellen OpenRGB-Master gebaut
und read-only erkannt. Der lokale Produktivdienst läuft bereits mit dem kombinierten
Direct-Pfad und bestand SDK-/API-Healthchecks; der gezielte Wechsel zu und von einem
Vendor-Firmwareeffekt steht noch aus.

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

## Vendor-Antworten und Zähler

**Gelöst für den häufigsten Fall (2026-08-24):** Auf einer nachweislich
verbindungsfreien Verbindung (keine andere Software spricht mit Interface 2)
akzeptiert das Gerät `counter=0` beim allerersten Schreibversuch — bestätigt live,
siehe `PROTOCOL.md` "Kaltstart-Problem des Zählers gelöst". `LightMountControllerDetect.cpp`
ruft jetzt automatisch `SetCounter(0)` direkt nach der Geräte-Erkennung auf.
**Weiterhin nicht abgesichert:** falls beim Verbindungsaufbau bereits eine andere
Software (z. B. ein offener `iocenter.bequiet.com`-Browser-Tab) verbunden ist, könnte
`counter=0` fälschlich kollidieren. Der Controller prüft nun die bestätigte 64-Byte-
Antwort inklusive CRC, Counter, Marker und Subcommand. Der Zähler wird erst nach einem
gültigen ACK erhöht. Nach Timeout oder Ablehnung wird er als unsynchron markiert und
kein geratenes Retry gesendet. Unabhängige Tastendruck-Telemetrie auf Interface 2 wird
beim Warten auf das ACK ignoriert und niemals protokolliert.

## Kleine Änderung an der LampArray-Basis

`HIDLampArrayController-subclass.patch` macht den Destruktor virtuell, initialisiert
Report-IDs deterministisch und liefert die HID-Schreibergebnisse zurück. Das ist nötig,
damit der Light-Mount-Controller die LampArray-Basis sicher ableiten und Moduswechsel
auf Fehler prüfen kann. Bestehende LampArray-Aufrufer bleiben quellkompatibel.
