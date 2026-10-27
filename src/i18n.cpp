//
// i18n.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "i18n.h"
#include <fatfs/ff.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>

#define JSMN_STATIC
#include "../third_party/jsmn.h"

static const char From[] = "i18n";

CI18n *CI18n::s_pThis = 0;

const char *T (const char *pSpanish)
{
	CI18n *p = CI18n::Get ();
	return p != 0 ? p->Translate (pSpanish) : pSpanish;
}

CI18n::CI18n (void)
:	m_pText (0),
	m_pEntries (0),
	m_nEntries (0),
	m_nEmpty (0),
	m_nLanguages (0)
{
	strcpy (m_Current, "es");
	strcpy (m_Code[0], "es");
	strcpy (m_Name[0], "Español");
	m_nLanguages = 1;
	s_pThis = this;
}

CI18n::~CI18n (void)
{
	Clear ();
	s_pThis = 0;
}

void CI18n::Clear (void)
{
	delete [] m_pText;
	delete [] m_pEntries;
	m_pText = 0;
	m_pEntries = 0;
	m_nEntries = 0;
	m_nEmpty = 0;
}

u32 CI18n::Hash (const char *p)
{
	u32 h = 2166136261u;			// FNV-1a
	while (*p != '\0')
	{
		h ^= (u8) *p++;
		h *= 16777619u;
	}
	return h;
}

// Lee "_idioma" de un fichero (solo el principio)
static boolean ReadLanguageName (const char *pPath, char *pName, unsigned nSize)
{
	FIL File;
	if (f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		return FALSE;
	}
	char Buffer[512];
	UINT nRead = 0;
	f_read (&File, Buffer, sizeof Buffer - 1, &nRead);
	f_close (&File);
	Buffer[nRead] = '\0';

	const char *p = strstr (Buffer, "\"_idioma\"");
	if (p == 0)
	{
		return FALSE;
	}
	p = strchr (p + 9, '"');
	if (p == 0)
	{
		return FALSE;
	}
	p++;
	unsigned n = 0;
	while (*p != '\0' && *p != '"' && n < nSize - 1)
	{
		pName[n++] = *p++;
	}
	pName[n] = '\0';
	return n > 0;
}

