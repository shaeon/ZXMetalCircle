#
# Makefile - ZXCircle
#
# Uso: make CIRCLEHOME=/ruta/a/circle   (Circle configurado con ./configure -r <modelo>)
#

CIRCLEHOME ?= ../circle

OBJS	= src/main.o src/kernel.o src/spectrum.o src/ay8912.o src/audiomixer.o \
	  src/tape.o src/inflate.o src/snapshot.o src/interface1.o src/zxkeyboard.o src/gpiokeyboard.o src/recreated.o src/video.o src/menu.o src/network.o src/webserver.o src/plus3disk.o src/sdcard.o src/harddisk.o src/tnfs.o src/tnfsbrowser.o src/earinput.o src/earsource.o src/exclusions.o src/optcheck.o src/spectranet.o src/snetcircle.o src/settings.o src/esxfolder.o src/zxnet.o src/if1link.o src/i18n.o src/videocore.o \
	  src/download/httpsclient.o src/download/zxinfo.o src/download/circleglue.o src/roms.o \
	  third_party/z80cpp/z80.o \
	  third_party/mame/upd765.o third_party/mame/fdc_pll.o third_party/mame/dskimage.o \
	  third_party/mame/compat/floppy.o third_party/mame/compat/scheduler.o

LIBS	= $(CIRCLEHOME)/addon/wlan/hostap/wpa_supplicant/libwpa_supplicant.a \
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

include $(CIRCLEHOME)/Rules.mk

INCLUDE	+= -I third_party/mbedtls/include -I third_party/mbedtls -I src -I third_party/z80cpp -I third_party/compat -I third_party/mame/compat -I third_party/mame -I .
DEFINE	+= -DWITH_BREAKPOINT_SUPPORT -DMBEDTLS_CONFIG_FILE='"zxmc_config.h"'
OPTIMIZE = -O3

src/roms.o: roms/spectrum.rom roms/128-0.rom roms/128-1.rom

-include $(DEPS)
