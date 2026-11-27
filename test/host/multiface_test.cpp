// El Multiface con sus ROMs de roms/: MF1 en el 48K, MF128 en el 128K y MF3
// en el +2A. La NMI (botón rojo) tiene que sacar el menú (una línea
// amarilla y otra roja abajo) y R (return) volver a lo que había, con el
// Multiface fuera. Mientras espera una tecla, el MF no está paginado: copia
// a la RAM una rutina que quita su ROM (IN 0x1F) para leer el teclado con
// la del Spectrum y la vuelve a poner (IN 0x9F); por eso se mira si ha
// entrado en algún momento, no si sigue dentro.
//
// También: sin ROM para el modelo no hay Multiface, y con multiface=0
// tampoco.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o multiface_test multiface_test.cpp $CORE roms_host.S
#include "testutil.h"

static bool s_bEntered;

static void RunWatching (int n)
{
	for (int i = 0; i < n; i++)
	{
		run (1);
		s_bEntered = s_bEntered || S->IsMultifacePaged ();
	}
}

int main (void)
{
	S = new CSpectrum (&kb, &tape);

	static const char *ROMs[3] = {ROMDIR "mf1.rom", ROMDIR "mf128.rom", ROMDIR "mfplus3.rom"};
	std::vector<u8> MF[3];
	for (int i = 0; i < 3; i++)
	{
		MF[i] = ReadFile (ROMs[i]);
		chk ("ROM del Multiface de 8 KB", MF[i].size () == 0x2000);
		if (MF[i].size () != 0x2000) return 1;
	}
	const u8 *pPlus2A = LoadROM ("plus2a", 4);
	chk ("ROM del +2A", pPlus2A != 0);
	if (pPlus2A == 0) return 1;
	S->SetModelROM (ModelPlus2A, pPlus2A);

	// sin las ROMs del MF: la NMI no lo saca
	S->SetModel (Model48K);
	run (150);
	chk ("48K sin ROM del MF: no disponible", !S->IsMultifaceAvailable ());

	for (int i = 0; i < 3; i++)
	{
		S->SetMultifaceROM ((TCodeModel) i, MF[i].data ());
	}

	static const struct { TModel Model; const char *pName; } Models[] =
	{
		{Model48K,    "MF1 (48K)"},
		{Model128K,   "MF128 (128K)"},
		{ModelPlus2A, "MF3 (+2A)"}
	};

	for (auto &M : Models)
	{
		char Text[80];
		S->SetModel (M.Model);
		run (150);
		if (M.Model != Model48K)
		{
			press ({ENTER});		// del menú de la ROM al BASIC
			run (100);
		}
		chk ((snprintf (Text, sizeof Text, "%s: disponible", M.pName), Text), S->IsMultifaceAvailable ());

		unsigned nInk = InkPixels (22, 23);
		bool bBasic = PaperCells (22, 7) == 32 && PaperCells (23, 7) == 32;

		s_bEntered = false;
		S->TriggerNMI ();
		RunWatching (100);
		chk ((snprintf (Text, sizeof Text, "%s: la NMI lo pagina", M.pName), Text), s_bEntered);
		chk ((snprintf (Text, sizeof Text, "%s: menú abajo (amarillo y rojo)", M.pName), Text),
		     PaperCells (22, 6) == 32 && PaperCells (23, 2) == 32);

		press ({key ('r')});
		run (50);
		chk ((snprintf (Text, sizeof Text, "%s: R vuelve a lo de antes", M.pName), Text),
		     bBasic && PaperCells (22, 7) == 32 && PaperCells (23, 7) == 32 && InkPixels (22, 23) == nInk);
		chk ((snprintf (Text, sizeof Text, "%s: y el MF queda fuera", M.pName), Text), !S->IsMultifacePaged ());
		chk ((snprintf (Text, sizeof Text, "%s: con la ROM del Spectrum abajo", M.pName), Text),
		     S->ReadMemory (0x0066) != MF[(int) MachineTiming[M.Model].CodeModel][0x0066]
		     || S->ReadMemory (0x0067) != MF[(int) MachineTiming[M.Model].CodeModel][0x0067]);
	}

	// multiface=0
	S->SetMultifaceEnabled (FALSE);
	S->SetModel (Model48K);
	run (150);
	chk ("multiface=0: no disponible", !S->IsMultifaceAvailable ());
	s_bEntered = false;
	S->TriggerNMI ();
	RunWatching (50);
	chk ("multiface=0: la NMI no lo pagina", !s_bEntered);

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
