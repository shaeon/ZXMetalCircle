// SEEK repetido mientras la cabeza se mueve, como el cargador de Vindicators
// (+3, Domark): tras un RECALIBRATE, espera a ver T0 con Sense Drive Status
// (sin Sense Interrupt Status) y pide el SEEK enseguida; luego repite el SEEK
// cada vez que Sense Interrupt Status da 80 (aún no ha llegado). La cabeza
// tiene que acabar en la pista pedida: READ ID tiene que leer ese cilindro.
// Antes, el PCN llegaba al destino pero la cabeza se quedaba atrás (la 1 en
// vez de la 5), y el juego cargaba datos de otra pista.
//
//   g++ -O2 -std=c++17 -I. -I$S -I$M/compat -I$M -o seek_test plus3/seek_test.cpp $M/*.cpp $M/compat/*.cpp
#include "emu.h"
#include "upd765.h"
#include "imagedev/floppy.h"
#include "dskimage.h"
#include <stdio.h>
#include <vector>

upd765a_device *F;
int fails = 0;
void chk (const char *n, bool ok) { printf ("%s %s\n", ok ? "ok  " : "FALLO", n); if (!ok) fails++; }
void adv (int us) { mame_scheduler::advance_to (mame_scheduler::now () + attotime::from_usec (us)); }
bool waitmsr (u8 mask, u8 val, int maxus = 2000000)
{
	for (int t = 0; t < maxus; t += 5) { if ((F->msr_r () & mask) == val) return true; adv (5); }
	return false;
}
void cmd (std::vector<u8> b)
{
	for (u8 x : b) { if (!waitmsr (0xC0, 0x80)) { printf ("  timeout escribiendo comando\n"); return; } F->fifo_w (x); adv (10); }
}
std::vector<u8> results ()
{
	std::vector<u8> r;
	while (waitmsr (0xD0, 0xD0, 3000000)) { r.push_back (F->fifo_r ()); adv (10); }
	return r;
}

// RECALIBRATE y espera a T0 con Sense Drive Status, como el cargador (que
// lo vuelve a pedir si no lo ve en 255 consultas)
static void RecalibrateLikeLoader (void)
{
	for (int nTry = 0; nTry < 20; nTry++)
	{
		cmd ({0x07, 0x00});
		for (int i = 0; i < 255; i++)
		{
			cmd ({0x04, 0x00});
			auto r = results ();
			if (r.size () == 1 && (r[0] & 0x10))
			{
				return;
			}
		}
	}
	printf ("  no se llega a T0\n");
}

// SEEK repetido hasta que Sense Interrupt Status da el final; devuelve el PCN
static int SeekLikeLoader (u8 nTrack)
{
	for (int nTry = 0; nTry < 2000; nTry++)
	{
		cmd ({0x0F, 0x00, nTrack});
		cmd ({0x08});
		auto r = results ();
		if (r.size () == 2 && (r[0] & 0xC0) == 0)
		{
			return r[1];
		}
	}
	return -1;
}

static int ReadIDCylinder (void)
{
	cmd ({0x4A, 0x00});
	auto r = results ();
	return r.size () == 7 && (r[0] & 0xC0) == 0 ? r[3] : -1;
}

int main ()
{
	machine_config mc;
	upd765a_device fdc (mc, "fdc", nullptr, 4000000, true, false);
	F = &fdc;
	fdc.start ();
	floppy_image_device fl (0, 42, false);
	CDskImage img;
	chk ("disco +3 en blanco", img.CreateBlank3 ());
	fl.insert (&img);
	fdc.set_floppy (&fl);
	fl.mon_w (0);
	adv (500000);
	cmd ({0x03, 0xAF, 0x03});		// SPECIFY como el cargador
	adv (100);

	// la cabeza en la 4, como tras cargar las pistas 3 y 4
	cmd ({0x0F, 0x00, 0x04}); adv (300000); cmd ({0x08}); results ();
	chk ("de partida en la pista 4", ReadIDCylinder () == 4);

	// RECALIBRATE (sin consultar su interrupción) y SEEK a la 5 repetido
	RecalibrateLikeLoader ();
	int nPCN = SeekLikeLoader (5);
	int nCyl = ReadIDCylinder ();
	printf ("  PCN %d, READ ID cilindro %d\n", nPCN, nCyl);
	chk ("tras RECALIBRATE, SEEK repetido a 5: PCN 5", nPCN == 5);
	chk ("tras RECALIBRATE, SEEK repetido a 5: la cabeza está en la 5", nCyl == 5);

	// SEEK repetido de la 5 a la 9, sin RECALIBRATE
	nPCN = SeekLikeLoader (9);
	nCyl = ReadIDCylinder ();
	printf ("  PCN %d, READ ID cilindro %d\n", nPCN, nCyl);
	chk ("SEEK repetido de 5 a 9: la cabeza está en la 9", nPCN == 9 && nCyl == 9);

	// y hacia atrás, de la 9 a la 2
	nPCN = SeekLikeLoader (2);
	nCyl = ReadIDCylinder ();
	printf ("  PCN %d, READ ID cilindro %d\n", nPCN, nCyl);
	chk ("SEEK repetido de 9 a 2: la cabeza está en la 2", nPCN == 2 && nCyl == 2);

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails;
}
