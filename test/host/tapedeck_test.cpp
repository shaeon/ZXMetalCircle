// El casete del Spectrum (CTapeDeck), con la ROM del 48K y una cinta .TAP
// hecha aquí: un programa de 10 líneas REM (unos 3 KB) y detrás un bloque de
// datos que nadie lee.
//
//  - Carga instantánea, rápida con aceleración de cargadores, rápida sin ella
//    y a velocidad real: el programa queda igual en memoria; la aceleración
//    adelanta los flancos de los datos (la cinta acaba antes).
//  - LOAD "" arranca la cinta (trampa de 0x0556), también tras pararla a mano.
//  - Parada automática: cuando el programa deja de leer, la cinta se para
//    sola a mitad del bloque que nadie lee.
//  - Arranque automático: un cargador que no pasa por 0x0556 (entra en
//    0x0562, como Cabal) la arranca; tras una parada manual, no.
//  - SAVE con un CTapeSaveHandler: bloques de cabecera y datos con su flag y
//    su paridad, sin pasar por la rutina de la ROM (y no mientras se graba
//    por pulsos).
//  - Grabación por pulsos de la salida MIC (SAVE sin trampa): los pulsos del
//    tono guía, el sincronismo y los bits de los dos bloques.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o tapedeck_test tapedeck_test.cpp $CORE roms_host.S
#include "testutil.h"

static std::vector<u8> TAP, Expected;

static void AddBlock (std::vector<u8> &T, const std::vector<u8> &B)
{
	u8 nParity = 0;
	for (u8 b : B) nParity ^= b;
	T.push_back ((B.size () + 1) & 0xFF);
	T.push_back ((B.size () + 1) >> 8);
	T.insert (T.end (), B.begin (), B.end ());
	T.push_back (nParity);
}

// 10 REM AAA... hasta 100 REM JJJ..., sin autoarranque
static void MakeTAP (void)
{
	for (int i = 1; i <= 10; i++)
	{
		unsigned nLength = 1 + 290 + 1;		// REM, texto, ENTER
		Expected.push_back (0); Expected.push_back (i * 10);
		Expected.push_back (nLength & 0xFF); Expected.push_back (nLength >> 8);
		Expected.push_back (0xEA);		// REM
		for (int j = 0; j < 290; j++) Expected.push_back ('A' + i - 1);
		Expected.push_back (0x0D);
	}
	unsigned n = Expected.size ();
	std::vector<u8> H = {0x00, 0x00, 'p', 'r', 'u', 'e', 'b', 'a', ' ', ' ', ' ', ' ',
			     (u8) n, (u8) (n >> 8), 0x00, 0x80, (u8) n, (u8) (n >> 8)};
	AddBlock (TAP, H);
	std::vector<u8> D = {0xFF};
	D.insert (D.end (), Expected.begin (), Expected.end ());
	AddBlock (TAP, D);
	std::vector<u8> Rest (2000, 0x55);	// un bloque de datos que nadie lee
	Rest[0] = 0xFF;
	AddBlock (TAP, Rest);
}

// El programa BASIC (de PROG a VARS)
static std::vector<u8> Program (void)
{
	u16 nProg = S->peek8 (0x5C53) | (S->peek8 (0x5C54) << 8);
	u16 nVars = S->peek8 (0x5C4B) | (S->peek8 (0x5C4C) << 8);
	std::vector<u8> v;
	for (u16 a = nProg; a < nVars; a++) v.push_back (S->peek8 (a));
	return v;
}

static void Start (TTapeMode Mode, boolean bAccel)
{
	S->SetModel (Model48K);
	S->SetTapeMode (Mode);
	S->SetLoaderAcceleration (bAccel);
	S->Reset ();
	run (150);
	tape.InsertTAP (TAP.data (), TAP.size (), "t");
	S->TapeInserted ();
}

static void TypeLoad (void)
{
	press ({K (6, 3)});		// J: LOAD
	press ({SS, K (5, 0)});		// "
	press ({SS, K (5, 0)});
	press ({ENTER});
}

