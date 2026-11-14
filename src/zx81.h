//
// zx81.h
//
// El ZX81 con el emulador ZX81+ de Paul Farrow (www.fruitcake.plus.com),
// incluido con su permiso: una ROM de 16 KB para un cartucho del Interface 2
// que solo funciona en el 128K (usa la segunda pantalla del 128).
//
// CZX81Loader carga un programa .P como en ESPectrum y como explica Paul
// Farrow para los emuladores: elige "32K RAM" en el menú de la ROM, escribe
// PAUSE 5000 y, durante la pausa, copia el .P en 16393 (las variables del
// sistema del ZX81 y el programa). Las teclas se pulsan cuadro a cuadro,
// mientras se emula, con la matriz de teclas propia del emulador.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx81_h
#define _zx81_h

#include <circle/types.h>

class CSpectrum;
class CZXKeyboard;

#define ZX81_ROM_NAME		"zx81.rom"
#define ZX81_ROM_SIZE		0x4000
#define ZX81_KBD_BYTE		0x2042		// bit 0: 1 = teclado del ZX81, 0 = del 128
#define ZX81_PROG_ADDR		16393		// los .P empiezan en las variables del sistema
#define ZX81_PROG_MAX		(0x4000 - 9)	// lo que cabe en los 16 KB del ZX81

class CZX81Loader
{
public:
	CZX81Loader (void);

	// La distribución del teclado con la que arranca la ROM (en una copia)
	static void SetKeyboardLayout (u8 *pROM, boolean bZX81Layout);

	// Empieza la carga (justo después de poner el cartucho, con reset);
	// FALSE si el programa no cabe
	boolean Start (const u8 *pProgram, unsigned nSize);
	void Cancel (CZXKeyboard *pKeyboard);
	boolean IsActive (void) const		{ return m_nStep >= 0; }

	// Tras cada cuadro emulado; TRUE el cuadro en que el programa ya está copiado
	boolean Frame (CSpectrum *pSpectrum, CZXKeyboard *pKeyboard);

private:
	u8 m_Program[ZX81_PROG_MAX];
	unsigned m_nSize;
	int m_nStep;			// -1: no hay carga
	unsigned m_nFrames;		// cuadros que quedan del paso
	boolean m_bPressed;		// teclas del paso pulsadas
};

#endif
