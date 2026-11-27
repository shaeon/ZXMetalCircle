// El estado de la máquina a través de CMachine (machinebase.h), como lo usa
// el kernel con F8 y al cargar un snapshot: en el 48K, el 128K y el +3, con
// .Z80, .SNA y .SZX, se graba (SaveState), se carga en otra máquina
// (LoadState, por la extensión) y las dos siguen igual: la misma imagen y la
// misma memoria tras otro cuadro. Y con la memoria LEC en uso no se graba
// (CanSaveState).
//
// Lo que los formatos no guardan, y aquí se tiene en cuenta (ya era así
// antes de la capa de máquinas):
//  - en qué punto del parpadeo (FLASH) va la ULA: se iguala antes de comparar
//    (si no, el cursor del 48K puede salir encendido en una y apagado en la
//    otra);
//  - el .SNA no tiene el puerto 0x1FFD del +2A/+3: en el +3 se comprueba que
//    eso es lo único que se pierde (con la ROM y la paginación normales, la
//    memoria tiene que salir igual).
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o state_test state_test.cpp $CORE roms_host.S
#include "testutil.h"
#include <string>

static CZXKeyboard kb2;
static CTape tape2;

static bool Same (CSpectrum *a, CSpectrum *b, const char **pWhat)
{
	if (memcmp (a->GetScreenBuffer (), b->GetScreenBuffer (),
		    SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel)) != 0)
	{
		*pWhat = "imagen";
		return false;
	}
	for (unsigned n = 0; n < 0x10000; n++)
	{
		if (a->ReadMemory (n) != b->ReadMemory (n))
		{
			*pWhat = "memoria";
			return false;
		}
	}
	return true;
}

int main (void)
{
	S = new CSpectrum (&kb, &tape);
	CSpectrum *T = new CSpectrum (&kb2, &tape2);
	const u8 *pPlus3 = LoadROM ("plus3", 4);
	chk ("ROM del +3", pPlus3 != 0);
	if (pPlus3 == 0) return 1;
	S->SetModelROM (ModelPlus3, pPlus3);
	T->SetModelROM (ModelPlus3, pPlus3);

	CMachine *pMachine = S;
	chk ("tres formatos: z80, sna, szx", pMachine->GetStateFormats () == 3
		&& strcmp (pMachine->GetStateExt (0), "z80") == 0
		&& strcmp (pMachine->GetStateExt (1), "sna") == 0
		&& strcmp (pMachine->GetStateExt (2), "szx") == 0);

	static u8 Buffer[SNAPSHOT_MAX_SIZE];
	for (TModel Model : {Model48K, Model128K, ModelPlus3})
	{
		for (unsigned nFormat = 0; nFormat < pMachine->GetStateFormats (); nFormat++)
		{
			S->SetModel (Model);
			S->Reset ();
			run (120);
			type ("10 print 42\n");	// algo en la memoria y en la pantalla
			run (20);

			char Name[80];
			snprintf (Name, sizeof Name, "%s, .%s", S->GetShortName (), pMachine->GetStateExt (nFormat));
			chk ((std::string (Name) + ": se puede grabar").c_str (), pMachine->CanSaveState () == 0);
			unsigned nSize = pMachine->SaveState (nFormat, Buffer, sizeof Buffer);
			char File[16];
			snprintf (File, sizeof File, "f.%s", pMachine->GetStateExt (nFormat));
			CMachine *pOther = T;
			const char *pError = nSize > 0 ? pOther->LoadState (Buffer, nSize, File) : "vacío";
			chk ((std::string (Name) + ": se graba y se carga").c_str (), pError == 0);
			chk ((std::string (Name) + ": el mismo modelo").c_str (), T->GetModel () == Model);

			T->m_ULA.m_nFrames = S->m_ULA.m_nFrames;	// el parpadeo (ver arriba)
			T->m_ULA.m_bFlash = S->m_ULA.m_bFlash;
			if (Model == ModelPlus3 && nFormat == 1)
			{
				chk ("+3, .sna: el 0x1FFD no se guarda", T->m_Mem.GetPort1FFD () == 0
								    && S->m_Mem.GetPort1FFD () != 0);
				S->outPort (0x1FFD, 0x00);		// y sin él, todo igual
				T->outPort (0x1FFD, 0x00);
			}
			S->RunFrame (); S->GetAudio ()->ClearBuffer ();
			T->RunFrame (); T->GetAudio ()->ClearBuffer ();
			const char *pWhat = "";
			bool bSame = Same (S, T, &pWhat);
			chk ((std::string (Name) + ": sigue igual" + (bSame ? "" : std::string (" (") + pWhat + ")")).c_str (),
			     bSame);
		}
	}

	CMachine *pMachine2 = T;
	chk ("formato desconocido", pMachine2->LoadState (Buffer, 100, "f.xyz") != 0);

	// con la LEC en uso no se graba
	S->SetModel (Model48K);
	S->Reset ();
	chk ("LEC conectada", S->SetLECConnected (TRUE));
	S->outPort (0x00FD, 0x80);
	chk ("LEC en uso: no se graba", S->IsLECPaged () && pMachine->CanSaveState () != 0);
	S->SetLECConnected (FALSE);
	chk ("sin LEC, se graba", pMachine->CanSaveState () == 0);

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
