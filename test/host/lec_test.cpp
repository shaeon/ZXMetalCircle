// La LEC (Lorenzo Electronic Computing): 15 páginas de 32 KB en 0x0000-
// 0x7FFF que se eligen con OUT a un puerto con A1 = 0 (bit 7 = paginada,
// número en b3 b6 b5 b4). La página 15 es la RAM alta del propio 48K
// (0x8000-0xFFFF). Cada página guarda lo suyo, la pantalla no se toca y el
// reset la quita. En un 128K normal no hace nada (solo en modo 48K, que se
// prueba en cpm_test).
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o lec_test lec_test.cpp $CORE roms_host.S
#include "testutil.h"

static u8 Page (int nPage)	{ return 0x80 | ((nPage & 7) << 4) | (nPage & 8); }

int main (void)
{
	S = new CSpectrum (&kb, &tape);
	S->SetModel (Model48K);
	chk ("se conecta", S->SetLECConnected (TRUE) && S->IsLECConnected ());

	u8 nROM0 = S->peek8 (0x0000);
	S->poke8 (0x8000, 0x11);
	S->poke8 (0xC000, 0x22);
	S->poke8 (0x4000, 0x33);

	S->outPort (0x00FD, Page (3));
	chk ("página 3 activa", S->IsLECPaged ());
	S->poke8 (0x0000, 0xA3);
	S->poke8 (0x7FFF, 0xB3);
	S->outPort (0x00FD, Page (12));
	S->poke8 (0x0000, 0xAC);
	S->outPort (0x00FD, Page (3));
	chk ("la página 3 conserva 0x0000", S->peek8 (0x0000) == 0xA3);
	chk ("la página 3 conserva 0x7FFF", S->peek8 (0x7FFF) == 0xB3);
	S->outPort (0x00FD, Page (12));
	chk ("página 12 (con b3)", S->peek8 (0x0000) == 0xAC);

	S->outPort (0x00FD, Page (15));
	chk ("página 15: 0x0000 es la RAM de 0x8000", S->peek8 (0x0000) == 0x11);
	chk ("página 15: 0x4000 es la RAM de 0xC000", S->peek8 (0x4000) == 0x22);
	S->poke8 (0x0001, 0x55);

	S->outPort (0x00FD, 0x00);
	chk ("despaginada: la ROM", !S->IsLECPaged () && S->peek8 (0x0000) == nROM0);
	chk ("lo escrito en la página 15 está en 0x8001", S->peek8 (0x8001) == 0x55);
	chk ("la pantalla, intacta", S->peek8 (0x4000) == 0x33);

	S->outPort (0x00FD, Page (3));
	S->Reset ();
	chk ("el reset la quita", !S->IsLECPaged () && S->peek8 (0x0000) == nROM0);

	S->outPort (0x00FD, Page (3));
	S->SetLECConnected (FALSE);
	chk ("desconectada: fuera y con la ROM", !S->IsLECConnected () && !S->IsLECPaged ()
	     && S->peek8 (0x0000) == nROM0);

	CSpectrum *t = new CSpectrum (&kb, &tape);
	t->SetModel (Model128K);
	t->SetLECConnected (TRUE);
	t->outPort (0x7FFD, 0x83);
	chk ("128K normal: no hace nada", !t->IsLECPaged ());

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
