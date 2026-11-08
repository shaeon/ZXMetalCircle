# Network

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## Network: WiFi, web server and time

**Models**: WiFi on the Raspberry Pi 3, 3B+, 3A+, 4, 400, Zero W and Zero 2 W, and **in testing on the Pi 5 and 500** (Circle warns that on the Pi 5 WiFi may be unstable with an HDMI display at the same time; its firmware, `brcmfmac43455-sdio.raspberrypi,5-model-b.*`, is also in `firmware`). The Pis with a network socket (1B, 2B, 3B, 4, 5) can also use cable, as can a USB network adapter supported by Circle. With `net=auto` WiFi is used if there is a `wpa_supplicant.conf` on the SD card; to force cable, `net=eth`.

**Setting up the WiFi**:
1. On the SD card, rename `wpa_supplicant.conf.ejemplo` to `wpa_supplicant.conf`.
2. Write your network's name and password in it. Only 2.4 GHz networks with WPA/WPA2 work.
3. The WiFi chip firmware is already in the SD card's `firmware` folder.

At boot, the connection is made in the background. When it is ready, a message with the address appears, for example `WiFi http://192.168.1.50`, and the help (F1) shows it on the "Network:" line.

**Long operations**: the network and the emulation share core 0, so while a large file (from 32 KB) is read or written through the web page, or ZXDB is searched or downloaded from (HTTPS), the emulation stops for a moment. Meanwhile a notice is shown at the bottom, for example "Network: sending game.tap (512 KB)" or "Network: downloading from ZXDB...".

