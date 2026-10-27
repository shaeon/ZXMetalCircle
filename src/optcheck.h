//
// optcheck.h - Comprueba las opciones de cmdline.txt
//
// Un valor mal escrito (sounddev=sndpwn) no da error: el emulador usa el valor
// por defecto sin avisar. Esto lee cmdline.txt, busca opciones desconocidas y
// valores que no están en la lista de los admitidos y devuelve un aviso corto
// para la pantalla (y los escribe todos en el registro).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_optcheck_h
#define _zx_optcheck_h

#include <circle/types.h>

// Comprueba la línea de opciones (texto de cmdline.txt). Devuelve el número de
// problemas; en pMessage queda el aviso del primero ("" si no hay).
// pLog (opcional) se llama con cada problema.
typedef void TOptionLogger (const char *pText);
unsigned CheckOptions (const char *pCmdLine, char *pMessage, unsigned nSize,
		       TOptionLogger *pLog = 0);

#endif
