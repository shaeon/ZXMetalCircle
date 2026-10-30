// Teclado: informe de error de rollover (se mantiene el estado anterior) y
// Alt+V (mantener varias teclas pulsadas a la vez)
//
//   g++ -O2 -std=c++17 -I. -I../../src -o keyboard_hold_test keyboard_hold_test.cpp ../../src/zxkeyboard.cpp
#include "zxkeyboard.h"
#include <stdio.h>

int fails = 0;
void chk (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) fails++;
}

// ¿está pulsada la tecla de la fila nRow, bit nBit?
bool Pressed (CZXKeyboard &kb, unsigned nRow, unsigned nBit)
{
	return (kb.ReadPort ((u8) ~(1 << nRow)) & (1 << nBit)) == 0;
}

// V (0,4), C (0,3), R (2,3), T (2,4), O (5,1), I (5,2)
bool Victor (CZXKeyboard &kb)
{
	return    Pressed (kb, 0, 4) && Pressed (kb, 0, 3) && Pressed (kb, 2, 3)
	       && Pressed (kb, 2, 4) && Pressed (kb, 5, 1) && Pressed (kb, 5, 2);
}

bool NoKey (CZXKeyboard &kb)
{
	return (kb.ReadPort (0x00) & 0x1F) == 0x1F;
}

int main ()
{
	CZXKeyboard kb;
	kb.SetJoystickMode (JoystickNone);

	const u8 Keys[6] = {0x19, 0x0C, 0x06, 0x17, 0x12, 0x15};	// V I C T O R
	kb.UpdateFromReport (0, Keys);
	chk ("VICTOR por USB: las seis pulsadas", Victor (kb));

	const u8 Rollover[6] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
	kb.UpdateFromReport (0, Rollover);
	chk ("informe de rollover: siguen las seis pulsadas", Victor (kb));

	const u8 Empty[6] = {0, 0, 0, 0, 0, 0};
	kb.UpdateFromReport (0, Empty);
	chk ("informe vacío: se sueltan", NoKey (kb));

	unsigned n = kb.HoldKeys ("victor", 3);
	chk ("Alt+V con \"victor\": 6 teclas", n == 6);
	chk ("Alt+V: las seis pulsadas", Victor (kb));
	kb.TickHeld (); kb.TickHeld ();
	chk ("Alt+V: siguen tras 2 cuadros", Victor (kb));
	kb.TickHeld ();
	chk ("Alt+V: se sueltan tras 3 cuadros", NoKey (kb));

	n = kb.HoldKeys ("1 0", 2);
	chk ("Alt+V con \"1 0\": 1, espacio y 0", n == 3 && Pressed (kb, 3, 0) && Pressed (kb, 7, 0) && Pressed (kb, 4, 0));
	kb.TickHeld (); kb.TickHeld ();

	n = kb.HoldKeys ("?!", 5);
	chk ("Alt+V sin teclas válidas: nada", n == 0 && NoKey (kb));

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails;
}
