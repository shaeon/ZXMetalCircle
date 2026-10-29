#!/bin/bash
#
# build.sh - Compila ZXMetalCircle para todos los modelos de Raspberry Pi y prepara la SD
#
# Un único código para 32 bits (AArch32) y 64 bits (AArch64). Cada "objetivo"
# es una combinación de modelo y arquitectura, con su propio kernel:
#
#   Objetivo  Modelos                        Bits  Kernel
#   1         Pi 1, Zero, Zero W             32    kernel.img
#   2         Pi 2                           32    kernel7.img
#   3-32      Pi 3, Zero 2 W                 32    kernel8-32.img
#   4-32      Pi 4, 400                      32    kernel7l.img
#   3-64      Pi 3, Zero 2 W                 64    kernel8.img
#   4-64      Pi 4, 400                      64    kernel8-rpi4.img
#   5         Pi 5, 500                      64    kernel_2712.img
#
# config.txt elige el kernel de cada modelo: por defecto, 64 bits en la Pi 3,
# Zero 2 W, 4, 400 y 5; los de 32 bits de la Pi 3 y 4 quedan en la SD como
# alternativa (ver boot/config.txt).
#
# Requisitos: Arm GNU Toolchain (arm-none-eabi para 32 bits y aarch64-none-elf
# para 64 bits; probado con la 14.2), git, wget, make, python3, mtools y dosfstools
#
# Uso: ./build.sh                 (todos los objetivos; descarga Circle y el firmware si no están)
#      ./build.sh 3-64            (solo uno)
#      ./build.sh "1 3-64 5"      (varios)
#      QEMU=1 ./build.sh "1 3-64" (kernels para probar en QEMU, en qemu/: sin
#                                  sonido, que QEMU no emula; ver README)
#
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
WORK="${WORK:-$ROOT/build}"
CIRCLE_REPO="https://github.com/rsta2/circle.git"
CIRCLE_TAG="${CIRCLE_TAG:-master}"
PREFIX32="${PREFIX32:-${PREFIX:-arm-none-eabi-}}"
PREFIX64="${PREFIX64:-aarch64-none-elf-}"
JOBS="${JOBS:-$(nproc)}"
QEMU="${QEMU:-0}"

TARGETS="${1:-1 2 3-32 4-32 3-64 4-64 5}"

# Por objetivo: modelo para ./configure -r, bits, kernel y carpeta de Circle.
# Las carpetas de 32 bits se llaman como antes (circle-r1 ... circle-r4) para
# aprovechar lo ya compilado.
declare -A RASPPI=( [1]=1 [2]=2 [3-32]=3 [4-32]=4 [3-64]=3 [4-64]=4 [5]=5 )
declare -A BITS=(   [1]=32 [2]=32 [3-32]=32 [4-32]=32 [3-64]=64 [4-64]=64 [5]=64 )
declare -A KERNEL=( [1]=kernel.img [2]=kernel7.img [3-32]=kernel8-32.img [4-32]=kernel7l.img
		    [3-64]=kernel8.img [4-64]=kernel8-rpi4.img [5]=kernel_2712.img )
declare -A CDIR=(   [1]=circle-r1 [2]=circle-r2 [3-32]=circle-r3 [4-32]=circle-r4
		    [3-64]=circle-r3-64 [4-64]=circle-r4-64 [5]=circle-r5 )

for T in $TARGETS; do
	if [ -z "${RASPPI[$T]}" ]; then
		echo "Objetivo desconocido: $T (válidos: 1 2 3-32 4-32 3-64 4-64 5)"
		exit 1
	fi
	if [ "${BITS[$T]}" = 64 ]; then P="$PREFIX64"; else P="$PREFIX32"; fi
	if ! command -v "${P}gcc" > /dev/null; then
		echo "Falta el compilador ${P}gcc para el objetivo $T (Arm GNU Toolchain en el PATH)"
		exit 1
	fi
done

mkdir -p "$WORK" "$ROOT/sdcard"

if [ ! -d "$WORK/circle-src" ]; then
	git clone --depth 1 -b "$CIRCLE_TAG" "$CIRCLE_REPO" "$WORK/circle-src"
