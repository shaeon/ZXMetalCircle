// Sensibilidad del ratón (mousesens=, CZXKeyboard::SetMouseSensitivity): el
// movimiento del ratón USB se multiplica por un % antes de llegar al Kempston
// Mouse, guardando lo que sobra para que despacio también avance.
#include "zxkeyboard.h"
#include <stdio.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

int main (void)
{
	CZXKeyboard K;
	K.SetMousePresent (TRUE);

	// 100 %: tal cual (X a la derecha, Y hacia arriba)
	u8 x = K.ReadMouseX (), y = K.ReadMouseY ();
	K.MouseUpdate (0, 7, 3, 0);
	chk ("100 %: tal cual", K.ReadMouseX () == (u8) (x + 7) && K.ReadMouseY () == (u8) (y - 3));

	// 50 %: 7 → 3 (sobra medio), y otro 1 → 1 (el medio de antes)
	K.SetMouseSensitivity (50);
	x = K.ReadMouseX ();
	K.MouseUpdate (0, 7, 0, 0);
	chk ("50 %: 7 son 3", K.ReadMouseX () == (u8) (x + 3));
	K.MouseUpdate (0, 1, 0, 0);
	chk ("50 %: lo que sobró se suma", K.ReadMouseX () == (u8) (x + 4));

	// 10 %: diez movimientos de 1 son 1
	K.SetMouseSensitivity (10);
	x = K.ReadMouseX ();
	for (int i = 0; i < 9; i++) K.MouseUpdate (0, 1, 0, 0);
	chk ("10 %: nueve de 1, aún nada", K.ReadMouseX () == x);
	K.MouseUpdate (0, 1, 0, 0);
	chk ("10 %: el décimo, 1", K.ReadMouseX () == (u8) (x + 1));

	// hacia atrás igual, y en Y
	x = K.ReadMouseX (); y = K.ReadMouseY ();
	for (int i = 0; i < 20; i++) K.MouseUpdate (0, -1, 1, 0);
	chk ("10 %: hacia la izquierda y abajo", K.ReadMouseX () == (u8) (x - 2) && K.ReadMouseY () == (u8) (y - 2));

	// 200 %: el doble
	K.SetMouseSensitivity (200);
	x = K.ReadMouseX ();
	K.MouseUpdate (0, 5, 0, 0);
	chk ("200 %: el doble", K.ReadMouseX () == (u8) (x + 10));

	// fuera de rango: se queda en 10-200
	K.SetMouseSensitivity (0);
	x = K.ReadMouseX ();
	for (int i = 0; i < 10; i++) K.MouseUpdate (0, 1, 0, 0);
	chk ("0 se toma como 10 %", K.ReadMouseX () == (u8) (x + 1));

	// los botones y la rueda no cambian
	K.SetMouseSensitivity (30);
	K.MouseUpdate (1, 0, 0, 0);
	chk ("botones igual", (K.ReadMouseButtons () & 0x02) == 0);

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails;
}
