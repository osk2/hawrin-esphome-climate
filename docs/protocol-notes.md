# Hawrin AC IR protocol notes

These notes document the information needed to continue adding features to the `hawrin_ac` ESPHome component.

## Target remote

Tested with the **華菱 HG0220E29A** remote control.

The captured signal is not JVC/Drayton/LG despite ESPHome sometimes auto-detecting those decoders. Those are false positives caused by the leading bits. Treat the Pronto/raw waveform as the source of truth.

## Carrier and timing

The generated encoder uses raw timings instead of Pronto strings.

```text
carrier: 38 kHz
header: 9070us mark, 4520us space
bit mark: 580us
0 bit: 580us space
1 bit: 1680us space
frame gap: 8050us space
end gap: 10120us space
bit order: LSB-first per byte
```

A full message has three data frames:

```text
frame0: 6 bytes
frame1: 8 bytes
frame2: 7 bytes
```

The waveform order is:

```text
header
frame0 bytes
580/8050 gap
frame1 bytes
580/8050 gap
frame2 bytes
580/10120 end
```

## Byte layout

Flattened data is 21 bytes:

```text
byte0 byte1 byte2 byte3 byte4 byte5 byte6 byte7 byte8 byte9 byte10 byte11 byte12 byte13 byte14 byte15 byte16 byte17 byte18 byte19 byte20
```

Frame split:

```text
frame0 = byte0..byte5
frame1 = byte6..byte13
frame2 = byte14..byte20
```

Checksum for all currently implemented commands:

```text
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
```

## Full-state cool command

Used for normal climate state updates.

```text
frame0:
byte0 = 83
byte1 = 06
byte2 = fan
byte3 = temp
byte4 = 00
byte5 = 00

frame1:
byte6 = 90
byte7 = temp group
byte8..byte12 = 00
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8

frame2:
00 11 00 00 08 00 19
```

`byte4`, `byte5`, and `byte8` are `00` for cool full-state frames, so the general checksum reduces to a shorter form.

## Mode commands

Mode changes use command frames, not the normal cool full-state frame.

Cool mode can be sent as the full-state cool command above. The captured explicit mode command was:

```text
byte2 = 01
byte3 = A2
byte4 = 00
byte6 = 91
byte7 = 1D
frame2 = 00 06 00 00 08 00 0E
```

Dry mode:

```text
byte2 = 00
byte3 = 73
byte4 = 00
byte6 = 91
byte7 = 1E
frame2 = 00 06 00 00 08 00 0E
```

Fan-only mode:

```text
byte2 = 01
byte3 = 84
byte4 = 00
byte6 = 91
byte7 = 20
frame2 = 00 06 00 00 08 00 0E
```

Auto mode is exposed as ESPHome/Home Assistant `heat_cool`:

```text
byte2 = 01
byte3 = 71
byte4 = 80
byte6 = 91
byte7 = 1E
frame2 = 00 17 00 00 08 00 1F
```

Temperature encoding:

```text
byte3 = ((temp_c - 16) << 4) | 0x02

20C -> 42
21C -> 52
22C -> 62
23C -> 72
24C -> 82
25C -> 92
26C -> A2
27C -> B2
28C -> C2
29C -> D2
30C -> E2
```

Temperature group / byte7:

```text
20-23C -> 11
24-26C -> 12
27-30C -> 13
```

Fan encoding / byte2:

```text
auto   -> 00
high   -> 01
medium -> 02
low    -> 03
```

Example full-state bytes:

```text
cool 26 fan auto:
83 06 00 A2 00 00 90 12 00 00 00 00 00 20 00 11 00 00 08 00 19

cool 26 fan low:
83 06 03 A2 00 00 90 12 00 00 00 00 00 23 00 11 00 00 08 00 19
```

## Power

The remote only exposes a power toggle. Captured `on` and `off` transitions produced the same toggle command, so the component cannot implement reliable absolute off without an external AC state sensor.

```text
byte2 = 04
byte6 = 8F
byte7 = 13
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 01 00 00 08 00 09
```

`byte3` still carries the current target temperature.

## Eco preset

Eco is a command, not the normal full-state frame.

Eco on:

```text
byte2 = 03
byte6 = 91
byte7 = 05
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 20 0C 00 00 08 00 24
```

Eco off:

```text
byte2 = 00
byte6 = 91
byte7 = 05
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 0C 00 00 08 00 04
```