fi

# WiFi: wpa_supplicant (submódulo hostap) y firmware de los chips Broadcom
if [ ! -f "$WORK/circle-src/addon/wlan/hostap/wpa_supplicant/Makefile.circle" ]; then
	( cd "$WORK/circle-src" && git submodule update --init --depth 1 addon/wlan/hostap )
fi
if [ ! -f "$WORK/circle-src/addon/wlan/firmware/brcmfmac43430-sdio.bin" ]; then
	make -C "$WORK/circle-src/addon/wlan/firmware" firmware
fi
# firmware propio de la Pi 5 (mismo chip 43455, otros nombres de fichero)
if [ ! -f "$WORK/circle-src/addon/wlan/firmware/brcmfmac43455-sdio.raspberrypi,5-model-b.bin" ]; then
	make -C "$WORK/circle-src/addon/wlan/firmware" firmware5
fi

# Bibliotecas de Circle que usa el emulador
CIRCLE_LIBS="lib lib/usb lib/input lib/fs lib/sound lib/net lib/sched addon/SDCard addon/fatfs addon/wlan"

build_circle_libs ()
{
	local C="$1" ACTION="$2"
	for D in $CIRCLE_LIBS; do
		[ "$ACTION" = clean ] && make -C "$C/$D" clean > /dev/null
		make -C "$C/$D" -j"$JOBS" > /dev/null
	done
	[ "$ACTION" = clean ] && make -C "$C/addon/wlan/hostap/wpa_supplicant" -f Makefile.circle clean > /dev/null
	make -C "$C/addon/wlan/hostap/wpa_supplicant" -f Makefile.circle -j"$JOBS" > /dev/null
}

