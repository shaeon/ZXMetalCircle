// Lector de ZIP: directorio central, carpetas, entradas guardadas y con
// deflate, comprobación del CRC (data/prueba.zip)
//
//   g++ -O2 -std=c++17 -I. -I../../src -o zip_test zip_test.cpp ../../src/zipfile.cpp ../../src/inflate.cpp
#include "zipfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fails = 0;
void chk (const char *pName, bool bOK) { printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName); if (!bOK) fails++; }

int main ()
{
	static u8 Zip[16384], Out[8192];
	FILE *f = fopen ("data/prueba.zip", "rb");
	if (f == 0) { printf ("falta data/prueba.zip\n"); return 1; }
	unsigned n = fread (Zip, 1, sizeof Zip, f);
	fclose (f);

	CZipFile Z;
	chk ("abre el ZIP", Z.Open (Zip, n));
	chk ("tres entradas", Z.GetCount () == 3);
	chk ("la primera es una carpeta", Z.IsDirectory (0));
	chk ("nombre sin la ruta", strcmp (Z.GetBaseName (1), "Juego.tap") == 0);
	chk ("con deflate: 5000 bytes y CRC correcto", Z.Extract (1, Out, sizeof Out) == 5000);
	chk ("sin comprimir: 700 bytes y CRC correcto", Z.Extract (2, Out, sizeof Out) == 700);
	chk ("no cabe: error", Z.Extract (1, Out, 100) == -1);

	// estropear un byte de los datos de Disco.dsk (tras su cabecera local)
	unsigned nPos = 0;
	for (unsigned i = 0; i + 9 < n; i++)
		if (memcmp (Zip + i, "Disco.dsk", 9) == 0) { nPos = i + 9 + 50; break; }
	Zip[nPos] ^= 0xFF;
	CZipFile Z2;
	chk ("datos estropeados: el CRC lo detecta", Z2.Open (Zip, n) && Z2.Extract (2, Out, sizeof Out) == -1);

	chk ("no es un ZIP: no abre", !Z.Open ((const u8 *) "hola, esto no es un zip, solo texto", 35));

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails;
}
