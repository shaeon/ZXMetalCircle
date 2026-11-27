// Cómo se enganchan a la memoria y a la CPU la ROM del Interface 1 y la
// Spectranet (CInterface1ROM en pagers.h, CSpectranet::Fetch), con
// roms/if1-2.rom y roms/spectranet.rom:
//
//  - IF1: su ROM entra al leer la instrucción de 0x0008 o de 0x1708 y sale
//    al leer la de 0x0700 (esa aún es suya); el reset y desconectarlo la
//    retiran; no en el +2A/+3; no con la divMMC ni con la Spectranet.
//  - Spectranet: el reset la deja paginada (su firmware), 0x007C la retira,
//    0x0008 y 0x3FF8-0x3FFF la paginan; la NMI la pagina al leer 0x0066;
//    RETN (no RETI) borra su biestable de la NMI; la trampa programable lanza
//    una NMI sin paginar todavía; apagada no hace nada.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o if1_snet_test if1_snet_test.cpp $CORE roms_host.S
#include "testutil.h"

static std::vector<u8> IF1, Snet;

// Lo que hace la CPU en cada M1 (z80cpp): leer la instrucción y pasársela a
// breakpoint (ahí están las trampas del IF1)
static u8 M1 (u16 nAddress)
{
	return S->breakpoint (nAddress, S->fetchOpcode (nAddress));
}

static bool ShowsIF1 (void)
{
	for (u16 a = 0; a < 0x100; a++) if (S->peek8 (a) != IF1[a]) return false;
	return S->peek8 (0x2000) == IF1[0];		// repetida en 0x2000
}

static bool ShowsSnet (void)
{
	for (u16 a = 0; a < 0x100; a++) if (S->peek8 (a) != Snet[a]) return false;
	return true;
}

static void IF1Tests (void)
{
	S->SetModel (Model48K);
	S->SetIF1Connected (TRUE);
	chk ("IF1: conectado", S->IsIF1Connected ());
	S->Reset ();
	chk ("IF1: al empezar, la ROM del Spectrum", !ShowsIF1 ());

	u8 nOpcode = M1 (0x0008);
	chk ("IF1: 0x0008 la pagina (y la instrucción ya es suya)", ShowsIF1 () && nOpcode == IF1[0x0008]);
	M1 (0x0700);
	chk ("IF1: 0x0700 la retira", !ShowsIF1 ());
	nOpcode = M1 (0x1708);
	chk ("IF1: 0x1708 (CLOSE#) la pagina", ShowsIF1 () && nOpcode == IF1[0x1708]);
	nOpcode = M1 (0x0700);
	chk ("IF1: la instrucción de 0x0700 aún es suya", nOpcode == IF1[0x0700]);

	M1 (0x0008);
	S->Reset ();
	chk ("IF1: el reset la retira", !ShowsIF1 ());

	M1 (0x0008);
	S->SetIF1Connected (FALSE);
	chk ("IF1: desconectarlo la retira", !S->IsIF1Connected () && !ShowsIF1 ());
	M1 (0x0008);
	chk ("IF1: desconectado, 0x0008 no hace nada", !ShowsIF1 ());

	S->SetModel (ModelPlus3);
	S->SetIF1Connected (TRUE);
	S->Reset ();
	M1 (0x0008);
	chk ("IF1: en el +3 no se pagina", !ShowsIF1 ());
	S->SetIF1Connected (FALSE);

	S->SetModel (Model48K);
	S->SetDivMMCEnabled (TRUE);
	S->SetDivMMCROM (IF1.data ());		// cualquier ROM de 8 KB
	S->SetIF1Connected (TRUE);
	chk ("IF1: con la divMMC no se conecta", !S->IsIF1Connected ());
	S->SetDivMMCEnabled (FALSE);
	S->SetDivMMCROM (0);
}

