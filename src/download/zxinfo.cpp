//
// zxinfo.cpp
//
// Usa jsmn (MIT, Serge Zaitsev) para leer el JSON de api.zxinfo.dk.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "zxinfo.h"
#include "../i18n.h"
#include "inflate.h"

#ifdef __circle__
#include <circle/util.h>
#else
#include <string.h>
#endif

#define JSMN_STATIC
#include "../../third_party/jsmn.h"

#define SEARCH_URL	"https://api.zxinfo.dk/v3/search?mode=compact&contenttype=SOFTWARE&size=%u&query="
#define FILES_HOST	"https://spectrumcomputing.co.uk"

// Extensiones útiles, de más a menos preferida
static const char *s_Extensions[] = {".tzx", ".tap", ".pzx", ".z80", ".sna", ".szx", ".dsk", ".csw", ".rom", ".mdr"};
#define NUM_EXTENSIONS	(sizeof s_Extensions / sizeof s_Extensions[0])

static int ExtensionRank (const char *pName, size_t nLen)
{
	for (unsigned i = 0; i < NUM_EXTENSIONS; i++)
	{
		size_t l = strlen (s_Extensions[i]);
		for (size_t p = 0; p + l <= nLen; p++)
		{
			bool bMatch = true;
			for (size_t k = 0; k < l; k++)
			{
				char c = pName[p + k];
				if (c >= 'A' && c <= 'Z') c += 32;
				if (c != s_Extensions[i][k])
				{
					bMatch = false;
					break;
				}
			}
			// la extensión debe ir al final o seguida de ".zip"
			if (bMatch && (p + l == nLen || pName[p + l] == '.'))
			{
				return (int) i;
			}
		}
	}
	return -1;
}

static void Copy (char *pDest, size_t nSize, const char *pSrc, size_t nLen)
{
	size_t i = 0;
	for (; i + 1 < nSize && i < nLen; i++)
	{
		pDest[i] = pSrc[i];
	}
	pDest[i] = '\0';
}

//------------------------------------------------------------------------------
// JSON

struct TJSON
{
	const char *pText;
	jsmntok_t *pTok;
	int nTokens;
};

static int Skip (const TJSON &J, int i)
{
	int n = J.pTok[i].size;
	int j = i + 1;
	while (n-- > 0 && j < J.nTokens)
	{
		j = Skip (J, j);
	}
	return j;
}

static bool TokenIs (const TJSON &J, int i, const char *pStr)
{
	const jsmntok_t &t = J.pTok[i];
	size_t l = strlen (pStr);
	return t.type == JSMN_STRING && (size_t) (t.end - t.start) == l
	       && memcmp (J.pText + t.start, pStr, l) == 0;
}

// Valor de la clave pKey en el objeto i (-1 si no está)
static int Find (const TJSON &J, int i, const char *pKey)
{
	if (i < 0 || J.pTok[i].type != JSMN_OBJECT)
	{
		return -1;
	}
	int n = J.pTok[i].size;
	int j = i + 1;
	while (n-- > 0 && j < J.nTokens)
	{
		if (TokenIs (J, j, pKey))
		{
			return j + 1;
		}
		j = Skip (J, j + 1);	// salta clave y valor
	}
	return -1;
}

static void GetString (const TJSON &J, int i, char *pDest, size_t nSize)
{
	pDest[0] = '\0';
	if (i >= 0 && (J.pTok[i].type == JSMN_STRING || J.pTok[i].type == JSMN_PRIMITIVE))
	{
		Copy (pDest, nSize, J.pText + J.pTok[i].start, J.pTok[i].end - J.pTok[i].start);
	}
}

static void URLEncode (const char *pIn, char *pOut, size_t nSize)
{
	static const char Hex[] = "0123456789ABCDEF";
	size_t o = 0;
	for (const unsigned char *p = (const unsigned char *) pIn; *p != '\0' && o + 4 < nSize; p++)
	{
		if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9')
		    || *p == '-' || *p == '_' || *p == '.' || *p == '/' || *p == '(' || *p == ')')
		{
			pOut[o++] = *p;
		}
		else
		{
			pOut[o++] = '%';
			pOut[o++] = Hex[*p >> 4];
			pOut[o++] = Hex[*p & 15];
		}
	}
	pOut[o] = '\0';
}

