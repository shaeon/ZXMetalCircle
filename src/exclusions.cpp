//
// exclusions.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "exclusions.h"
#include <fatfs/ff.h>
#include <circle/util.h>

CExclusions *CExclusions::s_pThis = 0;

static char Lower (char c)
{
	return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

CExclusions::CExclusions (void)
:	m_pFirst (0)
{
	s_pThis = this;
}

CExclusions::~CExclusions (void)
{
	Clear ();
	s_pThis = 0;
}

void CExclusions::Clear (void)
{
	while (m_pFirst != 0)
	{
		TEntry *p = m_pFirst;
		m_pFirst = p->pNext;
		delete [] p->pPattern;
		delete p;
	}
}

void CExclusions::AddLine (const char *pLine)
{
	// sin comentario ni espacios a los lados; \ se trata como /
	char Buf[200];
	unsigned n = 0;
	while (*pLine == ' ' || *pLine == '\t')
	{
		pLine++;
	}
	for (; *pLine != '\0' && *pLine != '#' && *pLine != '\r' && *pLine != '\n'
	       && n < sizeof Buf - 1; pLine++)
	{
		Buf[n++] = *pLine == '\\' ? '/' : *pLine;
	}
	while (n > 0 && (Buf[n - 1] == ' ' || Buf[n - 1] == '\t'))
	{
		n--;
	}
	Buf[n] = '\0';

	boolean bDir = FALSE;
	while (n > 0 && Buf[n - 1] == '/')	// termina en / o \: carpeta
	{
		Buf[--n] = '\0';
		bDir = TRUE;
	}
	if (n == 0)
	{
		return;
	}

	TEntry *pEntry = new TEntry;
	pEntry->bDir = bDir;
	pEntry->bRooted = Buf[0] == '/';
	pEntry->pPattern = new char[n + 1];
	strcpy (pEntry->pPattern, Buf);
	pEntry->pNext = m_pFirst;
	m_pFirst = pEntry;
}

void CExclusions::Load (const char *pPath)
{
	Clear ();

	FIL File;
	if (f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		return;
	}

	char Line[200];
	while (f_gets (Line, sizeof Line, &File) != 0)
	{
		AddLine (Line);
	}

	f_close (&File);
}

boolean CExclusions::Match (const char *pPattern, const char *pText)
{
	// comparación con vuelta atrás para el último *
	const char *pStar = 0;
	const char *pResume = 0;
	while (*pText != '\0')
	{
		if (*pPattern == '*')
		{
			pStar = ++pPattern;
			pResume = pText;
		}
		else if (*pPattern == '?' || Lower (*pPattern) == Lower (*pText))
		{
			pPattern++;
			pText++;
		}
		else if (pStar != 0)
		{
			pPattern = pStar;
			pText = ++pResume;
		}
		else
		{
			return FALSE;
		}
	}
	while (*pPattern == '*')
	{
		pPattern++;
	}
	return *pPattern == '\0';
}

boolean CExclusions::IsExcluded (const char *pDir, const char *pName, boolean bDir) const
{
	if (m_pFirst == 0)
	{
		return FALSE;
	}

	// ruta completa desde la raíz: "/juegos/viejos/fichero"
	char Path[300];
	const char *pRel = pDir;
	if (strncmp (pRel, "SD:", 3) == 0)
	{
		pRel += 3;
	}
	unsigned n = 0;
	Path[n++] = '/';
	for (; *pRel != '\0' && n < sizeof Path - 2; pRel++)
	{
		char c = *pRel == '\\' ? '/' : *pRel;
		if (c == '/' && Path[n - 1] == '/')
		{
			continue;
		}
		Path[n++] = c;
	}
	if (Path[n - 1] != '/')
	{
		Path[n++] = '/';
	}
	for (; *pName != '\0' && n < sizeof Path - 1; pName++)
	{
		Path[n++] = *pName;
	}
	Path[n] = '\0';

	const char *pBase = Path;		// nombre (lo que sigue a la última /)
	for (const char *p = Path; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pBase = p + 1;
		}
	}

	for (const TEntry *p = m_pFirst; p != 0; p = p->pNext)
	{
		if (p->bDir != bDir)
		{
			continue;
		}
		if (p->bRooted ? Match (p->pPattern, Path) : Match (p->pPattern, pBase))
		{
			return TRUE;
		}
	}
	return FALSE;
}
