//
// video.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "video.h"

// Triple búfer: tres páginas que se van turnando con SetVirtualOffset. La
// pantalla toma el cambio de página en el siguiente retrazado, así que justo
// después de un cambio se sigue viendo la página anterior; con tres, la que se
// dibuja nunca es ni la que se ve ni la que está a punto de verse, sin tener
// que esperar al retrazado (que pararía la emulación). Vale mientras no haya
// más de un cambio por barrido de la pantalla: a velocidad normal el Spectrum
// da 50 imágenes por segundo y la pantalla 50 o 60 barridos.
// Antes había dos páginas y se podía dibujar en la que aún se veía (cortes).
// Si el firmware no da tres, se prueba con dos y luego con una.
//
// El firmware de la Pi 5 no cambia de página: muestra el framebuffer virtual
// entero, con las páginas una encima de otra. Allí se usa una sola página
// (puede verse algún corte de imagen).
#if RASPPI >= 5
	#define FB_MAX_PAGES	1
#else
	#define FB_MAX_PAGES	3
#endif

// La GPU escala el framebuffer a la pantalla, salvo en la Pi 5: allí el
// firmware lo muestra tal cual, arriba a la izquierda. En la Pi 5 la CPU hace
// siempre todo el aumento, con scanlines o sin ellas.
#if RASPPI >= 5
	#define GPU_NO_SCALE	true
#else
	#define GPU_NO_SCALE	false
#endif
#include "i18n.h"
#include "videolayout.h"
#include <circle/bcmpropertytags.h>
#include <circle/util.h>
#include <circle/logger.h>

// Colores de la interfaz (después de los 16 del Spectrum)
static const u32 UIPaletteRGB[COLOR_COUNT - 16] =
{
	0x10123A,	// COLOR_UI_PANEL
	0xE8E8E8,	// COLOR_UI_TEXT
	0x36EFDE,	// COLOR_UI_HILITE
	0x000000,	// COLOR_UI_HILITE_TEXT
	0xEEEB46,	// COLOR_UI_TITLE
	0x8A8CA0,	// COLOR_UI_DIM
	0x7FD6FF,	// COLOR_UI_DIR
};

// Parte visible de la imagen generada para cada tamaño de borde
static const TVisibleRect BorderRects[BorderSizeCount] =
{
	{BORDER_LEFT,      BORDER_TOP,      256,       192},	// sin borde
	{BORDER_LEFT - 32, BORDER_TOP - 24, 256 + 64,  192 + 48},	// estándar
	{BORDER_LEFT - 48, BORDER_TOP - 48, 256 + 96,  192 + 104},	// completo
	{0,                0,               SCREEN_WIDTH, SCREEN_HEIGHT},	// enorme
};

static const char *BorderNames[BorderSizeCount] =
{
	N_("sin borde"), N_("estándar"), N_("completo"), N_("enorme")
};

CVideo::CVideo (void)
:	m_pFrameBuffer (0),
	m_nDisplayWidth (640),
	m_nDisplayHeight (480),
	m_Border (BorderFull),
	m_bScanlines (FALSE),
	m_nFBWidth (0),
	m_nFBHeight (0),
	m_nOffsetX (0),
	m_nOffsetY (0),
	m_nRepeat (1),
	m_nOutWidth (0),
	m_nOutHeight (0),
	m_pXMap (0),
	m_pYMap (0),
	m_pYDim (0),
	m_pRowBuf (0),
	m_pDimBuf (0),
	m_nPage (0),
	m_nPages (1),
	m_pFont (0)
{
	m_nExtGlyphs = 0;
	m_Visible = BorderRects[BorderFull];

	memset (m_Palette, 0, sizeof m_Palette);
	for (unsigned i = 0; i < 16; i++)
	{
		m_Palette[i] = RGB565 (SpectrumPaletteRGB[i]);
	}
	for (unsigned i = 16; i < COLOR_COUNT; i++)
	{
		m_Palette[i] = RGB565 (UIPaletteRGB[i - 16]);
	}
}

CVideo::~CVideo (void)
{
	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;
	delete [] m_pXMap;
	delete [] m_pYMap;
	delete [] m_pYDim;
	delete [] m_pRowBuf;
	delete [] m_pDimBuf;
}

const char *CVideo::GetBorderName (TBorderSize Border)
{
	return Border < BorderSizeCount ? BorderNames[Border] : "?";
}