int ZXInfoSearch (CHTTPSClient *pClient, const char *pQuery, TZXInfoResult *pResults,
		  unsigned nMaxResults, unsigned char *pWork, size_t nWorkSize, const char **pError)
{
	*pError = "";
	if (nMaxResults > ZXINFO_MAX_RESULTS)
	{
		nMaxResults = ZXINFO_MAX_RESULTS;
	}

	static char URL[512];
	static char Encoded[256];
	URLEncode (pQuery, Encoded, sizeof Encoded);
	// SEARCH_URL lleva un %u para el número de resultados
	unsigned nAsk = nMaxResults * 2;	// se descartan los de otras máquinas
	const char *pFmt = SEARCH_URL;
	size_t n = 0;
	for (const char *p = pFmt; *p != '\0' && n < sizeof URL - 1; p++)
	{
		if (p[0] == '%' && p[1] == 'u')
		{
			char Num[8];
			unsigned v = nAsk, k = 0;
			do { Num[k++] = '0' + v % 10; v /= 10; } while (v > 0);
			while (k > 0 && n < sizeof URL - 1) URL[n++] = Num[--k];
			p++;
			continue;
		}
		URL[n++] = *p;
	}
	URL[n] = '\0';
	Copy (URL + n, sizeof URL - n, Encoded, strlen (Encoded));

	// El JSON va al principio de pWork y los "tokens" de jsmn detrás
	size_t nJSONMax = nWorkSize / 2;
	size_t nLength = 0;
	int nStatus = pClient->Get (URL, pWork, nJSONMax - 1, &nLength);
	if (nStatus != 200)
	{
		*pError = nStatus < 0 ? pClient->GetError () : N_("El buscador ha devuelto un error");
		return -1;
	}
	pWork[nLength] = '\0';

	jsmn_parser Parser;
	jsmn_init (&Parser);
	TJSON J;
	J.pText = (const char *) pWork;
	J.pTok = (jsmntok_t *) (pWork + ((nJSONMax + 7) & ~7));
	unsigned nMaxTokens = (unsigned) ((nWorkSize - ((nJSONMax + 7) & ~7)) / sizeof (jsmntok_t));
	J.nTokens = jsmn_parse (&Parser, J.pText, nLength, J.pTok, nMaxTokens);
	if (J.nTokens < 1)
	{
		*pError = N_("Respuesta del buscador no válida");
		return -1;
	}

	int nHits = Find (J, Find (J, 0, "hits"), "hits");
	if (nHits < 0 || J.pTok[nHits].type != JSMN_ARRAY)
	{
		return 0;
	}

	int nCount = 0;
	int nHit = nHits + 1;
	for (int h = 0; h < J.pTok[nHits].size && nCount < (int) nMaxResults; h++, nHit = Skip (J, nHit))
	{
		TZXInfoResult &R = pResults[nCount];
		memset (&R, 0, sizeof R);
		GetString (J, Find (J, nHit, "_id"), R.ID, sizeof R.ID);

		int nSrc = Find (J, nHit, "_source");
		GetString (J, Find (J, nSrc, "title"), R.Title, sizeof R.Title);
		GetString (J, Find (J, nSrc, "machineType"), R.Machine, sizeof R.Machine);
		// solo programas de Spectrum (fuera ZX81, SAM Coupé, etc.)
		if (memcmp (R.Machine, "ZX-Spectrum", 11) != 0 && memcmp (R.Machine, "Timex", 5) != 0)
		{
			continue;
		}
		char Year[8];
		GetString (J, Find (J, nSrc, "originalYearOfRelease"), Year, sizeof Year);
		R.nYear = 0;
		for (const char *p = Year; *p >= '0' && *p <= '9'; p++) R.nYear = R.nYear * 10 + (*p - '0');

		int nPubs = Find (J, nSrc, "publishers");
		if (nPubs >= 0 && J.pTok[nPubs].type == JSMN_ARRAY && J.pTok[nPubs].size > 0)
		{
			GetString (J, Find (J, nPubs + 1, "name"), R.Publisher, sizeof R.Publisher);
		}

		// mejor fichero de todas las ediciones
		int nBest = (int) NUM_EXTENSIONS;
		int nRels = Find (J, nSrc, "releases");
		if (nRels >= 0 && J.pTok[nRels].type == JSMN_ARRAY)
		{
			int nRel = nRels + 1;
			for (int r = 0; r < J.pTok[nRels].size; r++, nRel = Skip (J, nRel))
			{
				int nFiles = Find (J, nRel, "files");
				if (nFiles < 0 || J.pTok[nFiles].type != JSMN_ARRAY)
				{
					continue;
				}
				int nFile = nFiles + 1;
				for (int f = 0; f < J.pTok[nFiles].size; f++, nFile = Skip (J, nFile))
				{
					int nPath = Find (J, nFile, "path");
					if (nPath < 0)
					{
						continue;
					}
					const char *pPath = J.pText + J.pTok[nPath].start;
					size_t nLen = J.pTok[nPath].end - J.pTok[nPath].start;
					int nRank = ExtensionRank (pPath, nLen);
					// a igualdad de formato, mejor la versión sin "(...)" en el nombre
					bool bVariant = false;
					for (size_t k = 0; k < nLen; k++)
					{
						if (pPath[k] == '(')
						{
							bVariant = true;
						}
					}
					bool bBestVariant = false;
					for (const char *q = R.Path; *q != '\0'; q++)
					{
						if (*q == '(')
						{
							bBestVariant = true;
						}
					}
					if (nRank >= 0 && (nRank < nBest || (nRank == nBest && bBestVariant && !bVariant)))
					{
						nBest = nRank;
						Copy (R.Path, sizeof R.Path, pPath, nLen);
						const char *pExt = s_Extensions[nRank] + 1;
						for (unsigned k = 0; k < 4; k++)
						{
							R.Format[k] = pExt[k] >= 'a' ? pExt[k] - 32 : pExt[k];
						}
						R.Format[3] = '\0';
					}
				}
			}
		}

		nCount++;
	}

	return nCount;
}

