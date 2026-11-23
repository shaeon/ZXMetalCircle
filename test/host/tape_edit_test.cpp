// Edición de bloques de una cinta (F2 en el navegador de la cinta, ver
// CTape::DeleteBlocks y compañía): borrar, copiar, mover y renombrar, sin
// tocar el fichero hasta guardar y con vuelta atrás.
#include "tape.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

// Un bloque estándar: flag + datos + paridad
static std::vector<u8> Standard (u8 nFlag, const std::vector<u8> &Data)
{
	std::vector<u8> b = {nFlag};
	b.insert (b.end (), Data.begin (), Data.end ());
	u8 x = 0;
	for (u8 c : b) x ^= c;
	b.push_back (x);
	return b;
}

static std::vector<u8> Header (const char *pName, u16 nLength)
{
	std::vector<u8> d = {3};		// bytes
	char Name[10];
	memset (Name, ' ', 10);
	memcpy (Name, pName, strlen (pName));
	d.insert (d.end (), Name, Name + 10);
	u8 r[] = {(u8) nLength, (u8) (nLength >> 8), 0x00, 0x80, 0x00, 0x80};
	d.insert (d.end (), r, r + 6);
	return Standard (0x00, d);
}

static void AddTAP (std::vector<u8> &t, const std::vector<u8> &b)
{
	t.push_back (b.size () & 0xFF);
	t.push_back (b.size () >> 8);
	t.insert (t.end (), b.begin (), b.end ());
}

static void AddTZX (std::vector<u8> &t, const std::vector<u8> &b)
{
	u8 h[] = {0x10, 0xE8, 0x03, (u8) (b.size () & 0xFF), (u8) (b.size () >> 8)};
	t.insert (t.end (), h, h + 5);
	t.insert (t.end (), b.begin (), b.end ());
}

// "Nombre" de cada bloque: el de la cabecera o el primer byte de los datos
static std::string Order (CTape &T)
{
	std::string s;
	for (unsigned i = 0; i < T.GetBlockCount (); i++)
	{
		char Name[12];
		if (T.GetHeaderName (i, Name))
		{
			s += Name;
		}
		else
		{
			s += "d";
		}
		s += i + 1 < T.GetBlockCount () ? "," : "";
	}
	return s;
}

static bool SameAsFile (CTape &T, const std::vector<u8> &f)
{
	unsigned n;
	const u8 *p = T.GetData (&n);
	return n == f.size () && memcmp (p, f.data (), n) == 0;
}

