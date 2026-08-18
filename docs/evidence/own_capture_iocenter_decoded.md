# Eigener Capture: IO Center Web, freie Farbwahl (2026-08-18)

Eigener Mitschnitt (nicht aus dem OpenRGB-Ticket), aufgenommen während einer bewussten,
einzelnen Aktion: Nutzer öffnet iocenter.bequiet.com, wählt über einen Farbwähler die
Farbe `#1FB4FF` und wendet sie an. Aufnahme über `/sys/kernel/debug/usb/usbmon/1u`
(debugfs-Textinterface, **nicht** tshark/dumpcap — deren AppArmor-Profil erlaubt keinen
Zugriff auf `/dev/usbmon*`, siehe Vorgehen unten), gefiltert auf Device 061 (unsere
Light Mount). Rohdaten: `own_capture_iocenter_raw.txt` (nur unser Gerät, keine anderen
USB-Geräte des Bus 1 enthalten).

**Einschränkung der Aufnahmemethode:** Das debugfs-Textinterface zeigt nur die ersten
32 Byte jeder Nutzlast (Kernel-seitige Begrenzung des Text-Formatters, nicht
konfigurierbar) — für alle hier relevanten Kommandos (Länge ≤ 15 Byte Payload)
ausreichend, für längere Kommandos (z. B. den 41-Byte-Rainbow-Befehl) wäre das nicht
ausreichend.

## Neu entdeckt: periodisches Keepalive (Subcmd `0x03`)

Der Großteil des Captures (36 von 40 Kommando/Antwort-Paaren) ist ein Keepalive/Poll,
der allein durch offenes Browser-Tab alle ~1,5s gesendet wird, unabhängig von jeder
Nutzeraktion:

```
Kommando: 06 00 01 00 <seq_lo> <seq_hi> 03 00 (Rest Null)   Länge=6, Subcmd=0x03
Antwort:  06 00 01 00 <seq_lo> <seq_hi> 03 00 (Rest Null)   Länge=6 (Echo)
```

## Bestätigt: RGB-Kodierung für frei wählbare Farben

Drei echte Kommandos (Länge 15, Subcmd `0x06`) traten während der Farbwahl auf —
vermutlich zwei Live-Vorschau-Ereignisse beim Ziehen im Farbwähler, gefolgt vom
tatsächlich angewendeten Wert:

| Seq | Payload (Byte 8-15, hex) | payload[5:8] (R,G,B) |
|---|---|---|
| 0x10a6 | `00 00 64 32 00 ab 21 ff` | `ab 21 ff` (Vorschau) |
| 0x10ab | `00 00 64 32 00 1f cb ff` | `1f cb ff` (Vorschau) |
| 0x10ad | `00 00 64 32 00 1f b4 ff` | **`1f b4 ff`** |

Der letzte Wert (`1f b4 ff`) entspricht **exakt** dem vom Nutzer aus der UI abgelesenen
Hex-Wert `#1FB4FF`. **Damit ist die RGB-Kodierung für diese Kommandofamilie bestätigt:**
Byte 13-15 des Reports (= payload[5:8]) tragen Rot/Grün/Blau direkt, keine Kodierung,
kein Offset. Byte 8-12 (`00 00 64 32 00`) waren in allen drei Beobachtungen identisch —
vermutlich feste Parameter für „Static Color"-Modus:

- **Byte 10 = `0x64` (100 dezimal) = Helligkeit, bestätigt** — Nutzer hat verifiziert,
  dass die Helligkeit in IO Center Web durchgehend auf 100% steht. Wert passt exakt.
  Ein Test mit abweichender Helligkeit (z. B. 50%) stünde noch aus, um die Byte-Position
  wirklich zweifelsfrei von einem Zufallstreffer zu unterscheiden — aber die Größenordnung
  (100 dezimal = 100%) ist ein starkes Indiz.
- Byte 11 = `0x32` (50 dezimal) bleibt **unklar** — NICHT die Helligkeit (die ist bereits
  Byte 10), möglicherweise ein weiterer Effektparameter ohne sichtbare Auswirkung im
  Static-Color-Modus. Nicht weiter interpretiert.

