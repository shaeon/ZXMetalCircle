//
// zx81.cpp
//
// Carga de programas .P en el emulador ZX81+ de Paul Farrow (ver zx81.h)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "zx81.h"
#include "spectrum.h"
#include "zxkeyboard.h"
#include <circle/util.h>

// Posición en la matriz del Spectrum: fila * 8 + bit (0xFF: ninguna)
#define ZK(row, bit)		((u8) ((row) * 8 + (bit)))
#define ZK_NONE			0xFF
#define ZK_SHIFT		ZK (0, 0)
#define ZK_6			ZK (4, 4)	// SHIFT + 6: cursor abajo en el menú
#define ZK_NEWLINE		ZK (6, 0)
#define ZK_PAUSE		ZK (7, 2)	// M: PAUSE en modo K
#define ZK_5			ZK (3, 4)
#define ZK_0			ZK (4, 0)

struct TZX81Step
{
	u8 nKey1;
	u8 nKey2;
	u8 nHold;	// cuadros pulsadas
	u8 nWait;	// cuadros después
};

// Probado con la ROM v3.13 (zx81_test): el menú sale en unos 15 cuadros, el
// ZX81 tarda unos 140 en arrancar y PAUSE unos 20 en empezar. Las teclas se
// mantienen algo más que en ESPectrum (con 3 cuadros a veces se perdía la
// primera al escribir). Las mismas valen con las dos distribuciones.
static const TZX81Step LoadSteps[] =
{
	{ZK_NONE,     ZK_NONE, 0, 15},		// el menú de la ROM
	{ZK_SHIFT,    ZK_6,    3,  6},		// TAPE LOADER -> 1K
	{ZK_SHIFT,    ZK_6,    3,  6},		// -> 4K
	{ZK_SHIFT,    ZK_6,    3,  6},		// -> 16K
	{ZK_SHIFT,    ZK_6,    3,  6},		// -> 32K RAM
	{ZK_NEWLINE,  ZK_NONE, 3, 148},		// elegir y esperar al ZX81
	{ZK_PAUSE,    ZK_NONE, 4,  8},		// PAUSE
	{ZK_5,        ZK_NONE, 4,  8},		// 5000
	{ZK_0,        ZK_NONE, 4,  8},
	{ZK_0,        ZK_NONE, 4,  8},
	{ZK_0,        ZK_NONE, 4,  8},
	{ZK_NEWLINE,  ZK_NONE, 4, 26},		// y a esperar a que empiece
};

#define LOAD_STEPS	(sizeof LoadSteps / sizeof LoadSteps[0])

static void SetKeys (CZXKeyboard *pKeyboard, u8 nKey1, u8 nKey2)
{
	u8 Matrix[8] = {0};
	if (nKey1 != ZK_NONE)	Matrix[nKey1 >> 3] |= 1 << (nKey1 & 7);
	if (nKey2 != ZK_NONE)	Matrix[nKey2 >> 3] |= 1 << (nKey2 & 7);
	pKeyboard->SetScriptMatrix (Matrix);
}

CZX81Loader::CZX81Loader (void)
:	m_nSize (0),
	m_nStep (-1),
	m_nFrames (0),
	m_bPressed (FALSE)
{
}

void CZX81Loader::SetKeyboardLayout (u8 *pROM, boolean bZX81Layout)
{
	if (bZX81Layout)
	{
		pROM[ZX81_KBD_BYTE] |= 0x01;
	}
	else
	{
		pROM[ZX81_KBD_BYTE] &= ~0x01;
	}
}

boolean CZX81Loader::Start (const u8 *pProgram, unsigned nSize)
{
	if (nSize == 0 || nSize > ZX81_PROG_MAX)
	{
		return FALSE;
	}

	memcpy (m_Program, pProgram, nSize);
	m_nSize = nSize;
	m_bPressed = FALSE;
	m_nStep = 0;				// el primer paso solo espera
	m_nFrames = LoadSteps[0].nWait;

	return TRUE;
}

void CZX81Loader::Cancel (CZXKeyboard *pKeyboard)
{
	if (m_nStep >= 0 || m_bPressed)
	{
		static const u8 NoKeys[8] = {0};
		pKeyboard->SetScriptMatrix (NoKeys);
	}
	m_nStep = -1;
	m_bPressed = FALSE;
}

boolean CZX81Loader::Frame (CSpectrum *pSpectrum, CZXKeyboard *pKeyboard)
{
	if (m_nStep < 0)
	{
		return FALSE;
	}

	if (m_nFrames > 0 && --m_nFrames > 0)
	{
		return FALSE;
	}

	if (m_bPressed)
	{
		// se sueltan las teclas y se espera
		static const u8 NoKeys[8] = {0};
		pKeyboard->SetScriptMatrix (NoKeys);
		m_bPressed = FALSE;
		m_nFrames = LoadSteps[m_nStep].nWait;
		if (m_nFrames > 0)
		{
			return FALSE;
		}
	}

	// paso siguiente: se pulsan sus teclas (los pasos sin teclas solo esperan)
	if (++m_nStep < (int) LOAD_STEPS)
	{
		const TZX81Step *pStep = &LoadSteps[m_nStep];
		if (pStep->nHold > 0)
		{
			SetKeys (pKeyboard, pStep->nKey1, pStep->nKey2);
			m_bPressed = TRUE;
			m_nFrames = pStep->nHold;
		}
		else
		{
			m_nFrames = pStep->nWait;
		}
		return FALSE;
	}

	// En la pausa: el programa a la memoria del ZX81
	for (unsigned i = 0; i < m_nSize; i++)
	{
		pSpectrum->WriteMemory ((u16) (ZX81_PROG_ADDR + i), m_Program[i]);
	}
	m_nStep = -1;

	return TRUE;
}
