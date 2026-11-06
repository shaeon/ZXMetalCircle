# ZXMetalCircle GPIO pins

Which pins of the Raspberry Pi's 40-pin header the emulator uses, what for, and which options exclude each other because they share pins.

Numbers are always **GPIO (BCM)**; in brackets, the **physical pin** on the header. Every pin the emulator reads is an input with an internal resistor to 3.3 V (pull-up): **never connect 5 V to them**.

## What uses the GPIO

| Function | Option in `cmdline.txt` | In the settings (Alt+F1) |
| --- | --- | --- |
| Real 48K/128 keyboard (8 × 5 matrix), with joysticks wired to the matrix | `gpiokbd=48` (default) | 4 Keyboard and controllers: "Keyboard via GPIO" = 48K/128 |
| Real +2A/+3 keyboard (13 × 11 matrix) | `gpiokbd=plus3` | "Keyboard via GPIO" = +2A/+3 |
| No GPIO keyboard | `gpiokbd=0` | "Keyboard via GPIO" = none |
| Extended mode button (48K/128 keyboard only) | comes with `gpiokbd=48` | — |
| NMI button (48K/128 keyboard only) | comes with `gpiokbd=48` | — |
| Copy of the PWM sound on GPIO12 (Pi 1, 2, 3 and Zero) | `sounddev=sndpwm gpioaudio=1` | 2 Picture and sound: "Sound via GPIO12" |
| PWM sound on the Pi 5 (it has no jack) | `sounddev=sndpwm` | "Sound output" = jack (PWM) |
| Input from a real cassette player (with a comparator, e.g. LM393) | `ear=gpio earpin=...` | 6 Network, time and cassette: "Real cassette" = GPIO, "Cassette GPIO" |
| Logging through the serial port | `logdev=ttyS1` | — |

The keyboard wiring is pastbytes' design for ZXBaremulator (https://pastbytes.com/zxb/), with a diode on each row (cathode towards the Raspberry Pi). The +2A/+3 one follows spark2k06's zxunops2 project.

## Table by pin

"48K" is `gpiokbd=48` and "+3" is `gpiokbd=plus3`. An empty cell means that option does not use the pin.

| GPIO | Pin | 48K keyboard | +3 keyboard | Sound | Cassette (`ear=gpio`) | Other |
| ---: | ---: | --- | --- | --- | --- | --- |
| 2 | 3 | | column C9 | | | (I2C; the Pi fits a fixed pull-up) |
| 3 | 5 | | column C10 | | | (I2C; the Pi fits a fixed pull-up) |
| 4 | 7 | row A15 | row A15 | | | |
| 5 | 29 | column D4 | column D4 | | | |
| 6 | 31 | column D3 | column D3 | | | |
| 7 | 26 | | row R12 | | | |
| 8 | 24 | | column C5 | | | |
| 9 | 21 | | column C6 | | | |
| 10 | 19 | | column C7 | | | |
| 11 | 23 | | column C8 | | | |
| 12 | 32 | | row R11 | copy of the PWM (Pi 1-3, `gpioaudio=1`); PWM on the Pi 5 | | |
| 13 | 33 | column D2 | column D2 | PWM on the Pi 5 | | |
| 14 | 8 | | | | | serial port TX (`logdev=ttyS1`) |
| 15 | 10 | | | | **default with the +3 keyboard** | serial port RX (not used for logging) |
| 16 | 36 | NMI button | row R10 | | | |
| 17 | 11 | row A14 | row A14 | | | |
| 18 | 12 | row A8 | row A8 | | | |
| 19 | 35 | column D1 | column D1 | | | |
| 20 | 38 | | row R8 | | **default** (no keyboard or 48K keyboard) | |
| 21 | 40 | extended mode button | row R9 | | | |
| 22 | 15 | row A12 | row A12 | | | |
| 23 | 16 | row A9 | row A9 | | | |
| 24 | 18 | row A10 | row A10 | | | |
| 25 | 22 | row A11 | row A11 | | | |
| 26 | 37 | column D0 | column D0 | | | |
| 27 | 13 | row A13 | row A13 | | | |
| 0, 1 | 27, 28 | | | | | (HAT board EEPROM: not used) |

The buttons go from their GPIO to GND: the extended mode one from GPIO21 (pin 40) to pin 39, and the NMI one from GPIO16 (pin 36) to pin 34.

