//
// ula.h - La ULA del Spectrum: imagen, borde, colores, contención y bus flotante
//
// Fase 2 de la capa de máquinas: lo que hace la ULA (o la Gate Array del
// +2A/+3, o la ULA de los TK) sale de CSpectrum. CSpectrum le dice en qué
// t-state va y qué página es la pantalla; la ULA pinta, dice cuánto retrasa a
// la CPU (contención) y qué se ve en el bus cuando nadie lo ocupa. Los tiempos
// salen de TMachineTiming, así que un modelo con otra ULA (Pentagon, Timex...)
// pondrá aquí lo que cambia.
//
// Lo que se hace en cada acceso de la CPU (Delay, UpdateScreen, DrawCell)
// está aquí mismo, inline: CSpectrum lo tiene dentro (m_ULA, sin punteros ni
// funciones virtuales), y el compilador lo mete en su sitio como antes.
//
// Lógica portada a C++ desde JSpeccy (machine/Spectrum.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_ula_h
#define _zx_ula_h

#include "machine.h"
#include "palette.h"
#include <circle/types.h>

#define ULA_NO_PIXEL	(-1)

class CULA
{
	friend class CSnapshot;		// la paleta de ULAplus en los SZX

public:
	CULA (void);

	// Al elegir el modelo: tablas de tiempos, contención y paleta
	void SetTiming (const TMachineTiming *pTiming);
	// Reset de la máquina: ULAplus apagada
	void Reset (void);

	// La página de RAM que se ve en la pantalla (la elige la paginación)
	void SetScreenPage (const u8 *pPage)	{ m_pScreenPage = pPage; }
	const u8 *GetScreenPage (void) const	{ return m_pScreenPage; }

	TPixel *GetScreen (void)		{ return m_Screen; }

	// Contención: t-states que espera la CPU si accede a memoria o puertos
	// lentos en el t-state nTstates
	u8 Delay (int nTstates) const		{ return m_Delay[nTstates]; }

	// Comienzo y final del cuadro. nBorder: el color del borde (0-7) que
	// queda para el resto del cuadro
	void StartFrame (void)
	{
		m_nStep = 0;
		m_nLastChgBorder = m_nFirstBorderUpdate;
	}
	void EndFrame (unsigned nBorder);

	// Pinta la pantalla hasta el t-state nTstates (antes de cambiar lo que
	// se ve: escribir en la pantalla, cambiar la página o los colores)
	void UpdateScreen (int nTstates)
	{
		while (m_nStep < m_nSteps && m_StepStates[m_nStep] <= nTstates)
		{
			unsigned nAddr = m_StepAddr[m_nStep++];
			DrawCell (nAddr, m_pScreenPage[nAddr], m_pScreenPage[m_Scr2Attr[nAddr]]);
		}
	}
	// Pinta el borde hasta nTstates con el color nBorder (0-7), el que había
	// hasta ahora (antes de cambiarlo con OUT)
	void UpdateBorder (int nTstates, unsigned nBorder);

	// Efecto nieve: en el T4 del M1 (nTstates, después de contar el M1) con I
	// apuntando a memoria lenta (pPage) y R ya incrementado
	void Snow (const u8 *pPage, int nTstates, u8 nR);

	// Bus flotante del 48K/128K: lo que lee la ULA de la pantalla en
	// nTstates (0xFF si no lee nada)
	u8 FloatingBus (int nTstates) const;
	// Lo que lee la Gate Array del +2A/+3 en la fila nRow (0-191) y la
	// columna nCol (0-123) de su recorrido
	u8 VideoFetch (int nRow, int nCol) const;

	// Colores
	void SetPureBlack (boolean bPureBlack)	{ m_bPureBlack = bPureBlack; UpdatePalette (); }
	boolean IsPureBlack (void) const	{ return m_bPureBlack; }
	TPixel GetStdColor (unsigned nIndex) const	{ return m_StdColor[nIndex & 15]; }

