# Keyboard and mouse

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## Keys

The keys are in a single table (`KeyBindings` in `src/kernel.cpp`), with the group each one appears in on the list of all keys. The F1 help has two parts: the short one, with the everyday keys and the status (`ShortHelp`), and the list of **all keys** (T from the help), in three pages by topic (PgUp/PgDn; ESC goes back to the help), generated from the table. Changing an assignment only needs that table.

| Key | Function |
| --- | --- |
| F1 | Help and status; from there, ENTER opens the file browser, C the settings, U the drives, P the pokes, T the list of all keys and A the credits |
| Alt+F1 | The settings directly (ESC goes back to the Spectrum) |
| Alt+U | Drives: see, mount and empty each drive |
| Alt+K | Keyboard help |
| F2 | Tape: play / stop |
| F3 | Tape: block browser (list of what is on the tape; ENTER leaves it at the chosen block, Home = rewind) |
| F4 | Change joystick |
| F5 | NMI: Multiface, esxDOS browser or Spectranet menu |
| Alt+F5 | File browser (like F1 and ENTER). With `f5=files` they swap: F5 opens the files and Alt+F5 does the NMI. Inside, F2 copies, moves, renames, deletes or creates folders (see below) |
| F6 | Loading speed: instant / fast / real |
| F7 | List of models to choose from (the current one highlighted; those without a ROM say so) |
| Alt+F7 | Switch to the next model (16K, 48K, 128K, +2, +2A, +3, +3e) |
| F8 | Save a snapshot to `snaps/` (.Z80, .SNA or .SZX according to `snapformat=`) |
| F9 | AY stereo: ABC → ACB → BAC → mono |
| Alt+ + / Alt+ - / Alt+0 | Volume up / down (in steps of 10 %) / mute on-off. The + and - of the keypad also work |
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
| Alt+V | Press keys simultaneously (cheats like VICTOR in Profanation) |
| Alt+C | Pokes and cheats for the loaded game (see below) |
| Alt+O | Manual POKE |
| F12 or Alt+R | Reset |
| Pause or Alt+P | Pause |

PC keyboard to Spectrum keyboard mapping:

- Shift = CAPS SHIFT and Ctrl = SYMBOL SHIFT.
- Tab = EXTENDED MODE, Esc = BREAK, Backspace = DELETE and Caps Lock = CAPS LOCK.
- The punctuation of the Spanish keyboard (`, . - + ' < >` and their shifted variants) is translated to its SYMBOL SHIFT combination.
- **Cursor keys**: they are always the Spectrum's cursor keys (CAPS SHIFT + 5/6/7/8). With a joystick selected, they also move the joystick, and **the Windows keys** (or keypad 0) are fire; **both Ctrl keys are SYMBOL SHIFT**. This can be changed with `cursors=` and, the fire key, with `kbdfire=` or on page 5 of the settings: Windows keys, right Ctrl (as before 1.08) or none. With no joystick, or with `cursors=keys`, the fire key does not fire.

To load a tape: press F1 and then ENTER, choose the `.TAP`, `.TZX`, `.PZX` or `.CSW` and type `LOAD ""` (on the 128K/+2A, choose "Tape Loader" or "Loader" in the menu). The tape starts by itself when the ROM or a custom loader starts reading it, and stops by itself when the program stops reading it like a loader for 2 seconds. What counts is reading like a loader (the same IN over and over, with a counter that advances, as Fuse does), not reading a lot: menus that poll the keyboard non-stop neither start the tape nor keep it running, so multiloads find the next block where they left it. In fast mode, the emulator runs at full speed while loading.

## SD card files from the file browser (F2)

In the file browser (Alt+F5, or F1 and ENTER), **F2** opens a list of what can be done with the selected entry: **Copy**, **Move**, **Rename...**, **Delete...** and **New folder...** (on `[..]`, only the new folder; folders are not copied, but they can be moved). With the GPIO keyboard, F2 is extended mode + 2.

- **Rename** opens a line with the current name and the cursor before the extension. It types lowercase; capitals with Shift or CAPS SHIFT + letter; symbols with Ctrl or SYMBOL SHIFT + the key, as on the Spectrum: SS+8 `(`, SS+9 `)`, SS+0 `_`, SS+J `-`, SS+M `.`, SS+N `,`, SS+K `+`, SS+L `=`, SS+1 `!`, SS+2 `@`, SS+3 `#`, SS+4 `$`, SS+5 `%`, SS+6 `&`, SS+7 `'`, SS+Y `[`, SS+U `]`. The dot and hyphen of a USB keyboard work too. The cursor keys move the cursor, Home and End (CAPS SHIFT + 1 and 2) go to the ends, DELETE (CAPS SHIFT + 0) deletes the character on the left and Del (CAPS SHIFT + 9) the one under the cursor.
- **Delete** always asks, with "No, keep it" selected on entry. Folders only if they are empty.
- **New folder** creates one in the current folder.
- **Copy** and **Move** mark the selected entry (at the bottom it shows "To copy: games/Barbarian.tzx"). Go to the destination folder with the browser and there F2 offers **Paste here** (or **Move here**) and **Forget**. If there is already one with that name, it asks: don't copy it (selected on entry), copy it as "name (2)" or replace the existing one. Pasting a copy in its own folder makes it "name (2)" directly. Moving is immediate; copying shows a progress bar with the MB copied and is cancelled with ESC (BREAK). The copy is written with a temporary name and only gets its own at the end, so if it is cancelled or fails no half-copied file is left; before starting it checks that it fits on the card.
- The **web page** also has "copy" next to "move", and it is pasted the same way ("Paste here"). If there is already one with that name, the copy is made as "name (2)".

