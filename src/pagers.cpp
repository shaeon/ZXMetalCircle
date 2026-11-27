//
// pagers.cpp - Periféricos que tapan la memoria: Interface 2, LEC y Multiface
//
// Lógica portada a C++ desde JSpeccy (machine/Memory.java, Spectrum.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "pagers.h"
#include <circle/util.h>

////////////////////////////////////////////////////////////////////////////////
// Interface 2

boolean CInterface2::Insert (const u8 *pROM, unsigned nSize, TCodeModel CodeModel)
{
	if (nSize == 0 || nSize > 0x4000 || CodeModel == CodeModelPlus3)
	{
		return FALSE;
	}

	// Las imágenes de menos de 16 KB se rellenan con 0xFF (bus sin datos)
	memset (m_ROM, 0xFF, sizeof m_ROM);
	memcpy (m_ROM, pROM, nSize);
	m_bInserted = TRUE;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// LEC

CLEC::CLEC (const CMemory &rMem)
:	m_rMem (rMem),
	m_pRAM (0),
	m_bPaged (FALSE),
	m_bUsed (FALSE),
	m_nPage (0)
{
}

CLEC::~CLEC (void)
{
	delete [] m_pRAM;
}

boolean CLEC::SetConnected (boolean bConnected)
{
	if (bConnected && m_pRAM == 0)
	{
		m_pRAM = new u8[15 * 0x8000];	// 480 KB
		if (m_pRAM == 0)
		{
			return FALSE;
		}
		memset (m_pRAM, 0, 15 * 0x8000);
	}
	else if (!bConnected && m_pRAM != 0)
	{
		m_bPaged = FALSE;
		m_bUsed = FALSE;
		delete [] m_pRAM;
		m_pRAM = 0;
	}

	return TRUE;
}

// En los 128K, el puerto de la LEC (0x??FD) es también el de la paginación
// (0x7FFD) y el del chip AY (0xBFFD, 0xFFFD). En la máquina real chocarían; el
// emulador solo toma para la LEC las escrituras con la forma del CP/M de LEC:
// OUT (0xFD),A, en las que el byte alto del puerto es el propio valor. Así el
// AY y el resto del Spectrum siguen funcionando.
boolean CLEC::IsPort (u16 nPort, u8 nValue) const
{
	if (m_rMem.GetModel () == Model48K)
	{
		return (nPort & 0x0002) == 0;
	}
	return (nPort & 0xFF) == 0xFD && (nPort >> 8) == nValue;
}

// 128K en modo 48K: paginación bloqueada y la ROM del BASIC de 48K
boolean CLEC::Is128Mode (void) const
{
	TModel Model = m_rMem.GetModel ();
	if (Model == Model48K || Model == Model16K || !m_rMem.IsPagingLocked ())
	{
		return FALSE;
	}
	if (m_rMem.GetCodeModel () == CodeModelPlus3)
	{
		if (m_rMem.IsSpecialPaging ())
		{
			return FALSE;		// paginación especial (todo RAM)
		}
		return m_rMem.GetROMNumber () == 3;
	}
	return m_rMem.GetROMNumber () == 1;
}

void CLEC::Write (u8 nValue)
{
	if (nValue & 0x80)
	{
		// número de página en los bits b3 b6 b5 b4
		m_nPage = ((nValue & 0x70) >> 4) | (nValue & 0x08);
		m_bPaged = TRUE;
		m_bUsed = TRUE;
	}
	else
	{
		m_bPaged = FALSE;
	}
}

// Paginada: sus 32 KB ocupan 0x0000-0x7FFF y no hay ROM ni periféricos que se
// superpongan (como en JSpeccy)
boolean CLEC::Map (CMemory &rMem) const
{
	if (!m_bPaged || !IsActive ())
	{
		return FALSE;
	}

	for (unsigned nSlot = 0; nSlot < 2; nSlot++)
	{
		u8 *pPage;
		if (m_nPage == 15)
		{
			pPage = rMem.Bank (nSlot == 0 ? 2 : 0);	// la RAM alta del propio 48K
		}
		else
		{
			pPage = m_pRAM + m_nPage * 0x8000 + nSlot * 0x4000;
		}
		rMem.MapSlot (nSlot, pPage, pPage);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Multiface
//
// Basado en JSpeccy (machine/Memory.java y machine/Spectrum.java):
//  - El NMI (botón rojo) pagina el MF y lo desbloquea al leer la dirección 0x0066.
//  - MF1 (48K):   IN 0x9F pagina, IN 0x1F despagina.
//  - MF128 (128K): IN 0xBF pagina (si no está bloqueado; 0x01BF devuelve 0x7FFD),
//                 IN 0x3F despagina.
//  - MF3 (+2A):   IN 0x3F pagina (si no está bloqueado; 0x7F3F y 0x1F3F devuelven
//                 0x7FFD y 0x1FFD), IN 0xBF despagina.
//  - MF128/MF3: con el MF paginado y PC < 0x4000, OUT 0x3F bloquea y OUT 0xBF desbloquea.

CMultiface::CMultiface (const CMemory &rMem)
:	m_rMem (rMem),
	m_bEnabled (TRUE),
	m_bPaged (FALSE),
	m_bLocked (TRUE)
{
	for (unsigned i = 0; i < 3; i++)
	{
		m_bHasROM[i] = FALSE;
	}
	memset (m_Page, 0, sizeof m_Page);
}

void CMultiface::SetROM (TCodeModel CodeModel, const u8 *pROM)
{
	if (pROM != 0)
	{
		memcpy (m_ROM[CodeModel], pROM, 0x2000);
		m_bHasROM[CodeModel] = TRUE;
	}
	else
	{
		m_bHasROM[CodeModel] = FALSE;
	}
}

boolean CMultiface::Page (void)
{
	if (m_bPaged || m_rMem.IsSpecialPaging ())
	{
		return FALSE;
	}

	memcpy (m_Page, m_ROM[m_rMem.GetCodeModel ()], 0x2000);
	m_bPaged = TRUE;

	return TRUE;
}

boolean CMultiface::Unpage (void)
{
	if (!m_bPaged)
	{
		return FALSE;
	}

	m_bPaged = FALSE;

	return TRUE;
}

boolean CMultiface::In (u16 nPort, u8 *pValue, boolean *pbPaging)
{
	u8 nLow = nPort & 0xFF;

	switch (m_rMem.GetCodeModel ())
	{
	case CodeModel48K:
		if (nLow == 0x9F)
		{
			*pbPaging = Page ();
		}
		else if (nLow == 0x1F && m_bPaged)
		{
			*pbPaging = Unpage ();		// el MF1 también tenía un puerto Kempston
		}
		break;

	case CodeModel128K:
		if (nLow == 0xBF && !m_bLocked)
		{
			if (nPort == 0x01BF && m_bPaged)
			{
				*pValue = m_rMem.GetPort7FFD ();
				return TRUE;
			}
			*pbPaging = Page ();
		}
		else if (nLow == 0x3F && m_bPaged)
		{
			*pbPaging = Unpage ();
		}
		break;

	case CodeModelPlus3:
		if (nLow == 0xBF && m_bPaged)
		{
			*pbPaging = Unpage ();
		}
		else if (nLow == 0x3F)
		{
			if (!m_bLocked && m_bPaged)
			{
				if (nPort == 0x7F3F)
				{
					*pValue = m_rMem.GetPort7FFD ();
					return TRUE;
				}
				if (nPort == 0x1F3F)
				{
					*pValue = m_rMem.GetPort1FFD ();
					return TRUE;
				}
			}

			if (!m_bLocked)
			{
				*pbPaging = Page ();
			}
		}
		break;
	}

	return FALSE;
}

void CMultiface::Out (u16 nPort, u16 nPC)
{
	if (   !m_bPaged
	    || m_rMem.GetCodeModel () == CodeModel48K
	    || nPC >= 0x4000)
	{
		return;
	}

	u8 nLow = nPort & 0xFF;
	if (nLow == 0x3F)
	{
		m_bLocked = TRUE;
	}
	else if (nLow == 0xBF)
	{
		m_bLocked = FALSE;
	}
}

// Paginado: su ROM y su RAM ocupan los primeros 16 KB
void CMultiface::Map (CMemory &rMem)
{
	if (!m_bPaged)
	{
		return;
	}

	if (rMem.IsSpecialPaging ())
	{
		m_bPaged = FALSE;	// modo "all RAM" del +2A: el MF se retira
	}
	else
	{
		rMem.MapSlot (0, m_Page, m_Page);
	}
}
