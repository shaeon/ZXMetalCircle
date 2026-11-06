# Keyboard and mouse

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## Keys

The function keys are in a single table (`KeyBindings` in `src/kernel.cpp`). The F1 help is generated from it, so changing an assignment only needs that table.

| Key | Function |
| --- | --- |
| F1 | Help and status; from there, ENTER opens the file browser, C the settings, U the drives and A the credits |
| Alt+F1 | The settings directly (ESC goes back to the Spectrum) |
| Alt+U | Drives: see, mount and empty each drive |
| Alt+K | Keyboard help |
| F2 | Tape: play / stop |
| F3 | Tape: block browser (list of what is on the tape; ENTER leaves it at the chosen block, Home = rewind) |
| F4 | Change joystick |
| F5 | NMI: Multiface, esxDOS browser or Spectranet menu |
| Alt+F5 | File browser (like F1 and ENTER). With `f5=files` they swap: F5 opens the files and Alt+F5 does the NMI |
| F6 | Loading speed: instant / fast / real |
| F7 | List of models to choose from (the current one highlighted; those without a ROM say so) |
| Alt+F7 | Switch to the next model (16K, 48K, 128K, +2, +2A, +3, +3e) |
| F8 | Save a snapshot to `snaps/` (.Z80, .SNA or .SZX according to `snapformat=`) |
| F9 | AY stereo: ABC → ACB → BAC → mono |
| F10 | Speed: x1 → x2 → x4 → x10 |
| F11 | Remove the Interface 2 cartridge (with reset) |
| Alt+B | Border size: none → standard → full → huge |
| Alt+S | Scanlines on/off |
| Alt+W | Record the MIC output (the Spectrum's tape signal) |
| Alt+G | Record the tape input (real cassette, with `ear=`) |
| Alt+N | Microdrive: insert a new cartridge (unformatted) |
| Alt+M | Microdrives: save the changes and remove all cartridges |
| Alt+L / Alt+T / Alt+X | LEC memory / Spectranet / divMMC on/off (with reset) |
| Alt+F / Alt+E | +3 disk: new / eject |
| Alt+D | Keyboard diagnostics: what arrives over USB, the keys the Spectrum sees, frames per second and frame times |
| Alt+V | Press several keys at once (cheats like VICTOR in Profanation) |
| F12 or Alt+R | Reset |
| Pause or Alt+P | Pause |

PC keyboard to Spectrum keyboard mapping:

- Shift = CAPS SHIFT and Ctrl = SYMBOL SHIFT.
- Tab = EXTENDED MODE, Esc = BREAK, Backspace = DELETE and Caps Lock = CAPS LOCK.
- The punctuation of the Spanish keyboard (`, . - + ' < >` and their shifted variants) is translated to its SYMBOL SHIFT combination.
- **Cursor keys**: they are always the Spectrum's cursor keys (CAPS SHIFT + 5/6/7/8). With a joystick selected, they also move the joystick, and **the Windows keys** (or keypad 0) are fire; **both Ctrl keys are SYMBOL SHIFT**. This can be changed with `cursors=` and, the fire key, with `kbdfire=` or on page 5 of the settings: Windows keys, right Ctrl (as before 1.08) or none. With no joystick, or with `cursors=keys`, the fire key does not fire.

To load a tape: press F1 and then ENTER, choose the `.TAP`, `.TZX`, `.PZX` or `.CSW` and type `LOAD ""` (on the 128K/+2A, choose "Tape Loader" or "Loader" in the menu). The tape starts by itself when the ROM or a custom loader starts reading it, and stops by itself when the program stops reading it like a loader for 2 seconds. What counts is reading like a loader (the same IN over and over, with a counter that advances, as Fuse does), not reading a lot: menus that poll the keyboard non-stop neither start the tape nor keep it running, so multiloads find the next block where they left it. In fast mode, the emulator runs at full speed while loading.

## Several keys at once (Alt+V)

Some cheats need many keys pressed together, like **VICTOR** in Profanation for infinite lives. A USB keyboard reports at most six keys at a time, and many cannot detect some combinations of four, five or six keys.

**Alt+V** opens a line where you type the keys (letters, digits and space) and, with ENTER, the emulator holds them all down together for 1.5 seconds. It works with any keyboard. With a real Spectrum keyboard over GPIO or a Recreated ZX it is not needed: all of them can be pressed at once, as on the Spectrum.

If the USB keyboard cannot handle a combination and reports too many keys at once, the emulator keeps the keys it already had pressed until the next valid report, as the USB standard says, instead of releasing them all.

## Keyboard diagnostics (Alt+D)

Shown at the top left, without stopping the emulation:
- the last USB keyboard report (modifiers and up to 6 keys, in hexadecimal);
- the Spectrum matrix with the pressed keys highlighted, adding up all sources (USB, GPIO, Recreated, cursor keys);
- the real speed in frames per second, and how long each frame takes on core 0 (and the image scaling on core 1, on the 4-core models).

It shows exactly what the Spectrum receives when a key does something odd. Alt+D again removes it.

If the Raspberry Pi cannot draw every frame (for example, with scanlines at native resolution on a slow Pi), the emulator skips some frames, up to 4 in a row, instead of running slower. That way the Spectrum and the keyboard keep their speed.

## Recreated ZX Spectrum

The Recreated ZX Spectrum (Elite Systems, 2013) is a replica of the rubber-key 48K that works as a USB or Bluetooth keyboard. Here only USB is supported (Circle has no Bluetooth HID).

- Put it in **Layer A**, the game layer, with the switch at the back. In that layer, each key sends one character when pressed and another when released (for example, 1 sends `a` when pressed and `b` when released). So all 40 keys are known in any combination, as on the original membrane.
- **Through a USB cable it is recognised when connected**, by its USB identifier (VID:PID `0F39:0404`, TG3 Electronics; information from José Luis Sánchez), without pressing anything. `recreated=0` turns it off (for example, to use it in Layer B, QWERTY).
- **Automatic detection** (with `recreated=auto`, the default) for when it is not recognised by the identifier, for example through an adapter:
  - the emulator recognises it by the way it transmits: three complete press-release pairs (for example, 'a' and then 'b' for key 1) lasting less than 30 ms. Pressing any key 3 times is enough, and "Recreated ZX keyboard detected" appears;
  - meanwhile, those key presses do not reach the Spectrum;
  - a normal keyboard is recognised as such after 8 key presses, and until then each key may arrive with a delay of up to about 50 ms.
- It can be used alone or together with a normal USB keyboard and the real GPIO keyboard. Its keys are combined with those of the GPIO keyboard.
- **Extended mode**: CAPS SHIFT + SYMBOL SHIFT + ENTER, as with the GPIO keyboard and in ZXBaremulator. The keys available in that mode and with CAPS SHIFT are the same as with the GPIO keyboard: see the table in [03_gpio.md](03_gpio.md).
- In Layer B (QWERTY), the Recreated works as a normal keyboard.

## Kempston mouse

A connected (hot-plugged) USB mouse works as a Kempston mouse. As in ZXBaremulator, it is only active when the selected joystick is **not** Kempston, because they share ports. If no mouse is connected, the ports do not respond, so programs that detect it do not find it.

| Port | Content |
| --- | --- |
| 0xFADF | Buttons: bit 0 right, bit 1 left, bit 2 middle (active at 0); bits 4-7, wheel |
| 0xFBDF | X (grows to the right) |
| 0xFFDF | Y (grows upwards) |

Decoding is partial (A0, A5, A8 and A10), as in Fuse.
