//
// testutil.h - Lo común de las pruebas en el PC que manejan un Spectrum entero
//
// Un Spectrum (S) con su teclado (kb) y su cinta (tape), cuadros sin sonido,
// teclas del teclado del Spectrum (matriz GPIO), ROMs y ficheros del
// repositorio, y lo que hay en la pantalla (en la RAM, sin pintar).
//
// Las pruebas se ejecutan desde test/host (como en la Action).
//
#ifndef _testutil_h
#define _testutil_h

#include <initializer_list>
#define private public		// para mirar dentro (la página de pantalla)
#include "spectrum.h"
#undef private
#include <stdio.h>
#include <string.h>
#include <vector>

#define ROMDIR	"../../roms/"

static int fails = 0;
static inline void chk (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) fails++;
}

static CZXKeyboard kb;
static CTape tape;
static CSpectrum *S;

// El borrado del PC hace de cursor a la izquierda cuando se usa la LEC (el
// CP/M de LEC borra con él), como hace el kernel; se puede quitar para ver
// la diferencia
static bool s_bBackspaceAsLeft = true;

static inline void run (int n)
{
	for (int i = 0; i < n; i++)
	{
		kb.SetBackspaceAsLeft (s_bBackspaceAsLeft && S->IsLECInUse ());
		S->RunFrame ();
		S->GetAudio ()->ClearBuffer ();
	}
}

// Teclas del Spectrum: fila (0-7) y bit (0-4) de la matriz
#define K(r,b)	(((r) << 3) | (b))
#define CS	K(0,0)
#define SS	K(7,1)
#define ENTER	K(6,0)

static inline void press (std::initializer_list<int> Keys, int nHold = 6, int nGap = 8)
{
	u8 m[8] = {0};
	for (int k : Keys) m[k >> 3] |= 1 << (k & 7);
	kb.SetGPIOMatrix (m);
	run (nHold);
	u8 z[8] = {0};
	kb.SetGPIOMatrix (z);
	run (nGap);
}

static inline int key (char c)
{
	static const char *Rows[8] = {"#zxcv", "asdfg", "qwert", "12345", "09876", "poiuy", "\nlkjh", " $mnb"};
	for (int r = 0; r < 8; r++)
		for (int b = 0; b < 5; b++)
			if (Rows[r][b] == c) return K(r, b);
	return -1;
}

static inline void type (const char *p)
{
	for (; *p; p++) press ({key (*p)});
}

// Una tecla del teclado USB (código HID)
static inline void usb (u8 nKey)
{
	static CHostInput Input;		// los teclados USB llegan por aquí (hostinput.h)
	Input.SetMachineInput (&kb);
	u8 r[6] = {nKey, 0, 0, 0, 0, 0}, z[6] = {0};
	Input.UpdateFromReport (0, r);
	run (6);
	Input.UpdateFromReport (0, z);
	run (8);
}

static inline std::vector<u8> ReadFile (const char *pName)
{
	std::vector<u8> v;
	FILE *f = fopen (pName, "rb");
	if (f == 0) { printf ("falta %s\n", pName); return v; }
	int c;
	while ((c = fgetc (f)) != EOF) v.push_back ((u8) c);
	fclose (f);
	return v;
}

// Una ROM de roms/ en nParts trozos de 16 KB (prefijo-0.rom, -1.rom...);
// se queda en memoria mientras dure la prueba
static inline const u8 *LoadROM (const char *pPrefix, unsigned nParts)
{
	u8 *pROM = new u8[nParts * 0x4000];
	for (unsigned i = 0; i < nParts; i++)
	{
		char Name[256];
		snprintf (Name, sizeof Name, ROMDIR "%s-%u.rom", pPrefix, i);
		std::vector<u8> v = ReadFile (Name);
		if (v.size () != 0x4000) return 0;
		memcpy (pROM + i * 0x4000, v.data (), 0x4000);
	}
	return pROM;
}

// La pantalla que se ve (la que lee la ULA: banco 5, o el 7 en los 128K)
static inline u8 ScreenByte (unsigned nOffset)
{
	return S->m_ULA.GetScreenPage ()[nOffset];
}

// Atributo de la celda (fila 0-23, columna 0-31)
static inline u8 Attr (unsigned nRow, unsigned nCol)
{
	return ScreenByte (0x1800 + nRow * 32 + nCol);
}

// Píxeles de tinta en las filas de texto nFirst a nLast (0-23)
static inline unsigned InkPixels (unsigned nFirst, unsigned nLast)
{
	unsigned n = 0;
	for (unsigned nRow = nFirst; nRow <= nLast; nRow++)
		for (unsigned nLine = 0; nLine < 8; nLine++)
			for (unsigned nCol = 0; nCol < 32; nCol++)
			{
				unsigned y = nRow * 8 + nLine;
				unsigned nAddr = ((y & 0xC0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2) | nCol;
				n += __builtin_popcount (ScreenByte (nAddr));
			}
	return n;
}

// Celdas de la fila nRow (0-23) con el papel nPaper (0-7)
static inline unsigned PaperCells (unsigned nRow, unsigned nPaper)
{
	unsigned n = 0;
	for (unsigned nCol = 0; nCol < 32; nCol++)
		n += ((Attr (nRow, nCol) >> 3) & 7) == nPaper;
	return n;
}

#endif
