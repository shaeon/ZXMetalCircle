//
// harddisk.h - Imagen de disco duro / tarjeta en un fichero de la SD de la Pi
//
// Formatos: .img/.raw (en bruto), .vhd (en bruto con o sin el pie "conectix"
// de 512 bytes de los VHD fijos; los de MiST suelen ser una .img renombrada) y
// .hdf (cabecera RS-IDE; no se admiten los HDF de 256 bytes por sector).
//
// Para que leer sectores sueltos de un fichero de cientos de MB sea rápido se
// usa el "fast seek" de FatFs, que guarda en memoria el mapa de bloques del
// fichero. Las escrituras van directamente al fichero; f_sync () se hace al
// cabo de un momento sin escribir (Flush ()) y al desmontar.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_harddisk_h
#define _zx_harddisk_h

#include "sdcard.h"
#include <fatfs/ff.h>
#include <circle/types.h>

class CHardDiskImage : public CBlockDevice
{
public:
	CHardDiskImage (void);
	~CHardDiskImage (void);

	// Devuelve 0 si todo va bien o un texto con el error
	const char *Open (const char *pPath);
	void Close (void);
	boolean IsOpen (void) const		{ return m_bOpen; }

	u32 GetSectors (void) const		{ return m_nSectors; }
	boolean ReadSector (u32 nSector, u8 *pBuffer);
	boolean WriteSector (u32 nSector, const u8 *pBuffer);

	void Flush (void);			// confirma las escrituras pendientes
	boolean IsDirty (void) const		{ return m_bDirty; }
	// cuántas veces se ha escrito desde que se abrió (para saber si hay algo
	// nuevo que copiar de la imagen a la carpeta de esxDOS)
	u32 GetWriteCount (void) const		{ return m_nWrites; }
	const char *GetFormatName (void) const	{ return m_pFormat; }

private:
	FIL m_File;
	boolean m_bOpen;
	boolean m_bDirty;
	u32 m_nWrites;
	DWORD *m_pLinkMap;		// mapa de bloques para el fast seek
	u32 m_nDataOffset;		// bytes antes del sector 0 (cabecera HDF)
	u32 m_nSectors;
	const char *m_pFormat;
};

#endif