Free pins depending on the keyboard:

- **No GPIO keyboard**: all except 14 (if you use `logdev=ttyS1`).
- **48K/128 keyboard**: 2, 3, 7, 8, 9, 10, 11, 12, 15 and 20 are free (and 14 without serial logging).
- **+2A/+3 keyboard**: only 14 and 15 are left (the serial port).

## Shared pins: what is lost depending on the option

| If you choose... | ...this happens | Why |
| --- | --- | --- |
| `gpiokbd=plus3` | No extended mode button (CAPS SHIFT + SYMBOL SHIFT + ENTER works instead) and no NMI button | GPIO21 and GPIO16 are rows of the +3 keyboard |
| `gpiokbd=plus3` | No copy of the sound on GPIO12, even with `gpioaudio=1` | GPIO12 is row R11 |
| `gpiokbd=plus3` and `ear=gpio` | The cassette goes to GPIO15 (pin 10) instead of 20 | GPIO20 is row R8; 15 is the serial port's RX input, which logging (`logdev=ttyS1`) does not use |
| `gpiokbd=48` or `plus3` on the **Pi 5** with `sounddev=sndpwm` | The PWM does not start and the sound goes through HDMI | The Pi 5 outputs the PWM on GPIO12 and GPIO13, and 13 is a keyboard column: with the PWM playing, keys looked permanently pressed |
| `gpioaudio=1` on the **Pi 4 / 400** | Does nothing | On the Pi 4 the jack audio uses PWM1, which does not reach the header |
| `gpioaudio=1` | The sound becomes mono | GPIO12 is a single channel of PWM0 (the left one on some models and the right one on others) |
| `ear=gpio earpin=N` with a pin in use | The cassette is not enabled and the warning "earpin=N: ese GPIO ya se usa" ("that GPIO is already in use") appears | The pin belongs to the keyboard, to GPIO12 with sound, or is 0, 1 or 14 |
| `ear=gpio` and `ear=usb` | Only one at a time | It is the same option |

## In the settings (Alt+F1)

Options that are not used with the current choices are **greyed out** and cannot be changed (version 0.91 onwards):

| Option | Greyed out when... |
| --- | --- |
| Sound via GPIO12 | the sound output is not the jack (PWM); the GPIO keyboard is the +2A/+3 one; or the Pi is a 4, 400 or 5 |
| Cassette GPIO | "Real cassette" is not GPIO |
| Invert cassette | "Real cassette" is no |

Two cases the menu **cannot** flag, because it only greys out whole options, not individual values of a list:

- **"Cassette GPIO" with a pin in use**: it can be chosen, but at boot the warning "that GPIO is already in use" appears and the cassette is not enabled. Leave it on "automatic", which always picks a free pin (20, or 15 with the +3 keyboard).
- **"Sound output" = jack (PWM) on the Pi 5 with a GPIO keyboard**: it can be chosen, but the emulator switches to HDMI by itself (see the table above).

Changes to these options apply when the Pi restarts (they carry a `*` in the menu).

---

## Wiring

### Real keyboard and joysticks through GPIO

