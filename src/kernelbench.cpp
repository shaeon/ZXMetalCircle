//
// kernelbench.cpp - Medida de la velocidad de la emulación (bench=1)
//
// Con bench=1 en cmdline.txt, al arrancar se ejecuta siempre el mismo
// programa (el de abajo, hecho para esto: no depende de ningún fichero) en
// el 48K, el 128K y el +3, 50 cuadros para empezar y 500 medidos. Se mide
// solo lo que tarda en emularse cada cuadro (RunFrame): ni la espera al
// refresco, ni el sonido, ni el escalado. Los cuadros van a su velocidad
// normal (50 por segundo) y se ven en pantalla. El resultado sale abajo y se
// añade a SD:/medida.txt, para comparar versiones (la reestructuración en
// una capa de máquinas no debe hacer más lento el emulador).
//
// El programa trabaja sin parar, con las interrupciones desactivadas, para
// que cada cuadro cueste lo mismo:
//
//	8000 F3          DI
//	8001 31 00 00    LD SP,0
//	8004 21 00 00 loop: LD HL,0       ; la ROM a la pantalla (memoria contenida)
//	8007 11 00 40    LD DE,4000h
//	800A 01 00 18    LD BC,1800h
//	800D ED B0       LDIR
//	800F 0E 10       LD C,16          ; el borde: 16 x 256 OUT (FEh)
//	8011 06 00    b1: LD B,0
//	8013 D3 FE    b2: OUT (FEh),A
//	8015 3C          INC A
//	8016 E6 07       AND 7
//	8018 10 F9       DJNZ b2
//	801A 0D          DEC C
//	801B 20 F4       JR NZ,b1
//	801D 21 00 58    LD HL,5800h      ; los atributos
//	8020 3C          INC A
//	8021 77          LD (HL),A
//	8022 11 01 58    LD DE,5801h
//	8025 01 FF 02    LD BC,02FFh
//	8028 ED B0       LDIR
//	802A 16 10       LD D,16          ; el AY: 16 x 14 registros
//	802C 1E 00    a1: LD E,0
//	802E 01 FD FF a2: LD BC,FFFDh
//	8031 ED 59       OUT (C),E
//	8033 06 BF       LD B,BFh
//	8035 ED 51       OUT (C),D
//	8037 1C          INC E
//	8038 7B          LD A,E
//	8039 FE 0E       CP 14
//	803B 38 F1       JR C,a2
//	803D 15          DEC D
//	803E 20 EC       JR NZ,a1
//	8040 1E 00       LD E,0           ; paginación del 128: bancos 0 a 7
//	8042 01 FD 7F p1: LD BC,7FFDh
//	8045 ED 59       OUT (C),E
//	8047 21 00 00    LD HL,0          ; 1 KB a F000h (lejos del programa
//	804A 11 00 F0    LD DE,F000h      ; también con el banco 2 arriba)
//	804D 01 00 04    LD BC,0400h
//	8050 ED B0       LDIR
//	8052 1C          INC E
//	8053 7B          LD A,E
//	8054 FE 08       CP 8
//	8056 38 EA       JR C,p1
//	8058 DD 21 00 90 LD IX,9000h      ; instrucciones con IX
//	805C 06 00       LD B,0
//	805E DD 7E 01 x1: LD A,(IX+1)
//	8061 DD 86 02    ADD A,(IX+2)
//	8064 DD 77 03    LD (IX+3),A
//	8067 DD CB 04 06 RLC (IX+4)
//	806B DD 23       INC IX
//	806D 10 EF       DJNZ x1
//	806F C3 04 80    JP loop
//
// En el 48K, los OUT al AY y a 7FFDh no hacen nada (no se decodifican).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include "snapshot.h"
#include "benchprog.h"
#include "version.h"
#include "i18n.h"
#include <circle/machineinfo.h>
#include <circle/timer.h>
#include <circle/util.h>

#define BENCH_WARMUP	50
#define BENCH_FRAMES	500

static const char FromBench[] = "bench";


// Un .Z80 v3 con el programa en 8000h (el resto de la memoria, como quede
// tras el reset). Hardware: 0 = 48K, 4 = 128K, 7 = +3.
static unsigned MakeSnapshot (u8 *pBuffer, u8 nHardware)
{
	const unsigned nHeader = 32 + 54;
	memset (pBuffer, 0, nHeader + 3 + 0x4000);

	pBuffer[10] = 0x3F;			// I (como tras la ROM)
	pBuffer[12] = 0x0E;			// borde 7
	pBuffer[29] = 0x01;			// IM 1 (con DI: no se usa)
	pBuffer[30] = 54;			// v3
	pBuffer[32] = BENCH_ORG & 0xFF;		// PC
	pBuffer[33] = BENCH_ORG >> 8;
	pBuffer[34] = nHardware;

	// un bloque sin comprimir: el de 8000h (48K: página 4; 128K y +3: banco 2)
	u8 *pBlock = pBuffer + nHeader;
	pBlock[0] = 0xFF;
	pBlock[1] = 0xFF;
	pBlock[2] = nHardware == 0 ? 4 : 3 + 2;
	memcpy (pBlock + 3, BenchProgram, sizeof BenchProgram);

	return nHeader + 3 + 0x4000;
}

