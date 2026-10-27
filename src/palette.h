//
// palette.h - Colores del Spectrum y formato de píxel de la imagen (RGB565)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_palette_h
#define _zx_palette_h

#include <circle/types.h>

// La imagen se genera directamente en RGB565: así ULAplus puede cambiar la
// paleta en mitad del cuadro (efectos raster) sin afectar a lo ya dibujado.
typedef u16 TPixel;

static inline TPixel RGB565 (u32 nRGB)
{
	return   (((nRGB >> 19) & 0x1F) << 11)
	       | (((nRGB >> 10) & 0x3F) << 5)
	       |  ((nRGB >> 3) & 0x1F);
}

// Paleta del Spectrum (JSpeccy, Paleta48k): 0-7 normal, 8-15 con BRIGHT
static const u32 SpectrumPaletteRGB[16] =
{
	0x060800, 0x0D13A7, 0xBD0707, 0xC312AF, 0x07BA0C, 0x0DC6B4, 0xBCB914, 0xC2C4BC,
	0x060800, 0x161CB0, 0xCE1818, 0xDC2CC8, 0x28DC2D, 0x36EFDE, 0xEEEB46, 0xFDFFF7,
};

// Color ULAplus (GGGRRRBB) a RGB de 24 bits, como en JSpeccy
static inline u32 ULAplusToRGB (u8 nColor)
{
	u32 nBlue = (nColor & 0x03) << 1;
	if (nColor & 0x01)
	{
		nBlue |= 0x01;
	}
	nBlue = (nBlue << 5) | (nBlue << 2) | (nBlue & 0x03);

	u32 nRed = (nColor & 0x1C) >> 2;
	nRed = (nRed << 5) | (nRed << 2) | (nRed & 0x03);

	u32 nGreen = (nColor & 0xE0) >> 5;
	nGreen = (nGreen << 5) | (nGreen << 2) | (nGreen & 0x03);

	return ((nRed & 0xFF) << 16) | ((nGreen & 0xFF) << 8) | (nBlue & 0xFF);
}

#endif
