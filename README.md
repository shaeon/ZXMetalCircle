# ZXMetalCircle

[*Leer en español*](README_es.md)

*The Spanish README (`README_es.md`) is the reference; if they differ, it is the one that is up to date. Change history (in Spanish): [`CHANGELOG.md`](CHANGELOG.md).*

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

## Status (version 1.05)

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

You need both Arm GNU Toolchains (tested with 14.2): `arm-none-eabi` for the 32-bit kernels and `aarch64-none-elf` for the 64-bit ones, plus `git`, `wget`, `make`, `python3`, `mtools` and `dosfstools`:

```
./build.sh              # builds every kernel and leaves everything in sdcard/
./build.sh 3-64         # a single one
./build.sh "1 3-64 5"   # several
```

**On GitHub** (`.github/workflows/build.yml`) it builds by itself: on every push to `main` and on pull requests it runs the PC tests, builds the seven kernels and leaves the SD card as a zip in the *Actions* tab (*Artifacts* of each run). It can also be started by hand (*Actions → Build → Run workflow*). When a `vX.YY` tag is pushed it also creates a draft release with that zip: you only have to paste the notes and publish it.

The script downloads Circle and the Raspberry Pi firmware, builds the Circle libraries for each target (with multi-core support on the Pi 2, 3 and 4) and generates these kernels:

| Target | Model | Bits | Kernel |
| --- | --- | --- | --- |
| `1` | Pi 1 / Zero / Zero W | 32 | `kernel.img` |
| `2` | Pi 2 | 32 | `kernel7.img` |
| `3-32` | Pi 3 / Zero 2 W | 32 | `kernel8-32.img` |
| `4-32` | Pi 4 / 400 | 32 | `kernel7l.img` |
| `3-64` | Pi 3 / Zero 2 W | 64 | `kernel8.img` |
| `4-64` | Pi 4 / 400 | 64 | `kernel8-rpi4.img` |
| `5` | Pi 5 / 500 | 64 | `kernel_2712.img` |

The compiler prefixes can be changed with `PREFIX32=` and `PREFIX64=`.

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

## `cmdline.txt` options

On the SD card, the `cmdline.hlp` file (`cmdline-en.hlp` in English) explains all the options and their possible values. Most of them can also be changed in the emulator's settings (F1, C).

All the options go on a single line:

| Option | Values | Default |
| --- | --- | --- |
| `model=` | `16k`, `48k`, `128k`, `plus2`, `plus2a`, `plus3`, `plus3e` | `128k` |
| `tape=` | `flash` (instant), `fast`, `normal` (real) | `flash` |
| `tapeaccel=` | `1` (accelerate known loaders), `0` | `1` |
| `joystick=` | `none`, `kempston`, `fuller`, `sinclair1`, `sinclair2`, `cursor`, `qaop` | `kempston` |
| `sounddev=` | `sndhdmi`, `sndpwm`, `sndusb` (Pi 4/400), `none` | `sndhdmi` |
| `speaker=` | `auto`, `beeper`, `tv`, `off` (beeper speaker model) | `auto` |
| `beeperfilter=` | `1` (removes the 1-bit whine), `0` | `1` |
| `recreated=` | `0` (never), `auto` (through a USB cable it is recognised when connected; otherwise, automatic detection), `1` (every USB keyboard is a Recreated) | `auto` |
| `cursors=` | `both` (cursors = CAPS SHIFT+5-8 and joystick), `keys` (keys only), `joystick` (joystick only) | `both` |
| `f5=` | `nmi` (F5 = NMI, Alt+F5 = files), `files` (F5 = files, Alt+F5 = NMI) | `nmi` |
| `aystereo=` | `abc`, `acb`, `bac`, `mono` | `abc` |
| `tapesound=` | `1` (loading is heard), `0` (silent) | `1` |
| `border=` | `none`, `standard`, `full`, `huge` | `full` |
| `scanlines=` | `1` (on), `0` | `0` |
| `pureblack=` | `1` (pure black also in the 16K/48K palette), `0` | `0` |
| `multicore=` | `1` (image scaling on core 1, Pi 2/3/4), `0` | `1` |
| `issue=` | `3` (Issue 3), `2` (Issue 2, only affects the 16K/48K) | `3` |
| `speed=` | `1`, `2`, `4`, `10` | `1` |
| `ulaplus=` | `1` (ULAplus available), `0` (disabled) | `1` |
| `lang=` | `es`, `en` or any language in `locales/` | `es` |
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

## Spectranet (network card)

Dylan Smith's **Spectranet** (https://spectrum.alioth.net) gives the Spectrum a real network: it mounts **TNFS** servers from BASIC, opens TCP and UDP connections, and is used by programs such as IRC clients, browsers or demos that say "Spectranet not found" if they cannot find it.

**Turning it on**: add `spectranet=1` to `cmdline.txt`, or connect it at any time with **Alt+T** or in the settings (F1, C, page 5). The Pi needs a network (WiFi or cable).
- At boot you see "Alioth Spectranet", "Link.>OK", and the IP on the start-up screen. The Spectranet uses the Pi's network: its DHCP request is answered by the emulator itself with the Pi's IP, so there is nothing to configure. If the WiFi takes a while to connect, the Spectranet waits.
- **F5** (NMI) opens its menu: network settings, ROM modules, load data and snapshot manager.
- From BASIC, for example:
  - `%mount 0,"tnfs.markround.com"` and `%cat` to browse a TNFS server;
  - `%tapein "game.tap"` and `LOAD ""` to load a tape from the server;
  - `%connect #4,"server",port`, `PRINT #4` and `INPUT #4` for a TCP connection.
- **Connections**: TCP connections are made in the background, so the Spectrum (and F1) keep working meanwhile; if it does not connect within about 10 s, the program gets an error. The addresses 127.x.x.x, 0.x.x.x and the Pi's own IP fail at once ("Socket error"): Circle has no loopback interface, and before 0.48 a `%connect` to 127.0.0.1 froze the Pi. For a server on your PC, use the PC's IP on the local network.
- On the 128K models it starts in 48 BASIC. This is not a bug: the firmware does it (its BASIC extensions module forces "USR 0 mode"), because the `%` commands only work with the 48K ROM. The real card does the same. With the Spectranet connected, every reset (also when changing machine with F7) goes through its boot and ends up in 48 BASIC; to use the 128K menu, disconnect it with Alt+T.

**The firmware**: the official, open-source one (MIT licence), built from https://github.com/spectrumero/spectranet and installed with its installer inside the emulator itself. `roms/spectranet.rom` is that 128 KB flash, already configured with DHCP, the MAC 02:5A:58:4D:45:54 and the host name `zxmetal`. The first time it is copied to `spectranet.flash` in the root of the SD card, which is the one used from then on and saved automatically when it changes (settings, new modules…); to start again, delete that file. `utils/spectranet/` contains the official installer (`installer.tap`).

**It cannot be used together** with the divMMC (both page in at 0x0008), the Interface 1 or the Multiface: connecting the Spectranet disconnects them.

