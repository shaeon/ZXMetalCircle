# Models and ROMs

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## Microdigital TK90X and TK95

Microdigital's TK90X and TK95 (Brazil) were ZX Spectrum 48K clones. Choose them with F7 or with `model=tk90x` / `model=tk95`.

- **ULA** (`tkula=`): Microdigital's at 50 Hz (Uruguay; default), Microdigital's at 60 Hz (Brazil) or the original Ferranti one (the first exported TK90X). The picture timings and the clock change (about 3.58 MHz with Microdigital's).
- **TK90X ROM** (`tkrom=`): Rodolfo Guerra's v3 (default), or the original v1 (the most widespread) and v2. The TK95 always has v2.
- **ROM language** (`tklang=`): "automatic" follows the menu language. The originals only have Spanish and Portuguese: v1 has both in the same ROM and chooses at boot with a diode fitted to the TK90X sold in Brazil (the emulator simulates it with Portuguese); v2 and the TK95 have one ROM per language. v3 comes in 17 languages.
- **Port 254**: on the TK, bits 5 and 7 are not connected and give whatever is on the data bus (the byte the ULA is reading from video memory, or 1 when it is not reading); with the diode of the Brazilian TK90X, bit 7 is 0. Some games that read this port without masking those bits do not work on a real TK, nor here.

**Rodolfo Guerra's v3 ROM** (2021, unofficial) is the result of three years of work starting from v1, the most consistent one, adding what v2 brought. It is the most compatible TK ROM to date and includes, among many other improvements: native turbo loading at 4000 baud (without losing normal loading: the ROM detects which one is on the tape), an improved BASIC editor and faster keyboard reading. To gain space each ROM has a single language, hence one for each of the 17. All its features are described in the manuals (in Spanish, English and Portuguese) on his page: https://sites.google.com/view/rodolfoguerra . In the emulator, turbo loading should work with real tape loading, which plays the tape at its own speed; instant and fast loading only speed up normal loading for now.

## ROMs

Only the **16K/48K and 128K** ROMs are built into the kernel, so the emulator always boots. The others are loaded at power-on from the **`roms`** folder of the SD card, where they come included:

| File | Size | Machine or peripheral |
| --- | --- | --- |
| `plus2.rom` | 32 KB | Grey +2 |
| `plus2a.rom` | 64 KB | +2A |
| `plus3.rom` | 64 KB | +3 (and .DSK disks) |
| `plus3e-es.rom` / `plus3e-en.rom` | 64 KB | +3e (and hard disk); chosen with `p3elang=` |
| `if1.rom` | 8 KB | Interface 1 (microdrives, RS232, ZX Net, LEC's CP/M) |
| `mf1.rom`, `mf128.rom`, `mfplus3.rom` | 8 KB | Multiface One, 128 and 3 (F5); without them, F5 does a plain NMI |
| `esxmmc.bin` | 8 KB | esxDOS 0.8.9 for the divMMC (`divmmc=esxdos`) |
| `spectranet.rom` | 128 KB | Spectranet flash with the firmware installed (`spectranet=1`) |

**If one is missing** (or has the wrong size), whatever needs it becomes unavailable and the rest works:
- F7 skips that machine;
- `model=` boots on the 48K;
- the browser and the web page warn when loading a `.DSK`, a hard disk image or a microdrive;
- snapshots for that machine load on the 128K, with a warning.

At boot, "Missing ROMs: …" appears. Everything is explained in `roms/README.txt`, in case someone deletes a ROM by mistake.

**Your own ROMs**: `48.rom` and `128.rom` replace the ones built into the kernel, and `plus3e.rom` takes priority over the per-language +3e ones. With `rom48=`, `rom128=`, `romplus2=`, `romplus2a=`, `romplus3=`, `romplus3e=` and `romif1=` you can name another file in the folder (for example `romplus3e=zxp3.rom`). The tape traps only work if the ROM has the standard routines in place.
