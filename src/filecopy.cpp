//
// filecopy.cpp
//
// Copiar un fichero de la SD a trozos (ver filecopy.h)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "filecopy.h"
#include "i18n.h"
#include <circle/util.h>
#include <string.h>

#define COPY_BUFFER_SIZE	(64 * 1024)

static u8 s_Buffer[COPY_BUFFER_SIZE];	// uno para todos: las copias van de una en una

CFileCopy::CFileCopy (void)
:	m_bActive (FALSE),
	m_bReplace (FALSE),
	m_nDone (0),
	m_nTotal (0),
	m_pError ("")
{
	m_TempPath[0] = m_FinalPath[0] = '\0';
}

CFileCopy::~CFileCopy (void)
{
	Cancel ();
}

// La carpeta de una ruta, con su barra ("SD:/juegos/")
static void DirOf (const char *pPath, char *pDir, unsigned nSize)
{
	strncpy (pDir, pPath, nSize - 1);
	pDir[nSize - 1] = '\0';
	char *pSlash = 0;
	for (char *p = pDir; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pSlash = p;
		}
	}
	if (pSlash != 0)
	{
		pSlash[1] = '\0';
	}
}

const char *CFileCopy::Start (const char *pFrom, const char *pTo, const char *pTemp, boolean bReplace)
{
	Cancel ();
	m_pError = "";

	if (f_stat (pFrom, &m_Info) != FR_OK || (m_Info.fattrib & AM_DIR))
	{
		return m_pError = N_("No existe: %s");
	}

	// ¿cabe? (en la SD de la Pi: un solo volumen)
	DWORD nFreeClusters;
	FATFS *pFS;
	char Volume[8];
	unsigned n = 0;
	while (pTo[n] != '\0' && pTo[n] != ':' && n < sizeof Volume - 2)
	{
		Volume[n] = pTo[n];
		n++;
	}
	Volume[n] = ':';
	Volume[n + 1] = '\0';
	if (   f_getfree (Volume, &nFreeClusters, &pFS) == FR_OK
	    && (u64) nFreeClusters * pFS->csize * FF_MAX_SS < (u64) m_Info.fsize + 64 * 1024)
	{
		return m_pError = N_("No cabe en la tarjeta: %s");
	}

	char Dir[sizeof m_TempPath];
	DirOf (pTo, Dir, sizeof Dir);
	if (strlen (Dir) + strlen (pTemp) + 1 > sizeof m_TempPath || strlen (pTo) + 1 > sizeof m_FinalPath)
	{
		return m_pError = N_("No se pudo copiar %s");
	}
	strcpy (m_TempPath, Dir);
	strcat (m_TempPath, pTemp);
	strcpy (m_FinalPath, pTo);

	if (f_open (&m_From, pFrom, FA_READ) != FR_OK)
	{
		return m_pError = N_("No se pudo copiar %s");
	}
	if (f_open (&m_To, m_TempPath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
	{
		f_close (&m_From);
		return m_pError = N_("No se pudo copiar %s");
	}

	m_bActive = TRUE;
	m_bReplace = bReplace;
	m_nDone = 0;
	m_nTotal = m_Info.fsize;
	return 0;
}

void CFileCopy::Close (void)
{
	if (m_bActive)
	{
		f_close (&m_From);
		f_close (&m_To);
		m_bActive = FALSE;
	}
}

int CFileCopy::Step (unsigned nMaxBytes)
{
	if (!m_bActive)
	{
		return -1;
	}

	unsigned nCopied = 0;
	while (nCopied < nMaxBytes)
	{
		UINT nRead = 0, nWritten = 0;
		if (f_read (&m_From, s_Buffer, sizeof s_Buffer, &nRead) != FR_OK)
		{
			m_pError = N_("No se pudo leer %s");
			Cancel ();
			return -1;
		}
		if (nRead == 0)
		{
			break;			// terminado
		}
		if (f_write (&m_To, s_Buffer, nRead, &nWritten) != FR_OK || nWritten != nRead)
		{
			m_pError = N_("No se pudo escribir %s (¿tarjeta llena?)");
			Cancel ();
			return -1;
		}
		m_nDone += nRead;
		nCopied += nRead;
	}

	if (m_nDone < m_nTotal)
	{
		return 0;
	}

	// terminado: el nombre de verdad (y la fecha del original)
	Close ();
	if (m_bReplace)
	{
		f_unlink (m_FinalPath);
	}
	if (f_rename (m_TempPath, m_FinalPath) != FR_OK)
	{
		f_unlink (m_TempPath);
		m_pError = N_("No se pudo copiar %s");
		return -1;
	}
	FILINFO Time;
	Time.fdate = m_Info.fdate;
	Time.ftime = m_Info.ftime;
	f_utime (m_FinalPath, &Time);
	return 1;
}

void CFileCopy::Cancel (void)
{
	if (m_bActive)
	{
		Close ();
		f_unlink (m_TempPath);
	}
}

boolean CFileCopy::FreeName (const char *pDir, const char *pName, char *pOut, unsigned nOutSize)
{
	// el nombre y la extensión (las carpetas, sin extensión)
	unsigned nLen = strlen (pName);
	unsigned nDot = nLen;
	for (unsigned i = nLen; i-- > 1; )
	{
		if (pName[i] == '.')
		{
			nDot = i;
			break;
		}
	}

	size_t nDirLen = strlen (pDir);
	boolean bSlash = nDirLen > 0 && pDir[nDirLen - 1] == '/';
	for (unsigned nCopy = 2; nCopy < 100; nCopy++)
	{
		char Suffix[8] = " (";
		unsigned n = 2;
		if (nCopy >= 10)
		{
			Suffix[n++] = '0' + nCopy / 10;
		}
		Suffix[n++] = '0' + nCopy % 10;
		Suffix[n++] = ')';
		Suffix[n] = '\0';
		if (nLen + n + 1 > nOutSize)
		{
			return FALSE;
		}
		memcpy (pOut, pName, nDot);
		strcpy (pOut + nDot, Suffix);
		strcat (pOut, pName + nDot);

		char Path[512];
		if (nDirLen + strlen (pOut) + 2 > sizeof Path)
		{
			return FALSE;
		}
		strcpy (Path, pDir);
		if (!bSlash)
		{
			strcat (Path, "/");
		}
		strcat (Path, pOut);
		FILINFO Info;
		if (f_stat (Path, &Info) != FR_OK)
		{
			return TRUE;
		}
	}
	return FALSE;
}