The same checks as on the web page apply: system files (kernels, `cmdline.txt`, `config.txt`, the emulator's folders...) are not touched while "System files" is set to "protected" (`webprotect=1`, page 9), and what is in use (a +3 disk, a microdrive, the hard disk, the tape if SAVE writes to it) cannot be renamed or deleted: it says "In use". F2 does not work in TNFS folders or when the browser has been opened to choose a drive's file (Alt+U). "Changes in the browser" (page 8, `filechanges=0`) removes it altogether.

## Simultaneous keys (Alt+V)

Some cheats need many keys pressed together, like **VICTOR** in Profanation for infinite lives. A USB keyboard reports at most six keys at a time, and many cannot detect some combinations of four, five or six keys.

**Alt+V** opens a line where you type the keys (letters, digits and space) and, with ENTER, the emulator holds them all down together for 1.5 seconds. It works with any keyboard. With a real Spectrum keyboard over GPIO or a Recreated ZX it is not needed: all of them can be pressed at once, as on the Spectrum.

If the USB keyboard cannot handle a combination and reports too many keys at once, the emulator keeps the keys it already had pressed until the next valid report, as the USB standard says, instead of releasing them all.

## Pokes and cheats (Alt+C and Alt+O)

**Alt+C** (or P in the F1 help) opens the list of cheats for the loaded game: infinite lives, energy, level skip… They come from a **.POK** file, the format of the ZXDB and The Tipshop pokes, which the emulator looks for in the `pokes` folder of the SD card by the game's name: for `games/Barbarian.tzx`, `pokes/Barbarian.pok` or, as in the ZXDB collection (folders by initial), `pokes/b/Barbarian.pok` (`pokes/123/` if it starts with a digit). If it is not found and there is a network, **it looks for it in ZXDB**: it asks ZXInfo which game the loaded file is by its hash (SHA-512) or, if it was downloaded with the file browser's ZXDB search, by its entry; if ZXDB has pokes for it, the .pok is downloaded from Spectrum Computing to `pokes/<game>.pok` (next time it is already there, even without a network). It is tried once per game. If that fails too, the list says "no .pok" and has two lines at the end to find one: **Open a .pok...** opens the file browser in `pokes`, and **Search ZXDB for pokes...** searches for the game by its title (for files ZXDB does not recognise: modified, from another release, saved by yourself…) and downloads the .pok of the one chosen. A .pok chosen in the file browser also opens the list.

- Each cheat is chosen with its key (1-9, 0, A-Z) or with ENTER, and **it is applied at once**: it is marked `[x]`. Choosing it again removes it and memory goes back to how it was (with the original value in the .POK or, if there is none, the one there was when it was applied).
- Cheats that let you choose a number (lives, level…; value 256 in the .POK) ask for it: 0 to 255, or hexadecimal with H at the end (`C9H`).
- **Apply them with the game already loaded** and running: if applied before, the loading overwrites them. Some games need them in a given part (`[Pt1]`, `[Pt2]`, `[L3]` in the cheat's name).
- The pokes in a .POK are for a particular release of the game: on another one (a re-release, a compilation…) they may do nothing or crash it.
- Pokes with a bank (128K) go to that memory page, whether it is paged in or not. On 48K models the bank is ignored.

**Alt+O** (or the **Manual POKE...** line in the list) opens a line to type a POKE as in the magazines: the address and the value separated by a space (`35136 0` is `POKE 35136,0`), or in hexadecimal with H at the end (`8940H C9H`). Several fit in a row (`35136 0 35137 201`). They are applied at once to memory as it is paged and stay in the Alt+C list, so they can be removed. The address goes from 16384 to 65535: the ROM is not written.

Loading another game empties the list. A reset clears memory: if the game was a snapshot, or its tape, disk or cartridge has been removed, there is no game any more (Alt+C shows as when the emulator starts); if its tape, disk or cartridge is still in, its cheats stay in the list, not applied, for when it is loaded again. There are no pokes on the ZX81.

With the **GPIO keyboard**: extended mode + SYMBOL SHIFT + C opens the pokes and extended mode + SYMBOL SHIFT + O the manual POKE (like every Alt combination). In the list and the input line everything is done with the Spectrum keys: digits and letters, ENTER, BREAK to go back, CAPS SHIFT + 0 to delete and CAPS SHIFT + 5-8 to move.

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
