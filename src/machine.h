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

enum TCodeModel
{
	CodeModel48K,
	CodeModel128K,
	CodeModelPlus3
};

// El orden de los valores se mantiene por compatibilidad; el orden en que se
// recorren con F7 está en ModelCycle
enum TModel
{
	Model48K = 0,
	Model128K,
	ModelPlus2A,
	Model16K,
	ModelPlus2,		// +2 gris (Amstrad), electrónicamente un 128K
	ModelPlus3,		// +3: el +2A con disquetera (ROMs 4.0)
	ModelPlus3E,		// +3e: el +3 con las ROMs de Garry Lancaster y una ZXMMC
	ModelCount
};

static const TModel ModelCycle[ModelCount] =
{
	Model16K, Model48K, Model128K, ModelPlus2, ModelPlus2A, ModelPlus3, ModelPlus3E
};

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
};

// Geometría de la imagen generada: el borde más grande ("Huge border" de JSpeccy).
// Los demás tamaños de borde se obtienen recortando esta imagen al mostrarla.
#define BORDER_LEFT	64
#define BORDER_RIGHT	64
#define BORDER_TOP	56
#define BORDER_BOTTOM	56
#define SCREEN_WIDTH	(BORDER_LEFT + 256 + BORDER_RIGHT)	// 384
#define SCREEN_HEIGHT	(BORDER_TOP + 192 + BORDER_BOTTOM)	// 304

#define MAX_TSTATES_FRAME	71000	// mayor que cualquier nTstatesFrame + margen

#endif
