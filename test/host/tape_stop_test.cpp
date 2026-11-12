// Un bloque de datos sin pausa seguido de un bloque de parada: antes de parar,
// la cinta tiene que dar el flanco final que cierra el último pulso. Sin él, el
// cargador no termina de leer el último bit (la suma de comprobación) y da el
// bloque por fallido (Shadow of the Beast, de Gremlin). Ver CTape::StopsAfter.
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

static std::vector<u8> MakeTZX (bool bStop)
{
	std::vector<u8> t = {'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1A, 1, 20};
	// bloque 0x10: pausa 0, 3 bytes (marca 0xFF y dos de datos)
	u8 b1[] = {0x10, 0x00, 0x00, 0x03, 0x00, 0xFF, 0x55, 0xAA};
	t.insert (t.end (), b1, b1 + sizeof b1);
	if (bStop)
	{
		u8 b2[] = {0x20, 0x00, 0x00};		// pausa 0: parar la cinta
		t.insert (t.end (), b2, b2 + sizeof b2);
	}
	u8 b3[] = {0x10, 0xE8, 0x03, 0x02, 0x00, 0xFF, 0x00};	// otro bloque detrás
	t.insert (t.end (), b3, b3 + sizeof b3);
	return t;
}

// Reproduce hasta que la cinta se pare y devuelve
// los flancos dados y el instante del último
static unsigned PlayUntilStop (CTape &T, u64 *pLast, unsigned *pBlock)
{
	T.Play (0);
	u64 nLast = 0;
	for (unsigned nGuard = 0; T.IsPlaying () && nGuard < 1000000; nGuard++)
	{
		nLast = T.m_nNextEdge;
		T.GetEarBit (T.m_nNextEdge);
	}
	*pLast = nLast;
	*pBlock = T.m_nBlock;
	return T.GetEdgeCount ();
}

int main (void)
{
	// Referencia: el mismo bloque como último de la cinta (ya daba el flanco final)
	std::vector<u8> Ref = {'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1A, 1, 20, 0x10, 0x00, 0x00, 0x03, 0x00, 0xFF, 0x55, 0xAA};
	CTape R;
	R.InsertTZX (Ref.data (), Ref.size (), "ref");
	u64 nRefLast; unsigned nRefBlock;
	unsigned nRefEdges = PlayUntilStop (R, &nRefLast, &nRefBlock);

	std::vector<u8> Tzx = MakeTZX (true);
	CTape T;
	T.InsertTZX (Tzx.data (), Tzx.size (), "stop");
	u64 nLast; unsigned nBlock;
	unsigned nEdges = PlayUntilStop (T, &nLast, &nBlock);

	printf ("        como último bloque: %u flancos; antes de una parada: %u flancos\n", nRefEdges, nEdges);
	chk ("antes de una parada, el bloque sin pausa da los mismos flancos que al final de la cinta (con el final)", nEdges == nRefEdges);
	chk ("la cinta se para sola en la parada, sin tocar el bloque de detrás", !T.IsPlaying () && nBlock == 2 && nEdges == nRefEdges);

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