for T in $TARGETS; do
	R="${RASPPI[$T]}"
	if [ "${BITS[$T]}" = 64 ]; then P="$PREFIX64"; else P="$PREFIX32"; fi
	echo "=== Objetivo $T: Raspberry Pi $R, ${BITS[$T]} bits ==="
	C="$WORK/${CDIR[$T]}"
	if [ ! -f "$C/lib/libcircle.a" ]; then
		rm -rf "$C"
		cp -a "$WORK/circle-src" "$C"
		( cd "$C" && ./configure -r "$R" -p "$P" -f > /dev/null )
		build_circle_libs "$C"
	fi

	# Pi 2, 3 y 4: soporte multinúcleo de Circle (la imagen se amplía en el
	# núcleo 1, ver src/videocore.h). Cambia las cerraduras internas de Circle,
	# así que hay que recompilarlo entero. La Pi 5 no lo necesita (va sobrada)
	# y de momento se compila sin él.
	if [ "$R" -ge 2 ] && [ "$R" -le 4 ] && ! grep -q "ARM_ALLOW_MULTI_CORE" "$C/Config.mk"; then
		echo "DEFINE += -DARM_ALLOW_MULTI_CORE" >> "$C/Config.mk"
		build_circle_libs "$C" clean
	fi

	# FatFs con f_mkfs y f_expand (esxDOS sobre una carpeta crea su imagen FAT).
	# No cambian el tamaño de ninguna estructura: basta con recompilar FatFs.
	if ! grep -q "define FF_USE_MKFS[[:space:]]*1" "$C/addon/fatfs/ffconf.h"; then
		sed -i 's/#define FF_USE_MKFS\t\t0/#define FF_USE_MKFS\t\t1/' "$C/addon/fatfs/ffconf.h"
		sed -i 's/#define FF_USE_EXPAND\t0/#define FF_USE_EXPAND\t1/' "$C/addon/fatfs/ffconf.h"
		make -C "$C/addon/fatfs" clean > /dev/null
		make -C "$C/addon/fatfs" -j"$JOBS" > /dev/null
	fi

	# FatFs con "fast seek" (lo usan las imágenes de disco duro del +3e)
	if ! grep -q "define FF_USE_FASTSEEK.1" "$C/addon/fatfs/ffconf.h"; then
		sed -i 's/#define FF_USE_FASTSEEK\t0/#define FF_USE_FASTSEEK\t1/' "$C/addon/fatfs/ffconf.h"
		make -C "$C/addon/fatfs" clean > /dev/null
		make -C "$C/addon/fatfs" -j"$JOBS" > /dev/null
		# El fast seek cambia el tamaño de la estructura FIL de FatFs: todo lo que
		# la usa debe recompilarse con la misma cabecera. La WiFi la usa para leer
		# el firmware (p9chan) y wpa_supplicant.conf; sin esto, el chip WiFi no arranca.
		make -C "$C/addon/wlan" clean > /dev/null
		make -C "$C/addon/wlan" -j"$JOBS" > /dev/null
		make -C "$C/addon/wlan/hostap/wpa_supplicant" -f Makefile.circle clean > /dev/null
		make -C "$C/addon/wlan/hostap/wpa_supplicant" -f Makefile.circle -j"$JOBS" > /dev/null
	fi

	B="$WORK/zxcircle-$T"
	rm -rf "$B"
	mkdir -p "$B"
	cp -a "$ROOT/Makefile" "$ROOT/src" "$ROOT/roms" "$ROOT/third_party" "$B/"
	find "$B" \( -name '*.o' -o -name '*.d' \) -delete	# nunca reutilizar objetos de otro objetivo
	if [ "$QEMU" = 1 ]; then
		# QEMU no emula el sonido HDMI ni PWM (al fallar, Circle toca registros
		# HDMI que no existen y se cuelga) ni responde al DHCP
		echo "DEFINE += -DZX_QEMU" >> "$B/Makefile"
	fi
	make -C "$B" CIRCLEHOME="$C" -j"$JOBS"

	# Circle reserva 2 MB para el kernel (código + datos + BSS): si se pasa,
	# pisa la pila y el heap y falla al arrancar sin decir nada
	ELF="$B/${KERNEL[$T]%.img}.elf"
	TOTAL=$(${P}size "$ELF" | awk 'NR==2 {print $4}')
	if [ "$TOTAL" -ge $((2 * 1024 * 1024)) ]; then
		echo "ERROR: el kernel ocupa $TOTAL bytes (máximo 2 MB). Mueve búferes grandes al heap."
		exit 1
	fi
	echo "Tamaño del kernel en memoria: $TOTAL bytes ($((TOTAL * 100 / (2 * 1024 * 1024))) % de 2 MB)"

	if [ "$QEMU" = 1 ]; then
		mkdir -p "$ROOT/qemu"
		cp "$B/${KERNEL[$T]}" "$ROOT/qemu/"
	else
		cp "$B/${KERNEL[$T]}" "$ROOT/sdcard/"
	fi
done

if [ "$QEMU" = 1 ]; then
	echo
	echo "Kernels de prueba en $ROOT/qemu (no van a la SD). Ejemplos:"
	echo "  qemu-system-aarch64 -M raspi3b -kernel qemu/kernel8.img -drive file=sd.img,if=sd,format=raw -serial stdio -device usb-kbd -append \"logdev=ttyS1\""
	echo "  qemu-system-arm -M raspi0 -bios qemu/kernel.img -drive file=sd.img,if=sd,format=raw -serial stdio -device usb-kbd"
	exit 0
fi

# Firmware y armstub (solo hace falta una vez). El firmware incluye ya los
# ficheros de la Pi 5 y 500 (bcm2712*.dtb y overlays/bcm2712d0.dtbo).
if [ ! -f "$ROOT/sdcard/start.elf" ] || [ ! -f "$ROOT/sdcard/overlays/bcm2712d0.dtbo" ]; then
	make -C "$WORK/circle-src/boot" firmware
	cp "$WORK/circle-src/boot/"{bootcode.bin,start.elf,fixup.dat,start4.elf,fixup4.dat} "$ROOT/sdcard/"
	cp "$WORK/circle-src/boot/"*.dtb "$ROOT/sdcard/"
	mkdir -p "$ROOT/sdcard/overlays"
	cp "$WORK/circle-src/boot/bcm2712d0.dtbo" "$ROOT/sdcard/overlays/"
	cp "$WORK/circle-src/boot/LICENCE.broadcom" "$ROOT/sdcard/"
