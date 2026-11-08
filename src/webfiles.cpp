//
// webfiles.cpp
//
// ZXMetalCircle, Shaeon (Carlos Palmero). GPLv3.
//
#include "webfiles.h"
#include <string.h>
#include <strings.h>

// Carpetas de la raíz cuyo contenido también es del sistema
static const char *s_SystemTrees[] = {"overlays", "firmware", "certs", "locales", "update"};

// Carpetas de la raíz que usa el emulador: ellas no se tocan, su contenido sí
static const char *s_SystemFolders[] = {"roms", "hdd", "esxdos_i", "cpm", "docs", "utils", "joysticks",
					"saves", "snaps", "descargas", "tmp"};

// Ficheros de la raíz por nombre exacto
static const char *s_RootFiles[] = {"config.txt", "cmdline.txt", "excluir.txt", "tnfs.txt",
				    "bootcode.bin", "LICENSE", "README.md", "README_es.md", "CHANGELOG.md"};

static bool HasExt (const char *pName, const char *pExt)
{
	unsigned nLen = strlen (pName), nExt = strlen (pExt);
	return nLen > nExt && strcasecmp (pName + nLen - nExt, pExt) == 0;
}

static bool StartsWith (const char *pName, const char *pPrefix)
{
	return strncasecmp (pName, pPrefix, strlen (pPrefix)) == 0;
}

void WebNormalizePath (const char *pIn, char *pOut, unsigned nSize)
{
	if (strncasecmp (pIn, "SD:", 3) == 0)
	{
		pIn += 3;
	}
	while (*pIn == '/' || *pIn == '\\')
	{
		pIn++;
	}
	unsigned n = 0;
	for (; *pIn != '\0' && n + 1 < nSize; pIn++)
	{
		pOut[n++] = *pIn == '\\' ? '/' : *pIn;
	}
	while (n > 0 && pOut[n - 1] == '/')
	{
		n--;
	}
	pOut[n] = '\0';
}

bool WebValidName (const char *pName)
{
	if (pName == 0 || *pName == '\0' || strlen (pName) > 200)
	{
		return false;
	}
	if (strcmp (pName, ".") == 0 || strcmp (pName, "..") == 0)
	{
		return false;
	}
	for (const char *p = pName; *p != '\0'; p++)
	{
		if ((unsigned char) *p < 32 || strchr ("/\\:*?\"<>|", *p) != 0)
		{
			return false;
		}
	}
	// FAT no admite un nombre que acabe en punto o espacio, ni que empiece por espacio
	unsigned nLen = strlen (pName);
	return pName[0] != ' ' && pName[nLen - 1] != '.' && pName[nLen - 1] != ' ';
}

bool WebPathIsUnder (const char *pPath, const char *pDir)
{
	char Path[512], Dir[512];
	WebNormalizePath (pPath, Path, sizeof Path);
	WebNormalizePath (pDir, Dir, sizeof Dir);
	if (Dir[0] == '\0')
	{
		return true;			// todo está dentro de la raíz
	}
	unsigned n = strlen (Dir);
	return strncasecmp (Path, Dir, n) == 0 && (Path[n] == '\0' || Path[n] == '/');
}

bool WebIsSystemFolder (const char *pDir)
{
	char Dir[512];
	WebNormalizePath (pDir, Dir, sizeof Dir);
	for (unsigned i = 0; i < sizeof s_SystemTrees / sizeof s_SystemTrees[0]; i++)
	{
		if (WebPathIsUnder (Dir, s_SystemTrees[i]))
		{
			return true;
		}
	}
	return false;
}

bool WebIsSystem (const char *pPath)
{
	char Path[512];
	WebNormalizePath (pPath, Path, sizeof Path);
	if (Path[0] == '\0')
	{
		return true;			// la raíz misma
	}

	// dentro de una carpeta del sistema con contenido protegido
	if (WebIsSystemFolder (Path))
	{
		return true;
	}

	const char *pSlash = strchr (Path, '/');
	if (pSlash != 0)
	{
		return false;			// fuera de la raíz y de esas carpetas: libre
	}

	// en la raíz: las carpetas que usa el emulador...
	for (unsigned i = 0; i < sizeof s_SystemFolders / sizeof s_SystemFolders[0]; i++)
	{
		if (strcasecmp (Path, s_SystemFolders[i]) == 0)
		{
			return true;
		}
	}
	// ...y los ficheros del arranque
	for (unsigned i = 0; i < sizeof s_RootFiles / sizeof s_RootFiles[0]; i++)
	{
		if (strcasecmp (Path, s_RootFiles[i]) == 0)
		{
			return true;
		}
	}
	return    (StartsWith (Path, "kernel") && HasExt (Path, ".img"))
	       || HasExt (Path, ".dtb") || HasExt (Path, ".elf") || HasExt (Path, ".dat")
	       || HasExt (Path, ".bin") || HasExt (Path, ".hlp")
	       || StartsWith (Path, "LICENCE") || StartsWith (Path, "wpa_supplicant.conf");
}
