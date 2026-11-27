// divMMC (con roms/esxmmc.bin), ZXMMC del +3e y el reloj de esxDOS, sin
// ficheros: la tarjeta es una imagen de 1 MB en memoria.
//
//  - Paginación de la divMMC: CONMEM (puerto 0xE3 bit 7) pone la ROM de
//    esxDOS abajo y el banco elegido en 0x2000; los bancos guardan lo suyo; la
//    ROM no se escribe; MAPRAM (bit 6) pone el banco 3 abajo, de solo lectura,
//    y no se borra salvo con reset; los puntos de entrada (0x0038, 0x3Dxx) la
//    paginan y 0x1FF8 la retira; el reset la retira.
//  - La tarjeta por SPI, con la divMMC (0xE7/0xEB) y con la ZXMMC del +3e
//    (0x1F/0x3F): CMD0, leer un sector (CMD17) y escribirlo (CMD24).
//  - En el +3e, la divMMC solo pone sus puertos (sin paginación).
//  - El reloj (0x7C3B): un OUT fija la hora y cuatro IN la leen.
//  - esxDOS arranca sin tarjeta y deja el BASIC del 48K funcionando.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o divmmc_test divmmc_test.cpp $CORE roms_host.S
#include "testutil.h"

class CMemoryCard : public CBlockDevice
{
public:
	CMemoryCard (void) : m_Data (1024 * 1024, 0) {}
	u32 GetSectors (void) const		{ return m_Data.size () / 512; }
	boolean ReadSector (u32 n, u8 *p)	{ memcpy (p, &m_Data[n * 512], 512); return TRUE; }
	boolean WriteSector (u32 n, const u8 *p) { memcpy (&m_Data[n * 512], p, 512); return TRUE; }
	std::vector<u8> m_Data;
};

struct TSPI { u16 nSelect, nData; const char *pName; };
static const TSPI DivPorts = {0xE7, 0xEB, "divMMC"};
static const TSPI ZXPorts  = {0x1F, 0x3F, "ZXMMC"};

static void Select (const TSPI &P, bool bOn)	{ S->outPort (P.nSelect, bOn ? 0xFE : 0xFF); }
static void Send (const TSPI &P, u8 n)		{ S->outPort (P.nData, n); }
static u8 Receive (const TSPI &P)		{ return S->inPort (P.nData); }

// Una orden y su respuesta R1 (0xFF si no llega)
static u8 Command (const TSPI &P, u8 nCmd, u32 nArg)
{
	Send (P, 0xFF);
	Send (P, 0x40 | nCmd);
	Send (P, nArg >> 24); Send (P, nArg >> 16); Send (P, nArg >> 8); Send (P, nArg);
	Send (P, nCmd == 0 ? 0x95 : (nCmd == 8 ? 0x87 : 0x01));
	Receive (P);			// lo recibido mientras se mandaba el CRC
	for (int i = 0; i < 16; i++)
	{
		u8 n = Receive (P);
		if (n != 0xFF) return n;
	}
	return 0xFF;
}

static bool Init (const TSPI &P)
{
	Select (P, true);
	if (Command (P, 0, 0) != 0x01) return false;
	for (int i = 0; i < 100; i++)
	{
		Command (P, 55, 0);
		if (Command (P, 41, 0) == 0x00) return true;
	}
	return false;
}

static bool ReadSector (const TSPI &P, u32 nSector, u8 *pBuffer)
{
	if (Command (P, 17, nSector * 512) != 0x00) return false;	// SDSC: dirección en bytes
	int i;
	for (i = 0; i < 1000 && Receive (P) != 0xFE; i++) ;
	if (i == 1000) return false;
	for (int j = 0; j < 512; j++) pBuffer[j] = Receive (P);
	Receive (P); Receive (P);	// CRC
	return true;
}