void CI18n::ScanLanguages (void)
{
	m_nLanguages = 0;
	strcpy (m_Code[0], "es");
	strcpy (m_Name[0], "Español");
	m_nLanguages = 1;

	DIR Dir;
	FILINFO Info;
	if (f_opendir (&Dir, I18N_FOLDER) != FR_OK)
	{
		return;
	}
	while (   m_nLanguages < I18N_MAX_LANGUAGES
	       && f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
	{
		const char *pDot = strchr (Info.fname, '.');
		if (   (Info.fattrib & AM_DIR) || pDot == 0 || strcasecmp (pDot, ".json") != 0
		    || pDot - Info.fname < 2 || pDot - Info.fname > 5)
		{
			continue;
		}
		char Code[8];
		unsigned n = pDot - Info.fname;
		for (unsigned i = 0; i < n; i++)
		{
			char c = Info.fname[i];
			Code[i] = (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
		}
		Code[n] = '\0';
		if (strcmp (Code, "es") == 0)
		{
			continue;		// el español va dentro (es.json es de referencia)
		}

		CString Path;
		Path.Format ("%s/%s", I18N_FOLDER, Info.fname);
		char Name[32];
		if (!ReadLanguageName (Path, Name, sizeof Name))
		{
			strcpy (Name, Code);
		}
		strcpy (m_Code[m_nLanguages], Code);
		strcpy (m_Name[m_nLanguages], Name);
		m_nLanguages++;
	}
	f_closedir (&Dir);
}

// Copia una cadena JSON (sin las comillas) quitando los escapes
static unsigned Unescape (const char *pIn, unsigned nLen, char *pOut)
{
	unsigned n = 0;
	for (unsigned i = 0; i < nLen; i++)
	{
		char c = pIn[i];
		if (c != '\\' || i + 1 >= nLen)
		{
			pOut[n++] = c;
			continue;
		}
		c = pIn[++i];
		switch (c)
		{
		case 'n':	pOut[n++] = '\n';	break;
		case 't':	pOut[n++] = '\t';	break;
		case 'r':	pOut[n++] = '\r';	break;
		case 'u':
			if (i + 4 < nLen)
			{
				unsigned u = 0;
				for (unsigned k = 1; k <= 4; k++)
				{
					char h = pIn[i + k];
					u = u * 16 + (h >= '0' && h <= '9' ? h - '0'
						      : (h >= 'a' && h <= 'f' ? h - 'a' + 10 : h - 'A' + 10));
				}
				i += 4;
				if (u < 0x80)
				{
					pOut[n++] = (char) u;
				}
				else if (u < 0x800)
				{
					pOut[n++] = (char) (0xC0 | (u >> 6));
					pOut[n++] = (char) (0x80 | (u & 0x3F));
				}
				else
				{
					pOut[n++] = (char) (0xE0 | (u >> 12));
					pOut[n++] = (char) (0x80 | ((u >> 6) & 0x3F));
					pOut[n++] = (char) (0x80 | (u & 0x3F));
				}
			}
			break;
		default:	pOut[n++] = c;		break;	// \" \\ \/
		}
	}
	pOut[n++] = '\0';
	return n;
}

boolean CI18n::Load (const char *pPath)
{
	FIL File;
	if (f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		return FALSE;
	}
	unsigned nSize = f_size (&File);
	char *pJSON = new char[nSize + 1];
	UINT nRead = 0;
	f_read (&File, pJSON, nSize, &nRead);
	f_close (&File);
	pJSON[nRead] = '\0';

	// primero se cuentan los elementos
	jsmn_parser Parser;
	jsmn_init (&Parser);
	int nTokens = jsmn_parse (&Parser, pJSON, nRead, 0, 0);
	if (nTokens < 1)
	{
		delete [] pJSON;
		CLogger::Get ()->Write (From, LogWarning, "%s: JSON no valido", pPath);
		return FALSE;
	}
	jsmntok_t *pTok = new jsmntok_t[nTokens];
	jsmn_init (&Parser);
	jsmn_parse (&Parser, pJSON, nRead, pTok, nTokens);
	if (pTok[0].type != JSMN_OBJECT)
	{
		delete [] pTok;
		delete [] pJSON;
		return FALSE;
	}

	Clear ();
	m_pText = new char[nRead + 2];
	m_pEntries = new TEntry[pTok[0].size + 1];
	unsigned nText = 0;

	for (int i = 1; i + 1 < nTokens; i += 2)
	{
		jsmntok_t &Key = pTok[i];
		jsmntok_t &Value = pTok[i + 1];
		if (Key.type != JSMN_STRING || Value.type != JSMN_STRING)
		{
			continue;	// (no se esperan objetos ni listas)
		}
		if (pJSON[Key.start] == '_')
		{
			continue;	// cabecera: _idioma, _version
		}
		if (Value.end == Value.start)
		{
			m_nEmpty++;	// sin traducir: se verá el español
			continue;
		}
		TEntry &Entry = m_pEntries[m_nEntries++];
		Entry.pKey = m_pText + nText;
		nText += Unescape (pJSON + Key.start, Key.end - Key.start, m_pText + nText);
		Entry.pValue = m_pText + nText;
		nText += Unescape (pJSON + Value.start, Value.end - Value.start, m_pText + nText);
		Entry.nHash = Hash (Entry.pKey);
	}

	// ordenar por hash (inserción: unos cientos de textos)
	for (unsigned i = 1; i < m_nEntries; i++)
	{
		TEntry e = m_pEntries[i];
		unsigned j = i;
		while (j > 0 && m_pEntries[j - 1].nHash > e.nHash)
		{
			m_pEntries[j] = m_pEntries[j - 1];
			j--;
		}
		m_pEntries[j] = e;
	}

	delete [] pTok;
	delete [] pJSON;
	return TRUE;
}

boolean CI18n::SetLanguage (const char *pCode)
{
	if (pCode == 0 || *pCode == '\0' || strcasecmp (pCode, "es") == 0)
	{
		Clear ();
		strcpy (m_Current, "es");
		return TRUE;
	}

	CString Path;
	Path.Format ("%s/%s.json", I18N_FOLDER, pCode);
	if (!Load (Path))
	{
		CLogger::Get ()->Write (From, LogWarning, "no hay %s: se queda en espanol", (const char *) Path);
		Clear ();
		strcpy (m_Current, "es");
		return FALSE;
	}

	strncpy (m_Current, pCode, sizeof m_Current - 1);
	m_Current[sizeof m_Current - 1] = '\0';
	CLogger::Get ()->Write (From, LogNotice, "idioma %s: %u textos traducidos, %u sin traducir",
				m_Current, m_nEntries, m_nEmpty);
	return TRUE;
}

const char *CI18n::Translate (const char *pSpanish) const
{
	if (m_nEntries == 0 || pSpanish == 0)
	{
		return pSpanish;
	}

	u32 h = Hash (pSpanish);
	unsigned lo = 0, hi = m_nEntries;
	while (lo < hi)
	{
		unsigned mid = (lo + hi) / 2;
		if (m_pEntries[mid].nHash < h)
		{
			lo = mid + 1;
		}
		else
		{
			hi = mid;
		}
	}
	for (; lo < m_nEntries && m_pEntries[lo].nHash == h; lo++)
	{
		if (strcmp (m_pEntries[lo].pKey, pSpanish) == 0)
		{
			return m_pEntries[lo].pValue;
		}
	}
	return pSpanish;
}

////////////////////////////////////////////////////////////////////////////////

static unsigned PutUTF8 (unsigned u, char *pOut)
{
	if (u < 0x80)
	{
		pOut[0] = (char) u;
		return 1;
	}
	if (u < 0x800)
	{
		pOut[0] = (char) (0xC0 | (u >> 6));
		pOut[1] = (char) (0x80 | (u & 0x3F));
		return 2;
	}
	pOut[0] = (char) (0xE0 | (u >> 12));
	pOut[1] = (char) (0x80 | ((u >> 6) & 0x3F));
	pOut[2] = (char) (0x80 | (u & 0x3F));
	return 3;
}

void CP850ToUTF8 (const char *pIn, char *pOut, unsigned nSize)
{
	unsigned n = 0;
	for (; *pIn != '\0' && n + 4 < nSize; pIn++)
	{
		u8 c = (u8) *pIn;
		unsigned u = c < 0x80 ? c : ff_oem2uni (c, FF_CODE_PAGE);
		if (u == 0)
		{
			u = '?';
		}
		n += PutUTF8 (u, pOut + n);
	}
	pOut[n] = '\0';
}

unsigned UTF8Length (const char *p)
{
	unsigned n = 0;
	for (; *p != '\0'; p++)
	{
		if (((u8) *p & 0xC0) != 0x80)
		{
			n++;
		}
	}
	return n;
}
