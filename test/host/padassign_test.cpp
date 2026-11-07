// Reparto de los mandos entre los jugadores (src/padassign.cpp)
#include "padassign.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

static const u8 M = PAD_ASSIGN_MENU;
static bool Is (const u8 *p, u8 a, u8 b, u8 c, u8 d)	{ return p[0] == a && p[1] == b && p[2] == c && p[3] == d; }

int main (void)
{
	const char *Ids[4] = {"045e:028e", "054c:09cc", "045e:028e", "2dc8:6101"};	// Xbox, PS4, Xbox, 8BitDo
	boolean All[4] = {TRUE, TRUE, TRUE, TRUE};
	boolean Two[4] = {TRUE, TRUE, FALSE, FALSE};
	u8 P[4];

	AssignPads (4, Ids, Two, "auto", "auto", "p1", FALSE, P);
	chk ("sin jugador 2: los dos mandos, del jugador 1 (como antes)", P[0] == 0 && P[1] == 0);

	AssignPads (4, Ids, Two, "auto", "auto", "p1", TRUE, P);
	chk ("con jugador 2 y auto: el primero al 1 y el segundo al 2", P[0] == 0 && P[1] == 1);

	AssignPads (4, Ids, Two, "054c:09cc", "auto", "p1", TRUE, P);
	chk ("jugador 1 = el de PS4: el de PS4 al 1 y el otro al 2", P[1] == 0 && P[0] == 1);

	AssignPads (4, Ids, Two, "auto", "045e:028e", "p1", TRUE, P);
	chk ("jugador 2 = el de Xbox: el de Xbox al 2 y el otro al 1", P[0] == 1 && P[1] == 0);

	AssignPads (4, Ids, All, "045e:028e", "045e:028e", "menu", TRUE, P);
	chk ("dos mandos iguales (Xbox) para los dos: el primero al 1, el segundo al 2; los demás, menús",
	     Is (P, 0, M, 1, M));

	AssignPads (4, Ids, All, "auto", "auto", "menu", TRUE, P);
	chk ("cuatro mandos, extras solo menús", Is (P, 0, 1, M, M));
	AssignPads (4, Ids, All, "auto", "auto", "p1", TRUE, P);
	chk ("cuatro mandos, extras al jugador 1", Is (P, 0, 1, 0, 0));
	AssignPads (4, Ids, All, "auto", "auto", "p2", TRUE, P);
	chk ("cuatro mandos, extras al jugador 2", Is (P, 0, 1, 1, 1));
	AssignPads (4, Ids, All, "auto", "auto", "p2", FALSE, P);
	chk ("extras al jugador 2 sin jugador 2: van al 1", Is (P, 0, 0, 0, 0));

	AssignPads (4, Ids, Two, "2dc8:6101", "auto", "menu", TRUE, P);
	chk ("el mando elegido para el 1 no está: el 1 sin mando, el primero al 2, el otro menús",
	     P[0] == 1 && P[1] == M);

	boolean Gap[4] = {FALSE, TRUE, FALSE, TRUE};
	AssignPads (4, Ids, Gap, "auto", "auto", "menu", TRUE, P);
	chk ("huecos vacíos entre mandos: se reparten los conectados (huecos 2 y 4)", P[1] == 0 && P[3] == 1);

	// Dos mandos iguales: el segundo lleva su número (045e:028e#2)
	const char *Same[4] = {"045e:028e", "045e:028e#2", "x", "y"};
	boolean Both[4] = {TRUE, TRUE, FALSE, FALSE};
	AssignPads (4, Same, Both, "045e:028e#2", "045e:028e", "menu", TRUE, P);
	chk ("dos iguales, cruzados: el segundo (#2) al 1 y el primero al 2", P[1] == 0 && P[0] == 1);

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
