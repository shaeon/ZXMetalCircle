//
// zxinfo.h - Búsqueda de programas en ZXDB (api.zxinfo.dk) y descarga
//
// La búsqueda devuelve, para cada programa, el mejor fichero descargable
// (por este orden: .tzx, .tap, .z80, .sna, .szx, .dsk, .csw...). Los ficheros
// están en Spectrum Computing (https://spectrumcomputing.co.uk + ruta), casi
// siempre comprimidos en .zip: se descomprime el primer fichero útil.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_zxinfo_h
#define _zx_zxinfo_h

#include "httpsclient.h"

#define ZXINFO_MAX_RESULTS	20

struct TZXInfoResult
{
	char ID[12];
	char Title[64];
	char Publisher[48];
	char Machine[32];
	unsigned nYear;
	char Path[192];		// ruta del fichero en el servidor ("" = no hay)
	char Format[8];		// "TZX", "TAP"...
};

// Busca nQuery; devuelve el número de resultados (< 0: error, ver pError)
int ZXInfoSearch (CHTTPSClient *pClient, const char *pQuery, TZXInfoResult *pResults,
		  unsigned nMaxResults, unsigned char *pWork, size_t nWorkSize, const char **pError);

// Descarga la ruta y, si es un .zip, extrae el fichero útil. Devuelve su
// tamaño (0: error) y su nombre en pName.
size_t ZXInfoDownload (CHTTPSClient *pClient, const char *pPath, unsigned char *pOut, size_t nOutSize,
		       char *pName, size_t nNameSize, unsigned char *pWork, size_t nWorkSize,
		       const char **pError);

// Extrae de un .zip el primer fichero con extensión conocida
size_t ZipExtract (const unsigned char *pZip, size_t nZipSize, unsigned char *pOut, size_t nOutSize,
		   char *pName, size_t nNameSize);

#endif