boolean CVideo::Initialize (TBorderSize Border, boolean bScanlines)
{
	CBcmPropertyTags Tags;
	TPropertyTagDisplayDimensions Dimensions;
	if (   Tags.GetTag (PROPTAG_GET_DISPLAY_DIMENSIONS, &Dimensions, sizeof Dimensions)
	    && Dimensions.nWidth >= 320 && Dimensions.nHeight >= 200
	    && Dimensions.nWidth <= 8192 && Dimensions.nHeight <= 4320)
	{
		m_nDisplayWidth = Dimensions.nWidth;
		m_nDisplayHeight = Dimensions.nHeight;
	}

	return SetMode (Border, bScanlines);
}

boolean CVideo::ScanlinesAvailable (void) const
{
	TScanlineLayout Layout = {};
	return ComputeScanlineLayout (m_nDisplayWidth, m_nDisplayHeight,
				      m_Visible.nWidth, m_Visible.nHeight, &Layout, true, GPU_NO_SCALE);
}

boolean CVideo::SetMode (TBorderSize Border, boolean bScanlines)
{
	if (Border >= BorderSizeCount)
	{
		Border = BorderFull;
	}

	m_Border = Border;
	m_Visible = BorderRects[Border];
	m_bScanlines = bScanlines && ScanlinesAvailable ();

	if (CreateFrameBuffer ())
	{
		return TRUE;
	}

	// Si el framebuffer grande de las scanlines no se puede crear, sin ellas
	if (m_bScanlines)
	{
		m_bScanlines = FALSE;
		return CreateFrameBuffer ();
	}

	return FALSE;
}

