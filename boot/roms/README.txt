roms FOLDER
===========

(English version of LEEME.txt.)

Only the ZX Spectrum 16K/48K and 128K ROMs are built into the emulator, so
it always boots. All the others are loaded from this folder at power-on. If
one is missing (or has a different size), the machine or peripheral that
needs it becomes unavailable: the rest keeps working. At boot the warning
"Missing ROMs: ..." appears with the list.

The Sinclair and Amstrad ROMs are distributed with Amstrad's permission for
use in emulators; the +3e ones, with Garry Lancaster's permission. The
Multiface ROMs (c) Romantic Robot are not included: we have no permission to
distribute them. If you own a Multiface, copy the dump of its ROM here. esxDOS belongs to Papaya Dezign (https://esxdos.org), for
non-commercial use. The Spectranet firmware is by Dylan Smith (MIT licence).


1) ROMS THAT COME WITH ZXMETALCIRCLE
------------------------------------

 File            Size         What it is for       If it is missing...
 --------------  -----------  -------------------  ---------------------------
 plus2.rom       32768 bytes  Grey ZX Spectrum +2  F7 skips the +2.
                                                   model=plus2 boots the 48K.
                                                   +2 snapshots: on the 128K.

 plus2a.rom      65536 bytes  ZX Spectrum +2A      F7 skips the +2A.
                                                   model=plus2a boots the 48K.
                                                   +2A snapshots: on the 128K
                                                   (they may not work).

 plus3.rom       65536 bytes  ZX Spectrum +3       No +3 and no .DSK disks
                              (4.0 ROMs)           (the browser and the web
                                                   warn "roms/plus3.rom is
                                                   missing"). +3 snapshots:
                                                   on the 128K.

 plus3e-es.rom   65536 bytes  ZX Spectrum +3e      No +3e and no hard disk
 plus3e-en.rom                (ZXMMC version,      (.vhd .img .raw .hdf).
                              Spanish/English)     The one for the language
                                                   chosen with p3elang=es
                                                   (default) or p3elang=en is
                                                   used. One language is
                                                   enough.

 if1.rom          8192 bytes  Interface 1 (v2)     No Interface 1: no
                                                   microdrives (.mdr .mdv,
                                                   Alt+N), RS232, ZX Net or
                                                   LEC's CP/M.

 esxmmc.bin       8192 bytes  esxDOS 0.8.9 for     The divMMC cannot be
                              the divMMC           connected.

 spectranet.rom 131072 bytes  Spectranet flash     The Spectranet cannot be
                                                   connected.

If you delete one by mistake, it is in the ZXMetalCircle zip (roms folder
of zxcircle-sd.zip).


2) ROMS YOU CAN ADD YOURSELF
----------------------------

 Multiface ROMs (not included; the dump of your own Multiface's ROM):

   mf1.rom       8192 bytes   Multiface One (16K/48K)
   mf128.rom     8192 bytes   Multiface 128 (128K and +2)
   mfplus3.rom   8192 bytes   Multiface 3 (+2A, +3 and +3e); mf3.rom also works

   Without them, F5 does a plain NMI.

 Your own ROMs, instead of the included ones:

   48.rom       16384 bytes   for the 16K and 48K
   128.rom      32768 bytes   for the 128K (ROM 0 + ROM 1 one after the other)
   plus3e.rom   65536 bytes   for the +3e; if it exists, it takes priority
                              over plus3e-es.rom and plus3e-en.rom

   The 64 KB ROMs are the four banks (0, 1, 2 and 3) one after the other;
   the 32 KB ones, banks 0 and 1.


3) OTHER NAMES
--------------

In cmdline.txt you can name another file in this folder for each machine:
rom48=  rom128=  romplus2=  romplus2a=  romplus3=  romplus3e=  romif1=
For example, to use the same ROM as in the zxp3 core for MiST:

   romplus3e=zxp3.rom

The +3e needs the ROM version for the ZXMMC interface ("mmc"), which is
the one ZXMetalCircle emulates.

The tape loading and saving traps (instant loading, SAVE to the SD card)
only work if the ROM has the standard routines in place.
