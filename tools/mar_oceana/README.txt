MAR OCEANA MENU FOR ZXMETALCIRCLE
=================================

(English version of LEEME.txt.)

desUBIkado's "Mar Oceana" image (2022-01-23_Mar_Oceana_ZX-Uno_Spectrum+3e)
boots with a menu designed for the ZX-Uno: FilePlus, uGophy and NetMan use
the ZX-Uno's WiFi module, and "ZXUC Core Setup" configures the FPGA core. In
ZXMetalCircle they do nothing, so this menu removes them and keeps:

  Workbench +3e, CP/M 3.0, +3e Explorer, WB +3e Setup and Exit

It also removes the OUT 36411 (ZX-Uno speed register) and the title becomes
"ZXMetal +3e". The rest of the program is the original.

HOW TO INSTALL IT (from the emulator itself, without a PC)
----------------------------------------------------------
1. Mount the image (hdd=...) and in the +3e menu choose "+3 BASIC".
2. F1, choose utils/mar_oceana/menu_zxmetal.tap (the tape stays inserted).
3. Type:   LOAD "t:"              (tape as the default drive)
           LOAD ""                (loads the DISK program)
           SAVE "c:DISK" LINE 1   (saves it to the "sistema" partition)
4. F12 (reset) and "Cargador" (Loader): the new menu appears.

The original DISK is kept as DISK.BAK in the same partition; to go back:
ERASE "c:DISK" and MOVE "c:DISK.BAK" TO "c:DISK".

HOW TO INSTALL IT (from the PC, with the 3e utility)
----------------------------------------------------
  3e image.img del 63:DISK
  3e image.img put DISK 63:DISK

(63 is the number of the "sistema" partition in this image.)

mkmenu.py is the program that generates DISK and menu_zxmetal.tap from the
original DISK.