boolean CVideo::CreateFrameBuffer (void)
{
	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;

	unsigned nWidth = m_Visible.nWidth;
	unsigned nHeight = m_Visible.nHeight;

	TScanlineLayout Layout = {};
	if (   m_bScanlines
	    && ComputeScanlineLayout (m_nDisplayWidth, m_nDisplayHeight, nWidth, nHeight, &Layout,
				      RASPPI >= 2, GPU_NO_SCALE))
	{
		// Framebuffer con la proporción de la pantalla; cada píxel del Spectrum
		// ocupa r x r y la GPU aplica una escala entera (ver videolayout.h)
		m_nRepeat = Layout.nRepeat;
		m_nFBWidth = Layout.nFBWidth;
		m_nFBHeight = Layout.nFBHeight;
	}
	else if (   GPU_NO_SCALE
		 && ComputeFillLayout (m_nDisplayWidth, m_nDisplayHeight, nWidth, nHeight, &Layout))
	{
		// Pi 5 sin scanlines: la CPU amplía la imagen a toda la pantalla
		m_bScanlines = FALSE;
		m_nRepeat = Layout.nRepeat;
		m_nFBWidth = Layout.nFBWidth;
		m_nFBHeight = Layout.nFBHeight;
	}
	else
	{
		// Mismo aspecto que la pantalla: la GPU escala sin deformar
		m_bScanlines = FALSE;
		m_nRepeat = 1;
		m_nFBHeight = nHeight;
		m_nFBWidth = (nHeight * m_nDisplayWidth + m_nDisplayHeight - 1) / m_nDisplayHeight;
		m_nFBWidth = (m_nFBWidth + 15) & ~15;
		if (m_nFBWidth < nWidth)
		{
			m_nFBWidth = (nWidth + 15) & ~15;
		}
	}

	delete [] m_pXMap;	m_pXMap = 0;
	delete [] m_pYMap;	m_pYMap = 0;
	delete [] m_pYDim;	m_pYDim = 0;
	delete [] m_pRowBuf;	m_pRowBuf = 0;
	delete [] m_pDimBuf;	m_pDimBuf = 0;

	if (m_nRepeat == 0)
	{
		// Aumento fraccionario: tablas de columnas y líneas de origen
		m_nOutWidth = Layout.nOutWidth;
		m_nOutHeight = Layout.nOutHeight;
		m_pXMap = new u16[m_nOutWidth];
		m_pYMap = new u16[m_nOutHeight];
		m_pYDim = new u8[m_nOutHeight];
		m_pRowBuf = new TPixel[m_nOutWidth + 4];
		m_pDimBuf = new TPixel[m_nOutWidth + 4];
		for (unsigned x = 0; x < m_nOutWidth; x++)
		{
			m_pXMap[x] = x * nWidth / m_nOutWidth;
		}
		for (unsigned y = 0; y < m_nOutHeight; y++)
		{
			m_pYMap[y] = y * nHeight / m_nOutHeight;
		}
		for (unsigned y = 0; y < m_nOutHeight; y++)
		{
			m_pYDim[y] =    m_bScanlines
				     && (y + 1 == m_nOutHeight || m_pYMap[y + 1] != m_pYMap[y]);
		}
		m_nOffsetX = ((m_nFBWidth - m_nOutWidth) / 2) & ~3;	// 8 bytes (ver CopyPixels)
		m_nOffsetY = (m_nFBHeight - m_nOutHeight) / 2;
	}
	else
	{
		m_nOffsetX = ((m_nFBWidth - nWidth * m_nRepeat) / 2) & ~3;	// 8 bytes (ver CopyPixels)
		m_nOffsetY = (m_nFBHeight - nHeight * m_nRepeat) / 2;
		if (m_nRepeat > 1)
		{
			m_pRowBuf = new TPixel[nWidth * m_nRepeat + 4];
			m_pDimBuf = new TPixel[nWidth * m_nRepeat + 4];
		}
	}

	// Tres páginas; si el firmware no las da, dos, y si no, una
	for (m_nPages = FB_MAX_PAGES; m_nPages >= 1; m_nPages--)
	{
		m_pFrameBuffer = new CBcmFrameBuffer (m_nFBWidth, m_nFBHeight, 16,
						      m_nFBWidth, m_nFBHeight * m_nPages);
		if (m_pFrameBuffer == 0)
		{
			return FALSE;
		}

		// El firmware puede dar otra cosa de la pedida (la Pi 5 limita el
		// framebuffer): si no cuadra, mejor no dibujar fuera de él
		if (   m_pFrameBuffer->Initialize ()
		    && m_pFrameBuffer->GetWidth () == m_nFBWidth
		    && m_pFrameBuffer->GetHeight () == m_nFBHeight
		    && m_pFrameBuffer->GetDepth () == 16
		    && m_pFrameBuffer->GetPitch () >= m_nFBWidth * sizeof (TPixel)
		    && m_pFrameBuffer->GetBuffer () != 0
		    && m_pFrameBuffer->GetSize () >= m_pFrameBuffer->GetPitch () * m_nFBHeight * m_nPages)
		{
			break;
		}

		delete m_pFrameBuffer;
		m_pFrameBuffer = 0;
	}
	if (m_pFrameBuffer == 0)
	{
		return FALSE;
	}

	// Limpia las páginas
	u8 *pBuffer = (u8 *) (uintptr) m_pFrameBuffer->GetBuffer ();
	memset (pBuffer, 0, m_pFrameBuffer->GetPitch () * m_nFBHeight * m_nPages);

	// Se ve la página 0; se dibuja en la siguiente
	m_nPage = m_nPages > 1 ? 1 : 0;

	CLogger::Get ()->Write ("video", LogNotice, "Framebuffer %ux%u, %u pagina%s",
				m_nFBWidth, m_nFBHeight, m_nPages, m_nPages > 1 ? "s" : "");

	return TRUE;
}

void CVideo::Present (const TPixel *pScreen)
{
	Render (pScreen);
	Flip ();
}

// Muestra la página en la que acaba de dibujar Render
void CVideo::Flip (void)
{
	if (m_pFrameBuffer == 0)
	{
		return;
	}
	if (m_nPages > 1)
	{
		m_pFrameBuffer->SetVirtualOffset (0, m_nPage * m_nFBHeight);
		m_nPage = (m_nPage + 1) % m_nPages;	// la siguiente: ni la que se ve ni la que se va a ver
	}
}

