# Picture and sound

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## Beeper sound

The beeper (and the loading sound) goes through a speaker model taken from **Fuse** (by Fredrick Meunier: `sound/ula_filter.c`, `speaker_filter.c` and `tv_filter.c`), followed by a filter against the whine of 1-bit routines. The AY does not go through them.

1. **The ULA**: an asymmetric low-pass (rising with 36.5 µs and falling with 68.9 µs), tuned in Fuse by listening to recordings of real machines.
2. **The speaker**, according to the `speaker=` option ("Speaker" in the settings, page 2, applied at once):
   - `auto` (default, as in Fuse): the 16K and 48K sound through their **internal speaker**, and the 128K, +2 and +3 through the **television**. It changes by itself when the machine changes;
   - `beeper`: the 48K's internal speaker, which has no bass (750 Hz high-pass); it sounds thinner, like the real one;
   - `tv`: a television (100 Hz – 10 kHz);
   - `off`: no model (only the DC component is removed): the sound of version 0.55.
3. **Remove the 1-bit whine** (`beeperfilter=1`, "Remove 1-bit whine"): a 4th-order low-pass at 6 kHz. Many speech or multi-channel routines use a 10–20 kHz carrier that is not heard on the real machine; Fuse's model only lowers it by about 13 dB, so this filter stays: with the *We Are Vocoders* demo, the carrier (about 16 kHz) drops by about 50 dB and what is heard below 4 kHz stays the same.

Each model has its own volume compensation (the 48K one removes the bass and would otherwise sound quieter).

## Several cores (Pi 2, 3, 4 and Zero 2 W)

On the 4-core models, **scaling the image to the screen** (the magnification, the scanlines, the huge border) is done on **core 1**, while core 0 keeps everything else: emulation, sound, USB, network and SD card. It is what moves the most memory in each frame (about 4 MB at 1080p), so core 0 gets more headroom for the network, downloads, esxDOS or the Spectranet.

- Core 0 hands over each finished frame and core 1 scales it into the hidden page of the screen; core 0 flips the page as soon as core 1 has finished. If core 1 is still busy with the previous one, that frame is not shown (as when the emulation runs late). The image may arrive up to one frame (20 ms) later.
- Border or scanline changes pause core 1 for a moment.
- **Alt+D** shows how long each frame takes on core 0 and the scaling on core 1 (out of the 20 ms a frame lasts). On the single-core Pi 1 and Zero, it shows core 0's time including the image.
- `multicore=0` (or "Video on core 1" in the settings, page 2) goes back to doing everything on core 0. Applied after a restart.
- Internally: `src/videocore.h/.cpp` and Circle built with `ARM_ALLOW_MULTI_CORE` (Pi 2, 3 and 4 only; the Pi 5 uses a single core for now).
- **Tested**: the hand-over between the two cores, on the PC with threads (3,000 frames with random timings and 31 mode changes, without errors), and in QEMU with the 64-bit Pi 3 kernel (QEMU cannot boot the 32-bit Pi 2 and 3 kernels). It still has to be tested on a real Pi.

## Speed test

`utils/speedtest.tap` on the SD card is a program to check that the emulator runs at the right speed (the source is in `tools/speedtest`):

- **It measures the t-states of a frame**: 69888 on the 48K, 70908 on the 128K/+2/+2A, within ±2.
- **It shows the elapsed time according to the Spectrum itself** (frames × exact frame length). Every second there is a click and the border changes. Compare it with a stopwatch: after 10 minutes there should not be more than 1 second of difference.
- SPACE measures again.
- It loads on the 48K, 128K, +2 and +2A with `LOAD ""` or the Tape Loader. It does not work on the 16K, because the code goes at 32768.

The help (F1) also shows the real speed measured by the emulator, in frames per second: it should read 50.08 on the 48K and 50.02 on the 128K.

## Image: border and scanlines

**Border sizes** (the same as JSpeccy):

| Border | Image |
| --- | --- |
| None | 256 × 192 |
| Standard | 320 × 240 |
| Full | 352 × 296 (default) |
| Huge | 384 × 304 |

- The emulator always generates the image with the huge border and shows the chosen part. The less border, the bigger the Spectrum screen looks.
- With a small border, the menus enlarge the image to the full border while they are open.

**Scanlines**:
- The Raspberry Pi GPU scales the image keeping its aspect ratio. So the framebuffer always has the same aspect ratio as the screen, and each Spectrum pixel takes r × r pixels (r = 2 or 3).
- In each group of r lines, the last one is drawn at half brightness. The GPU then applies an integer scale, so all the scanlines look the same.
- The emulator chooses r and the scale so the image is as big as possible. For example, with the full border:

  | Screen | Visible image |
  | --- | --- |
  | 1920×1080 | 1056×888 |
  | 1920×1200 or 1600×1200 | 1408×1184 |
  | 4K | 2112×1776 |

  The calculation is in `src/videolayout.h` and is checked by `test/host/scanlines_layout_test.cpp`.
- **Size with scanlines**: the biggest integer magnification that fits is used (for example, x5 with the standard border at 1200 lines, which fills the screen). On the Pi 2, 3 and 4, if that magnification leaves the image below 90 % of its size without scanlines, a **fractional magnification at native resolution** is used: each Spectrum line takes 3 or 4 screen lines (or 4 or 5, depending on the case) and the last one is darkened. At those resolutions the difference between lines is hardly noticeable, and the image takes the same space as without scanlines.
  - Examples: the huge border at 1200 lines goes from x3 (76 %) to x3.95 (99 %); at 1080 lines, every border fills the screen.
  - On the Pi 1 and the Zero, with less power, the integer magnifications with a small framebuffer are kept.
  - On the 4-core Pis this scaling runs on core 1 (see "Several cores").
- **Sharpness**: `config.txt` has `scaling_kernel=8`, which makes the GPU enlarge with the nearest-neighbour method. Without that line, the GPU smooths the image when enlarging it and, without scanlines, the letters look blurred.
- If the screen is too small, the emulator says so and does not turn them on. Writing more pixels per frame costs some CPU; on a Pi Zero it may be noticeable.

## Sound

- **Stereo AY**: the three channels are spread between the two speakers, and the centre channel is heard on both sides at half volume. The possible layouts are:
  - **ABC** (default, the most common);
  - **ACB**;
  - **BAC**;
  - **mono**.

  Chosen with `aystereo=` or changed on the fly with F9. With the GPIO12 output the sound is always mono.
- **Loading sound**: as on the real Spectrum, the tape signal reaches the speaker and the pilot tone and the data can be heard. It is only heard when loading at real speed: in fast mode the emulator runs flat out and goes silent, and in instant mode there is no signal. Turned off with `tapesound=0`.
- **Beeper**: see "Beeper sound" (speaker model and 1-bit whine filter).

## Issue 2/3 and speed

- **Issue 2/3**:
  - on 16K/48K Issue 2 boards, the EAR bit read from port 0xFE is set both by the EAR bit (4) and by the MIC bit (3) of the last OUT. On Issue 3 and later, only by EAR;
  - some old programs, written for Issue 2 boards, only read the keyboard properly in that mode;
  - the default is Issue 3. It is changed with `issue=2` or in the settings (F1, C, page 1), and only affects the 16K and 48K models;
  - .Z80 snapshots save and restore this setting.
- **Speed**:
  - F10 cycles between x1, x2, x4 and x10;
  - above x1, several frames are emulated for each screen frame and the sound is muted;
  - x10 equals a 35 MHz Z80. Whether it is reached depends on the Raspberry Pi: the Pi 3/4 should have power to spare, a Zero may not get there.
