// El borde entero se pinta en todos los modelos: cada píxel del borde tiene un
// T-state en la tabla. Antes, en los TK (con el papel desplazado), la esquina
// de abajo a la derecha caía después del final del cuadro y no se pintaba (una
// raya negra). En el TK a 60 Hz, las líneas que no tiene las rellena RunFrame.
#define private public
#include "spectrum.h"
#undef private
#include <stdio.h>
#include <string.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

int main (void)
{
	CZXKeyboard K;
	CTape T;
	static u8 Rom[32768];
	struct { const char *pName; TModel Model; TTKULA ULA; } C[] = {
		{"48K", Model48K, TKULA50Hz}, {"128K", Model128K, TKULA50Hz},
		{"TK90X a 50 Hz", ModelTK90X, TKULA50Hz}, {"TK90X a 60 Hz", ModelTK90X, TKULA60Hz},
		{"TK95 a 60 Hz", ModelTK95, TKULA60Hz}, {"TK90X Ferranti", ModelTK90X, TKULAFerranti}};
	for (auto &c : C)
	{
		CSpectrum *s = new CSpectrum (&K, &T);
		s->SetModelROM (ModelTK90X, Rom);
		s->SetModelROM (ModelTK95, Rom);
		s->SetTKULA (c.ULA);
		s->SetModel (c.Model);

		static bool Painted[SCREEN_WIDTH * SCREEN_HEIGHT];
		memset (Painted, 0, sizeof Painted);
		for (int t = s->m_nFirstBorderUpdate & ~3; t < s->m_nLastBorderUpdate; t += 4)
		{
			int p = s->m_States2Border[t];
			if (p >= 0)
			{
				for (int i = 0; i < 8; i++) Painted[p + i] = true;
			}
		}
		// las líneas que el modelo no tiene (TK a 60 Hz) las rellena RunFrame
		const TMachineTiming *Tm = s->m_pTiming;
		int nTop = BORDER_TOP - Tm->nUpBorderWidth;
		int nBottom = BORDER_BOTTOM - (Tm->nScanLines - Tm->nUpBorderWidth - 192);
		unsigned nMissing = 0;
		for (int y = 0; y < SCREEN_HEIGHT; y++)
		{
			if ((nTop > 0 && y < nTop) || (nBottom > 0 && y >= SCREEN_HEIGHT - nBottom))
			{
				continue;
			}
			for (int x = 0; x < SCREEN_WIDTH; x++)
			{
				bool bPaper = y >= BORDER_TOP && y < BORDER_TOP + 192 && x >= BORDER_LEFT && x < BORDER_LEFT + 256;
				if (!bPaper && !Painted[y * SCREEN_WIDTH + x])
				{
					nMissing++;
				}
			}
		}
		char What[80];
		snprintf (What, sizeof What, "%s: todo el borde se pinta (%u píxeles sin pintar)", c.pName, nMissing);
		chk (What, nMissing == 0);
		delete s;
	}

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
