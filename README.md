# ZXMetalCircle

[*Leer en español*](README_es.md)

*The Spanish README (`README_es.md`) is the reference; if they differ, it is the one that is up to date. Change history (in Spanish): [`CHANGELOG.md`](CHANGELOG.md).*

**Usage guides** (building, writing programs and saving to tape, GPIO pins, USB gamepads...): the [`docs`](docs/README.md) folder.

(The project was first called ZXCircle: the files and the code keep that name.)

Bare-metal ZX Spectrum emulator (16K to +3e) for the Raspberry Pi, written in C++ on top of Rene Stange's [Circle](https://github.com/rsta2/circle) library.

It does not need Linux: the Pi boots straight into the emulator from the SD card.

Author: **Shaeon (Carlos Palmero)**, © 2026.

## Thanks

- To **José Luis Sánchez**, for **JSpeccy** and **ZXBaremulator**, which inspired this project, and for **z80cpp**, the Z80 CPU it uses.
- To **Rene Stange**, for the **Circle** library, on which the whole emulator is built.
- To the **RetroWiki** group (https://www.retrowiki.es), for their help and for keeping the community alive.
- To **Jordi** (https://jbinary.itch.io), for his advice about the Spectrum.
- To **Víctor Iborra (EremusOne)**, for **ESPectrum** (https://github.com/EremusOne/ESPectrum), the Spectrum emulator for ESP32.
- To **Rodolfo Guerra**, for the TK90X v3 ROMs in 17 languages, included with his permission, and for his help with the TK90X and TK95 (https://sites.google.com/view/rodolfoguerra).

The licence and the credits can also be seen in the emulator itself: F1 and then **A**.

## Credits and origin of the code

- **Interface 1 ZX Net network**: a translation of SpectRusty's (© 2020-2022 Rafal Michalski, LGPL 3 or later). **RS232**: based on Fuse's (Gergely Szász, GPL 2 or later).
- **Beeper speaker model**: from Fuse (Fredrick Meunier: `ula_filter.c`, `speaker_filter.c`, `tv_filter.c`, GPL 2 or later).
- **Loader acceleration**: adapted from Fuse (Philip Kendall and others: `loader.c`, `loader_detector.c`, GPL 2 or later).
- **Instant loading and tapes**: the LD-BYTES trap and the registers it leaves follow Fuse (`tape_traps.c`); the polarity of TZX blocks follows libspectrum; the PZX format follows the pzxtools specification (Patrik Rak).
- **Protected disks**: the weak area of the +3 Speedlock follows SAMdisk (Simon Owen, MIT, `IsSpectrumSpeedlockTrack`); CRC errors in the header and the weak stretch of sectors with several copies follow Fuse (`disk.c`).
- **Spectranet** © Dylan Smith (https://spectrum.alioth.net), firmware under the MIT licence; the card emulation is based on Fuse's (Philip Kendall and others, GPL v2 or later).
- **esxDOS 0.8.9** © 2005-2021 Papaya Dezign (https://esxdos.org): ROM and system files, unmodified, for non-commercial use.
- **Mbed TLS** 3.6 (Apache-2.0), **jsmn** (MIT, Serge Zaitsev), **ZXInfo API** (api.zxinfo.dk) and **Spectrum Computing** (ZXDB). **Mozilla** root certificate list (MPL-2.0).
- **Multiface ROMs** © Romantic Robot UK Ltd: **not included**, as we have no permission to distribute them (permission has been requested). The Multiface emulation works with the dump of your own Multiface's ROM in `roms/` (`mf1.rom`, `mf128.rom`, `mfplus3.rom`).
- **+3e ROMs** by Garry Lancaster (https://worldofspectrum.org/zxplus3e/), version 1.43 for the ZXMMC; free distribution with Amstrad's permission.
- **zxp3** by Kyp (https://github.com/Kyp069/zxp3) and **MiST** (`sd_card.v`): reference for the ZXMMC and the SD card.
- **MAME** (https://github.com/mamedev/mame): uPD765 controller and floppy PLL (BSD-3-Clause, Olivier Galibert); the .DSK reading and the MFM track generation follow its code. Licence text in `third_party/mame/LICENSE`.
- **Z80 CPU**: [z80cpp](https://github.com/jsanchezv/z80cpp) by José Luis Sánchez (GPL 3), unmodified, in `third_party/z80cpp`.
- **ULA, contention, floating bus, paging and AY-3-8912**: ported to C++ from José Luis Sánchez's [JSpeccy](https://github.com/jsanchezv/JSpeccy).
- **ROMs**: JSpeccy's (Spectrum and Interface 1). Amstrad allows the Spectrum ROMs to be distributed for use in emulators.
- **Circle**: Rene Stange (GPL 3).

## AI assistance and resources

Parts of this emulator were developed with the assistance of Claude (Anthropic), used for code, tests, debugging and documentation. Access to Claude was provided by Advanced Computer Trading, S.L. (actsl.com), which also authorises the publication of this work under the license below. All design decisions, integration, testing on real hardware and final review were carried out by the author, Shaeon (Carlos Palmero).

## License

Copyright (C) 2026 Shaeon (Carlos Palmero).

This program is free software: you can redistribute it and/or modify it under the terms of the **GNU General Public License** as published by the Free Software Foundation, either **version 3** of the License, or (at your option) any later version. See [LICENSE](LICENSE).

Version 3 is required because the Circle library and z80cpp are licensed under the GPL v3. Third-party code keeps its own copyright notices and licenses, all compatible with the GPL v3: Fuse (GPL v2 or later), SpectRusty (LGPL v3 or later), MAME uPD765 and floppy PLL (BSD-3-Clause), Mbed TLS (Apache-2.0), jsmn (MIT), Spectranet firmware (MIT).

The ROMs and system files distributed with the emulator are not covered by the GPL and belong to their respective owners: the Sinclair and Amstrad ROMs are distributed with Amstrad's permission for use in emulators; the +3e ROMs with Garry Lancaster's permission; esxDOS (Papaya Dezign) for non-commercial use; the LEC CP/M by Jiří Lamač, free for non-profit use with credit. The Raspberry Pi firmware and the WiFi firmware are downloaded by `build.sh` and keep their own licences (`LICENCE.broadcom` and the files in `firmware/`).

## Disclaimer

This project is provided **"as is", without warranty of any kind**, express or implied, including but not limited to the warranties of merchantability and fitness for a particular purpose. In no event shall the authors or contributors be liable for any claim, damages or other liability arising from the use of this software, of the hardware it runs on, or of any connected equipment (including anything wired to the GPIO pins). Use it at your own risk.

ZX Spectrum and Sinclair are trademarks of their respective owners; Raspberry Pi is a trademark of Raspberry Pi Ltd. This project is not affiliated with or endorsed by them.

## Status (version 1.07)

Done:

- 16K, 48K, 128K, +2 (grey), +2A, +3 and +3e models, with JSpeccy's timings.
- Memory and I/O contention, floating bus and 0x7FFD reads on the 128K.
- Full border and screen redrawn according to the t-state, so multicolour and border effects work.
- ULAplus: 64-colour mode, including palette changes in mid-frame.
- Selectable border size (none, standard, full, huge) and scanlines.
- Beeper and AY-3-8912 at 48 kHz, through HDMI or the 3.5 mm jack (PWM). The AY has selectable stereo (ABC, ACB, BAC or mono). The beeper goes through a speaker model taken from Fuse.
- Loading sound: the tape signal is heard through the speaker, as on the real Spectrum.
- Issue 2 or Issue 3 keyboard and emulation speed x1, x2, x4 or x10.
- TAP, TZX, PZX and CSW tapes with three loading modes:
  - **instant**: trap in the ROM's LD-BYTES, as in Fuse: at 0x0562 (it works whether the routine is entered at 0x0556 or straight half-way, as Ocean's Cabal loader does). It leaves the registers as the ROM does (the same as Fuse's `tape_traps.c`) and returns through SA/LD-RET, which restores the border and enables interrupts;
  - **fast**: real pulse loading at top speed, with the stripes visible;
  - **real**: pulse loading at normal speed.
- **Model**: if a TZX says in its hardware info block (0x33) that it does not run on the current model, on inserting it you are asked whether to switch to one where it runs (for example, from 128K to 48K).
- **"Turbo" blocks with ROM timings**: they also load instantly, as in JSpeccy.
- **Block browser (F3)**: lists what is on the tape, like Fuse's: the ROM headers ("Program:", "Bytes:" with name, address and length) and their data, the turbo blocks, tones, pulses and pauses, and in another colour what helps to find your way in a multiload: the TZX texts and groups ("Level 2") and the "STOP THE TAPE" blocks. The ">" marks where the tape is. ENTER leaves it stopped at the start of the chosen block: instant loading reads from there, and in fast or real mode it starts by itself when the program reads it. Home and End go to the first and last block.
- **TZX as in Fuse (libspectrum)**: the first pulse of a block that follows a pause, or at the start of the tape, is low level (this matters to loaders that measure each bit as low plus high level, such as Basil the Great Mouse Detective's); a pause of 0 does not change the level; stop blocks (0x20 with pause 0, and 0x2A on the 16K and 48K) stop the tape, but the loader can start it again, because multiloads use them between levels; at the end of the tape, it cannot.
- **Loader acceleration** (`tapeaccel=1`, default), adapted from Fuse (`loader.c` and `loader_detector.c`, by Philip Kendall, GPL 2 or later): in fast loading, and in blocks instant loading cannot take, if the loop waiting for each tape edge is a known one (the ROM's and its copies in RAM, Microprose, Alkatraz, Digital Integration, the search loaders), during the data the edge is brought forward and the registers are left as if the loop had found it. Pilot tone and sync pulses are emulated normally. It also works with many turbo loaders. With Cabal (Ocean, 128K) in fast loading, the emulated frames drop from 31,031 to 12,455; with a test tape with a standard and a turbo block, from 4,710 to 2,213, with no wrong bytes (`test/host/accel`).
- Automatic tape start and stop.
- TZX:
  - signal blocks: 0x10 (standard), 0x11 (turbo), 0x12 (tone), 0x13 (pulses), 0x14 (pure data), 0x15 (direct recording), 0x18 (CSW, uncompressed or Z-RLE) and 0x19 (generalised data);
  - control blocks: 0x20 (pause/stop), 0x23 (jump), 0x24/0x25 (loops), 0x26/0x27 (call sequence and return), 0x28 (select), 0x2A (stop if 48K), 0x2B (level) and the informative ones;
  - with a **0x28** block (tapes with several parts or versions), the tape stops and a list appears to choose what to load: cursor keys and ENTER, or the option's number; ESC carries on with the next block;
  - non-standard blocks are always loaded by pulses, even if instant loading is selected.
- Standalone .CSW files (versions 1 and 2, RLE and Z-RLE).
- .PZX files (Perfect ZX Tape): pulses (PULS), data with its initial level (DATA), pauses (PAUS), texts (BRWS) and stops (STOP). They keep the real polarity of the signal, which TZX does not: some loaders need it (Forbidden Planet). Data blocks with ROM timings also load instantly.
- **Polarity**: in the block browser (F3), the **P** key inverts the tape signal. It is for TZX files that have it the wrong way round (with Forbidden Planet's, the loader slips half a bit and never finishes). It goes back to normal when another tape is inserted.
- Interface 2 cartridges (.ROM).
- Interface 1 with up to 8 microdrives (.MDR and .MDV cartridges, read and write), RS232 and ZX Net network.
- 528 KB LEC memory expansion (48K, and from 48 BASIC on the 128K models), with which LEC's CP/M 2.2 on microdrives works (included in `cpm/`).
- Saving with SAVE: the blocks saved by the ROM go to .TAP files in the SD card's `saves` folder.
- Snapshots: .Z80 (versions 1, 2 and 3), .SNA (48K and 128K) and .SZX loading; saving as .Z80 v3, .SNA or .SZX with F8.
- Recording of the tape signal (MIC output) with Alt+W, for programs with their own saving routines.
- Spectrum +3 with disk drive: .DSK disks (standard and extended, with protections: weak sectors, Speedlock, long tracks, CRC errors) and 720K disks (the drive becomes a 3.5" one by itself), with MAME's uPD765 controller.
- Spectrum +3e (Garry Lancaster's ROMs) with a hard disk on a ZXMMC card: .VHD, .IMG, .RAW and .HDF images.
- divMMC with esxDOS 0.8.9: its card is an image (`hdd/esxdos.img`) and whatever is left in the `esxdos_i` folder is copied into it; divMMC ports on the +3e.
- Spectranet network card, with the official firmware.
- Your own ROMs for any machine (`roms` folder of the SD card).
- Network over WiFi or cable: web server to manage the SD card from a browser (it can be turned off, made read-only or protected with a password), time by NTP, and search and download of programs from ZXDB over HTTPS.
- Settings screens in the emulator (F1, C), with saving to `cmdline.txt` and Pi restart.
- Menus and web page in Spanish and English (more languages can be added without recompiling).
- On the 4-core Pis, the image scaling runs on core 1.
- SD card file browser with folders and jump by letter; when reopened, the cursor stays on the last file chosen.
- Hot-plug USB keyboard.
- Tape input from a real cassette player, through a GPIO (with a comparator) or a USB sound card (Pi 4/400), with recording to .TZX (Alt+G).
- Real Spectrum keyboard and two Atari joysticks through GPIO, with pastbytes' wiring for ZXBaremulator: 48K, Spectrum+, 128 and grey +2 keyboards (8 × 5) or the +2A/+2B/+3 one (13 × 11), chosen with `gpiokbd=` (see below).
- Kempston, Fuller, Sinclair 1/2, Cursor/AGF/Protek and QAOPSpaceM joysticks, used with USB gamepads or the keyboard's cursor keys. Kempston is decoded by A5 only, like the original interface, so it also works with games that do not read port 0x1F (Las Tres Luces de Glaurung, Silent Shadow).
- Hot-plug USB gamepads (up to 4):
  - with a known mapping: PS3, PS4 DualShock, Xbox 360, Xbox One, Switch Pro and 8BitDo;
  - generic HID gamepads, including Mayflash adapters in DirectInput mode.
- Kempston mouse (with wheel, like the Kempston Mouse Turbo) using a USB mouse.
- Recreated ZX Spectrum USB keyboard in the game layer (Layer A): through a cable it is recognised when connected (VID:PID), otherwise by automatic detection.
- Multiface One (48K), Multiface 128 (128K) and Multiface 3 (+2A). Triggered with F5 or with the GPIO NMI button.
- Options in `cmdline.txt`.

**RS232** (`FORMAT "t";9600`, `OPEN #4;"t"`, `PRINT #4`, `INPUT #4`, and `OPEN #3;"t"` so that `LPRINT` and `LLIST` go through it):
- The Pi acts as a terminal on **TCP port 2323**: connect from the PC with `telnet Pi-IP 2323`, `nc IP 2323` or PuTTY in "Raw" mode. What the Spectrum sends comes out there, and what you type reaches it.
- If nobody is connected, the output is appended to `saves/rs232.txt` on the SD card: so `LLIST` with `OPEN #3;"t"` "prints" the listing to a file.
- It is emulated as in Fuse: the IF1 ROM sends and reads the bits one by one through port 0xF7, and the bytes are rebuilt by counting accesses, regardless of the chosen speed. The IF1 always sees a ready terminal (DTR).

**ZX Net network** (`FORMAT "n";1`, `SAVE *"n";2`, `LOAD *"n";1`, `OPEN #4;"n";2`…):
- The Pis running ZXMetalCircle on the same local network form a Spectrum network: packets travel over **UDP on port 5050**, by broadcast.
- The emulation is SpectRusty's (Rafal Michalski, LGPL 3), translated to C++: it recognises each byte of the packets (8-byte header and up to 255 data bytes) by the timing of the accesses to port 0xF7 and sends them whole; the UDP format is SpectRusty's, so it should also talk to it.
- `rs232=0` and `zxnet=0` disable them (also in the settings, peripherals page). The Interface 1 must be connected (`if1=1`).

**Tested on the PC**: a program with `FORMAT "t";9600`, `OPEN #4;"t"`, `PRINT #4;"HOLA RS232"` and `INPUT #4;a$` sends "HOLA RS232" + CR + LF and receives "ZX OK"; and two emulated Spectrums joined in memory pass a program with `SAVE *"n";2` on station 1 and `LOAD *"n";1` on station 2. TCP and UDP still have to be tested on the Pi.

## Building

How to set up the compiler on Linux or Windows and generate the SD card: [docs/01_building.md](docs/01_building.md).

## 32 and 64 bits

The same code is built for 32 bits (AArch32) and 64 bits (AArch64). `config.txt` picks the kernel for each model:

- **Pi 1, Zero and Pi 2**: 32 bits.
- **Pi 3, Zero 2 W, 4 and 400**: 64 bits. The 32-bit kernels are also on the SD card: to use them, set `arm_64bit=0` and the 32-bit kernel in the `[pi3]`, `[pi02]` or `[pi4]` section of `config.txt` (the file itself explains the lines).
- **Pi 5 and 500**: 64 bits (Circle does not support 32 bits on them). First tests on a real Pi 5 (0.76): it boots, USB keyboard and help work. Differences from the other models:
  - **Single-page image**: the Pi 5 firmware does not flip pages (`SetVirtualOffset`) and showed both, one above the other and small. The image is drawn straight into the visible page; some tearing may be seen.
  - **The CPU scales the image**: the GPU does not scale the framebuffer (it shows it as it is, at the top left), so the framebuffer is always at the display resolution and the CPU does all the scaling, with or without scanlines. With scanlines and full border at 1080p, core 0 takes 7.9 ms per frame including the image.
  - **Sound through HDMI0** (the connector next to the power input): Circle's HDMI sound does not work on HDMI1. With no jack, PWM goes out through GPIO12 and GPIO13, which are GPIO keyboard lines, so it is only used with `gpiokbd=0` (before, on HDMI1, PWM made the keyboard look as if it were typing by itself).
  - No multi-core yet: the image is scaled on core 0.

**No tearing**: on the Pi 1 to 4 the image is drawn with triple buffering (three pages that take turns), so it is never drawn into the page being shown or the one about to be shown, and no tearing is seen at normal speed. At x2 speed or more some may be seen. The Pi 5 has a single page (see above).

**Smooth scrolling (optional)**: the Spectrum produces 50 frames per second and many screens run at 60 Hz, so some frames last longer than others and scrolling judders. If your screen accepts 50 Hz, it can be forced in `config.txt`, for example `hdmi_group=1` and `hdmi_mode=31` (1080p at 50 Hz) or `hdmi_mode=19` (720p at 50 Hz).

Both versions have the same Spectrum features. In 64 bits the code is somewhat larger and the compiler has more registers to optimise the Z80 with; the real speed difference is still to be measured on a Pi with Alt+D.

**Testing in QEMU**: `QEMU=1 ./build.sh "1 3-64"` leaves in `qemu/` kernels built with `ZX_QEMU` (no sound, because QEMU does not emulate HDMI or PWM and Circle hangs when trying, and a fixed IP for QEMU's user network). The Pi Zero is tested in 32 bits (`qemu-system-arm -M raspi0 -bios qemu/kernel.img`, no options) and the Pi 3 in 64 bits (`qemu-system-aarch64 -M raspi3b -kernel qemu/kernel8.img`, with the `cmdline.txt` options in `-append`). Those kernels do not go on the SD card.

- **Tested**: the 64-bit Pi 3 kernel in QEMU (`qemu-system-aarch64 -M raspi3b`), with the image on core 1, the USB keyboard, the F1 help, the 48K and the +3. It still has to be tested on a real Pi 3 and 4, and the Pi 5 entirely.

## Installing

1. Format an SD card as FAT32.
2. Copy the whole content of `sdcard/` (or of the release's `ZXMetalCircle-x.xx-sd.zip`) onto it.
3. Also copy your `.TAP` files, organised in folders as you like.
4. Connect a USB keyboard and an HDMI screen, and power on the Pi.

**To update** to a new version, copy the contents of `sdcard/` over your SD card. The kernels, `config.txt`, `cmdline.txt` and the help files are replaced, and your games, `saves/`, `snaps/`, `descargas/` and the esxDOS card (`hdd/esxdos.img`, which new versions do not include) are not touched. If you have your own changes in `cmdline.txt` (or for the WiFi), keep a copy first and put them back.

## Usage guides

Everything about how to use the emulator is in the [`docs`](docs/README.md) folder, in English and Spanish:

| # | Guide | Contents |
| --- | --- | --- |
| 01 | [Building](docs/01_building.md) | building on Linux and Windows (WSL), testing in QEMU |
| 02 | [Writing programs and saving to tape](docs/02_basic_and_tape.md) | BASIC programs, `SAVE` to the SD card, loading them |
| 03 | [GPIO pins](docs/03_gpio.md) | pins, real keyboard and joysticks through GPIO, real cassette |
| 04 | [Joysticks and USB gamepads](docs/04_usb_gamepads.md) | two players, which gamepad belongs to whom, button profiles |
| 05 | [Settings](docs/05_settings.md) | `cmdline.txt` options, the settings (Alt+F1), languages, `excluir.txt` |
| 06 | [Keyboard and mouse](docs/06_keyboard_and_mouse.md) | keys, several keys at once, diagnostics, Recreated ZX, Kempston mouse |
| 07 | [Files and tapes](docs/07_files_and_tapes.md) | drives, ZIP files, snapshots, Interface 2 cartridges |
| 08 | [Disks](docs/08_disks.md) | +3 and .DSK disks, +3e and hard disk, divMMC with esxDOS |
| 09 | [Peripherals](docs/09_peripherals.md) | Spectranet, Interface 1 and Microdrives, LEC and CP/M, Multiface, ULAplus |
| 10 | [Network](docs/10_network.md) | WiFi, web server and time, searching and downloading programs, TNFS |
| 11 | [Picture and sound](docs/11_picture_and_sound.md) | border and scanlines, sound, Issue 2/3 and speed, several cores |
| 12 | [Models and ROMs](docs/12_models_and_roms.md) | TK90X and TK95, the ROMs |
| 13 | [Updating the emulator](docs/13_updating.md) | updating from GitHub (page 10 of the settings) |

## Structure

```
src/kernel.*       boot, main loop, audio sync, function keys, settings
src/main.cpp       entry point (creates the kernel and starts it)
src/spectrum.*     machine: memory, ULA, ports, contention, tape traps
src/ay8912.*       AY-3-8912 sound chip
src/audiomixer.*   sample generation (beeper + AY) per t-state, speaker model
src/tape.*         TAP/TZX/PZX/CSW tape (pulses, instant loading and blocks)
src/inflate.*      zlib decompressor (CSW Z-RLE blocks and .SZX pages)
src/zipfile.*      ZIP file reader (central directory, deflate and CRC)
src/snapshot.*     .Z80/.SNA/.SZX snapshots
src/interface1.*   Interface 1, microdrives (.MDR and .MDV) and RS232
src/zxnet.*        Interface 1 ZX Net network (from SpectRusty)
src/if1link.*      RS232 (TCP 2323) and ZX Net (UDP 5050) over the Pi's network
src/spectranet.*   Spectranet card (flash, W5100) and snetcircle.* (Circle sockets)
src/esxfolder.*    esxDOS card (image) and import from the esxdos_i folder
src/settings.*     settings screens (F1, C) and saving to cmdline.txt
src/optcheck.*     checks cmdline.txt and warns about unknown options or values
src/exclusions.*   files and folders that are not shown (SD:/excluir.txt)
src/i18n.*         languages (locales/*.json)
src/videocore.*    image scaling on core 1 (Pi 2/3/4)
src/network.*      network: WiFi/cable, NTP, summer time, mDNS
src/earinput.*     EAR input from a real cassette; earsource.* (GPIO with a comparator or USB sound card)
src/webserver.*    web server to manage the SD card
src/plus3disk.*    +3 disk drive (wrapper around MAME's code)
src/sdcard.*       SD card in SPI mode (+3e ZXMMC, divMMC)
src/harddisk.*     hard disk image in a file (VHD, IMG, HDF) with fast seek
src/tnfs.*         TNFS client (read) with a pluggable transport
src/tnfsbrowser.*  TNFS servers in the file browser (Circle UDP)
src/download/      HTTPS client, ZXDB search, ZIP; circleglue.* joins Mbed TLS with Circle
third_party/mbedtls/ Mbed TLS 3.6 (zxmc_config.h: reduced configuration)
third_party/mame/  MAME's uPD765 and PLL, compatibility layer, .DSK images
src/zxkeyboard.*   USB keyboard -> Spectrum matrix and joysticks
src/recreated.*    Recreated ZX Spectrum keyboard (game layer, by VID:PID or detection)
src/gpiokeyboard.* real keyboard and joysticks through GPIO
src/video.*        GPU framebuffer with triple buffering and text drawing (UTF-8, European letters)
src/palette.h      Spectrum and ULAplus colours (RGB565)
src/menu.*         file browser, help, settings and credits screens
src/roms.S         embedded ROMs
tools/i18n/        actualizar.py: keeps the translations up to date
```