// Carga con LOAD "" hasta que la cinta se para (o 300 cuadros con la carga
// instantánea): el programa, los cuadros que ha tardado y cómo ha quedado
struct TLoad { std::vector<u8> Prog; unsigned nFrames; bool bPlayed, bAutoStopped; };
static TLoad Load (TTapeMode Mode, boolean bAccel, bool bManualStop = false)
{
	TLoad L;
	Start (Mode, bAccel);
	if (bManualStop) S->TapeStop ();
	TypeLoad ();
	L.bPlayed = tape.IsPlaying ();
	unsigned n;
	for (n = 0; n < (Mode == TapeModeFlash ? 300 : 5000) && (Mode == TapeModeFlash || tape.IsPlaying ()); n++)
	{
		run (1);
	}
	L.nFrames = n;
	L.Prog = Program ();
	L.bAutoStopped = !tape.IsPlaying () && !tape.IsAtEnd ();	// a mitad del último bloque
	return L;
}

// Un cargador que entra directamente en 0x0562 (sin la trampa de 0x0556):
// 100 bytes en 0x8000, con DI y el flag 0xFF en A'
static void JumpToLoader (void)
{
	u16 nSP = S->m_CPU.getRegSP () - 2;
	S->poke8 (nSP, 0x00); S->poke8 (nSP + 1, 0x00);		// vuelta (no se llega)
	S->m_CPU.setRegSP (nSP);
	S->m_CPU.setRegIX (0x8000);
	S->m_CPU.setRegDE (100);
	S->m_CPU.setRegAFx (0xFF01);				// flag y LOAD
	S->m_CPU.setIFF1 (false);
	S->m_CPU.setIFF2 (false);
	S->m_CPU.setRegPC (0x0562);
}

class CTestSave : public CTapeSaveHandler
{
public:
	boolean SaveTapeBlock (const u8 *pBlock, unsigned nLength)
	{
		Blocks.push_back (std::vector<u8> (pBlock, pBlock + nLength));
		return TRUE;
	}
	std::vector<std::vector<u8>> Blocks;
};

static bool ParityOK (const std::vector<u8> &B)
{
	u8 n = 0;
	for (u8 b : B) n ^= b;
	return n == 0;
}

// 10 PRINT 9 y SAVE "x"
static void SaveProgram (void)
{
	press ({key ('1')}); press ({key ('0')});
	press ({key ('p')}); press ({key ('9')}); press ({ENTER});
	run (10);
	press ({key ('s')});		// SAVE
	press ({SS, K (5, 0)}); press ({key ('x')}); press ({SS, K (5, 0)});
	press ({ENTER});
}

