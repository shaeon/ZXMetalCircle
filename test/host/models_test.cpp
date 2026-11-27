// El registro de modelos (machine.h): de él salen model=, F7, las opciones
// en gris de la configuración y, desde la fase 3e, las ROMs de cada modelo
// (la opción que la elige, su fichero, su tamaño y el nombre del aviso de las
// que faltan). Se compara con la tabla de ROMs que tenía el kernel, copiada
// aquí tal cual, y se mira que FindModel y ModelHas sigan dando lo mismo.
//
//   g++ -O2 -std=c++17 -I. -I$S -o models_test models_test.cpp
#include "machine.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
static void chk (const char *n, bool ok) { printf ("%s %s\n", ok ? "ok   " : "FALLO", n); if (!ok) fails++; }

static bool Same (const char *a, const char *b) { return a == b || (a != 0 && b != 0 && strcmp (a, b) == 0); }

// kernel.cpp, s_ModelROMs (hasta la fase 3d)
static const struct { const char *pOption, *pDefault; unsigned nSize; TModel Model; const char *pShort; } Old[] =
{
	{"rom48",     "48.rom",     0x4000,  Model48K,    0},
	{"rom128",    "128.rom",    0x8000,  Model128K,   0},
	{"romplus2",  "plus2.rom",  0x8000,  ModelPlus2,  "+2"},
	{"romplus2a", "plus2a.rom", 0x10000, ModelPlus2A, "+2A"},
	{"romplus3",  "plus3.rom",  0x10000, ModelPlus3,  "+3"},
	{"romplus3e", 0,            0x10000, ModelPlus3E, "+3e"},
};

int main ()
{
	chk ("seis modelos con ROM propia", ModelROMCount == sizeof Old / sizeof Old[0]);
	bool bSame = true;
	for (unsigned i = 0; i < ModelROMCount && i < sizeof Old / sizeof Old[0]; i++)
	{
		const TModelInfo &M = GetModelROMInfo (i);
		bool b =    M.Model == Old[i].Model && Same (M.pROMOption, Old[i].pOption)
			 && Same (M.pROMFile, Old[i].pDefault) && M.nROMSize == Old[i].nSize
			 && Same (M.pROMShort, Old[i].pShort);
		if (!b) printf ("  distinta: %u (%s)\n", i, M.pKey);
		bSame = bSame && b;
	}
	chk ("las ROMs del registro, como la tabla que tenía el kernel (y en su orden)", bSame);

	chk ("16K y TK sin ROM propia",    GetModelInfo (Model16K).pROMOption == 0
					&& GetModelInfo (ModelTK90X).pROMOption == 0
					&& GetModelInfo (ModelTK95).pROMOption == 0);
	chk ("FindModel: clave y alias",   FindModel ("plus2a") != 0 && FindModel ("+2A") != 0
					&& FindModel ("+2a")->Model == ModelPlus2A && FindModel ("pentagon") == 0);
	chk ("ModelHas: disco del +3 y +3e", ModelHas (ModelPlus3, MODEL_DISK) && ModelHas (ModelPlus3E, MODEL_DISK)
					&& !ModelHas (ModelPlus2A, MODEL_DISK));
	chk ("cada modelo, una vez",       [] { for (unsigned i = 0; i < ModelCount; i++) if (GetModelInfo (ModelList[i].Model).Model != ModelList[i].Model) return false; return true; } ());

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
