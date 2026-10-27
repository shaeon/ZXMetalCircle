//
// inflate.h - Descompresor deflate (RFC 1951) / zlib (RFC 1950) mínimo
//
// Solo se usa para los bloques TZX 0x18 (CSW) con compresión Z-RLE.
// Implementación sencilla, decodificando los códigos Huffman bit a bit
// (el mismo enfoque que "puff" de Mark Adler).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_inflate_h
#define _zx_inflate_h

#include <circle/types.h>

// Devuelve el número de bytes descomprimidos, o -1 si hay error o no cabe
int ZlibInflate (const u8 *pSource, unsigned nSourceLength, u8 *pDest, unsigned nDestLength);
int RawInflate (const u8 *pSource, unsigned nSourceLength, u8 *pDest, unsigned nDestLength);	// sin cabecera (ZIP)

#endif
