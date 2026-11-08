// Reglas de las operaciones con ficheros de la web (src/webfiles.cpp)
#include "webfiles.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

int main (void)
{
	// nombres
	chk ("nombre normal", WebValidName ("Manic Miner.tap"));
	chk ("con acentos y paréntesis", WebValidName ("Cañón (1985).tzx"));
	chk ("vacío no", !WebValidName (""));
	chk ("con / no", !WebValidName ("a/b.tap"));
	chk ("con \\ no", !WebValidName ("a\\b.tap"));
	chk ("con : no", !WebValidName ("C:juego.tap"));
	chk (".. no", !WebValidName (".."));
	chk ("acabado en punto no", !WebValidName ("juego."));
	chk ("acabado en espacio no", !WebValidName ("juego "));
	chk ("con * ? < > | \" no", !WebValidName ("a*b") && !WebValidName ("a?b") && !WebValidName ("a<b")
	     && !WebValidName ("a>b") && !WebValidName ("a|b") && !WebValidName ("a\"b"));

	// del sistema: ficheros del arranque
	chk ("kernel8.img", WebIsSystem ("kernel8.img"));
	chk ("SD:/kernel_2712.img", WebIsSystem ("SD:/kernel_2712.img"));
	chk ("config.txt y cmdline.txt", WebIsSystem ("config.txt") && WebIsSystem ("/cmdline.txt"));
	chk ("un .dtb, start4.elf, fixup.dat, bootcode.bin", WebIsSystem ("bcm2711-rpi-4-b.dtb")
	     && WebIsSystem ("start4.elf") && WebIsSystem ("fixup.dat") && WebIsSystem ("bootcode.bin"));
	chk ("cmdline.hlp, LICENCE.broadcom, wpa_supplicant.conf", WebIsSystem ("cmdline.hlp")
	     && WebIsSystem ("LICENCE.broadcom") && WebIsSystem ("wpa_supplicant.conf"));
	chk ("un .img de disco duro en la raíz es libre", !WebIsSystem ("juegos.img"));
	chk ("un juego en la raíz es libre", !WebIsSystem ("manic.tap"));
	chk ("un kernel8.img dentro de una carpeta es libre", !WebIsSystem ("copias/kernel8.img"));

	// carpetas
	chk ("la raíz", WebIsSystem (""));
	chk ("roms (la carpeta)", WebIsSystem ("roms") && WebIsSystem ("SD:/roms/"));
	chk ("una ROM propia dentro de roms es libre", !WebIsSystem ("roms/ZX128_v2EN.rom"));
	chk ("saves, snaps, descargas, joysticks, tmp", WebIsSystem ("saves") && WebIsSystem ("snaps")
	     && WebIsSystem ("descargas") && WebIsSystem ("joysticks") && WebIsSystem ("tmp"));
	chk ("una partida dentro de saves es libre", !WebIsSystem ("saves/juego.z80"));
	chk ("locales y lo que tiene dentro", WebIsSystem ("locales") && WebIsSystem ("locales/pt.json"));
	chk ("update, certs, overlays, firmware: dentro también", WebIsSystem ("update/x.zip")
	     && WebIsSystem ("certs/cacert.pem") && WebIsSystem ("overlays/a/b.dtbo") && WebIsSystem ("firmware/x"));
	chk ("carpeta del usuario libre", !WebIsSystem ("juegos") && !WebIsSystem ("juegos/arcade"));
	chk ("no se distinguen mayúsculas", WebIsSystem ("ROMS") && WebIsSystem ("Locales/en.json")
	     && WebIsSystem ("KERNEL8.IMG"));

	// carpetas donde no se puede poner nada
	chk ("dentro de locales, update... no", WebIsSystemFolder ("locales") && WebIsSystemFolder ("update")
	     && WebIsSystemFolder ("certs/sub"));
	chk ("en roms, saves o una carpeta propia sí", !WebIsSystemFolder ("roms") && !WebIsSystemFolder ("saves")
	     && !WebIsSystemFolder ("juegos") && !WebIsSystemFolder (""));
	chk ("una carpeta que empieza igual no es la misma (localesX)", !WebIsSystemFolder ("localesX"));

	// rutas dentro de otras
	chk ("juegos/a.tap dentro de juegos", WebPathIsUnder ("juegos/a.tap", "juegos"));
	chk ("juegos dentro de juegos", WebPathIsUnder ("SD:/juegos", "juegos/"));
	chk ("juegos2/a.tap no está dentro de juegos", !WebPathIsUnder ("juegos2/a.tap", "juegos"));
	chk ("todo está dentro de la raíz", WebPathIsUnder ("x/y", ""));
	chk ("sin distinguir mayúsculas", WebPathIsUnder ("Juegos/A.TAP", "JUEGOS"));

	char Out[64];
	WebNormalizePath ("SD:/juegos/arcade/", Out, sizeof Out);
	chk ("normalizar: SD:/juegos/arcade/ -> juegos/arcade", strcmp (Out, "juegos/arcade") == 0);
	WebNormalizePath ("hdd\\esxdos.img", Out, sizeof Out);
	chk ("normalizar: hdd\\esxdos.img -> hdd/esxdos.img", strcmp (Out, "hdd/esxdos.img") == 0);

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