The full GPIO pin table (keyboard, sound, cassette, serial port) and which options share pins is in [docs/03_gpio.md](#).

It is compatible with Jorge Castillo's (pastbytes) "keyboard and 2 Atari-compatible joysticks" wiring for ZXBaremulator: https://pastbytes.com/zxb/. Two keyboard types are supported, chosen with `gpiokbd=`:

| `gpiokbd=` | Keyboards | Matrix |
| --- | --- | --- |
| `48` (default; also `1` or `128`) | Rubber-key 48K, Spectrum+, **128** and **grey +2** | 8 × 5 |
| `plus3` (also `plus2a`) | **+2A, +2B and +3** | 13 × 11 |
| `0` | none (leaves the pins free) | — |

### 48K, Spectrum+, 128 and grey +2 keyboards

These keyboards have the same 8 × 5 matrix as the 48K. The extra keys of the Spectrum+, 128 and +2 (cursors, EDIT, DELETE, BREAK, EXTEND MODE, `;`, `"`, `.`, `,`…) close two contacts at once inside the membrane, for example CAPS SHIFT + 5. The Spectrum, and this reader, see them just as on the real machine, so nothing else needs configuring.

They connect with their two ribbons: **KB1** (5 ways, data D0–D4) and **KB2** (8 ways, address A8–A15). The order of the lines in the ribbons is the one used by spark2k06's zxunops2 project (https://github.com/spark2k06/zxunops2):

| Ribbon | Pin | Signal | Keys | GPIO (BCM) | Pi pin |
| --- | --- | --- | --- | --- | --- |
| KB2 | 1 | A11 | 1 2 3 4 5 | 25 | 22 |
| KB2 | 2 | A10 | Q W E R T | 24 | 18 |
| KB2 | 3 | A9 | A S D F G | 23 | 16 |
| KB2 | 4 | A12 | 0 9 8 7 6 | 22 | 15 |
| KB2 | 5 | A13 | P O I U Y | 27 | 13 |
| KB2 | 6 | A8 | CAPS SHIFT Z X C V | 18 | 12 |
| KB2 | 7 | A14 | ENTER L K J H | 17 | 11 |
| KB2 | 8 | A15 | SPACE SYMBOL SHIFT M N B | 4 | 7 |
| KB1 | 1 | D0 | | 26 | 37 |
| KB1 | 2 | D1 | | 19 | 35 |
| KB1 | 3 | D2 | | 13 | 33 |
| KB1 | 4 | D3 | | 6 | 31 |
| KB1 | 5 | D4 | | 5 | 29 |

Optionally, also:

| Signal | GPIO (BCM) | Pin |
| --- | --- | --- |
| Extended mode button (to GND, pin 39) | 21 | 40 |
| NMI button (to GND, pin 34) | 16 | 36 |
| PWM audio output (see below) | 12 | 32 |

### +2A, +2B and +3 keyboard

The Amstrad keyboard of these models does not close two contacts for the extra keys: it has them on **5 rows and 6 columns of their own**, and the +2A/+3 board converts them. ZXMetalCircle reads the full 13 × 11 matrix, like the ATmega644 version of zxunops2, and does the same conversion.

- The first 8 rows and the first 5 columns are those of the previous table (A8–A15 and D0–D4).
- These lines are also needed:

| zxunops2 line | GPIO (BCM) | Pi pin |
| --- | --- | --- |
| Row 9 (R8) | 20 | 38 |
| Row 10 (R9) | 21 | 40 |
| Row 11 (R10) | 16 | 36 |
| Row 12 (R11) | 12 | 32 |
| Row 13 (R12) | 7 | 26 |
| Column 6 (C5) | 8 | 24 |
| Column 7 (C6) | 9 | 21 |
| Column 8 (C7) | 10 | 19 |
| Column 9 (C8) | 11 | 23 |
| Column 10 (C9) | 2 | 3 |
| Column 11 (C10) | 3 | 5 |

- **Conversion of the extra keys** (the same as zxunops2's `traduceextra2a()`):

| Key | Row, column | On the Spectrum |
| --- | --- | --- |
| `"` | R8, C9 | SYMBOL SHIFT + P |
| `;` | R8, C10 | SYMBOL SHIFT + O |
| `.` | R9, C5 | SYMBOL SHIFT + M |
| `,` | R9, C6 | SYMBOL SHIFT + N |
| TRUE VIDEO | R10, C5 | CAPS SHIFT + 3 |
| INV. VIDEO | R10, C6 | CAPS SHIFT + 4 |
| EDIT | R10, C7 | CAPS SHIFT + 1 |
| CAPS LOCK | R10, C8 | CAPS SHIFT + 2 |
| Cursor left | R10, C10 | CAPS SHIFT + 5 |
| Cursor right | R11, C5 | CAPS SHIFT + 8 |
| Cursor up | R11, C6 | CAPS SHIFT + 7 |
| DELETE | R11, C7 | CAPS SHIFT + 0 |
| GRAPH | R11, C8 | CAPS SHIFT + 9 |
| Cursor down | R11, C10 | CAPS SHIFT + 6 |
| BREAK | R12, C7 | CAPS SHIFT + SPACE |
| EXTEND MODE | R12, C8 | CAPS SHIFT + SYMBOL SHIFT |

- **Shared pins**: this keyboard uses GPIO21, GPIO16 and GPIO12, so with `gpiokbd=plus3` there is **no extended mode button** (the CAPS SHIFT + SYMBOL SHIFT + ENTER combination works), **no NMI button** and **no audio on GPIO12**: sound comes out through HDMI or the jack. It also uses GPIO2 and GPIO3, which have fixed pull-up resistors on the Pi; for reading columns they do no harm.
- Altogether it takes 24 of the 26 GPIOs on the header. GPIO14 and 15 remain, the serial port ones: GPIO15 (pin 10) can be used as **tape input** (`ear=gpio`, see below), because logging through the serial port only uses GPIO14.

### Checking the wiring

The keyboard diagnostics (**Alt+D**) shows on screen the Spectrum matrix with the pressed keys highlighted. With `gpiokbd=plus3`, it adds a "+3 R8-12" line with the state of the extra rows, one number per row (bit 0 = C5 … bit 5 = C10).

If pressing a key lights up another one, most likely a ribbon is reversed or has a swapped wire. For example, if "1" lights up "B", KB2 is the wrong way round.

### Common details

- Each row has a diode, with the cathode towards the Raspberry Pi. The columns use the internal pull-up resistors, so no extra components are needed.
- The emulator never drives the rows high: it leaves them as inputs or pulls them low to select them.
- The joysticks are wired to the matrix as in the Interface 2: JOY1 is keys 6-0 (Sinclair 1) and JOY2 keys 1-5 (Sinclair 2).
- **Extended mode**: turned on with the button or with CAPS SHIFT + SYMBOL SHIFT + ENTER, and a blue keyboard icon appears at the bottom right. The same combination goes back to normal mode. The PC keys the Spectrum does not have are obtained like this:

  | With the real keyboard | Acts as |
  | --- | --- |
  | **Extended mode:** 1 … 0 | F1 … F10 |
  | **Extended mode:** Q, W | F11, F12 (as in ESPectrum) |
  | **Extended mode:** SYMBOL SHIFT + key | Alt + key (SS+1 = Alt+F1, SS+K keyboard help, SS+R reset, SS+P pause) |
  | CAPS SHIFT + 1, 2 | Home, End |
  | CAPS SHIFT + 3, 4 | PgUp, PgDn |
  | CAPS SHIFT + 5, 6, 7, 8 | cursor keys |
  | CAPS SHIFT + 9 | Delete (for example, to empty a drive in Alt+U) |
  | CAPS SHIFT + 0 | Backspace (go up a folder) |
  | CAPS SHIFT + SPACE (BREAK) | Esc |
  | SYMBOL SHIFT + M, typing an IP address in the settings | the dot |

  The CAPS SHIFT combinations work in the menus and in extended mode.

- **In the menus**: names that are too long scroll by themselves when selected (the others end in `>`). The cursor keys and PgUp/PgDn repeat when held down (after 400 ms and then every 60 ms), also with CAPS SHIFT + 5/6/7/8 on the GPIO keyboard. You navigate with CAPS SHIFT + 5/6/7/8 (cursors), ENTER, BREAK (Esc) and CAPS SHIFT + 0 (go up a folder). Letters jump in the list.
- The USB keyboard and the GPIO one can be used at the same time.
- **NMI** (only with `gpiokbd=48`): a button between GPIO16 (pin 36) and GND (pin 34) raises an NMI, which activates the Multiface. It is debounced.

### Tape input: real cassette

The emulated Spectrum can load from a real cassette, like the original. The signal arrives through a **GPIO** (with a comparator) or through the input of a **USB sound card**. The loading is heard through the speaker (with `tapesound=1`) and can be **recorded to a .TZX** to preserve the tapes.

| Option | Values | Default |
| --- | --- | --- |
| `ear=` | `0` (no input), `gpio`, `usb` | `0` |
| `earpin=` | GPIO (BCM) of the input with `ear=gpio` | `20` (pin 38); with `gpiokbd=plus3`, `15` (pin 10) |
| `earinvert=` | `1` if the signal arrives inverted | `0` |

### Through GPIO (any Raspberry Pi)

A cassette player's headphone output is an alternating signal from a few tenths of a volt to 1–2 V, centred on 0 V. The GPIO only accepts 0–3.3 V, so it **cannot be connected directly**: a comparator is needed to turn it into logic levels, as the Spectrum's ULA does. With an **LM393** (dual comparator that works at 3.3 V) powered from the Pi:

```
                3.3V (pin 1)                       3.3V
                  │                                 │
                [R1 100k]          ┌──[R5 1M]──┐  [R6 10k]
tape    ──[C1 1µF]──┬──────────────┤           │    │
(jack tip)          │      (3) IN+ ┴─┐          │    │
                  [R2 100k]          │ LM393 (1)├────┴──── GPIO20 (pin 38)
                    │      (2) IN− ┬─┘ OUT
                   GND             │
                3.3V──[R3 100k]──┬─┘
                                 ├──[C2 100nF]── GND
                          GND──[R4 100k]─┘
tape (jack ground) ─────────────────────────────────── GND (pin 39)

LM393: pin 8 = 3.3V, pin 4 = GND
```

- **C1** removes the DC and **R1/R2** centre the signal on 1.65 V.
- **R3/R4** (with C2 to stabilise it) set the reference at 1.65 V.
- **R5** gives about 160 mV of hysteresis, so noise does not produce false pulses.
- The LM393 output is open collector, with **R6** to 3.3 V: it never exceeds what the Pi accepts.
- Cassette volume: medium-high; tone, flat.
- With the 48 keyboard (`gpiokbd=48`), the input defaults to **GPIO20 (pin 38)**, which is free.
- With the **+3 keyboard** (`gpiokbd=plus3`) all the other GPIOs are in use, so the input defaults to **GPIO15 (pin 10)**. It is the serial port's RX input; the diagnostic log (`logdev=ttyS1`) only uses the TX output (GPIO14, pin 8), so it keeps working.
- If `earpin=` names a GPIO already in use (by the keyboard, its buttons, the GPIO12 audio or GPIO14), the emulator warns and does not enable the input.

**How it is read accurately**: each edge on the pin raises an interrupt that records it with the time of the 1 MHz clock. Before emulating each frame, the emulator hands the Spectrum that frame's edges, converted to t-states, with a fixed delay of 40 ms (two frames). So even if the emulation runs in bursts, the Z80 sees the signal at the right moment.

### Through a USB sound card (Pi 4 and 400)

A cheap USB sound card with a microphone or line input works without soldering anything: the signal is sampled at 48 kHz and the emulator decides the levels in software.
- It removes the DC.
- It follows the signal's amplitude: rising quickly and falling slowly.
- It applies hysteresis with a minimum, so it does not invent edges in silences.

With `sounddev=sndusb`, the same card also outputs the emulator's **sound**.

**It only works on the Pi 4 and 400**: Circle only has USB audio with those models' xHCI controller. On the others, `ear=usb` and `sndusb` warn and do nothing; use `ear=gpio`. USB sound uses a bit more CPU than HDMI or the jack, but the Pi 4 has plenty.

### Use

- The input only acts when there is a signal (some change in the last second and a half). If a virtual tape is playing, that one wins.
- It works at x1 speed, like a real cassette: instant and fast loading do not apply to the external input.
- The help (F1) shows "Cassette: signal / no signal".
- **Alt+G** starts and stops recording the input to `saves/rec_NNN.tzx`, with a direct recording block (CSW, 16 t-states per sample). Alt+W still records the Spectrum's own MIC output.

**Tested** on the PC with the emulator core:
- A tape's signal, delivered as edges with their time, loads with `LOAD ""` on a 48K. It still loads with a timing error of up to ±57 µs, far more than an interrupt adds.
- The same tape converted into 48 kHz samples (filtered as from a cassette, with DC and noise), passed through the USB input's detector: 37,107 of 37,447 edges are detected and the program loads.
- It has not yet been tested with real hardware (the LM393 and a USB card).

### Audio through GPIO12

With `sounddev=sndpwm`, the PWM signal also comes out of GPIO12 (pin 32), in mono. It works on the Pi 1, Zero, Zero 2, 2 and 3, but not on the Pi 4/400: there PWM audio uses a block that does not reach the header, so the jack must be used. It is a PWM signal, not analogue audio, and needs a filter:

```
GPIO12 (pin 32) ──[ 1 kΩ ]──┬──[ 10 µF + ]── output (to an amplifier, e.g. PAM8403)
                            │
                          [10 nF]
                            │
GND (pin 30 or 34) ─────────┴────────────── GND
```
