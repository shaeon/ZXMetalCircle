//
// zxmemory.cpp - Memoria del Spectrum: RAM, ROMs y paginación
//
// Lógica portada a C++ desde JSpeccy (machine/Memory.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "zxmemory.h"
#include <circle/util.h>

extern "C" const u8 rom_48[];
extern "C" const u8 rom_128_0[];
extern "C" const u8 rom_128_1[];

CMemory::CMemory (void)
:	m_Model (Model48K),
	m_pTiming (&MachineTiming[Model48K]),
	m_pBasicROM (rom_48),
	m_pScreenPage (0),
	m_nPort7FFD (0),
	m_nPort1FFD (0)
{
	memset (m_RAM, 0, sizeof m_RAM);
	memset (m_ROMSink, 0, sizeof m_ROMSink);
	memset (m_NoRAM, 0xFF, sizeof m_NoRAM);
	for (unsigned i = 0; i < 4; i++)
	{
		m_ROM[i] = rom_48;
		m_pReadPage[i] = rom_48;
		m_pWritePage[i] = m_ROMSink;
		m_bContendedRAM[i] = FALSE;
		m_bContendedIO[i] = FALSE;
	}
}

void CMemory::SetModel (TModel Model, const TMachineTiming *pTiming, const u8 *pROM)
{
	m_Model = Model;
	m_pTiming = pTiming;

	switch (m_pTiming->CodeModel)
	{
	case CodeModel48K:
		if (pROM == 0)
		{
			pROM = rom_48;
		}
		m_ROM[0] = m_ROM[1] = m_ROM[2] = m_ROM[3] = pROM;
		m_pBasicROM = pROM;
		break;

	case CodeModel128K:
		if (pROM == 0)
		{
			m_ROM[0] = m_ROM[2] = rom_128_0;
			m_ROM[1] = m_ROM[3] = rom_128_1;
		}
		else
		{
			m_ROM[0] = m_ROM[2] = pROM;
			m_ROM[1] = m_ROM[3] = pROM + 0x4000;
		}
		m_pBasicROM = m_ROM[1];
		break;

	default:		// +2A, +3 y +3e: cuatro bancos de 16 KB
		for (unsigned i = 0; i < 4; i++)
		{
			m_ROM[i] = pROM + i * 0x4000;
		}
		m_pBasicROM = m_ROM[3];
		break;
	}
}

void CMemory::Reset (void)
{
	memset (m_RAM, 0, sizeof m_RAM);
	m_nPort7FFD = 0;
	m_nPort1FFD = 0;
}

unsigned CMemory::GetROMNumber (void) const
{
	switch (m_pTiming->CodeModel)
	{
	case CodeModel128K:
		return (m_nPort7FFD >> 4) & 1;

	case CodeModelPlus3:
		return ((m_nPort1FFD >> 1) & 0x02) | ((m_nPort7FFD >> 4) & 0x01);

	default:
		return 0;
	}
}

void CMemory::UpdatePaging (void)
{
	switch (m_pTiming->CodeModel)
	{
	case CodeModel48K:
		m_pReadPage[0] = m_ROM[0];
		m_pReadPage[1] = m_RAM[5];
		m_pReadPage[2] = m_RAM[2];
		m_pReadPage[3] = m_RAM[0];
		if (m_Model == Model16K)
		{
			m_pReadPage[2] = m_pReadPage[3] = m_NoRAM;	// las escrituras se pierden
		}
		m_pScreenPage = m_RAM[5];
		break;

	case CodeModel128K:
		m_pReadPage[0] = m_ROM[GetROMNumber ()];
		m_pReadPage[1] = m_RAM[5];
		m_pReadPage[2] = m_RAM[2];
		m_pReadPage[3] = m_RAM[m_nPort7FFD & 7];
		m_pScreenPage = m_RAM[(m_nPort7FFD & 0x08) ? 7 : 5];
		break;

	case CodeModelPlus3:
		if (m_nPort1FFD & 0x01)
		{
			// Modos especiales "all RAM"
			static const u8 Special[4][4] =
			{
				{0, 1, 2, 3}, {4, 5, 6, 7}, {4, 5, 6, 3}, {4, 7, 6, 3}
			};
			const u8 *pBanks = Special[(m_nPort1FFD >> 1) & 3];
			for (unsigned i = 0; i < 4; i++)
			{
				m_pReadPage[i] = m_RAM[pBanks[i]];
			}
		}
		else
		{
			m_pReadPage[0] = m_ROM[GetROMNumber ()];
			m_pReadPage[1] = m_RAM[5];
			m_pReadPage[2] = m_RAM[2];
			m_pReadPage[3] = m_RAM[m_nPort7FFD & 7];
		}
		m_pScreenPage = m_RAM[(m_nPort7FFD & 0x08) ? 7 : 5];
		break;
	}

	for (unsigned nSlot = 0; nSlot < 4; nSlot++)
	{
		const u8 *pPage = m_pReadPage[nSlot];

		int nBank = -1;
		for (unsigned i = 0; i < 8; i++)
		{
			if (pPage == m_RAM[i])
			{
				nBank = i;
				break;
			}
		}

		m_pWritePage[nSlot] = nBank >= 0 ? m_RAM[nBank] : m_ROMSink;

		switch (m_pTiming->CodeModel)
		{
		case CodeModel48K:
			m_bContendedRAM[nSlot] = nSlot == 1;
			m_bContendedIO[nSlot] = nSlot == 1;
			break;

		case CodeModel128K:
			m_bContendedRAM[nSlot] = nBank >= 0 && (nBank & 1);
			m_bContendedIO[nSlot] = m_bContendedRAM[nSlot];
			break;

		case CodeModelPlus3:
			m_bContendedRAM[nSlot] = nBank >= 4;
			m_bContendedIO[nSlot] = FALSE;
			break;
		}
	}
}
