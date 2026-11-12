// ULAplus: solo sus puertos (0xBF3B registro, 0xFF3B datos). Antes respondía
// también a los de la divMMC (0xE3, 0xEB) y a los del reloj de esxDOS (0x7C3B,
// 0x7D3B), y esxDOS dejaba el modo ULAplus encendido al salir de la NMI.
#define private public		// para poner el contador de T-states (como floatbus_test)
#include "spectrum.h"
#undef private
#include <stdio.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

int main (void)
{
	CZXKeyboard Keyboard;
	CTape Tape;
	CSpectrum *s = new CSpectrum (&Keyboard, &Tape);
	s->SetULAplusEnabled (TRUE);

	// Lo que hace esxDOS con la tarjeta: bloques con OTIR por 0xEB, con el
	// contador B en la parte alta del puerto y los datos del sector como valor.
	// Con B = 0x10 (A14 = 0) un dato 0x40 se tomaba como "elegir el registro de
	// modo", y con B = 0x50 (A14 = 1) un dato 0x01, como "activarlo"
	s->outPort (0x10EB, 0x40);
	s->outPort (0x50EB, 0x01);
	s->outPort (0x10E3, 0x40);
	s->outPort (0x50E3, 0x01);
	chk ("los puertos de la divMMC (0xE3, 0xEB) no activan ULAplus", !s->IsULAplusActive ());

	// Lo que hace esxDOS al salir del navegador de la NMI: apaga ULAplus y
	// luego escribe en su reloj (0x7C3B, 0x7D3B), que antes lo volvía a encender
	s->outPort (0xBF3B, 0x40);
	s->outPort (0xFF3B, 0x00);
	s->outPort (0x7C3B, 0x01);
	s->outPort (0x7D3B, 0x01);
	chk ("los puertos del reloj de esxDOS (0x7C3B, 0x7D3B) no tocan ULAplus", !s->IsULAplusActive ());

	// Los de verdad: registro 0x40 (modo) y dato 1 = activo
	s->outPort (0xBF3B, 0x40);
	s->outPort (0xFF3B, 0x01);
	chk ("0xBF3B (modo) y 0xFF3B (1) activan ULAplus", s->IsULAplusActive ());
	s->outPort (0xBF3B, 0x40);
	s->outPort (0xFF3B, 0x00);
	chk ("y con 0 lo desactivan", !s->IsULAplusActive ());

	// Desactivado en la configuración: nada
	s->SetULAplusEnabled (FALSE);
	s->outPort (0xBF3B, 0x40);
	s->outPort (0xFF3B, 0x01);
	chk ("con ULAplus desactivado, sus puertos no hacen nada", !s->IsULAplusActive ());

	// La paleta, como la cargan los programas con OTIR: el registro por 0xBF3B y
	// el dato por un puerto cuya parte alta va bajando (0xFF3B, 0xFE3B... 0xC03B).
	// Con la decodificación exacta solo llegaba la primera entrada
	s->SetULAplusEnabled (TRUE);
	for (unsigned i = 0; i < 64; i++)
	{
		s->outPort (0xBF3B, i);
		s->outPort ((uint16_t) (((0xFF - i) << 8) | 0x3B), (u8) (i * 3 + 1));
	}
	bool bAll = true;
	for (unsigned i = 0; i < 64; i++)
	{
		bAll = bAll && s->m_ULAplusPalette[i] == (u8) (i * 3 + 1);
	}
	chk ("las 64 entradas de la paleta, con la parte alta del puerto bajando (OTIR)", bAll);
	s->outPort (0xA03B, 5);					// registro por un espejo (A15=1, A14=0)
	chk ("el registro por un espejo (0xA03B) y el dato leído por otro (0xC23B)", s->inPort (0xC23B) == 5 * 3 + 1);
	s->outPort (0xBF3B, 0x40);
	s->outPort (0x7C3B, 0x01);				// el reloj de esxDOS: A15 = 0
	chk ("0x7C3B (A15 = 0) no escribe en ULAplus", !s->IsULAplusActive ());

	// Contención: los puertos de ULAplus, como los de la ULA aunque sean impares.
	// Tres Spectrum iguales llevados al mismo T-state, dentro del papel (donde la
	// ULA hace esperar), y un OUT a un puerto distinto en cada uno
	unsigned Cost[3];
	const uint16_t Ports[3] = {0xFFFE, 0xFF3B, 0xFF3D};	// ULA, ULAplus, impar cualquiera
	for (unsigned n = 0; n < 3; n++)
	{
		CSpectrum *t = new CSpectrum (&Keyboard, &Tape);
		t->SetULAplusEnabled (TRUE);
		t->m_nTstates = 14336 + 224 * 8 + 2;		// 48K: línea 8 del papel, en la zona contenida
		int nStart = t->m_nTstates;
		t->outPort (Ports[n], 0);
		Cost[n] = (unsigned) (t->m_nTstates - nStart);
		delete t;
	}
	printf ("        OUT a 0xFFFE: %u, a 0xFF3B: %u, a 0xFF3D: %u t-states\n", Cost[0], Cost[1], Cost[2]);
	chk ("un OUT a 0xFF3B (ULAplus) tarda como uno a la ULA (0xFFFE)", Cost[1] == Cost[0]);
	chk ("y más que a un puerto impar cualquiera (0xFF3D)", Cost[1] > Cost[2]);

	// Con ULAplus desactivado, 0xFF3B es un puerto impar más
	{
		CSpectrum *t = new CSpectrum (&Keyboard, &Tape);
		t->SetULAplusEnabled (FALSE);
		t->m_nTstates = 14336 + 224 * 8 + 2;
		int nStart = t->m_nTstates;
		t->outPort (0xFF3B, 0);
		unsigned nCost = (unsigned) (t->m_nTstates - nStart);
		chk ("con ULAplus desactivado, 0xFF3B tarda como un impar cualquiera", nCost == Cost[2]);
	}

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
