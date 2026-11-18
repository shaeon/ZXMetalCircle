# Building ZXMetalCircle on Linux and Windows

This guide explains how to set up a computer to build the emulator and generate the SD card, just as the GitHub Action does.

- **Linux:** builds directly (tested on Ubuntu and Debian).
- **Windows:** builds inside **WSL** (the Linux built into Windows 10 and 11). The steps inside WSL are the same as on Linux.
- **Shortcut, on either:** if you have **Docker**, you can skip steps 1 to 3 and build with the same image the Action uses (see "Building with Docker", below).

You need about **3 GB of disk space** and an internet connection: the first build downloads Circle and the Raspberry Pi firmware.

---

## 1. Windows only: install WSL

1. Open **PowerShell as administrator** and type:
   ```
   wsl --install -d Ubuntu-24.04
   ```
2. Restart the PC if asked to.
3. Open **Ubuntu** from the Start menu. The first time it asks for a Linux user name and password (they do not have to match your Windows ones).

From here on, **everything is typed in the Ubuntu window**, just as on Linux.

> **Important:** work inside the Linux folder (`~`, your Ubuntu home folder), **not** in `/mnt/c/...` (the Windows drives). From `/mnt/c` the build is much slower, and Git may change the scripts' line endings and make them fail with errors such as `bad interpreter`.

---

## 2. Install the tools

On Linux (or in the WSL Ubuntu window):

```bash
sudo apt update
sudo apt install -y git wget make python3 mtools dosfstools zip xz-utils
```

| Tool | What for |
| --- | --- |
| git | downloading the code and Circle |
| wget | downloading the Raspberry Pi firmware |
| make | building |
| python3 | the translation tools (`tools/i18n`) |
| mtools, dosfstools | preparing the SD card's FAT images |
| zip | packaging the SD card |
| xz-utils | unpacking the compiler |

---

## 3. Install the compiler (Arm GNU Toolchain 14.2)

The emulator is built with the **Arm GNU Toolchain 14.2.rel1**: one for 32 bits (`arm-none-eabi`) and another for 64 bits (`aarch64-none-elf`). It is the same one the Action uses.

```bash
mkdir -p ~/arm-toolchain
cd ~/arm-toolchain
base=https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel
for t in arm-none-eabi aarch64-none-elf; do
  wget "$base/arm-gnu-toolchain-14.2.rel1-x86_64-$t.tar.xz"
  tar xf "arm-gnu-toolchain-14.2.rel1-x86_64-$t.tar.xz"
  rm "arm-gnu-toolchain-14.2.rel1-x86_64-$t.tar.xz"
done
```

Add it to the `PATH` so that it is always found, also when opening a new window:

```bash
for d in ~/arm-toolchain/*/bin; do echo "export PATH=\"$d:\$PATH\"" >> ~/.bashrc; done
source ~/.bashrc
```

Check that both respond:

```bash
arm-none-eabi-gcc --version
aarch64-none-elf-gcc --version
```

Both must say `14.2.Rel1`.

> If your computer is ARM-based (for example a Mac with Apple Silicon or Linux on ARM), replace `x86_64` with `aarch64` in the file names.

---

## 4. Get the code

```bash
cd ~
git clone https://github.com/shaeon/ZXMetalCircle.git
cd ZXMetalCircle
```

To build the development branch:

```bash
git checkout dev
```

---

## 5. Build

**All Raspberry Pi models** (the same as the Action):

```bash
./build.sh
```

The first time it downloads and builds Circle for each model and downloads the firmware: it takes a while (15 to 30 minutes, depending on the computer). After that only what has changed is rebuilt.

**Only some models**, much faster for testing:

```bash
./build.sh 3-64            # just one
./build.sh "1 3-64 5"      # several
```

| Target | Models | Bits | Kernel |
| --- | --- | --- | --- |
| `1` | Pi 1, Zero, Zero W | 32 | `kernel.img` |
| `2` | Pi 2 | 32 | `kernel7.img` |
| `3-32` | Pi 3, Zero 2 W | 32 | `kernel8-32.img` |
| `4-32` | Pi 4, 400 | 32 | `kernel7l.img` |
| `3-64` | Pi 3, Zero 2 W | 64 | `kernel8.img` |
| `4-64` | Pi 4, 400 | 64 | `kernel8-rpi4.img` |
| `5` | Pi 5, 500 | 64 | `kernel_2712.img` |

When it finishes it says:

```
Listo: copia el contenido de .../sdcard a una tarjeta FAT32.
```

("Done: copy the contents of .../sdcard to a FAT32 card"), and for each kernel it shows how much memory it takes (`Tamaño del kernel en memoria: ... (47 % de 4 MB)`).

Useful options:

| Variable | What for |
| --- | --- |
| `JOBS=4 ./build.sh` | build with 4 processes (by default, all the computer has) |
| `QEMU=1 ./build.sh 3-64` | kernels for testing in QEMU (in the `qemu/` folder; they do not go on the SD card) |
| `LIMPIO=1 ./build.sh` | build from scratch. Without it, only what has changed since the last build is recompiled (and whatever uses a header that was touched) |

---

## 6. Copy the result to the SD card

The contents of the **`sdcard`** folder go in the root of a card formatted as **FAT32**.

