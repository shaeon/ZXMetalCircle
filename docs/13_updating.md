# Updating the emulator

*ZXMetalCircle guide. Index of guides: [docs](README.md). What the emulator is: [README](../README.md).*

## Updating the emulator

From version 1.07, the emulator can update itself from the releases published on GitHub. It is on **page 10 of the settings** (Alt+F1 and then PgUp/PgDn):

| Row | What it does |
| --- | --- |
| Installed version | the one you have |
| Check for update | with ENTER, asks GitHub; if there is a new version, the row then says "ENTER: install 1.07" |
| Saved package | with ENTER, lists the packages in the `update` folder to install one again (also an older version) |
| Check at boot | `updboot=`: with `1`, the first time there is network it checks by itself, in the background (the Spectrum keeps running meanwhile), and shows the result: "You have the latest", "There is a new version" or "Could not reach GitHub". Off by default |
| Saved packages | `updkeep=`: `2` keeps in `update` the installed version's package and the previous one (to go back); `all`, all of them |
| Settings on update | `updconfig=`: `keep` keeps your configuration files; `new` uses the new ones |

When there is a new version, the help (F1) says so on its last line: "New version: 1.07 (Alt+F1, page 10)".

**How it installs:**
- Before starting, anything pending is saved (+3 disks, Microdrives...) and, if you changed the settings without saving them, it asks.
- The package (the release's own zip) is downloaded to `update/` and checked in full before anything is touched.
- Only the files that changed are copied. Each one is written under another name and then renamed, and the kernels are copied last.
- **Your configuration files are not overwritten**: `cmdline.txt`, `config.txt`, `excluir.txt`, `tnfs.txt` and the esxDOS configuration. If the package has a different one, it is left next to yours as `.nuevo` (for example `cmdline.txt.nuevo`) so you can compare them. With "use the new", the package's one is used and yours is kept as `.bak`.
- The `esxdos_i` folder is compared with the previous version's package, so esxDOS is not imported again if it has not changed.
- When it finishes, a screen shows how many files changed and which configuration files were left as `.nuevo` or `.bak`, and **the Pi restarts by itself** after 5 seconds: what is on the SD card is no longer what is running. If nothing changed, it does not restart.

**If something fails:**
- If the download fails or the package is damaged, nothing is touched.
- If it fails halfway through copying (SD card full, write error), it does not restart: it offers to try again. Until it finishes, `update/EN_CURSO` exists and you are told at boot: "Update not finished: Alt+F1, page 10". It can be installed again from "Saved package".
- As a last resort, the package in `update/` is the release's own zip: it can be unzipped onto the SD card by hand from a PC.

GitHub limits anonymous queries to 60 per hour; if the limit is reached, the emulator finds out the version from the release page.
