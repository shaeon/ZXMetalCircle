//
// zxmemory.h - Memoria del Spectrum: RAM, ROMs y paginación
//
// Fase 2b de la capa de máquinas: la memoria sale de CSpectrum. CMemory
// tiene los 8 bancos de RAM, las ROMs del modelo, los puertos de paginación
// (0x7FFD del 128K y 0x1FFD del +2A/+3) y, a partir de ellos, qué página se
// ve en cada uno de los cuatro trozos de 16 KB, cuáles tienen contención y
// cuál es la pantalla.
//
// Los periféricos que tapan un trozo (Multiface, Interface 1 y 2, LEC...)
// lo hacen después con MapSlot; de momento siguen en CSpectrum.
//
// Lo que se usa en cada acceso del Z80 (ReadPage, WritePage, IsContended)
// es inline: CMemory va dentro de CSpectrum y el compilador lo deja como
// estaba, un puntero por trozo.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_zxmemory_h
#define _zx_zxmemory_h

#include "machine.h"
#include <circle/types.h>

class CMemory
{
	friend class CSnapshot;		// los bancos y los puertos en los snapshots

public:
	CMemory (void);

	// Al elegir el modelo: sus ROMs (pROM = la del modelo, o 0 para las de
	// dentro del emulador en el 48K y el 128K)
	void SetModel (TModel Model, const TMachineTiming *pTiming, const u8 *pROM);
	// Reset: RAM a 0 y puertos de paginación a 0 (falta UpdatePaging)
	void Reset (void);

	// Calcula las páginas de los cuatro trozos según el modelo y los puertos
	// (sin periféricos)
	void UpdatePaging (void);
	// Un periférico tapa el trozo nSlot (sin contención; pWrite puede ser
	// GetROMSink () si no se escribe)
	void MapSlot (unsigned nSlot, const u8 *pRead, u8 *pWrite)
	{
		m_pReadPage[nSlot] = pRead;
		m_pWritePage[nSlot] = pWrite;
		m_bContendedRAM[nSlot] = FALSE;
		m_bContendedIO[nSlot] = FALSE;
	}

	// Cada acceso del Z80
	const u8 *ReadPage (unsigned nSlot) const	{ return m_pReadPage[nSlot]; }
	u8 *WritePage (unsigned nSlot) const		{ return m_pWritePage[nSlot]; }
	boolean IsContended (unsigned nSlot) const	{ return m_bContendedRAM[nSlot]; }
	boolean IsContendedIO (unsigned nSlot) const	{ return m_bContendedIO[nSlot]; }

	u8 *Bank (unsigned nBank)			{ return m_RAM[nBank]; }
	const u8 *Bank (unsigned nBank) const		{ return m_RAM[nBank]; }
	u8 *GetROMSink (void)				{ return m_ROMSink; }
	const u8 *GetBasicROM (void) const		{ return m_pBasicROM; }	// la de LD-BYTES
	const u8 *GetScreenPage (void) const		{ return m_pScreenPage; }

	// Puertos de paginación (después de cambiarlos, UpdatePaging)
	u8 GetPort7FFD (void) const			{ return m_nPort7FFD; }
	void SetPort7FFD (u8 nValue)			{ m_nPort7FFD = nValue; }
	u8 GetPort1FFD (void) const			{ return m_nPort1FFD; }
	void SetPort1FFD (u8 nValue)			{ m_nPort1FFD = nValue; }
	boolean IsPagingLocked (void) const		{ return (m_nPort7FFD & 0x20) != 0; }
	// +2A/+3 en los modos "todo RAM" (0x1FFD bit 0)
	boolean IsSpecialPaging (void) const
		{ return m_pTiming->CodeModel == CodeModelPlus3 && (m_nPort1FFD & 0x01); }
	TModel GetModel (void) const			{ return m_Model; }
	TCodeModel GetCodeModel (void) const		{ return m_pTiming->CodeModel; }

	// La ROM que se ve abajo (0-1 en el 128K, 0-3 en el +2A/+3; 0 en el 48K)
	unsigned GetROMNumber (void) const;

private:
	TModel m_Model;
	const TMachineTiming *m_pTiming;

	u8 m_RAM[8][0x4000];
	u8 m_ROMSink[0x4000];		// destino de las escrituras en ROM
	u8 m_NoRAM[0x4000];		// memoria inexistente (16K): se lee 0xFF
	const u8 *m_ROM[4];
	const u8 *m_pBasicROM;		// ROM donde vive LD-BYTES (para el trap de carga)

	const u8 *m_pReadPage[4];
	u8 *m_pWritePage[4];
	boolean m_bContendedRAM[4];
	boolean m_bContendedIO[4];
	const u8 *m_pScreenPage;

	u8 m_nPort7FFD;
	u8 m_nPort1FFD;
};

#endif
