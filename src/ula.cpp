//
// ula.cpp - La ULA del Spectrum: imagen, borde, colores, contención y bus flotante
//
// Lógica portada a C++ desde JSpeccy (machine/Spectrum.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "ula.h"
#include <circle/util.h>

CULA::CULA (void)
:	m_pTiming (&MachineTiming[Model48K]),
	m_pScreenPage (0),
	m_bPureBlack (FALSE),
	m_nSteps (0),
	m_nStep (0),
	m_nFirstBorderUpdate (0),
	m_nLastBorderUpdate (0),
	m_nLastChgBorder (0),
	m_bFlash (FALSE),
	m_nFrames (0),
	m_nLastByte (0),
	m_nLastAttr (0),
	m_bULAplusEnabled (TRUE),
	m_bULAplusActive (FALSE),
	m_bULAplusUsed (FALSE),
	m_nULAplusRegister (0)
{
	UpdatePalette ();
	for (unsigned i = 0; i < 64; i++)
	{
		SetULAplusColor (i, 0);
	}

	for (unsigned nAttr = 0; nAttr < 256; nAttr++)
	{
		u8 nBright = (nAttr & 0x40) >> 3;
		m_Ink[nAttr]   = (nAttr & 0x07) | nBright;
		m_Paper[nAttr] = ((nAttr >> 3) & 0x07) | nBright;
	}

	// Tabla con el offset del primer byte de cada línea de pantalla
	for (unsigned nLine = 0; nLine < 192; nLine++)
	{
		m_ScrAddr[nLine] = ((nLine & 0xC0) << 5) | ((nLine & 0x07) << 8) | ((nLine & 0x38) << 2);
	}

	for (unsigned nAddr = 0; nAddr < 0x1800; nAddr++)
	{
		unsigned nRow  = ((nAddr & 0xE0) >> 5) | ((nAddr & 0x1800) >> 8);
		unsigned nCol  = nAddr & 0x1F;
		unsigned nScan = (nAddr & 0x700) >> 8;
		m_BufAddr[nAddr] = (nRow * 8 + nScan + BORDER_TOP) * SCREEN_WIDTH + nCol * 8 + BORDER_LEFT;
		m_Scr2Attr[nAddr] = 0x1800 + nRow * 32 + nCol;
	}

	memset (m_Screen, 0, sizeof m_Screen);
}

void CULA::SetTiming (const TMachineTiming *pTiming)
{
	m_pTiming = pTiming;
	BuildTables ();
}

void CULA::Reset (void)
{
	m_bULAplusActive = FALSE;
	m_bULAplusUsed = FALSE;
	m_nULAplusRegister = 0;
}

// Paleta según el modelo, como JSpeccy: la del 48K (medida) en el 16K y el
// 48K, la de siempre en los demás. pureblack: negro puro también en la del 48K.
void CULA::UpdatePalette (void)
{
	boolean b48 = m_pTiming->CodeModel == CodeModel48K;	// también los TK
	for (unsigned i = 0; i < 16; i++)
	{
		u32 nRGB = b48 ? SpectrumPaletteRGB[i] : SpectrumPalette128RGB[i];
		if (m_bPureBlack && (i & 7) == 0)
		{
			nRGB = 0x000000;
		}
		m_StdColor[i] = RGB565 (nRGB);
	}
}

////////////////////////////////////////////////////////////////////////////////
// ULAplus

void CULA::SetULAplusColor (unsigned nIndex, u8 nColor)
{
	m_ULAplusPalette[nIndex] = nColor;
	m_ULAplusColor[nIndex] = RGB565 (ULAplusToRGB (nColor));
}

u8 CULA::ULAplusRead (void) const
{
	return m_nULAplusRegister == 0x40 ? (m_bULAplusActive ? 0x01 : 0x00)
					  : m_ULAplusPalette[m_nULAplusRegister];
}

