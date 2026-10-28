// Puertos en los que se lee el Kempston: decodificado solo por A5 = 0 (con
// A0 = 1; con A0 = 0 gana la ULA). Juegos que no leen el 0x1F: Las Tres Luces
// de Glaurung (0xDF) y Silent Shadow (0xD3).
//
//   sed 's#"roms/#"../../roms/#' ../../src/roms.S > roms_host.S
//   g++ -O2 -std=c++17 -DWITH_BREAKPOINT_SUPPORT -I. -I../../src -I../../third_party/z80cpp \
//       -I../../third_party/mame/compat -I../../third_party/mame -o kempston_ports_test \
//       kempston_ports_test.cpp ../../src/spectrum.cpp ../../src/ay8912.cpp ../../src/audiomixer.cpp \
//       ../../src/tape.cpp ../../src/zxkeyboard.cpp ../../src/interface1.cpp ../../src/plus3disk.cpp \
//       ../../src/sdcard.cpp ../../src/spectranet.cpp ../../src/earinput.cpp ../../src/zxnet.cpp \
//       ../../third_party/mame/*.cpp ../../third_party/mame/compat/*.cpp ../../src/inflate.cpp \
//       ../../third_party/z80cpp/z80.cpp roms_host.S
#include "spectrum.h"
#include <stdio.h>

int fails = 0;
void chk (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) fails++;
}

int main ()
{
	CZXKeyboard kb;
	CTape tape;
	CSpectrum *s = new CSpectrum (&kb, &tape);
	s->SetModel (Model48K);
	kb.SetJoystickMode (JoystickKempston);
	kb.SetGamePadState (1, JOY_RIGHT | JOY_FIRE);		// 0x11 en el Kempston

	chk ("0x001F (el puerto de siempre)", s->inPort (0x001F) == 0x11);
	chk ("0x00DF (Las Tres Luces de Glaurung)", s->inPort (0x00DF) == 0x11);
	chk ("0xFFD3 (Silent Shadow: IN A,(0D3h))", s->inPort (0xFFD3) == 0x11);
	chk ("0x005F", s->inPort (0x005F) == 0x11);
	chk ("0x007F con A5 = 1: no es el Kempston", s->inPort (0x007F) != 0x11);
	chk ("0x00FF con A5 = 1: no es el Kempston", s->inPort (0x00FF) != 0x11);
	chk ("0xFEDE con A0 = 0: gana la ULA (teclado, bits 0-4 a 1)", (s->inPort (0xFEDE) & 0x1F) == 0x1F);
	chk ("0x00FE: la ULA", (s->inPort (0x00FE) & 0x1F) == 0x1F);

	kb.SetJoystickMode (JoystickSinclair1);
	chk ("sin Kempston elegido, 0x001F no es el joystick", s->inPort (0x001F) != 0x11);

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails;
}
