// Bus flotante del +2A/+3 (como el core zxp3 de Kyp): solo en la familia del
// 0x0FFD (A15-A12 = 0, A1 = 0) y sin bloqueo de paginación; devuelve el último
// valor del bus (lectura de vídeo de la Gate Array o acceso de la CPU a
// 0x4000-0x5FFF) con el bit 0 a 1. Los demás puertos sin dueño dan 0xFF.
//
//   (se compila como kempston_ports_test.cpp, más -DROMDIR si hace falta)
#define private public
#include "spectrum.h"
#undef private
#include <stdio.h>
#include <stdlib.h>
#ifndef ROMDIR
#define ROMDIR "../../roms/"
#endif
static void SetPlus3ROM (CSpectrum *p)
{
	static u8 r[65536];
	for (int i = 0; i < 4; i++)
	{
		char n[256]; snprintf (n, sizeof n, ROMDIR "plus3-%d.rom", i);
		FILE *f = fopen (n, "rb");
		if (f == 0) { printf ("falta %s\n", n); exit (1); }
		if (fread (r + i * 16384, 1, 16384, f) != 16384) { printf ("%s incompleta\n", n); exit (1); }
		fclose (f);
	}
	p->SetModelROM (ModelPlus3, r);
}

int fails = 0;
void chk (const char *pName, bool bOK) { printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName); if (!bOK) fails++; }

int main ()
{
	CZXKeyboard kb; CTape tape;
	CSpectrum *s = new CSpectrum (&kb, &tape);
	SetPlus3ROM (s);
	s->SetModel (ModelPlus3);
	const TMachineTiming *T = s->m_pTiming;
	int L = T->nTstatesLine, up = T->nUpBorderWidth, off = T->nOutOffset;

	// pantalla con valores reconocibles: bitmap y atributo distintos
	for (int i = 0; i < 0x1800; i++) s->m_pScreenPage[i] = 0x10 | (i & 0x0E);
	for (int i = 0x1800; i < 0x1B00; i++) s->m_pScreenPage[i] = 0x80 | (i & 0x0E);
	unsigned a0 = s->m_ScrAddr[0];
	s->m_bBusValid = FALSE;
	s->m_nPort7FFD &= ~0x20;

	// valores en el instante exacto (Plus3FloatingBus; inPort suma el tiempo de la E/S)
	s->m_nTstates = up * L + off + 0;
	chk ("fila 0, col 0: bitmap del primer byte", s->Plus3FloatingBus () == s->m_pScreenPage[a0]);
	s->m_nTstates = up * L + off + 1;
	chk ("fila 0, col 1: su atributo", s->Plus3FloatingBus () == s->m_pScreenPage[s->m_Scr2Attr[a0]]);
	s->m_nTstates = up * L + off + 6;
	chk ("fila 0, col 6 (sin lectura): se mantiene la de la col 3", s->Plus3FloatingBus () == s->m_pScreenPage[s->m_Scr2Attr[a0 + 1]]);
	s->m_nTstates = 10;
	unsigned a191 = s->m_ScrAddr[191] + 123 / 4 + 1;
	chk ("borde superior: la última lectura del cuadro anterior", s->Plus3FloatingBus () == s->m_pScreenPage[s->m_Scr2Attr[a191]]);
	s->m_nTstates = (up + 5) * L + off + 200;
	chk ("borde derecho: la última lectura de esa línea (col 123)", s->Plus3FloatingBus () == s->m_pScreenPage[s->m_Scr2Attr[s->m_ScrAddr[5] + 123 / 4 + 1]]);

	s->m_nTstates = (up + 10) * L + off + 5;
	s->NoteBus (0x24);
	s->m_nTstates += 2;
	chk ("acceso de la CPU después de la Gate Array: su valor", s->Plus3FloatingBus () == 0x24);
	s->m_nTstates = (up + 10) * L + off + 9;
	chk ("la Gate Array lee después: vuelve a mandar ella", s->Plus3FloatingBus () == s->m_pScreenPage[s->m_Scr2Attr[s->m_ScrAddr[10] + 2]]);
	s->m_nTstates = (up + 10) * L + off + 5;
	s->NoteBus (0x24);
	s->m_nTstates = 0;

	// el puerto: bit 0 a 1
	s->m_nTstates = 10; s->NoteBus (0x24); s->m_nTstates = 20;
	chk ("puerto 0x0FFD: el valor del bus con el bit 0 a 1 (0x25)", s->inPort (0x0FFD) == 0x25);

	chk ("puerto 0x0FFC (A0 = 0): también", (s->inPort (0x0FFC) & 1) == 1 && s->inPort (0x0FFC) != 0xFF);
	chk ("puerto 0x1FFD (A12 = 1): 0xFF", s->inPort (0x1FFD) == 0xFF);
	chk ("puerto 0x00FF (A1 = 1): 0xFF", s->inPort (0x00FF) == 0xFF);
	s->m_nPort7FFD |= 0x20;
	chk ("con la paginación bloqueada: 0xFF", s->inPort (0x0FFD) == 0xFF);
	s->m_nPort7FFD &= ~0x20;

	s->SetModel (Model48K);
	for (int i = 0; i < 0x1800; i++) s->m_pScreenPage[i] = 0x10 | (i & 0x0E);
	s->m_nTstates = s->m_pTiming->nUpBorderWidth * s->m_pTiming->nTstatesLine + s->m_pTiming->nOutOffset;
	u8 v = s->FloatingBus ();
	chk ("48K: sigue el bus flotante de siempre", v == s->m_pScreenPage[s->m_ScrAddr[0]]);
	s->m_nTstates = 10;
	chk ("48K: en el borde, 0xFF (no se mantiene el último valor)", s->inPort (0x0FFD) == 0xFF);

	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails;
}
