//
// golden_test.cpp - Pruebas de referencia: que la emulación no cambie
//
// Para la reestructuración en una capa de máquinas: cada modelo ejecuta
// siempre lo mismo (arrancar, unas teclas, el programa de bench=1, cargar una
// cinta, grabar en el disco del +3...) y de cada prueba se saca una huella
// (FNV-1a de 64 bits) de todo lo que se ve y se oye: la pantalla (con el
// borde), las 8 páginas de RAM, el sonido de cada cuadro, los registros del
// Z80, los t-states y los puertos de paginación. Las huellas buenas están en
// golden/huellas.txt; si un cambio da otra huella, la emulación ha cambiado.
//
//   ./golden_test            compara con golden/huellas.txt (la Action)
//   ./golden_test --grabar   escribe golden/huellas.txt (solo cuando el
//                            cambio de la emulación es a propósito)
//   ./golden_test --ppm      además, vuelca la pantalla final de cada prueba
//
// (se compila como snapshot_snet_test.cpp: el núcleo, más snapshot.cpp)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include <initializer_list>
#define private public
#define protected public
#include "spectrum.h"
#include "snapshot.h"
#undef private
#undef protected
#include "benchprog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>

#ifndef ROMDIR
#define ROMDIR "../../roms/"
#endif
#define GOLDEN_FILE	"golden/huellas.txt"

static CZXKeyboard kb;
static CTape tape;
static CSpectrum *S;
static bool s_bPPM = false;

//------------------------------------------------------------------------------
// Huella

struct CHash
{
	uint64_t h = 1469598103934665603ULL;
	void Add (const void *p, size_t n)
	{
		const uint8_t *b = (const uint8_t *) p;
		for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ULL; }
	}
	void Add16 (unsigned v) { uint8_t b[2] = {(uint8_t) v, (uint8_t) (v >> 8)}; Add (b, 2); }
	void Add32 (unsigned v) { Add16 (v & 0xFFFF); Add16 (v >> 16); }
};

static CHash s_Audio;		// el sonido de todos los cuadros de la prueba

// Un cuadro: lo emula y añade su sonido a la huella
static void run (int n)
{
	for (int i = 0; i < n; i++)
	{
		S->RunFrame ();
		CAudioMixer *pAudio = S->GetAudio ();
		s_Audio.Add (pAudio->GetBuffer (), pAudio->GetFrameCount () * 2 * sizeof (s16));
		s_Audio.Add32 (pAudio->GetFrameCount ());
		pAudio->ClearBuffer ();
	}
}

