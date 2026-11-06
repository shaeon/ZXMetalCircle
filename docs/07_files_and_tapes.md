# Files and tapes

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## ZIP files

`.zip` files load straight from the file browser (F1) or the web page, without unzipping them first:

- **If the ZIP holds a single loadable file**, it loads when you choose the ZIP. If it holds several (for example, both sides of a game), a list lets you choose; other files (texts, instructions) are not shown.
- **Tapes, snapshots and cartridges** (`.tap`, `.tzx`, `.pzx`, `.csw`, `.z80`, `.sna`, `.szx`, `.rom`) load on the fly, from memory, without writing anything to the card.
- **Disks and Microdrive cartridges** (`.dsk`, `.mdr`, `.mdv`), which the program may write to, are extracted to the `tmp` folder on the card (created if missing) and loaded from there, so whatever is saved stays in that file. If it is already in `tmp`, you are asked whether to use the existing one (with its changes) or overwrite it with the one in the ZIP.
- Normal ZIP files are supported (stored or deflate, which is almost all of them), up to 32 MB and 512 files. Not encrypted ones.
- With many files to choose from, the list scrolls: cursor keys, PgUp/PgDn, Home and End; the top right shows where you are ("23/100").

## Drives (F1 and then U, or Alt+U)

A screen with what is inserted in each drive: **tape**, the +3 **disk A:** and **disk B:**, **hard disk** (+3e or divMMC), **Interface 2 cartridge** and **microdrives 1 to 8**.

- **ENTER** opens the file browser with only the files for that drive (tapes, `.dsk`, hard disk images, `.rom`, `.mdr`/`.mdv`), and what you choose goes into that drive. So a disk can be inserted into drive **B:** or a cartridge into a given microdrive. After choosing, you go back to the drives screen; ESC in the browser also goes back.
- **DEL** (or Backspace) empties the drive, saving whatever is pending first (disks, cartridges, what esxDOS wrote).
- What is mounted here is **for this session only**. The hard disk the Pi boots with is chosen in the settings ("Boot hard disk", page 1), from the images in the `hdd` folder (those without spaces in their name: `cmdline.txt` separates options with spaces).

## Interface 2 cartridges

The Sinclair Interface 2 had a slot for 16 KB ROM cartridges which, when plugged in, replaced the Spectrum ROM; the game started at power-on. There were about ten games in this format (Jetpac, Pssst, Cookie, Tranz Am, Chess…).

- They are loaded from the browser (F1, ENTER), choosing a `.ROM` file of up to 16 KB. If it is smaller, the rest is filled with 0xFF.
- Inserting it resets the machine and the cartridge starts. **F11** removes it and boots again with the normal ROM.
- It works on the 16K, 48K, 128K and +2. The +2A does not support it, as in JSpeccy.
- The Interface 2 joysticks are the Sinclair 1 and Sinclair 2 modes.
- Loading a snapshot removes the cartridge, unless it is a .SZX that includes it (IF2R block).

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
