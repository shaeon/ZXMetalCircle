// Cartuchos del Interface 2 con test_if2.rom (16 KB: borde azul y barras de
// colores en la pantalla) y sus primeros 8 KB como cartucho pequeño (se
// rellena con 0xFF hasta 16 KB). Se admiten en el 48K, el 128K y el +2, y el
// +2A los rechaza (no tiene el conector). Al meterlo se ve en lugar de la
// ROM y sobrevive al reset; al quitarlo vuelve la ROM (el BASIC o el menú).
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o cartridge_test cartridge_test.cpp $CORE roms_host.S
#include "testutil.h"

// ¿Se ve el cartucho? Barras de colores: muchas celdas sin el papel blanco
static bool ShowsCartridge (void)
{
	unsigned nWhite = 0;
	for (unsigned nRow = 0; nRow < 24; nRow++) nWhite += PaperCells (nRow, 7);
	return nWhite < 24 * 32 / 2;
}

// ¿Se ve lo de la ROM (el BASIC del 48K o el menú de los 128K)? Casi todo
// con papel blanco
static bool ShowsROM (void)
{
	unsigned nWhite = 0;
	for (unsigned nRow = 0; nRow < 24; nRow++) nWhite += PaperCells (nRow, 7);
	return nWhite >= 24 * 32 - 64;
}

int main (void)
{
	S = new CSpectrum (&kb, &tape);

	std::vector<u8> ROM = ReadFile ("test_if2.rom");
	chk ("test_if2.rom de 16 KB", ROM.size () == 0x4000);
	if (ROM.size () != 0x4000) return 1;
	std::vector<u8> ROM8K (ROM.begin (), ROM.begin () + 0x2000);

	S->SetModelROM (ModelPlus2, LoadROM ("plus2", 2));
	const u8 *pPlus2A = LoadROM ("plus2a", 4);
	chk ("ROM del +2A", pPlus2A != 0);
	if (pPlus2A == 0) return 1;
	S->SetModelROM (ModelPlus2A, pPlus2A);

	static const struct { TModel Model; const char *pName; } Models[] =
	{
		{Model48K,   "48K"},
		{Model128K,  "128K"},
		{ModelPlus2, "+2"}
	};

	for (auto &M : Models)
	{
		char Text[80];
		S->SetModel (M.Model);
		u8 nROM0 = S->ReadMemory (0x0000);
		chk ((snprintf (Text, sizeof Text, "%s: se admite", M.pName), Text),
		     S->InsertCartridge (ROM.data (), ROM.size ()));
		run (50);
		chk ((snprintf (Text, sizeof Text, "%s: en lugar de la ROM", M.pName), Text),
		     S->ReadMemory (0x0000) == ROM[0] && S->ReadMemory (0x3FFF) == ROM[0x3FFF]
		     && S->ReadMemory (0x0010) == ROM[0x0010]);
		chk ((snprintf (Text, sizeof Text, "%s: se ve el cartucho", M.pName), Text), ShowsCartridge ());

		S->WriteMemory (0x0010, ROM[0x0010] ^ 0xFF);
		chk ((snprintf (Text, sizeof Text, "%s: no se puede escribir", M.pName), Text),
		     S->ReadMemory (0x0010) == ROM[0x0010]);

		S->Reset ();
		run (50);
		chk ((snprintf (Text, sizeof Text, "%s: sigue tras el reset", M.pName), Text),
		     S->IsCartridgeInserted () && ShowsCartridge ());

		S->EjectCartridge ();
		run (150);
		chk ((snprintf (Text, sizeof Text, "%s: al quitarlo vuelve la ROM", M.pName), Text),
		     !S->IsCartridgeInserted () && S->ReadMemory (0x0000) == nROM0);
		chk ((snprintf (Text, sizeof Text, "%s: y lo de su ROM", M.pName), Text), ShowsROM ());
	}

	// 8 KB: el resto, 0xFF
	S->SetModel (Model48K);
	chk ("8 KB: se admite", S->InsertCartridge (ROM8K.data (), ROM8K.size ()));
	run (50);
	chk ("8 KB: sus 8 KB y 0xFF detrás",
	     S->ReadMemory (0x1FFF) == ROM8K[0x1FFF] && S->ReadMemory (0x2000) == 0xFF && S->ReadMemory (0x3FFF) == 0xFF);
	chk ("8 KB: se ve el cartucho", ShowsCartridge ());
	S->EjectCartridge ();

	// lo que no vale
	chk ("vacío: no", !S->InsertCartridge (ROM.data (), 0));
	std::vector<u8> Big (0x4001, 0);
	chk ("más de 16 KB: no", !S->InsertCartridge (Big.data (), Big.size ()));
	chk ("y no queda nada metido", !S->IsCartridgeInserted ());

	S->SetModel (ModelPlus2A);
	u8 nROM0 = S->ReadMemory (0x0000);
	chk ("+2A: no tiene conector", !S->InsertCartridge (ROM.data (), ROM.size ()));
	run (10);
	chk ("+2A: sigue su ROM", !S->IsCartridgeInserted () && S->ReadMemory (0x0000) == nROM0);

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
