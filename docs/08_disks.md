# Disks

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

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
