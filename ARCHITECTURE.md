# ARCHITECTURE

## Zielarchitektur

```
┌─────────────────────────┐
│  Notification-Client     │  Phase 5: Freedesktop/KDE-Benachrichtigungen,
│  (kleiner User-Daemon)   │  Systemzustände, Zeitsteuerung
└───────────┬──────────────┘
            │ OpenRGB SDK / D-Bus / Unix-Socket
┌───────────▼──────────────┐
│  OpenRGB                 │  LED-Modelle, Zonen, Direct Mode, Profile,
│  (LightMountController)  │  Scheduler-Plugin, Hardware-Sync-Plugin
└───────────┬──────────────┘
            │ hidraw (exakte VID/PID/Interface/Usage-Auswahl)
┌───────────▼──────────────┐
│  be quiet! Light Mount    │  USB 1.1, 4 HID-Interfaces, Interface 2
│  (373f:0002)              │  vermutlich Konfigurationskanal (0x83 IN / 0x04 OUT)
└───────────────────────────┘
```

Grundprinzip: Hardwareprotokoll und LED-Topologie leben in OpenRGB (`Controllers/LightMountController/`,
analog zu `Controllers/MountainKeyboardController/`). Anwendungsspezifische Logik
(Benachrichtigungen, Zeitsteuerung) ist ein dünner Client des OpenRGB-SDK, nicht Teil
des Hardwaretreibers. Eine eigene GUI/ein eigener Daemon wird nur für das gebaut, was
OpenRGB nicht abdeckt (Phase 5/6).

## Phasenplan

Siehe `STATE.md` für den aktuellen Stand. Kurzfassung:

- **Phase 0** — Bestand/Reproduzierbarkeit (System, Deskriptoren, Baseline)
- **Phase 1** — Sicheres Protokolllabor (Dry-Run-CLI, PCAP-Analyse, Hypothesen)
- **Phase 2** — RGB-MVP (Erkennung, statische Farbe, Direct Mode, ≥2 Einzeltasten, volle Matrix)
- **Phase 3** — OpenRGB-Integration (idiomatischer Controller, udev-Regeln, Upstream-Patch-Vorbereitung)
- **Phase 4** — Übriger Funktionsumfang (Effekte, Profile, Onboard-Speicher, Remapping, Makros)
- **Phase 5** — Linux-Automatisierung (SDK-Client, D-Bus/Socket, Notification-Bridge)
- **Phase 6** — Optionale Oberfläche und Paketierung (Debian-Paket, systemd-User-Service)

## Referenzcode

Mountain Everest gilt als wahrscheinlichste Protokollverwandtschaft (be quiet! hat
Mountain übernommen), beweist aber keine Protokollidentität:

- `Controllers/MountainKeyboardController/MountainKeyboardController.{cpp,h}`
- `Controllers/MountainKeyboardController/MountainKeyboardControllerDetect.cpp`
- `Controllers/MountainKeyboardController/RGBController_MountainKeyboard.cpp`

Bekannte Mountain-Kommandofamilien (**nur Referenzhypothesen**, bis Light-Mount-Trace
sie bestätigt): `0x14 0x2c` (Farbdaten), `0x14 0x2d` (Edge-Daten), `0x14 0xa0`
(Abschluss/Bestätigung), `0x13 ... 0x55` (Speichern).

## Fremdcode-Herkunft

Übernommener Code aus OpenRGB oder anderen GPL-kompatiblen Quellen wird mit Herkunft
und vorgenommenen Änderungen dokumentiert (siehe `DECISIONS.md`), nicht stillschweigend kopiert.
