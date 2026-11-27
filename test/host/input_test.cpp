// La entrada (hostinput.h y zxkeyboard.h) con una secuencia larga y fija de
// pulsaciones, como llegan del USB: dos teclados, informes de error
// (demasiadas teclas), Alt (atajos del emulador), teclas de función, menú
// abierto o no (capturado), cambios de joystick, de los cursores y de la
// tecla de disparo, mandos y teclas inyectadas. De cada paso se anota todo lo
// que sale: las 8 filas del teclado del Spectrum, el Kempston, el Fuller, la
// cola de teclas del emulador y la tecla de navegación mantenida. La huella
// de todo eso es la que daba el teclado antes de separar la parte genérica
// (CHostInput) de la del Spectrum (CZXKeyboard), en la fase 3d.
//
//   g++ -O2 -std=c++17 -I. -I$S -o input_test input_test.cpp $S/hostinput.cpp $S/zxkeyboard.cpp
#include "zxkeyboard.h"
#ifndef OLD_API
#include "hostinput.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define EXPECTED	0x15466ffa02b84323ULL		// huella con el teclado de antes de la fase 3d

static int fails = 0;
static void chk (const char *n, bool ok) { printf ("%s %s\n", ok ? "ok   " : "FALLO", n); if (!ok) fails++; }

static uint64_t s_nHash = 1469598103934665603ULL;
static void Add (uint64_t n) { for (int i = 0; i < 8; i++) { s_nHash ^= (n >> (8 * i)) & 0xFF; s_nHash *= 1099511628211ULL; } }

static uint32_t s_nSeed = 12345;
static unsigned Rand (unsigned n) { s_nSeed = s_nSeed * 1103515245 + 12345; return (s_nSeed >> 16) % n; }

int main ()
{
	CZXKeyboard kb;
#ifdef OLD_API
	CZXKeyboard &in = kb;
#else
	CHostInput in;
	in.SetMachineInput (&kb);
#endif
	// teclas que importan: letras, números, Enter, Espacio, Borrar, Esc, Tab,
	// Bloq Mayús, símbolos, cursores, RePág/AvPág, teclado numérico, F1-F12,
	// Pausa y los errores 1-3
	static const u8 Keys[] = {0x04, 0x05, 0x14, 0x1D, 0x1E, 0x27, 0x28, 0x2C, 0x2A, 0x29, 0x2B, 0x39,
				  0x2D, 0x2E, 0x33, 0x34, 0x36, 0x37, 0x38, 0x4F, 0x50, 0x51, 0x52, 0x4B, 0x4E,
				  0x59, 0x61, 0x62, 0x58, 0x3A, 0x3B, 0x45, 0x48, 0x01, 0x02, 0x00, 0x00};
	unsigned nEvents = 0;
	for (unsigned nStep = 0; nStep < 20000; nStep++)
	{
		unsigned nAction = Rand (100);
		if (nAction < 70)
		{
			u8 Raw[6] = {0, 0, 0, 0, 0, 0};
			unsigned nKeys = Rand (4);
			for (unsigned i = 0; i < nKeys; i++) Raw[i] = Keys[Rand (sizeof Keys)];
			u8 nMods = Rand (3) == 0 ? (u8) Rand (256) : 0;
			unsigned nKbd = Rand (3);			// 2 = el informe ya juntado
			if (nKbd == 2) in.UpdateFromReport (nMods, Raw);
			else in.UpdateFromKeyboard (nKbd, nMods, Raw);
		}
		else if (nAction < 73)	in.ForgetKeyboard (Rand (2));
		else if (nAction < 78)	in.SetCaptured (Rand (2) == 0);
		else if (nAction < 81)	kb.SetJoystickMode ((TJoystickMode) Rand (JoystickModeCount));
		else if (nAction < 83)	kb.SetCursorMode ((TCursorMode) Rand (3));
		else if (nAction < 85)	kb.SetKbdFire ((TKbdFire) Rand (3));
		else if (nAction < 90)	kb.SetGamePadState (Rand (2), (u8) Rand (64));
		else if (nAction < 92)	in.InjectKeyEvent ((u8) Rand (0x60), (u8) Rand (256));
		else if (nAction < 94)	kb.SetBackspaceAsLeft (Rand (2) == 0);
		// lo que sale
		for (unsigned r = 0; r < 8; r++) Add (kb.ReadPort ((u8) ~(1 << r)));
		Add (kb.ReadPort (0x00));
		Add (kb.ReadKempston ());
		Add (kb.ReadFuller ());
		Add (in.GetHeldNavKey ());
		Add (in.IsCaptured ());
		if (Rand (4) == 0)
		{
			TKeyEvent Event;
			while (in.GetKeyEvent (&Event))
			{
				Add (0x10000 | (Event.nKey << 8) | Event.nModifiers);
				nEvents++;
			}
		}
	}
	printf ("huella %016llx (%u teclas a la cola del emulador)\n", (unsigned long long) s_nHash, nEvents);
	chk ("la misma huella que el teclado de antes", s_nHash == EXPECTED);

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
