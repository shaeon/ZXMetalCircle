# USB gamepads in ZXMetalCircle

How USB gamepads work in the emulator (version 1.06) and what cannot be done yet. This guide will change when gamepad support is extended.

## Supported gamepads

- **With their known mapping**: PS3, PS4 DualShock, Xbox 360, Xbox One, Switch Pro and 8BitDo.
- **Generic HID gamepads**, including Mayflash adapters in DirectInput mode.
- **Up to 4 at a time**, plugged and unplugged while running.

## How they are mapped

**All gamepads drive the same emulated joystick**: the one selected (with F4, the Select button or the `joystick=` option in `cmdline.txt`).

| Gamepad button | Kempston | Fuller | Sinclair 1 | Sinclair 2 | Cursor | QAOPSpaceM |
| --- | --- | --- | --- | --- | --- | --- |
| D-pad and left stick | directions | directions | 6-9 | 1-5 | 5-8 | Q A O P |
| **A / Cross, Y / Triangle, L1, R1** | fire | fire | fire (0) | fire (5) | fire (0) | SPACE |
| **B / Circle, X / Square, L2, R2** | fire 2 | fire | X | Z | 9 | M |

- **Fire 2** is a second button with a useful key for each joystick type: on the Kempston, the second fire bit; on the others, a key that games often use.
- **Select / Share / View / Capture** changes the joystick, in the same order as ZXBaremulator: none → Kempston → Fuller → Sinclair 1 → Sinclair 2 → Cursor → QAOPSpaceM.
- **Start / Options** opens the help and the tape browser.
- **In the menus**, the D-pad moves, fire acts as ENTER and fire 2 as ESC.

### Generic gamepads

Since it is not known which button is which, a fixed layout by number is used:

- directions come from the first "hat" (the D-pad) or from the first two axes;
- buttons 1, 4, 5 and 6 are fire; 2, 3, 7 and 8, fire 2;
- 9 is Select and 10, Start.

## What cannot be done yet

1. **Choosing what each button does.** The layout is fixed: a button cannot be assigned to a specific Spectrum key (for example, a button for the `H` of a game that says "press H for hyperspace").
2. **Two players.** With two gamepads, both move the same joystick. Two players would need, for example, the first gamepad to be Sinclair 1 and the second Sinclair 2, or Kempston and Cursor.
3. **Fixing a generic gamepad that does not match the layout by number**, for example one whose buttons are numbered in a different order.

---

## More details

### USB gamepads

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
