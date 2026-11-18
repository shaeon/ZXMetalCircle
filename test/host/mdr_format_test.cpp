// Cartuchos de microdrive grabados por el Interface 1: tienen que poder
// guardarse como .MDR (IsSinclairFormat). El Interface 1 escribe unos bytes de
// más tras cada bloque y FORMAT deja un resto al dar la vuelta la cinta; nada de
// eso lo lee nadie, así que no debe obligar a guardar en .MDV.
// - FORMAT y SAVE desde BASIC: .MDR, y lo grabado se vuelve a cargar del .MDR.
// - Multiface One, save a cartridge con format+ en un cartucho nuevo: .MDR, con
//   sus ficheros, y en un Spectrum nuevo LOAD *"m";1;"juego" lo recupera.
// - Grabar y borrar en un cartucho cargado de un .MDR no estropea los demás
//   sectores (antes, cada registro grabado pisaba la cabecera del siguiente).
// - Un formato propio (otros bloques) sigue necesitando .MDV.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o mdr_format_test mdr_format_test.cpp $CORE roms_host.S
#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
#include <vector>

int fails = 0;
void chk (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) fails++;
}

static CZXKeyboard kb;
static CTape tape;
static CSpectrum *S;

static void run (int n)
{
	for (int i = 0; i < n; i++)
	{
		S->RunFrame ();
		S->GetAudio ()->ClearBuffer ();
	}
}

#define K(r,b) (((r)<<3)|(b))
#define CS K(0,0)
#define SS K(7,1)

static void press (std::initializer_list<int> keys)
{
	u8 m[8] = {0};
	for (int k : keys) m[k >> 3] |= 1 << (k & 7);
	kb.SetGPIOMatrix (m);
	run (6);
	u8 z[8] = {0};
	kb.SetGPIOMatrix (z);
	run (6);
}

static int key (char c)
{
	const char *rows[8] = {"#zxcv", "asdfg", "qwert", "12345", "09876", "poiuy", "\nlkjh", " $mnb"};
	for (int r = 0; r < 8; r++)
		for (int b = 0; b < 5; b++)
			if (rows[r][b] == c) return K(r, b);
	return -1;
}

// teclas tal cual (en modo K, una letra es su palabra clave)
static void type (const char *t)
{
	for (; *t; t++)
	{
		if (*t == '"')		press ({SS, K(5,0)});
		else if (*t == ';')	press ({SS, K(5,1)});
		else if (*t == '*')	press ({SS, K(7,4)});
		else if (*t == '~')	{ press ({CS, SS}); press ({SS, K(4,0)}); }	// FORMAT (modo E, SS+0)
		else			press ({key (*t)});
	}
}

static std::vector<u8> rd (const char *f)
{
	std::vector<u8> v;
	FILE *h = fopen (f, "rb");
	if (h == 0) { printf ("falta %s\n", f); return v; }
	int c;
	while ((c = fgetc (h)) != EOF) v.push_back (c);
	fclose (h);
	return v;
}

static std::vector<u8> IF1ROM, MF1ROM;

static void NewSpectrum (void)
{
	delete S;
	S = new CSpectrum (&kb, &tape);
	S->SetIF1ROM (IF1ROM.data ());
	S->SetMultifaceROM ((TCodeModel) 0, MF1ROM.data ());
	S->SetIF1Connected (TRUE);
	run (150);
}

static CMicrodrive *Drive (void)
{
	return S->GetInterface1 ()->GetDrive (0);
}

// ¿hay un fichero con ese nombre en el .MDR? (en la descripción de los registros)
static bool HasFile (const u8 *pMDR, unsigned nSize, const char *pName)
{
	char Name[11];
	snprintf (Name, sizeof Name, "%-10s", pName);
	for (unsigned i = 0; i + 543 <= nSize; i += 543)
	{
		const u8 *pRec = pMDR + i + 15;
		if ((pRec[0] & 0x01) == 0 && memcmp (pRec + 4, Name, 10) == 0)
		{
			return true;
		}
	}
	return false;
}

static u8 MDR[MDR_FILE_SIZE];