void CKernel::RunBenchmark (void)
{
	static const struct
	{
		TModel Model;
		u8 nHardware;
		const char *pName;
	}
	Tests[] =
	{
		{Model48K,   0, "48K"},
		{Model128K,  4, "128K"},
		{ModelPlus3, 7, "+3"}
	};

	TModel SavedModel = m_pSpectrum->GetModel ();
	u8 *pSnapshot = new u8[32 + 54 + 3 + 0x4000];
	if (pSnapshot == 0)
	{
		return;
	}

	CString Report;
	Report.Format ("ZXMetalCircle %s - %s, %u bits, %u MHz, %u C%s\r\n",
		       ZXCIRCLE_VERSION, CMachineInfo::Get ()->GetMachineName (),
#if AARCH == 64
		       64,
#else
		       32,
#endif
		       GetCPUMHz (), GetCelsius (),
#ifdef ARM_ALLOW_MULTI_CORE
		       m_pVideoCore != 0 ? ", multinucleo" : ""
#else
		       ""
#endif
		       );
	CString Summary;

	for (unsigned t = 0; t < sizeof Tests / sizeof Tests[0]; t++)
	{
		if (!m_pSpectrum->IsModelAvailable (Tests[t].Model))
		{
			CString Line;
			Line.Format ("%-5s sin sus ROMs: no se mide\r\n", Tests[t].pName);
			Report.Append (Line);
			continue;
		}
		unsigned nSize = MakeSnapshot (pSnapshot, Tests[t].nHardware);
		const char *pError = CSnapshot::Load (m_pSpectrum, pSnapshot, nSize, "bench.z80");
		if (pError != 0)
		{
			m_Logger.Write (FromBench, LogError, "%s: %s", Tests[t].pName, pError);
			continue;
		}

		unsigned nFrameUs = m_pMachine->GetFrameUs ();
		u64 nSum = 0;
		unsigned nMin = ~0U, nMax = 0;
		CString Busy;
		for (unsigned nFrame = 0; nFrame < BENCH_WARMUP + BENCH_FRAMES; nFrame++)
		{
			unsigned nStart = CTimer::GetClockTicks ();
			m_pMachine->RunFrame ();
			unsigned nUs = CTimer::GetClockTicks () - nStart;
			m_pMachine->GetAudio ()->ClearBuffer ();	// sin sonido

			if (nFrame >= BENCH_WARMUP)
			{
				nSum += nUs;
				if (nUs < nMin) nMin = nUs;
				if (nUs > nMax) nMax = nUs;
			}

			// se ve (con el aviso), fuera de la medida, a 50 cuadros por segundo
			if (nFrame % 25 == 0)
			{
				Busy.Format (T ("Midiendo la velocidad: %s (%u s)"), Tests[t].pName,
					     (BENCH_WARMUP + BENCH_FRAMES - nFrame) / 50);
				ShowMessage (Busy);
			}
			memcpy (m_pOverlay, m_pMachine->GetScreenBuffer (),
				SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
			m_nMessageFrames = 1;
			DrawMessage (m_pOverlay);
			ShowFrame (m_pOverlay);
			while (CTimer::GetClockTicks () - nStart < nFrameUs)
			{
				m_Scheduler.Yield ();
			}
		}

		unsigned nAvg = (unsigned) (nSum / BENCH_FRAMES);
		unsigned nPercent10 = nAvg * 1000 / nFrameUs;
		CString Line;
		Line.Format ("%-5s media %5u us  min %5u  max %5u  (%u,%u %% de %u us)\r\n",
			     Tests[t].pName, nAvg, nMin, nMax, nPercent10 / 10, nPercent10 % 10, nFrameUs);
		Report.Append (Line);
		m_Logger.Write (FromBench, LogNotice, "%s", (const char *) Line);

		CString Part;
		Part.Format ("%s%s %u,%u ms", Summary.GetLength () > 0 ? "  " : "",
			     Tests[t].pName, nAvg / 1000, nAvg % 1000 / 100);
		Summary.Append (Part);
	}
	delete [] pSnapshot;

	Report.Append ("\r\n");
	WriteFile ("SD:/medida.txt", (const u8 *) (const char *) Report, Report.GetLength (), TRUE);

	m_pSpectrum->SetModel (SavedModel);		// y vuelta a empezar, como al arrancar

	strncpy (m_OptionWarning, Summary, sizeof m_OptionWarning - 1);
	m_OptionWarning[sizeof m_OptionWarning - 1] = '\0';
	ShowMessage (m_OptionWarning);
	m_nWarningFrames = 1500;			// unos 30 s (también en medida.txt)
	m_nMessageFrames = m_nWarningFrames;
}
