//
// zipfile.cpp - Lector de ficheros ZIP (solo lectura)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "zipfile.h"
#include "inflate.h"
#include <circle/util.h>

#define SIG_LOCAL	0x04034B50
#define SIG_CENTRAL	0x02014B50
#define SIG_END		0x06054B50

static u16 Get16 (const u8 *p)	{ return p[0] | (p[1] << 8); }
static u32 Get32 (const u8 *p)	{ return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32) p[3] << 24); }

CZipFile::CZipFile (void)
:	m_pData (0),
	m_nSize (0),
	m_nEntries (0)
{
}

boolean CZipFile::Open (const u8 *pData, unsigned nSize)
{
	m_pData = pData;
	m_nSize = nSize;
	m_nEntries = 0;

	if (pData == 0 || nSize < 22)
	{
		return FALSE;
	}

	// fin del directorio central: al final, tras un comentario de hasta 64K
	int nEnd = -1;
	unsigned nLowest = nSize > 22 + 0xFFFF ? nSize - 22 - 0xFFFF : 0;
	for (unsigned i = nSize - 22; ; i--)
	{
		if (Get32 (pData + i) == SIG_END)
		{
			nEnd = (int) i;
			break;
		}
		if (i == nLowest)
		{
			break;
		}
	}
	if (nEnd < 0)
	{
		return FALSE;
	}

	unsigned nCount = Get16 (pData + nEnd + 10);
	u32 nDirSize = Get32 (pData + nEnd + 12);
	u32 nDirOffset = Get32 (pData + nEnd + 16);
	if (nDirOffset > nSize || nDirSize > nSize - nDirOffset)
	{
		return FALSE;
	}

	unsigned nPos = nDirOffset;
	for (unsigned n = 0; n < nCount && m_nEntries < ZIP_MAX_ENTRIES; n++)
	{
		if (nPos + 46 > nSize || Get32 (pData + nPos) != SIG_CENTRAL)
		{
			break;
		}

		unsigned nNameLen = Get16 (pData + nPos + 28);
		unsigned nExtraLen = Get16 (pData + nPos + 30);
		unsigned nCommentLen = Get16 (pData + nPos + 32);
		if (nPos + 46 + nNameLen > nSize)
		{
			break;
		}

		TEntry &E = m_Entry[m_nEntries];
		E.nFlags = Get16 (pData + nPos + 8);
		E.nMethod = Get16 (pData + nPos + 10);
		E.nCRC = Get32 (pData + nPos + 16);
		E.nCompressed = Get32 (pData + nPos + 20);
		E.nSize = Get32 (pData + nPos + 24);
		E.nLocalOffset = Get32 (pData + nPos + 42);

		unsigned nCopy = nNameLen < ZIP_NAME_LEN - 1 ? nNameLen : ZIP_NAME_LEN - 1;
		memcpy (E.Name, pData + nPos + 46, nCopy);
		E.Name[nCopy] = '\0';

		m_nEntries++;
		nPos += 46 + nNameLen + nExtraLen + nCommentLen;
	}

	return m_nEntries > 0;
}

const char *CZipFile::GetName (unsigned nEntry) const
{
	return nEntry < m_nEntries ? m_Entry[nEntry].Name : "";
}

const char *CZipFile::GetBaseName (unsigned nEntry) const
{
	const char *pName = GetName (nEntry);
	const char *pBase = pName;
	for (const char *p = pName; *p != '\0'; p++)
	{
		if ((*p == '/' || *p == '\\') && p[1] != '\0')
		{
			pBase = p + 1;
		}
	}
	return pBase;
}

unsigned CZipFile::GetSize (unsigned nEntry) const
{
	return nEntry < m_nEntries ? m_Entry[nEntry].nSize : 0;
}

boolean CZipFile::IsDirectory (unsigned nEntry) const
{
	const char *pName = GetName (nEntry);
	unsigned nLen = strlen (pName);
	return nLen == 0 || pName[nLen - 1] == '/' || pName[nLen - 1] == '\\';
}

int CZipFile::Extract (unsigned nEntry, u8 *pDest, unsigned nDestSize) const
{
	if (nEntry >= m_nEntries || pDest == 0)
	{
		return -1;
	}

	const TEntry &E = m_Entry[nEntry];
	if ((E.nFlags & 0x01) || E.nSize > nDestSize)		// cifrado, o no cabe
	{
		return -1;
	}

	unsigned nLocal = E.nLocalOffset;
	if (nLocal + 30 > m_nSize || Get32 (m_pData + nLocal) != SIG_LOCAL)
	{
		return -1;
	}
	unsigned nStart = nLocal + 30 + Get16 (m_pData + nLocal + 26) + Get16 (m_pData + nLocal + 28);
	if (nStart > m_nSize || E.nCompressed > m_nSize - nStart)
	{
		return -1;
	}

	int nLength;
	if (E.nMethod == 0)
	{
		if (E.nCompressed != E.nSize)
		{
			return -1;
		}
		memcpy (pDest, m_pData + nStart, E.nSize);
		nLength = (int) E.nSize;
	}
	else if (E.nMethod == 8)
	{
		nLength = RawInflate (m_pData + nStart, E.nCompressed, pDest, nDestSize);
	}
	else
	{
		return -1;					// método no admitido
	}

	if (nLength != (int) E.nSize || CRC32 (pDest, E.nSize) != E.nCRC)
	{
		return -1;
	}

	return nLength;
}

u32 CZipFile::CRC32 (const u8 *pData, unsigned nLength)
{
	static u32 Table[256];
	static boolean bTable = FALSE;
	if (!bTable)
	{
		for (u32 i = 0; i < 256; i++)
		{
			u32 c = i;
			for (unsigned k = 0; k < 8; k++)
			{
				c = (c & 1) ? 0xEDB88320 ^ (c >> 1) : c >> 1;
			}
			Table[i] = c;
		}
		bTable = TRUE;
	}

	u32 nCRC = 0xFFFFFFFF;
	for (unsigned i = 0; i < nLength; i++)
	{
		nCRC = Table[(nCRC ^ pData[i]) & 0xFF] ^ (nCRC >> 8);
	}
	return nCRC ^ 0xFFFFFFFF;
}
