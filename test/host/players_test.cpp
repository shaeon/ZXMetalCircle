// Dos jugadores (CZXKeyboard): cada mando va a un jugador (o solo a los
// menús), cada jugador tiene su joystick, y cada puerto o juego de teclas
// lee al jugador que lo tiene.
#include "zxkeyboard.h"
#include <stdio.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}
static u8 Row (CZXKeyboard &kb, u8 nHigh)	{ return kb.ReadPort (nHigh) & 0x1F; }	// 0 = pulsada

int main (void)
{
	CZXKeyboard kb;

	// Sin tocar nada: un solo jugador, todos los mandos son suyos (como antes)
	kb.SetJoystickMode (JoystickKempston);
	kb.SetGamePadState (0, JOY_UP);
	kb.SetGamePadState (1, JOY_FIRE);
	chk ("por defecto: dos mandos suman en el Kempston del jugador 1", kb.ReadKempston () == (JOY_UP | JOY_FIRE));
	chk ("por defecto: no hay jugador 2", kb.GetJoystickMode2 () == JoystickNone);

	// Kempston (jugador 1) + Sinclair 2 (jugador 2)
	kb.SetJoystickMode2 (JoystickSinclair2);
	kb.SetPadPlayer (1, 1);
	kb.SetGamePadState (0, JOY_LEFT);
	kb.SetGamePadState (1, JOY_FIRE);
	chk ("Kempston + Sinclair 2: el mando 1 mueve el Kempston", kb.ReadKempston () == JOY_LEFT);
	chk ("Kempston + Sinclair 2: el mando 2 pulsa el 5 (disparo de Sinclair 2)", Row (kb, 0xF7) == (0x1F & ~0x10));
	kb.SetGamePadState (0, 0);
	chk ("Kempston + Sinclair 2: el disparo del jugador 2 no llega al Kempston", kb.ReadKempston () == 0);

	// Sinclair 1 (jugador 1) + Sinclair 2 (jugador 2): el clásico del 128K
	kb.SetJoystickMode (JoystickSinclair1);
	kb.SetGamePadState (0, JOY_FIRE);	// 0
	kb.SetGamePadState (1, JOY_UP);		// 4
	chk ("Sinclair 1 + 2: jugador 1 pulsa el 0", Row (kb, 0xEF) == (0x1F & ~0x01));
	chk ("Sinclair 1 + 2: jugador 2 pulsa el 4", Row (kb, 0xF7) == (0x1F & ~0x08));
	chk ("Sinclair 1 + 2: el Kempston no lee nada (nadie lo tiene)", kb.ReadKempston () == 0);

	// Kempston en el jugador 2
	kb.SetJoystickMode (JoystickSinclair2);
	kb.SetJoystickMode2 (JoystickKempston);
	kb.SetGamePadState (0, 0);
	kb.SetGamePadState (1, JOY_RIGHT | JOY_FIRE2);
	chk ("Kempston en el jugador 2: lee su mando", kb.ReadKempston () == (JOY_RIGHT | JOY_FIRE2));
	chk ("Kempston en el jugador 2: el jugador 1 no pulsa teclas", Row (kb, 0xF7) == 0x1F);

	// Un mando solo para los menús
	kb.SetJoystickMode (JoystickKempston);
	kb.SetJoystickMode2 (JoystickNone);
	kb.SetPadPlayer (1, PAD_MENU_ONLY);
	kb.SetGamePadState (0, 0);
	kb.SetGamePadState (1, JOY_FIRE);
	chk ("mando solo de menús: no mueve el joystick", kb.ReadKempston () == 0);
	kb.SetPadPlayer (1, 0);
	chk ("el mismo mando, otra vez del jugador 1: dispara", kb.ReadKempston () == JOY_FIRE);

	// Ratón Kempston: no si algún jugador usa Kempston
	kb.SetMousePresent (TRUE);
	kb.SetJoystickMode (JoystickSinclair1);
	kb.SetJoystickMode2 (JoystickKempston);
	chk ("ratón: no, si el jugador 2 usa Kempston", !kb.IsMouseActive ());
	kb.SetJoystickMode2 (JoystickNone);
	chk ("ratón: sí, si nadie usa Kempston", kb.IsMouseActive ());

	// Teclas del Spectrum de un mando (su perfil)
	kb.SetMousePresent (FALSE);
	kb.SetJoystickMode (JoystickKempston);
	kb.SetJoystickMode2 (JoystickNone);
	kb.SetPadPlayer (0, 0);
	u8 H[8] = {0, 0, 0, 0, 0, 0, 0x10, 0};		// la H (fila 6, bit 4)
	kb.SetPadKeys (0, H);
	chk ("teclas de un mando: la H llega al Spectrum", Row (kb, 0xBF) == (0x1F & ~0x10));
	kb.SetPadPlayer (0, PAD_MENU_ONLY);
	chk ("teclas de un mando de solo menús: no llegan", Row (kb, 0xBF) == 0x1F);
	kb.SetPadPlayer (0, 0);
	kb.SetCaptured (TRUE);
	chk ("teclas de un mando con un menú abierto: no llegan", Row (kb, 0xBF) == 0x1F);
	kb.SetCaptured (FALSE);
	u8 None[8] = {0};
	kb.SetPadKeys (0, None);
	chk ("al soltar el botón, la H se suelta", Row (kb, 0xBF) == 0x1F);

	// Qué se puede combinar
	chk ("combinar: Kempston con Sinclair 2 sí",  CZXKeyboard::CanCombine (JoystickKempston, JoystickSinclair2));
	chk ("combinar: Sinclair 1 con Sinclair 2 sí", CZXKeyboard::CanCombine (JoystickSinclair1, JoystickSinclair2));
	chk ("combinar: el mismo dos veces no",       !CZXKeyboard::CanCombine (JoystickKempston, JoystickKempston));
	chk ("combinar: Cursor con Sinclair 1 no (6, 7, 8, 0)", !CZXKeyboard::CanCombine (JoystickCursor, JoystickSinclair1)
	     && !CZXKeyboard::CanCombine (JoystickSinclair1, JoystickCursor));
	chk ("combinar: con 'ninguno' siempre",       CZXKeyboard::CanCombine (JoystickKempston, JoystickNone));

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
