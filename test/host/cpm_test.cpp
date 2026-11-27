// El CP/M de LEC (boot/cpm/cpm22_lec.tzx) con la LEC conectada: se carga
// con LOAD "" desde la cinta, arranca, escribe su indicador y responde a los
// comandos (DIR lista los ficheros; uno que no existe solo se repite con
// «?»). En el 48K y en un 128K en modo 48K (menú de la ROM: «48 BASIC").
// Con la LEC en uso, el kernel hace que el borrado del PC sea el cursor a la
// izquierda (IsLECInUse).
//
// No se lee el texto (el CP/M de LEC usa letras de 4 píxeles): se cuentan
// los píxeles de tinta de la pantalla.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o cpm_test cpm_test.cpp $CORE roms_host.S
#include "testutil.h"

static bool Boot (TModel Model, const std::vector<u8> &Tape)
{
	S->SetModel (Model);
	S->SetLECConnected (TRUE);
	S->Reset ();
	run (150);
	if (Model != Model48K)
	{
		for (int i = 0; i < 3; i++) press ({CS, K(4,4)});	// abajo hasta «48 BASIC»
		press ({ENTER});
		run (100);
	}

	tape.InsertTZX (Tape.data (), Tape.size (), "cpm");
	S->TapeInserted ();
	S->SetTapeMode (TapeModeFlash);
	press ({K(6,3)});			// J = LOAD
	press ({SS, K(5,0)});			// "
	press ({SS, K(5,0)});			// "
	press ({ENTER});
	run (1500);

	return S->IsLECInUse ();
}

int main (void)
{
	S = new CSpectrum (&kb, &tape);

	std::vector<u8> Tape = ReadFile ("../../boot/cpm/cpm22_lec.tzx");
	chk ("la cinta del CP/M", Tape.size () > 0);
	if (Tape.size () == 0) return 1;

	static const struct { TModel Model; const char *pName; } Models[] =
	{
		{Model48K,  "48K"},
		{Model128K, "128K en modo 48K"}
	};

	for (auto &M : Models)
	{
		char Text[80];
		chk ((snprintf (Text, sizeof Text, "%s: arranca y usa la LEC", M.pName), Text), Boot (M.Model, Tape));

		unsigned nBoot = InkPixels (0, 23);
		chk ((snprintf (Text, sizeof Text, "%s: escribe su indicador", M.pName), Text), nBoot > 200);

		type ("dir");
		press ({ENTER});
		run (500);
		unsigned nDir = InkPixels (0, 23);
		chk ((snprintf (Text, sizeof Text, "%s: DIR lista los ficheros", M.pName), Text), nDir > nBoot + 250);

		type ("xyzzy");
		press ({ENTER});
		run (300);
		unsigned nBad = InkPixels (0, 23) - nDir;
		chk ((snprintf (Text, sizeof Text, "%s: un comando que no existe solo se repite", M.pName), Text),
		     nBad > 50 && nBad < 250);

		S->SetLECConnected (FALSE);
		chk ((snprintf (Text, sizeof Text, "%s: desconectada, ya no se usa", M.pName), Text),
		     !S->IsLECConnected () && !S->IsLECInUse ());
	}

	// sin la LEC, el CP/M no arranca
	S->SetModel (Model48K);
	tape.Eject ();
	tape.InsertTZX (Tape.data (), Tape.size (), "cpm");
	S->TapeInserted ();
	S->SetTapeMode (TapeModeFlash);
	run (150);
	press ({K(6,3)}); press ({SS, K(5,0)}); press ({SS, K(5,0)}); press ({ENTER});
	run (1500);
	chk ("sin LEC: no la usa", !S->IsLECInUse ());

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