fi
# Todos los DTB del firmware, de la misma versión que baja Circle (Circle solo
# trae los de los modelos que los necesitan). Con el suyo en la raíz, cada
# modelo arranca con su descripción de hardware oficial y el firmware no avisa
# de que le falta (sugerencia de José Luis Sánchez, probado en todas sus Pi,
# incluida una Pi 1 B+).
FW_COMMIT=$(sed -n 's/^FIRMWARE ?= *//p' "$WORK/circle-src/boot/Makefile")
for f in bcm2708-rpi-b.dtb bcm2708-rpi-b-plus.dtb bcm2708-rpi-b-rev1.dtb bcm2708-rpi-cm.dtb \
	 bcm2708-rpi-zero.dtb bcm2708-rpi-zero-w.dtb bcm2709-rpi-2-b.dtb bcm2709-rpi-cm2.dtb \
	 bcm2710-rpi-2-b.dtb bcm2710-rpi-3-b.dtb bcm2710-rpi-3-b-plus.dtb bcm2710-rpi-cm3.dtb \
	 bcm2710-rpi-cm0.dtb bcm2710-rpi-zero-2.dtb bcm2710-rpi-zero-2-w.dtb \
	 bcm2711-rpi-4-b.dtb bcm2711-rpi-400.dtb bcm2711-rpi-cm4.dtb bcm2711-rpi-cm4-io.dtb \
	 bcm2711-rpi-cm4s.dtb bcm2712-rpi-5-b.dtb bcm2712d0-rpi-5-b.dtb bcm2712-d-rpi-5-b.dtb \
	 bcm2712-rpi-500.dtb bcm2712-rpi-cm5-cm5io.dtb bcm2712-rpi-cm5l-cm5io.dtb \
	 bcm2712-rpi-cm5-cm4io.dtb bcm2712-rpi-cm5l-cm4io.dtb; do
	if [ ! -f "$ROOT/sdcard/$f" ]; then
		wget -q -O "$ROOT/sdcard/$f.tmp" "https://github.com/raspberrypi/firmware/raw/$FW_COMMIT/boot/$f" \
			&& mv "$ROOT/sdcard/$f.tmp" "$ROOT/sdcard/$f" \
			|| { rm -f "$ROOT/sdcard/$f.tmp"; echo "ERROR: no se pudo descargar $f" >&2; exit 1; }
	fi
done

# armstub de la Pi 4: uno para 32 bits y otro para 64 (la Pi 5 no lo necesita)
if [ ! -f "$ROOT/sdcard/armstub7-rpi4.bin" ] && command -v "${PREFIX32}gcc" > /dev/null; then
	make -C "$WORK/circle-src/boot/armstub" PREFIX="$PREFIX32" armstub7-rpi4.bin > /dev/null
	cp "$WORK/circle-src/boot/armstub/armstub7-rpi4.bin" "$ROOT/sdcard/"
fi
if [ ! -f "$ROOT/sdcard/armstub8-rpi4.bin" ] && command -v "${PREFIX64}gcc" > /dev/null; then
	make -C "$WORK/circle-src/boot/armstub" PREFIX64="$PREFIX64" armstub8-rpi4.bin > /dev/null
	cp "$WORK/circle-src/boot/armstub/armstub8-rpi4.bin" "$ROOT/sdcard/"
fi

cp "$ROOT/boot/config.txt" "$ROOT/boot/cmdline.txt" "$ROOT/sdcard/"
mkdir -p "$ROOT/sdcard/roms"
cp "$ROOT/boot/roms/LEEME.txt" "$ROOT/boot/roms/README.txt" "$ROOT/sdcard/roms/"
mkdir -p "$ROOT/sdcard/firmware"
cp "$WORK/circle-src/addon/wlan/firmware/"*.bin "$WORK/circle-src/addon/wlan/firmware/"*.txt \
   "$WORK/circle-src/addon/wlan/firmware/"*.clm_blob "$ROOT/sdcard/firmware/"

