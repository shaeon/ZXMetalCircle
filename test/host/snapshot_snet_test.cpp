// Con la Spectranet activada, un snapshot tiene que seguir con la ROM del
// Spectrum, no con la de la Spectranet (que el reset deja paginada para
// arrancar su firmware). Antes, Ant Attack se quedaba esperando una tecla:
// su bucle llama a KEY-SCAN (0x028E) y caía en la ROM de la Spectranet.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o snapshot_snet_test snapshot_snet_test.cpp ../../src/snapshot.cpp $CORE roms_host.S
#include "spectrum.h"
#include "snapshot.h"
#include <stdio.h>
#include <vector>

int fails = 0;
void chk (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) fails++;
}

static std::vector<u8> ReadFile (const char *pName)
{
	std::vector<u8> v;
	FILE *f = fopen (pName, "rb");
	if (!f) return v;
	int c;
	while ((c = fgetc (f)) != EOF) v.push_back (c);
	fclose (f);
	return v;
}

int main ()
{
	CZXKeyboard kb;
	CTape tape;
	CSpectrum *s = new CSpectrum (&kb, &tape);

	std::vector<u8> Flash = ReadFile ("../../roms/spectranet.rom");
	chk ("flash de la Spectranet", Flash.size () == 131072);
	s->GetSpectranet ()->LoadFlash (Flash.data (), Flash.size ());
	s->SetSpectranetEnabled (TRUE);
	s->Reset ();
	chk ("al encender, la Spectranet está paginada", s->GetSpectranet ()->IsPaged ());

	std::vector<u8> Snap = ReadFile ("ulaplus.z80");
	const char *pError = CSnapshot::Load (s, Snap.data (), Snap.size (), "ulaplus.z80");
	chk ("carga el snapshot", pError == 0);
	chk ("después de cargarlo, la Spectranet no está paginada", !s->GetSpectranet ()->IsPaged ());

	// KEY-SCAN de la ROM del 48: 0x028E empieza por LD L,0x2F (2E 2F); se lee
	// como lo lee el Z80 (con la Spectranet paginada saldría su ROM)
	chk ("en 0x028E está la ROM del Spectrum", s->peek8 (0x028E) == 0x2E && s->peek8 (0x028F) == 0x2F);

	for (int i = 0; i < 50; i++)
	{
		s->RunFrame ();
		s->GetAudio ()->ClearBuffer ();
	}
	chk ("tras 50 cuadros sigue sin paginar", !s->GetSpectranet ()->IsPaged ());

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails ? 1 : 0;
}
