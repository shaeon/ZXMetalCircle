#
# Makefile - ZXCircle
#
# Uso: make CIRCLESTDLIB=/ruta/a/circle-stdlib   (configurado con ./configure -r <modelo>
#      y con la newlib y las bibliotecas de Circle ya compiladas; ver build.sh)
#

CIRCLESTDLIB ?= ../circle-stdlib

# Config.mk de circle-stdlib: CIRCLEHOME, las cabeceras de newlib (-nostdinc
# -isystem ...) y la ruta de sus bibliotecas
include $(CIRCLESTDLIB)/Config.mk

OBJS	= src/main.o src/kernel.o src/spectrum.o src/ula.o src/zxmemory.o src/pagers.o src/divmmc.o src/tapedeck.o src/ay8912.o src/audiomixer.o \
	  src/tape.o src/inflate.o src/zipfile.o src/updater.o src/updatefs.o src/kernelupdate.o src/padassign.o src/padprofile.o src/kernelpad.o src/kernelpokes.o src/kernelsystem.o src/kernelwifi.o src/kernelbench.o src/wificonfig.o src/pokfile.o src/kernelfiles.o src/kerneltape.o src/filecopy.o src/kernelzx81.o src/zx81.o src/snapshot.o src/interface1.o src/zxkeyboard.o src/gpiokeyboard.o src/recreated.o src/video.o src/menu.o src/network.o src/webserver.o src/webfiles.o src/plus3disk.o src/sdcard.o src/harddisk.o src/tnfs.o src/tnfsbrowser.o src/earinput.o src/earsource.o src/exclusions.o src/optcheck.o src/spectranet.o src/snetcircle.o src/settings.o src/esxfolder.o src/zxnet.o src/if1link.o src/i18n.o src/videocore.o \
	  src/download/httpsclient.o src/download/zxinfo.o src/download/circleglue.o src/roms.o \
	  third_party/z80cpp/z80.o \
	  third_party/mame/upd765.o third_party/mame/fdc_pll.o third_party/mame/dskimage.o \
	  third_party/mame/compat/floppy.o third_party/mame/compat/scheduler.o

# Orden de enlace (el de los ejemplos de circle-stdlib): primero newlib, para
# que sus funciones (strlen, strcmp, memcmp, memchr... en ensamblador
# optimizado para ARMv7/ARMv8) ganen a las que pudiera traer Circle. Por diseño
# de circle-newlib, memcpy, memset y memmove (lib/util*.o) y malloc y free
# (lib/alloc.o) son siempre los de Circle: newlib se compila sin ellos. build.sh
# lo comprueba en el mapa del enlazador (tools/origen_simbolos.py).
LIBS	= $(CIRCLE_STDLIB_INSTALL_DIR)/lib/libm.a \
	  $(CIRCLE_STDLIB_INSTALL_DIR)/lib/libc.a \
	  $(CIRCLE_STDLIB_INSTALL_DIR)/lib/libcirclenewlib.a \
	  $(CIRCLEHOME)/addon/wlan/hostap/wpa_supplicant/libwpa_supplicant.a \
	  $(CIRCLEHOME)/addon/wlan/libwlan.a \
	  $(CIRCLEHOME)/lib/net/libnet.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
	  $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	  $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/sound/libsound.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

# Mbed TLS (HTTPS): se compila toda la biblioteca; lo no configurado queda vacío
OBJS	+= $(patsubst %.c,%.o,$(wildcard third_party/mbedtls/library/*.c))
# gcc 14 avisa en mbedtls_xor (inline desde ctr_drbg.c) de un índice fuera de
# un array de 48 bytes que no ocurre (falso positivo; la 15 ya no lo da)
third_party/mbedtls/library/ctr_drbg.o: CFLAGS += -Wno-array-bounds

include $(CIRCLEHOME)/Rules.mk

INCLUDE	:= $(CIRCLE_STDLIB_INCLUDES) $(INCLUDE)
INCLUDE	+= -I third_party/mbedtls/include -I third_party/mbedtls -I src -I third_party/z80cpp -I third_party/compat -I third_party/mame/compat -I third_party/mame -I .
DEFINE	+= -DWITH_BREAKPOINT_SUPPORT -DMBEDTLS_CONFIG_FILE='"zxmc_config.h"'
OPTIMIZE = -O3

src/roms.o: roms/spectrum.rom roms/128-0.rom roms/128-1.rom

-include $(DEPS)
