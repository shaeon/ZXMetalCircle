// Bloques TZX que no siguen la regla de extensión (ID + longitud de 32 bits):
// los obsoletos 34 (Emulation info, 8 bytes fijos) y 40 (Snapshot, tipo y
// longitud de 3 bytes). Un bloque de datos detrás de ellos tiene que seguir
// encontrándose, también en la carga instantánea. Especificación: https://worldofspectrum.net/TZXformat.html
#define private public
#include "tape.h"
#undef private
#include <stdio.h>
#include <vector>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

static std::vector<u8> Tzx (std::vector<u8> Before)
{
	std::vector<u8> t = {'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1A, 1, 13};
	t.insert (t.end (), Before.begin (), Before.end ());
	u8 Data[] = {0x10, 0xE8, 0x03, 0x03, 0x00, 0xFF, 0x12, 0xED};	// datos: FF 12 ED
	t.insert (t.end (), Data, Data + sizeof Data);
	return t;
}

static void Check (const char *pName, std::vector<u8> Before)
{
	std::vector<u8> t = Tzx (Before);
	CTape T;
	T.InsertTZX (t.data (), t.size (), "test");
	bool bOK =    T.m_nBlocks == 2
		   && T.m_pData[T.m_BlockOffset[1]] == 0x10
		   && T.m_pData[T.m_BlockOffset[1] + 6] == 0x12;
	char What[100];
	snprintf (What, sizeof What, "%s: el bloque de datos de detrás se encuentra (%u bloques)", pName, T.m_nBlocks);
	chk (What, bOK);
	snprintf (What, sizeof What, "%s: no cuenta como desconocido", pName);
	chk (What, T.m_nUnsupported == 0);

	// La carga instantánea se lo salta y llega al bloque de datos
	T.Rewind ();
	unsigned nLength = 0;
	const u8 *pData = T.GetStandardBlock (&nLength);
	snprintf (What, sizeof What, "%s: la carga instantánea llega al bloque de datos", pName);
	chk (What, pData != 0 && nLength == 3 && pData[1] == 0x12);
}

int main (void)
{
	// 34: flags (2), refresco (1), frecuencia (2), reservados (3)
	Check ("bloque 34 (Emulation info)", {0x34, 0x63, 0x00, 0x01, 0x32, 0x00, 0x00, 0x00, 0x00});
	// 40: tipo (1), longitud (3) y la instantánea (aquí 5 bytes)
	Check ("bloque 40 (Snapshot)", {0x40, 0x00, 0x05, 0x00, 0x00, 1, 2, 3, 4, 5});
	// Uno de extensión normal, para comparar: 2B (nivel), longitud 1
	Check ("bloque 2B (Set signal level, regla de extensión)", {0x2B, 0x01, 0x00, 0x00, 0x00, 0x01});

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
