//
// updater.cpp
//
// Instalación de una actualización desde el paquete de la SD (ver updater.h)
//
#include "updater.h"
#include "zipfile.h"
#include <string.h>
#include <stdarg.h>

// Une trozos de texto (terminados en 0) en pDest: en la Pi no está la
// función de C para dar formato a textos
static void Join (char *pDest, unsigned nSize, ...)
{
	va_list Args;
	va_start (Args, nSize);
	unsigned n = 0;
	const char *p;
	while ((p = va_arg (Args, const char *)) != 0)
	{
		while (*p != '\0' && n + 1 < nSize)
		{
			pDest[n++] = *p++;
		}
	}
	pDest[n] = '\0';
	va_end (Args);
}

static boolean EqualNoCase (const char *a, const char *b)
{
	for (; *a != '\0' && *b != '\0'; a++, b++)
	{
		char ca = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a;
		char cb = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
		if (ca != cb)
		{
			return FALSE;
		}
	}
	return *a == *b;
}

static boolean StartsNoCase (const char *pText, const char *pPrefix)
{
	for (; *pPrefix != '\0'; pText++, pPrefix++)
	{
		char ca = *pText >= 'A' && *pText <= 'Z' ? *pText + 32 : *pText;
		char cb = *pPrefix >= 'A' && *pPrefix <= 'Z' ? *pPrefix + 32 : *pPrefix;
		if (ca != cb)
		{
			return FALSE;
		}
	}
	return TRUE;
}

boolean CUpdater::IsConfigFile (const char *pPath)
{
	static const char *Config[] =
	{
		"cmdline.txt", "config.txt", "excluir.txt", "tnfs.txt",
		"esxdos_i/SYS/CONFIG/ESXDOS.CFG", "esxdos_i/SYS/CONFIG/TRDOS.CFG"
	};
	for (unsigned i = 0; i < sizeof Config / sizeof Config[0]; i++)
	{
		if (EqualNoCase (pPath, Config[i]))
		{
			return TRUE;
		}
	}
	return FALSE;
}

boolean CUpdater::IsKernel (const char *pPath)
{
	unsigned nLen = strlen (pPath);
	return StartsNoCase (pPath, "kernel") && strchr (pPath, '/') == 0
	       && nLen > 4 && EqualNoCase (pPath + nLen - 4, ".img");
}

boolean CUpdater::InEsxImport (const char *pPath)
{
	return StartsNoCase (pPath, "esxdos_i/");
}

int CUpdater::CompareVersions (const char *a, const char *b)
{
	// "1.06" frente a "1.10": cada parte como número
	while (*a != '\0' || *b != '\0')
	{
		unsigned na = 0, nb = 0;
		while (*a >= '0' && *a <= '9') na = na * 10 + *a++ - '0';
		while (*b >= '0' && *b <= '9') nb = nb * 10 + *b++ - '0';
		if (na != nb)
		{
			return na < nb ? -1 : 1;
		}
		if (*a == '.') a++;
		if (*b == '.') b++;
		if (*a != '\0' && (*a < '0' || *a > '9')) a++;	// sufijos: se ignoran
		if (*b != '\0' && (*b < '0' || *b > '9')) b++;
	}
	return 0;
}

static void AddNote (TUpdateResult *pResult, const char *pPath, const char *pSuffix)
{
	if (pResult->nNotes < UPDATE_MAX_NOTES)
	{
		Join (pResult->Notes[pResult->nNotes++], UPDATE_NOTE_LEN, pPath, pSuffix, (const char *) 0);
	}
}

// Crea las carpetas de la ruta ("a/b/c.txt": "a" y "a/b")
static boolean MakeDirs (CUpdateFS *pFS, const char *pPath)
{
	char Dir[ZIP_NAME_LEN];
	for (const char *p = strchr (pPath, '/'); p != 0; p = strchr (p + 1, '/'))
	{
		unsigned nLen = p - pPath;
		if (nLen >= sizeof Dir)
		{
			return FALSE;
		}
		memcpy (Dir, pPath, nLen);
		Dir[nLen] = '\0';
		if (!pFS->MakeDir (Dir))
		{
			return FALSE;
		}
	}
	return TRUE;
}

// ¿El fichero de la SD es igual que la entrada (tamaño y CRC)?
static boolean SameAsEntry (CUpdateFS *pFS, const char *pPath, unsigned nSize, u32 nCRC, u8 *pBuffer)
{
	unsigned nOnSD;
	if (!pFS->GetSize (pPath, &nOnSD) || nOnSD != nSize)
	{
		return FALSE;
	}
	return pFS->Read (pPath, pBuffer, nSize) && CZipFile::CRC32 (pBuffer, nSize) == nCRC;
}

// Escribe con otro nombre y renombra: nunca queda un fichero a medias
static boolean SafeWrite (CUpdateFS *pFS, const char *pPath, const u8 *pData, unsigned nSize)
{
	char Tmp[ZIP_NAME_LEN + 8];
	Join (Tmp, sizeof Tmp, pPath, ".new", (const char *) 0);
	unsigned nDummy;
	if (pFS->GetSize (Tmp, &nDummy))
	{
		pFS->Remove (Tmp);
	}
	if (!pFS->Write (Tmp, pData, nSize))
	{
		return FALSE;
	}
	if (pFS->GetSize (pPath, &nDummy) && !pFS->Remove (pPath))
	{
		return FALSE;
	}
	return pFS->Rename (Tmp, pPath);
}

