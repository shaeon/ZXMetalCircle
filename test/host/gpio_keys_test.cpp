// Teclado real (GPIO, Recreated ZX) en los menús y en modo extendido: las
// teclas del PC que se consiguen con CAPS SHIFT y en modo extendido
// (CZXKeyboard::TranslateGPIOKey), partiendo de la tecla real de la matriz.
#include "zxkeyboard.h"
#include <stdio.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

// Matriz del Spectrum: fila 3 = 1-5, fila 4 = 0-6, fila 2 = Q-T, fila 7 = SPACE...
static u8 Key (unsigned nRow, unsigned nBit)	{ return CZXKeyboard::MatrixKeyToHID (nRow, nBit); }

int main (void)
{
	u8 K1 = Key (3, 0), K2 = Key (3, 1), K3 = Key (3, 2), K4 = Key (3, 3), K5 = Key (3, 4);
	u8 K0 = Key (4, 0), K9 = Key (4, 1), K8 = Key (4, 2), K7 = Key (4, 3), K6 = Key (4, 4);
	u8 KQ = Key (2, 0), KW = Key (2, 1), KE = Key (2, 2), KSpace = Key (7, 0), KM = Key (7, 2);

	chk ("la matriz da 1, 0, Q, W, M y ESPACIO",
	     K1 == 0x1E && K0 == 0x27 && KQ == 0x14 && KW == 0x1A && KM == 0x10 && KSpace == HID_KEY_SPACE);

	// CAPS SHIFT, con y sin modo extendido
	for (int bExt = 0; bExt <= 1; bExt++)
	{
		const char *p = bExt ? " (modo extendido)" : " (menú)";
		char T[96];
		snprintf (T, sizeof T, "CS+1..4 = Inicio, Fin, RePág, AvPág%s", p);
		chk (T, CZXKeyboard::TranslateGPIOKey (K1, TRUE, bExt) == 0x4A && CZXKeyboard::TranslateGPIOKey (K2, TRUE, bExt) == 0x4D
		     && CZXKeyboard::TranslateGPIOKey (K3, TRUE, bExt) == 0x4B && CZXKeyboard::TranslateGPIOKey (K4, TRUE, bExt) == 0x4E);
		snprintf (T, sizeof T, "CS+5..8 = flechas%s", p);
		chk (T, CZXKeyboard::TranslateGPIOKey (K5, TRUE, bExt) == HID_KEY_LEFT && CZXKeyboard::TranslateGPIOKey (K6, TRUE, bExt) == HID_KEY_DOWN
		     && CZXKeyboard::TranslateGPIOKey (K7, TRUE, bExt) == HID_KEY_UP && CZXKeyboard::TranslateGPIOKey (K8, TRUE, bExt) == HID_KEY_RIGHT);
		snprintf (T, sizeof T, "CS+9 = Supr, CS+0 = Retroceso, CS+ESPACIO = ESC%s", p);
		chk (T, CZXKeyboard::TranslateGPIOKey (K9, TRUE, bExt) == 0x4C && CZXKeyboard::TranslateGPIOKey (K0, TRUE, bExt) == HID_KEY_BACKSPACE
		     && CZXKeyboard::TranslateGPIOKey (KSpace, TRUE, bExt) == HID_KEY_ESC);
	}

	// Modo extendido: teclas de función
	chk ("extendido: 1 = F1, 5 = F5, 0 = F10",
	     CZXKeyboard::TranslateGPIOKey (K1, FALSE, TRUE) == HID_KEY_F1 && CZXKeyboard::TranslateGPIOKey (K5, FALSE, TRUE) == HID_KEY_F1 + 4
	     && CZXKeyboard::TranslateGPIOKey (K0, FALSE, TRUE) == HID_KEY_F1 + 9);
	chk ("extendido: Q = F11 y W = F12 (como ESPectrum)",
	     CZXKeyboard::TranslateGPIOKey (KQ, FALSE, TRUE) == 0x44 && CZXKeyboard::TranslateGPIOKey (KW, FALSE, TRUE) == 0x45);
	chk ("extendido: las demás letras no cambian (E sigue siendo E)", CZXKeyboard::TranslateGPIOKey (KE, FALSE, TRUE) == KE);

	// Fuera del modo extendido, sin CAPS SHIFT, nada cambia
	chk ("menú sin modo extendido: 1, Q y W siguen siendo 1, Q y W",
	     CZXKeyboard::TranslateGPIOKey (K1, FALSE, FALSE) == K1 && CZXKeyboard::TranslateGPIOKey (KQ, FALSE, FALSE) == KQ
	     && CZXKeyboard::TranslateGPIOKey (KW, FALSE, FALSE) == KW);
	chk ("extendido: CS+Q también es F11", CZXKeyboard::TranslateGPIOKey (KQ, TRUE, TRUE) == 0x44);

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