# Licencias del firmware WiFi (Broadcom y Cypress: redistribución en binario,
# sin modificar). Se sacan del debian/copyright del mismo commit de
# firmware-nonfree que usa Circle; si no se puede descargar, queda la referencia.
FWCOMMIT=$(sed -n 's/^FIRMWARE ?= *//p' "$WORK/circle-src/addon/wlan/firmware/Makefile")
FWURL="https://github.com/RPi-Distro/firmware-nonfree/tree/$FWCOMMIT"
if [ ! -f "$ROOT/sdcard/firmware/LICENCE.txt" ]; then
	{
		echo "WiFi firmware for the Broadcom/Cypress chips of the Raspberry Pi."
		echo "Source: $FWURL/debian/config/brcm80211"
		echo "Copyright Broadcom Corporation and Cypress Semiconductor Corporation."
		echo "Redistributed unmodified, in binary form, under the licences below"
		echo "(the .txt files: Linaro / Raspberry Pi, GPL-2)."
		echo
		wget -q -O - "https://github.com/RPi-Distro/firmware-nonfree/raw/$FWCOMMIT/debian/copyright" \
			| awk 'BEGIN { RS = ""; ORS = "\n\n" } /^License: binary-redist-(Broadcom-wifi|Cypress)/' \
			|| echo "(Licence text: see debian/copyright at the source above.)"
	} > "$ROOT/sdcard/firmware/LICENCE.txt"
fi
# Licencia y documentación del emulador (en la raíz de la SD; el selector no las muestra)
cp "$ROOT/LICENSE" "$ROOT/README.md" "$ROOT/README_es.md" "$ROOT/CHANGELOG.md" "$ROOT/sdcard/"
cp "$ROOT/boot/wpa_supplicant.conf.ejemplo" "$ROOT/boot/cmdline.hlp" "$ROOT/boot/cmdline-en.hlp" "$ROOT/boot/tnfs.txt" "$ROOT/boot/excluir.txt" "$ROOT/sdcard/"
# ROMs que se cargan de la SD (solo las del 48K y el 128K van en el kernel)
mkdir -p "$ROOT/sdcard/roms"
cat "$ROOT/roms/plus2-0.rom"  "$ROOT/roms/plus2-1.rom"  > "$ROOT/sdcard/roms/plus2.rom"
cat "$ROOT/roms/plus2a-"[0-3].rom > "$ROOT/sdcard/roms/plus2a.rom"
cat "$ROOT/roms/plus3-"[0-3].rom  > "$ROOT/sdcard/roms/plus3.rom"
cat "$ROOT/roms/plus3e-es-"[0-3].rom > "$ROOT/sdcard/roms/plus3e-es.rom"
cat "$ROOT/roms/plus3e-en-"[0-3].rom > "$ROOT/sdcard/roms/plus3e-en.rom"
cp "$ROOT/roms/if1-2.rom" "$ROOT/sdcard/roms/if1.rom"
# ROMs del Multiface (© Romantic Robot): no se publican sin su permiso. Si
# están en roms/ (las de cada uno), van a la SD; si no, el Multiface no está.
for MF in mf1.rom mf128.rom mfplus3.rom; do
	[ -f "$ROOT/roms/$MF" ] && cp "$ROOT/roms/$MF" "$ROOT/sdcard/roms/"
done
cp "$ROOT/third_party/esxdos/ESXMMC.BIN" "$ROOT/sdcard/roms/esxmmc.bin"
cp "$ROOT/roms/spectranet.rom" "$ROOT/sdcard/roms/spectranet.rom"
mkdir -p "$ROOT/sdcard/utils/spectranet"
cp "$ROOT/third_party/spectranet/installer.tap" "$ROOT/third_party/spectranet/LEEME.txt" "$ROOT/third_party/spectranet/README.txt" "$ROOT/third_party/spectranet/LICENSE.txt" "$ROOT/sdcard/utils/spectranet/"

