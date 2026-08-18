# Research Sources

Primärquellen, wie im Master-Prompt (`LIGHTMOUNT_AGENT_LOOP_PROMPT.md`) benannt.

## OpenRGB

- [OpenRGB Issue #4950 – New Device: be quiet! Light Mount Keyboard](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/4950)
  — enthält USB-Deskriptoren und einen usbmon/Wireshark-Mitschnitt von IO-Center-Web-Aktionen.
- [OpenRGB Controllers-Verzeichnis](https://gitlab.com/CalcProgrammer1/OpenRGB/-/tree/master/Controllers)
- [MountainKeyboardController.cpp](https://gitlab.com/CalcProgrammer1/OpenRGB/-/raw/master/Controllers/MountainKeyboardController/MountainKeyboardController.cpp)
- [MountainKeyboardController.h](https://gitlab.com/CalcProgrammer1/OpenRGB/-/raw/master/Controllers/MountainKeyboardController/MountainKeyboardController.h)
- [MountainKeyboardControllerDetect.cpp](https://gitlab.com/CalcProgrammer1/OpenRGB/-/raw/master/Controllers/MountainKeyboardController/MountainKeyboardControllerDetect.cpp)
- [RGBController_MountainKeyboard.cpp](https://gitlab.com/CalcProgrammer1/OpenRGB/-/raw/master/Controllers/MountainKeyboardController/RGBController_MountainKeyboard.cpp)
- [OpenRGB SDK](https://openrgb.org/sdk.html)
- [OpenRGB Scheduler Plugin](https://openrgb.org/plugin_scheduler.html)
- [OpenRGB Hardware Sync Plugin](https://openrgb.org/plugin_hardware_sync.html)
- [OpenRGB Effects Plugin](https://openrgb.org/plugin_effects.html)

## Hersteller

- [be quiet! Light Mount Silent Linear DE](https://www.bequiet.com/de/keyboards/5650)
- [be quiet! IO Center](https://www.bequiet.com/en/software)
- [be quiet! IO-Center-Tutorial](https://www.bequiet.com/de/software/tutorial)
- [IO Center Web](https://iocenter.bequiet.com/)
- IO Center Windows-Installer (`IO Center Installer.exe`, Version 1.2.0, Inno Setup
  6.3.0) — vom Nutzer manuell heruntergeladen (bequiet.com ist per Cloudflare
  bot-geschützt, kein automatisierter Zugriff möglich), statisch analysiert, siehe
  `docs/evidence/windows-app-static-analysis.md`. Datei selbst nicht im Repo (Rohdaten
  proprietär, siehe `DECISIONS.md`).

## Community

- [be-quiet!-Communitybericht mit USB-Deskriptoren und Reset-Reproduktion](https://www.reddit.com/r/bequietofficial/comments/1lwl9gf/just_got_my_light_mount_with_the_linear_silent/)

## Web-HID-Referenz

- [Chrome WebHID-Dokumentation](https://developer.chrome.com/docs/capabilities/hid)
- [MDN: HIDDevice.sendReport](https://developer.mozilla.org/en-US/docs/Web/API/HIDDevice/sendReport)

Herstellerdokumentation bestätigt Per-Key-ARGB und Onboard-Speicher, garantiert aber
nicht identischen Funktionsumfang zwischen Web- und Windows-Client — tatsächlich
erreichbarer Umfang wird durch Messung ermittelt, nicht durch Marketingtexte.