//------------------------------------------------------------------------------
// ZIP

static unsigned Get16 (const unsigned char *p)	{ return p[0] | (p[1] << 8); }
static unsigned long Get32 (const unsigned char *p)
	{ return p[0] | (p[1] << 8) | ((unsigned long) p[2] << 16) | ((unsigned long) p[3] << 24); }

size_t ZipExtract (const unsigned char *pZip, size_t nZipSize, unsigned char *pOut, size_t nOutSize,
		   char *pName, size_t nNameSize)
{
	// fin del directorio central
	if (nZipSize < 22)
	{
		return 0;
	}
	size_t nEnd = nZipSize - 22;
	while (Get32 (pZip + nEnd) != 0x06054B50)
	{
		if (nEnd == 0 || nZipSize - nEnd > 22 + 65535)
		{
			return 0;
		}
		nEnd--;
	}

	unsigned nEntries = Get16 (pZip + nEnd + 10);
	size_t nDir = Get32 (pZip + nEnd + 16);

	// la entrada con la extensión preferida
	int nBestRank = (int) NUM_EXTENSIONS;
	size_t nBestEntry = 0;
	size_t p = nDir;
	for (unsigned i = 0; i < nEntries && p + 46 <= nZipSize; i++)
	{
		if (Get32 (pZip + p) != 0x02014B50)
		{
			break;
		}
		unsigned nNameLen = Get16 (pZip + p + 28);
		unsigned nExtra = Get16 (pZip + p + 30);
		unsigned nComment = Get16 (pZip + p + 32);
		int nRank = ExtensionRank ((const char *) pZip + p + 46, nNameLen);
		if (nRank >= 0 && nRank < nBestRank)
		{
			nBestRank = nRank;
			nBestEntry = p;
		}
		p += 46 + nNameLen + nExtra + nComment;
	}
	if (nBestRank == (int) NUM_EXTENSIONS)
	{
		return 0;
	}

	const unsigned char *e = pZip + nBestEntry;
	unsigned nMethod = Get16 (e + 10);
	size_t nCompressed = Get32 (e + 20);
	size_t nSize = Get32 (e + 24);
	unsigned nNameLen = Get16 (e + 28);
	size_t nLocal = Get32 (e + 42);

	// nombre sin carpetas
	const char *pFull = (const char *) e + 46;
	unsigned nStart = 0;
	for (unsigned k = 0; k < nNameLen; k++)
	{
		if (pFull[k] == '/' || pFull[k] == '\\')
		{
			nStart = k + 1;
		}
	}
	Copy (pName, nNameSize, pFull + nStart, nNameLen - nStart);

	if (nLocal + 30 > nZipSize || Get32 (pZip + nLocal) != 0x04034B50 || nSize > nOutSize)
	{
		return 0;
	}
	size_t nData = nLocal + 30 + Get16 (pZip + nLocal + 26) + Get16 (pZip + nLocal + 28);
	if (nData + nCompressed > nZipSize)
	{
		return 0;
	}

	if (nMethod == 0)
	{
		memcpy (pOut, pZip + nData, nSize);
		return nSize;
	}
	if (nMethod == 8)
	{
		int r = RawInflate (pZip + nData, nCompressed, pOut, nSize);
		return r == (int) nSize ? nSize : 0;
	}
	return 0;
}

size_t ZXInfoDownload (CHTTPSClient *pClient, const char *pPath, unsigned char *pOut, size_t nOutSize,
		       char *pName, size_t nNameSize, unsigned char *pWork, size_t nWorkSize,
		       const char **pError)
{
	*pError = "";

	static char URL[512];
	static char Encoded[384];
	URLEncode (pPath, Encoded, sizeof Encoded);
	Copy (URL, sizeof URL, FILES_HOST, strlen (FILES_HOST));
	size_t n = strlen (URL);
	Copy (URL + n, sizeof URL - n, Encoded, strlen (Encoded));

	size_t nLength = 0;
	int nStatus = pClient->Get (URL, pWork, nWorkSize, &nLength);
	if (nStatus != 200)
	{
		*pError = nStatus < 0 ? pClient->GetError () : N_("El servidor no tiene ese fichero");
		return 0;
	}

	// ¿ZIP?
	if (nLength >= 4 && Get32 (pWork) == 0x04034B50)
	{
		size_t nSize = ZipExtract (pWork, nLength, pOut, nOutSize, pName, nNameSize);
		if (nSize == 0)
		{
			*pError = N_("No se pudo descomprimir el fichero");
		}
		return nSize;
	}

	// sin comprimir: el nombre es el final de la ruta
	const char *pBase = pPath;
	for (const char *p = pPath; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pBase = p + 1;
		}
	}
	Copy (pName, nNameSize, pBase, strlen (pBase));
	if (nLength > nOutSize)
	{
		*pError = N_("Fichero demasiado grande");
		return 0;
	}
	memcpy (pOut, pWork, nLength);
	return nLength;
}
