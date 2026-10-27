//
// interface1.h - ZX Interface 1 con hasta 8 microdrives
//
// Portado a C++ desde JSpeccy (machine/Interface1.java, utilities/Microdrive.java)
// (c) José Luis Sánchez, GPLv3
//
// La cinta del microdrive se modela como una secuencia de bytes, cada uno marcado
// como dato o como hueco (gap). Los cartuchos se cargan y graban en formato .MDR
// (el de Fuse y Spectaculator: 254 sectores de 15 + 528 bytes y un byte final de
// protección contra escritura).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_interface1_h
#define _zx_interface1_h

#include <circle/types.h>
#include "zxnet.h"

#define MDR_MAX_SECTORS		254
#define MDR_HEADER_SIZE		15
#define MDR_RECORD_SIZE		528
#define MDR_FILE_SIZE		(MDR_MAX_SECTORS * (MDR_HEADER_SIZE + MDR_RECORD_SIZE) + 1)
#define MICRODRIVES		8

class CMicrodrive
{
public:
	CMicrodrive (void);
	~CMicrodrive (void);

	boolean InsertMDR (const u8 *pData, unsigned nSize);
	boolean InsertBlank (unsigned nSectors = 180);	// sin formatear (180 = JSpeccy)
	void Eject (void);

	boolean IsCartridge (void) const	{ return m_pData != 0; }
	boolean IsModified (void) const		{ return m_bModified; }
	void ClearModified (void)		{ m_bModified = FALSE; }
	boolean IsWriteProtected (void) const	{ return m_bWriteProtected; }
	void SetWriteProtected (boolean bOn)	{ m_bWriteProtected = bOn; m_bModified = TRUE; }

	// Convierte la cinta a .MDR; devuelve el tamaño (0 si no hay sectores)
	unsigned ExportMDR (u8 *pBuffer, unsigned nBufferSize) const;

	// .MDV (formato "MDVT" de JSpeccy): la cinta en bruto, datos y huecos. Sirve
	// para cualquier formato, también el del CP/M de LEC, que no usa los sectores
	// de Sinclair y por tanto no cabe en un .MDR.
	boolean InsertMDV (const u8 *pData, unsigned nSize);
	unsigned ExportMDV (u8 *pBuffer, unsigned nBufferSize) const;
	unsigned GetMDVSize (void) const;	// tamaño máximo de ExportMDV

	// TRUE si todo lo grabado son sectores de Sinclair (se puede guardar en .MDR)
	boolean IsSinclairFormat (void) const;

	// Acceso desde el Interface 1. nNow = t-states absolutos.
	void Selected (void);
	u8 ReadStatus (void);
	u8 ReadData (void);
	void WriteData (u8 nValue);
	void WriteControl (u8 nValue, u64 nNow);

private:
	boolean Allocate (unsigned nLength);
	void Advance (void)	{ if (++m_nPos >= m_nLength) m_nPos = 0; }
	boolean IsPreamble (unsigned nPos) const;
	unsigned CountSectors (void) const;

private:
	u8 *m_pData;		// bytes de la cinta
	u8 *m_pGap;		// 0 = dato, distinto de 0 = hueco
	unsigned m_nLength;
	unsigned m_nPos;
	u8 m_nLastData;
	boolean m_bModified;
	boolean m_bWriteProtected;
	boolean m_bWrGapPending;
	u64 m_nStartGap;
};

// Cola de bytes (RS232 y red del Interface 1)
class CByteQueue
{
public:
	CByteQueue (void) : m_nHead (0), m_nTail (0) {}
	void Clear (void)		{ m_nHead = m_nTail = 0; }
	boolean IsEmpty (void) const	{ return m_nHead == m_nTail; }
	boolean Put (u8 n)
	{
		unsigned nNext = (m_nTail + 1) % sizeof m_Data;
		if (nNext == m_nHead)
		{
			return FALSE;		// llena
		}
		m_Data[m_nTail] = n;
		m_nTail = nNext;
		return TRUE;
	}
	boolean Get (u8 *pn)
	{
		if (m_nHead == m_nTail)
		{
			return FALSE;
		}
		*pn = m_Data[m_nHead];
		m_nHead = (m_nHead + 1) % sizeof m_Data;
		return TRUE;
	}
private:
	u8 m_Data[4096];
	unsigned m_nHead, m_nTail;
};

// RS232 y red (ZX Net) del Interface 1, como en Fuse (peripherals/if1.c, de
// Gergely Szasz): la ROM del IF1 manda y lee los bits uno a uno por el puerto
// 0xF7 al ritmo de la velocidad elegida, así que cada OUT/IN es un bit y se
// pueden rehacer los bytes contando accesos, sin medir tiempos.
//   - RS232 (COMMS DATA = 1): 0xF7 bit 0 = salida (RX DATA); bit 7 al leer =
//     entrada (TX DATA); CTS en el bit 4 de 0xEF. Se entregan bytes enteros.
//   - Red (COMMS DATA = 0): ZX Net, ver zxnet.h.
class CInterface1
{
public:
	CInterface1 (void);

	void Reset (void);

	u8 ReadControlPort (void);
	u8 ReadDataPort (void);			// cuesta MDR_BYTE_TSTATES
	u8 ReadLanPort (u64 nNow);
	void WriteControlPort (u8 nValue, u64 nNow);
	void WriteDataPort (u8 nValue);		// cuesta MDR_BYTE_TSTATES
	void WriteLanPort (u8 nValue, u64 nNow);
	CZXNet *GetZXNet (void)			{ return &m_ZXNet; }

	// Bytes que salen y entran por el RS232 y por la red
	CByteQueue *GetRS232Out (void)		{ return &m_RS232Out; }
	CByteQueue *GetRS232In (void)		{ return &m_RS232In; }


	CMicrodrive *GetDrive (unsigned nDrive)	{ return nDrive < MICRODRIVES ? &m_Drive[nDrive] : 0; }
	int GetRunningDrive (void) const;	// -1 si ninguno

private:
	CMicrodrive m_Drive[MICRODRIVES];
	u8 m_nFlipFlop;
	unsigned m_nSelected;
	boolean m_bCommsClk;

	// RS232 y red
	boolean m_bCommsData;
	boolean m_bCTS;
	int m_nCountOut;
	u8 m_nDataOut;
	int m_nCountIn;
	u8 m_nDataIn;
	boolean m_bTX;			// línea TX DATA hacia el Spectrum
	CByteQueue m_RS232Out, m_RS232In;
	CZXNet m_ZXNet;
};

#define MDR_BYTE_TSTATES	168	// 12 us por bit, 8 bits, a 3,5 MHz

#endif