**How it is emulated** (based on Fuse):
- Memory: 128 KB of AM29F010 flash (with programming and erasing), 128 KB of RAM, and the 4 KB windows at 0x0000–0x3FFF (fixed page 0, pages A and B through ports 0x003B and 0x013B, fixed RAM page 3).
- Paged in when executing 0x0008 or 0x3FF8–0x3FFF, with the NMI and through port 0x033B; paged out when executing 0x007C. The programmable trap (port 0x023B) raises an NMI, and the card pages in when it is accepted.
- The W5100 network chip is emulated at socket level: each TCP or UDP socket of the chip is a Circle socket.

**Tested** on the PC (with real sockets): firmware installation, boot with DHCP, NMI menu, settings saved in the flash, `%mount` and `%cat` against a TNFS server, `%tapein` + `LOAD ""` loading a program from the server, and a TCP connection in both directions with `%connect`, `INPUT #4` and `PRINT #4`. In QEMU, the boot and the F5 menu (QEMU has no network).

## Beeper sound

The beeper (and the loading sound) goes through a speaker model taken from **Fuse** (by Fredrick Meunier: `sound/ula_filter.c`, `speaker_filter.c` and `tv_filter.c`), followed by a filter against the whine of 1-bit routines. The AY does not go through them.

1. **The ULA**: an asymmetric low-pass (rising with 36.5 µs and falling with 68.9 µs), tuned in Fuse by listening to recordings of real machines.
2. **The speaker**, according to the `speaker=` option ("Speaker" in the settings, page 2, applied at once):
   - `auto` (default, as in Fuse): the 16K and 48K sound through their **internal speaker**, and the 128K, +2 and +3 through the **television**. It changes by itself when the machine changes;
   - `beeper`: the 48K's internal speaker, which has no bass (750 Hz high-pass); it sounds thinner, like the real one;
   - `tv`: a television (100 Hz – 10 kHz);
   - `off`: no model (only the DC component is removed): the sound of version 0.55.
3. **Remove the 1-bit whine** (`beeperfilter=1`, "Remove 1-bit whine"): a 4th-order low-pass at 6 kHz. Many speech or multi-channel routines use a 10–20 kHz carrier that is not heard on the real machine; Fuse's model only lowers it by about 13 dB, so this filter stays: with the *We Are Vocoders* demo, the carrier (about 16 kHz) drops by about 50 dB and what is heard below 4 kHz stays the same.

Each model has its own volume compensation (the 48K one removes the bass and would otherwise sound quieter).

## Several cores (Pi 2, 3, 4 and Zero 2 W)

On the 4-core models, **scaling the image to the screen** (the magnification, the scanlines, the huge border) is done on **core 1**, while core 0 keeps everything else: emulation, sound, USB, network and SD card. It is what moves the most memory in each frame (about 4 MB at 1080p), so core 0 gets more headroom for the network, downloads, esxDOS or the Spectranet.

- Core 0 hands over each finished frame and core 1 scales it into the hidden page of the screen; core 0 flips the page as soon as core 1 has finished. If core 1 is still busy with the previous one, that frame is not shown (as when the emulation runs late). The image may arrive up to one frame (20 ms) later.
- Border or scanline changes pause core 1 for a moment.
- **Alt+D** shows how long each frame takes on core 0 and the scaling on core 1 (out of the 20 ms a frame lasts). On the single-core Pi 1 and Zero, it shows core 0's time including the image.
- `multicore=0` (or "Video on core 1" in the settings, page 2) goes back to doing everything on core 0. Applied after a restart.
- Internally: `src/videocore.h/.cpp` and Circle built with `ARM_ALLOW_MULTI_CORE` (Pi 2, 3 and 4 only; the Pi 5 uses a single core for now).
- **Tested**: the hand-over between the two cores, on the PC with threads (3,000 frames with random timings and 31 mode changes, without errors), and in QEMU with the 64-bit Pi 3 kernel (QEMU cannot boot the 32-bit Pi 2 and 3 kernels). It still has to be tested on a real Pi.

## Languages

The emulator is in **Spanish** (the main language, built into the program) and **English**. Choose it with `lang=en` in `cmdline.txt` or on page 1 of the settings (F1, C), where the change is seen at once and saved with G. The web page changes too.

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

From the help screen (F1), the **C** key opens the settings: seven pages with the `cmdline.txt` options that can be changed without editing the file. Options that are not used with the current choices are shown in grey and the cursor skips them: they cannot be changed until they make sense again (for example, after choosing another model): for example, the Issue 2/3 keyboard outside the 16K and 48K, the +3e language and hard disk on the other models, the SAVE format without SAVE to the SD card, or the netmask, gateway and DNS with automatic IP.

| Page | Options |
| --- | --- |
| 1 Machine | language, model, Issue 2/3 keyboard, speed, ULAplus, +3e language*, boot hard disk* |
| 2 Video and sound | border, scanlines, AY sound, hear loading, speaker, remove 1-bit whine, sound output*, sound on GPIO12*, video on core 1* |
| 3 Tapes and saving | loading mode, SAVE to the SD card, SAVE format, F8 format |
| 4 Keyboard and gamepads | joystick, PC cursor keys, Recreated ZX, GPIO keyboard* |
| 5 Peripherals | Spectranet, divMMC with esxDOS, LEC memory, Interface 1, Multiface, RS232*, ZX Net* |
| 6 Network, time and tape | network*, time zone*, summer time*, fixed IP*, mask*, gateway*, DNS*, check HTTPS*, real cassette*, invert cassette*, cassette GPIO* |
| 7 ROMs and folders | ROM of each machine*, of the Interface 1*, of esxDOS* and the Spectranet flash* (those in the `roms` folder with the right size), esxDOS folder* |
| 8 Web server | web page* (yes, read-only or no) and its password* |

- Up and down to choose; **left and right** (or ENTER) to change the value; **PgUp and PgDn** (or Tab) to change page.
- **IP addresses are typed in**: ENTER on "Fixed IP", "Network mask", "Gateway" or "DNS server" and type the digits and dots (also on the numeric keypad); Backspace deletes, DEL empties it (automatic, with DHCP), ENTER accepts it if it is a valid IP and ESC leaves it as it was.
- The ROM and hard disk lists only show files without spaces in their names (`cmdline.txt` separates options with spaces).
- Changes apply **at once**, except those marked with `*`, which need the Pi to be restarted.
- **G** saves the values in `cmdline.txt`. It keeps the file's other options (`hdd=`, `ip=`, `logdev=`…) and their order, changes the ones already there and adds those that are not at their default value. It leaves a copy in `cmdline.bak` first.
- **R** restarts the Raspberry Pi. It asks for confirmation ("No" is the highlighted option) and, if there are unsaved changes, offers to save them first. Before restarting, everything pending is saved: +3 disks, microdrive cartridges, the Spectranet flash, what esxDOS wrote and the RS232 output.
- The settings show what is in `cmdline.txt` (read again when opened if there are no pending changes, in case it was changed from the web page), plus whatever is changed with the keys: F4, F6, F7, F9, F10, Alt+B, Alt+S and the peripherals (Alt+T/X/L). So G also saves those changes. What changes by itself, such as the model when loading a 128K snapshot, does not go into the settings, so it is not saved by accident. If something is unsaved, the screen says so.

## Drives (F1 and then U, or Alt+U)

A screen with what is inserted in each drive: **tape**, the +3 **disk A:** and **disk B:**, **hard disk** (+3e or divMMC), **Interface 2 cartridge** and **microdrives 1 to 8**.

