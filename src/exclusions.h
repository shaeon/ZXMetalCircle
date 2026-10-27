//
// exclusions.h - Ficheros y carpetas que no se muestran (SD:/excluir.txt)
//
// Una entrada por línea; lo que va tras # se ignora.
//   - Si termina en / o \, es una carpeta; si no, un fichero.
//   - Si empieza por / o \, es una ruta desde la raíz de la SD (puede tener
//     varias carpetas: /juegos/viejos/); si no, vale para ese nombre en
//     cualquier carpeta.
//   - Se admiten los comodines * y ?. No distingue mayúsculas (como la FAT).
// Lo usan el selector de ficheros (F1) y la página web.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_exclusions_h
#define _zx_exclusions_h

#include <circle/types.h>

#define EXCLUSIONS_FILE		"SD:/excluir.txt"

class CExclusions
{
public:
	CExclusions (void);
	~CExclusions (void);

	// Vuelve a leer el fichero (sin fichero, no se excluye nada)
	void Load (const char *pPath = EXCLUSIONS_FILE);

	// Añade una línea con el formato del fichero (lo usa Load ())
	void AddLine (const char *pLine);

	// pDir: carpeta que se está listando ("SD:/", "SD:/juegos", "/juegos"...)
	boolean IsExcluded (const char *pDir, const char *pName, boolean bDir) const;

	static CExclusions *Get (void)	{ return s_pThis; }

	// Comparación con comodines (* y ?) sin distinguir mayúsculas
	static boolean Match (const char *pPattern, const char *pText);

private:
	void Clear (void);

	struct TEntry
	{
		char *pPattern;		// sin la / final; con / inicial si es ruta desde la raíz
		boolean bDir;
		boolean bRooted;
		TEntry *pNext;
	};

	TEntry *m_pFirst;
	static CExclusions *s_pThis;
};

#endif