// Copia una línea de píxeles al framebuffer. El framebuffer está en la memoria
// de la GPU, que Circle mapea como memoria de dispositivo: sin caché y sin
// juntar escrituras, así que cada escritura es una transacción con el bus y
// leer de ahí es todavía más lento. Además, en memoria de dispositivo un
// acceso no alineado a su propio tamaño es un fallo de alineación (la Pi se
// cuelga; QEMU no lo comprueba).
//
// - El memcpy de Circle solo copia por bloques si origen y destino están
//   alineados (a 4 bytes en 32 bits, a 8 en 64); si no, copia byte a byte, y
//   en el framebuffer eso multiplicaba por 4 el tiempo en 64 bits.
// - Un bucle normal tampoco vale: GCC lo vectoriza y escribe 16 bytes de golpe
//   (str q en 64 bits, vst1 en 32) en direcciones que no están alineadas a 16.
//
// Por eso el destino es volatile: cada escritura es exactamente de 64, 32 o
// 16 bits, según la alineación, y nunca de bytes sueltos ni de 128 bits.
static void CopyPixels (void *pDest, const void *pSrc, unsigned nBytes)
{
	uintptr nAlign = (uintptr) pDest | (uintptr) pSrc;
	u8 *pD = (u8 *) pDest;
	const u8 *pS = (const u8 *) pSrc;

	if ((nAlign & 7) == 0)
	{
		volatile u64 *pD64 = (volatile u64 *) pD;
		const u64 *pS64 = (const u64 *) pS;
		unsigned n = nBytes / 8;
		for (; n >= 4; n -= 4)
		{
			u64 a = pS64[0], b = pS64[1], c = pS64[2], d = pS64[3];
			pD64[0] = a; pD64[1] = b; pD64[2] = c; pD64[3] = d;
			pD64 += 4;
			pS64 += 4;
		}
		for (; n > 0; n--)
		{
			*pD64++ = *pS64++;
		}
		pD = (u8 *) pD64;
		pS = (const u8 *) pS64;
		nBytes &= 7;
	}
	else if ((nAlign & 3) == 0)
	{
		volatile u32 *pD32 = (volatile u32 *) pD;
		const u32 *pS32 = (const u32 *) pS;
		for (unsigned n = nBytes / 4; n > 0; n--)
		{
			*pD32++ = *pS32++;
		}
		pD = (u8 *) pD32;
		pS = (const u8 *) pS32;
		nBytes &= 3;
	}

	volatile u16 *pD16 = (volatile u16 *) pD;
	const u16 *pS16 = (const u16 *) pS;
	for (unsigned n = nBytes / 2; n > 0; n--)
	{
		*pD16++ = *pS16++;
	}
}

// Línea a la mitad de brillo (scanline), en RAM normal
static void DimPixels (TPixel *pDest, const TPixel *pSrc, unsigned nPixels)
{
	for (unsigned x = 0; x < nPixels; x++)
	{
		pDest[x] = (pSrc[x] >> 1) & 0x7BEF;
	}
}

// Amplía la pantalla del Spectrum a la página oculta (puede ir en otro núcleo:
// solo lee pScreen y escribe en la página oculta). Cada línea se prepara en
// RAM normal (m_pRowBuf, m_pDimBuf) y al framebuffer solo se escribe, nunca
// se lee.
void CVideo::Render (const TPixel *pScreen)
{
	if (m_pFrameBuffer == 0)
	{
		return;
	}

	unsigned nPitch = m_pFrameBuffer->GetPitch ();
	u8 *pBase = (u8 *) (uintptr) m_pFrameBuffer->GetBuffer ()
		    + (m_nPage * m_nFBHeight + m_nOffsetY) * nPitch + m_nOffsetX * sizeof (TPixel);

	unsigned nWidth = m_Visible.nWidth;

	if (m_nRepeat == 1)
	{
		unsigned nLineBytes = nWidth * sizeof (TPixel);
		for (int y = 0; y < m_Visible.nHeight; y++)
		{
			CopyPixels (pBase + y * nPitch,
				    pScreen + (m_Visible.y + y) * SCREEN_WIDTH + m_Visible.x, nLineBytes);
		}
	}
	else if (m_nRepeat == 0)
	{
		// Aumento fraccionario: cada línea del Spectrum se amplía una vez en
		// m_pRowBuf (y m_pDimBuf) y se copia a sus 3 o 4 líneas de pantalla;
		// la última de cada grupo va a la mitad de brillo
		unsigned nLineBytes = m_nOutWidth * sizeof (TPixel);
		int nLastSrc = -1;
		for (unsigned y = 0; y < m_nOutHeight; y++)
		{
			int nSrc = m_pYMap[y];
			if (nSrc != nLastSrc)
			{
				const TPixel *pSrc = pScreen + (m_Visible.y + nSrc) * SCREEN_WIDTH + m_Visible.x;
				for (unsigned x = 0; x < m_nOutWidth; x++)
				{
					m_pRowBuf[x] = pSrc[m_pXMap[x]];
				}
				DimPixels (m_pDimBuf, m_pRowBuf, m_nOutWidth);
				nLastSrc = nSrc;
			}

			CopyPixels (pBase + y * nPitch, m_pYDim[y] ? m_pDimBuf : m_pRowBuf, nLineBytes);
		}
	}
	else
	{
		// Cada píxel se repite r veces en horizontal; la línea se repite r veces
		// y la última copia va a la mitad de brillo (scanline)
		unsigned nOutWidth = nWidth * m_nRepeat;
		unsigned nLineBytes = nOutWidth * sizeof (TPixel);

		for (int y = 0; y < m_Visible.nHeight; y++)
		{
			const TPixel *pSrc = pScreen + (m_Visible.y + y) * SCREEN_WIDTH + m_Visible.x;

			TPixel *pOut = m_pRowBuf;
			for (unsigned x = 0; x < nWidth; x++)
			{
				for (unsigned r = 0; r < m_nRepeat; r++)
				{
					*pOut++ = pSrc[x];
				}
			}
			DimPixels (m_pDimBuf, m_pRowBuf, nOutWidth);

			u8 *pDest = pBase + y * m_nRepeat * nPitch;
			for (unsigned r = 0; r < m_nRepeat - 1; r++, pDest += nPitch)
			{
				CopyPixels (pDest, m_pRowBuf, nLineBytes);
			}
			CopyPixels (pDest, m_bScanlines ? m_pDimBuf : m_pRowBuf, nLineBytes);
		}
	}
}