	// ULAplus (64 colores, puertos 0xBF3B/0xFF3B)
	void SetULAplusEnabled (boolean bEnabled)	{ m_bULAplusEnabled = bEnabled; }
	boolean IsULAplusEnabled (void) const		{ return m_bULAplusEnabled; }
	boolean IsULAplusActive (void) const		{ return m_bULAplusActive; }
	boolean IsULAplusInUse (void) const		{ return m_bULAplusActive || m_bULAplusUsed; }
	// Un puerto de ULAplus (con ULAplus conectada)
	boolean IsULAplusPort (u16 nPort) const
	{
		u16 nMasked = nPort & 0xC0FF;
		return m_bULAplusEnabled && (nMasked == 0x803B || nMasked == 0xC03B);
	}
	u8 ULAplusRead (void) const;
	// nTstates y nBorder: para pintar antes lo anterior con los colores de antes
	void ULAplusWrite (u16 nPort, u8 nValue, int nTstates, unsigned nBorder);

private:
	void BuildTables (void);
	void UpdatePalette (void);
	int TstatesToBorderPix (int nTstates);
	void SetULAplusColor (unsigned nIndex, u8 nColor);
	TPixel BorderColor (unsigned nBorder) const
	{
		// con ULAplus, el borde usa los colores de papel de la subpaleta 0
		return m_bULAplusActive ? m_ULAplusColor[8 + (nBorder & 0x07)] : m_StdColor[nBorder & 0x07];
	}

	void DrawCell (unsigned nAddr, u8 nByte, u8 nAttr)
	{
		m_nLastByte = nByte;
		m_nLastAttr = nAttr;

		TPixel nInk, nPaper;
		if (m_bULAplusActive)
		{
			// FLASH y BRIGHT eligen una de las 4 subpaletas de 16 colores
			unsigned nCLUT = (nAttr & 0xC0) >> 2;
			nInk = m_ULAplusColor[nCLUT | (nAttr & 0x07)];
			nPaper = m_ULAplusColor[nCLUT | 0x08 | ((nAttr >> 3) & 0x07)];
		}
		else
		{
			nInk = m_StdColor[m_Ink[nAttr]];
			nPaper = m_StdColor[m_Paper[nAttr]];
			if ((nAttr & 0x80) && m_bFlash)
			{
				TPixel nTemp = nInk; nInk = nPaper; nPaper = nTemp;
			}
		}

		TPixel *pPixel = &m_Screen[m_BufAddr[nAddr]];
		for (unsigned nMask = 0x80; nMask != 0; nMask >>= 1)
		{
			*pPixel++ = (nByte & nMask) ? nInk : nPaper;
		}
	}

private:
	const TMachineTiming *m_pTiming;
	const u8 *m_pScreenPage;

	TPixel m_Screen[SCREEN_WIDTH * SCREEN_HEIGHT];
	TPixel m_StdColor[16];		// colores normales del Spectrum en RGB565
	boolean m_bPureBlack;

	u8 m_Delay[MAX_TSTATES_FRAME + 128];		// contención
	int m_States2Border[MAX_TSTATES_FRAME + 128];
	int m_StepStates[6144];
	u16 m_StepAddr[6144];
	unsigned m_nSteps, m_nStep;
	int m_nFirstBorderUpdate, m_nLastBorderUpdate, m_nLastChgBorder;
	u16 m_ScrAddr[192];		// offset (0-0x17FF) del primer byte de cada línea
	u16 m_Scr2Attr[6144];
	u32 m_BufAddr[6144];
	u8 m_Ink[256], m_Paper[256];
	boolean m_bFlash;
	unsigned m_nFrames;
	u8 m_nLastByte, m_nLastAttr;	// lo último que leyó la ULA (para el doble efecto)

	// ULAplus
	boolean m_bULAplusEnabled;
	boolean m_bULAplusActive;
	boolean m_bULAplusUsed;		// se ha escrito en ella desde el reset
	u8 m_nULAplusRegister;		// 0-63 = entrada de la paleta, 0x40 = modo
	u8 m_ULAplusPalette[64];	// GGGRRRBB
	TPixel m_ULAplusColor[64];
};

#endif
