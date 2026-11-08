// Filas "Mando del jugador" de la configuración (CSettings, "*pad") con dos
// mandos iguales: el segundo lleva su número (045e:028e#2) y se puede elegir
#include "settings.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

int main (void)
{
	CSettings S;
	int n = S.Find ("pad1");
	const char Ids[2][SETTING_PAD_ID] = {"045e:028e", "045e:028e#2"};
	const char Names[2][SETTING_NAME_LEN] = {"XBOX", "XBOX 2"};
	S.SetPads (2, Ids, Names);

	chk ("empieza en automático", strcmp (S.GetValue (n), "auto") == 0);
	S.Step (n, 1);
	chk ("-> el primero (045e:028e, \"XBOX\")", strcmp (S.GetValue (n), "045e:028e") == 0 && strcmp (S.GetValueName (n), "XBOX") == 0);
	S.Step (n, 1);
	chk ("-> el segundo igual (045e:028e#2, \"XBOX 2\")", strcmp (S.GetValue (n), "045e:028e#2") == 0 && strcmp (S.GetValueName (n), "XBOX 2") == 0);
	S.Step (n, 1);
	chk ("-> vuelve a automático", strcmp (S.GetValue (n), "auto") == 0);
	S.Step (n, -1);
	chk ("<- desde automático, el segundo", strcmp (S.GetValue (n), "045e:028e#2") == 0);
	S.SetPads (1, Ids, Names);
	chk ("el segundo, desconectado: se ve su código \"(no conectado)\"", strstr (S.GetValueName (n), "045e:028e#2") != 0
	     && strstr (S.GetValueName (n), "no conectado") != 0);

	// La fila del mando del jugador 2 nunca en gris, como la del 1
	int n2 = S.Find ("pad2");
	chk ("\"Mando del jugador 2\" se puede elegir aunque su joystick sea \"ninguno\"",
	     strcmp (S.GetValue (S.Find ("joystick2")), "none") == 0 && S.IsApplicable (n2));

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