void CVideo::FillRect (TPixel *pScreen, int x, int y, int w, int h, u8 nColor)
{
	for (int j = y; j < y + h; j++)
	{
		if (j < 0 || j >= SCREEN_HEIGHT)
		{
			continue;
		}

		for (int i = x; i < x + w; i++)
		{
			if (i >= 0 && i < SCREEN_WIDTH)
			{
				pScreen[j * SCREEN_WIDTH + i] = m_Palette[nColor];
			}
		}
	}
}

void CVideo::DrawChar (TPixel *pScreen, int x, int y, char chChar, u8 nInk, u8 nPaper)
{
	DrawGlyph (pScreen, x, y, GetGlyph ((u8) chChar), nInk, nPaper);
}

void CVideo::DrawGlyph (TPixel *pScreen, int x, int y, const u8 *pGlyph, u8 nInk, u8 nPaper)
{
	TPixel nInkColor = m_Palette[nInk], nPaperColor = m_Palette[nPaper];

	for (int j = 0; j < 8; j++)
	{
		int py = y + j;
		if (py < 0 || py >= SCREEN_HEIGHT)
		{
			continue;
		}

		u8 nBits = pGlyph[j];
		TPixel *pPixel = pScreen + py * SCREEN_WIDTH + x;
		for (int i = 0; i < 8; i++, nBits <<= 1)
		{
			if (x + i >= 0 && x + i < SCREEN_WIDTH)
			{
				pPixel[i] = (nBits & 0x80) ? nInkColor : nPaperColor;
			}
		}
	}
}

// Glifo de un carácter Unicode: ASCII de la ROM, letras de BuildExtFont o '?'
const u8 *CVideo::GetGlyph (unsigned nCode) const
{
	if (nCode >= 32 && nCode < 127)
	{
		return m_pFont + (nCode - 32) * 8;
	}
	for (unsigned i = 0; i < m_nExtGlyphs; i++)
	{
		if (m_ExtCode[i] == nCode)
		{
			return m_ExtGlyph[i];
		}
	}
	return m_pFont + ('?' - 32) * 8;
}

