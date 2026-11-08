// ULAplus: solo sus puertos (0xBF3B registro, 0xFF3B datos). Antes respondía
// también a los de la divMMC (0xE3, 0xEB) y a los del reloj de esxDOS (0x7C3B,
// 0x7D3B), y esxDOS dejaba el modo ULAplus encendido al salir de la NMI.
#include "spectrum.h"
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

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