**Rückwirkende Erklärung für Frame 2605** aus dem alten Fremd-Capture (siehe
`usbmon3_decoded_commands.txt`): Payload `00 00 64 32 00 e6 30 00` — mit dem jetzt
bekannten Schema ist das dieselbe Kommandofamilie mit RGB `e6 30 00` (ein
Orange-Rot-Ton). Passt inhaltlich zu Frame 2747 (dort separat als "Orange"-Preset
bestätigt), auch wenn es sich um unterschiedliche Kommandos handelt — nicht als
Bestätigung derselben Farbe überinterpretieren, nur als plausibilitätsstützende
Beobachtung festgehalten.

## Korrektur: Byte 2-3 ist KEIN fester Protokoll-Konstantenwert

Alle 20 Frames aus dem alten Fremd-Capture hatten Byte 2-3 = `02 00`. In diesem eigenen
Capture (andere Browser-Session) ist Byte 2-3 durchgehend `01 00`. Die bisherige
Doku-Formulierung „konstant 0x0002" war also nur für die eine damalige Session korrekt,
nicht universell. Wahrscheinlich eine Art Sitzungs-/Verbindungs-ID, die pro
WebHID-Verbindung neu vergeben wird — noch nicht geklärt, wodurch der genaue Wert
bestimmt wird. `src/protocol/report.cpp` (`build_report`) hartkodiert derzeit `0x02` an
dieser Stelle; das ist für exakte Reproduktion bekannter Fixtures korrekt, aber nicht
als allgemeingültige Protokollregel misszuverstehen (Code-Kommentar entsprechend
präzisiert, siehe `report.h`).

## Neu entdeckt: unaufgeforderte Push-Frames spiegeln Tastendrücke (Subcmd `0x02`, Geräte→Host)

Während der Aufnahme drückte der Nutzer die Pfeiltaste-rechts und Enter (normale
Tastatureingabe, keine sensiblen Inhalte). Zeitgleich beobachtet:

```
Interface 0 (normale Tastatur, EP 0x81 IN):
  C Ii:1:061:1 ... = 00 00 4f 00 ...   (Keycode 0x4F = Pfeil rechts)
  C Ii:1:061:1 ... = 00 00 28 00 ...   (Keycode 0x28 = Enter)

Interface 2 (Vendor-Kanal, EP3 IN), fast zeitgleich (< 100µs Versatz):
  C Ii:1:061:3 = 0c 00 01 00 <seq> 02 5c 00 01 01 00 4f 00 ...
  C Ii:1:061:3 = 0c 00 01 00 <seq> 02 0a 00 01 01 00 28 00 ...
```

Payload-Byte 4 (relativ, = raw Byte 12) enthält **exakt** denselben Keycode-Wert
(`0x4f`, dann `0x28`), der im selben Moment auf Interface 0 gesendet wurde. Byte 7
(Flags) unterscheidet sich pro Ereignis (`0x5c`, `0x0a`) — Bedeutung unklar.

**Hypothese (durch Timing + exakten Byte-Match gut gestützt, aber nicht durch
gezielten Einzeltest verifiziert):** Da WebHID-APIs im Browser aus Sicherheitsgründen
typischerweise keinen Zugriff auf echte Tastatur-Interfaces gewähren (Keylogging-Schutz),
spiegelt die Firmware ausgewählte Tastendrücke zusätzlich über den Vendor-Kanal
(Interface 2), damit die Web-App z. B. Pfeiltasten/Enter zur UI-Navigation im
Farbwähler auswerten kann, ohne die Browser-Beschränkung zu umgehen. Sicherheits-
relevante Nebenbeobachtung: dieser Mechanismus bedeutet, dass eine Website mit
WebHID-Zugriff auf Interface 2 zumindest einzelne Tastendrücke mitlesen kann — siehe
Vermerk in `SECURITY.md`.

## Offene Fragen (bewusst nicht geraten)

- Bedeutung von Byte 10 (`0x64`) und Byte 11 (`0x32`) im Static-Color-Kommando —
  Helligkeit/Speed-Parameter plausibel, nicht verifiziert (kein Einzeltest mit
  variierter Helligkeit durchgeführt).
- Bedeutung des „Flags"-Bytes bei den Tastendruck-Push-Frames (`0x5c` vs. `0x0a`).
- Wodurch Byte 2-3 (Sitzungs-ID?) bestimmt wird und ob es für eigene Kommandos frei
  wählbar ist oder vom Gerät vorgegeben wird.
