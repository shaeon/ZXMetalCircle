# Joysticks and USB gamepads

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

How to play with USB gamepads and with the keyboard: the two players, which gamepad belongs to whom, and what each button does. Everything is on **page 5 of the settings, "Joysticks and pads"** (Alt+F1 and PgDn).

## Supported gamepads

- **With their known mapping**: PS3, PS4 DualShock, Xbox 360, Xbox One, Switch Pro and 8BitDo.
- **Generic HID gamepads**, including Mayflash adapters in DirectInput mode.
- **Up to 4 at a time**, plugged and unplugged while running.

## The players

Each player has their own **emulated joystick**:

| Row on page 5 | Option | What it is |
| --- | --- | --- |
| Player 1 joystick | `joystick=` | none, Kempston (default), Fuller, Sinclair 1, Sinclair 2, Cursor or QAOP. F4 changes it on the fly |
| Player 2 joystick | `joystick2=` | the same, or none (default: no player 2) |

The Spectrum can only read two joysticks at once if they do not overlap, so **the same one cannot be chosen for both, nor Cursor with Sinclair 1** (they share keys 6, 7, 8 and 0). Combinations that do not work are skipped when choosing. The most common ones:

| Player 1 | Player 2 |
| --- | --- |
| Kempston | Sinclair 1, Sinclair 2, Cursor, Fuller or QAOP |
| Sinclair 1 | Sinclair 2 (the classic for 128K two-player games) |

## Which gamepad belongs to each player

| Row on page 5 | Option | Values |
| --- | --- | --- |
| Player 1 gamepad | `pad1=` | automatic (the first one connected) or a specific gamepad |
| Player 2 gamepad | `pad2=` | automatic (the next one) or a specific gamepad. Greyed out if there is no player 2 |
| Extra gamepads | `padextra=` | they join player 1 (default) or player 2, or are for the menus only |

- **A specific gamepad** is stored by vendor and product (`045e:028e` is an Xbox 360 controller). The arrows go through the connected gamepads; if the chosen one is not there, its code is shown with "(not connected)".
- **With two gamepads of the same model**, the first one connected is player 1's.
- **On gamepads with player lights** (PS3, PS4...), the light shows which player it belongs to: 1 or 2, and off if it is for the menus only.
- **F4** changes player 1's joystick. A button with the `joystick` action (see the profiles) changes the joystick of the player its gamepad belongs to.
- **Without changing anything**, everything works as always: a single player, and all gamepads are theirs.

## The keyboard

- **The cursor keys** belong to player 1 (`cursors=`: keys, joystick or both).
- **The keyboard fire key** (`kbdfire=`): **both Windows keys** (default), the right Ctrl or none. With the Windows keys, **both Ctrl keys are SYMBOL SHIFT**. The numeric keypad 0 is always fire.

## What each button does: the profiles

**By default**, each gamepad does what it always did:

| Gamepad button | Kempston | Fuller | Sinclair 1 | Sinclair 2 | Cursor | QAOP |
| --- | --- | --- | --- | --- | --- | --- |
| D-pad and left stick | directions | directions | 6-9 | 1-5 | 5-8 | Q A O P |
| **A / Cross, Y / Triangle, L1, R1** | fire | fire | fire (0) | fire (5) | fire (0) | SPACE |
| **B / Circle, X / Square, L2, R2** | fire 2 | fire | X | Z | 9 | M |

- **Start** opens the settings (like Alt+F1) and **Select** (Back on Xbox controllers) the file browser.
- **In the menus**, the D-pad moves, fire acts as ENTER, fire 2 as ESC, and **L and R** (LB and RB on Xbox controllers) change page, like PgUp and PgDn.
- **On generic gamepads**, where it is not known which button is which: buttons 1, 4, 5 and 6 are fire; 2, 3, 7 and 8, fire 2; 9, Select (files), and 10, Start (settings). In the menus, 5 and 6 change page.

### Which button is which

On generic gamepads the buttons only have numbers, and you cannot tell which is which. To find out:

- **Alt+D** (diagnostics) shows one line per gamepad: its code, which player it belongs to (`J1`, `J2` or `menu`), **the buttons being pressed**, with the same name or number as in the profiles, and where the stick points. For example: `1 045E:028E J1: LB A stick ↑<`.
- **On the "Gamepad buttons" screen**, pressing a button highlights its row, and the bottom line says **"You pressed: Button 5"**.

