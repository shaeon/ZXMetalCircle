//
// pagers.h - Periféricos que tapan la memoria: Interface 2, LEC y Multiface
//
// Fase 2c de la capa de máquinas: los periféricos salen de CSpectrum. Todos
// siguen la misma forma, sin funciones virtuales:
//
//   Reset ()		lo que hace el botón de reset
//   Map (CMemory &)	si está paginado, pone sus páginas encima (MapSlot)
//   y sus puertos	CSpectrum se los pasa cuando le tocan
//
// Cuando su paginación cambia, devuelven TRUE (o lo dicen en *pbPaging) y
// CSpectrum vuelve a calcular las páginas (UpdatePaging).
//
// Lógica portada a C++ desde JSpeccy (machine/Memory.java, Spectrum.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_pagers_h
#define _zx_pagers_h

#include "zxmemory.h"
#include <circle/types.h>

////////////////////////////////////////////////////////////////////////////////
// Interface 2: un cartucho de 16 KB en lugar de la ROM (no en el +2A/+3)

class CInterface2
{
	friend class CSnapshot;		// los cartuchos de los .z80 y .szx

public:
	CInterface2 (void) : m_bInserted (FALSE) {}

	// FALSE si no vale (vacío, más de 16 KB o un +2A/+3). Falta el reset.
	boolean Insert (const u8 *pROM, unsigned nSize, TCodeModel CodeModel);
	boolean Eject (void)		{ boolean bWas = m_bInserted; m_bInserted = FALSE; return bWas; }
	boolean IsInserted (void) const	{ return m_bInserted; }

	void Map (CMemory &rMem) const
	{
		if (m_bInserted && rMem.GetCodeModel () != CodeModelPlus3)
		{
			rMem.MapSlot (0, m_ROM, rMem.GetROMSink ());
		}
	}

private:
	boolean m_bInserted;
	u8 m_ROM[0x4000];
};

////////////////////////////////////////////////////////////////////////////////
// LEC (Lorenzo Electronic Computing): 480 KB en 0x0000-0x7FFF, en el 48K o en
// un 128K en modo 48K, para el CP/M de LEC

class CLEC
{
public:
	CLEC (const CMemory &rMem);
	~CLEC (void);

	// FALSE si no hay memoria. Al desconectar, hay que recalcular las páginas.
	boolean SetConnected (boolean bConnected);
	boolean IsConnected (void) const	{ return m_pRAM != 0; }

	void Reset (void)			{ m_bPaged = FALSE; m_bUsed = FALSE; }
	boolean IsPaged (void) const		{ return m_bPaged; }
	boolean IsUsed (void) const		{ return m_bUsed; }	// desde el reset

	// Conectada y con un modelo en el que funciona
	boolean IsActive (void) const
		{ return m_pRAM != 0 && (m_rMem.GetModel () == Model48K || Is128Mode ()); }
	boolean IsPort (u16 nPort, u8 nValue) const;
	void Write (u8 nValue);			// falta UpdatePaging

	// TRUE si la ha puesto (entonces no se pone nada más encima)
	boolean Map (CMemory &rMem) const;

private:
	boolean Is128Mode (void) const;

	const CMemory &m_rMem;
	u8 *m_pRAM;			// páginas 0-14 (la 15 es la RAM alta del propio 48K)
	boolean m_bPaged;
	boolean m_bUsed;
	unsigned m_nPage;
};

////////////////////////////////////////////////////////////////////////////////
// Multiface (MF1 en 48K, MF128 en 128K, MF3 en +2A/+3): 8 KB de ROM y 8 KB de
// RAM en 0x0000-0x3FFF

class CMultiface
{
public:
	CMultiface (const CMemory &rMem);

	// Las ROMs (8 KB) se cargan desde la SD; sin ROM no hay Multiface para
	// ese modelo
	void SetROM (TCodeModel CodeModel, const u8 *pROM);
	void SetEnabled (boolean bEnabled)	{ m_bEnabled = bEnabled; }
	// Conectado y con ROM para este modelo (la divMMC y la Spectranet, que
	// usan la NMI, las mira CSpectrum)
	boolean IsReady (void) const
		{ return m_bEnabled && m_bHasROM[m_rMem.GetCodeModel ()]; }

	void Reset (void)			{ m_bPaged = FALSE; m_bLocked = TRUE; }
	boolean IsPaged (void) const		{ return m_bPaged; }
	void Unlock (void)			{ m_bLocked = FALSE; }

	// TRUE si cambia la paginación (con el MF listo)
	boolean Page (void);
	boolean Unpage (void);

	// Puertos (con el MF listo). In: TRUE si el MF da el valor
	boolean In (u16 nPort, u8 *pValue, boolean *pbPaging);
	void Out (u16 nPort, u16 nPC);

	u8 Read (u16 nAddress) const		{ return m_Page[nAddress]; }
	boolean IsROM (u16 nAddress) const	{ return m_bPaged && nAddress < 0x2000; }

	void Map (CMemory &rMem);

private:
	const CMemory &m_rMem;
	boolean m_bEnabled;
	boolean m_bPaged;
	boolean m_bLocked;
	boolean m_bHasROM[3];
	u8 m_ROM[3][0x2000];
	u8 m_Page[0x4000];		// 0x0000-0x1FFF ROM, 0x2000-0x3FFF RAM
};

#endif
