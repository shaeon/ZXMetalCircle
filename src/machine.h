//
// machine.h - Modelos de Spectrum y sus temporizaciones
//
// Constantes tomadas de JSpeccy (machine/MachineTypes.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_machine_h
#define _zx_machine_h

#include <circle/util.h>

enum TCodeModel
{
	CodeModel48K,
	CodeModel128K,
	CodeModelPlus3
};

// El orden de los valores se mantiene por compatibilidad; el orden en que se
// recorren con F7 está en ModelList (más abajo)
enum TModel
{
	Model48K = 0,
	Model128K,
	ModelPlus2A,
	Model16K,
	ModelPlus2,		// +2 gris (Amstrad), electrónicamente un 128K
	ModelPlus3,		// +3: el +2A con disquetera (ROMs 4.0)
	ModelPlus3E,		// +3e: el +3 con las ROMs de Garry Lancaster y una ZXMMC
	ModelTK90X,		// Microdigital TK90X (clon brasileño del 48K)
	ModelTK95,		// Microdigital TK95 (sucesor del TK90X)
	ModelCount
};

enum TTKULA
{
	TKULA50Hz,		// Microdigital a 50 Hz (Uruguay, Argentina)
	TKULA60Hz,		// Microdigital a 60 Hz (Brasil)
	TKULAFerranti		// Ferranti (los primeros de exportación): tiempos del 48K
};

// Registro de modelos: lo que distingue a cada uno fuera de sus tiempos (que
// están en MachineTiming). De aquí salen la lista de F7 (en este orden), el
// nombre en cmdline.txt (model=) y en la configuración, y las preguntas «¿tiene
// disquetera?», «¿es un TK?»... Un modelo nuevo se añade aquí (y sus tiempos
// en MachineTiming) en vez de en cada sitio que repasa los modelos.
enum TModelFamily
{
	ModelFamilySinclair,		// 16K, 48K, 128K
	ModelFamilyAmstrad,		// +2, +2A, +3 (y el +3e, que usa su hardware)
	ModelFamilyMicrodigital		// TK90X, TK95
};

#define MODEL_DISK	(1 << 0)	// disquetera del +3
#define MODEL_ZXMMC	(1 << 1)	// ZXMMC (+3e): su disco duro, sin la DivMMC
#define MODEL_TK	(1 << 2)	// ULA y ROMs de Microdigital (tkula=, tkrom=)
#define MODEL_48K	(1 << 3)	// 16K o 48K de Sinclair: Issue 2/3, su paleta
#define MODEL_SNOW	(1 << 4)	// ULA de Sinclair: el efecto nieve (no el +2A/+3 ni los TK)

struct TModelInfo
{
	TModel Model;
	const char *pKey;		// en cmdline.txt (model=) y en la configuración
	const char *pAlias;		// otro nombre que se acepta en model= (0 si no hay)
	TModelFamily Family;
	unsigned nFlags;		// MODEL_*

	// Su ROM en SD:/roms (fase 3e): la opción de la configuración que la
	// elige, el fichero por defecto (0: según el idioma, el +3e), el tamaño y
	// el nombre para el aviso de las que faltan (0: hay una incluida en el
	// kernel, la del 48K y la del 128K). pROMOption = 0: no tiene una propia
	// (el 16K usa la del 48K; los TK, las de tkrom= y tklang=)
	const char *pROMOption;
	const char *pROMFile;
	unsigned nROMSize;
	const char *pROMShort;
};