**With a profile**, each button does whatever you want. Each gamepad model has its own profiles, **one per joystick type**, and can have **several named ones** for the same type (for example, one per game).

### The "Gamepad buttons" screen

It opens with **ENTER on the last row of page 5**. If several gamepads are connected, it first asks which one. It edits the profile for the joystick type of that gamepad's player.

- **The first row is the profile**: "default" or a named one. ← → go through the saved ones. **N** creates a new one (copying the one shown), **R** renames it and **DEL** deletes it.
- **The other rows, one button each.** Pressing a button on the gamepad highlights its row and the bottom line says which one it is ("You pressed: Button 5"). While the screen is open, that gamepad's buttons do nothing else.
  - **← →**: none, fire, fire 2, up, down, left, right, joystick, help, settings or files.
  - **ENTER**: a Spectrum key, CAPS SHIFT + key, SYMBOL SHIFT + key, or an emulator key (F1 to F12, Alt+F1 to Alt+F12, Enter, Esc, Tab, Pause, PgUp, PgDn, Home, End, Delete and the cursor keys).
  - **DEL**: the button does nothing.
- **G** saves the profile, makes it the active one and applies it straight away.
- **ESC** exits. If there are unsaved changes, it asks.

Spectrum keys stay pressed while the button is held; emulator keys are sent once when it is pressed. The stick (and the "hat" of generic gamepads) always moves the joystick.

### The profile files

They are saved on the SD card, in the `joysticks` folder, one folder per gamepad model (`vendor-product`):

```
joysticks/045e-028e/              Xbox 360 controller
    kempston.txt                  the default Kempston profile
    kempston-SHOOTER.txt          another Kempston profile
    sinclair2.txt
    activo.txt                    kempston = SHOOTER
```

- **`<type>.txt`** is the default profile for that joystick type (`none`, `kempston`, `fuller`, `sinclair1`, `sinclair2`, `cursor` or `qaop`). If it does not exist, the gamepad does what the table above says.
- **`<type>-<name>.txt`** is a named profile, and **`activo.txt`** says which one is used for each type.

They are text files that can be written by hand or copied to another card. One line per button; whatever is not mentioned stays as by default. The file format uses Spanish words, and also accepts English ones:

```
nombre = SHOOTER
A = fire
B = fire 2
X = key H
Y = key SS+P
LB = pc F5
RB = pc Alt+F1
L3 = pc Enter
R3 = key CS+SPACE
Select = files
Start = settings
```

| Buttons | Names |
| --- | --- |
| Known gamepads | A, B, X, Y, LB, RB, LT, RT, Select, Start, L3, R3, Guia (or Guide, Home), Arriba/Up, Abajo/Down, Izquierda/Left, Derecha/Right, Plus, Minus, Touchpad (also Cross, Circle, Square, Triangle, L1, R1, L2, R2...) |
| Generic gamepads | 1 to 32 |

| Action | What it does |
| --- | --- |
| `none` (`ninguno`) | nothing |
| `fire`, `fire 2` (`disparo`, `disparo 2`) | the joystick's fire buttons |
| `up`, `down`, `left`, `right` | the joystick's directions |
| `joystick` | change joystick (like F4) |
| `help` (`ayuda`) | the help (F1); `menu` also works |
| `settings` (`configuracion`) | the settings (Alt+F1; what Start does) |
| `files` (`ficheros`) | the file browser (what Select does) |
| `key <K>` (`tecla <K>`) | a Spectrum key: A-Z, 0-9, ENTER, SPACE (ESPACIO), CS or SS; with `CS+` or `SS+` in front, together with CAPS SHIFT or SYMBOL SHIFT (`key CS+SPACE` is BREAK) |
| `pc <K>` | an emulator key: F1-F12, Enter, Esc, Tab, Backspace, Space, Pause, PgUp, PgDn, Home, End, Delete, the cursor keys, letters and digits; with `Alt+`, `Ctrl+` or `Shift+` in front |

When the emulator saves a profile, it writes it in Spanish (`disparo`, `tecla`...). If a profile has lines that cannot be understood, the rest are applied and a warning appears: "Profile with errors".