- **On Linux:** copy **the contents** of `sdcard` (not the folder itself) to the root of the card.
- **On Windows (WSL):** the folder is at `\\wsl$\Ubuntu-24.04\home\YOUR_USER\ZXMetalCircle\sdcard`. You can open it in Windows Explorer by typing, inside the project folder in Ubuntu:
  ```bash
  explorer.exe sdcard
  ```
  and copy **all its contents** to the root of the card.

What is **not** included in the generated SD card: the esxDOS card (`esxdos.img`), which the emulator creates itself.

---

## 7. Optional: the tests on the PC

The same tests the Action runs, without needing the Pi:

```bash
cd test/host
S=../../src
g++ -O2 -std=c++17 -I. -I$S -o update_test update_test.cpp $S/updater.cpp $S/zipfile.cpp $S/inflate.cpp
./update_test
```

It must end with `todo OK` ("all OK"). It needs the PC's normal compiler (`sudo apt install -y g++`). The other tests, and how they are built, are in `.github/workflows/build.yml`.

## 8. Optional: testing in QEMU

```bash
sudo apt install -y qemu-system-arm
QEMU=1 ./build.sh 3-64
```

QEMU needs an SD card image. Create it once, with the contents of `sdcard` (and the games you want, added later with `mcopy`):

```bash
dd if=/dev/zero of=sd.img bs=1M count=256
mkfs.vfat -F 32 sd.img
mcopy -s -i sd.img sdcard/* ::/
```

And start it like this (the `cmdline.txt` options go in `-append`):

```bash
qemu-system-aarch64 -M raspi3b -kernel qemu/kernel8.img -drive file=sd.img,if=sd,format=raw -device usb-kbd -append "model=128k"
```

QEMU does not emulate sound or networking, and it is much slower than a real Pi; it is useful for checking menus, file loading and booting.

---

## Building with Docker

The GitHub Action builds inside a Docker image with everything already installed (the tools and the compiler). The same image works at home: you only need Docker (on Windows, **Docker Desktop**).

1. Get the code **without converting line endings** (otherwise the script fails with `$'\r': command not found`):
   ```
   git clone -c core.autocrlf=false https://github.com/shaeon/ZXMetalCircle.git
   cd ZXMetalCircle
   ```
2. Build, from that folder. On Linux, or on Windows with PowerShell:
   ```
   docker run --rm -v "${PWD}:/src" ghcr.io/shaeon/zxmetalcircle-build:1 ./build.sh
   ```
   As in step 5, you can build a single model (for example, `./build.sh 4-64`).

The result ends up in the `sdcard` folder, as when building without Docker. The first time, Docker downloads the image, which takes about 2 GB of disk space.

## If something fails

| Message | What to do |
| --- | --- |
| `Falta el compilador ...gcc` (compiler missing) | the compiler is not in the `PATH`: repeat the end of step 3 and open a new window |
| `bad interpreter` or `$'\r': command not found` | the code was downloaded with Windows line endings: clone it inside WSL (step 4), not on `C:` |
| `mcopy: command not found` | `mtools` is missing (step 2) |
| It stops while downloading Circle or the firmware | check the connection and run `./build.sh` again; if it keeps failing, delete the `build` folder (below) and start from scratch |
| `ERROR: el kernel ocupa ... (máximo 4 MB)` (kernel too big) | the kernel has grown too much: large buffers have to be moved to dynamic memory |

To start from scratch (for example, after changing the Circle version), delete the `build` folder:

```bash
rm -rf build sdcard
./build.sh
```

---

## More details

### Building

You need both Arm GNU Toolchains (tested with 14.2): `arm-none-eabi` for the 32-bit kernels and `aarch64-none-elf` for the 64-bit ones, plus `git`, `wget`, `make`, `python3`, `mtools` and `dosfstools`:

```
./build.sh              # builds every kernel and leaves everything in sdcard/
./build.sh 3-64         # a single one
./build.sh "1 3-64 5"   # several
```

**On GitHub** (`.github/workflows/build.yml`) it builds by itself: on every push to `main` and on pull requests it runs the PC tests, builds the seven kernels and leaves the SD card as a zip in the *Actions* tab (*Artifacts* of each run). It can also be started by hand (*Actions → Build → Run workflow*). When a `vX.YY` tag is pushed it also creates a draft release with that zip: you only have to paste the notes and publish it.

The script downloads Circle and the Raspberry Pi firmware, builds the Circle libraries for each target (with multi-core support on the Pi 2, 3 and 4) and generates these kernels:

| Target | Model | Bits | Kernel |
| --- | --- | --- | --- |
| `1` | Pi 1 / Zero / Zero W | 32 | `kernel.img` |
| `2` | Pi 2 | 32 | `kernel7.img` |
| `3-32` | Pi 3 / Zero 2 W | 32 | `kernel8-32.img` |
| `4-32` | Pi 4 / 400 | 32 | `kernel7l.img` |
| `3-64` | Pi 3 / Zero 2 W | 64 | `kernel8.img` |
| `4-64` | Pi 4 / 400 | 64 | `kernel8-rpi4.img` |
| `5` | Pi 5 / 500 | 64 | `kernel_2712.img` |

The compiler prefixes can be changed with `PREFIX32=` and `PREFIX64=`.