int main (void)
{
	S = new CSpectrum (&kb, &tape);
	MakeTAP ();

	// carga en los cuatro modos
	TLoad Flash = Load (TapeModeFlash, TRUE);
	chk ("instantánea: carga el programa sin reproducir la cinta", Flash.Prog == Expected && !Flash.bPlayed
									&& !tape.IsPlaying ());

	TLoad Fast = Load (TapeModeFast, TRUE);
	chk ("rápida: LOAD \"\" arranca la cinta", Fast.bPlayed);
	chk ("rápida con aceleración: el programa", Fast.Prog == Expected);
	chk ("rápida: se para sola cuando el programa deja de leer", Fast.bAutoStopped);
	chk ("rápida: IsTapeAccelerating", (Start (TapeModeFast, TRUE), TypeLoad (), S->IsTapeAccelerating ()));

	TLoad Slow = Load (TapeModeFast, FALSE);
	chk ("rápida sin aceleración: el programa", Slow.Prog == Expected && Slow.bAutoStopped);

	TLoad Normal = Load (TapeModeNormal, TRUE);
	chk ("normal: el programa", Normal.Prog == Expected && Normal.bPlayed && Normal.bAutoStopped);
	chk ("normal: no es IsTapeAccelerating", (Start (TapeModeNormal, TRUE), TypeLoad (),
	     tape.IsPlaying () && !S->IsTapeAccelerating ()));

	// la aceleración adelanta los flancos de los datos: la cinta acaba antes
	printf ("cuadros: rápida %u, sin aceleración %u, normal %u\n", Fast.nFrames, Slow.nFrames, Normal.nFrames);
	chk ("aceleración: la cinta acaba en menos cuadros", Fast.nFrames * 10 < Slow.nFrames * 9);
	chk ("sin aceleración, como a velocidad real", Slow.nFrames == Normal.nFrames);

	// parada manual: LOAD "" la vuelve a arrancar (trampa de 0x0556)
	TLoad Manual = Load (TapeModeFast, TRUE, true);
	chk ("tras parar a mano, LOAD \"\" arranca la cinta", Manual.bPlayed && Manual.Prog == Expected);

	// arranque automático, con un cargador que no pasa por 0x0556; la cinta
	// está parada a mitad del bloque que nadie lee
	JumpToLoader ();
	run (30);
	chk ("un cargador propio arranca la cinta", tape.IsPlaying ());
	run (150);
	chk ("y no se para mientras el cargador lee (más de 2 s)", tape.IsPlaying ());

	Load (TapeModeFast, TRUE);
	S->TapeStop ();
	JumpToLoader ();
	run (30);
	chk ("tras parar a mano, un cargador propio no la arranca", !tape.IsPlaying ());

	// SAVE instantáneo
	CTestSave Save;
	S->SetSaveHandler (&Save);
	S->SetTapeMode (TapeModeFlash);
	S->Reset ();
	run (150);
	SaveProgram ();
	run (100);
	press ({ENTER});		// "Start tape, then press any key"
	run (100);
	chk ("SAVE: dos bloques", Save.Blocks.size () == 2);
	if (Save.Blocks.size () == 2)
	{
		const std::vector<u8> &H = Save.Blocks[0], &D = Save.Blocks[1];
		chk ("SAVE: cabecera de programa \"x\"", H.size () == 19 && H[0] == 0x00 && H[1] == 0 && H[2] == 'x');
		chk ("SAVE: datos con flag FF", D.size () > 2 && D[0] == 0xFF);
		chk ("SAVE: paridad", ParityOK (H) && ParityOK (D));
	}

	// grabación por pulsos (sin trampa: la ROM graba "al aire", aunque haya
	// un CTapeSaveHandler)
	Save.Blocks.clear ();
	S->Reset ();
	run (150);
	chk ("grabación: empieza", S->StartRecording () && S->IsRecording () && !S->IsRecordingEarInput ());
	SaveProgram ();
	run (100);
	press ({ENTER});
	run (500);
	S->StopRecording ();
	chk ("grabación: SAVE no usa la trampa", Save.Blocks.empty ());
	S->SetSaveHandler (0);
	unsigned nLength, nPulses, nRate;
	const u8 *pRec = S->GetRecording (&nLength, &nPulses, &nRate);
	// cabecera: 8063 pulsos de guía + 2 + 19 bytes; datos: 3223 + 2 + bytes
	chk ("grabación: frecuencia de muestreo (3,5 MHz / 16)", nRate == 3500000 / 16);
	chk ("grabación: los pulsos de los dos bloques", pRec != 0 && nPulses > 8063 + 2 + 19 * 16 + 3223 + 2
						   && nPulses < 8063 + 3223 + 400 * 16);
	chk ("grabación: un pulso del tono guía (2168 t-states = 135 muestras)",
	     pRec != 0 && nLength > 1000 && (pRec[500] == 135 || pRec[500] == 136));
	chk ("grabación: parada", !S->IsRecording ());
	S->FreeRecording ();
	S->GetRecording (&nLength, &nPulses, &nRate);
	chk ("grabación: liberada", nLength == 0 && nPulses == 0);

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
