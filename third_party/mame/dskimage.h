//
// dskimage.h - Imágenes de disco .DSK (CPCEMU "MV - CPC" y "EXTENDED CPC DSK")
//
// La lectura sigue a dsk_dsk.cpp de MAME (license:BSD-3-Clause, Olivier Galibert y
// otros) y la generación de la pista MFM a build_pc_track_mfm() de flopimg.cpp,
// también de MAME: hueco 4a, IAM, hueco 1 y, por cada sector, sincronismo,
// IDAM con CRC, hueco 2, DAM (normal o borrada) con datos y CRC, y hueco 3.
// Se ha reescrito sin la librería estándar de C++ (Circle no la enlaza).
//
// Al grabar, las pistas que el Spectrum ha escrito se decodifican de nuevo desde
// el MFM y la imagen se guarda siempre en formato extendido.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef ZX_DSKIMAGE_H
#define ZX_DSKIMAGE_H

#include "emu.h"

#define DSK_MAX_TRACKS		84
#define DSK_MAX_SIDES		2
#define DSK_MAX_SECTORS		29

struct TDskSector
{
	u8 C, H, R, N;		// identificación (cilindro, cara, sector, tamaño)
	u8 ST1, ST2;		// estado del FDC guardado en la imagen
	u16 nLength;		// bytes de datos guardados
	u8 *pData;		// 0 si el sector no tiene datos (ST1 bit 2)
	u16 nWeakFrom = 256;	// sector débil: byte desde el que cambia en cada lectura
	u16 nWeakLen = 0;	// cuántos bytes (0 = hasta el final)
};

struct TTrackLayout		// reparto de una pista al construirla (ver GetTrackLayout)
{
	unsigned nGap4a;
	bool bIAM;
	unsigned nGap1;
	unsigned nGap3;
	unsigned nCells;
};

struct TDskTrack
{
	bool bPresent;
	u8 nGap3;
	u8 nFiller;
	u8 nSectors;
	TDskSector Sector[DSK_MAX_SECTORS];
};

class CDskImage
{
public:
	CDskImage (void);
	~CDskImage (void);

	bool Load (const u8 *pData, unsigned nSize);
	bool CreateBlank3 (void);		// disco del +3 formateado (40 pistas, 9x512, una cara)
	bool CreateBlank720 (void);		// 3,5" de 720K para +3DOS (80 pistas, dos caras, 9x512)

	// Guarda en formato extendido. Devuelve el tamaño (0 si no cabe).
	unsigned Save (u8 *pBuffer, unsigned nBufferSize) const;
	unsigned GetSaveSize (void) const;	// tamaño máximo de Save()

	int GetTracks (void) const	{ return m_nTracks; }
	int GetSides (void) const	{ return m_nSides; }
	bool IsWriteProtected (void) const	{ return m_bWriteProtected; }
	void SetWriteProtected (bool bOn)	{ m_bWriteProtected = bOn; }

	// Celdas MFM de una pista (1 bit por celda, el bit 7 del primer byte es la
	// primera celda). Devuelve false si la pista no existe (queda sin flujo).
	// Celdas necesarias para una pista: 100.000 (2 us a 300 rpm) o más si los
	// sectores no caben, como hace MAME (la pista "gira" un poco más deprisa;
	// lo usan protecciones con sectores de 6 KB, como la de Turrican)
	static void GetTrackLayout (const TDskTrack &T, TTrackLayout *pLayout);
	unsigned GetTrackCells (int nCyl, int nSide) const;

	bool BuildTrack (int nCyl, int nSide, u8 *pCells, unsigned nCells,
			 unsigned *pWeakStart = 0, unsigned *pWeakEnd = 0, unsigned *pnWeak = 0) const;

	// Sectores "débiles" (como en MAME): con error de CRC en los datos y sin marca
	// de borrado. Desde el byte 256 de los datos la superficie no tiene flujo
	// definido y cada lectura da valores distintos (lo comprueba Speedlock).

	// Decodifica una pista MFM escrita por el Spectrum y la guarda como sectores
	void DecodeTrack (int nCyl, int nSide, const u8 *pCells, unsigned nCells);

private:
	static void DetectSpeedlock (TDskTrack &T);
	void CompleteLongSector (TDskTrack &T);	// sector largo cortado dentro de otro sector
	void Clear (void);

	int m_nTracks, m_nSides;
	bool m_bWriteProtected;
	TDskTrack m_Track[DSK_MAX_TRACKS][DSK_MAX_SIDES];
};

#endif
