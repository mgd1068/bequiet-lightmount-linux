# STATE

## Aktuelle Phase

Phase 0 — Bestand und Reproduzierbarkeit.

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

## Nächster konkreter Schritt

1. OpenRGB Issue #4950 auf neue Aktivität/Code prüfen (WebFetch), vorhandenen
   Capture-Anhang aus der Originalquelle laden, offline unter `captures-private/`
   ablegen (git-ignoriert, nicht committen).
2. Lokal installierten OpenRGB-Quellstand bzw. GitLab-Quelle des
   Mountain-Everest-Controllers lesen und strukturell mit den hier gesicherten
   Light-Mount-Deskriptoren vergleichen (Report-ID vorhanden/fehlend, Byte-Layout).
3. Anhand des Web-Captures prüfen, ob Interface-2- oder Interface-3-artige 64-Byte-Reports
   vorkommen und welche Aktionen (Farbe, Helligkeit, Profil) abgedeckt sind.
4. Erste überprüfbare Protokollhypothese für ein konkretes Kommando (z. B. "LED aus")
   formulieren und das sichere Offline-Testgerüst (Dry-Run-CLI, Sprache: C++ zur
   OpenRGB-Kompatibilität, siehe `DECISIONS.md`) beginnen — weiterhin ohne Schreibzugriff
   auf `hidraw`.

## Hypothese / erwartetes Ergebnis / Risiko / Rückfall (für den nächsten Schritt)

- **Hypothese:** Der vorhandene usbmon/Wireshark-Capture aus Issue #4950 enthält 64-Byte-
  Reports, die sich Interface 2 oder Interface 3 zuordnen lassen (Paketlänge, Feature- vs.
  Interrupt-Transfer erkennbar in usbmon-URB-Metadaten).
- **Erwartetes Ergebnis:** Klarheit, welches der beiden Vendor-Interfaces IO Center Web
  tatsächlich für Farbbefehle nutzt, und ob Per-Key-Kommandos im Capture überhaupt
  vorkommen.
- **Sicherheitsrisiko:** keins — reine Offline-Analyse einer bereits vorhandenen,
  fremden Aufzeichnung, kein Gerätezugriff.
- **Rückfall:** falls der Capture-Anhang nicht mehr abrufbar oder keine Farbaktion
  enthalten ist — als Blocker dokumentieren und mit Interface-2-Struktur-Vergleich zum
  Mountain-Controller allein weiterarbeiten, bevor ein eigener (Windows-VM-)Capture-Plan
  nötig wird.

## Blocker

Keiner.
