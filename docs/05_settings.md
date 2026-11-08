# Settings

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## `cmdline.txt` options

On the SD card, the `cmdline.hlp` file (`cmdline-en.hlp` in English) explains all the options and their possible values. Most of them can also be changed in the emulator's settings (F1, C).

All the options go on a single line:

| Option | Values | Default |
| --- | --- | --- |
| `model=` | `16k`, `48k`, `128k`, `plus2`, `plus2a`, `plus3`, `plus3e`, `tk90x`, `tk95` | `128k` |
| `tkula=` | TK90X and TK95 ULA: `50` (Microdigital at 50 Hz: Uruguay), `60` (Microdigital at 60 Hz: Brazil) or `ferranti` (the first export units; 48K timings) | `50` |
| `tkrom=` | TK90X ROM: `v3` (Rodolfo Guerra's, the most compatible), `v1` (original, the most widespread) or `v2` (original; the TK95 always has it). Applied at once: with a TK, it restarts with the new ROM | `v3` |
| `tklang=` | Language of the TK90X and TK95 ROM: `auto` (the menu's), `es`, `en`, `pt`, `ca`, `cs`, `de`, `eo`, `eu`, `gl`, `it`, `nl`, `no`, `pl`, `ro`, `ru`, `sv`, `vl`. The originals only have Spanish and Portuguese (other languages use Spanish): v1 chooses with the diode of the Brazilian TK90X (bit 7 of port 254 at 0 gives Portuguese), v2 and the TK95 with their Spanish or Brazilian ROM. v3 has all 17. Applied at once | `auto` |
| `updboot=` | `1`: check for a new version the first time there is network. Also on page 10 of the settings | `0` |
| `updkeep=` | Packages kept in `update/`: `2` (the installed one and the previous one) or `all` | `2` |
| `updconfig=` | On update, the configuration files: `keep` (kept; the new one is left as `.nuevo`) or `new` (the new one is used; yours is kept as `.bak`) | `keep` |
| `tape=` | `flash` (instant), `fast`, `normal` (real) | `flash` |
| `tapeaccel=` | `1` (accelerate known loaders), `0` | `1` |
| `diskaccel=` | `1` (+3/+3e: accelerated emulation while the disk drive is in use; copy protections are not affected), `0` | `0` |
| `joystick=` | `none`, `kempston`, `fuller`, `sinclair1`, `sinclair2`, `cursor`, `qaop` | `kempston` |
| `sounddev=` | `sndhdmi`, `sndpwm`, `sndusb` (Pi 4/400), `none` | `sndhdmi` |
| `speaker=` | `auto`, `beeper`, `tv`, `off` (beeper speaker model) | `auto` |
| `beeperfilter=` | `1` (removes the 1-bit whine), `0` | `1` |
| `recreated=` | `0` (never), `auto` (through a USB cable it is recognised when connected; otherwise, automatic detection), `1` (every USB keyboard is a Recreated) | `auto` |
| `cursors=` | `both` (cursors = CAPS SHIFT+5-8 and joystick), `keys` (keys only), `joystick` (joystick only) | `both` |
| `kbdfire=` | Keyboard key that acts as joystick fire: `win` (both Windows keys; both Ctrl keys are SYMBOL SHIFT), `rctrl` (right Ctrl, as before 1.08) or `none` | `win` |
| `joystick2=` | Player 2 joystick: same values as `joystick=`; it cannot be player 1's, nor Cursor with Sinclair 1 | `none` |
| `pad1=` | Player 1 gamepad: `auto` (connection order) or a specific gamepad (`vendor:product`, for example `045e:028e`) | `auto` |
| `pad2=` | Player 2 gamepad, likewise | `auto` |
| `padextra=` | The other gamepads: `p1` or `p2` (they join that player) or `menu` (menus only) | `p1` |
| `f5=` | `nmi` (F5 = NMI, Alt+F5 = files), `files` (F5 = files, Alt+F5 = NMI) | `nmi` |
| `aystereo=` | `abc`, `acb`, `bac`, `mono` | `abc` |
| `tapesound=` | `1` (loading is heard), `0` (silent) | `1` |
| `border=` | `none`, `standard`, `full`, `huge` | `full` |
| `scanlines=` | `1` (on), `0` | `0` |
| `pureblack=` | `1` (pure black also in the 16K/48K palette), `0` | `0` |
| `indicators=` | `1` (the D, H, M1-M8 and recording dot at the top of the picture), `0` (none of them; the keyboard extended-mode icon is still shown) | `1` |
| `multicore=` | `1` (image scaling on core 1, Pi 2/3/4), `0` | `1` |
| `issue=` | `3` (Issue 3), `2` (Issue 2, only affects the 16K/48K) | `3` |
| `speed=` | `1`, `2`, `4`, `10` | `1` |
| `ulaplus=` | `1` (ULAplus available), `0` (disabled) | `1` |
| `lang=` | `es` (Spanish), `en` (English), `pt` (Brazilian Portuguese), `pt-pt` (European Portuguese) or any other language in `locales/` | `es` |
| `net=` | `auto` (WiFi if there is a `wpa_supplicant.conf`, otherwise cable), `wifi`, `eth`, `0` | `auto` |
| `tz=` | time offset in minutes (60 = mainland Spain, 0 = UK and Canary Islands) | `60` |
| `dst=` | `1` (automatic European summer time), `0` | `1` |
| `ip=`, `mask=`, `gw=`, `dns=` | fixed IP (without `ip=`, DHCP is used) | — |
| `lec=` | `1` (LEC memory connected; on the 128K models it is used from 48 BASIC), `0` | `0` |
| `if1=` | `auto` (connects when a microdrive is inserted), `1` (always), `0` (never) | `auto` |
| `rs232=`, `zxnet=` | `1`, `0` (Interface 1 RS232 and ZX Net) | `1` |
| `divmmc=` | `0`, `esxdos` | `0` |
| `esxdir=` | folder imported into the esxDOS card, `0` for none | `esxdos_i` |
| `spectranet=` | `1`, `0` | `0` |
| `savefmt=` | `tap`, `tzx` (format of SAVE recordings) | `tap` |
| `snapformat=` | `z80`, `sna`, `szx` (F8 format) | `z80` |
| `savetrap=` | `1` (SAVE writes to `saves/` on the SD card), `0` (disabled) | `1` |
| `multiface=` | `1` (active if its ROM is on the SD card), `0` (disabled) | `1` |
| `gpioaudio=` | `1` (with `sounddev=sndpwm`, audio also comes out of GPIO12, in mono), `0` | `1` |
| `gpiokbd=` | `48`, `plus3` (GPIO keyboard type), `0` (none) | `48` |
| `logdev=` | `ttyS1` (messages through the serial port) | |

If the chosen sound device fails, the emulator automatically tries the other one.

## Languages

The emulator is in **Spanish** (the main language, built into the program), **English** and **Portuguese**, Brazilian and European. Choose it with `lang=en` in `cmdline.txt` or on page 1 of the settings (F1, C), where the change is seen at once and saved with G. The web page changes too.

- The translations are in the `locales/` folder of the SD card (visible in the web page, so they can be downloaded and uploaded): one file per language, `en.json`, with the Spanish text as the key and its translation as the value. If a translation is missing (or empty, `""`), the Spanish text is shown.
- `es.json` is only a reference: the full list of texts, for anyone who wants to translate into another language. To add one, copy `en.json` to, for example, `fr.json`, change `"_idioma"` to `"Français"` and translate the values: the emulator finds it by itself and it appears in the settings.
- The menus have the European letters: accents, diaeresis, ñ, ç, ¡¿, «», €, æ, ø, ß and the most common Central European ones (č, š, ž, ł, ő…).
- The documentation is in Spanish and English: `README_es.md` / `README.md`, `cmdline.hlp` / `cmdline-en.hlp`, and the `LEEME.txt` / `README.txt` files. The log (`logdev=`) stays in Spanish.

**Keeping the translations up to date** (for developers): every text shown on screen is written in Spanish inside `T ("…")`, or inside `N_ ("…")` in tables (and translated with `T ()` when used). `build.sh` runs `tools/i18n/actualizar.py`, which:
- rebuilds `locales/es.json`;
- adds the new texts, untranslated, to the other languages;
- keeps the ones no longer used as `"_huerfano|text"` (in case the Spanish only changed slightly);
- warns about what is missing and about translations whose `%u`, `%s`… do not match the Spanish. It only warns: the build carries on.

## Settings in the emulator

From the help screen (F1), the **C** key opens the settings: ten pages with the `cmdline.txt` options that can be changed without editing the file. Options that are not used with the current choices are shown in grey and the cursor skips them: they cannot be changed until they make sense again (for example, after choosing another model): for example, the Issue 2/3 keyboard outside the 16K and 48K, the +3e language and hard disk on the other models, the SAVE format without SAVE to the SD card, or the netmask, gateway and DNS with automatic IP.

| Page | Options |
| --- | --- |
| 1 Machine | language, model, Issue 2/3 keyboard, speed, ULAplus, +3e language*, boot hard disk* |
| 2 Video and sound | border, scanlines, AY sound, hear loading, speaker, remove 1-bit whine, sound output*, sound on GPIO12*, video on core 1* |
| 3 Tapes and saving | loading mode, SAVE to the SD card, SAVE format, F8 format |
| 4 Keyboard | F5 / Alt+F5, Recreated ZX, GPIO keyboard* |
| 5 Joysticks and pads | players 1 and 2 joysticks, each player's gamepad, extra gamepads, PC cursor keys, keyboard fire, gamepad buttons (see [04_usb_gamepads.md](04_usb_gamepads.md)) |
| 6 Peripherals | Spectranet, divMMC with esxDOS, LEC memory, Interface 1, Multiface, RS232*, ZX Net* |
| 7 Network, time and tape | network*, time zone*, summer time*, fixed IP*, mask*, gateway*, DNS*, check HTTPS*, real cassette*, invert cassette*, cassette GPIO* |
| 8 ROMs and folders | ROM of each machine*, of the Interface 1*, of esxDOS* and the Spectranet flash* (those in the `roms` folder with the right size), esxDOS folder* |
| 9 Web server | web page* (yes, read-only or no) and its password* |
| 10 Update | check and install, saved package, check at boot, saved packages, settings on update |

- Up and down to choose; **left and right** (or ENTER) to change the value; **PgUp and PgDn** (or Tab) to change page.
- **For options with many values** (model, TK ROM language, joysticks, time zone, cassette GPIO), **ENTER opens the whole list**, with the current value highlighted: choose with the cursor keys (or 1-9) and ENTER to go back to the settings; ESC goes back without changing it. The arrows still go one by one.
- **IP addresses are typed in**: ENTER on "Fixed IP", "Network mask", "Gateway" or "DNS server" and type the digits and dots (also on the numeric keypad); Backspace deletes, DEL empties it (automatic, with DHCP), ENTER accepts it if it is a valid IP and ESC leaves it as it was.
- The ROM and hard disk lists only show files without spaces in their names (`cmdline.txt` separates options with spaces).
- Changes apply **at once**, except those marked with `*`, which need the Pi to be restarted.
- **G** saves the values in `cmdline.txt`. It keeps the file's other options (`hdd=`, `ip=`, `logdev=`…) and their order, changes the ones already there and adds those that are not at their default value. It leaves a copy in `cmdline.bak` first.
- **R** restarts the Raspberry Pi. It asks for confirmation ("No" is the highlighted option) and, if there are unsaved changes, offers to save them first. Before restarting, everything pending is saved: +3 disks, microdrive cartridges, the Spectranet flash, what esxDOS wrote and the RS232 output.
- The settings show what is in `cmdline.txt` (read again when opened if there are no pending changes, in case it was changed from the web page), plus whatever is changed with the keys: F4, F6, F7, F9, F10, Alt+B, Alt+S and the peripherals (Alt+T/X/L). So G also saves those changes. What changes by itself, such as the model when loading a 128K snapshot, does not go into the settings, so it is not saved by accident. If something is unsaved, the screen says so.

## Hiding files: excluir.txt

`excluir.txt`, in the root of the SD card, says which files and folders are **not shown** in the file browser (F1) or the web page. It comes ready to hide what the Raspberry Pi needs to boot: `bootcode.bin`, `start*.elf`, `fixup*.dat`, the `kernel*.img` files (which would otherwise appear in the browser as hard disk images), `armstub*.bin`, the `*.dtb` files, and the `firmware` and `certs` folders.

Format, one entry per line (anything after `#` is ignored):
- If it ends in **`/` or `\`**, it is a **folder**; otherwise, a file.
- If it **starts** with `/` or `\`, it is a path from the root of the SD card (it can have several folders: `/games/old/`); otherwise, it applies to that name in any folder.
- The wildcards `*` and `?` are allowed, and it is not case-sensitive.

The file is read again every time the browser opens, so changes (including those made from the web page) apply without restarting.

## Misspelt options in cmdline.txt

A misspelt value in `cmdline.txt` (for example `sounddev=sndpwn`) gives no error: the emulator uses the default value. At boot, ZXMetalCircle checks the file and shows a warning on screen for about 10 seconds ("sounddev=sndpwn not valid"; with "(+N)" if there are more problems). It also warns about options that do not exist and about a `cmdline.txt` with more than one line (the Raspberry Pi only reads the first). All problems are written to the log (`logdev=ttyS1`).
