//
// video.h - Salida de vídeo (framebuffer de la GPU, doble búfer), tamaño de
//           borde, scanlines y utilidades de dibujo
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_video_h
#define _zx_video_h

#include "machine.h"
#include "palette.h"
#include <circle/bcmframebuffer.h>
#include <circle/types.h>

// Colores adicionales para la interfaz (después de los 16 del Spectrum)
#define COLOR_UI_PANEL		16
#define COLOR_UI_TEXT		17
#define COLOR_UI_HILITE		18
#define COLOR_UI_HILITE_TEXT	19
#define COLOR_UI_TITLE		20
#define COLOR_UI_DIM		21
#define COLOR_UI_DIR		22
#define COLOR_COUNT		23

// Tamaños de borde (los de JSpeccy)
enum TBorderSize
{
	BorderNone,		// 256 x 192
	BorderStandard,		// 320 x 240
	BorderFull,		// 352 x 296
	BorderHuge,		// 384 x 304
	BorderSizeCount
};

struct TVisibleRect
{
	int x, y;		// esquina dentro de la imagen generada
	int nWidth, nHeight;
};

class CVideo
{
public:
	CVideo (void);
	~CVideo (void);

	boolean Initialize (TBorderSize Border = BorderFull, boolean bScanlines = FALSE);

	// Cambia el tamaño del borde y/o las scanlines (recrea el framebuffer)
	boolean SetMode (TBorderSize Border, boolean bScanlines);
	TBorderSize GetBorderSize (void) const	{ return m_Border; }
	boolean GetScanlines (void) const	{ return m_bScanlines; }
	boolean ScanlinesAvailable (void) const;	// la pantalla tiene resolución suficiente
	const TVisibleRect &GetVisibleRect (void) const	{ return m_Visible; }
	static const char *GetBorderName (TBorderSize Border);

	// Muestra la parte visible de la imagen (cambio de página en el siguiente refresco)
	void Present (const TPixel *pScreen);		// Render + Flip
	void Render (const TPixel *pScreen);		// a la página oculta
	void Flip (void);				// muestra esa página

	// Utilidades de dibujo sobre una imagen SCREEN_WIDTH x SCREEN_HEIGHT.
	// Los colores son índices: 0-15 del Spectrum y COLOR_UI_* de la interfaz.
	// Fuente de la ROM (32-127) más las letras de Europa (ver BuildExtFont)
	void SetFont (const u8 *pFont)		{ m_pFont = pFont; BuildExtFont (); }
	void FillRect (TPixel *pScreen, int x, int y, int w, int h, u8 nColor);
	void DrawChar (TPixel *pScreen, int x, int y, char chChar, u8 nInk, u8 nPaper);
	void DrawText (TPixel *pScreen, int x, int y, const char *pText, u8 nInk, u8 nPaper,
		       int nMaxChars = 1000);

private:
	boolean CreateFrameBuffer (void);

private:
	CBcmFrameBuffer *m_pFrameBuffer;
	unsigned m_nDisplayWidth, m_nDisplayHeight;

	TBorderSize m_Border;
	boolean m_bScanlines;
	TVisibleRect m_Visible;

	unsigned m_nFBWidth, m_nFBHeight;	// tamaño de cada página del framebuffer
	unsigned m_nOffsetX, m_nOffsetY;	// dónde empieza la imagen dentro de la página
	unsigned m_nRepeat;			// líneas de framebuffer por línea de imagen
						// (0 = aumento fraccionario)
	unsigned m_nOutWidth, m_nOutHeight;	// tamaño de la imagen con aumento fraccionario
	u16 *m_pXMap;				// columna de origen de cada columna de salida
	u16 *m_pYMap;				// línea de origen de cada línea de salida
	u8 *m_pYDim;				// 1 = última línea de su línea de origen (oscura)
	TPixel *m_pRowBuf;			// línea ya ampliada (en RAM normal, con caché)
	TPixel *m_pDimBuf;			// la misma línea a la mitad de brillo
	unsigned m_nPage;

	u16 m_Palette[256];
	const u8 *m_pFont;

	// Letras acentuadas y signos (Latin-1 y parte de Latin Extended-A),
	// hechos a partir de las letras de la ROM
	void BuildExtFont (void);
	const u8 *GetGlyph (unsigned nCode) const;
	void DrawGlyph (TPixel *pScreen, int x, int y, const u8 *pGlyph, u8 nInk, u8 nPaper);
	enum { EXT_GLYPHS = 160 };
	u16 m_ExtCode[EXT_GLYPHS];
	u8 m_ExtGlyph[EXT_GLYPHS][8];
	unsigned m_nExtGlyphs;
};

#endif