void CUpdater::Install (CUpdateFS *pFS, const u8 *pZip, unsigned nZipSize,
			const u8 *pPrev, unsigned nPrevSize,
			boolean bReplaceConfig, const char *pVersion,
			TUpdateResult *pResult, TProgress *pProgress, void *pParam)
{
	memset (pResult, 0, sizeof *pResult);

	CZipFile *pPackage = new CZipFile (UPDATE_MAX_ENTRIES);
	CZipFile *pPrevPackage = new CZipFile (UPDATE_MAX_ENTRIES);
	if (pPackage == 0 || pPrevPackage == 0)
	{
		delete pPackage;
		delete pPrevPackage;
		pResult->Status = UpdateNoMemory;
		return;
	}
	boolean bPrev = pPrev != 0 && pPrevPackage->Open (pPrev, nPrevSize);

	// 1. El paquete entero, comprobado antes de tocar nada
	unsigned nMax = 1;
	if (!pPackage->Open (pZip, nZipSize) || pPackage->GetCount () == 0)
	{
		pResult->Status = UpdateBadPackage;
		delete pPackage;
		delete pPrevPackage;
		return;
	}
	unsigned nCount = pPackage->GetCount ();
	for (unsigned i = 0; i < nCount; i++)
	{
		if (pPackage->GetSize (i) > nMax)
		{
			nMax = pPackage->GetSize (i);
		}
	}
	u8 *pData = new u8[nMax];
	u8 *pOnSD = new u8[nMax];
	if (pData == 0 || pOnSD == 0)
	{
		delete [] pData;
		delete [] pOnSD;
		delete pPackage;
		delete pPrevPackage;
		pResult->Status = UpdateNoMemory;
		return;
	}
	for (unsigned i = 0; i < nCount; i++)
	{
		if (pProgress != 0)
		{
			pProgress (pParam, i, 2 * nCount);
		}
		if (!pPackage->IsDirectory (i)
		    && pPackage->Extract (i, pData, nMax) != (int) pPackage->GetSize (i))
		{
			pResult->Status = UpdateBadPackage;
			Join (pResult->Failed, sizeof pResult->Failed, pPackage->GetName (i), (const char *) 0);
			delete [] pData;
			delete [] pOnSD;
			delete pPackage;
			delete pPrevPackage;
			return;
		}
	}

	// 2. La marca: si se corta, al arrancar se sabe que no terminó
	pFS->MakeDir (UPDATE_DIR);
	pFS->Write (UPDATE_MARKER, (const u8 *) pVersion, strlen (pVersion));

	// 3. Los ficheros; los kernels en la segunda vuelta
	pResult->Status = UpdateOK;
	unsigned nStep = nCount;
	for (unsigned nPass = 0; nPass < 2 && pResult->Status == UpdateOK; nPass++)
	{
		for (unsigned i = 0; i < nCount; i++)
		{
			const char *pPath = pPackage->GetName (i);
			if (pPackage->IsDirectory (i) || IsKernel (pPath) != (nPass == 1))
			{
				continue;
			}
			if (pProgress != 0)
			{
				pProgress (pParam, nStep++, 2 * nCount);
			}
			unsigned nSize = pPackage->GetSize (i);
			u32 nCRC = pPackage->GetCRC (i);
			boolean bConfig = IsConfigFile (pPath);

			if (InEsxImport (pPath))
			{
				// esxdos_i: frente al paquete anterior, no frente a la SD.
				// Sin paquete anterior no se toca. Su configuración, solo
				// si se pide poner la nueva.
				boolean bCopy = FALSE;
				if (bPrev && (!bConfig || bReplaceConfig))
				{
					bCopy = TRUE;
					for (unsigned j = 0; j < pPrevPackage->GetCount (); j++)
					{
						if (EqualNoCase (pPrevPackage->GetName (j), pPath)
						    && pPrevPackage->GetSize (j) == nSize
						    && pPrevPackage->GetCRC (j) == nCRC)
						{
							bCopy = FALSE;
							break;
						}
					}
				}
				if (!bCopy)
				{
					pResult->nUnchanged++;
					continue;
				}
			}
			else if (SameAsEntry (pFS, pPath, nSize, nCRC, pOnSD))
			{
				pResult->nUnchanged++;
				continue;
			}

			pPackage->Extract (i, pData, nMax);
			if (!MakeDirs (pFS, pPath))
			{
				pResult->Status = UpdateWriteError;
				Join (pResult->Failed, sizeof pResult->Failed, pPath, (const char *) 0);
				break;
			}

			unsigned nDummy;
			boolean bOK;
			if (bConfig && !InEsxImport (pPath) && pFS->GetSize (pPath, &nDummy))
			{
				// configuración del usuario distinta de la nueva
				char Other[ZIP_NAME_LEN + 8];
				if (bReplaceConfig)
				{
					Join (Other, sizeof Other, pPath, ".bak", (const char *) 0);
					if (pFS->GetSize (Other, &nDummy))
					{
						pFS->Remove (Other);
					}
					bOK = pFS->Rename (pPath, Other) && SafeWrite (pFS, pPath, pData, nSize);
					AddNote (pResult, pPath, ".bak");
				}
				else
				{
					// el nuevo, al lado; no cuenta como cambio (no afecta a
					// nada en marcha) y si ya estaba igual no se reescribe
					Join (Other, sizeof Other, pPath, ".nuevo", (const char *) 0);
					if (!SameAsEntry (pFS, Other, nSize, nCRC, pOnSD))
					{
						if (!SafeWrite (pFS, Other, pData, nSize))
						{
							pResult->Status = UpdateWriteError;
							Join (pResult->Failed, sizeof pResult->Failed, Other, (const char *) 0);
							break;
						}
						AddNote (pResult, pPath, ".nuevo");
					}
					pResult->nUnchanged++;
					continue;
				}
			}
			else
			{
				bOK = SafeWrite (pFS, pPath, pData, nSize);
			}
			if (!bOK)
			{
				pResult->Status = UpdateWriteError;
				Join (pResult->Failed, sizeof pResult->Failed, pPath, (const char *) 0);
				break;
			}
			pResult->nChanged++;
		}
	}

	// 4. Terminada: fuera la marca (si falló, se queda)
	if (pResult->Status == UpdateOK)
	{
		pFS->Remove (UPDATE_MARKER);
	}

	delete [] pData;
	delete [] pOnSD;
	delete pPackage;
	delete pPrevPackage;
}

