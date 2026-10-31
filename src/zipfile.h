//
// zipfile.h - Lector de ficheros ZIP (solo lectura)
//
// Lee el directorio central de un ZIP que ya está en memoria y extrae sus
// entradas: sin comprimir (método 0) o con deflate (método 8, el de casi
// todos), comprobando el CRC-32. No admite ZIP cifrados ni ZIP64.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_zipfile_h
#define _zx_zipfile_h

#include <circle/types.h>

#define ZIP_MAX_ENTRIES		64
#define ZIP_NAME_LEN		96

class CZipFile
{
public:
	CZipFile (void);

	// Lee el directorio central; pData debe seguir existiendo mientras se usa
	boolean Open (const u8 *pData, unsigned nSize);

	unsigned GetCount (void) const		{ return m_nEntries; }
	const char *GetName (unsigned nEntry) const;	// con la ruta dentro del ZIP
	const char *GetBaseName (unsigned nEntry) const; // sin la ruta
	unsigned GetSize (unsigned nEntry) const;	// tamaño descomprimido
	boolean IsDirectory (unsigned nEntry) const;

	// Extrae una entrada; devuelve los bytes o -1 (error, CRC, no cabe...)
	int Extract (unsigned nEntry, u8 *pDest, unsigned nDestSize) const;

	static u32 CRC32 (const u8 *pData, unsigned nLength);

private:
	struct TEntry
	{
		char Name[ZIP_NAME_LEN];
		u16 nMethod;
		u16 nFlags;
		u32 nCRC;
		u32 nCompressed;
		u32 nSize;
		u32 nLocalOffset;
	};

	const u8 *m_pData;
	unsigned m_nSize;
	TEntry m_Entry[ZIP_MAX_ENTRIES];
	unsigned m_nEntries;
};

#endif