- **ENTER** opens the file browser with only the files for that drive (tapes, `.dsk`, hard disk images, `.rom`, `.mdr`/`.mdv`), and what you choose goes into that drive. So a disk can be inserted into drive **B:** or a cartridge into a given microdrive. After choosing, you go back to the drives screen; ESC in the browser also goes back.
- **DEL** (or Backspace) empties the drive, saving whatever is pending first (disks, cartridges, what esxDOS wrote).
- What is mounted here is **for this session only**. The hard disk the Pi boots with is chosen in the settings ("Boot hard disk", page 1), from the images in the `hdd` folder (those without spaces in their name: `cmdline.txt` separates options with spaces).

## Peripherals that page over the ROM

The Spectranet (Alt+T), the divMMC with esxDOS (Alt+X), the LEC memory (Alt+L) and the Interface 1 are connected with their key or on page 5 of the settings (F1, C). They cannot be used at the same time: connecting one disconnects the others. Each change resets the Spectrum. The divMMC is not used on the +3e (there the card goes through the ZXMMC), and on the 128K models the LEC memory is used from 48 BASIC.

## Hiding files: excluir.txt

`excluir.txt`, in the root of the SD card, says which files and folders are **not shown** in the file browser (F1) or the web page. It comes ready to hide what the Raspberry Pi needs to boot: `bootcode.bin`, `start*.elf`, `fixup*.dat`, the `kernel*.img` files (which would otherwise appear in the browser as hard disk images), `armstub*.bin`, the `*.dtb` files, and the `firmware` and `certs` folders.