static void SnetTests (void)
{
	S->SetModel (Model48K);
	S->SetSpectranetEnabled (TRUE);
	S->SetIF1Connected (TRUE);
	chk ("Spectranet: con ella no hay IF1", !S->IsIF1Connected ());

	S->Reset ();
	chk ("Spectranet: el reset la deja paginada", ShowsSnet ());
	S->fetchOpcode (0x007C);
	chk ("Spectranet: 0x007C la retira", !ShowsSnet ());
	S->Reset ();
	chk ("Spectranet: el reset la vuelve a paginar", ShowsSnet ());
	S->fetchOpcode (0x007C);

	u8 nOpcode = S->fetchOpcode (0x0008);
	chk ("Spectranet: 0x0008 la pagina antes de leer", ShowsSnet () && nOpcode == Snet[0x0008]);
	S->fetchOpcode (0x007C);
	S->fetchOpcode (0x3FF8);
	chk ("Spectranet: 0x3FF8 la pagina", ShowsSnet ());
	S->fetchOpcode (0x007C);

	// NMI (botón): se pagina al aceptarse, al leer 0x0066
	S->m_Snet.TakeNMI ();
	chk ("Spectranet: NMI pendiente, aún sin paginar", !ShowsSnet ());
	nOpcode = S->fetchOpcode (0x0066);
	chk ("Spectranet: la NMI la pagina en 0x0066", ShowsSnet () && nOpcode == Snet[0x0066]
						     && S->m_Snet.m_bNMIFlipFlop);

	// RETI no borra el biestable; RETN sí
	S->poke8 (0x7FFF, 0x00);
	S->poke8 (0x8000, 0xED); S->poke8 (0x8001, 0x4D);
	S->poke8 (0x8002, 0xED); S->poke8 (0x8003, 0x45);
	S->fetchOpcode (0x7FFF);	// un NOP: lo de 0x0066 puede ser un prefijo
	S->fetchOpcode (0x8000); S->fetchOpcode (0x8001);
	chk ("Spectranet: RETI no borra la NMI", S->m_Snet.m_bNMIFlipFlop);
	S->fetchOpcode (0x8002); S->fetchOpcode (0x8003);
	chk ("Spectranet: RETN la borra", !S->m_Snet.m_bNMIFlipFlop);
	S->fetchOpcode (0x007C);

	// trampa programable en 0x8000: NMI, sin paginar todavía
	S->m_Snet.m_nTrap = 0x8000;
	S->m_Snet.m_bTrapActive = TRUE;
	S->m_CPU.setNMI (false);
	nOpcode = S->fetchOpcode (0x8000);
	chk ("Spectranet: la trampa lanza la NMI", S->m_CPU.isNMI () && nOpcode == 0xED && !ShowsSnet ());
	S->fetchOpcode (0x0066);
	chk ("Spectranet: y la NMI la pagina", ShowsSnet ());
	S->m_CPU.setNMI (false);
	S->m_Snet.m_bTrapActive = FALSE;

	S->SetSpectranetEnabled (FALSE);
	S->Reset ();
	S->fetchOpcode (0x0008);
	chk ("Spectranet apagada: ni el reset ni 0x0008 la paginan", !ShowsSnet ());
}

int main (void)
{
	S = new CSpectrum (&kb, &tape);

	IF1 = ReadFile (ROMDIR "if1-2.rom");
	chk ("if1-2.rom de 8 KB", IF1.size () == 0x2000);
	Snet = ReadFile (ROMDIR "spectranet.rom");
	chk ("spectranet.rom de 128 KB", Snet.size () == 0x20000);
	if (IF1.size () != 0x2000 || Snet.size () != 0x20000) return 1;
	const u8 *pPlus3 = LoadROM ("plus3", 4);
	chk ("ROM del +3", pPlus3 != 0);
	if (pPlus3 == 0) return 1;
	S->SetModelROM (ModelPlus3, pPlus3);

	S->SetIF1Connected (TRUE);
	chk ("IF1: sin su ROM no se conecta", !S->IsIF1Connected ());
	S->SetIF1ROM (IF1.data ());
	S->GetSpectranet ()->LoadFlash (Snet.data (), Snet.size ());

	IF1Tests ();
	SnetTests ();

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
