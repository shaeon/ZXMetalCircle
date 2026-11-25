//
// z80_q_test.cpp - Registro Q del Z80 (bits 5 y 3 de F en SCF y CCF) tras un
// prefijo DD o FD
//
// SCF y CCF ponen los bits 5 y 3 de F según A, F y Q (lo que la instrucción
// anterior escribió en F). Un prefijo DD/FD cuenta como una instrucción que
// no toca F, así que deja Q a 0: tras "CP 28h", un "DD 37" (SCF con prefijo)
// pone esos bits de A|F y no solo de A, como un SCF directo. Arreglo de José
// Luis Sánchez en z80cpp (lastFlagQ = false al leer el prefijo).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "z80.h"
#include <cstdio>
#include <cstring>

class CTestBus : public Z80operations
{
public:
	uint8_t Mem[0x10000];

	CTestBus (void)						{ memset (Mem, 0, sizeof Mem); }
	uint8_t fetchOpcode (uint16_t address) override		{ return Mem[address]; }
	uint8_t peek8 (uint16_t address) override		{ return Mem[address]; }
	void poke8 (uint16_t address, uint8_t value) override	{ Mem[address] = value; }
	uint16_t peek16 (uint16_t address) override
		{ return Mem[address] | (Mem[(uint16_t) (address + 1)] << 8); }
	void poke16 (uint16_t address, RegisterPair word) override
		{ Mem[address] = word.byte8.lo; Mem[(uint16_t) (address + 1)] = word.byte8.hi; }
	uint8_t inPort (uint16_t) override			{ return 0xFF; }
	void outPort (uint16_t, uint8_t) override		{ }
	void addressOnBus (uint16_t, int32_t) override		{ }
	void interruptHandlingTime (int32_t) override		{ }
	bool isActiveINT (void) override			{ return false; }
	uint8_t breakpoint (uint16_t, uint8_t opcode) override	{ return opcode; }
	void execDone (void)					{ }
};

static unsigned s_nErrors = 0;

// Ejecuta el programa (3 instrucciones) y devuelve los bits 5 y 3 de F
static uint8_t Run (const uint8_t *pProgram, unsigned nLength)
{
	static CTestBus Bus;
	memset (Bus.Mem, 0, sizeof Bus.Mem);
	memcpy (Bus.Mem, pProgram, nLength);
	Z80 CPU (&Bus);
	CPU.reset ();
	CPU.setRegPC (0);
	for (unsigned i = 0; i < 3; i++)
	{
		CPU.execute ();
	}
	return CPU.getFlags () & 0x28;
}

static void Check (const char *pName, uint8_t nGot, uint8_t nExpected)
{
	if (nGot != nExpected)
	{
		printf ("FALLO %s: F&28h = %02Xh, se esperaba %02Xh\n", pName, nGot, nExpected);
		s_nErrors++;
	}
}

int main (void)
{
	// LD A,0 ; CP 28h (F bits 5/3 = 28h, Q = F) ; SCF
	static const uint8_t SCF[] = {0x3E, 0x00, 0xFE, 0x28, 0x37};
	Check ("SCF tras CP", Run (SCF, sizeof SCF), 0x00);		// (Q ^ F) | A = 0

	// lo mismo con DD 37 y FD 37: el prefijo deja Q = 0
	static const uint8_t DDSCF[] = {0x3E, 0x00, 0xFE, 0x28, 0xDD, 0x37};
	Check ("DD SCF tras CP", Run (DDSCF, sizeof DDSCF), 0x28);	// F | A = 28h
	static const uint8_t FDSCF[] = {0x3E, 0x00, 0xFE, 0x28, 0xFD, 0x37};
	Check ("FD SCF tras CP", Run (FDSCF, sizeof FDSCF), 0x28);

	// CCF igual
	static const uint8_t CCF[] = {0x3E, 0x00, 0xFE, 0x28, 0x3F};
	Check ("CCF tras CP", Run (CCF, sizeof CCF), 0x00);
	static const uint8_t DDCCF[] = {0x3E, 0x00, 0xFE, 0x28, 0xDD, 0x3F};
	Check ("DD CCF tras CP", Run (DDCCF, sizeof DDCCF), 0x28);

	if (s_nErrors == 0)
	{
		printf ("z80_q_test: todo OK\n");
	}
	return s_nErrors == 0 ? 0 : 1;
}