Format, one entry per line (anything after `#` is ignored):
- If it ends in **`/` or `\`**, it is a **folder**; otherwise, a file.
- If it **starts** with `/` or `\`, it is a path from the root of the SD card (it can have several folders: `/games/old/`); otherwise, it applies to that name in any folder.
- The wildcards `*` and `?` are allowed, and it is not case-sensitive.

The file is read again every time the browser opens, so changes (including those made from the web page) apply without restarting.

## Misspelt options in cmdline.txt

A misspelt value in `cmdline.txt` (for example `sounddev=sndpwn`) gives no error: the emulator uses the default value. At boot, ZXMetalCircle checks the file and shows a warning on screen for about 10 seconds ("sounddev=sndpwn not valid"; with "(+N)" if there are more problems). It also warns about options that do not exist and about a `cmdline.txt` with more than one line (the Raspberry Pi only reads the first). All problems are written to the log (`logdev=ttyS1`).

## Keys

The function keys are in a single table (`KeyBindings` in `src/kernel.cpp`). The F1 help is generated from it, so changing an assignment only needs that table.

| Key | Function |
| --- | --- |
| F1 | Help and status; from there, ENTER opens the file browser, C the settings, U the drives and A the credits |
| Alt+U | Drives: see, mount and empty each drive |
| Alt+K | Keyboard help |
| F2 | Tape: play / stop |
| F3 | Tape: block browser (list of what is on the tape; ENTER leaves it at the chosen block, Home = rewind) |
| F4 | Change joystick |
| F5 | NMI: Multiface, esxDOS browser or Spectranet menu |
| Alt+F5 | File browser (like F1 and ENTER). With `f5=files` they swap: F5 opens the files and Alt+F5 does the NMI |
| F6 | Loading speed: instant / fast / real |
| F7 | Change model (16K → 48K → 128K → +2 → +2A → +3 → +3e), with reset |
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
- **Cursor keys**: they are always the Spectrum's cursor keys (CAPS SHIFT + 5/6/7/8). With a joystick selected, they also move the joystick, and right Ctrl (or keypad 0) is fire. This can be changed with `cursors=`.

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

## Spectrum +3 and .DSK disks

**Loading a disk**: choose a `.DSK` in the browser (F1) or press "load" in the web page. The disk goes into drive **A:**. If the model is not the +3, the emulator switches to the +3 with the disk already inside; in the start-up menu, **Loader** boots it. You can also start with `model=plus3`.

**Keys**:
- **Alt+F**: new disk, already formatted for +3DOS, in the first free drive (A:, then B:). It asks for the format: **180K** (3", single sided, 40 tracks, the +3's) or **720K** (3.5", double sided, 80 tracks).
- **720K disks**: each drive adapts to the disk inserted. With a DSK that has two sides or more than 44 tracks, that drive becomes an 80-track double-sided 3.5" drive, like the ones fitted to the +3 for those disks; with the others it stays as the +3's 3" drive. The drives screen and the message on inserting the disk show "(3.5\" drive)".
- **Alt+E**: ejects both disks (saving the changes first).

**Saving**: what the Spectrum writes to a disk is saved automatically to its file once the motor has been stopped for 2 seconds. New disks are saved as `saves/disk_NNN.dsk`. They are always saved in extended format. While the disk spins, a red **D** appears at the top right.

**How it is emulated**:
- The controller is **MAME's uPD765A** (`third_party/mame/upd765.cpp` and `fdc_pll.cpp`, BSD-3 licence, by Olivier Galibert), trimmed down to the +3 model and otherwise unchanged. It reads the disk surface bit by bit with its PLL, like the real chip, so protections relying on format details work.
- A compatibility layer (`third_party/mame/compat`) gives it what it expects from MAME: time (it advances with the Z80 t-states), timers and the drive, which delivers the track's magnetic transitions.
- `dskimage.cpp` reads `.DSK` files following MAME's `dsk_dsk.cpp` and builds the MFM tracks like its `build_pc_track_mfm()`. It includes longer than normal tracks (6 KB and 8 KB sectors), weak sectors (which change on every read) and CRC errors. When the image stores several copies of a sector (the EDSK extension for weak sectors), a single copy goes on the track, weak from where the copies start to differ. An 8K sector does not fit on a track: only what is really on the disk is written (up to where the copies differ, or at most 6352 bytes) and the rest is read by wrapping round the track, as on the original. This is how **Coin-Op Hits** loads: it stores 6305 bytes in 8K sectors written with narrower bit cells (see https://simonowen.com/samdisk/sys_plus3/). CRC errors in a sector header (ST1 without ST2) are recorded in the header, as in Fuse; in weak sectors with several copies only the stretch where the copies differ changes. The weak sectors of the +3 Speedlock are recognised as in SAMdisk ("SPEEDLOCK" in the first sector): only the 32 bytes from 336 change (Dragon Ninja). When the motor is switched off, the disk keeps spinning and the drive ready for 300 ms (1.5 revolutions, as in Fuse), for loaders that switch the motor off and read straight away (The Deep). What is written is decoded again to be saved.
- The wiring is that of MAME's `specpls3` driver: uPD765A at 4 MHz, ports 0x2FFD (status) and 0x3FFD (data), motor on bit 3 of 0x1FFD, +3 4.0 ROMs.

**Tested** with protected commercial disks (RoboCop, RoboCop 2, Batman The Movie, Turrican) and with SAVE, CAT and LOAD in +3 BASIC. Reading the disk costs CPU (about 230 µs per frame on a modern PC while the motor spins, against 380 µs for the rest of the +3): plenty on the Pi 3 and Pi 4, tight on the Zero W.

## Spectrum +3e and hard disk

The **+3e** is the +3 with Garry Lancaster's ROMs, which add the IDEDOS system: hard disks and cards with partitions, drives C:, D:…, CP/M on a partition, mountable disk images, etc. ZXMetalCircle emulates the **ZXMMC** interface, like Kyp's zxp3 core for MiST/SiDi, so the images used there work as they are, without conversion.

**Using an image**:
1. Copy the image to the Pi's SD card, for example into a `hdd` folder.
2. Choose it in the browser (F1), or with "load" in the web page, or put it in `cmdline.txt` with `hdd=hdd/image.vhd`.
3. The emulator switches to the +3e and resets it, because the +3e reads the partitions at boot. The start-up screen should show "1 MMC".

**Formats**:
- `.img`/`.raw`: raw.
- `.vhd`: raw, with or without the fixed VHD footer. On MiST, a "VHD" is usually a renamed `.img`.
- `.hdf`: with the RS-IDE header; 256-bytes-per-sector HDFs, typical of the 8-bit IDE interface, are not supported.

The file can be up to 4 GB, the FAT32 limit.

**Writes**: what the +3e writes goes straight to the image and is committed to the SD card after one second without activity. While the disk is in use, a blue **H** appears at the top right.

**How it is emulated**:
- The ZXMMC uses port **0x1F** (bit 0 = card select, active low) and **0x3F** (SPI data). As in zxp3's `spi.v`, each access to 0x3F returns the byte of the previous transfer.
- `src/sdcard.cpp` emulates an SD card in SPI mode:
  - images **up to 2 GB**: **SDSC** card (commands carry the address in bytes);
  - larger images: **SDHC** (the address is the sector number), like a real card of that size;
  - blocks carry their real **CRC16**;
  - with the card not selected, the data line reads 0, as on the ZX-Uno;
  - the ZXMMC and the divMMC each have their own data register, 0 at power-on.

  All of this is needed by programs that talk to the card directly, such as Otivax's CP/M 3, which only knows SDSC cards and detects the interface through those details.
- `src/harddisk.cpp` reads and writes sectors of the file using FatFs's *fast seek* (enabled when building Circle), so it does not walk the file's cluster chain on every access.

**ROMs**: it includes the +3e 1.43 ROMs for the ZXMMC, in Spanish (default) and English (`p3elang=en`). Garry Lancaster distributes them for free, with Amstrad's permission to modify and distribute the +3 ROMs. If you want to use the same ROM as on the FPGA, copy it as `roms/plus3e.rom` or use `romplus3e=zxp3.rom`.

**Tested**: with an empty image, the +3e runs `FORMAT TO 0,100`, `NEW DATA "test",4`, `MOVE "c:" IN "test"`, `SAVE` and `CAT` without problems. The resulting image is read correctly by the PC `3e` utility (partition table and file). In QEMU, mounting the image from the browser, `CAT TAB` and `LOAD` work.

## divMMC with esxDOS

The **divMMC** is the most widely used SD card interface for the Spectrum today, and **esxDOS** its system: loading tapes, snapshots and TR-DOS disks from the card, commands `.ls`, `.cd`, `.tapein`…, and a file browser on the NMI. ZXMetalCircle includes **esxDOS 0.8.9** (© Papaya Dezign, https://esxdos.org, non-commercial use).

**Turning it on**: add `divmmc=esxdos` to `cmdline.txt`, or connect it at any time with **Alt+X** or in the settings (F1, C, page 5).

**The card is an image**: `hdd/esxdos.img`, a FAT16 card the emulator creates the first time (16 MB, and bigger if needed, up to 2 GB). esxDOS reads it by sectors, like a real card.

**To put files on it, leave them in the `esxdos_i` folder** ("import"), in the root of the SD card, from the PC or by uploading them through the web page. When the divMMC is connected and on every reset (F12, F7) they are copied to the card, keeping the subfolders, and **deleted from the folder**:
- Each copy is read again and compared with the original before deleting it. Whatever cannot be imported (it does not fit, the name is not valid…) stays in the folder and a message says how many and why, for example "esxDOS: 40 imported, 2 not (card root is full: use subfolders)".
- If the card already has a file with the same name, the new one replaces it ("esxDOS: 3 imported (1 replaced)").
- If everything does not fit, a bigger card is created and what was already there is moved to it.
- While importing, the progress is shown ("esxDOS: importing 12 of 40 (9 of 20 MB)..."); the emulation is stopped.
- With the folder empty nothing is done, and without the divMMC connected, nothing either.
- The folder comes with the esxDOS system (SYS, BIN, TMP) and an empty JUEGOS folder: they go into the card the first time.
- What the Spectrum saves (`.cp`, snapshots, `.tapeout`…) stays on the card. The image is not included in the SD cards of new versions, so updating by copying the new SD card over the old one does not lose it.
- **In 0.94 and earlier** the folder was `esxdos` and it was synchronised with the image both ways. On startup, if `esxdos` exists and `esxdos_i` does not, it is renamed, and its contents are imported like anything else (what was already on the card is replaced by the same and is no longer duplicated on the SD card).
- `esxdir=other` uses another folder to import from; `esxdir=0` turns importing off (the `hdd=` image, or `hdd/esxdos.img`, is used as it is).
- When connected, the esxDOS screen appears ("sda: ZX Metal", "hd0: …FAT16") and then BASIC.

**What cannot be used together with it** (the same happens on the real machine):
- The **Interface 1**: it uses the same ports (0xE7, 0xEB) and also pages in at 0x0008.
- The **LEC memory** (Alt+L).
- The **Multiface**: the NMI is handled by esxDOS.
- The **Spectranet**.

Connecting one disconnects the others.

**On the +3e**: the divMMC is not used (the +3e ROMs use the ZXMMC), but its **card ports (0xE7 and 0xEB)** are always present and give access to the same image, as on the ZX-Uno. So programs that talk to the card directly (for example, the CP/M 3.0 of the ZX-Uno Workbench) can find it.

**How it is emulated**:
- Ports 0xE3 (control: CONMEM, MAPRAM and bank), 0xE7 (card select) and 0xEB (SPI data).
- 8 KB of ROM (`roms/esxmmc.bin`) and 128 KB of RAM in 8 KB banks.
- Automatic paging when executing 0x0000, 0x0008, 0x0038, 0x0066, 0x04C6 and 0x0562 (after reading the instruction) or 0x3D00–0x3DFF (at once), and paged out when executing 0x1FF8–0x1FFF, as described in the divIDE/divMMC documentation and emulated by ZEsarUX.
- The F5 NMI is raised in mid-frame: if it arrived right with the frame interrupt, while esxDOS was handling it, it would be ignored.

**Tested** on the PC and in QEMU:
- the folder: creating the image from `esxdos/`, `.ls` and `.cd`, and a file made with `.cp` that goes back to the folder by itself;
- esxDOS 0.8.9 boot on the 48K and 128K;
- `.tapein` followed by `LOAD ""`, which loads a program from the card;
- the NMI browser with F5;
- `.128`.

### The "Mar Océana" image (desUBIkado)

It works fully on the +3e, including the menu's **CP/M 3.0**, which mounts its partitions as C: and D:. Its start-up menu is designed for the ZX-Uno. `utils/mar_oceana/` contains a version for ZXMetalCircle (`menu_zxmetal.tap`) without the options that do nothing here (the ZX-Uno's FilePlus, uGophy and NetMan over WiFi, and ZXUC Core Setup); the `README.txt` in that folder explains how to install it from the emulator itself.

`utils/mar_oceana/cpm_sdhc/` also contains a CP/M driver (`DIVMMC.BIN`) modified for SDSC and SDHC cards. It is experimental: it works in ZXMetalCircle, but in the zxp3 core it gave error 23 when loading CP/M. With images of up to 2 GB, use the original one, which is what comes in the image.

## ROMs

Only the **16K/48K and 128K** ROMs are built into the kernel, so the emulator always boots. The others are loaded at power-on from the **`roms`** folder of the SD card, where they come included:

| File | Size | Machine or peripheral |
| --- | --- | --- |
| `plus2.rom` | 32 KB | Grey +2 |
| `plus2a.rom` | 64 KB | +2A |
| `plus3.rom` | 64 KB | +3 (and .DSK disks) |
| `plus3e-es.rom` / `plus3e-en.rom` | 64 KB | +3e (and hard disk); chosen with `p3elang=` |
| `if1.rom` | 8 KB | Interface 1 (microdrives, RS232, ZX Net, LEC's CP/M) |
| `mf1.rom`, `mf128.rom`, `mfplus3.rom` | 8 KB | Multiface One, 128 and 3 (F5); **not included**, put your own; without them, F5 does a plain NMI |
| `esxmmc.bin` | 8 KB | esxDOS 0.8.9 for the divMMC (`divmmc=esxdos`) |
| `spectranet.rom` | 128 KB | Spectranet flash with the firmware installed (`spectranet=1`) |

**If one is missing** (or has the wrong size), whatever needs it becomes unavailable and the rest works:
- F7 skips that machine;
- `model=` boots on the 48K;
- the browser and the web page warn when loading a `.DSK`, a hard disk image or a microdrive;
- snapshots for that machine load on the 128K, with a warning.

At boot, "Missing ROMs: …" appears. Everything is explained in `roms/README.txt`, in case someone deletes a ROM by mistake.

**Your own ROMs**: `48.rom` and `128.rom` replace the ones built into the kernel, and `plus3e.rom` takes priority over the per-language +3e ones. With `rom48=`, `rom128=`, `romplus2=`, `romplus2a=`, `romplus3=`, `romplus3e=` and `romif1=` you can name another file in the folder (for example `romplus3e=zxp3.rom`). The tape traps only work if the ROM has the standard routines in place.

## Network: WiFi, web server and time

**Models**: WiFi on the Raspberry Pi 3, 3B+, 3A+, 4, 400, Zero W and Zero 2 W, and **in testing on the Pi 5 and 500** (Circle warns that on the Pi 5 WiFi may be unstable with an HDMI display at the same time; its firmware, `brcmfmac43455-sdio.raspberrypi,5-model-b.*`, is also in `firmware`). The Pis with a network socket (1B, 2B, 3B, 4, 5) can also use cable, as can a USB network adapter supported by Circle. With `net=auto` WiFi is used if there is a `wpa_supplicant.conf` on the SD card; to force cable, `net=eth`.

**Setting up the WiFi**:
1. On the SD card, rename `wpa_supplicant.conf.ejemplo` to `wpa_supplicant.conf`.
2. Write your network's name and password in it. Only 2.4 GHz networks with WPA/WPA2 work.
3. The WiFi chip firmware is already in the SD card's `firmware` folder.

At boot, the connection is made in the background. When it is ready, a message with the address appears, for example `WiFi http://192.168.1.50`, and the help (F1) shows it on the "Network:" line.

