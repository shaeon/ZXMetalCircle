# Writing programs and saving them to tape with ZXMetalCircle

This guide explains how to write a BASIC program on the emulated Spectrum, save it to a "tape" and load it back, with a "Hello, world" example.

## 1. Before you start: the keyboard

The PC keyboard works as the Spectrum keyboard:

| PC key | On the Spectrum |
| --- | --- |
| Shift | CAPS SHIFT |
| **Left Ctrl** | **SYMBOL SHIFT** |
| Backspace | DELETE |
| Cursor keys | CAPS SHIFT + 5, 6, 7, 8 |
| `,` `;` `.` `:` `+` `*` `-` `_` `'` `?` `<` `>` | typed directly with their PC key: the emulator presses SYMBOL SHIFT + the right key for you |
| **Quotes `"`** | **Left Ctrl + P** (SYMBOL SHIFT + P) |
| **Exclamation mark `!`** | **Left Ctrl + 1** (SYMBOL SHIFT + 1) |

If you get lost, **Alt+K** shows the keyboard help.

## 2. Choose the model

How you type depends on the model (change it with **F7**):

- **48K (and 16K, TK90X, TK95)**: each command is on a key. At the start of a line the cursor is in **K** (*keyword*) mode, and pressing a key types the whole word: `P` types `PRINT`, `J` types `LOAD`, `S` types `SAVE`... The words on each key are printed on the Spectrum keyboard, and also in the keyboard help (Alt+K).
- **128K, +2, +2A and +3**: in "128 BASIC" or "+3 BASIC" you type words letter by letter, as on a modern computer: `p`, `r`, `i`, `n`, `t` types `print`.

To start with, a **128K or +2** (or the +3 in "+3 BASIC") is easier: you do not need to know the keys.

## 3. Write the program: "Hello, world"

The program:

```basic
10 BORDER 1: PAPER 1: INK 7: CLS
20 PRINT AT 10,9;"Hello, world!"
30 PAUSE 0
```

It sets the border and the background to blue with white letters, clears the screen, writes the message in the middle and waits for a key.

### On a 128K, +2 or +3

1. Start it and choose **"128 BASIC"** (or "+3 BASIC") with the cursor keys and ENTER.
2. Type each line as it is, with its number in front, and press **ENTER** at the end of each one. Remember: quotes are **Ctrl + P** and the exclamation mark is **Ctrl + 1**.
   ```
   10 border 1: paper 1: ink 7: cls
   20 print at 10,9;"Hello, world!"
   30 pause 0
   ```
   If a line has an error, the Spectrum does not accept it and marks where the problem is; fix it and press ENTER again.
3. Type `run` and ENTER to run it. When it finishes (press a key to leave the `PAUSE 0`), wait for the cursor to come back before typing again.

### On a 48K

On the 48K each keyword is a key. Line 20, for example, is typed like this:

| You press | You get |
| --- | --- |
| `2` `0` | `20` |
| `P` | `PRINT` (the cursor was in K mode) |
| SYMBOL SHIFT + `I` | `AT` |
| `1` `0` `,` `9` `;` | `10,9;` |
| Ctrl + `P` | `"` |
| Shift + `H`, then `e`, `l`, `l`, `o`, `,`, space, `w`, `o`, `r`, `l`, `d` | `Hello, world` |
| Ctrl + `1` | `!` |
| Ctrl + `P` | `"` |
| ENTER | the line goes into the program |

The other words of line 10 are on these keys: `BORDER` is `B`; `PAPER` and `INK` are typed in extended mode (press Shift and left Ctrl together, then Ctrl + `C` for `PAPER` or Ctrl + `X` for `INK`); `CLS` is `V`, and `PAUSE` (line 30) is `M`. To run it, `RUN` is `R`.

## 4. Save it to tape

When the program works, save it with:

```basic
SAVE "hello" LINE 10
```

(On the 48K: `S` for `SAVE`, Ctrl + `P` for the quotes, `hello`, Ctrl + `P` again, and `LINE` in extended mode: Shift + left Ctrl together, then Ctrl + `3`.)

`LINE 10` makes the program **start by itself** at line 10 when it is loaded. Without `LINE`, it loads and waits for you to type `RUN`.

The Spectrum answers **"Start tape, then press any key"**: press **ENTER** (or any other key **except SPACE**: at that moment SPACE acts as BREAK and cancels the save, with the message "D BREAK - CONT repeats"). Straight away the message **"Grabado en saves/hello.tap"** ("Saved to...") appears at the bottom and the Spectrum answers "0 OK".

**What the emulator does:** instead of waiting for the real time of a recording, it writes the blocks straight to the SD card:

- The file goes into the **`saves`** folder of the SD card and is named after the program: `saves/hello.tap`. If it already exists, `hello_2.tap`, `hello_3.tap`... are created.
- That file becomes **the current tape**. Whatever you save next is added to the end, as on a real tape.
- **Game tapes are never modified**: if the current tape is a game, a new file is started.
- With the option `savefmt=tzx` (or in the settings, "Formato del SAVE") it is saved as `.tzx` instead of `.tap`.

### On the +3: choosing the tape

By default the +3 saves to and loads from **disk** (drive A:). To use the tape, first type:

```basic
SAVE "t:"
LOAD "t:"
```

`SAVE "t:"` makes `SAVE` save to tape, and `LOAD "t:"` makes `LOAD` load from tape. With `SAVE "a:"` and `LOAD "a:"` you go back to the disk.

## 5. Load it back

1. Clear the program from memory with `NEW` (on the 48K, the `A` key), or reset with F12.
2. Press **F1**, go into the `saves` folder and choose **`hello.tap`**: it becomes the current tape.
3. Load it:
   - **48K:** type `LOAD ""` (`J` and Ctrl + `P` twice) and ENTER.
   - **128K or +2:** choose **"Tape Loader"** in the menu, or type `load ""`.
   - **+3:** choose **"Loader"** in the menu (with no disk inserted, it loads from tape), or type `load "t:"` and then `load ""`.
4. Since it was saved with `LINE 10`, the program starts by itself and "Hello, world!" appears.

If you have just saved and want to load without changing tape, rewind it first with **F3** (Home and ENTER), because after saving it is at the end.

## 6. Take it to the PC (or bring it over)

- **From the PC**, on the emulator's web page (the address is shown in the help, F1), go into the `saves` folder and download `hello.tap`. It works in any Spectrum emulator.
- **The other way round**, you can upload a `.tap` from the PC through the web page and load it on the Spectrum.

## 7. Other things you can save

| Command | Saves |
| --- | --- |
| `SAVE "screen" SCREEN$` | the current screen |
| `SAVE "code" CODE 32768,1000` | 1000 bytes of memory from address 32768 |
| `SAVE "data" DATA a()` | the array `a()` |

Programs with **their own saving routines** (turbo loaders, games that save their progress) do not use the ROM and do not go through this system. For those there is **Alt+W**: it records the tape signal as it is, like a blank tape. Press it before saving and again when done; it creates `saves/rec_001.tzx`.

---

*Tested on ZXMetalCircle 1.06 with a 128K: the program is typed, runs, is saved to `saves/hola.tap` (109 bytes) and, when loaded with "Tape Loader", starts by itself.*

*ZXMetalCircle, by Shaeon (Carlos Palmero). More guides in [docs](README.md).*

---

## Reference: saving with SAVE

### Saving with SAVE

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