static uint64_t StateHash (void)
{
	CHash H;
	H.Add (S->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
	H.Add (S->m_Mem.m_RAM, sizeof S->m_Mem.m_RAM);
	H.Add (&s_Audio.h, sizeof s_Audio.h);
	Z80 &C = S->m_CPU;
	unsigned Regs[] = {C.getRegAF (), C.getRegBC (), C.getRegDE (), C.getRegHL (),
			   C.getRegAFx (), C.getRegBCx (), C.getRegDEx (), C.getRegHLx (),
			   C.getRegIX (), C.getRegIY (), C.getRegSP (), C.getRegPC (),
			   C.getRegI (), C.getRegR ()};
	for (unsigned r : Regs) H.Add16 (r);
	H.Add32 ((unsigned) S->m_nTstates);
	H.Add32 (S->m_ULA.m_nFrames);	// (cuadros: los cuenta la ULA, para el parpadeo)
	uint8_t Ports[] = {S->m_nPortFE, S->m_Mem.m_nPort7FFD, S->m_Mem.m_nPort1FFD};
	H.Add (Ports, sizeof Ports);
	return H.h;
}

static void dump (const char *pName)
{
	std::string File = std::string ("golden_") + pName + ".ppm";
	for (char &c : File) if (c == '+' || c == ' ' || c == '/') c = '_';
	FILE *o = fopen (File.c_str (), "wb");
	if (o == 0) return;
	fprintf (o, "P6 %d %d 255\n", SCREEN_WIDTH, SCREEN_HEIGHT);
	TPixel *p = S->GetScreenBuffer ();
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
	{
		u16 c = p[i];
		fputc (((c >> 11) & 31) * 255 / 31, o);
		fputc (((c >> 5) & 63) * 255 / 63, o);
		fputc ((c & 31) * 255 / 31, o);
	}
	fclose (o);
}

//------------------------------------------------------------------------------
// Teclado (la matriz del teclado GPIO: fila, bit), como en plus3/p3test.cpp

#define K(r, b)	(((r) << 3) | (b))
#define CS	K (0, 0)
#define SS	K (7, 1)
#define ENTER	K (6, 0)

static void press (std::initializer_list<int> Keys, int nHold = 5)
{
	u8 m[8] = {0};
	for (int k : Keys) m[k >> 3] |= 1 << (k & 7);
	kb.SetGPIOMatrix (m);
	run (nHold);
	u8 z[8] = {0};
	kb.SetGPIOMatrix (z);
	run (5);
}

static int key (char c)
{
	const char *Rows[8] = {"#zxcv", "asdfg", "qwert", "12345", "09876", "poiuy", "\nlkjh", " $mnb"};
	for (int r = 0; r < 8; r++)
		for (int b = 0; b < 5; b++)
			if (Rows[r][b] == c) return K (r, b);
	return -1;
}

static void type (const char *t)
{
	for (; *t; t++)
	{
		char c = *t;
		if (c == '"')		press ({SS, K (5, 0)});
		else if (c == ':')	press ({SS, K (0, 1)});
		else			press ({key (c)});
		if (c == '\n') run (40);
	}
}

//------------------------------------------------------------------------------
// Modelos y sus ROMs

static bool LoadParts (const char *pPrefix, unsigned nParts, u8 *pROM)
{
	for (unsigned i = 0; i < nParts; i++)
	{
		char Name[256];
		if (nParts == 1)	snprintf (Name, sizeof Name, ROMDIR "%s.rom", pPrefix);
		else			snprintf (Name, sizeof Name, ROMDIR "%s-%u.rom", pPrefix, i);
		FILE *f = fopen (Name, "rb");
		if (f == 0) { printf ("falta %s\n", Name); return false; }
		size_t n = fread (pROM + i * 0x4000, 1, 0x4000, f);
		fclose (f);
		if (n != 0x4000) { printf ("%s incompleta\n", Name); return false; }
	}
	return true;
}

struct TModelDef
{
	const char *pName;
	TModel Model;
	const char *pROM;	// 0: la incluida (48K y 128K)
	unsigned nParts;
	int nTKULA;		// -1 si no es un TK
};

static const TModelDef Models[] =
{
	{"16k",		Model16K,	0,		0, -1},
	{"48k",		Model48K,	0,		0, -1},
	{"128k",	Model128K,	0,		0, -1},
	{"+2",		ModelPlus2,	"plus2",	2, -1},
	{"+2a",		ModelPlus2A,	"plus2a",	4, -1},
	{"+3",		ModelPlus3,	"plus3",	4, -1},
	{"+3e",		ModelPlus3E,	"plus3e-es",	4, -1},
	{"tk90x-50",	ModelTK90X,	"tk90x-v2-es",	1, TKULA50Hz},
	{"tk90x-60",	ModelTK90X,	"tk90x-v2-es",	1, TKULA60Hz},
	{"tk90x-ferr",	ModelTK90X,	"tk90x-v2-es",	1, TKULAFerranti},
	{"tk95-50",	ModelTK95,	"tk95-es",	1, TKULA50Hz},
};

static bool SetUp (const TModelDef &M)
{
	static u8 ROMs[16][0x10000];
	static unsigned nNext = 0;
	if (M.pROM != 0)
	{
		u8 *pROM = ROMs[nNext++ % 16];
		if (!LoadParts (M.pROM, M.nParts, pROM)) return false;
		S->SetModelROM (M.Model, pROM);
	}
	if (M.nTKULA >= 0)
	{
		S->SetTKULA ((TTKULA) M.nTKULA);
	}
	tape.Eject ();
	S->SetModel (M.Model);
	s_Audio = CHash ();
	return true;
}

//------------------------------------------------------------------------------
// Las pruebas

static std::vector<std::pair<std::string, uint64_t>> s_Results;

static void Result (const std::string &Name)
{
	uint64_t h = StateHash ();
	s_Results.push_back ({Name, h});
	printf ("%-28s %016llx\n", Name.c_str (), (unsigned long long) h);
	if (s_bPPM) dump (Name.c_str ());
}

static bool ReadFile (const char *pName, std::vector<u8> *pData)
{
	FILE *f = fopen (pName, "rb");
	if (f == 0) { printf ("falta %s\n", pName); return false; }
	u8 Buf[65536];
	size_t n;
	while ((n = fread (Buf, 1, sizeof Buf, f)) > 0) pData->insert (pData->end (), Buf, Buf + n);
	fclose (f);
	return true;
}

static void TestModel (const TModelDef &M)
{
	std::string Base = M.pName;

	// 1. arrancar
	if (!SetUp (M)) { s_Results.push_back ({Base + "/sin-rom", 0}); return; }
	run (250);
	Result (Base + "/arranque");

	// 2. unas teclas (las mismas en todos: lo que hagan depende de la ROM)
	press ({CS, K (4, 4)});		// abajo (6 en el 48K)
	press ({ENTER});
	run (60);
	type ("10 print 42\n");
	type ("run\n");
	run (100);
	Result (Base + "/teclas");

	// 3. el programa de bench=1 (no cabe en el 16K: sin RAM en 8000h)
	if (M.Model != Model16K)
	{
		SetUp (M);
		run (100);
		for (unsigned i = 0; i < sizeof BenchProgram; i++)
		{
			S->WriteMemory (BENCH_ORG + i, BenchProgram[i]);
		}
		S->m_CPU.setRegPC (BENCH_ORG);
		S->m_CPU.setIFF1 (false);
		S->m_CPU.setIFF2 (false);
		run (150);
		Result (Base + "/bench");
	}
}

// Cargar una cinta a velocidad real con LOAD "" (48K): los pulsos, el EAR y
// el sonido de la carga
static void TestTape (void)
{
	std::vector<u8> Data;
	if (!ReadFile ("rec48.tzx", &Data)) { s_Results.push_back ({"48k/cinta-normal", 0}); return; }
	for (TTapeMode Mode : {TapeModeNormal, TapeModeFlash})
	{
		SetUp (Models[1]);
		S->SetTapeMode (Mode);
		run (150);
		tape.InsertTZX (Data.data (), Data.size (), "t");
		S->TapeInserted ();
		press ({K (6, 3)});		// J: LOAD
		press ({SS, K (5, 0)});		// "
		press ({SS, K (5, 0)});
		press ({ENTER});
		run (Mode == TapeModeNormal ? 2500 : 300);
		Result (Mode == TapeModeNormal ? "48k/cinta-normal" : "48k/cinta-flash");
	}
	S->SetTapeMode (TapeModeFlash);
}

// +3: un disco vacío y SAVE (la controladora y el disco)
static void TestDisk (void)
{
	if (!SetUp (Models[5])) return;
	run (150);
	S->GetDisk ()->InsertBlank (0);
	press ({CS, K (4, 4)});
	press ({ENTER});
	run (80);
	type ("10 print 42\n");
	type ("save \"prueba\"\n");
	run (300);
	type ("cat\n");
	run (200);
	Result ("+3/disco");

	// y lo que ha quedado grabado en el disco (.DSK)
	static u8 DSK[1 << 20];
	unsigned n = S->GetDisk ()->Save (0, DSK, sizeof DSK);
	CHash H;
	H.Add (DSK, n);
	H.Add32 (n);
	s_Results.push_back ({"+3/disco-dsk", H.h});
	printf ("%-28s %016llx (%u bytes, modificado %d)\n", "+3/disco-dsk", (unsigned long long) H.h,
		n, S->GetDisk ()->IsModified (0));
}

// Snapshot con ULAplus (la paleta de 64 colores)
static void TestULAplus (void)
{
	std::vector<u8> Data;
	if (!ReadFile ("ulaplus.z80", &Data)) { s_Results.push_back ({"48k/ulaplus", 0}); return; }
	SetUp (Models[1]);
	S->SetULAplusEnabled (TRUE);
	const char *pError = CSnapshot::Load (S, Data.data (), Data.size (), "ulaplus.z80");
	if (pError != 0) printf ("ulaplus.z80: %s\n", pError);
	s_Audio = CHash ();
	run (100);
	Result ("48k/ulaplus");
	S->SetULAplusEnabled (FALSE);
}

//------------------------------------------------------------------------------

int main (int argc, char **argv)
{
	bool bWrite = false;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp (argv[i], "--grabar") == 0) bWrite = true;
		else if (strcmp (argv[i], "--ppm") == 0) s_bPPM = true;
	}

	S = new CSpectrum (&kb, &tape);
	for (const TModelDef &M : Models)
	{
		TestModel (M);
	}
	TestTape ();
	TestDisk ();
	TestULAplus ();

	if (bWrite)
	{
		FILE *f = fopen (GOLDEN_FILE, "w");
		if (f == 0) { printf ("no se puede escribir " GOLDEN_FILE "\n"); return 1; }
		fprintf (f, "# Huellas de referencia de la emulación (golden_test.cpp). Se regeneran\n"
			    "# con ./golden_test --grabar solo cuando un cambio de la emulación es a\n"
			    "# propósito, y el commit lo dice.\n");
		for (auto &R : s_Results)
			fprintf (f, "%s %016llx\n", R.first.c_str (), (unsigned long long) R.second);
		fclose (f);
		printf ("golden_test: %zu huellas grabadas en " GOLDEN_FILE "\n", s_Results.size ());
		return 0;
	}

	std::map<std::string, uint64_t> Golden;
	FILE *f = fopen (GOLDEN_FILE, "r");
	if (f == 0) { printf ("falta " GOLDEN_FILE " (./golden_test --grabar)\n"); return 1; }
	char Line[256];
	while (fgets (Line, sizeof Line, f))
	{
		if (Line[0] == '#') continue;
		char Name[128]; unsigned long long h;
		if (sscanf (Line, "%127s %llx", Name, &h) == 2) Golden[Name] = h;
	}
	fclose (f);

	unsigned nFails = 0;
	for (auto &R : s_Results)
	{
		auto it = Golden.find (R.first);
		if (it == Golden.end ())
		{
			printf ("FALLO %s: no está en " GOLDEN_FILE "\n", R.first.c_str ());
			nFails++;
		}
		else if (it->second != R.second)
		{
			printf ("FALLO %s: %016llx, la buena es %016llx\n", R.first.c_str (),
				(unsigned long long) R.second, (unsigned long long) it->second);
			nFails++;
		}
	}
	if (Golden.size () != s_Results.size ())
	{
		printf ("FALLO: %zu huellas en " GOLDEN_FILE " y %zu pruebas\n", Golden.size (), s_Results.size ());
		nFails++;
	}
	if (nFails == 0)
	{
		printf ("golden_test: las %zu huellas coinciden\n", s_Results.size ());
	}
	return nFails == 0 ? 0 : 1;
}