Observed at 26C:

```text
eco on:
83 06 03 A2 00 00 91 05 00 00 00 00 00 35 20 0C 00 00 08 00 24

eco off:
83 06 00 A2 00 00 91 05 00 00 00 00 00 36 00 0C 00 00 08 00 04
```

## Sleep preset

Sleep mode is a command, not the normal full-state frame. Unlike display and swing, sleep on/off captures are different, so it is exposed as the standard ESPHome `sleep` preset.

Sleep on:

```text
byte2 = 08
byte6 = 94
byte7 = 22
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 03 00 00 08 00 0B
```

Sleep off:

```text
byte2 = 00
byte6 = 94
byte7 = 22
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 03 00 00 08 00 0B
```

Observed at 26C:

```text
sleep on:
83 06 08 A2 00 00 94 22 00 00 00 00 00 1C 00 03 00 00 08 00 0B

sleep off:
83 06 00 A2 00 00 94 22 00 00 00 00 00 14 00 03 00 00 08 00 0B
```

## Boost preset

Turbo / powerful mode is exposed as the standard ESPHome `boost` preset. On/off captures are different, so this is not a toggle.

Boost on:

```text
byte2 = 01
byte3 = 02
byte5 = 90
byte6 = 97
byte7 = 39
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 04 00 00 08 00 0C
```

Boost off:

```text
byte2 = 00
byte6 = 97
byte7 = 34
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 04 00 00 08 00 0C
```

Observed:

```text
boost on:
83 06 01 02 00 90 97 39 00 00 00 00 00 3D 00 04 00 00 08 00 0C

boost off at 26C:
83 06 00 A2 00 00 97 34 00 00 00 00 00 01 00 04 00 00 08 00 0C
```

## Display

Display on/off captures were identical, so display is a toggle.

```text
byte2 = 00
byte6 = B1
byte7 = 06
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 00 00 00 08 00 08
```

Observed at 26C:

```text
83 06 00 A2 00 00 B1 06 00 00 00 00 00 15 00 00 00 00 08 00 08
```

## Swing

Vertical swing on/off captures were identical, so vertical swing is exposed as a toggle.

```text
byte2 = 80
byte6 = 94
byte7 = 23
byte8 = 40
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 07 00 00 08 00 0F
```

Observed at 26C:

```text
83 06 80 A2 00 00 94 23 40 00 00 00 00 D5 00 07 00 00 08 00 0F
```

Horizontal swing on/off captures were identical, so horizontal swing is exposed as a toggle.

```text
byte2 = 00
byte6 = 94
byte7 = 24
byte8 = 80
byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8
frame2 = 00 08 00 00 08 00 00
```

Observed at 26C:

```text
83 06 00 A2 00 00 94 24 80 00 00 00 00 92 00 08 00 00 08 00 00
```

## Earlier false starts / caution

Some real captures from temperature up/down key presses used event-like frames:

```text
byte6 = 8F
frame2 = 00 02 00 00 08 00 0A
```

Those captures were useful for identifying fields, but not reliable for replay as full-state climate commands. For normal climate control, use the full-state family:

```text
byte6 = 90
frame2 = 00 11 00 00 08 00 19
```

Do not infer future full-state behavior from temperature +/- event captures unless replay is tested.

## Adding a new feature

Recommended capture method:

1. Set the remote to a known baseline: `cool`, `26C`, `fan auto`, swing off if available.
2. Capture the feature in both directions when possible:
   - `feature_on`
   - `feature_off`
   - or `feature_toggle_1` / `feature_toggle_2` when the remote has only a toggle button.
3. Capture the surrounding full state again after the feature action if the feature appears to persist.
4. Convert Pronto to LSB-first bytes and compare:
   - `byte2`
   - `byte4` / `byte5`
   - `byte6`
   - `byte7`
   - `byte13`
   - `frame2`
5. Validate whether `byte13 = byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8` still holds.
6. Replay-test the generated raw command before adding it to `hawrin_ac.cpp`.

A good sample set for a new toggle-like feature:

```text
baseline_cool_26_auto
feature_toggle_1
feature_toggle_2
baseline_cool_26_auto_again
```

A good sample set for an explicit on/off feature:

```text
baseline_cool_26_auto
feature_on
feature_off
feature_on_again
```

When a capture produces identical `feature_on` and `feature_off` bytes, expose it as a toggle only.