static constexpr TModelInfo ModelList[ModelCount] =
{
	{Model16K,    "16k",    0,     ModelFamilySinclair,     MODEL_48K | MODEL_SNOW,
	 0,           0,            0,       0},
	{Model48K,    "48k",    0,     ModelFamilySinclair,     MODEL_48K | MODEL_SNOW,
	 "rom48",     "48.rom",     0x4000,  0},
	{Model128K,   "128k",   0,     ModelFamilySinclair,     MODEL_SNOW,
	 "rom128",    "128.rom",    0x8000,  0},
	{ModelPlus2,  "plus2",  "+2",  ModelFamilyAmstrad,      MODEL_SNOW,
	 "romplus2",  "plus2.rom",  0x8000,  "+2"},
	{ModelPlus2A, "plus2a", "+2a", ModelFamilyAmstrad,      0,
	 "romplus2a", "plus2a.rom", 0x10000, "+2A"},
	{ModelPlus3,  "plus3",  "+3",  ModelFamilyAmstrad,      MODEL_DISK,
	 "romplus3",  "plus3.rom",  0x10000, "+3"},
	{ModelPlus3E, "plus3e", "+3e", ModelFamilyAmstrad,      MODEL_DISK | MODEL_ZXMMC,
	 "romplus3e", 0,            0x10000, "+3e"},
	{ModelTK90X,  "tk90x",  0,     ModelFamilyMicrodigital, MODEL_TK,
	 0,           0,            0,       0},
	{ModelTK95,   "tk95",   0,     ModelFamilyMicrodigital, MODEL_TK,
	 0,           0,            0,       0}
};

// Fila de ModelList de cada TModel (el valor de TModel no es el orden de F7),
// para no recorrer la tabla en cada pregunta
static constexpr unsigned char ModelRow[ModelCount] =
{
	1,	// Model48K
	2,	// Model128K
	4,	// ModelPlus2A
	0,	// Model16K
	3,	// ModelPlus2
	5,	// ModelPlus3
	6,	// ModelPlus3E
	7,	// ModelTK90X
	8	// ModelTK95
};

static constexpr bool ModelRowsMatch (unsigned i = 0)
{
	return i == ModelCount || (ModelList[ModelRow[i]].Model == (TModel) i && ModelRowsMatch (i + 1));
}
static_assert (ModelRowsMatch (), "ModelRow no lleva a la fila de cada modelo");

static inline const TModelInfo &GetModelInfo (TModel Model)
{
	return ModelList[ModelRow[Model]];
}

// Los modelos con ROM propia en SD:/roms, en el orden de F7 (de ahí salen la
// carga de ROMs y sus listas en la configuración)
static constexpr unsigned CountModelROMs (unsigned i = 0)
{
	return i >= ModelCount ? 0 : (ModelList[i].pROMOption != 0 ? 1 : 0) + CountModelROMs (i + 1);
}
static constexpr unsigned ModelROMCount = CountModelROMs ();

// El n-ésimo de ellos
static inline const TModelInfo &GetModelROMInfo (unsigned nROM)
{
	for (unsigned i = 0; i < ModelCount; i++)
	{
		if (ModelList[i].pROMOption != 0 && nROM-- == 0)
		{
			return ModelList[i];
		}
	}
	return ModelList[0];
}

static inline bool ModelHas (TModel Model, unsigned nFlag)
{
	return (GetModelInfo (Model).nFlags & nFlag) != 0;
}

// El modelo de un nombre de cmdline.txt o de la configuración ("plus2a" o
// "+2a", sin mirar mayúsculas); 0 si no es ninguno
static inline const TModelInfo *FindModel (const char *pName)
{
	for (unsigned i = 0; i < ModelCount; i++)
	{
		if (   strcasecmp (pName, ModelList[i].pKey) == 0
		    || (ModelList[i].pAlias != 0 && strcasecmp (pName, ModelList[i].pAlias) == 0))
		{
			return &ModelList[i];
		}
	}
	return 0;
}

struct TMachineTiming
{
	const char *pName;
	const char *pShortName;
	TCodeModel CodeModel;
	unsigned nClockFreq;
	int nTstatesFrame;
	int nTstatesLine;
	int nUpBorderWidth;
	int nScanLines;
	int nFirstScrByte;
	int nLastScrUpdate;
	int nOutOffset;
	int nOutBorderOffset;
	int nLengthINT;
	bool bHasAY;
	int nPaperCol;		// columna de la línea en la que empieza el papel (0 si no se
				// indica; 96 en la ULA de Microdigital de los TK)
};

