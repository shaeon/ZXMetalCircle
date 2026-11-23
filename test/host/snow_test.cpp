// Efecto nieve del 16K/48K (CSpectrum::Snow): según en qué fase de los 8
// t-states de la ULA cae el T4 de la lectura de una instrucción con I entre
// 0x40 y 0x7F, el primer par se lee con la fila de R (nieve) o el segundo
// repite el primero (doble). Tabla de fases de Weiv (2022).
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

static CZXKeyboard K;
static CTape T;

// Un Spectrum al principio del cuadro con la pantalla llena de bytes distintos
static CSpectrum *Prepare (TModel Model, u8 nI, u8 nR, bool bSnow)
{
	CSpectrum *s = new CSpectrum (&K, &T);
	s->SetModel (Model);
	s->SetSnowEnabled (bSnow);
	for (unsigned i = 0; i < 0x1B00; i++)
	{
		s->m_pScreenPage[i] = (u8) (i * 7 + (i >> 8));
	}
	s->m_nStep = 0;
	s->m_CPU.setRegI (nI);
	s->m_CPU.setRegR (nR);
	return s;
}

// El M1 (en 0x8000, sin contención) acaba con su T4 en el t-state nT4
static void FetchAt (CSpectrum *s, int nT4)
{
	s->m_nTstates = nT4 + 1 - 4;
	s->fetchOpcode (0x8000);
}

static bool SameCell (CSpectrum *s, unsigned nA, unsigned nB)
{
	TPixel *p = s->m_Screen;
	return memcmp (&p[s->m_BufAddr[s->m_StepAddr[nA]]], &p[s->m_BufAddr[s->m_StepAddr[nB]]], 8 * sizeof (TPixel)) == 0;
}

int main (void)
{
	const int nLine0 = 14335;	// fase 0 del primer par de la línea 0 (48K)

	// nieve: T4 en la fase 2 del primer par de la línea 0 → la celda 0 se lee
	// de (0 & ~0x7F) | R+1 y su atributo de (0x1800 & ~0x7F) | R+1
	CSpectrum *s = Prepare (Model48K, 0x40, 5, true);
	FetchAt (s, nLine0 + 2);
	chk ("nieve: la celda 0 ya dibujada", s->m_nStep == 1);
	chk ("nieve: el byte de la fila R+1", s->m_nLastByte == s->m_pScreenPage[6]);
	chk ("nieve: el atributo de la fila R+1", s->m_nLastAttr == s->m_pScreenPage[0x1806]);
	delete s;

	// la línea 5, segundo grupo de 16 píxeles (celdas 2 y 3), R = 0x7F: la fila
	// es R+1 = 0x00 (7 bits)
	s = Prepare (Model48K, 0x7F, 0x7F, true);
	FetchAt (s, nLine0 + 5 * 224 + 8 + 2);
	unsigned nAddr = s->m_StepAddr[5 * 32 + 2];
	chk ("nieve: otra línea y otro par", s->m_nStep == 5 * 32 + 3
					 && s->m_nLastByte == s->m_pScreenPage[nAddr & ~0x7F]);
	delete s;

	// doble: T4 en la fase 4 → la celda 3 repite la 2
	s = Prepare (Model48K, 0x40, 5, true);
	FetchAt (s, nLine0 + 8 + 4);
	chk ("doble: la celda 3 repite la 2", s->m_nStep == 4 && SameCell (s, 2, 3));
	delete s;

	// las demás fases, nada
	static const int Phases[] = {0, 1, 3, 5, 6, 7};
	for (int nPhase : Phases)
	{
		s = Prepare (Model48K, 0x40, 5, true);
		FetchAt (s, nLine0 + nPhase);
		char Text[64];
		snprintf (Text, sizeof Text, "fase %d: sin nieve", nPhase);
		chk (Text, s->m_nStep == 0);
		delete s;
	}

	// fuera del papel (borde derecho: columna 130 de la línea) nada
	s = Prepare (Model48K, 0x40, 5, true);
	FetchAt (s, nLine0 + 128 + 2);
	chk ("borde: sin nieve", s->m_nStep == 0);
	delete s;

	// sin nieve: I fuera de 0x40-0x7F, snow=0, otros modelos
	struct { const char *pName; TModel Model; u8 nI; bool bSnow; } No[] = {
		{"I = 0x3F", Model48K, 0x3F, true}, {"I = 0x80", Model48K, 0x80, true},
		{"snow=0", Model48K, 0x40, false}, {"128K (de momento)", Model128K, 0x40, true},
		{"+2A", ModelPlus2A, 0x40, true}};
	for (auto &n : No)
	{
		s = Prepare (n.Model, n.nI, 5, n.bSnow);
		FetchAt (s, nLine0 + 2);
		chk (n.pName, s->m_nStep == 0 && !s->m_bSnow == !(n.bSnow && n.Model == Model48K));
		delete s;
	}

	// el 16K también
	s = Prepare (Model16K, 0x40, 5, true);
	FetchAt (s, nLine0 + 2);
	chk ("16K: nieve", s->m_nStep == 1 && s->m_nLastByte == s->m_pScreenPage[6]);
	delete s;

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails;
}
