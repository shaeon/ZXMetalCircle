esxDOS v0.9.0
(c) 2005-2026 Papaya Dezign
---------------------------

This version has the following (notable) new features (for more details check the changelog):

* A proper manual for the first time, courtesy of SCjoe!
* LFN support, limited to 65 chars
* Full support for Timex TS/TC2068, courtesy of Hugo Pinto of the LOAD ZX Museum team!
* New BASIC commands: NEW, COPY, MOVE, FORMAT, and streams with OPEN #, CLOSE #, POINT # and CLEAR #
* LOAD/SAVE/VERIFY now support DATA, and CAT, ERASE, MOVE and COPY support wildcards
* System commands now live in /SYS/BIN
* New Commands: .df, .mount, .umount, .trkplay, .fm, .vi, .editor, .sview, .dbasic, .fontedit and .touch
* Updated Commands: .ls, .cp, .mv, .rm, .cd, .mkdir, .chmod, .file, .snapload, .divideo and .sercp

How to (safely) install/upgrade:
--------------------------------

1. Copy the tape from FLASHERS for your interface to your CF/SD/HDD, load it and follow the instructions. Power Off and remove media. The ROM images are in ROMS.
2. Copy the SYS, BIN (and /TMP if using DivIDE) directories to your CF/SD/HDD.
3. Insert media. Power On. Reset while keeping space pressed to reinit if needed.

Notes:
------

a) If you replace the SYS directory *before* flashing the new ROM, your previous ROM of esxDOS will not boot correctly.
b) The system commands are now in /SYS/BIN, and are found before the ones in /BIN. Old copies of them left in /BIN are not used and can be deleted.
c) NMI.SYS is now loaded at $2000 and RTC.SYS at $2F00: third-party NMI.SYS or RTC.SYS files made for older versions must be reassembled for these addresses.
d) Drive names in .command arguments must now be quoted: .ls "hd1:/games"
e) Questions? Bugreports? Ask on the usual places or send an e-mail to: bugs at esxdos dot org

Changelog
---------

[02/01/2022] auto-LOAD: "AutoLoadColour" parameter in ESXDOS.CFG was not being applied (ub880d)
[02/01/2022] TR-DOS: Fixed a bug with the RETURN command (lordcoxis + reported by zxcygnus)
[22/03/2023] TR-DOS: Fixed games with the stack just above the screen getting their attributes overwritten (lordcoxis + reported by Shadow Maker)
[22/09/2023] FAT Driver: Rewrote path handling: ".." at the root stays at the root, a directory can no longer be moved into itself, and paths over 127 characters no longer overflow (lordcoxis)
[22/09/2023] FAT Driver: Volume label was treated as a file (lordcoxis)
[29/05/2024] FAT Driver: Backported fat_seek() from esxDOS v1.x (lordcoxis)
[29/05/2024] FAT Driver: Fixed a *critical* bug which corrupted the FAT when copying files between two drives (lordcoxis)
[01/06/2024] Tape Emulator: Backported the version from esxDOS v1.x (lordcoxis)
[01/06/2024] FAT Driver: Backported FAT_FSINFO from esxDOS v1.x (lordcoxis)
[01/06/2024] FAT Driver: Made sure the +3DOS header gets written whenever there's a filesize change (lordcoxis)
[27/09/2026] Kernel: Merged Hugo Pinto's amazing TC2068 patches (Hugo Pinto)
[28/09/2026] FAT Driver: Fixed a long-standing bug that would corrupt the root dir on FAT32 when dealing with empty files (ub880d+lordcoxis)
[30/09/2026] FAT Driver: Fixed a write bug with disk full, and a truncate() bug (lordcoxis)
[02/10/2026] FAT Driver: Backported the F_FCNTL(getpath) syscall from esxDOS v1.x (lordcoxis)
[02/10/2026] FAT Driver: Backported the smaller FSYS_TAB table code from esxDOS v1.x (lordcoxis)
[03/10/2026] FAT Driver: Backported the LFN driver from esxDOS v1.x (lordcoxis)
[03/10/2026] BASIC: Updated CAT to adapt better to long filenames (lordcoxis)
[04/10/2026] Kernel: Added rst $30 DRV_PREFIX syscall as a kludge for esxDOS v1.x style drives (lordcoxis)
[04/10/2026] BASIC: Added DATA to LOAD, VERIFY and SAVE (lordcoxis)
[04/10/2026] Commands: .cd, .cp, .mv, .mkdir and .file accept drive names, and .cp and .mv accept wildcards (lordcoxis)
[05/10/2026] BASIC: Added MOVE "old" TO "new" command (lordcoxis)
[05/10/2026] BASIC: Added wildcards to CAT, ERASE and MOVE (lordcoxis)
[06/10/2026] BASIC: LOAD can no longer overwrite esxDOS at $0000-$3FFF (lordcoxis)
[06/10/2026] BIOS: ESXDOS.SYS and BETADISK.SYS are now checked against the ROM version (lordcoxis)
[07/10/2026] BASIC: Added FORMAT command, for FAT16 and FAT32 (lordcoxis)
[08/10/2026] BASIC: Added NEW "dir" and COPY "file" TO "file/dir" commands (lordcoxis)
[08/10/2026] BASIC: Added Uni-Dos style streams: OPEN #, CLOSE #, POINT #, CLEAR #, (POINT #s) and (LEN #s) (lordcoxis)
[15/10/2026] Kernel: NMI.SYS is now loaded at $2000 and can be up to 3840 bytes, RTC.SYS moved to $2F00 (lordcoxis)
[17/10/2026] Commands: Added .df, and rewrote .ls with a long listing (-l) and wildcards (lordcoxis)
[20/10/2026] Kernel: System commands moved to /SYS/BIN, searched before /BIN (lordcoxis)
[27/10/2026] Commands: Drive names in arguments must now be quoted, as in esxDOS v1.x (lordcoxis)
[28/10/2026] BASIC: Drive names can be given inside the file name ("hd1:/games"), and COPY can copy to another drive (lordcoxis)
[01/11/2026] Docs: Added the esxDOS User Manual in english language (SCjoe + lordcoxis)
[03/11/2026] NMI: Added the latest version of NMI.SYS by ub880d, with LFN support and 52 columns display (ub880d)
[08/11/2026] Commands: Added .trkplay (PT3, STC and SQT), which replaces .playpt3, .playstc and .playsqt (lordcoxis)
[09/11/2026] BIOS: ESXDOS.SYS is loaded from the first drive that has this ROM's version (lordcoxis + ub880d)

Thanks/Greets
-------------

Greets to all the usual peeps (y'all know who you are - just check v0.8.5's README if in doubt!).
Special thanks to all the great "hardware benefactors": LaesQ, LMN128, Pavel Urbancik, Pavel Vymetalek, SpecNext Team, TFW8b, Tygrys, UB880D, Zaxon!

18.11.2026 / Papaya Dezign - All rights perversed
