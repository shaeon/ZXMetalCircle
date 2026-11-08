//
// padassign.cpp
//
// A qué jugador va cada mando conectado (ver padassign.h)
//
#include "padassign.h"
#include <string.h>

static boolean IsAuto (const char *p)
{
	return p == 0 || p[0] == '\0' || strcmp (p, "auto") == 0;
}

// El primer mando conectado con ese id (o cualquiera, con "auto") que no
// sea nExcept; -1 si no hay
static int Pick (unsigned nPads, const char *const Ids[], const boolean bConnected[],
		 const char *pWant, int nExcept)
{
	for (unsigned i = 0; i < nPads; i++)
	{
		if (bConnected[i] && (int) i != nExcept && (IsAuto (pWant) || strcmp (pWant, Ids[i]) == 0))
		{
			return i;
		}
	}
	return -1;
}

void AssignPads (unsigned nPads, const char *const Ids[], const boolean bConnected[],
		 const char *pPad1, const char *pPad2, const char *pExtra, boolean bPlayer2,
		 u8 Players[])
{
	// Elige primero quien tenga un mando concreto (el jugador 1 si lo tienen
	// los dos), para que "auto" no se quede con el mando elegido por el otro
	int nP1 = -1, nP2 = -1;
	if (bPlayer2 && !IsAuto (pPad2) && IsAuto (pPad1))
	{
		nP2 = Pick (nPads, Ids, bConnected, pPad2, -1);
		nP1 = Pick (nPads, Ids, bConnected, pPad1, nP2);
	}
	else
	{
		nP1 = Pick (nPads, Ids, bConnected, pPad1, -1);
		if (bPlayer2 && !IsAuto (pPad2))
		{
			nP2 = Pick (nPads, Ids, bConnected, pPad2, nP1);
		}
	}
	if (bPlayer2 && nP2 < 0 && IsAuto (pPad2))
	{
		nP2 = Pick (nPads, Ids, bConnected, "auto", nP1);
	}

	u8 nExtra =   pExtra != 0 && strcmp (pExtra, "menu") == 0 ? PAD_ASSIGN_MENU
		    : (pExtra != 0 && strcmp (pExtra, "p2") == 0 && bPlayer2 ? 1 : 0);
	for (unsigned i = 0; i < nPads; i++)
	{
		Players[i] = (int) i == nP1 ? 0 : ((int) i == nP2 ? 1 : nExtra);
	}
}