**Long operations**: the network and the emulation share core 0, so while a large file (from 32 KB) is read or written through the web page, or ZXDB is searched or downloaded from (HTTPS), the emulation stops for a moment. Meanwhile a notice is shown at the bottom, for example "Network: sending game.tap (512 KB)" or "Network: downloading from ZXDB...".

**Web server**: open that address in the PC's or phone's browser. From there you can:
- browse the folders of the SD card;
- **upload** several files at once to the current folder (up to 8 MB in total);
- **download** any file (up to 4 MB), for example your saved games, snapshots or microdrive cartridges;
- **load** a tape, snapshot, cartridge or microdrive straight into the Spectrum;
- **delete** files (it asks for confirmation) and **create folders**.

It also announces itself on the local network (mDNS) as the "ZXMetalCircle" web service.

**Protecting the page** (F1, C, page 8, or in `cmdline.txt`):
- `web=0` turns it off completely (the server is not even started); in F1 the network line says "(web off)".
- `web=ro` makes it **read-only**: you can browse the SD card, download files, search ZXDB and load a game, but not upload, delete, create folders or download from ZXDB to the SD card (those options do not even appear).
- `webpass=` sets a **password** (in the settings it is typed with lowercase letters and numbers, and shown as `********`). The page asks for it once and then creates a random session key that goes into the links and forms (Circle's web server gives no access to cookies or to the browser's password). The keys are lost when the Pi is switched off; after 5 failed attempts you have to wait 30 s. It is not encrypted (HTTP): it protects you from curious people on your network, not from someone eavesdropping.

**Time**: taken from `pool.ntp.org`. The files you save (SAVE, snapshots, microdrives) carry the real date and time. The zone is set with `tz=` and European summer time is applied automatically (`dst=1`).

**Zero W**: it has a single core, so the WiFi takes time from the emulation. When connecting (WPA encryption is costly) and while uploading or downloading files there may be short glitches in picture or sound; the Spectrum does not fall behind, because the emulator skips frames if needed. At rest it is not noticeable. Booting with WiFi on a Zero W shows a warning.

## Searching and downloading programs (HTTPS)

The emulator's web page has a search box: you type a title and it searches **ZXDB**, Spectrum Computing's database, through the ZXInfo API.
- Only Spectrum programs are shown: no ZX81, SAM Coupé, etc.
- For each one the best available file is chosen, in this order: TZX, TAP, Z80, SNA, SZX, DSK, CSW…
- With "**download and load**", the emulator fetches it from spectrumcomputing.co.uk, unzips it if it comes as a ZIP, saves it in the **`descargas`** (downloads) folder of the SD card and loads it into the Spectrum. If needed, it changes model, for example to the +3 for a `.DSK`.

**How it works**:
- `src/download/httpsclient.cpp` is an HTTP/1.1 client with redirects and chunked responses.
- **Mbed TLS 3.6** (`third_party/mbedtls`, Apache-2.0 licence) encrypts the connection with TLS 1.2. Its configuration is in `zxmc_config.h`: ECDHE/RSA, AES-GCM, ChaCha20.
- `src/download/zxinfo.cpp` parses the JSON with **jsmn** (MIT) and unzips the ZIPs with the emulator's own decompressor.
- Connections are made in a Circle task with a 128 KB stack, so the Spectrum keeps running meanwhile.

**Certificates**: the server's certificate is checked against Mozilla's root authority list, in `certs/cacert.pem` on the SD card. The expiry date is not checked, because Mbed TLS was built without a clock. With `tlsverify=0` nothing is checked; use it only if it fails because of a certificate and you trust the network.

**Tested**: on the PC, with the same files and normal sockets, search and download work against the real servers (Manic Miner, Head over Heels…). Also used on the Pi.

## TNFS servers (Spectranet) in the file browser