// El valor de texto de "clave": "valor" a partir de p; 0 si no hay
static const char *FindString (const char *p, const char *pEnd, const char *pKey,
			       char *pValue, unsigned nSize)
{
	unsigned nKey = strlen (pKey);
	for (; p + nKey + 2 < pEnd; p++)
	{
		if (*p != '"' || strncmp (p + 1, pKey, nKey) != 0 || p[nKey + 1] != '"')
		{
			continue;
		}
		const char *q = p + nKey + 2;
		while (q < pEnd && (*q == ' ' || *q == ':' || *q == '\t' || *q == '\r' || *q == '\n'))
		{
			q++;
		}
		if (q >= pEnd || *q != '"')
		{
			continue;
		}
		q++;
		unsigned n = 0;
		while (q < pEnd && *q != '"' && n + 1 < nSize)
		{
			pValue[n++] = *q++;
		}
		pValue[n] = '\0';
		return q;
	}
	return 0;
}

boolean CUpdater::ParseRelease (const char *pJSON, unsigned nLength,
				char *pVersion, unsigned nVersionSize,
				char *pURL, unsigned nURLSize)
{
	const char *pEnd = pJSON + nLength;
	char Tag[32];
	if (FindString (pJSON, pEnd, "tag_name", Tag, sizeof Tag) == 0)
	{
		return FALSE;
	}
	Join (pVersion, nVersionSize, Tag[0] == 'v' || Tag[0] == 'V' ? Tag + 1 : Tag, (const char *) 0);

	const char *p = pJSON;
	while ((p = FindString (p, pEnd, "browser_download_url", pURL, nURLSize)) != 0)
	{
		unsigned n = strlen (pURL);
		if (n > 7 && EqualNoCase (pURL + n - 7, "-sd.zip"))
		{
			return TRUE;
		}
	}
	return FALSE;
}

void CUpdater::PackageName (const char *pVersion, char *pName, unsigned nSize)
{
	Join (pName, nSize, UPDATE_DIR, "/ZXMetalCircle-", pVersion, "-sd.zip", (const char *) 0);
}

boolean CUpdater::ParseTagURL (const char *pFinalURL,
			       char *pVersion, unsigned nVersionSize,
			       char *pURL, unsigned nURLSize)
{
	const char *p = strstr (pFinalURL, "/releases/tag/");
	if (p == 0)
	{
		return FALSE;
	}
	const char *pTag = p + 14;
	char Tag[32];
	unsigned n = 0;
	while (pTag[n] != '\0' && pTag[n] != '/' && pTag[n] != '?' && pTag[n] != '#' && n + 1 < sizeof Tag)
	{
		Tag[n] = pTag[n];
		n++;
	}
	Tag[n] = '\0';
	if (n == 0)
	{
		return FALSE;
	}
	Join (pVersion, nVersionSize, Tag[0] == 'v' || Tag[0] == 'V' ? Tag + 1 : Tag, (const char *) 0);
	char Base[512];
	unsigned nBase = (unsigned) (p - pFinalURL) < sizeof Base ? (unsigned) (p - pFinalURL) : sizeof Base - 1;
	memcpy (Base, pFinalURL, nBase);
	Base[nBase] = '\0';
	Join (pURL, nURLSize, Base, "/releases/download/", Tag, "/ZXMetalCircle-", pVersion, "-sd.zip",
	      (const char *) 0);
	return TRUE;
}