// El texto va en UTF-8 (nMaxChars cuenta caracteres, no bytes). Un byte que
// no forma UTF-8 válido se toma como Latin-1.
void CVideo::DrawText (TPixel *pScreen, int x, int y, const char *pText, u8 nInk, u8 nPaper,
		       int nMaxChars)
{
	const u8 *p = (const u8 *) pText;
	for (int n = 0; *p != '\0' && n < nMaxChars; n++, x += 8)
	{
		unsigned nCode = *p++;
		if (nCode >= 0xC0 && nCode < 0xF0 && (*p & 0xC0) == 0x80)
		{
			if (nCode < 0xE0)
			{
				nCode = ((nCode & 0x1F) << 6) | (*p++ & 0x3F);
			}
			else if ((p[1] & 0xC0) == 0x80)
			{
				nCode = ((nCode & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
				p += 2;
			}
		}
		DrawGlyph (pScreen, x, y, GetGlyph (nCode), nInk, nPaper);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Letras de Europa para los menús. Las acentuadas se componen con la letra de
// la ROM y la marca: en las minúsculas, la marca ocupa las dos filas de
// arriba; en las mayúsculas, la letra se reduce a 5 filas (se quita una fila
// repetida) para dejarle sitio. Unos pocos signos se dibujan a mano.

enum TMark { MarkNone, MarkAcute, MarkGrave, MarkCirc, MarkDiaer, MarkTilde, MarkRing,
	     MarkCedilla, MarkCaron, MarkOgonek, MarkDblAcute, MarkDot, MarkBreve, MarkStroke };

static const struct { u16 nCode; char cBase; u8 nMark; } Composed[] =
{
	{0xC0, 'A', MarkGrave}, {0xC1, 'A', MarkAcute}, {0xC2, 'A', MarkCirc}, {0xC3, 'A', MarkTilde},
	{0xC4, 'A', MarkDiaer}, {0xC5, 'A', MarkRing}, {0xC7, 'C', MarkCedilla},
	{0xC8, 'E', MarkGrave}, {0xC9, 'E', MarkAcute}, {0xCA, 'E', MarkCirc}, {0xCB, 'E', MarkDiaer},
	{0xCC, 'I', MarkGrave}, {0xCD, 'I', MarkAcute}, {0xCE, 'I', MarkCirc}, {0xCF, 'I', MarkDiaer},
	{0xD1, 'N', MarkTilde},
	{0xD2, 'O', MarkGrave}, {0xD3, 'O', MarkAcute}, {0xD4, 'O', MarkCirc}, {0xD5, 'O', MarkTilde},
	{0xD6, 'O', MarkDiaer},
	{0xD9, 'U', MarkGrave}, {0xDA, 'U', MarkAcute}, {0xDB, 'U', MarkCirc}, {0xDC, 'U', MarkDiaer},
	{0xDD, 'Y', MarkAcute},
	{0xE0, 'a', MarkGrave}, {0xE1, 'a', MarkAcute}, {0xE2, 'a', MarkCirc}, {0xE3, 'a', MarkTilde},
	{0xE4, 'a', MarkDiaer}, {0xE5, 'a', MarkRing}, {0xE7, 'c', MarkCedilla},
	{0xE8, 'e', MarkGrave}, {0xE9, 'e', MarkAcute}, {0xEA, 'e', MarkCirc}, {0xEB, 'e', MarkDiaer},
	{0xEC, 'i', MarkGrave}, {0xED, 'i', MarkAcute}, {0xEE, 'i', MarkCirc}, {0xEF, 'i', MarkDiaer},
	{0xF1, 'n', MarkTilde},
	{0xF2, 'o', MarkGrave}, {0xF3, 'o', MarkAcute}, {0xF4, 'o', MarkCirc}, {0xF5, 'o', MarkTilde},
	{0xF6, 'o', MarkDiaer},
	{0xF9, 'u', MarkGrave}, {0xFA, 'u', MarkAcute}, {0xFB, 'u', MarkCirc}, {0xFC, 'u', MarkDiaer},
	{0xFD, 'y', MarkAcute}, {0xFF, 'y', MarkDiaer},
	// Europa central y del este (Latin Extended-A, las más usadas)
	{0x102, 'A', MarkBreve}, {0x103, 'a', MarkBreve}, {0x104, 'A', MarkOgonek}, {0x105, 'a', MarkOgonek},
	{0x106, 'C', MarkAcute}, {0x107, 'c', MarkAcute}, {0x10C, 'C', MarkCaron}, {0x10D, 'c', MarkCaron},
	{0x10E, 'D', MarkCaron}, {0x10F, 'd', MarkCaron}, {0x118, 'E', MarkOgonek}, {0x119, 'e', MarkOgonek},
	{0x11A, 'E', MarkCaron}, {0x11B, 'e', MarkCaron}, {0x11E, 'G', MarkBreve}, {0x11F, 'g', MarkBreve},
	{0x130, 'I', MarkDot}, {0x141, 'L', MarkStroke}, {0x142, 'l', MarkStroke},
	{0x143, 'N', MarkAcute}, {0x144, 'n', MarkAcute}, {0x147, 'N', MarkCaron}, {0x148, 'n', MarkCaron},
	{0x150, 'O', MarkDblAcute}, {0x151, 'o', MarkDblAcute}, {0x154, 'R', MarkAcute}, {0x155, 'r', MarkAcute},
	{0x158, 'R', MarkCaron}, {0x159, 'r', MarkCaron}, {0x15A, 'S', MarkAcute}, {0x15B, 's', MarkAcute},
	{0x15E, 'S', MarkCedilla}, {0x15F, 's', MarkCedilla}, {0x160, 'S', MarkCaron}, {0x161, 's', MarkCaron},
	{0x162, 'T', MarkCedilla}, {0x163, 't', MarkCedilla}, {0x164, 'T', MarkCaron}, {0x165, 't', MarkCaron},
	{0x16E, 'U', MarkRing}, {0x16F, 'u', MarkRing}, {0x170, 'U', MarkDblAcute}, {0x171, 'u', MarkDblAcute},
	{0x179, 'Z', MarkAcute}, {0x17A, 'z', MarkAcute}, {0x17B, 'Z', MarkDot}, {0x17C, 'z', MarkDot},
	{0x17D, 'Z', MarkCaron}, {0x17E, 'z', MarkCaron},
	{0x218, 'S', MarkCedilla}, {0x219, 's', MarkCedilla}, {0x21A, 'T', MarkCedilla}, {0x21B, 't', MarkCedilla},
};

// Marcas de arriba: dos filas (0 y 1)
static const u8 MarkRows[][2] =
{
	{0x00, 0x00},	// MarkNone
	{0x08, 0x10},	// MarkAcute
	{0x10, 0x08},	// MarkGrave
	{0x18, 0x24},	// MarkCirc
	{0x00, 0x24},	// MarkDiaer
	{0x32, 0x4C},	// MarkTilde
	{0x18, 0x18},	// MarkRing
	{0x00, 0x00},	// MarkCedilla (abajo)
	{0x24, 0x18},	// MarkCaron
	{0x00, 0x00},	// MarkOgonek (abajo)
	{0x12, 0x24},	// MarkDblAcute
	{0x00, 0x10},	// MarkDot
	{0x24, 0x18},	// MarkBreve
	{0x00, 0x00},	// MarkStroke (cruza la letra)
};

static const struct { u16 nCode; u8 Rows[8]; } Drawn[] =
{
	{0xA1, {0x00, 0x10, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00}},	// ¡
	{0xBF, {0x00, 0x10, 0x00, 0x10, 0x20, 0x42, 0x3C, 0x00}},	// ¿
	{0xAB, {0x00, 0x00, 0x12, 0x24, 0x48, 0x24, 0x12, 0x00}},	// «
	{0xBB, {0x00, 0x00, 0x48, 0x24, 0x12, 0x24, 0x48, 0x00}},	// »
	{0xAA, {0x00, 0x1C, 0x24, 0x1C, 0x00, 0x3C, 0x00, 0x00}},	// ª
	{0xBA, {0x00, 0x18, 0x24, 0x18, 0x00, 0x3C, 0x00, 0x00}},	// º
	{0xB0, {0x00, 0x18, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00}},	// °
	{0xB7, {0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00}},	// ·
	{0xD7, {0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00}},	// ×
	{0xF7, {0x00, 0x00, 0x10, 0x00, 0x7C, 0x00, 0x10, 0x00}},	// ÷
	{0xDF, {0x00, 0x38, 0x44, 0x78, 0x44, 0x44, 0x78, 0x40}},	// ß
	{0xC6, {0x00, 0x3E, 0x48, 0x48, 0x7E, 0x48, 0x4E, 0x00}},	// Æ
	{0xE6, {0x00, 0x00, 0x6C, 0x12, 0x7C, 0x90, 0x6E, 0x00}},	// æ
	{0xD8, {0x00, 0x3E, 0x46, 0x4A, 0x52, 0x62, 0x7C, 0x00}},	// Ø
	{0xF8, {0x00, 0x02, 0x3C, 0x4C, 0x54, 0x64, 0x3C, 0x40}},	// ø
	{0x152, {0x00, 0x7E, 0x90, 0x9C, 0x90, 0x90, 0x7E, 0x00}},	// Œ
	{0x153, {0x00, 0x00, 0x6C, 0x92, 0x9E, 0x90, 0x6E, 0x00}},	// œ
	{0x131, {0x00, 0x00, 0x00, 0x30, 0x10, 0x10, 0x38, 0x00}},	// ı
	{0x20AC, {0x00, 0x1C, 0x22, 0xF8, 0x20, 0xF8, 0x22, 0x1C}},	// €
	{0x2018, {0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00}},	// ‘
	{0x2019, {0x00, 0x08, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00}},	// ’
	{0x201C, {0x00, 0x24, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00}},	// “
	{0x201D, {0x00, 0x24, 0x24, 0x48, 0x00, 0x00, 0x00, 0x00}},	// ”
	{0x2013, {0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00}},	// –
	{0x2014, {0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00}},	// —
	{0x2026, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x00}},	// …
	{0x2192, {0x00, 0x00, 0x08, 0x04, 0x7E, 0x04, 0x08, 0x00}},	// →
	{0x2190, {0x00, 0x00, 0x10, 0x20, 0x7E, 0x20, 0x10, 0x00}},	// ←
	{0x2191, {0x00, 0x10, 0x38, 0x54, 0x10, 0x10, 0x10, 0x00}},	// ↑
	{0x2193, {0x00, 0x10, 0x10, 0x10, 0x54, 0x38, 0x10, 0x00}},	// ↓
};

void CVideo::BuildExtFont (void)
{
	m_nExtGlyphs = 0;
	if (m_pFont == 0)
	{
		return;
	}

	// los que tiene la propia ROM fuera de su sitio ASCII
	static const struct { u16 nCode; u8 nROM; } ROMChars[] = { {0xA3, 0x60}, {0xA9, 0x7F} };	// £ ©
	for (unsigned i = 0; i < sizeof ROMChars / sizeof ROMChars[0]; i++)
	{
		m_ExtCode[m_nExtGlyphs] = ROMChars[i].nCode;
		memcpy (m_ExtGlyph[m_nExtGlyphs++], m_pFont + (ROMChars[i].nROM - 32) * 8, 8);
	}

	for (unsigned i = 0; i < sizeof Drawn / sizeof Drawn[0] && m_nExtGlyphs < EXT_GLYPHS; i++)
	{
		m_ExtCode[m_nExtGlyphs] = Drawn[i].nCode;
		memcpy (m_ExtGlyph[m_nExtGlyphs++], Drawn[i].Rows, 8);
	}

	for (unsigned i = 0; i < sizeof Composed / sizeof Composed[0] && m_nExtGlyphs < EXT_GLYPHS; i++)
	{
		char cBase = Composed[i].cBase;
		u8 nMark = Composed[i].nMark;
		u8 Rows[8];
		memcpy (Rows, m_pFont + (cBase - 32) * 8, 8);
		boolean bUpper = cBase >= 'A' && cBase <= 'Z';

		if (nMark == MarkCedilla || nMark == MarkOgonek)
		{
			Rows[7] |= nMark == MarkCedilla ? 0x10 : 0x04;
		}
		else if (nMark == MarkStroke)
		{
			Rows[3] |= 0x18;
			Rows[4] |= 0x20;
		}
		else
		{
			if (bUpper)
			{
				// la letra (filas 1-6) pasa a 2-6: se quita una fila repetida
				int nDrop = 2;
				for (int r = 2; r <= 6; r++)
				{
					if (Rows[r] == Rows[r - 1])
					{
						nDrop = r;
						break;
					}
				}
				u8 Letter[8] = {0};
				int k = 2;
				for (int r = 1; r <= 6; r++)
				{
					if (r != nDrop)
					{
						Letter[k++] = Rows[r];
					}
				}
				Letter[7] = Rows[7];
				memcpy (Rows, Letter, 8);
			}
			else if (cBase == 'i')
			{
				Rows[0] = Rows[1] = Rows[2] = 0;	// sin el punto
			}
			Rows[0] |= MarkRows[nMark][0];
			Rows[1] |= MarkRows[nMark][1];
		}

		m_ExtCode[m_nExtGlyphs] = Composed[i].nCode;
		memcpy (m_ExtGlyph[m_nExtGlyphs++], Rows, 8);
	}
}