# Tarjeta de esxDOS: imagen FAT16 de 64 MB con una partición, SYS, BIN y TMP
make_esxdos_image ()
{
	local IMG="$1"
	rm -f "$IMG"
	truncate -s 64M "$IMG"
	python3 - "$IMG" << 'PYEOF'
import struct, sys
f = open(sys.argv[1], 'r+b')
total = 64 * 1024 * 1024 // 512
start = 2048
mbr = bytearray(512)
mbr[446:462] = struct.pack('<B3sB3sII', 0, b'\0\0\0', 0x0E, b'\0\0\0', start, total - start)
mbr[510] = 0x55
mbr[511] = 0xAA
f.write(mbr)
PYEOF
	mkfs.fat -F 16 -n ESXDOS --offset 2048 -h 2048 "$IMG" $(( (64 * 1024 * 1024 / 512 - 2048) / 2 )) > /dev/null
	mcopy -s -i "$IMG@@1M" "$ROOT/third_party/esxdos/SYS" "$ROOT/third_party/esxdos/BIN" ::/
	mmd -i "$IMG@@1M" ::/TMP ::/JUEGOS
}
# Idiomas: pone al día locales/ (solo avisa de lo que falta) y la copia a la SD
python3 "$ROOT/tools/i18n/actualizar.py" || echo "AVISO: no se pudieron revisar las traducciones"
rm -rf "$ROOT/sdcard/locales"
cp -r "$ROOT/locales" "$ROOT/sdcard/locales"

# Carpeta de importación de esxDOS (divmmc=esxdos): lo que hay en ella se copia
# a la imagen hdd/esxdos.img (que crea el emulador) y se borra de la carpeta.
# Va con el sistema de esxDOS, que entra en la imagen la primera vez. La imagen
# no va en la SD: al actualizar copiando encima, machacaría la del usuario.
rm -rf "$ROOT/sdcard/esxdos" "$ROOT/sdcard/esxdos_i"
mkdir -p "$ROOT/sdcard/esxdos_i/TMP" "$ROOT/sdcard/esxdos_i/JUEGOS"
cp -r "$ROOT/third_party/esxdos/SYS" "$ROOT/third_party/esxdos/BIN" "$ROOT/sdcard/esxdos_i/"
rm -f "$ROOT/sdcard/hdd/esxdos.img"

# Menú y controlador del CP/M para la imagen Mar Océana
mkdir -p "$ROOT/sdcard/utils/mar_oceana/cpm_sdhc"
cp "$ROOT/tools/mar_oceana/menu_zxmetal.tap" "$ROOT/tools/mar_oceana/LEEME.txt" "$ROOT/tools/mar_oceana/README.txt" "$ROOT/sdcard/utils/mar_oceana/"
cp "$ROOT/tools/mar_oceana/cpm_sdhc/"* "$ROOT/sdcard/utils/mar_oceana/cpm_sdhc/"
mkdir -p "$ROOT/sdcard/certs"
cp "$ROOT/boot/certs/cacert.pem" "$ROOT/sdcard/certs/"
mkdir -p "$ROOT/sdcard/hdd"
printf 'Copia aqui las imagenes de tarjeta SD: discos duros del +3e (.vhd, .img, .raw, .hdf)\ny tarjetas FAT para esxDOS. esxdos.img es la tarjeta de la divMMC: la crea el\nemulador, y lo que dejes en la carpeta esxdos_i se copia a ella.\nSe montan desde el selector (F1), la web o con hdd= en cmdline.txt.\n' > "$ROOT/sdcard/hdd/LEEME.txt"
printf 'Copy SD card images here: +3e hard disks (.vhd, .img, .raw, .hdf)\nand FAT cards for esxDOS. esxdos.img is the divMMC card: the emulator creates it,\nand whatever you leave in the esxdos_i folder is copied into it.\nThey are mounted from the file browser (F1), the web page or with hdd= in cmdline.txt.\n' > "$ROOT/sdcard/hdd/README.txt"
mkdir -p "$ROOT/sdcard/utils"
cp "$ROOT/boot/utils/"* "$ROOT/sdcard/utils/"
mkdir -p "$ROOT/sdcard/cpm"
cp "$ROOT/boot/cpm/"* "$ROOT/sdcard/cpm/"

echo
echo "Listo: copia el contenido de $ROOT/sdcard a una tarjeta FAT32."