With the network running, the root of the browser (F1) has a first entry **`[Internet (TNFS)]`**:
- Inside are the Spectrum TNFS servers from `SD:/tnfs.txt`: by default tnfs.markround.com (more than 16,000 programs), vexed4.alioth.net, tnfs.millhill.org and tnfs.bytedelight.com.
- They are browsed like folders of the SD card.
- When you choose a file, it is downloaded to **`SD:/descargas/<server>/`** and loaded like any other: tape, snapshot, `.DSK`…

**How it works**:
- TNFS is the Spectranet's network file system: UDP, port 16384.
- `src/tnfs.cpp` implements the reading part: MOUNT, OPENDIR/READDIR, STAT, OPEN/READ/CLOSE and UMOUNT, with retries and respecting the minimum time the server asks for.
- So that long lists do not take long, it uses **OPENDIRX/READDIRX**, which return many entries per datagram and whether they are folders. If the server does not know them, it uses the classic method and asks with STAT only about what has no file extension.
- While talking to the server, the menu shows "Connecting…", "Reading the list…" or "Downloading…".

The emulated **Spectranet** (see its section) lets the Spectrum use these servers directly too.

**Tested**: on the PC, the client against the reference server `tnfsd`:
- listing of the root and of subfolders, with READDIRX and with the classic method;
- STAT of files and folders;
- byte-for-byte identical download of a 128 KB `.DSK`;
- names with spaces, and errors.

Also used on the Pi.

## Speed test

`utils/speedtest.tap` on the SD card is a program to check that the emulator runs at the right speed (the source is in `tools/speedtest`):

- **It measures the t-states of a frame**: 69888 on the 48K, 70908 on the 128K/+2/+2A, within ±2.
- **It shows the elapsed time according to the Spectrum itself** (frames × exact frame length). Every second there is a click and the border changes. Compare it with a stopwatch: after 10 minutes there should not be more than 1 second of difference.
- SPACE measures again.
- It loads on the 48K, 128K, +2 and +2A with `LOAD ""` or the Tape Loader. It does not work on the 16K, because the code goes at 32768.

The help (F1) also shows the real speed measured by the emulator, in frames per second: it should read 50.08 on the 48K and 50.02 on the 128K.

## Image: border and scanlines

**Border sizes** (the same as JSpeccy):

| Border | Image |
| --- | --- |
| None | 256 × 192 |
| Standard | 320 × 240 |
| Full | 352 × 296 (default) |
| Huge | 384 × 304 |

- The emulator always generates the image with the huge border and shows the chosen part. The less border, the bigger the Spectrum screen looks.
- With a small border, the menus enlarge the image to the full border while they are open.

**Scanlines**:
- The Raspberry Pi GPU scales the image keeping its aspect ratio. So the framebuffer always has the same aspect ratio as the screen, and each Spectrum pixel takes r × r pixels (r = 2 or 3).
- In each group of r lines, the last one is drawn at half brightness. The GPU then applies an integer scale, so all the scanlines look the same.
- The emulator chooses r and the scale so the image is as big as possible. For example, with the full border:

  | Screen | Visible image |
  | --- | --- |
  | 1920×1080 | 1056×888 |
  | 1920×1200 or 1600×1200 | 1408×1184 |
  | 4K | 2112×1776 |

  The calculation is in `src/videolayout.h` and is checked by `test/host/scanlines_layout_test.cpp`.
- **Size with scanlines**: the biggest integer magnification that fits is used (for example, x5 with the standard border at 1200 lines, which fills the screen). On the Pi 2, 3 and 4, if that magnification leaves the image below 90 % of its size without scanlines, a **fractional magnification at native resolution** is used: each Spectrum line takes 3 or 4 screen lines (or 4 or 5, depending on the case) and the last one is darkened. At those resolutions the difference between lines is hardly noticeable, and the image takes the same space as without scanlines.
  - Examples: the huge border at 1200 lines goes from x3 (76 %) to x3.95 (99 %); at 1080 lines, every border fills the screen.
  - On the Pi 1 and the Zero, with less power, the integer magnifications with a small framebuffer are kept.
  - On the 4-core Pis this scaling runs on core 1 (see "Several cores").
- **Sharpness**: `config.txt` has `scaling_kernel=8`, which makes the GPU enlarge with the nearest-neighbour method. Without that line, the GPU smooths the image when enlarging it and, without scanlines, the letters look blurred.
- If the screen is too small, the emulator says so and does not turn them on. Writing more pixels per frame costs some CPU; on a Pi Zero it may be noticeable.

## Sound

- **Stereo AY**: the three channels are spread between the two speakers, and the centre channel is heard on both sides at half volume. The possible layouts are:
  - **ABC** (default, the most common);
  - **ACB**;
  - **BAC**;
  - **mono**.

  Chosen with `aystereo=` or changed on the fly with F9. With the GPIO12 output the sound is always mono.
- **Loading sound**: as on the real Spectrum, the tape signal reaches the speaker and the pilot tone and the data can be heard. It is only heard when loading at real speed: in fast mode the emulator runs flat out and goes silent, and in instant mode there is no signal. Turned off with `tapesound=0`.
- **Beeper**: see "Beeper sound" (speaker model and 1-bit whine filter).

## Issue 2/3 and speed

- **Issue 2/3**:
  - on 16K/48K Issue 2 boards, the EAR bit read from port 0xFE is set both by the EAR bit (4) and by the MIC bit (3) of the last OUT. On Issue 3 and later, only by EAR;
  - some old programs, written for Issue 2 boards, only read the keyboard properly in that mode;
  - the default is Issue 3. It is changed with `issue=2` or in the settings (F1, C, page 1), and only affects the 16K and 48K models;
  - .Z80 snapshots save and restore this setting.
- **Speed**:
  - F10 cycles between x1, x2, x4 and x10;
  - above x1, several frames are emulated for each screen frame and the sound is muted;
  - x10 equals a 35 MHz Z80. Whether it is reached depends on the Raspberry Pi: the Pi 3/4 should have power to spare, a Zero may not get there.

## ULAplus

ULAplus is a modern extension of the ULA that adds a palette of 64 colours, chosen from 256. Many recent games and demos use it, and old programs are not affected.

- **Ports**: 0xBF3B (register) and 0xFF3B (data), decoded as in JSpeccy.
- **Palette**: registers 0–63 are the palette entries, in GGGRRRBB format. Register 64 (mode group) turns the palette on or off.
- **With the mode on**, the FLASH and BRIGHT bits of each attribute choose one of the 4 sub-palettes of 16 colours (8 ink and 8 paper), so there is no more flashing. The border uses the paper colours of the first sub-palette.
- Palette changes in mid-frame are respected: the image is generated directly in colour (RGB565), and what was drawn before the change keeps its colours.
- A reset brings back the normal mode.
- .SZX files with a palette block (PLT) also restore ULAplus.
- It can be turned off with `ulaplus=0`.

## Interface 2 cartridges

The Sinclair Interface 2 had a slot for 16 KB ROM cartridges which, when plugged in, replaced the Spectrum ROM; the game started at power-on. There were about ten games in this format (Jetpac, Pssst, Cookie, Tranz Am, Chess…).

