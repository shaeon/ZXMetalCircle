SPECTRANET FIRMWARE
===================

(English version of LEEME.txt.)

Spectranet (c) Dylan Smith, https://spectrum.alioth.net
Source code: https://github.com/spectrumero/spectranet (MIT licence, see
LICENSE.txt). Built with binutils-z80 from commit
12c167d99e1fd5bfd04f50da6cf4a062285e138b Thu Feb 12 10:07:12 2026 +0000

roms/spectranet.rom (on the SD card) is the 128 KB flash with the firmware and
the standard modules (basext, streams, messages, config, snapman) installed by
the official installer (installer.tap) inside the emulator itself, with this
configuration: DHCP on, MAC address 02:5A:58:4D:45:54, host name zxmetal.

The first time the Spectranet is turned on (spectranet=1) it is copied to
SD:/spectranet.flash, which is the one used and updated from then on
(settings changes, new modules...). To start again, delete spectranet.flash.
