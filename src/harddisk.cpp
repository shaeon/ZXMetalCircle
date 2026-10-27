//
// harddisk.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "harddisk.h"
#include "i18n.h"
#include <circle/util.h>

CHardDiskImage::CHardDiskImage (void)
:	m_bOpen (FALSE),
	m_bDirty (FALSE),
	m_nWrites (0),
	m_pLinkMap (0),
	m_nDataOffset (0),
	m_nSectors (0),
	m_pFormat ("")
{
}

CHardDiskImage::~CHardDiskImage (void)
{
	Close ();
}

const char *CHardDiskImage::Open (const char *pPath)
{
	Close ();

	if (f_open (&m_File, pPath, FA_READ | FA_WRITE | FA_OPEN_EXISTING) != FR_OK)
	{
		return N_("No se pudo abrir la imagen");
	}

	FSIZE_t nSize = f_size (&m_File);
	m_nDataOffset = 0;
	m_pFormat = "imagen";

	u8 Header[512];
	UINT nRead = 0;
	if (nSize < 512 || f_read (&m_File, Header, 512, &nRead) != FR_OK || nRead != 512)
	{
		f_close (&m_File);
		return N_("Imagen demasiado pequeña");
	}

	// HDF (RS-IDE): cabecera de 128 bytes o la indicada en el byte 9
	if (memcmp (Header, "RS-IDE\x1A", 7) == 0)
	{
		if (Header[8] & 0x01)
		{
			f_close (&m_File);
			return N_("HDF de 256 bytes por sector: no admitido");
		}
		m_nDataOffset = Header[7] >= 0x11 ? (Header[9] | (Header[10] << 8)) : 128;
		if (m_nDataOffset < 128 || m_nDataOffset > 4096)
		{
			m_nDataOffset = 128;
		}
		m_pFormat = "HDF";
	}

	FSIZE_t nData = nSize - m_nDataOffset;

	// VHD fijo: pie de 512 bytes que empieza por "conectix"
	u8 Footer[8];
	if (   m_nDataOffset == 0 && nSize >= 1024
	    && f_lseek (&m_File, nSize - 512) == FR_OK
	    && f_read (&m_File, Footer, 8, &nRead) == FR_OK && nRead == 8
	    && memcmp (Footer, "conectix", 8) == 0)
	{
		nData -= 512;
		m_pFormat = "VHD";
	}

	m_nSectors = (u32) (nData / 512);

	// Mapa de bloques para el fast seek (se amplía si no cabe)
	for (unsigned nEntries = 256; nEntries <= 65536; nEntries *= 4)
	{
		m_pLinkMap = new DWORD[nEntries];
		if (m_pLinkMap == 0)
		{
			break;
		}
		m_pLinkMap[0] = nEntries;
		m_File.cltbl = m_pLinkMap;
		FRESULT Result = f_lseek (&m_File, CREATE_LINKMAP);
		if (Result == FR_OK)
		{
			break;
		}
		m_File.cltbl = 0;
		delete [] m_pLinkMap;
		m_pLinkMap = 0;
		if (Result != FR_NOT_ENOUGH_CORE)
		{
			break;
		}
	}

	m_bOpen = TRUE;
	m_bDirty = FALSE;

	return 0;
}

void CHardDiskImage::Close (void)
{
	if (!m_bOpen)
	{
		return;
	}

	Flush ();
	m_File.cltbl = 0;
	f_close (&m_File);
	delete [] m_pLinkMap;
	m_pLinkMap = 0;
	m_bOpen = FALSE;
	m_nSectors = 0;
}

boolean CHardDiskImage::ReadSector (u32 nSector, u8 *pBuffer)
{
	UINT nRead = 0;
	return    m_bOpen && nSector < m_nSectors
	       && f_lseek (&m_File, (FSIZE_t) nSector * 512 + m_nDataOffset) == FR_OK
	       && f_read (&m_File, pBuffer, 512, &nRead) == FR_OK && nRead == 512;
}

boolean CHardDiskImage::WriteSector (u32 nSector, const u8 *pBuffer)
{
	UINT nWritten = 0;
	boolean bOK =    m_bOpen && nSector < m_nSectors
		      && f_lseek (&m_File, (FSIZE_t) nSector * 512 + m_nDataOffset) == FR_OK
		      && f_write (&m_File, pBuffer, 512, &nWritten) == FR_OK && nWritten == 512;
	if (bOK)
	{
		m_nWrites++;
	}
	if (bOK)
	{
		m_bDirty = TRUE;
	}
	return bOK;
}

void CHardDiskImage::Flush (void)
{
	if (m_bOpen && m_bDirty)
	{
		f_sync (&m_File);
		m_bDirty = FALSE;
	}
}
