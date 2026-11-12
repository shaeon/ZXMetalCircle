# Peripherals

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## Spectranet (network card)

Dylan Smith's **Spectranet** (https://spectrum.alioth.net) gives the Spectrum a real network: it mounts **TNFS** servers from BASIC, opens TCP and UDP connections, and is used by programs such as IRC clients, browsers or demos that say "Spectranet not found" if they cannot find it.

**Turning it on**: add `spectranet=1` to `cmdline.txt`, or connect it at any time with **Alt+T** or in the settings (F1, C, page 6). The Pi needs a network (WiFi or cable).
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

## Peripherals that page over the ROM

The Spectranet (Alt+T), the divMMC with esxDOS (Alt+X), the LEC memory (Alt+L) and the Interface 1 are connected with their key or on page 6 of the settings (F1, C). They cannot be used at the same time: connecting one disconnects the others. Each change resets the Spectrum. The divMMC is not used on the +3e (there the card goes through the ZXMMC), and on the 128K models the LEC memory is used from 48 BASIC.

## ULAplus

ULAplus is a modern extension of the ULA that adds a palette of 64 colours, chosen from 256. Many recent games and demos use it, and old programs are not affected.

- **Ports**: 0xBF3B (register) and 0xFF3B (data). A15, A14 and A7-A0 are checked, not A13-A8, as in Rodolfo Guerra's ToK'ra FPGA core: programs load the palette with OTIR, which keeps lowering the high byte of the port. The esxDOS clock ports (0x7C3B, 0x7D3B) are not ULAplus ports.
- **Contention**: on Sinclair models, its ports have the same contention as the ULA ones, even though they are odd.
- **On the TK90X and TK95**, ULAplus is an extension: it works in Rodolfo Guerra's ToK'ra FPGA core, but a period TK did not have it, and its contention on a real TK is not verified yet, so its ports are treated like any other odd port.
- **Palette**: registers 0–63 are the palette entries, in GGGRRRBB format. Register 64 (mode group) turns the palette on or off.
- **With the mode on**, the FLASH and BRIGHT bits of each attribute choose one of the 4 sub-palettes of 16 colours (8 ink and 8 paper), so there is no more flashing. The border uses the paper colours of the first sub-palette.
- Palette changes in mid-frame are respected: the image is generated directly in colour (RGB565), and what was drawn before the change keeps its colours.
- A reset brings back the normal mode.
- .SZX files with a palette block (PLT) also restore ULAplus.
- It can be turned off with `ulaplus=0`.

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

- It is connected with `lec=1`, Alt+L or in the settings (F1, C, page 6).
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

## Multiface

The one matching each model is emulated, following JSpeccy's logic:

| Model | Multiface | File in `roms/` on the SD card |
| --- | --- | --- |
| 16K and 48K | Multiface One | `mf1.rom` |
| 128K and +2 | Multiface 128 | `mf128.rom` |
| +2A | Multiface 3 | `mfplus3.rom` (or `mf3.rom`) |

- **The ROMs** (© Romantic Robot) go in the SD card's `roms` folder: `mf1.rom`, `mf128.rom` and `mfplus3.rom` (8192 bytes each). If one is missing, F5 does a plain NMI on that machine.
- It is entered with F5, with the GPIO NMI button or, in the real keyboard's extended mode, with key 5.
- Port paging (0x9F/0x1F, 0xBF/0x3F), the MF128/MF3 lock (including "invisible" mode), reading 0x7FFD/0x1FFD from the MF and its own 8 KB of RAM all work.