- They are loaded from the browser (F1, ENTER), choosing a `.ROM` file of up to 16 KB. If it is smaller, the rest is filled with 0xFF.
- Inserting it resets the machine and the cartridge starts. **F11** removes it and boots again with the normal ROM.
- It works on the 16K, 48K, 128K and +2. The +2A does not support it, as in JSpeccy.
- The Interface 2 joysticks are the Sinclair 1 and Sinclair 2 modes.
- Loading a snapshot removes the cartridge, unless it is a .SZX that includes it (IF2R block).

## Interface 1 and microdrives

The Sinclair Interface 1 added up to 8 microdrives (small endless tapes of about 90–126 KB), a local network and an RS232 port to the Spectrum. The Interface 1 and the microdrives are emulated with JSpeccy's logic; the RS232 and the network, as described in "Status".

- **Inserting a cartridge**: choose an `.MDR` or `.MDV` file in the browser (F1, ENTER). It goes into the first free drive: 1, then 2, etc.
- **New cartridge**: Alt+N inserts an unformatted one with 180 sectors (about 90 KB, like a real one; the same as JSpeccy uses) into the first free drive. It has to be formatted from BASIC: `FORMAT "m";2;"name"`.
- **Commands**: the Interface 1 ones in 48K BASIC, for example `CAT 1`, `SAVE *"m";1;"prog"`, `LOAD *"m";1;"prog"`, `ERASE "m";1;"prog"`, `MOVE`… On the Spectrum keyboard, FORMAT, CAT, ERASE and MOVE are in extended mode with SYMBOL SHIFT (above the numbers).
- **Saving**: when a cartridge is modified, it is saved as soon as the motor stops, and Alt+M saves whatever is pending and removes all the cartridges. New ones are saved as `saves/mdr_001.…`. The format is chosen automatically:
  - **.MDR** if the whole content is Sinclair sectors. To find out, the emulator converts it, loads it again and checks that nothing is lost.
  - **.MDV** (the raw tape, JSpeccy's "MDVT" format) if not, as happens with LEC's CP/M. JSpeccy's compressed .MDV files can also be loaded, and the ones saved use its same sector size (792, or 587 if they come from an .MDR), so JSpeccy can open them and save them again. A cartridge inserted as .MDV is always saved as .MDV.
- While a microdrive is spinning, "M1".."M8" appears at the top right.
- The Interface 1 ROM 2 is used, as in JSpeccy, and the `.MDR` format of Fuse and Spectaculator: 254 sectors and a final write-protect byte, which is respected.
- With `if1=auto` (default), the Interface 1 connects when the first cartridge is inserted, so it changes nothing if microdrives are not used. The +2A does not support it.
- The RS232 (TCP 2323) and the ZX Net network (UDP 5050) go out through the Pi's network: see the Interface 1 part of "Status".

## LEC memory and CP/M

The LEC expansion (Jiří Lamač, 1987) replaced the 48K's memory chips with bigger ones. With a port (0xFD; in fact, any port with A1 = 0) a 32 KB RAM page can be placed at 0x0000–0x7FFF instead of the ROM and the screen. There are 16 pages: page 15 is the Spectrum's own upper RAM, which is always at 0x8000–0xFFFF. Its big use was **LEC's CP/M 2.2**, which works with microdrives.

- It is connected with `lec=1`, Alt+L or in the settings (F1, C, page 5).
- **On the 128K models** (128, +2, +2A, +3, +3e) it works in 48K mode: after connecting it, choose **48 BASIC** in the menu (or enter with USR 0) and load CP/M as on the 48K. The original LEC was for the 48K, and on a real 128K its port would clash with the paging port (0x7FFD) and the AY ports (0xBFFD/0xFFFD), which also end in FD. The emulator only takes for the LEC the writes with the shape CP/M uses (`OUT (0xFD),A`, where the port's high byte is the value itself), so the AY and the rest keep working. Tested: CP/M boots and lists the disk on the 48K, 128K, +2A and +3. The +2A and +3 have no Interface 1, so there is no microdrive drive B:. While the LEC is paged in there is no ROM, so the tape traps, the Interface 1, the Multiface and the AY do not act, as in JSpeccy and on the real hardware.
- The screen is still the 48K's original RAM, which the ULA always reads.
- With the LEC paged in, snapshots cannot be saved, because the formats do not include that memory.

**Trying CP/M** (the tape is in `cpm/` on the SD card, with the author's permission to distribute it):
1. Set the 48K with LEC (`lec=1` or Alt+L) and press Alt+N to insert a new cartridge, which will be drive B:.
2. F1, ENTER, go into `cpm` and choose `cpm22_lec.tzx`. Type `LOAD ""`.
3. After loading, CP/M counts the memory (528 KB), mounts a RAM disk (A:) with PW, FORMAT, MLOAD and MSAVE, and shows `A>`.
4. Format the microdrive: `format`, `1`, `8`, ENTER. At the end it says "Operation completed, 96K bytes available"; press ENTER to go back.
5. B: can now be used, for example `save 4 b:test.com` and `dir b:`.

**Keys in CP/M**:
- **Delete** (the PC's backspace key or left arrow): while CP/M is running, the PC's backspace key sends cursor left, which is the clean backspace. The Spectrum's DELETE is CP/M 2.2's "rubout", which deletes but echoes what was deleted (the word comes out backwards). In BASIC, backspace is still DELETE.
- **Delete the line**: Shift + left Ctrl + X.
- **Ctrl-C** (restart CP/M, at the start of the line): Shift + left Ctrl + C.
6. The cartridge is saved automatically as `saves/mdr_001.mdv`. In another session, insert that .MDV before starting CP/M and you will have your files in B:.

## Snapshots

A snapshot is a "photo" of the whole Spectrum: Z80 registers, memory, paging, border and sound chip. When loaded, the program carries on exactly where it was. It serves to save the game at any moment or to load games instantly.

- **Loading**: from the browser (F1 and then ENTER), like tapes. Formats:
  - .Z80, versions 1, 2 and 3, compressed or not;
  - 48K and 128K .SNA;
  - .SZX, the Spectaculator and Fuse format, with compressed or uncompressed pages.
- The emulator switches by itself to the snapshot's model (16K, 48K, 128K, +2 or +2A). A 128K .SNA loads on the +2 or +2A if that is the current model, because the format does not tell them apart.
- **Saving**: F8 saves the state to `snaps/<tape>_001.z80`, `_002`, etc., with the current tape's name or `snap` if there is none. The format is chosen with `snapformat=`:
  - compressed **.Z80** v3, the most compatible and the default;
  - **.SNA**: on the 48K, the PC goes on the stack, as the format requires; the +2A is saved as a 128K;
  - **.SZX**: the most complete (it includes the ULAplus palette); pages are stored uncompressed.

  Both .Z80 and .SZX store the exact position within the frame.
- Pentagon 128 snapshots load as a 128K, the closest there is.
- The state of the Multiface and the tape is not saved. Save the snapshot with the Multiface closed.

## Saving with SAVE

When the ROM saves a block (SAVE, SAVE ... CODE, SAVE ... SCREEN$, "Tape" in the 128K/+2A menus…), the emulator traps the SA-BYTES routine and writes the block straight to the SD card, without waiting for the real saving time.

- The files go to the SD card's `saves` folder, created automatically. The name comes from the header: `SAVE "test"` creates `saves/test.tap`, and if it already exists, `test_2.tap`, `test_3.tap`…
- **Game tapes are never modified.** If the current tape is not a recordings tape, a new file is created and becomes the current tape. Further recordings are appended to that file, as on a real tape.
- If you load a .TAP from the `saves` folder in the browser, what you save afterwards is appended to it.
- After saving, the tape is left at the end. To load again, rewind with F3 (Home and ENTER) or choose it again in the browser.
- With `savefmt=tzx` recordings go to .TZX files with standard blocks (0x10) instead of .TAP.
- Custom saving routines (turbo, special loaders) do not go through the ROM: for those there is signal recording (below).

### Recording the tape signal (Alt+W)

It works like putting a blank tape to record: every change of the Spectrum's MIC output is recorded with 16 t-state precision. It works with any saving routine, including turbo ones and those of games that save progress.

1. Press **Alt+W**. A red dot appears at the top left.
2. Make the program save (SAVE, the game's save option…).
3. Press **Alt+W** again to stop.

`saves/rec_001.tzx` is created (with a CSW block 0x18) and becomes the current tape, rewound and ready to load. While recording, the SAVE trap is disabled, so ROM saves are also recorded as a signal.

## Recreated ZX Spectrum

The Recreated ZX Spectrum (Elite Systems, 2013) is a replica of the rubber-key 48K that works as a USB or Bluetooth keyboard. Here only USB is supported (Circle has no Bluetooth HID).

- Put it in **Layer A**, the game layer, with the switch at the back. In that layer, each key sends one character when pressed and another when released (for example, 1 sends `a` when pressed and `b` when released). So all 40 keys are known in any combination, as on the original membrane.
- **Through a USB cable it is recognised when connected**, by its USB identifier (VID:PID `0F39:0404`, TG3 Electronics; information from José Luis Sánchez), without pressing anything. `recreated=0` turns it off (for example, to use it in Layer B, QWERTY).
- **Automatic detection** (with `recreated=auto`, the default) for when it is not recognised by the identifier, for example through an adapter:
  - the emulator recognises it by the way it transmits: three complete press-release pairs (for example, 'a' and then 'b' for key 1) lasting less than 30 ms. Pressing any key 3 times is enough, and "Recreated ZX keyboard detected" appears;
  - meanwhile, those key presses do not reach the Spectrum;
  - a normal keyboard is recognised as such after 8 key presses, and until then each key may arrive with a delay of up to about 50 ms.
- It can be used alone or together with a normal USB keyboard and the real GPIO keyboard. Its keys are combined with those of the GPIO keyboard.
- **Extended mode**: CAPS SHIFT + SYMBOL SHIFT + ENTER, as with the GPIO keyboard and in ZXBaremulator. In that mode, 1–0 act as F1–F10 and SYMBOL SHIFT as Alt.
- In Layer B (QWERTY), the Recreated works as a normal keyboard.

## Multiface

The one matching each model is emulated, following JSpeccy's logic:

| Model | Multiface | File in `roms/` on the SD card |
| --- | --- | --- |
| 16K and 48K | Multiface One | `mf1.rom` |
| 128K and +2 | Multiface 128 | `mf128.rom` |
| +2A | Multiface 3 | `mfplus3.rom` (or `mf3.rom`) |

- **The ROMs** (© Romantic Robot) are **not included**: we have no permission to distribute them. Copy the dump of your own Multiface's ROM to the SD card's `roms` folder as `mf1.rom`, `mf128.rom` or `mfplus3.rom` (8192 bytes each). If one is missing, F5 does a plain NMI on that machine.
- It is entered with F5, with the GPIO NMI button or, in the real keyboard's extended mode, with key 5.
- Port paging (0x9F/0x1F, 0xBF/0x3F), the MF128/MF3 lock (including "invisible" mode), reading 0x7FFD/0x1FFD from the MF and its own 8 KB of RAM all work.

## USB gamepads

They are hot-plugged, up to 4 at a time. They all drive the selected emulated joystick.

| Button | Kempston | Fuller | Sinclair 1 | Sinclair 2 | Cursor | QAOPSpaceM |
| --- | --- | --- | --- | --- | --- | --- |
| A / Cross, Y / Triangle, L1, R1 | Fire | Fire | Fire | Fire | Fire | SPACE |
| B / Circle, X / Square, L2, R2 | Fire 2 | Fire | X | Z | 9 | M |

- The D-pad and the left stick move the joystick.
- **Select / Share / View / Capture** changes the joystick, in the same cycle as ZXBaremulator: none → Kempston → Fuller → Sinclair 1 → Sinclair 2 → Cursor → QAOPSpaceM.
- **Start / Options** opens the help and the tape browser.
- In the menus, the D-pad navigates, fire equals ENTER and fire 2 equals ESC.
- Generic gamepads: the first "hat" or the first two axes are the directions. Buttons 1, 4, 5 and 6 are fire; buttons 2, 3, 7 and 8 are fire 2; button 9 is Select and button 10 is Start.
- Power: several gamepads draw quite a lot at start-up. Use a power supply of at least 2.5 A.

## Kempston mouse

A connected (hot-plugged) USB mouse works as a Kempston mouse. As in ZXBaremulator, it is only active when the selected joystick is **not** Kempston, because they share ports. If no mouse is connected, the ports do not respond, so programs that detect it do not find it.

| Port | Content |
| --- | --- |
| 0xFADF | Buttons: bit 0 right, bit 1 left, bit 2 middle (active at 0); bits 4-7, wheel |
| 0xFBDF | X (grows to the right) |
| 0xFFDF | Y (grows upwards) |

Decoding is partial (A0, A5, A8 and A10), as in Fuse.

## Real keyboard and joysticks through GPIO

The full GPIO pin table (keyboard, sound, cassette, serial port) and which options share pins is in [GPIO.md](GPIO.md) (in Spanish).

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
- **Extended mode**: turned on with the button or with CAPS SHIFT + SYMBOL SHIFT + ENTER, and a blue keyboard icon appears at the bottom right. In that mode, 1–0 act as F1–F10 and SYMBOL SHIFT acts as Alt (SS+K shows the keyboard help, SS+R resets, SS+P pauses). The same combination goes back to normal mode.
- **In the menus**: names that are too long scroll by themselves when selected (the others end in `>`). The cursor keys and PgUp/PgDn repeat when held down (after 400 ms and then every 60 ms), also with CAPS SHIFT + 5/6/7/8 on the GPIO keyboard. You navigate with CAPS SHIFT + 5/6/7/8 (cursors), ENTER, BREAK (Esc) and CAPS SHIFT + 0 (go up a folder). Letters jump in the list.
- The USB keyboard and the GPIO one can be used at the same time.
- **NMI** (only with `gpiokbd=48`): a button between GPIO16 (pin 36) and GND (pin 34) raises an NMI, which activates the Multiface. It is debounced.

## Tape input: real cassette

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

## Structure

```
src/kernel.*       boot, main loop, audio sync, function keys, settings
src/main.cpp       entry point (creates the kernel and starts it)
src/spectrum.*     machine: memory, ULA, ports, contention, tape traps
src/ay8912.*       AY-3-8912 sound chip
src/audiomixer.*   sample generation (beeper + AY) per t-state, speaker model
src/tape.*         TAP/TZX/PZX/CSW tape (pulses, instant loading and blocks)
src/inflate.*      zlib decompressor (CSW Z-RLE blocks and .SZX pages)
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
