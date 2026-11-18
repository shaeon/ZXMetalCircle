// El ZX81 con el emulador ZX81+ de Paul Farrow (roms/zx81.rom) como cartucho
// del Interface 2 en el 128K: arranca, se escribe un programa, se guarda
// como .P (de 16393 a E_LINE) y se vuelve a cargar con CZX81Loader en un
// Spectrum nuevo; al hacer RUN tiene que salir lo que imprime.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o zx81_test zx81_test.cpp ../../src/zx81.cpp $CORE roms_host.S
#include <initializer_list>
#include "spectrum.h"
#include "zx81.h"
#include <stdio.h>
#include <string.h>
#include <vector>

int fails = 0;
void chk (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) fails++;
}

static CZXKeyboard kb;
static CTape tape;

static void run (CSpectrum *s, int n, CZX81Loader *pLoader = 0)
{
	for (int i = 0; i < n; i++)
	{
		s->RunFrame ();
		s->GetAudio ()->ClearBuffer ();
		if (pLoader != 0)
		{
			pLoader->Frame (s, &kb);
		}
	}
}

#define K(r,b) (((r) << 3) | (b))
static void press (CSpectrum *s, std::initializer_list<int> keys)
{
	u8 m[8] = {0};
	for (int k : keys) m[k >> 3] |= 1 << (k & 7);
	kb.SetGPIOMatrix (m);
	run (s, 6);
	u8 z[8] = {0};
	kb.SetGPIOMatrix (z);
	run (s, 12);
}

static u16 peek16 (CSpectrum *s, u16 a)	{ return s->ReadMemory (a) | s->ReadMemory (a + 1) << 8; }

static CSpectrum *NewZX81 (const std::vector<u8> &ROM, bool bZX81Layout = true)
{
	CSpectrum *s = new CSpectrum (&kb, &tape);
	s->SetModel (Model128K);
	std::vector<u8> r (ROM);
	CZX81Loader::SetKeyboardLayout (r.data (), bZX81Layout);	// el teclado del ZX81 o del 128
	s->InsertCartridge (r.data (), r.size ());
	return s;
}

// ¿Está "HOLA" (en los códigos del ZX81) en la pantalla del ZX81?
static bool ScreenHasHola (CSpectrum *s)
{
	static const u8 Hola[] = {0x2D, 0x34, 0x31, 0x26};
	u16 nDFile = peek16 (s, 16396);
	for (u16 a = nDFile; a < nDFile + 800; a++)
	{
		bool bMatch = true;
		for (unsigned i = 0; i < sizeof Hola; i++)
		{
			if (s->ReadMemory (a + i) != Hola[i]) bMatch = false;
		}
		if (bMatch) return true;
	}
	return false;
}

int main ()
{
	std::vector<u8> ROM;
	FILE *f = fopen ("../../roms/zx81.rom", "rb");
	int c;
	while (f && (c = fgetc (f)) != EOF) ROM.push_back (c);
	if (f) fclose (f);
	chk ("roms/zx81.rom de 16 KB", ROM.size () == ZX81_ROM_SIZE);
	if (ROM.size () != ZX81_ROM_SIZE) return 1;

	// 1. Arrancar, elegir 32K RAM y escribir 10 PRINT "HOLA"
	CSpectrum *s = NewZX81 (ROM);
	run (s, 15);
	for (int i = 0; i < 4; i++) press (s, {K(0,0), K(4,4)});	// SHIFT+6: abajo
	press (s, {K(6,0)});						// NEWLINE
	run (s, 142);
	press (s, {K(3,0)});						// 1
	press (s, {K(4,0)});						// 0
	press (s, {K(5,0)});						// P: PRINT
	press (s, {K(0,0), K(5,0)});					// SHIFT+P: "
	press (s, {K(6,4)});						// H
	press (s, {K(5,1)});						// O
	press (s, {K(6,1)});						// L
	press (s, {K(1,0)});						// A
	press (s, {K(0,0), K(5,0)});					// "
	press (s, {K(6,0)});						// NEWLINE
	run (s, 20);
	chk ("la línea 10 está en el programa", s->ReadMemory (16509) == 0 && s->ReadMemory (16510) == 10);

	// el .P: de 16393 (VERSN) a E_LINE
	u16 nELine = peek16 (s, 16404);
	chk ("E_LINE razonable", nELine > 16509 && nELine < 32768);
	std::vector<u8> P;
	for (u16 a = 16393; a < nELine; a++) P.push_back (s->ReadMemory (a));
	delete s;

	// 2. Cargarlo con CZX81Loader en un Spectrum nuevo
	s = NewZX81 (ROM);
	CZX81Loader Loader;
	chk ("Start acepta el .P", Loader.Start (P.data (), P.size ()));
	chk ("un .P vacío no vale", !CZX81Loader ().Start (P.data (), 0));
	int nFrames = 0;
	while (Loader.IsActive () && nFrames < 1000)
	{
		run (s, 1, &Loader);
		nFrames++;
	}
	printf ("      carga en %d cuadros\n", nFrames);
	chk ("la carga termina", !Loader.IsActive ());
	run (s, 50);
	chk ("después de cargar, la línea 10 está", s->ReadMemory (16509) == 0 && s->ReadMemory (16510) == 10);
	chk ("y el mismo E_LINE", peek16 (s, 16404) == nELine);

	press (s, {K(2,3)});						// R: RUN
	press (s, {K(6,0)});						// NEWLINE
	run (s, 50);
	chk ("RUN imprime HOLA", ScreenHasHola (s));
	delete s;

	// 3. La carga vale también con el teclado del 128 (mismas teclas)
	s = NewZX81 (ROM, false);
	chk ("teclado del 128: bit 0 de 2042h a 0", s->ReadMemory (ZX81_KBD_BYTE) == (ROM[ZX81_KBD_BYTE] & ~1));
	CZX81Loader Loader128;
	Loader128.Start (P.data (), P.size ());
	nFrames = 0;
	while (Loader128.IsActive () && nFrames < 1000)
	{
		run (s, 1, &Loader128);
		nFrames++;
	}
	run (s, 50);
	press (s, {K(2,3)});						// RUN
	press (s, {K(6,0)});
	run (s, 50);
	chk ("teclado del 128: carga y RUN imprime HOLA", ScreenHasHola (s));

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails ? 1 : 0;
}