int main (void)
{
	IF1ROM = rd ("../../roms/if1-2.rom");
	MF1ROM = rd ("../../roms/mf1.rom");
	if (IF1ROM.size () != 8192 || MF1ROM.size () != 8192)
	{
		return 1;
	}

	// 1. FORMAT y SAVE desde BASIC
	NewSpectrum ();
	Drive ()->InsertBlank ();
	type ("~\"m\";1;\"prueba\"\n");
	run (2500);
	chk ("FORMAT: se puede guardar como .MDR", Drive ()->IsSinclairFormat ());

	type ("10p9\n");
	type ("s*\"m\";1;\"x\"\n");
	run (1500);
	chk ("SAVE: se puede guardar como .MDR", Drive ()->IsSinclairFormat ());
	unsigned nSize = Drive ()->ExportMDR (MDR, sizeof MDR);
	chk ("SAVE: el fichero está en el .MDR", HasFile (MDR, nSize, "x"));

	// se vuelve a cargar del .MDR en un Spectrum nuevo
	NewSpectrum ();
	chk ("el .MDR se carga", Drive ()->InsertMDR (MDR, nSize));
	chk ("un .MDR cargado sigue siendo .MDR", Drive ()->IsSinclairFormat ());
	type ("j*\"m\";1;\"x\"\n");
	run (1500);
	u16 nProg = S->ReadMemory (23635) | S->ReadMemory (23636) << 8;
	chk ("LOAD *: vuelve la línea 10 PRINT 9",
	     S->ReadMemory (nProg) == 0 && S->ReadMemory (nProg + 1) == 10 && S->ReadMemory (nProg + 4) == 0xF5);

	// 2. Un cartucho .MDR con varios ficheros: grabar y borrar no estropea nada
	NewSpectrum ();
	Drive ()->InsertBlank ();
	type ("~\"m\";1;\"prueba\"\n");
	run (2500);
	const char *Names[] = {"a", "b", "c", "d", "e", "f"};
	for (int i = 0; i < 6; i++)
	{
		char Line[16];
		snprintf (Line, sizeof Line, "%dp9\n", i + 1);	// cada uno, con su número de línea
		type ("a\n");				// NEW
		run (200);
		type (Line);
		type ("s*\"m\";1;\"");
		type (Names[i]);
		type ("\"\n");
		run (1500);
	}
	nSize = Drive ()->ExportMDR (MDR, sizeof MDR);
	unsigned nSectors = (nSize - 1) / 543;

	NewSpectrum ();
	Drive ()->InsertMDR (MDR, nSize);
	for (int i = 0; i < 6; i += 2)		// ERASE "m";1;"a", "c" y "e"
	{
		press ({CS, SS});
		press ({SS, K(4,3)});
		type ("\"m\";1;\"");
		type (Names[i]);
		type ("\"\n");
		run (1500);
	}
	type ("a\n");
	run (200);
	type ("10p9\n");
	for (int i = 0; i < 4; i++)
	{
		char Cmd[32];
		snprintf (Cmd, sizeof Cmd, "s*\"m\";1;\"n%d\"\n", i);
		type (Cmd);
		run (1500);
	}
	chk ("SAVE y ERASE en un .MDR: sigue siendo .MDR", Drive ()->IsSinclairFormat ());
	nSize = Drive ()->ExportMDR (MDR, sizeof MDR);
	chk ("SAVE y ERASE en un .MDR: no se pierde ningún sector", (nSize - 1) / 543 == nSectors);

	bool bAll = true;
	for (int i = 1; i < 6; i += 2)
	{
		type ("j*\"m\";1;\"");
		type (Names[i]);
		type ("\"\n");
		run (1500);
		nProg = S->ReadMemory (23635) | S->ReadMemory (23636) << 8;
		bool bOK = S->ReadMemory (nProg) == 0 && S->ReadMemory (nProg + 1) == i + 1;
		printf ("      LOAD %s: %s\n", Names[i], bOK ? "bien" : "MAL");
		bAll &= bOK;
	}
	chk ("SAVE y ERASE en un .MDR: los otros ficheros se siguen cargando", bAll);

	// 3. Multiface One: save, cartridge, format+ en un cartucho nuevo
	NewSpectrum ();
	S->SetIF1Connected (FALSE);
	type ("10p9\n");
	Drive ()->InsertBlank ();
	S->SetIF1Connected (TRUE);
	S->TriggerNMI ();
	run (100);
	const char *pMenu[] = {"s", "juego", "\n", "c", "f"};	// save, nombre, cartridge, format+
	for (const char *p : pMenu)
	{
		type (p);
		run (60);
	}
	// hasta que termina: el motor parado 6 s seguidos (entre formatear y grabar
	// se para mucho menos)
	int nFrames = 0, nStill = 0;
	while (nFrames < 6000 && nStill < 300)
	{
		run (50);
		nFrames += 50;
		nStill = nFrames > 500 && S->GetInterface1 ()->GetRunningDrive () < 0 ? nStill + 50 : 0;
	}
	printf ("      (format+ y save: %d cuadros)\n", nFrames);
	chk ("Multiface: se puede guardar como .MDR", Drive ()->IsSinclairFormat ());
	nSize = Drive ()->ExportMDR (MDR, sizeof MDR);
	chk ("Multiface: están los ficheros", HasFile (MDR, nSize, "juego") && HasFile (MDR, nSize, "juego    1"));

	// 4. Un formato propio: un bloque con preámbulo que no es de Sinclair
	CMicrodrive c;
	c.InsertBlank ();
	c.Selected ();
	for (int i = 0; i < 10; i++) c.WriteData (0);
	c.WriteData (0xFF); c.WriteData (0xFF);
	for (int i = 0; i < 1000; i++) c.WriteData (i & 0xFE);
	chk ("formato propio: necesita .MDV", !c.IsSinclairFormat ());

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails;
}
