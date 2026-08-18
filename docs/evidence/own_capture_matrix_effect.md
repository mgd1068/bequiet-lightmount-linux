# Eigener Capture: Matrix-Effekt (2026-08-18)

Gezielter Mitschnitt (analog zu `own_capture_iocenter_decoded.md`), diesmal: Nutzer
wählt den vorgeformten **Matrix**-Effekt (mit Animation) in IO Center Web. Aufnahme über
`/sys/kernel/debug/usb/usbmon/1u`, gefiltert auf Device 061, Interface 2.

## Ergebnis: löst die seit Iteration 2 offene Frage zu Frame 2341/3109

Genau ein „echtes" Kommando (kein Keepalive) im Mitschnitt:

```
Kommando: 1d 00 01 00 c7 10 06 00 05 01 64 32 02 04 0d 02 08 00 00 3b 00 21 00 8f 11 43 00 ff 41 64 00 00 (+ Nullpadding)
          Länge=29 (0x1d), Session=0x0001, Seq=0x10c7, Subcmd=0x06, Flags=0x00
Antwort:  06 00 01 00 c7 10 06 00 (+ Nullpadding)   -- normale kurze Bestätigung (Byte3=0x00, akzeptiert)
```

**Der Payload-Teil ist byteidentisch mit Frame 2341/3109 aus dem alten Fremd-Capture**
(`usbmon3_decoded_commands.txt`: `0501643202040d020800003b0021008f114300ff416400`).
Damit ist jetzt geklärt, wofür dieses seit Iteration 2 unentschlüsselte Kommando steht:
**der Matrix-Effekt.** Kein Raten mehr nötig — direkter Vergleich zweier unabhängiger
Captures (fremd + eigen, gleicher Payload, gleiche Aktion).

## Payload-Beobachtungen (deskriptiv, nicht vollständig gedeutet)

```
Byte  8:  05   -- vermutlich Effekt-ID (Matrix), analog zu 03=Rainbow-Zyklus
Byte  9:  01
Byte 10:  64 (100)  -- passt zum wiederholt beobachteten Helligkeits-Byte-Muster
Byte 11:  32 (50)   -- weiterhin unklar (gleiche Position wie bei anderen Effekten)
Byte 12:  02
Byte 13:  04
Byte 14:  0d (13)
Byte 15:  02
Byte 16:  08
Byte 17:  00
Byte 18:  00
Byte 19:  3b (59)
Byte 20:  00
Byte 21:  21 (33)   -- Zahlenwert erinnert an die 33%-Stufe aus dem Rainbow-Kommando,
Byte 22:  00           könnte Zufall sein, nicht überinterpretiert
Byte 23:  8f (143)
Byte 24:  11 (17)   -- ebenso erinnert an die 17%-Stufe, nicht überinterpretiert
Byte 25:  43 (67)
Byte 26:  00
Byte 27:  ff
Byte 28:  41 (65)
Byte 29:  64 (100)
Byte 30:  00
Byte 31:  00
```

Deutlich komplexere Struktur als die Static-Color- oder Rainbow-Kommandos — vermutlich
mehrere Parameter (evtl. zwei Farben + Geschwindigkeit + Richtung, in Analogie zu
Mountains `SendColorMatrixCmd`-Konzept, aber **nicht** byteidentisch dazu, siehe
`PROTOCOL.md`-Strukturvergleich). Nicht weiter gedeutet, um nicht zu raten — für eine
vollständige Zuordnung wäre ein gezielter Einzelparameter-Test nötig (z. B. nur die
Geschwindigkeit ändern, Rest konstant halten).

## Konsequenz für BACKLOG/PROTOCOL

Der Backlog-Punkt „Frame 2341/3109 strukturell einordnen" ist damit als **Matrix-Effekt**
identifiziert. Die genaue Byte-für-Byte-Bedeutung bleibt offen.