int main (void)
{
	// TAP: A (cabecera + datos), B (cabecera + datos), un bloque suelto
	std::vector<u8> Tap;
	AddTAP (Tap, Header ("A", 2)); AddTAP (Tap, Standard (0xFF, {1, 2}));
	AddTAP (Tap, Header ("B", 2)); AddTAP (Tap, Standard (0xFF, {3, 4}));
	AddTAP (Tap, Standard (0xFF, {5}));

	CTape T;
	chk ("TAP: se mete", T.InsertTAP (Tap.data (), Tap.size (), "prueba"));
	chk ("TAP: se puede editar", T.CanEdit () == CTape::EditOK);
	chk ("cabeceras", T.IsHeader (0) && !T.IsHeader (1) && T.IsHeader (2) && !T.IsHeader (4));
	chk ("grupos: cabecera + datos = 2, suelto = 1", T.GetGroup (0) == 2 && T.GetGroup (2) == 2 && T.GetGroup (4) == 1 && T.GetGroup (1) == 1);
	chk ("sin cambios", !T.IsModified () && Order (T) == "A,d,B,d,d");

	chk ("mover B (2 bloques) delante de A", T.MoveBlocks (2, 2, 0) && Order (T) == "B,d,A,d,d");
	chk ("ya hay cambios", T.IsModified ());
	unsigned n;
	T.GetData (&n);
	chk ("mismo tamaño tras mover", n == Tap.size ());

	chk ("copiar A al final", T.CopyBlocks (2, 2, 5) && Order (T) == "B,d,A,d,d,A,d" && T.GetBlockCount () == 7);
	chk ("borrar el suelto", T.DeleteBlocks (4, 1) && Order (T) == "B,d,A,d,A,d");
	chk ("mover el primero detrás del último", T.MoveBlocks (0, 2, 6) && Order (T) == "A,d,A,d,B,d");
	chk ("mover al mismo sitio no hace nada", T.MoveBlocks (2, 2, 2) && T.MoveBlocks (2, 2, 4) && Order (T) == "A,d,A,d,B,d");
	chk ("no se borra todo", !T.DeleteBlocks (0, 6) && T.GetBlockCount () == 6);
	chk ("fuera de rango", !T.DeleteBlocks (5, 2) && !T.CopyBlocks (0, 1, 7));

	// renombrar: el nombre, relleno con espacios, y la paridad
	chk ("renombrar", T.RenameHeader (4, "JUEGO LARGO!") && Order (T) == "A,d,A,d,JUEGO LARG,d");
	chk ("no se renombran datos", !T.RenameHeader (5, "X"));
	T.RenameHeader (0, "Z");
	{
		const u8 *p = T.GetData (&n);
		const u8 *h = p + 2;		// primer bloque
		u8 x = 0;
		for (unsigned i = 0; i < 19; i++) x ^= h[i];
		chk ("paridad de la cabecera renombrada", x == 0 && h[2] == 'Z' && h[3] == ' ' && h[11] == ' ');
	}
	chk ("la cinta se puede reproducir (bloques bien indexados)", T.GetBlockCount () == 6);

	T.DiscardEdits ();
	chk ("descartar: como estaba", !T.IsModified () && SameAsFile (T, Tap) && Order (T) == "A,d,B,d,d");

	// guardar: lo de ahora es lo bueno
	T.DeleteBlocks (4, 1);
	T.KeepEdits ();
	chk ("guardado: sin cambios pendientes", !T.IsModified () && T.GetBlockCount () == 4);
	T.DiscardEdits ();
	chk ("descartar después de guardar no deshace", T.GetBlockCount () == 4);

	// renombrar como primer cambio y descartar
	T.RenameHeader (0, "OTRO");
	chk ("renombrar marca cambios", T.IsModified ());
	T.DiscardEdits ();
	char Name[12];
	chk ("descartar el renombrado", T.GetHeaderName (0, Name) && strcmp (Name, "A") == 0);

	// la posición de la cinta sigue a su bloque
	T.SeekBlock (2);			// B
	T.MoveBlocks (2, 2, 0);
	chk ("la cinta sigue en B tras moverlo", T.GetBlock () == 0);
	T.SeekBlock (3);			// datos de A
	T.DeleteBlocks (0, 2);			// borrar B (delante)
	chk ("borrar lo de delante: la cinta se corre", T.GetBlock () == 1);

	// TZX: cabecera de 10 bytes y bloques 0x10
	std::vector<u8> Tzx = {'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1A, 1, 20};
	AddTZX (Tzx, Header ("UNO", 2)); AddTZX (Tzx, Standard (0xFF, {1, 2}));
	{
		u8 Text[] = {0x30, 3, 'h', 'o', 'l'};	// texto
		Tzx.insert (Tzx.end (), Text, Text + 5);
	}
	AddTZX (Tzx, Header ("DOS", 2)); AddTZX (Tzx, Standard (0xFF, {3, 4}));
	CTape Z;
	chk ("TZX: se mete", Z.InsertTZX (Tzx.data (), Tzx.size (), "z") && Z.GetBlockCount () == 5);
	chk ("TZX: se puede editar", Z.CanEdit () == CTape::EditOK && Z.GetGroup (0) == 2 && Z.GetGroup (3) == 2);
	chk ("TZX: mover DOS delante", Z.MoveBlocks (3, 2, 0) && Order (Z) == "DOS,d,UNO,d,d");
	{
		const u8 *p = Z.GetData (&n);
		chk ("TZX: la cabecera del fichero sigue delante", n == Tzx.size () && memcmp (p, Tzx.data (), 10) == 0);
	}
	Z.RenameHeader (0, "TRES");
	Z.DiscardEdits ();
	chk ("TZX: descartar", SameAsFile (Z, Tzx));

	// TZX con saltos: no
	std::vector<u8> Jump = Tzx;
	u8 J[] = {0x23, 0x01, 0x00};
	Jump.insert (Jump.end (), J, J + 3);
	CTape S;
	S.InsertTZX (Jump.data (), Jump.size (), "s");
	chk ("TZX con saltos: no se edita", S.CanEdit () == CTape::EditJumps && !S.DeleteBlocks (0, 1) && !S.IsModified ());

	// pegar en otra cinta: de TAP a TZX (bloques 0x10 con 1 s de pausa)
	{
		CTape A, B;
		A.InsertTAP (Tap.data (), Tap.size (), "a");
		B.InsertTZX (Tzx.data (), Tzx.size (), "b");
		chk ("otra cinta, otro número", A.GetSerial () != B.GetSerial ());
		unsigned nClip;
		u8 *pClip = A.ExtractBlocks (2, 2, &nClip);		// B y sus datos
		chk ("TAP -> TZX: pegado", pClip != 0 && B.InsertBlocks (pClip, nClip, false, 0) == 1
					    && Order (B) == "B,d,UNO,d,d,DOS,d" && B.IsModified ());
		const u8 *p = B.GetData (&n);
		chk ("TAP -> TZX: bloque 0x10 con pausa de 1 s", p[10] == 0x10 && p[11] == 0xE8 && p[12] == 0x03
					    && n == Tzx.size () + (21 + 3) + (6 + 3) && memcmp (p + 13, Tap.data () + 21 + 6, 21) == 0);
		delete [] pClip;

		// de TZX a TAP: los 0x10 sí (sin el ID ni la pausa)
		CTape C;
		C.InsertTAP (Tap.data (), Tap.size (), "c");
		pClip = B.ExtractBlocks (2, 2, &nClip);			// UNO y sus datos
		chk ("TZX -> TAP: pegado al final", C.InsertBlocks (pClip, nClip, true, 5) == 1
					    && Order (C) == "A,d,B,d,d,UNO,d");
		delete [] pClip;

		// el texto (0x30) no cabe en un TAP: no se pega nada
		pClip = B.ExtractBlocks (4, 1, &nClip);
		chk ("TZX -> TAP: un bloque de texto no cabe", C.InsertBlocks (pClip, nClip, true, 0) == -1
					    && C.GetBlockCount () == 7);
		delete [] pClip;

		// la cinta sigue a su bloque
		C.SeekBlock (1);
		pClip = A.ExtractBlocks (4, 1, &nClip);
		chk ("pegar delante: la cinta se corre", C.InsertBlocks (pClip, nClip, false, 0) == 1 && C.GetBlock () == 2);
		delete [] pClip;
		C.DiscardEdits ();
		chk ("pegar y descartar", SameAsFile (C, Tap));
	}

	// expulsar con cambios: se pierden
	Z.DeleteBlocks (2, 1);
	Z.Eject ();
	chk ("expulsar olvida los cambios", !Z.IsModified ());

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