static bool WriteSector (const TSPI &P, u32 nSector, const u8 *pBuffer)
{
	if (Command (P, 24, nSector * 512) != 0x00) return false;
	Send (P, 0xFF);
	Send (P, 0xFE);
	for (int j = 0; j < 512; j++) Send (P, pBuffer[j]);
	Send (P, 0xFF); Send (P, 0xFF);	// CRC
	Receive (P);
	u8 nResponse = 0xFF;
	for (int i = 0; i < 16 && nResponse == 0xFF; i++) nResponse = Receive (P);
	for (int i = 0; i < 1000 && Receive (P) != 0xFF; i++) ;	// ocupada
	return (nResponse & 0x1F) == 0x05;
}

static void CardTests (const TSPI &P, CMemoryCard &Card)
{
	char Text[80];
	chk ((snprintf (Text, sizeof Text, "%s: CMD0 y ACMD41", P.pName), Text), Init (P));

	u8 Buffer[512];
	for (int j = 0; j < 512; j++) Card.m_Data[7 * 512 + j] = (u8) (j * 7 + 3);
	chk ((snprintf (Text, sizeof Text, "%s: lee un sector", P.pName), Text),
	     ReadSector (P, 7, Buffer) && memcmp (Buffer, &Card.m_Data[7 * 512], 512) == 0);

	for (int j = 0; j < 512; j++) Buffer[j] = (u8) (255 - j);
	chk ((snprintf (Text, sizeof Text, "%s: escribe un sector", P.pName), Text),
	     WriteSector (P, 9, Buffer) && memcmp (Buffer, &Card.m_Data[9 * 512], 512) == 0);
	Select (P, false);
}

// ¿Se ve abajo la ROM de esxDOS? (los primeros 256 bytes: empieza igual que
// la del Spectrum, con DI)
static std::vector<u8> ESX;
static bool ShowsESX (void)
{
	for (u16 a = 0; a < 0x100; a++) if (S->peek8 (a) != ESX[a]) return false;
	return true;
}

