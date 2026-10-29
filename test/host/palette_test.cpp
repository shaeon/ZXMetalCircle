// Paleta según el modelo (como JSpeccy) y negro puro opcional (pureblack)
//
//   (se compila como kempston_ports_test.cpp, ver allí)
#include "spectrum.h"
#include "palette.h"
#include <stdio.h>

int fails = 0;
void chk (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) fails++;
}

int main ()
{
	CZXKeyboard kb;
	CTape tape;
	CSpectrum *s = new CSpectrum (&kb, &tape);

	s->SetModel (Model48K);
	chk ("48K: negro de la paleta medida (#060800)", s->GetStdColor (0) == RGB565 (0x060800));
	chk ("48K: azul de la paleta medida (#0D13A7)", s->GetStdColor (1) == RGB565 (0x0D13A7));
	chk ("48K: blanco con BRIGHT (#FDFFF7)", s->GetStdColor (15) == RGB565 (0xFDFFF7));

	s->SetModel (Model128K);
	chk ("128K: negro puro", s->GetStdColor (0) == 0);
	chk ("128K: azul 0xC0", s->GetStdColor (1) == RGB565 (0x0000C0));
	chk ("128K: blanco con BRIGHT 0xFFFFFF", s->GetStdColor (15) == 0xFFFF);

	s->SetModel (Model16K);
	chk ("16K: paleta medida, como el 48K", s->GetStdColor (0) == RGB565 (0x060800));

	s->SetPureBlack (true);
	chk ("16K con negro puro: negro #000000", s->GetStdColor (0) == 0);
	chk ("16K con negro puro: negro con BRIGHT #000000", s->GetStdColor (8) == 0);
	chk ("16K con negro puro: el azul no cambia", s->GetStdColor (1) == RGB565 (0x0D13A7));

	s->SetModel (Model48K);
	chk ("48K con negro puro (se mantiene al cambiar de modelo)", s->GetStdColor (0) == 0);
	s->SetPureBlack (false);
	chk ("48K sin negro puro: vuelve el #060800", s->GetStdColor (0) == RGB565 (0x060800));

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails;
}