**Web server**: open that address in the PC's or phone's browser. From there you can:
- browse the folders of the SD card;
- **load** a file into the Spectrum **by clicking its name**: it does the same as choosing it in the Pi's file browser (tape, snapshot, cartridge, microdrive, +3 disk or hard disk). Files the emulator cannot load are shown without a link. A hard disk (`.hdf`, `.img`, `.vhd`, `.raw`) asks for confirmation, because mounting it restarts the Spectrum;
- **see what is inside a ZIP** by clicking its name, and **load** the file you want from it by clicking its name (without choosing it on the Pi's screen). Disks and microdrive cartridges from a ZIP are extracted to `tmp`, as from the Pi; if one is already there, the Pi's screen asks whether to use it or overwrite it;
- **download** any file up to 4 MB, for example your saved games, snapshots or microdrive cartridges. Bigger ones show "over 4 MB": copy them by taking the card out;
- **upload** several files at once to the current folder (up to 32 MB in total, like the ZIP files the emulator opens);
- **rename** files and folders;
- **move** files and folders: "move" marks it, you go to the destination folder and click "Paste here" (or "Cancel"). It is instant, because nothing is copied: it just changes folder;
- **replace** a file with another one you choose on your computer (it asks for confirmation);
- **delete** files and empty folders (it asks for confirmation) and **create folders**.

**What cannot be touched from the web page** (it cannot be renamed, moved, replaced or deleted):
- **What the emulator is using**: a +3 disk or a microdrive cartridge that is inserted, the mounted hard disk (by default, `hdd/esxdos.img`) and the inserted tape if SAVEs go to the SD card. It is shown as "in use": eject it first.
- **System files**, shown as "protected": the boot files in the root (kernels, firmware, `config.txt`, `cmdline.txt`...), the folders the emulator uses (`roms`, `saves`, `snaps`, `descargas`, `joysticks`, `hdd`..., although their contents can be changed, for example to add your ROMs or delete saved games) and everything in `overlays`, `firmware`, `certs`, `locales` and `update`. If you need to change any of that from the web page (for example, to upload a kernel), set **"System files" to "unprotected"** on page 9 of the settings (or `webprotect=0`): then you can, but every operation on a system file asks for confirmation. That option can only be changed on the Pi itself, not from the web page. What is in use stays protected anyway.

Uploads are first written to a temporary file, and only replace the existing one when they finish: if something fails halfway, the original is not lost. And everything that changes something carries a security code that only the page itself knows, so that another web page cannot do it through a hidden link: if you ever see "Request expired", reload the page (it happens, for example, after restarting the Pi).

It also announces itself on the local network (mDNS) as the "ZXMetalCircle" web service.

**Protecting the page** (F1, C, page 9, or in `cmdline.txt`):
- `web=0` turns it off completely (the server is not even started); in F1 the network line says "(web off)".
- `web=ro` makes it **read-only**: you can browse the SD card, download files, search ZXDB and load a game, but not upload, delete, rename, move, replace, create folders or download from ZXDB to the SD card (those options do not even appear).
- `webpass=` sets a **password** (in the settings it is typed with lowercase letters and numbers, and shown as `********`). The page asks for it once and then creates a random session key that goes into the links and forms (Circle's web server gives no access to cookies or to the browser's password). The keys are lost when the Pi is switched off; after 5 failed attempts you have to wait 30 s. It is not encrypted (HTTP): it protects you from curious people on your network, not from someone eavesdropping.

**Time**: taken from `pool.ntp.org`. The files you save (SAVE, snapshots, microdrives) carry the real date and time. The zone is set with `tz=` and European summer time is applied automatically (`dst=1`).

**Zero W**: it has a single core, so the WiFi takes time from the emulation. When connecting (WPA encryption is costly) and while uploading or downloading files there may be short glitches in picture or sound; the Spectrum does not fall behind, because the emulator skips frames if needed. At rest it is not noticeable. Booting with WiFi on a Zero W shows a warning.

## Searching and downloading programs (HTTPS)

The emulator's web page has a search box: you type a title and it searches **ZXDB**, Spectrum Computing's database, through the ZXInfo API.
- Only Spectrum programs are shown: no ZX81, SAM Coupé, etc.
- For each one the best available file is chosen, in this order: TZX, TAP, Z80, SNA, SZX, DSK, CSW…
- With "**download and load**", the emulator fetches it from spectrumcomputing.co.uk, unzips it if it comes as a ZIP, saves it in the **`descargas`** (downloads) folder of the SD card and loads it into the Spectrum. If needed, it changes model, for example to the +3 for a `.DSK`.

**How it works**:
- `src/download/httpsclient.cpp` is an HTTP/1.1 client with redirects and chunked responses.
- **Mbed TLS 3.6** (`third_party/mbedtls`, Apache-2.0 licence) encrypts the connection with TLS 1.2. Its configuration is in `zxmc_config.h`: ECDHE/RSA, AES-GCM, ChaCha20.
- `src/download/zxinfo.cpp` parses the JSON with **jsmn** (MIT) and unzips the ZIPs with the emulator's own decompressor.
- Connections are made in a Circle task with a 128 KB stack, so the Spectrum keeps running meanwhile.

**Certificates**: the server's certificate is checked against Mozilla's root authority list, in `certs/cacert.pem` on the SD card. The expiry date is not checked, because Mbed TLS was built without a clock. With `tlsverify=0` nothing is checked; use it only if it fails because of a certificate and you trust the network.

**Tested**: on the PC, with the same files and normal sockets, search and download work against the real servers (Manic Miner, Head over Heels…). Also used on the Pi.

## TNFS servers (Spectranet) in the file browser

With the network running, the root of the browser (F1) has a first entry **`[Internet (TNFS)]`**:
- Inside are the Spectrum TNFS servers from `SD:/tnfs.txt`: by default tnfs.markround.com (more than 16,000 programs), vexed4.alioth.net, tnfs.millhill.org and tnfs.bytedelight.com.
- They are browsed like folders of the SD card.
- When you choose a file, it is downloaded to **`SD:/descargas/<server>/`** and loaded like any other: tape, snapshot, `.DSK`…

**How it works**:
- TNFS is the Spectranet's network file system: UDP, port 16384.
- `src/tnfs.cpp` implements the reading part: MOUNT, OPENDIR/READDIR, STAT, OPEN/READ/CLOSE and UMOUNT, with retries and respecting the minimum time the server asks for.
- So that long lists do not take long, it uses **OPENDIRX/READDIRX**, which return many entries per datagram and whether they are folders. If the server does not know them, it uses the classic method and asks with STAT only about what has no file extension.
- While talking to the server, the menu shows "Connecting…", "Reading the list…" or "Downloading…".

The emulated **Spectranet** (see its section) lets the Spectrum use these servers directly too.

**Tested**: on the PC, the client against the reference server `tnfsd`:
- listing of the root and of subfolders, with READDIRX and with the classic method;
- STAT of files and folders;
- byte-for-byte identical download of a 128 KB `.DSK`;
- names with spaces, and errors.

Also used on the Pi.
