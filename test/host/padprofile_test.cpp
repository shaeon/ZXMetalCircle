// Perfiles de mando (src/padprofile.cpp)
#include "padprofile.h"
#include "zxkeyboard.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}
#define B(n)	(1U << (n))

// El mapeo de antes de los perfiles (kernel.cpp), para compararlo
static u8 OldKnown (u32 b)
{
	u8 j = 0;
	if (b & B(15)) j |= JOY_UP;
	if (b & B(17)) j |= JOY_DOWN;
	if (b & B(18)) j |= JOY_LEFT;
	if (b & B(16)) j |= JOY_RIGHT;
	if (b & (B(9) | B(7) | B(5) | B(6)))	j |= JOY_FIRE;	// A Y LB RB
	if (b & (B(8) | B(10) | B(3) | B(4)))	j |= JOY_FIRE2;	// B X LT RT
	return j;
}
static u8 OldGeneric (u32 b)
{
	u8 j = 0;
	if (b & (B(0) | B(3) | B(4) | B(5)))	j |= JOY_FIRE;
	if (b & (B(1) | B(2) | B(6) | B(7)))	j |= JOY_FIRE2;
	return j;
}

int main (void)
{
	CPadProfile P;
	TPadOutput O;

	// 1. Por defecto, igual que antes, botón a botón (y alguna combinación)
	P.SetDefault (TRUE);
	bool bSame = true;
	for (unsigned i = 0; i < 22; i++)
	{
		P.Evaluate (B(i), 0, &O);
		if (O.nJoy != OldKnown (B(i))) bSame = false;
	}
	P.Evaluate (B(9) | B(8) | B(15), 0, &O);
	chk ("conocido por defecto: cada botón hace lo mismo que antes", bSame && O.nJoy == (JOY_FIRE | JOY_FIRE2 | JOY_UP));
	P.Evaluate (B(11), 0, &O);
	bool bSel = O.bNextJoy;
	P.Evaluate (B(14), 0, &O);
	chk ("conocido por defecto: Select cambia el joystick y Start abre el menú", bSel && O.bMenu);

	P.SetDefault (FALSE);
	bSame = true;
	for (unsigned i = 0; i < 32; i++)
	{
		P.Evaluate (B(i), 0, &O);
		if (O.nJoy != OldGeneric (B(i))) bSame = false;
	}
	P.Evaluate (B(8), 0, &O);
	bSel = O.bNextJoy;
	P.Evaluate (B(9), 0, &O);
	chk ("genérico por defecto: igual que antes; 9 joystick y 10 menú", bSame && bSel && O.bMenu);

	// 2. Un perfil escrito a mano
	const char *pFile =
		"# perfil de prueba\n"
		"nombre = matamarcianos\r\n"
		"A = disparo 2\n"
		"  X =  tecla H  \n"
		"Y = tecla SS+P\n"
		"LB = pc F5\n"
		"RB = pc Alt+F1\n"
		"L3 = pc Enter\n"
		"R3 = tecla CS+ESPACIO\n"
		"Cruz = disparo\n";		// alias de A: lo deja en disparo
	P.SetDefault (TRUE);
	chk ("perfil a mano: se lee entero", P.Parse (pFile, strlen (pFile), TRUE));
	chk ("perfil a mano: el nombre", strcmp (P.GetName (), "matamarcianos") == 0);
	P.Evaluate (B(9), 0, &O);
	chk ("perfil a mano: A (por su alias Cruz) = disparo", O.nJoy == JOY_FIRE);
	P.Evaluate (B(8), 0, &O);
	chk ("perfil a mano: lo que no dice se queda como estaba (B = disparo 2)", O.nJoy == JOY_FIRE2);
	P.Evaluate (B(10), 0, &O);
	chk ("perfil a mano: X = tecla H (fila 6, bit 4), mantenida", O.Matrix[6] == 0x10 && O.nJoy == 0);
	P.Evaluate (B(7), 0, &O);
	chk ("perfil a mano: Y = SYMBOL SHIFT + P", O.Matrix[7] == 0x02 && O.Matrix[5] == 0x01);
	P.Evaluate (B(13), 0, &O);
	chk ("perfil a mano: R3 = CAPS SHIFT + ESPACIO (BREAK)", O.Matrix[0] == 0x01 && O.Matrix[7] == 0x01);
	P.Evaluate (B(5) | B(6), 0, &O);
	chk ("perfil a mano: LB = F5 y RB = Alt+F1, al pulsar",
	     O.nPCKeys == 2 && O.PCKey[0] == 0x3E && O.PCMods[0] == 0 && O.PCKey[1] == 0x3A && O.PCMods[1] == 0x04);
	P.Evaluate (B(5), B(5), &O);
	chk ("perfil a mano: mantener LB no repite F5", O.nPCKeys == 0);
	P.Evaluate (B(10), B(10), &O);
	chk ("perfil a mano: mantener X sigue pulsando la H", O.Matrix[6] == 0x10);

	// 3. Guardar y volver a leer: igual
	char Text[2048];
	P.Format (Text, sizeof Text, TRUE);
	CPadProfile Q;
	Q.SetDefault (FALSE);			// otro punto de partida: todo lo dice el fichero
	Q.SetDefault (TRUE);
	bool bRead = Q.Parse (Text, strlen (Text), TRUE);
	bool bEqual = strcmp (Q.GetName (), P.GetName ()) == 0;
	for (unsigned i = 0; i < PAD_BUTTONS; i++)
	{
		const TPadAction &a = P.GetAction (i), &b = Q.GetAction (i);
		if (a.Type != b.Type || a.nArg != b.nArg || (a.Type != PadActNone && a.nArg2 != b.nArg2)) bEqual = false;
	}
	chk ("guardar y volver a leer: el mismo perfil", bRead && bEqual);
	chk ("lo guardado se lee bien a ojo", strstr (Text, "X = tecla H\n") && strstr (Text, "RB = pc Alt+F1\n")
	     && strstr (Text, "Y = tecla SS+P\n") && strstr (Text, "LB = pc F5\n"));

	// 4. Acciones sueltas
	TPadAction A;
	chk ("pc F10, F11, F12", CPadProfile::ParseAction ("pc F10", &A) && A.nArg == 0x43
	     && CPadProfile::ParseAction ("pc F12", &A) && A.nArg == 0x45);
	chk ("pc Alt+F5 y pc Ctrl+Shift+Esc", CPadProfile::ParseAction ("pc Alt+F5", &A) && A.nArg == 0x3E && A.nArg2 == 0x04
	     && CPadProfile::ParseAction ("pc Ctrl+Shift+Esc", &A) && A.nArg == 0x29 && A.nArg2 == 0x03);
	chk ("en inglés: fire 2, key SPACE, pc PgDn", CPadProfile::ParseAction ("fire 2", &A) && A.nArg == JOY_FIRE2
	     && CPadProfile::ParseAction ("key SPACE", &A) && A.nArg == (7 << 3)
	     && CPadProfile::ParseAction ("pc PgDn", &A) && A.nArg == 0x4E);
	chk ("acciones que no existen: no", !CPadProfile::ParseAction ("pc F13", &A) && !CPadProfile::ParseAction ("tecla ?", &A)
	     && !CPadProfile::ParseAction ("volar", &A));

	// 5. Líneas que no se entienden: se avisa y las demás valen
	const char *pBad = "A = volar\nW = disparo\nB = disparo\n";
	P.SetDefault (TRUE);
	bool bParse = P.Parse (pBad, strlen (pBad), TRUE);
	P.Evaluate (B(8), 0, &O);
	chk ("líneas malas: se avisa y las buenas se aplican (B = disparo)", !bParse && O.nJoy == JOY_FIRE);
	P.Evaluate (B(9), 0, &O);
	chk ("líneas malas: A se queda como estaba", O.nJoy == JOY_FIRE);

	// 6. Genéricos: botones por número
	const char *pGen = "11 = pc Esc\n12 = tecla 1\n";
	P.SetDefault (FALSE);
	P.Parse (pGen, strlen (pGen), FALSE);
	P.Evaluate (B(10) | B(11), 0, &O);
	chk ("genérico: 11 = Esc y 12 = la tecla 1", O.nPCKeys == 1 && O.PCKey[0] == 0x29 && O.Matrix[3] == 0x01);

	// 7. El perfil activo (activo.txt)
	char F[64];
	const char *pAct = "sinclair2 = nivel 3\r\n  kempston =  matamarcianos  \n";
	CPadProfile::ActiveFile (pAct, strlen (pAct), "kempston", F, sizeof F);
	chk ("activo.txt: kempston = matamarcianos -> kempston-matamarcianos.txt", strcmp (F, "kempston-matamarcianos.txt") == 0);
	CPadProfile::ActiveFile (pAct, strlen (pAct), "sinclair2", F, sizeof F);
	chk ("activo.txt: nombres con espacios (sinclair2-nivel 3.txt)", strcmp (F, "sinclair2-nivel 3.txt") == 0);
	CPadProfile::ActiveFile (pAct, strlen (pAct), "cursor", F, sizeof F);
	chk ("activo.txt: un tipo que no aparece -> cursor.txt", strcmp (F, "cursor.txt") == 0);
	CPadProfile::ActiveFile (0, 0, "kempston", F, sizeof F);
	chk ("sin activo.txt -> kempston.txt", strcmp (F, "kempston.txt") == 0);

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
