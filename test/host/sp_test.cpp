// Snapshots .SP (emulador «Spectrum» de Pedro Gimeno y VgaSpec): uno de 48K,
// uno de 16K y la variante con longitud y dirección 0, que lleva una ROM de
// 16K delante de los 48K de RAM (se mete como cartucho del Interface 2).
// Se comprueban registros, interrupciones, borde, memoria y modelo, y que una
// firma o un tamaño que no cuadran se rechazan.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o sp_test sp_test.cpp ../../src/snapshot.cpp $CORE roms_host.S
#define private public
#define protected public
#include "spectrum.h"
#include "snapshot.h"
#undef private
#undef protected
#include <stdio.h>
#include <string.h>
#include <vector>

int fails = 0;
void chk (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) fails++;
}

static void W (std::vector<u8> &v, unsigned nOffset, u16 nValue)
{
	v[nOffset] = nValue & 0xFF;
	v[nOffset + 1] = nValue >> 8;
}

// cabecera de 38 bytes; nStatus: bit 0 IFF1, bit 1 IM 2, bit 2 IFF2
static std::vector<u8> Header (u16 nLength, u16 nStart, u16 nStatus)
{
	std::vector<u8> v (38, 0);
	v[0] = 'S'; v[1] = 'P';
	W (v, 2, nLength); W (v, 4, nStart);
	W (v, 6, 0x1122); W (v, 8, 0x3344); W (v, 10, 0x5566); W (v, 12, 0x7788);	// BC DE HL AF
	W (v, 14, 0x99AA); W (v, 16, 0xBBCC);						// IX IY
	W (v, 18, 0x0102); W (v, 20, 0x0304); W (v, 22, 0x0506); W (v, 24, 0x0708);	// BC' DE' HL' AF'
	v[26] = 0x42; v[27] = 0x3F;							// R I
	W (v, 28, 0x7F00); W (v, 30, 0x8123);						// SP PC
	v[34] = 5;									// borde
	W (v, 36, nStatus);
	return v;
}

static CZXKeyboard kb;
static CTape tape;

static void CheckRegs (CSpectrum *s, const char *pWhat, u16 nPC)
{
	Z80 &c = s->m_CPU;
	char Name[96];
	snprintf (Name, sizeof Name, "%s: registros", pWhat);
	chk (Name, c.getRegBC () == 0x1122 && c.getRegDE () == 0x3344 && c.getRegHL () == 0x5566
		   && c.getRegAF () == 0x7788 && c.getRegIX () == 0x99AA && c.getRegIY () == 0xBBCC
		   && c.getRegBCx () == 0x0102 && c.getRegDEx () == 0x0304 && c.getRegHLx () == 0x0506
		   && c.getRegAFx () == 0x0708 && c.getRegI () == 0x3F && c.getRegSP () == 0x7F00
		   && c.getRegPC () == nPC);
	snprintf (Name, sizeof Name, "%s: borde", pWhat);
	chk (Name, s->m_nPortFE == 5);
}

int main ()
{
	CSpectrum *s = new CSpectrum (&kb, &tape);

	// 1. 48K: toda la RAM, IM 2 con interrupciones activas
	std::vector<u8> v = Header (0xC000, 0x4000, 0x07);
	for (unsigned i = 0; i < 0xC000; i++) v.push_back ((i * 7 + (i >> 8)) & 0xFF);
	chk ("48K: se carga", CSnapshot::Load (s, v.data (), v.size (), "juego.sp") == 0);
	chk ("48K: modelo 48K", s->GetModel () == Model48K);
	CheckRegs (s, "48K", 0x8123);
	chk ("48K: IFF1, IFF2 e IM 2", s->m_CPU.isIFF1 () && s->m_CPU.isIFF2 () && s->m_CPU.getIM () == Z80::IM2);
	bool bMem = true;
	for (unsigned a = 0x4000; a < 0x10000; a += 0x123)
		bMem &= s->ReadMemory (a) == (((a - 0x4000) * 7 + ((a - 0x4000) >> 8)) & 0xFF);
	bMem &= s->ReadMemory (0xFFFF) == ((0xBFFF * 7 + (0xBFFF >> 8)) & 0xFF);
	chk ("48K: la memoria", bMem);
	chk ("48K: sin cartucho, la ROM del 48", !s->IsCartridgeInserted () && s->ReadMemory (0x028E) == 0x2E);

	// 2. 16K: IM 1 sin interrupciones
	v = Header (0x4000, 0x4000, 0x00);
	for (unsigned i = 0; i < 0x4000; i++) v.push_back ((i & 0xFF) ^ 0x5A);
	W (v, 30, 0x6000);
	chk ("16K: se carga", CSnapshot::Load (s, v.data (), v.size (), "JUEGO.SP") == 0);
	chk ("16K: modelo 16K", s->GetModel () == Model16K);
	CheckRegs (s, "16K", 0x6000);
	chk ("16K: sin interrupciones e IM 1", !s->m_CPU.isIFF1 () && !s->m_CPU.isIFF2 () && s->m_CPU.getIM () == Z80::IM1);
	chk ("16K: la memoria", s->ReadMemory (0x4000) == 0x5A && s->ReadMemory (0x7FFF) == (0xFF ^ 0x5A));

	// 3. Con ROM: longitud y dirección 0, 16K de ROM y 48K de RAM
	v = Header (0, 0, 0x01);
	for (unsigned i = 0; i < 0x4000; i++) v.push_back (0xC9 ^ (i & 0x0F));		// la ROM
	for (unsigned i = 0; i < 0xC000; i++) v.push_back (i >> 8);			// la RAM
	chk ("con ROM: se carga", CSnapshot::Load (s, v.data (), v.size (), "rom.sp") == 0);
	chk ("con ROM: modelo 48K", s->GetModel () == Model48K);
	CheckRegs (s, "con ROM", 0x8123);
	chk ("con ROM: la ROM es un cartucho", s->IsCartridgeInserted ());
	chk ("con ROM: la ROM está en 0000", s->ReadMemory (0x0000) == 0xC9 && s->ReadMemory (0x3FFF) == (0xC9 ^ 0x0F));
	chk ("con ROM: la RAM", s->ReadMemory (0x4000) == 0 && s->ReadMemory (0x8000) == 0x40 && s->ReadMemory (0xFFFF) == 0xBF);

	// después, un .SP normal quita el cartucho
	v = Header (0x4000, 0x4000, 0x00);
	v.resize (38 + 0x4000, 0);
	CSnapshot::Load (s, v.data (), v.size (), "otro.sp");
	chk ("un .SP sin ROM quita el cartucho", !s->IsCartridgeInserted ());

	// 4. Los que no valen
	v = Header (0xC000, 0x4000, 0);
	v.resize (38 + 0xC000, 0);
	v[1] = 'X';
	chk ("firma mala: se rechaza", CSnapshot::Load (s, v.data (), v.size (), "malo.sp") != 0);
	v[1] = 'P';
	v.resize (38 + 0x8000);
	chk ("le faltan datos: se rechaza", CSnapshot::Load (s, v.data (), v.size (), "corto.sp") != 0);
	v = Header (0x4000, 0x2000, 0);
	v.resize (38 + 0x4000, 0);
	chk ("encima de la ROM: se rechaza", CSnapshot::Load (s, v.data (), v.size (), "rom.sp") != 0);

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails;
}