static const TMachineTiming MachineTiming[ModelCount] =
{
	// 48K
	{"ZX Spectrum 48K", "48k", CodeModel48K, 3500000, 69888, 224, 64, 312,
	 14336, 57246, 3, 0, 32, false},
	// 128K
	{"ZX Spectrum 128K", "128k", CodeModel128K, 3546900, 70908, 228, 63, 311,
	 14362, 58040, 1, 2, 36, true},
	// +2A
	{"ZX Spectrum +2A", "+2A", CodeModelPlus3, 3546900, 70908, 228, 63, 311,
	 14364, 58044, 1, -1, 32, true},
	// 16K (mismas temporizaciones que el 48K)
	{"ZX Spectrum 16K", "16k", CodeModel48K, 3500000, 69888, 224, 64, 312,
	 14336, 57246, 3, 0, 32, false},
	// +2 gris (mismas temporizaciones que el 128K)
	{"ZX Spectrum +2", "+2", CodeModel128K, 3546900, 70908, 228, 63, 311,
	 14362, 58040, 1, 2, 36, true},
	// +3 (mismas temporizaciones que el +2A)
	{"ZX Spectrum +3", "+3", CodeModelPlus3, 3546900, 70908, 228, 63, 311,
	 14364, 58044, 1, -1, 32, true},
	// +3e (mismo hardware que el +3)
	{"ZX Spectrum +3e", "+3e", CodeModelPlus3, 3546900, 70908, 228, 63, 311,
	 14364, 58044, 1, -1, 32, true},
	// TK90X y TK95 con ULA Ferranti: los tiempos del 48K (los primeros de
	// exportación a Uruguay y Argentina). La ULA de Microdigital (50 y 60 Hz)
	// tiene otros tiempos y vendrá aparte.
	{"Microdigital TK90X", "tk90x", CodeModel48K, 3500000, 69888, 224, 64, 312,
	 14336, 57246, 3, 0, 32, false},
	{"Microdigital TK95", "tk95", CodeModel48K, 3500000, 69888, 224, 64, 312,
	 14336, 57246, 3, 0, 32, false}
};
// TK90X y TK95 con la ULA de Microdigital (la que hicieron clonando la de
// Ferranti; funciona a 50 y a 60 Hz). Tiempos de ESPectrum: 228 T-states por
// línea, el papel empieza 96 T-states después del principio de la línea y el
// reloj es de unos 3,58 MHz (3,5 MHz x 1,021591929, el ajuste de cinta de
// ESPectrum). [modelo: 0 TK90X, 1 TK95][ULA: 0 a 50 Hz, 1 a 60 Hz]
static const TMachineTiming TKMicroTiming[2][2] =
{
	{
		{"Microdigital TK90X", "tk90x", CodeModel48K, 3575572, 71136, 228, 64, 312,
		 14688, 58362, 3, 0, 32, false, 96},
		{"Microdigital TK90X", "tk90x", CodeModel48K, 3575572, 59736, 228, 38, 262,
		 8760, 52434, 3, 0, 32, false, 96}
	},
	{
		{"Microdigital TK95", "tk95", CodeModel48K, 3575572, 71136, 228, 64, 312,
		 14688, 58362, 3, 0, 32, false, 96},
		{"Microdigital TK95", "tk95", CodeModel48K, 3575572, 59736, 228, 38, 262,
		 8760, 52434, 3, 0, 32, false, 96}
	}
};


// Geometría de la imagen generada: el borde más grande ("Huge border" de JSpeccy).
// Los demás tamaños de borde se obtienen recortando esta imagen al mostrarla.
#define BORDER_LEFT	64
#define BORDER_RIGHT	64
#define BORDER_TOP	56
#define BORDER_BOTTOM	56
#define SCREEN_WIDTH	(BORDER_LEFT + 256 + BORDER_RIGHT)	// 384
#define SCREEN_HEIGHT	(BORDER_TOP + 192 + BORDER_BOTTOM)	// 304

#define MAX_TSTATES_FRAME	72000	// mayor que cualquier nTstatesFrame + margen (TK a 50 Hz: 71136)

#endif