void CULA::ULAplusWrite (u16 nPort, u8 nValue, int nTstates, unsigned nBorder)
{
	if ((nPort & 0x4000) == 0)
	{
		// registro: grupo 00 = paleta (entrada 0-63), grupo 01 = modo
		m_nULAplusRegister = (nValue & 0xC0) == 0x40 ? 0x40 : (nValue & 0x3F);
		return;
	}

	// Antes de cambiar colores o modo se dibuja todo lo anterior con los actuales
	UpdateScreen (nTstates);
	UpdateBorder (nTstates, nBorder);
	m_bULAplusUsed = TRUE;

	if (m_nULAplusRegister == 0x40)
	{
		m_bULAplusActive = (nValue & 0x01) != 0;
	}
	else
	{
		SetULAplusColor (m_nULAplusRegister, nValue);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Bus flotante

u8 CULA::FloatingBus (int nTstates) const
{
	const TMachineTiming *T = m_pTiming;

	if (nTstates < T->nFirstScrByte || nTstates > T->nLastScrUpdate)
	{
		return 0xFF;
	}

	int nCol = ((nTstates - T->nPaperCol) % T->nTstatesLine) - T->nOutOffset;
	if (nCol < 0 || nCol > 124)
	{
		return 0xFF;
	}

	int nRow = (nTstates - T->nPaperCol) / T->nTstatesLine - T->nUpBorderWidth;
	if (nRow < 0 || nRow > 191)
	{
		return 0xFF;
	}

	unsigned nAddr = m_ScrAddr[nRow] + nCol / 4;
	switch (nCol % 8)
	{
	case 0:
		return m_pScreenPage[nAddr];

	case 1:
		return m_pScreenPage[m_Scr2Attr[nAddr]];

	case 2:
		nAddr++;
		return nAddr < 0x1800 ? m_pScreenPage[nAddr] : 0xFF;

	case 3:
		nAddr++;
		return nAddr < 0x1800 ? m_pScreenPage[m_Scr2Attr[nAddr]] : 0xFF;

	default:
		return 0xFF;
	}
}

u8 CULA::VideoFetch (int nRow, int nCol) const
{
	unsigned nAddr = m_ScrAddr[nRow] + nCol / 4;
	switch (nCol % 8)
	{
	case 0:	return m_pScreenPage[nAddr];
	case 1:	return m_pScreenPage[m_Scr2Attr[nAddr]];
	case 2:	nAddr++; return nAddr < 0x1800 ? m_pScreenPage[nAddr] : 0xFF;
	default: nAddr++; return nAddr < 0x1800 ? m_pScreenPage[m_Scr2Attr[nAddr]] : 0xFF;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Imagen

int CULA::TstatesToBorderPix (int nTstates)
{
	const TMachineTiming *T = m_pTiming;

	// relativo al principio del papel en su línea (nPaperCol: 0 salvo en
	// la ULA de Microdigital de los TK). Un tiempo negativo es el final de
	// la línea anterior a la primera (el borde izquierdo de la línea 0 en el
	// TK a 60 Hz), no el final del cuadro. Y uno posterior al final del cuadro
	// sigue en la última línea: en los TK, con el papel desplazado, el borde
	// derecho de la última línea cae ahí (antes se le daba la vuelta al cuadro
	// y esa esquina, abajo a la derecha, no se pintaba nunca: una raya negra)
	nTstates -= T->nPaperCol;
	int nRow = nTstates >= 0 ? nTstates / T->nTstatesLine : -1;
	int nCol = nTstates - nRow * T->nTstatesLine;

	// Primera línea visible y primera columna del borde izquierdo
	int nFirstRow, nLeftStart;
	if (T->CodeModel == CodeModel48K)
	{
		nFirstRow = T->nUpBorderWidth;			// 64 (38 en el TK a 60 Hz)
		nLeftStart = (T->nTstatesLine - 24) + (24 - BORDER_LEFT / 2);	// 200 en el 48K
	}
	else
	{
		nFirstRow = 63;
		nLeftStart = 204 + (24 - BORDER_LEFT / 2);
	}
	int nLastRow = nFirstRow + 192;

	if (nRow < nFirstRow - BORDER_TOP - 1 || nRow > nLastRow + BORDER_BOTTOM - 1)
	{
		return ULA_NO_PIXEL;
	}

	if (nRow == nFirstRow - BORDER_TOP - 1 && nCol < nLeftStart)
	{
		return ULA_NO_PIXEL;
	}

	if (nRow == nLastRow + BORDER_BOTTOM - 1 && nCol > 127 + BORDER_RIGHT / 2)
	{
		return ULA_NO_PIXEL;
	}

	// Zona de retrazo horizontal (invisible)
	if (nCol > 127 + BORDER_RIGHT / 2 && nCol < nLeftStart)
	{
		return ULA_NO_PIXEL;
	}

	// Zona de pantalla (no es borde)
	if (nRow > nFirstRow - 1 && nRow < nLastRow && nCol < 128)
	{
		return ULA_NO_PIXEL;
	}

	if (nCol > 176)
	{
		nRow++;
		nCol -= nLeftStart;
	}
	else
	{
		nCol += BORDER_RIGHT / 2 - (BORDER_RIGHT - BORDER_LEFT) / 2;
	}

	nRow -= nFirstRow - BORDER_TOP;

	return nRow * SCREEN_WIDTH + nCol * 2;
}

void CULA::BuildTables (void)
{
	const TMachineTiming *T = m_pTiming;

	// Momentos en que la ULA lee cada byte de la pantalla
	int nStart, nEnd, nOffset;
	switch (T->CodeModel)
	{
	case CodeModel48K:
	default:
		nStart = T->nFirstScrByte; nEnd = T->nLastScrUpdate + 2; nOffset = 2;	// 57248 en el 48K
		break;

	case CodeModel128K:
		nStart = 14364; nEnd = T->nLastScrUpdate; nOffset = 0;
		break;

	case CodeModelPlus3:
		nStart = T->nFirstScrByte; nEnd = T->nLastScrUpdate; nOffset = 2;
		break;
	}

	m_nSteps = 0;
	for (int t = nStart; t < nEnd && m_nSteps < 6144; t += 4)
	{
		int nCol = ((t - T->nPaperCol) % T->nTstatesLine) / 4;
		if (nCol > 31)
		{
			continue;
		}

		int nScan = (t - T->nPaperCol) / T->nTstatesLine - T->nUpBorderWidth;
		if (nScan < 0 || nScan > 191)
		{
			continue;
		}

		m_StepStates[m_nSteps] = t + nOffset;
		m_StepAddr[m_nSteps] = m_ScrAddr[nScan] + nCol;
		m_nSteps++;
	}

	// Borde
	if (T->CodeModel == CodeModel48K)
	{
		m_nFirstBorderUpdate = (T->nUpBorderWidth - BORDER_TOP) * T->nTstatesLine - BORDER_LEFT / 2 + T->nPaperCol;
		m_nLastBorderUpdate = (T->nUpBorderWidth + 191 + BORDER_BOTTOM) * T->nTstatesLine + 128 + BORDER_RIGHT
				      + T->nPaperCol;
		if (m_nFirstBorderUpdate < 0)
		{
			m_nFirstBorderUpdate = 0;	// TK a 60 Hz: menos líneas por encima del papel
		}
		if (m_nLastBorderUpdate > MAX_TSTATES_FRAME)
		{
			m_nLastBorderUpdate = MAX_TSTATES_FRAME;
		}
	}
	else
	{
		m_nFirstBorderUpdate = (63 - BORDER_TOP) * T->nTstatesLine - BORDER_RIGHT / 2;
		m_nLastBorderUpdate = (254 + BORDER_BOTTOM) * T->nTstatesLine + 128 + BORDER_RIGHT;
	}

	for (unsigned i = 0; i < sizeof m_States2Border / sizeof m_States2Border[0]; i++)
	{
		m_States2Border[i] = ULA_NO_PIXEL;
	}

	for (int t = m_nFirstBorderUpdate; t < m_nLastBorderUpdate; t += 4)
	{
		int nPix = TstatesToBorderPix (t);
		m_States2Border[t] = m_States2Border[t + 1] =
		m_States2Border[t + 2] = m_States2Border[t + 3] = nPix;
	}

	// Contención
	memset (m_Delay, 0, sizeof m_Delay);
	switch (T->CodeModel)
	{
	case CodeModel48K:
	default:
		// 14335 a 57247 en el 48K; desde el primer byte de pantalla en los TK
		for (int nIdx = T->nFirstScrByte - 1; nIdx < T->nLastScrUpdate + 1; nIdx += T->nTstatesLine)
		{
			for (int n = 0; n < 128; n += 8)
			{
				static const u8 Pattern[8] = {6, 5, 4, 3, 2, 1, 0, 0};
				memcpy (&m_Delay[nIdx + n], Pattern, 8);
			}
		}
		break;

	case CodeModel128K:
		for (int nIdx = 14361; nIdx < 58037; nIdx += T->nTstatesLine)
		{
			for (int n = 0; n < 128; n += 8)
			{
				static const u8 Pattern[8] = {6, 5, 4, 3, 2, 1, 0, 0};
				memcpy (&m_Delay[nIdx + n], Pattern, 8);
			}
		}
		break;

	case CodeModelPlus3:
		for (int nIdx = 14361; nIdx < 58036; nIdx += T->nTstatesLine)
		{
			for (int n = 0; n < 128; n += 8)
			{
				static const u8 Pattern[8] = {1, 0, 7, 6, 5, 4, 3, 2};
				memcpy (&m_Delay[nIdx + n], Pattern, 8);
			}
			m_Delay[nIdx + 128] = 1;	// las líneas tienen 129 t-states de contención
		}
		break;
	}

	m_nStep = 0;
	m_nLastChgBorder = m_nFirstBorderUpdate;

	UpdatePalette ();			// la del 48K o la del 128K
}

// Efecto nieve (16K, 48K, 128K y +2). En los T3 y T4 de cada M1 el Z80
// refresca la memoria con I:R en el bus; si I apunta a memoria lenta (0x40-
// 0x7F, o en el 128K 0xC0-0xFF con una página impar arriba), ese refresco
// choca con la lectura de la ULA, que lee 16 píxeles cada 8 t-states: imagen
// y atributo del primer par en las fases 2 y 3, y del segundo en las 4 y 5
// (la fase 0 es el primer t-state de contención de la línea: 14335 en el
// 48K y 14361 en el 128K, como en Spectramine de Weiv y en ESPectrum). Según en qué fase cae el T4 de la lectura de la instrucción:
//   - fase 2 (nieve): la fila de la memoria (A0-A6) del primer par sale de R
//     y no de la ULA: se leen la imagen y el atributo de (dirección & ~0x7F)
//     | R, del mismo bloque de 128 bytes (por eso el dibujo se reconoce)
//   - fase 4 (doble): el segundo par no se lee y se repite el primero
// R es el de después de sumarle 1 en este M1, como dice Weiv (ESPectrum usa
// el de antes).
void CULA::Snow (const u8 *pPage, int nTstates, u8 nR)
{
	const TMachineTiming *T = m_pTiming;
	int nRel = nTstates - 1 - (T->nFirstScrByte - 1);	// el T4, desde 14335
	if (nRel < 0)
	{
		return;
	}
	unsigned nLine = nRel / T->nTstatesLine;
	unsigned nCol = nRel % T->nTstatesLine;
	if (nLine > 191 || nCol > 127)
	{
		return;
	}
	unsigned nPhase = nCol & 7;
	if (nPhase != 2 && nPhase != 4)
	{
		return;
	}

	unsigned nCell = nLine * 32 + (nCol >> 3) * 2 + (nPhase == 4);	// la celda de 8 píxeles
	UpdateScreen (m_StepStates[nCell] - 1);		// todo lo de antes, normal
	if (m_nStep != nCell)
	{
		return;
	}
	m_nStep++;

	unsigned nAddr = m_StepAddr[nCell];
	if (nPhase == 2)
	{
		unsigned nRow = (nR + 1) & 0x7F;	// ya sumado (Weiv); z80cpp lo suma al volver
		DrawCell (nAddr, pPage[(nAddr & ~0x7F) | nRow],
			  pPage[(m_Scr2Attr[nAddr] & ~0x7F) | nRow]);
	}
	else
	{
		DrawCell (nAddr, m_nLastByte, m_nLastAttr);
	}
}

void CULA::UpdateBorder (int nTstates, unsigned nBorder)
{
	if (nTstates < m_nLastChgBorder || m_nLastChgBorder > m_nLastBorderUpdate)
	{
		return;
	}

	TPixel nColor = BorderColor (nBorder);

	nTstates += m_pTiming->nOutBorderOffset;
	nTstates &= ~3;

	while (m_nLastChgBorder < nTstates && m_nLastChgBorder < m_nLastBorderUpdate)
	{
		int nPix = m_States2Border[m_nLastChgBorder];
		m_nLastChgBorder += 4;

		if (nPix == ULA_NO_PIXEL)
		{
			continue;
		}

		TPixel *pPixel = &m_Screen[nPix];
		for (unsigned i = 0; i < 8; i++)
		{
			pPixel[i] = nColor;
		}
	}

	m_nLastChgBorder = nTstates;
}

void CULA::EndFrame (unsigned nBorder)
{
	UpdateScreen (0x7FFFFFFF);
	UpdateBorder (m_nLastBorderUpdate + 4, nBorder);

	// Cuadros con menos líneas de borde de las que tiene la imagen (TK a
	// 60 Hz: 38 por encima del papel y 32 por debajo): esas líneas no
	// existen y se pintan del color del borde, en vez de quedarse como
	// estuvieran. En los demás modelos sobran líneas y no se hace nada.
	const TMachineTiming *T = m_pTiming;
	int nTop = BORDER_TOP - T->nUpBorderWidth;
	int nBottom = BORDER_BOTTOM - (T->nScanLines - T->nUpBorderWidth - 192);
	if (nTop > 0 || nBottom > 0)
	{
		TPixel nColor = BorderColor (nBorder);
		for (int y = 0; y < SCREEN_HEIGHT; y++)
		{
			if ((nTop > 0 && y < nTop) || (nBottom > 0 && y >= SCREEN_HEIGHT - nBottom))
			{
				TPixel *pRow = &m_Screen[y * SCREEN_WIDTH];
				for (int x = 0; x < SCREEN_WIDTH; x++)
				{
					pRow[x] = nColor;
				}
			}
		}
	}

	// (con ULAplus activo no hay parpadeo: FLASH elige subpaleta)
	if ((++m_nFrames & 0x0F) == 0)
	{
		m_bFlash = !m_bFlash;
	}
}
