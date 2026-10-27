// Prueba de la aceleración de cargadores: carga turbo.tzx (ver gen_turbo.py)
// y compara lo cargado con los datos. Uso: ./accel_test <modo cinta 0-2> [noaccel]
#include <initializer_list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spectrum.h"
CZXKeyboard kb; CTape tape;
static void key (u8 mods, std::initializer_list<u8> k) { u8 r[6] = {0}; int i = 0; for (u8 x : k) r[i++] = x; kb.UpdateFromReport (mods, r); }
static void run (CSpectrum &s, int n) { for (int i = 0; i < n; i++) { s.RunFrame (); s.GetAudio ()->ClearBuffer (); } }
static int compare (CSpectrum &s, const char *pFile, u16 nAddr)
{
	FILE *f = fopen (pFile, "rb"); static u8 b[65536]; int n = fread (b, 1, sizeof b, f); fclose (f);
	int nBad = 0;
	for (int i = 0; i < n; i++) if (s.ReadMemory ((u16) (nAddr + i)) != b[i]) nBad++;
	return nBad;
}
int main (int argc, char **argv)
{
	int nMode = atoi (argv[1]);
	CSpectrum *s = new CSpectrum (&kb, &tape);
	s->SetModel (Model48K); s->SetTapeMode ((TTapeMode) nMode);
	if (argc > 2) s->SetLoaderAcceleration (false);
	run (*s, 150);
	FILE *f = fopen ("turbo.tzx", "rb"); static u8 buf[1 << 20]; int n = fread (buf, 1, sizeof buf, f); fclose (f);
	tape.InsertTZX (buf, n, "t"); s->TapeInserted ();
	key (0, {0x0D}); run (*s, 8); key (0, {}); run (*s, 8);
	for (int i = 0; i < 2; i++) { key (0x01, {0x13}); run (*s, 8); key (0, {}); run (*s, 8); }
	key (0, {0x28}); run (*s, 8); key (0, {}); run (*s, 8);
	int nFrames = 0;
	while (nFrames < 40000 && !(tape.IsAtEnd () && !tape.IsPlaying () && s->ReadMemory (0x8FFF) != 0xAA))
	{
		run (*s, 1); nFrames++;
		if (tape.IsAtEnd () && !tape.IsPlaying ()) { run (*s, 100); nFrames += 100; break; }
	}
	int nA = compare (*s, "turbo_A.bin", 0xA000), nB = compare (*s, "turbo_B.bin", 0xC000);
	printf ("modo %d%s: %d cuadros; bloque A (estándar) acarreo %d, %d bytes mal; bloque B (turbo) acarreo %d, %d bytes mal\n",
		nMode, argc > 2 ? " sin acelerar" : "", nFrames, s->ReadMemory (0x8FFE), nA, s->ReadMemory (0x8FFF), nB);
	return (nA == 0 && nB == 0 && s->ReadMemory (0x8FFE) == 1 && s->ReadMemory (0x8FFF) == 1) ? 0 : 1;
}
