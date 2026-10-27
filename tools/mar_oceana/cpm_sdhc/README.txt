CP/M 3 DRIVER (OTIVAX) FOR SDSC AND SDHC CARDS
==============================================

(English version of LEEME.txt.)

DIVMMC.BIN is the "DivMMC driver v0.8 (c) otivax 2017" driver from the Mar
Oceana image, modified so that it detects the card type before each read or
write (CMD58 command, CCS bit) and works with both:

- SDSC (up to 2 GB): byte address, like the original.
- SDHC: sector number (the original read the wrong sectors).

The file is still 1664 bytes long. MD5 821f898137beefb49d607110aa13e09c.

TESTED ON ZXMETALCIRCLE (Mar Oceana image, +3e):
- With an SDSC card and with an SDHC card: CP/M boots, DIR lists correctly
  and PIP D:=C:DATE.COM writes a copy identical to the original.
- The original driver with SDHC: empty DIR and PIP writes nothing.

WARNING: on the zxp3 core for MiST (with the port changes) CP/M booted with
the original DIVMMC.BIN, but with this one it gave "ERROR DE DISCO NUMERO: 23"
(disk error number 23) when loading (+3DOS error 23: file not found). It was
not found out whether it was the installation of the file in the image or the
driver itself. In ZXMetalCircle, with images of up to 2 GB, use the ORIGINAL
DIVMMC.BIN: this one is not needed. It only makes sense with images larger
than 2 GB.

ZXMMC.BIN is the same for the ZXMMC interface (ports 0x1F/0x3F); it is
generated with the same script but untested.

INSTALLING IT (with the 3e utility; back up the image first)
  3e image.img get 1:DIVMMC.BIN          (backup copy)
  3e image.img del 1:DIVMMC.BIN
  3e image.img put DIVMMC.BIN 1:DIVMMC.BIN

parchear_sdhc.py applies the change to an original DIVMMC.BIN or ZXMMC.BIN:
  python3 parchear_sdhc.py ORIGINAL.BIN OUTPUT.BIN
divmmc_sdhc_sdsc_hecho.md explains the change in detail (in Spanish).