int main (void)
{
	S = new CSpectrum (&kb, &tape);
	CMemoryCard Card;
	S->GetSDCard ()->Insert (&Card);

	ESX = ReadFile (ROMDIR "esxmmc.bin");
	chk ("esxmmc.bin de 8 KB", ESX.size () == 0x2000);
	if (ESX.size () != 0x2000) return 1;
	const u8 *pPlus3e = LoadROM ("plus3e-es", 4);
	chk ("ROM del +3e", pPlus3e != 0);
	if (pPlus3e == 0) return 1;
	S->SetModelROM (ModelPlus3E, pPlus3e);

	S->SetModel (Model48K);
	S->SetDivMMCEnabled (TRUE);
	chk ("sin ROM no hay divMMC", !S->IsDivMMCActive () && !S->HasDivMMCPorts ());
	S->SetDivMMCROM (ESX.data ());
	chk ("con ROM, activa", S->IsDivMMCActive () && S->HasDivMMCPorts ());
	S->Reset ();

	// CONMEM
	S->outPort (0x00E3, 0x80);
	chk ("CONMEM: la ROM de esxDOS abajo", ShowsESX () && S->peek8 (0x1FFF) == ESX[0x1FFF]);
	S->poke8 (0x0000, ESX[0] ^ 0xFF);
	chk ("CONMEM: su ROM no se escribe", ShowsESX ());
	S->poke8 (0x2000, 0x11);
	S->outPort (0x00E3, 0x81);
	S->poke8 (0x2000, 0x22);
	S->outPort (0x00E3, 0x8F);
	S->poke8 (0x3FFF, 0x33);
	S->outPort (0x00E3, 0x80);
	chk ("bancos: el 0 guarda lo suyo", S->peek8 (0x2000) == 0x11);
	S->outPort (0x00E3, 0x81);
	chk ("bancos: el 1 guarda lo suyo", S->peek8 (0x2000) == 0x22);
	S->outPort (0x00E3, 0x8F);
	chk ("bancos: el 15 guarda lo suyo", S->peek8 (0x3FFF) == 0x33);
	S->outPort (0x00E3, 0x00);
	chk ("sin CONMEM: la ROM del Spectrum", !ShowsESX ());

	// paginación automática
	S->fetchOpcode (0x0038);
	chk ("0x0038 la pagina (después de leer)", ShowsESX ());
	S->fetchOpcode (0x1FF8);
	chk ("0x1FF8 la retira", !ShowsESX ());
	u8 nOpcode = S->fetchOpcode (0x3D00);
	chk ("0x3D00 la pagina antes de leer", ShowsESX () && nOpcode == S->peek8 (0x3D00));
	S->fetchOpcode (0x1FFF);
	chk ("0x1FFF la retira", !ShowsESX ());

	// MAPRAM: el banco 3 abajo, de solo lectura, y no se borra
	S->outPort (0x00E3, 0x83);
	S->poke8 (0x2000, 0x5A);
	S->outPort (0x00E3, 0x43);
	S->fetchOpcode (0x0038);
	chk ("MAPRAM: el banco 3 abajo", S->peek8 (0x0000) == 0x5A);
	S->poke8 (0x2000, 0xA5);
	chk ("MAPRAM: el banco 3, de solo lectura", S->peek8 (0x2000) == 0x5A);
	S->outPort (0x00E3, 0x03);
	chk ("MAPRAM: no se borra con OUT", S->peek8 (0x0000) == 0x5A);
	S->Reset ();
	chk ("el reset la retira (y borra MAPRAM)", !ShowsESX ());
	S->outPort (0x00E3, 0x80);
	chk ("tras el reset, CONMEM vuelve a dar la ROM", ShowsESX ());
	S->outPort (0x00E3, 0x00);
	chk ("con la divMMC no hay Multiface", !S->IsMultifaceAvailable ());

	// reloj
	S->SetRTC (0x5B8C, 0xA3EF);
	S->outPort (0x7C3B, 0);
	chk ("reloj: fecha y hora", S->inPort (0x7C3B) == 0x8C && S->inPort (0x7C3B) == 0x5B
				   && S->inPort (0x7C3B) == 0xEF && S->inPort (0x7C3B) == 0xA3);
	chk ("reloj: después, 0xFF", S->inPort (0x7C3B) == 0xFF);

	// la tarjeta por la divMMC
	CardTests (DivPorts, Card);

	// esxDOS arranca sin tarjeta y deja el BASIC
	S->GetSDCard ()->Insert (0);
	S->Reset ();
	run (400);
	press ({key ('p')});		// PRINT (la P en modo K)
	type ("1");
	press ({SS, key ('k')});	// +
	type ("1");
	press ({ENTER});
	run (50);
	chk ("esxDOS sin tarjeta: el BASIC funciona (PRINT 1+1)",
	     S->peek8 (0x5C3A) == 0xFF && InkPixels (0, 0) > 0);	// ERR_NR sin error y el 2 arriba
	S->GetSDCard ()->Insert (&Card);

	// +3e: ZXMMC y, de la divMMC, solo los puertos
	S->SetModel (ModelPlus3E);
	chk ("+3e: la divMMC no se activa", !S->IsDivMMCActive () && S->HasDivMMCPorts ());
	S->outPort (0x00E3, 0x80);
	chk ("+3e: CONMEM no pagina nada", !ShowsESX ());
	CardTests (ZXPorts, Card);
	CardTests (DivPorts, Card);

	// sin divmmc=esxdos
	S->SetModel (Model48K);
	S->SetDivMMCEnabled (FALSE);
	chk ("divmmc=0: sin puertos", !S->IsDivMMCActive () && !S->HasDivMMCPorts ());
	S->outPort (0x00E3, 0x80);
	chk ("divmmc=0: CONMEM no hace nada", S->peek8 (0x0000) == 0xF3);	// DI de la ROM del 48K

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
