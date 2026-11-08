//
// padassign.h
//
// A qué jugador va cada mando conectado (pad1=, pad2=, padextra=). Sin
// nada de la Pi, para poder probarlo en el PC.
//
#ifndef _zx_padassign_h
#define _zx_padassign_h

#include <circle/types.h>

#define PAD_ASSIGN_MENU	0xFF		// el mando solo sirve en los menús

// Ids: "vid:pid" de cada hueco (vale cualquier cosa si no hay mando);
// bConnected: si hay mando en el hueco. pPad1, pPad2: "auto" o un id.
// pExtra: "menu", "p1" o "p2". bPlayer2: si hay jugador 2.
// Deja en Players[i] 0 (jugador 1), 1 (jugador 2) o PAD_ASSIGN_MENU.
// - jugador 1: el mando de pPad1 o, con "auto", el primero conectado que no
//   sea el elegido expresamente para el jugador 2; si el elegido no está, no
//   tiene mando. Si los dos eligen un mando concreto (aunque sea el mismo
//   modelo), el jugador 1 elige primero;
// - jugador 2 (solo con bPlayer2): el de pPad2 o, con "auto", el siguiente;
// - los demás: pExtra ("p2" sin jugador 2 cuenta como "p1").
void AssignPads (unsigned nPads, const char *const Ids[], const boolean bConnected[],
		 const char *pPad1, const char *pPad2, const char *pExtra, boolean bPlayer2,
		 u8 Players[]);

#endif
