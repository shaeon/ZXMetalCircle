// Dos teclados USB a la vez (p. ej. el de la Pi 400 y otro enchufado): el
// informe de uno no suelta las teclas que se tienen pulsadas en el otro, aunque
// sea un informe sin teclas como el que muchos teclados repiten cada poco.
//
//   g++ -O2 -std=c++17 -I. -I$S -o multikbd_test multikbd_test.cpp $S/hostinput.cpp $S/zxkeyboard.cpp
#include "zxkeyboard.h"
#include <stdio.h>
int fails = 0;
void chk (const char *n, bool ok) { printf ("%s %s\n", ok ? "ok   " : "FALLO", n); if (!ok) fails++; }
#define KEY_A	0x04	// fila FD, bit 0
#define KEY_Q	0x14	// fila FB, bit 0
static bool Pressed (CZXKeyboard &kb, u8 nRow) { return (kb.ReadPort (nRow) & 0x01) == 0; }
int main ()
{
	CZXKeyboard kb;
	CHostInput in;			// los teclados USB llegan por aquí (hostinput.h)
	in.SetMachineInput (&kb);
	const u8 None[6] = {0}, A[6] = {KEY_A}, Q[6] = {KEY_Q}, Err[6] = {1, 1, 1, 1, 1, 1};

	in.UpdateFromKeyboard (1, 0, A);			// A en el segundo teclado
	in.UpdateFromKeyboard (0, 0, None);		// el primero repite "nada"
	chk ("A sigue pulsada tras un informe vacío del otro teclado", Pressed (kb, 0xFD));

	in.UpdateFromKeyboard (0, 0, Q);			// Q en el primero
	chk ("A y Q a la vez, cada una en un teclado", Pressed (kb, 0xFD) && Pressed (kb, 0xFB));

	in.UpdateFromKeyboard (0, 0x02, None);		// Mayúsculas en el primero: CAPS SHIFT
	chk ("Mayúsculas de un teclado con la A del otro", Pressed (kb, 0xFD) && (kb.ReadPort (0xFE) & 0x01) == 0);

	in.UpdateFromKeyboard (0, 0, Err);			// error de demasiadas teclas: se mantiene
	chk ("informe de error: nada cambia", Pressed (kb, 0xFD) && (kb.ReadPort (0xFE) & 0x01) == 0);

	in.UpdateFromKeyboard (1, 0, None);		// se suelta la A
	in.UpdateFromKeyboard (0, 0, None);
	chk ("soltadas en los dos: nada pulsado", !Pressed (kb, 0xFD) && !Pressed (kb, 0xFB));

	in.UpdateFromKeyboard (1, 0, A);
	in.ForgetKeyboard (1);				// desenchufado con la A pulsada
	chk ("desenchufado: su A se suelta", !Pressed (kb, 0xFD));

	in.UpdateFromKeyboard (0, 0, Q);
	in.UpdateFromKeyboard (1, 0, A);
	in.ForgetKeyboard (1);
	chk ("desenchufado el otro: la Q del primero sigue", Pressed (kb, 0xFB) && !Pressed (kb, 0xFD));

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails;
}
