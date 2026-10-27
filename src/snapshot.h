//
// snapshot.h - Carga de snapshots .SNA, .Z80 y .SZX, y grabación en .Z80 (versión 3)
//
// Formatos:
//   .SNA  https://worldofspectrum.org/faq/reference/formats.htm
//   .Z80  https://worldofspectrum.org/faq/reference/z80format.htm
//   .SZX  https://www.spectaculator.com/docs/zx-state/intro.shtml
//
// .SNA: en 48K el PC se guarda en la pila (como manda el formato), en una copia
// de la memoria; el +2A se graba como 128K (el formato no tiene 0x1FFD).
// .SZX: páginas sin comprimir, registros, puertos, AY y paleta ULAplus.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_snapshot_h
#define _zx_snapshot_h

#include "spectrum.h"
#include <circle/types.h>

#define SNAPSHOT_MAX_SIZE	(256 * 1024)	// .Z80 de 128K sin comprimir: ~131 KB

class CSnapshot
{
public:
	// Carga un snapshot (el formato se deduce de la extensión del nombre).
	// Si hace falta, cambia el modelo emulado. Devuelve un mensaje de error o 0.
	static const char *Load (CSpectrum *pSpectrum, const u8 *pData, unsigned nSize,
				 const char *pFileName);

	// Graban el estado actual. Devuelven el tamaño o 0.
	static unsigned SaveZ80 (CSpectrum *pSpectrum, u8 *pBuffer, unsigned nBufferSize);	// .Z80 v3
	static unsigned SaveSNA (CSpectrum *pSpectrum, u8 *pBuffer, unsigned nBufferSize);
	static unsigned SaveSZX (CSpectrum *pSpectrum, u8 *pBuffer, unsigned nBufferSize);

private:
	static const char *LoadSNA (CSpectrum *pSpectrum, const u8 *pData, unsigned nSize);
	static const char *LoadZ80 (CSpectrum *pSpectrum, const u8 *pData, unsigned nSize);
	static const char *LoadSZX (CSpectrum *pSpectrum, const u8 *pData, unsigned nSize);

	static boolean DecompressZ80 (const u8 *pIn, unsigned nInSize, u8 *pOut, unsigned nOutSize,
				      boolean bEndMarker);
	static unsigned CompressZ80 (const u8 *pIn, unsigned nInSize, u8 *pOut, unsigned nOutSize);

	static void SetModel (CSpectrum *pSpectrum, TModel Model);
	static void Finish (CSpectrum *pSpectrum);
};

#endif
