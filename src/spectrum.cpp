//
// spectrum.cpp - Máquina ZX Spectrum 48K/128K/+2A
//
// Lógica de la ULA, contención, bus flotante y paginación portada a C++
// desde JSpeccy (machine/Spectrum.java, machine/Memory.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "spectrum.h"
#include <circle/util.h>

// ROMs embebidas (roms.S)
extern "C" const u8 rom_48[];
extern "C" const u8 rom_128_0[];
extern "C" const u8 rom_128_1[];

// Niveles del beeper según los bits EAR (4) y MIC (3) del puerto 0xFE
static const int BeeperLevel[4] = {0, 1500, 9000, 10500};

// Detección de carga desde la cinta
#define AUTOSTOP_FRAMES		100	// 2 s sin cargar -> se para la cinta
#define AUTOPLAY_FRAMES		10
#define LOADER_READS_PLAY	500	// lecturas de cargador por cuadro para arrancar la cinta
#define LOADER_READS_ACTIVE	20	// por debajo, durante 2 s, se para

#define NO_PIXEL		(-1)

CSpectrum::CSpectrum (CZXKeyboard *pKeyboard, CTape *pTape)
:	m_pKeyboard (pKeyboard),
	m_pTape (pTape),
	m_CPU (this),
	m_Model (Model48K),
	m_pTiming (&MachineTiming[Model48K]),
	m_nTstates (0),
	m_nFrameStart (0),
	m_nFrames (0),
	m_bFlash (FALSE),
	m_TapeMode (TapeModeFast),
	m_nFEReads (0),
	m_nIdleFrames (0),
	m_nLoadingFrames (0),
	m_bAutoStopped (TRUE),
	m_bLoaderAccel (TRUE),
	m_AccelMode (AccelNone),
	m_nAccelPC (0),
	m_nAccelEdges (0),
	m_nSeenEdge (0),
	m_nFrameEdges (0),
	m_nObservedEdges (0),
	m_nLoaderReads (0),
	m_nLastFEPC (0),
	m_nLastFEPort (0)
{
	m_pSaveHandler = 0;
	m_bRecording = FALSE;
	m_bRecordEar = FALSE;
	m_pEarInput = 0;
	m_pRecBuffer = 0;
	m_nRecLength = m_nRecCapacity = m_nRecPulses = 0;
	m_nRecLastEdge = 0;
	m_nRecRemainder = 0;
	m_bIssue2 = FALSE;
	m_bIF2Inserted = FALSE;
	m_bIF1Connected = FALSE;
	m_bIF1Paged = FALSE;
	m_pLECRAM = 0;
	m_pDisk = new CPlus3Disk;
	m_bNMIPending = FALSE;
	m_bSnet = FALSE;
	m_bSnetNMIPending = FALSE;
	m_nRTCDate = 0;
	m_nRTCTime = 0;
	m_nRTCIndex = 4;
	memset (m_RTCLatch, 0, sizeof m_RTCLatch);
	m_bPrevED = FALSE;
	m_nMMCData = 0x00;		// registros SPI a 0 al encender (como en zxp3 y el ZX-Uno);
	m_nDivData = 0x00;		// el cargador de CP/M de Otivax detecta la interfaz por esto
	m_bDivMMC = FALSE;
	m_bDivROM = FALSE;
	m_bDivActive = FALSE;
	m_bDivAutomap = FALSE;
	m_bDivAutomapLoad = FALSE;
	m_nDivControl = 0;
	m_pDivRAM = new u8[128 * 1024];
	memset (m_pDivRAM, 0, 128 * 1024);
	// (m_nMMCData y m_nDivData se ponen a 0 más arriba)
	m_bIF1ROM = FALSE;
	m_FallbackFrom = ModelCount;
	for (unsigned i = 0; i < ModelCount; i++)
	{
		m_pModelROM[i] = 0;
	}
	m_bLECPaged = FALSE;
	m_bLECUsed = FALSE;
	m_nLECPage = 0;
	memset (m_IF1Page, 0xFF, sizeof m_IF1Page);	// sin ROM hasta SetIF1ROM ()
	memset (m_NoRAM, 0xFF, sizeof m_NoRAM);

	for (unsigned i = 0; i < 16; i++)
	{
		m_StdColor[i] = RGB565 (SpectrumPaletteRGB[i]);
	}
	m_bULAplusEnabled = TRUE;
	m_bULAplusActive = FALSE;
	m_nULAplusRegister = 0;
	for (unsigned i = 0; i < 64; i++)
	{
		SetULAplusColor (i, 0);
	}
	m_bMFEnabled = TRUE;
	m_bMFPaged = FALSE;
	m_bMFLocked = TRUE;
	for (unsigned i = 0; i < 3; i++)
	{
		m_bMFHasROM[i] = FALSE;
	}
	memset (m_MFPage, 0, sizeof m_MFPage);

	for (unsigned nAttr = 0; nAttr < 256; nAttr++)
	{
		u8 nBright = (nAttr & 0x40) >> 3;
		m_Ink[nAttr]   = (nAttr & 0x07) | nBright;
		m_Paper[nAttr] = ((nAttr >> 3) & 0x07) | nBright;
	}

	// Tabla con el offset del primer byte de cada línea de pantalla
	for (unsigned nLine = 0; nLine < 192; nLine++)
	{
		m_ScrAddr[nLine] = ((nLine & 0xC0) << 5) | ((nLine & 0x07) << 8) | ((nLine & 0x38) << 2);
	}

	for (unsigned nAddr = 0; nAddr < 0x1800; nAddr++)
	{
		unsigned nRow  = ((nAddr & 0xE0) >> 5) | ((nAddr & 0x1800) >> 8);
		unsigned nCol  = nAddr & 0x1F;
		unsigned nScan = (nAddr & 0x700) >> 8;
		m_BufAddr[nAddr] = (nRow * 8 + nScan + BORDER_TOP) * SCREEN_WIDTH + nCol * 8 + BORDER_LEFT;
		m_Scr2Attr[nAddr] = 0x1800 + nRow * 32 + nCol;
	}

	memset (m_Screen, 0, sizeof m_Screen);

	m_CPU.setBreakpoint (true);

	m_Audio.SetTape (m_pTape);

	SetModel (Model48K);
}

CSpectrum::~CSpectrum (void)
{
	FreeRecording ();
	delete [] m_pLECRAM;
	delete m_pDisk;
	delete [] m_pDivRAM;
	m_pKeyboard = 0;
	m_pTape = 0;
}

const u8 *CSpectrum::GetROMFont (void) const
{
	return rom_48 + 0x3D00;
}

// ¿Hay ROM para el modelo? El 16K/48K y el 128K siempre (van en el kernel)
boolean CSpectrum::IsModelAvailable (TModel Model) const
{
	return    Model == Model16K || Model == Model48K || Model == Model128K
	       || m_pModelROM[Model] != 0;
}

void CSpectrum::SetModelROM (TModel Model, const u8 *pROM)
{
	m_pModelROM[Model] = pROM;
	if (Model == Model48K)
	{
		m_pModelROM[Model16K] = pROM;
	}
}

void CSpectrum::SetIF1ROM (const u8 *pROM)
{
	memcpy (m_IF1Page, pROM, 0x2000);
	memcpy (m_IF1Page + 0x2000, pROM, 0x2000);
	m_bIF1ROM = TRUE;
}

void CSpectrum::SetModel (TModel Model)
{
	// sin su ROM, el +2/+2A/+3/+3e no se puede emular: se usa el 128K
	if (!IsModelAvailable (Model))
	{
		m_FallbackFrom = Model;
		Model = Model128K;
	}

	m_Model = Model;
	m_pTiming = &MachineTiming[Model];

	const u8 *pROM = m_pModelROM[Model];
	switch (m_pTiming->CodeModel)
	{
	case CodeModel48K:
		if (pROM == 0)
		{
			pROM = rom_48;
		}
		m_ROM[0] = m_ROM[1] = m_ROM[2] = m_ROM[3] = pROM;
		m_pBasicROM = pROM;
		break;

	case CodeModel128K:
		if (pROM == 0)
		{
			m_ROM[0] = m_ROM[2] = rom_128_0;
			m_ROM[1] = m_ROM[3] = rom_128_1;
		}
		else
		{
			m_ROM[0] = m_ROM[2] = pROM;
			m_ROM[1] = m_ROM[3] = pROM + 0x4000;
		}
		m_pBasicROM = m_ROM[1];
		break;

	default:		// +2A, +3 y +3e: cuatro bancos de 16 KB
		for (unsigned i = 0; i < 4; i++)
		{
			m_ROM[i] = pROM + i * 0x4000;
		}
		m_pBasicROM = m_ROM[3];
		break;
	}

	UpdateDivActive ();

	BuildTables ();

	m_Audio.Configure (m_pTiming->nClockFreq, m_pTiming->bHasAY ? &m_AY : 0);
	m_pTape->SetMachine (m_pTiming->nClockFreq, m_pTiming->CodeModel == CodeModel48K);

	Reset ();
}

void CSpectrum::Reset (void)
{
	memset (m_RAM, 0, sizeof m_RAM);

	m_nPortFE = 0;
	m_nPort7FFD = 0;
	m_nPort1FFD = 0;
	m_bEarOut = FALSE;
	m_bMFPaged = FALSE;
	m_bMFLocked = TRUE;
	m_bULAplusActive = FALSE;
	m_nULAplusRegister = 0;
	m_bIF1Paged = FALSE;
	m_IF1.Reset ();
	m_bLECPaged = FALSE;
	m_bLECUsed = FALSE;
	m_pDisk->Reset ();
	m_SDCard.Select (FALSE);
	m_bDivAutomap = FALSE;
	m_bDivAutomapLoad = FALSE;
	m_nDivControl = 0;		// CONMEM y MAPRAM se borran con el reset
	m_bPrevED = FALSE;
	m_bSnetNMIPending = FALSE;
	if (m_bSnet)
	{
		m_Snet.Reset ();
	}
	UpdateDivActive ();
	UpdatePaging ();

	m_AY.Reset ();
	m_Audio.Reset ();

	m_CPU.reset ();
	m_nTstates = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Memoria

void CSpectrum::UpdatePaging (void)
{
	switch (m_pTiming->CodeModel)
	{
	case CodeModel48K:
		m_pReadPage[0] = m_ROM[0];
		m_pReadPage[1] = m_RAM[5];
		m_pReadPage[2] = m_RAM[2];
		m_pReadPage[3] = m_RAM[0];
		if (m_Model == Model16K)
		{
			m_pReadPage[2] = m_pReadPage[3] = m_NoRAM;	// las escrituras se pierden
		}
		m_pScreenPage = m_RAM[5];
		break;

	case CodeModel128K:
		m_pReadPage[0] = m_ROM[(m_nPort7FFD >> 4) & 1];
		m_pReadPage[1] = m_RAM[5];
		m_pReadPage[2] = m_RAM[2];
		m_pReadPage[3] = m_RAM[m_nPort7FFD & 7];
		m_pScreenPage = m_RAM[(m_nPort7FFD & 0x08) ? 7 : 5];
		break;

	case CodeModelPlus3:
		if (m_nPort1FFD & 0x01)
		{
			// Modos especiales "all RAM"
			static const u8 Special[4][4] =
			{
				{0, 1, 2, 3}, {4, 5, 6, 7}, {4, 5, 6, 3}, {4, 7, 6, 3}
			};
			const u8 *pBanks = Special[(m_nPort1FFD >> 1) & 3];
			for (unsigned i = 0; i < 4; i++)
			{
				m_pReadPage[i] = m_RAM[pBanks[i]];
			}
		}
		else
		{
			unsigned nROM = ((m_nPort1FFD >> 1) & 0x02) | ((m_nPort7FFD >> 4) & 0x01);
			m_pReadPage[0] = m_ROM[nROM];
			m_pReadPage[1] = m_RAM[5];
			m_pReadPage[2] = m_RAM[2];
			m_pReadPage[3] = m_RAM[m_nPort7FFD & 7];
		}
		m_pScreenPage = m_RAM[(m_nPort7FFD & 0x08) ? 7 : 5];
		break;
	}

	for (unsigned nSlot = 0; nSlot < 4; nSlot++)
	{
		const u8 *pPage = m_pReadPage[nSlot];

		int nBank = -1;
		for (unsigned i = 0; i < 8; i++)
		{
			if (pPage == m_RAM[i])
			{
				nBank = i;
				break;
			}
		}

		m_pWritePage[nSlot] = nBank >= 0 ? m_RAM[nBank] : m_ROMSink;

		switch (m_pTiming->CodeModel)
		{
		case CodeModel48K:
			m_bContendedRAM[nSlot] = nSlot == 1;
			m_bContendedIO[nSlot] = nSlot == 1;
			break;

		case CodeModel128K:
			m_bContendedRAM[nSlot] = nBank >= 0 && (nBank & 1);
			m_bContendedIO[nSlot] = m_bContendedRAM[nSlot];
			break;

		case CodeModelPlus3:
			m_bContendedRAM[nSlot] = nBank >= 4;
			m_bContendedIO[nSlot] = FALSE;
			break;
		}
	}

	// LEC paginada: sus 32 KB ocupan 0x0000-0x7FFF y no hay ROM ni periféricos
	// que se superpongan (como en JSpeccy)
	if (m_bLECPaged && LECActive ())
	{
		for (unsigned nSlot = 0; nSlot < 2; nSlot++)
		{
			u8 *pPage;
			if (m_nLECPage == 15)
			{
				pPage = m_RAM[nSlot == 0 ? 2 : 0];	// la RAM alta del propio 48K
			}
			else
			{
				pPage = m_pLECRAM + m_nLECPage * 0x8000 + nSlot * 0x4000;
			}
			m_pReadPage[nSlot] = pPage;
			m_pWritePage[nSlot] = pPage;
			m_bContendedRAM[nSlot] = FALSE;
			m_bContendedIO[nSlot] = FALSE;
		}
		return;
	}

	// Cartucho del Interface 2 (no en el +2A/+3)
	if (m_bIF2Inserted && m_pTiming->CodeModel != CodeModelPlus3)
	{
		m_pReadPage[0] = m_IF2ROM;
		m_pWritePage[0] = m_ROMSink;
		m_bContendedRAM[0] = FALSE;
		m_bContendedIO[0] = FALSE;
	}

	// ROM del Interface 1 (8 KB, repetida en los 16 KB)
	if (m_bIF1Paged && IF1Active ())
	{
		m_pReadPage[0] = m_IF1Page;
		m_pWritePage[0] = m_ROMSink;
		m_bContendedRAM[0] = FALSE;
		m_bContendedIO[0] = FALSE;
	}

	// Multiface paginado: su ROM y su RAM ocupan los primeros 16 KB
	if (m_bMFPaged)
	{
		if (m_pTiming->CodeModel == CodeModelPlus3 && (m_nPort1FFD & 0x01))
		{
			m_bMFPaged = FALSE;	// modo "all RAM" del +2A: el MF se retira
		}
		else
		{
			m_pReadPage[0] = m_MFPage;
			m_pWritePage[0] = m_MFPage;
			m_bContendedRAM[0] = FALSE;
			m_bContendedIO[0] = FALSE;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Multiface
//
// Basado en JSpeccy (machine/Memory.java y machine/Spectrum.java):
//  - El NMI (botón rojo) pagina el MF y lo desbloquea al leer la dirección 0x0066.
//  - MF1 (48K):   IN 0x9F pagina, IN 0x1F despagina.
//  - MF128 (128K): IN 0xBF pagina (si no está bloqueado; 0x01BF devuelve 0x7FFD),
//                 IN 0x3F despagina.
//  - MF3 (+2A):   IN 0x3F pagina (si no está bloqueado; 0x7F3F y 0x1F3F devuelven
//                 0x7FFD y 0x1FFD), IN 0xBF despagina.
//  - MF128/MF3: con el MF paginado y PC < 0x4000, OUT 0x3F bloquea y OUT 0xBF desbloquea.

void CSpectrum::SetMultifaceROM (TCodeModel CodeModel, const u8 *pROM)
{
	if (pROM != 0)
	{
		memcpy (m_MFROM[CodeModel], pROM, 0x2000);
		m_bMFHasROM[CodeModel] = TRUE;
	}
	else
	{
		m_bMFHasROM[CodeModel] = FALSE;
	}
}

boolean CSpectrum::IsMultifaceAvailable (void) const
{
	// con la divMMC, la NMI es suya (el navegador de esxDOS)
	return m_bMFEnabled && m_bMFHasROM[m_pTiming->CodeModel] && !m_bDivActive && !m_bSnet;
}

boolean CSpectrum::InsertCartridge (const u8 *pROM, unsigned nSize)
{
	if (nSize == 0 || nSize > 0x4000 || m_pTiming->CodeModel == CodeModelPlus3)
	{
		return FALSE;
	}

	// Las imágenes de menos de 16 KB se rellenan con 0xFF (bus sin datos)
	memset (m_IF2ROM, 0xFF, sizeof m_IF2ROM);
	memcpy (m_IF2ROM, pROM, nSize);
	m_bIF2Inserted = TRUE;
	Reset ();

	return TRUE;
}

void CSpectrum::EjectCartridge (void)
{
	if (m_bIF2Inserted)
	{
		m_bIF2Inserted = FALSE;
		Reset ();
	}
}

boolean CSpectrum::SetLECConnected (boolean bConnected)
{
	if (bConnected && m_pLECRAM == 0)
	{
		m_pLECRAM = new u8[15 * 0x8000];	// 480 KB
		if (m_pLECRAM == 0)
		{
			return FALSE;
		}
		memset (m_pLECRAM, 0, 15 * 0x8000);
	}
	else if (!bConnected && m_pLECRAM != 0)
	{
		m_bLECPaged = FALSE;
		m_bLECUsed = FALSE;
		delete [] m_pLECRAM;
		m_pLECRAM = 0;
		UpdatePaging ();
	}

	return TRUE;
}

// En los 128K, el puerto de la LEC (0x??FD) es también el de la paginación
// (0x7FFD) y el del chip AY (0xBFFD, 0xFFFD). En la máquina real chocarían; el
// emulador solo toma para la LEC las escrituras con la forma del CP/M de LEC:
// OUT (0xFD),A, en las que el byte alto del puerto es el propio valor. Así el
// AY y el resto del Spectrum siguen funcionando.
boolean CSpectrum::IsLECPort (u16 nPort, u8 nValue) const
{
	if (m_Model == Model48K)
	{
		return (nPort & 0x0002) == 0;
	}
	return (nPort & 0xFF) == 0xFD && (nPort >> 8) == nValue;
}

// 128K en modo 48K: paginación bloqueada y la ROM del BASIC de 48K
boolean CSpectrum::LEC128Mode (void) const
{
	if (m_Model == Model48K || m_Model == Model16K || (m_nPort7FFD & 0x20) == 0)
	{
		return FALSE;
	}
	if (m_pTiming->CodeModel == CodeModelPlus3)
	{
		if (m_nPort1FFD & 0x01)
		{
			return FALSE;		// paginación especial (todo RAM)
		}
		unsigned nROM = ((m_nPort1FFD >> 1) & 0x02) | ((m_nPort7FFD >> 4) & 0x01);
		return nROM == 3;
	}
	return (m_nPort7FFD & 0x10) != 0;
}

void CSpectrum::LECWrite (u8 nValue)
{
	if (nValue & 0x80)
	{
		// número de página en los bits b3 b6 b5 b4
		m_nLECPage = ((nValue & 0x70) >> 4) | (nValue & 0x08);
		m_bLECPaged = TRUE;
		m_bLECUsed = TRUE;
	}
	else
	{
		m_bLECPaged = FALSE;
	}

	UpdatePaging ();
}

void CSpectrum::SetIF1Connected (boolean bConnected)
{
	if (!m_bIF1ROM || m_bDivActive || m_bSnet)
	{
		bConnected = FALSE;		// sin roms/if1.rom, o con la divMMC (puertos 0xE7/0xEB), no hay IF1
	}
	m_bIF1Connected = bConnected;
	if (!bConnected && m_bIF1Paged)
	{
		m_bIF1Paged = FALSE;
		UpdatePaging ();
	}
}

void CSpectrum::SetULAplusColor (unsigned nIndex, u8 nColor)
{
	m_ULAplusPalette[nIndex] = nColor;
	m_ULAplusColor[nIndex] = RGB565 (ULAplusToRGB (nColor));
}

void CSpectrum::ULAplusWrite (uint16_t nPort, u8 nValue)
{
	if ((nPort & 0x4000) == 0)
	{
		// registro: grupo 00 = paleta (entrada 0-63), grupo 01 = modo
		m_nULAplusRegister = (nValue & 0xC0) == 0x40 ? 0x40 : (nValue & 0x3F);
		return;
	}

	// Antes de cambiar colores o modo se dibuja todo lo anterior con los actuales
	UpdateScreen (m_nTstates);
	UpdateBorder (m_nTstates);

	if (m_nULAplusRegister == 0x40)
	{
		m_bULAplusActive = (nValue & 0x01) != 0;
	}
	else
	{
		SetULAplusColor (m_nULAplusRegister, nValue);
	}
}

void CSpectrum::PageMultiface (void)
{
	if (   m_bMFPaged
	    || !IsMultifaceAvailable ()
	    || (m_pTiming->CodeModel == CodeModelPlus3 && (m_nPort1FFD & 0x01)))
	{
		return;
	}

	memcpy (m_MFPage, m_MFROM[m_pTiming->CodeModel], 0x2000);
	m_bMFPaged = TRUE;
	UpdatePaging ();
}

void CSpectrum::UnpageMultiface (void)
{
	if (!m_bMFPaged)
	{
		return;
	}

	m_bMFPaged = FALSE;
	UpdatePaging ();
}

boolean CSpectrum::MultifaceIn (uint16_t nPort, u8 *pValue)
{
	if (!IsMultifaceAvailable ())
	{
		return FALSE;
	}

	u8 nLow = nPort & 0xFF;

	switch (m_pTiming->CodeModel)
	{
	case CodeModel48K:
		if (nLow == 0x9F)
		{
			PageMultiface ();
		}
		else if (nLow == 0x1F && m_bMFPaged)
		{
			UnpageMultiface ();		// el MF1 también tenía un puerto Kempston
		}
		break;

	case CodeModel128K:
		if (nLow == 0xBF && !m_bMFLocked)
		{
			if (nPort == 0x01BF && m_bMFPaged)
			{
				*pValue = m_nPort7FFD;
				return TRUE;
			}
			PageMultiface ();
		}
		else if (nLow == 0x3F && m_bMFPaged)
		{
			UnpageMultiface ();
		}
		break;

	case CodeModelPlus3:
		if (nLow == 0xBF && m_bMFPaged)
		{
			UnpageMultiface ();
		}
		else if (nLow == 0x3F)
		{
			if (!m_bMFLocked && m_bMFPaged)
			{
				if (nPort == 0x7F3F)
				{
					*pValue = m_nPort7FFD;
					return TRUE;
				}
				if (nPort == 0x1F3F)
				{
					*pValue = m_nPort1FFD;
					return TRUE;
				}
			}

			if (!m_bMFLocked)
			{
				PageMultiface ();
			}
		}
		break;
	}

	return FALSE;
}

void CSpectrum::MultifaceOut (uint16_t nPort)
{
	if (   !IsMultifaceAvailable ()
	    || !m_bMFPaged
	    || m_pTiming->CodeModel == CodeModel48K
	    || m_CPU.getRegPC () >= 0x4000)
	{
		return;
	}

	u8 nLow = nPort & 0xFF;
	if (nLow == 0x3F)
	{
		m_bMFLocked = TRUE;
	}
	else if (nLow == 0xBF)
	{
		m_bMFLocked = FALSE;
	}
}

inline void CSpectrum::Contend (uint16_t nAddress, int nTstates)
{
	if (m_bContendedRAM[nAddress >> 14])
	{
		m_nTstates += m_Delay[m_nTstates];
	}
	m_nTstates += nTstates;
}

inline boolean CSpectrum::IsScreenWrite (uint16_t nAddress) const
{
	return    m_pWritePage[nAddress >> 14] == m_pScreenPage
	       && (nAddress & 0x3FFF) < 0x1B00;
}

inline void CSpectrum::WriteByte (uint16_t nAddress, uint8_t nValue)
{
	if (nAddress < 0x4000 && DivPaged ())
	{
		DivWrite (nAddress, nValue);
		return;
	}

	if (nAddress < 0x4000 && m_bSnet && m_Snet.IsPaged ())
	{
		m_Snet.Write (nAddress, nValue);
		return;
	}

	if (nAddress < 0x2000 && m_bMFPaged)
	{
		return;		// ROM del Multiface
	}

	if (IsScreenWrite (nAddress))
	{
		UpdateScreen (m_nTstates);
	}

	m_pWritePage[nAddress >> 14][nAddress & 0x3FFF] = nValue;
}

uint8_t CSpectrum::fetchOpcode (uint16_t address)
{
	Contend (address, 4);
	if (m_bDivActive)
	{
		return DivFetch (address);
	}
	if (m_bSnet)
	{
		return SnetFetch (address);
	}
	return m_pReadPage[address >> 14][address & 0x3FFF];
}

// Spectranet: paginación en el ciclo M1, trampa programable y RETN
u8 CSpectrum::SnetFetch (u16 nAddress)
{
	if (m_Snet.M1Early (nAddress))
	{
		// La trampa programable lanza una NMI: la instrucción de esta dirección
		// se ejecuta todavía de la memoria normal, y la tarjeta se pagina al
		// aceptarse la NMI (al leer 0x0066), como hace Fuse
		m_bSnetNMIPending = TRUE;
		m_CPU.triggerNMI ();
	}
	if (m_bSnetNMIPending && nAddress == 0x0066)
	{
		m_bSnetNMIPending = FALSE;
		m_Snet.NMI ();
	}

	u8 nOpcode = (nAddress < 0x4000 && m_Snet.IsPaged ()) ? m_Snet.Read (nAddress)
							      : m_pReadPage[nAddress >> 14][nAddress & 0x3FFF];

	// RETN (ED 45, 55, 5D, 65, 6D, 75, 7D; ED 4D es RETI) borra el biestable de la NMI
	if (m_bPrevED && (nOpcode & 0xC7) == 0x45 && nOpcode != 0x4D)
	{
		m_Snet.RETN ();
	}
	m_bPrevED = nOpcode == 0xED && !m_bPrevED;

	m_Snet.M1Late (nAddress);
	return nOpcode;
}

void CSpectrum::SetSpectranetEnabled (boolean bOn)
{
	m_bSnet = bOn;
	if (bOn)
	{
		m_Snet.Reset ();
	}
}

uint8_t CSpectrum::peek8 (uint16_t address)
{
	Contend (address, 3);
	if (address < 0x4000 && DivPaged ())
	{
		return DivRead (address);
	}
	if (address < 0x4000 && m_bSnet && m_Snet.IsPaged ())
	{
		return m_Snet.Read (address);
	}
	return m_pReadPage[address >> 14][address & 0x3FFF];
}

// divMMC: memoria en 0x0000-0x3FFF cuando está paginada
//   CONMEM:            0x0000 ROM (esxDOS)       0x2000 banco elegido
//   MAPRAM (sin CONMEM): 0x0000 banco 3 (lectura) 0x2000 banco elegido
//   normal:            0x0000 ROM                0x2000 banco elegido
u8 CSpectrum::DivRead (u16 nAddress) const
{
	if (nAddress < 0x2000)
	{
		if (!(m_nDivControl & 0x80) && (m_nDivControl & 0x40))
		{
			return m_pDivRAM[3 * 0x2000 + nAddress];
		}
		return m_DivROM[nAddress];
	}
	return m_pDivRAM[(m_nDivControl & 0x0F) * 0x2000 + (nAddress - 0x2000)];
}

void CSpectrum::DivWrite (u16 nAddress, u8 nValue)
{
	if (nAddress < 0x2000)
	{
		return;			// ROM protegida (el puente de escritura de la EEPROM, cerrado)
	}

	unsigned nBank = m_nDivControl & 0x0F;
	if (!(m_nDivControl & 0x80) && (m_nDivControl & 0x40) && nBank == 3)
	{
		return;			// con MAPRAM, el banco 3 es de solo lectura
	}
	m_pDivRAM[nBank * 0x2000 + (nAddress - 0x2000)] = nValue;
}

// Lectura de una instrucción (ciclo M1) con la paginación automática
u8 CSpectrum::DivFetch (u16 nAddress)
{
	// 0x3D00-0x3DFF: se pagina antes de leer (la instrucción ya es de la divMMC)
	if (!m_bDivAutomap && (nAddress & 0xFF00) == 0x3D00)
	{
		m_bDivAutomap = TRUE;
	}

	u8 nOpcode = (nAddress < 0x4000 && DivPaged ()) ? DivRead (nAddress)
							: m_pReadPage[nAddress >> 14][nAddress & 0x3FFF];

	// los demás puntos de entrada paginan después de leer la instrucción, y
	// 0x1FF8-0x1FFF despagina también después
	if (!m_bDivAutomap)
	{
		if (   nAddress == 0x0000 || nAddress == 0x0008 || nAddress == 0x0038
		    || nAddress == 0x0066 || nAddress == 0x04C6 || nAddress == 0x0562)
		{
			m_bDivAutomap = TRUE;
			m_bDivAutomapLoad = nAddress == 0x0562;	// ver la carga instantánea
		}
	}
	else if (nAddress >= 0x1FF8 && nAddress <= 0x1FFF)
	{
		m_bDivAutomap = FALSE;
		m_bDivAutomapLoad = FALSE;
	}

	return nOpcode;
}

void CSpectrum::SetDivMMCROM (const u8 *pROM)
{
	m_bDivROM = pROM != 0;
	if (pROM != 0)
	{
		memcpy (m_DivROM, pROM, sizeof m_DivROM);
	}
	UpdateDivActive ();
}

void CSpectrum::SetDivMMCEnabled (boolean bOn)
{
	m_bDivMMC = bOn;
	m_bDivAutomap = FALSE;
	m_bDivAutomapLoad = FALSE;
	m_nDivControl = 0;
	UpdateDivActive ();
}

void CSpectrum::poke8 (uint16_t address, uint8_t value)
{
	Contend (address, 3);
	WriteByte (address, value);
}

uint16_t CSpectrum::peek16 (uint16_t address)
{
	Contend (address, 3);
	uint8_t nLSB = (address < 0x4000 && DivPaged ()) ? DivRead (address)
		     : (address < 0x4000 && m_bSnet && m_Snet.IsPaged ()) ? m_Snet.Read (address)
							: m_pReadPage[address >> 14][address & 0x3FFF];

	address++;
	Contend (address, 3);
	uint8_t nMSB = (address < 0x4000 && DivPaged ()) ? DivRead (address)
		     : (address < 0x4000 && m_bSnet && m_Snet.IsPaged ()) ? m_Snet.Read (address)
							: m_pReadPage[address >> 14][address & 0x3FFF];

	return (nMSB << 8) | nLSB;
}

void CSpectrum::poke16 (uint16_t address, RegisterPair word)
{
	Contend (address, 3);
	WriteByte (address, word.byte8.lo);

	address++;
	Contend (address, 3);
	WriteByte (address, word.byte8.hi);
}

void CSpectrum::addressOnBus (uint16_t address, int32_t wstates)
{
	if (   m_bContendedRAM[address >> 14]
	    && m_pTiming->CodeModel != CodeModelPlus3)
	{
		for (int32_t i = 0; i < wstates; i++)
		{
			m_nTstates += m_Delay[m_nTstates] + 1;
		}
	}
	else
	{
		m_nTstates += wstates;
	}
}

void CSpectrum::interruptHandlingTime (int32_t wstates)
{
	m_nTstates += wstates;
}

bool CSpectrum::isActiveINT (void)
{
	int nTstates = m_nTstates;
	if (nTstates >= m_pTiming->nTstatesFrame)
	{
		nTstates -= m_pTiming->nTstatesFrame;
	}

	return nTstates >= 0 && nTstates < m_pTiming->nLengthINT;
}

////////////////////////////////////////////////////////////////////////////////
// Entrada/salida

// Contención de E/S (ver comentario en JSpeccy, Spectrum.preIO/postIO):
//   byte alto en página contenida | bit 0 | patrón
//                 No              |   0   | N:1, C:3
//                 No              |   1   | N:1, N:3
//                 Sí              |   0   | C:1, C:3
//                 Sí              |   1   | C:1, C:1, C:1, C:1
void CSpectrum::PreIO (uint16_t nPort)
{
	if (m_bContendedIO[nPort >> 14])
	{
		m_nTstates += m_Delay[m_nTstates];
	}
	m_nTstates++;
}

void CSpectrum::PostIO (uint16_t nPort)
{
	if (m_pTiming->CodeModel == CodeModelPlus3)
	{
		m_nTstates += 3;
		return;
	}

	if (nPort & 0x0001)
	{
		if (m_bContendedIO[nPort >> 14])
		{
			for (unsigned i = 0; i < 3; i++)
			{
				m_nTstates += m_Delay[m_nTstates] + 1;
			}
		}
		else
		{
			m_nTstates += 3;
		}
	}
	else
	{
		m_nTstates += m_Delay[m_nTstates] + 3;
	}
}

u8 CSpectrum::FloatingBus (void)
{
	const TMachineTiming *T = m_pTiming;

	if (m_nTstates < T->nFirstScrByte || m_nTstates > T->nLastScrUpdate)
	{
		return 0xFF;
	}

	int nCol = (m_nTstates % T->nTstatesLine) - T->nOutOffset;
	if (nCol < 0 || nCol > 124)
	{
		return 0xFF;
	}

	int nRow = m_nTstates / T->nTstatesLine - T->nUpBorderWidth;
	if (nRow < 0 || nRow > 191)
	{
		return 0xFF;
	}

	unsigned nAddr = m_ScrAddr[nRow] + nCol / 4;
	switch (nCol % 8)
	{
	case 0:
		return m_pScreenPage[nAddr];

	case 1:
		return m_pScreenPage[m_Scr2Attr[nAddr]];

	case 2:
		nAddr++;
		return nAddr < 0x1800 ? m_pScreenPage[nAddr] : 0xFF;

	case 3:
		nAddr++;
		return nAddr < 0x1800 ? m_pScreenPage[m_Scr2Attr[nAddr]] : 0xFF;

	default:
		return 0xFF;
	}
}

uint8_t CSpectrum::inPort (uint16_t port)
{
	PreIO (port);
	PostIO (port);

	u8 nMFValue;
	if (MultifaceIn (port, &nMFValue))
	{
		return nMFValue;
	}

	// Reloj de esxDOS (RTC.SYS de ZXMetalCircle)
	if (port == 0x7C3B && m_bDivActive)
	{
		return m_nRTCIndex < 4 ? m_RTCLatch[m_nRTCIndex++] : 0xFF;
	}

	// Spectranet: 0x003B, 0x013B, 0x023B y 0x033B (dirección completa)
	if (m_bSnet && m_Snet.IsPort (port))
	{
		return m_Snet.ReadPort (port, m_nPortFE,
					m_pTiming->CodeModel != CodeModel48K && (m_nPort7FFD & 0x08));
	}

	// divMMC: 0xEB igual que el 0x3F de la ZXMMC
	if (HasDivMMCPorts () && (port & 0xFF) == 0xEB)
	{
		u8 nValue = m_nDivData;
		m_nDivData = m_SDCard.Exchange (0xFF);
		return nValue;
	}

	// ZXMMC: la lectura de 0x3F devuelve lo recibido en la transferencia
	// anterior y lanza otra con 0xFF (como el spi.v de zxp3)
	if (HasZXMMC () && (port & 0xFF) == 0x3F)
	{
		u8 nValue = m_nMMCData;
		m_nMMCData = m_SDCard.Exchange (0xFF);
		return nValue;
	}

	// Disquetera del +3: 0x2FFD estado, 0x3FFD datos
	if (HasDiskDrive ())
	{
		if ((port & 0xF002) == 0x2000)
		{
			return m_pDisk->ReadStatus (GetAbsoluteTstates (), m_pTiming->nClockFreq);
		}
		if ((port & 0xF002) == 0x3000)
		{
			return m_pDisk->ReadData (GetAbsoluteTstates (), m_pTiming->nClockFreq);
		}
	}

	// Interface 1: 0xE7 datos del microdrive, 0xEF control, 0xF7 RS232/red
	if (IF1Active ())
	{
		switch (port & 0x0018)
		{
		case 0x00:
			m_nTstates += MDR_BYTE_TSTATES;
			return m_IF1.ReadDataPort ();

		case 0x08:
			return m_IF1.ReadControlPort ();

		case 0x10:
			return m_IF1.ReadLanPort (GetAbsoluteTstates ());

		default:
			break;
		}
	}

	// Kempston: decodificado solo por A5 = 0, como el interfaz original, el
	// core zxp3 de Kyp y lo que propone José Luis Sánchez. Hay juegos españoles
	// que no leen el 0x1F: Las Tres Luces de Glaurung lee el 0xDF y Silent
	// Shadow el 0xD3 (A6 y A7 a 1), y con A5 = A6 = A7 = 0 (Fuse, MAME) el
	// joystick no les funcionaba. Con A0 = 0 gana la ULA (teclado), para no
	// quitarle el teclado a un programa que lea un puerto par con A5 = 0.
	if (   m_pKeyboard->GetJoystickMode () == JoystickKempston
	    && (port & 0x0021) == 0x0001)
	{
		return m_pKeyboard->ReadKempston ();
	}

	// Ratón Kempston: 0xFADF botones, 0xFBDF X, 0xFFDF Y (decodificación parcial
	// por A0, A5, A8 y A10, como Fuse). Solo si hay ratón y el joystick no es Kempston.
	if (m_pKeyboard->IsMouseActive ())
	{
		if ((port & 0x0121) == 0x0001)
		{
			return m_pKeyboard->ReadMouseButtons ();
		}
		if ((port & 0x0521) == 0x0101)
		{
			return m_pKeyboard->ReadMouseX ();
		}
		if ((port & 0x0521) == 0x0501)
		{
			return m_pKeyboard->ReadMouseY ();
		}
	}

	// Fuller Box (0x7F)
	if (   m_pKeyboard->GetJoystickMode () == JoystickFuller
	    && (port & 0x00FF) == 0x7F)
	{
		return m_pKeyboard->ReadFuller ();
	}

	// ULA: teclado y EAR
	if ((port & 0x0001) == 0)
	{
		m_nFEReads++;
		CountLoaderRead (port);

		boolean bEar = m_bEarOut;
		if (m_pTape->IsPlaying ())
		{
			// el audio avanza primero, para que oiga la cinta en el momento justo
			m_Audio.AdvanceTo (m_nTstates);
			bEar = m_pTape->GetEarBit (GetAbsoluteTstates ());

			// cargadores reconocidos: se salta su bucle de espera del flanco
			if (m_bLoaderAccel && m_TapeMode != TapeModeNormal)
			{
				CheckLoaderAcceleration ();
				bEar = m_pTape->GetEarBit (GetAbsoluteTstates ());
			}

			// ¿Ha cambiado la señal desde la lectura anterior? Un cargador lee
			// al menos una vez en cada pulso, así que ve casi todos los flancos
			u32 nEdges = m_pTape->GetEdgeCount ();
			if (nEdges != m_nSeenEdge)
			{
				m_nSeenEdge = nEdges;
				m_nObservedEdges++;
			}
		}
		else if (IsEarInputActive ())
		{
			// casete real: el nivel de su señal en este instante
			m_Audio.AdvanceTo (m_nTstates);
			bEar = m_pEarInput->GetLevel (m_nTstates);
		}

		u8 nValue = m_pKeyboard->ReadPort (port >> 8);
		if (!bEar)
		{
			nValue &= 0xBF;
		}

		return nValue;
	}

	if (m_bULAplusEnabled && (port & 0x4004) == 0x4000)
	{
		return m_nULAplusRegister == 0x40 ? (m_bULAplusActive ? 0x01 : 0x00)
						  : m_ULAplusPalette[m_nULAplusRegister];
	}

	if (m_pTiming->bHasAY)
	{
		if ((port & 0xC002) == 0xC000)
		{
			return m_AY.ReadRegister ();
		}

		// En el +2A/+3, leer 0xBFFD devuelve lo mismo que 0xFFFD
		if (   m_pTiming->CodeModel == CodeModelPlus3
		    && (port & 0xC002) == 0x8000)
		{
			return m_AY.ReadRegister ();
		}
	}

	if (m_pTiming->CodeModel == CodeModelPlus3)
	{
		return 0xFF;
	}

	u8 nFloatBus = FloatingBus ();

	// Solo en el 128K (no +2/+2A/+3): leer 0x7FFD escribe el valor leído en el puerto
	if (   m_Model == Model128K
	    && (port & 0x8002) == 0
	    && (m_nPort7FFD & 0x20) == 0)
	{
		if ((m_nPort7FFD ^ nFloatBus) & 0x08)
		{
			UpdateScreen (m_nTstates);
		}
		m_nPort7FFD = nFloatBus;
		UpdatePaging ();
	}

	return nFloatBus;
}

void CSpectrum::outPort (uint16_t port, uint8_t value)
{
	PreIO (port);

	// LEC: en el 48K, cualquier puerto con A1 = 0 (tiene preferencia, como en
	// JSpeccy); en los 128K, solo el OUT (0xFD),A del CP/M (ver IsLECPort)
	if (LECActive () && IsLECPort (port, value))
	{
		LECWrite (value);
		PostIO (port);
		return;
	}

	MultifaceOut (port);

	if (m_bSnet && m_Snet.IsPort (port))
	{
		m_Snet.WritePort (port, value);
	}

	if (port == 0x7C3B && m_bDivActive)		// reloj de esxDOS: se fija la hora
	{
		m_RTCLatch[0] = m_nRTCDate & 0xFF;
		m_RTCLatch[1] = m_nRTCDate >> 8;
		m_RTCLatch[2] = m_nRTCTime & 0xFF;
		m_RTCLatch[3] = m_nRTCTime >> 8;
		m_nRTCIndex = 0;
	}

	// divMMC: 0xE3 control (MAPRAM no se puede borrar salvo con reset),
	// 0xE7 bit 0 = CS de la tarjeta 0 (activa en bajo), 0xEB datos SPI
	if (HasDivMMCPorts ())
	{
		switch (port & 0xFF)
		{
		case 0xE3:
			if (m_bDivActive)
			{
				m_nDivControl = value | (m_nDivControl & 0x40);
			}
			break;

		case 0xE7:
			m_SDCard.Select ((value & 0x01) == 0);
			break;

		case 0xEB:
			m_nDivData = m_SDCard.Exchange (value);
			break;
		}
	}

	// ZXMMC: 0x1F bit 0 = CS de la tarjeta (activa en bajo), 0x3F = datos SPI
	if (HasZXMMC ())
	{
		if ((port & 0xFF) == 0x1F)
		{
			m_SDCard.Select ((value & 0x01) == 0);
		}
		else if ((port & 0xFF) == 0x3F)
		{
			m_nMMCData = m_SDCard.Exchange (value);
		}
	}

	if (IF1Active ())
	{
		switch (port & 0x0018)
		{
		case 0x00:
			m_IF1.WriteDataPort (value);
			m_nTstates += MDR_BYTE_TSTATES;
			break;

		case 0x08:
			m_IF1.WriteControlPort (value, GetAbsoluteTstates ());
			break;

		case 0x10:
			m_IF1.WriteLanPort (value, GetAbsoluteTstates ());
			break;

		default:
			break;
		}
	}

	if ((port & 0x0001) == 0)
	{
		if ((m_nPortFE ^ value) & 0x07)
		{
			UpdateBorder (m_nTstates);
		}

		if ((m_nPortFE ^ value) & 0x18)
		{
			m_Audio.AdvanceTo (m_nTstates);
			m_Audio.SetBeeper (BeeperLevel[(value >> 3) & 3]);
		}

		if (m_pTiming->CodeModel != CodeModelPlus3)
		{
			// Issue 3: solo el bit EAR (4); Issue 2 (solo 16K/48K): EAR o MIC (3)
			u8 nIssueMask = (m_bIssue2 && m_pTiming->CodeModel == CodeModel48K) ? 0x18 : 0x10;
			m_bEarOut = (value & nIssueMask) != 0;
		}

		if (m_bRecording && !m_bRecordEar && ((m_nPortFE ^ value) & 0x08))
		{
			RecordEdge (GetAbsoluteTstates ());	// cambio del bit MIC
		}

		m_nPortFE = value;
	}

	// ULAplus: 0xBF3B registro, 0xFF3B datos (decodificación por A2 = 0, como JSpeccy)
	if (m_bULAplusEnabled && (port & 0x0004) == 0)
	{
		ULAplusWrite (port, value);
	}

	if (m_pTiming->bHasAY && (port & 0x8002) == 0x8000)
	{
		if (port & 0x4000)
		{
			m_AY.SetAddressLatch (value);
		}
		else
		{
			m_Audio.AdvanceTo (m_nTstates);
			m_AY.WriteRegister (value);
		}
	}

	PostIO (port);

	if ((m_nPort7FFD & 0x20) != 0)		// paginación bloqueada
	{
		return;
	}

	boolean bPage7FFD = FALSE;
	if (m_pTiming->CodeModel == CodeModel128K)
	{
		bPage7FFD = (port & 0x8002) == 0;
	}
	else if (m_pTiming->CodeModel == CodeModelPlus3)
	{
		bPage7FFD = (port & 0xC002) == 0x4000;

		if ((port & 0xF002) == 0x1000)
		{
			m_nPort1FFD = value;
			UpdatePaging ();

			// bit 3: motor de la disquetera
			if (HasDiskDrive ())
			{
				m_pDisk->SetMotor ((value & 0x08) != 0, GetAbsoluteTstates (), m_pTiming->nClockFreq);
			}
		}

		// 0x3FFD: datos del uPD765
		if (HasDiskDrive () && (port & 0xF002) == 0x3000)
		{
			m_pDisk->WriteData (value, GetAbsoluteTstates (), m_pTiming->nClockFreq);
		}
	}

	if (bPage7FFD)
	{
		if ((m_nPort7FFD ^ value) & 0x08)
		{
			UpdateScreen (m_nTstates);
		}

		m_nPort7FFD = value;
		UpdatePaging ();
	}
}

////////////////////////////////////////////////////////////////////////////////
// Vídeo

int CSpectrum::TstatesToBorderPix (int nTstates)
{
	const TMachineTiming *T = m_pTiming;

	nTstates %= T->nTstatesFrame;
	int nRow = nTstates / T->nTstatesLine;
	int nCol = nTstates % T->nTstatesLine;

	// Primera línea visible y primera columna del borde izquierdo
	int nFirstRow, nLeftStart;
	if (T->CodeModel == CodeModel48K)
	{
		nFirstRow = 64;
		nLeftStart = 200 + (24 - BORDER_LEFT / 2);
	}
	else
	{
		nFirstRow = 63;
		nLeftStart = 204 + (24 - BORDER_LEFT / 2);
	}
	int nLastRow = nFirstRow + 192;

	if (nRow < nFirstRow - BORDER_TOP - 1 || nRow > nLastRow + BORDER_BOTTOM - 1)
	{
		return NO_PIXEL;
	}

	if (nRow == nFirstRow - BORDER_TOP - 1 && nCol < nLeftStart)
	{
		return NO_PIXEL;
	}

	if (nRow == nLastRow + BORDER_BOTTOM - 1 && nCol > 127 + BORDER_RIGHT / 2)
	{
		return NO_PIXEL;
	}

	// Zona de retrazo horizontal (invisible)
	if (nCol > 127 + BORDER_RIGHT / 2 && nCol < nLeftStart)
	{
		return NO_PIXEL;
	}

	// Zona de pantalla (no es borde)
	if (nRow > nFirstRow - 1 && nRow < nLastRow && nCol < 128)
	{
		return NO_PIXEL;
	}

	if (nCol > 176)
	{
		nRow++;
		nCol -= nLeftStart;
	}
	else
	{
		nCol += BORDER_RIGHT / 2 - (BORDER_RIGHT - BORDER_LEFT) / 2;
	}

	nRow -= nFirstRow - BORDER_TOP;

	return nRow * SCREEN_WIDTH + nCol * 2;
}

void CSpectrum::BuildTables (void)
{
	const TMachineTiming *T = m_pTiming;

	// Momentos en que la ULA lee cada byte de la pantalla
	int nStart, nEnd, nOffset;
	switch (T->CodeModel)
	{
	case CodeModel48K:
	default:
		nStart = T->nFirstScrByte; nEnd = 57248; nOffset = 2;
		break;

	case CodeModel128K:
		nStart = 14364; nEnd = T->nLastScrUpdate; nOffset = 0;
		break;

	case CodeModelPlus3:
		nStart = T->nFirstScrByte; nEnd = T->nLastScrUpdate; nOffset = 2;
		break;
	}

	m_nSteps = 0;
	for (int t = nStart; t < nEnd && m_nSteps < 6144; t += 4)
	{
		int nCol = (t % T->nTstatesLine) / 4;
		if (nCol > 31)
		{
			continue;
		}

		int nScan = t / T->nTstatesLine - T->nUpBorderWidth;
		if (nScan < 0 || nScan > 191)
		{
			continue;
		}

		m_StepStates[m_nSteps] = t + nOffset;
		m_StepAddr[m_nSteps] = m_ScrAddr[nScan] + nCol;
		m_nSteps++;
	}

	// Borde
	if (T->CodeModel == CodeModel48K)
	{
		m_nFirstBorderUpdate = (64 - BORDER_TOP) * T->nTstatesLine - BORDER_LEFT / 2;
		m_nLastBorderUpdate = (255 + BORDER_BOTTOM) * T->nTstatesLine + 128 + BORDER_RIGHT;
	}
	else
	{
		m_nFirstBorderUpdate = (63 - BORDER_TOP) * T->nTstatesLine - BORDER_RIGHT / 2;
		m_nLastBorderUpdate = (254 + BORDER_BOTTOM) * T->nTstatesLine + 128 + BORDER_RIGHT;
	}

	for (unsigned i = 0; i < sizeof m_States2Border / sizeof m_States2Border[0]; i++)
	{
		m_States2Border[i] = NO_PIXEL;
	}

	for (int t = m_nFirstBorderUpdate; t < m_nLastBorderUpdate; t += 4)
	{
		int nPix = TstatesToBorderPix (t);
		m_States2Border[t] = m_States2Border[t + 1] =
		m_States2Border[t + 2] = m_States2Border[t + 3] = nPix;
	}

	// Contención
	memset (m_Delay, 0, sizeof m_Delay);
	switch (T->CodeModel)
	{
	case CodeModel48K:
	default:
		for (int nIdx = 14335; nIdx < 57247; nIdx += T->nTstatesLine)
		{
			for (int n = 0; n < 128; n += 8)
			{
				static const u8 Pattern[8] = {6, 5, 4, 3, 2, 1, 0, 0};
				memcpy (&m_Delay[nIdx + n], Pattern, 8);
			}
		}
		break;

	case CodeModel128K:
		for (int nIdx = 14361; nIdx < 58037; nIdx += T->nTstatesLine)
		{
			for (int n = 0; n < 128; n += 8)
			{
				static const u8 Pattern[8] = {6, 5, 4, 3, 2, 1, 0, 0};
				memcpy (&m_Delay[nIdx + n], Pattern, 8);
			}
		}
		break;

	case CodeModelPlus3:
		for (int nIdx = 14361; nIdx < 58036; nIdx += T->nTstatesLine)
		{
			for (int n = 0; n < 128; n += 8)
			{
				static const u8 Pattern[8] = {1, 0, 7, 6, 5, 4, 3, 2};
				memcpy (&m_Delay[nIdx + n], Pattern, 8);
			}
			m_Delay[nIdx + 128] = 1;	// las líneas tienen 129 t-states de contención
		}
		break;
	}

	m_nStep = 0;
	m_nLastChgBorder = m_nFirstBorderUpdate;
}

void CSpectrum::UpdateScreen (int nTstates)
{
	while (m_nStep < m_nSteps && m_StepStates[m_nStep] <= nTstates)
	{
		unsigned nAddr = m_StepAddr[m_nStep++];

		u8 nByte = m_pScreenPage[nAddr];
		u8 nAttr = m_pScreenPage[m_Scr2Attr[nAddr]];

		TPixel nInk, nPaper;
		if (m_bULAplusActive)
		{
			// FLASH y BRIGHT eligen una de las 4 subpaletas de 16 colores
			unsigned nCLUT = (nAttr & 0xC0) >> 2;
			nInk = m_ULAplusColor[nCLUT | (nAttr & 0x07)];
			nPaper = m_ULAplusColor[nCLUT | 0x08 | ((nAttr >> 3) & 0x07)];
		}
		else
		{
			nInk = m_StdColor[m_Ink[nAttr]];
			nPaper = m_StdColor[m_Paper[nAttr]];
			if ((nAttr & 0x80) && m_bFlash)
			{
				TPixel nTemp = nInk; nInk = nPaper; nPaper = nTemp;
			}
		}

		TPixel *pPixel = &m_Screen[m_BufAddr[nAddr]];
		for (unsigned nMask = 0x80; nMask != 0; nMask >>= 1)
		{
			*pPixel++ = (nByte & nMask) ? nInk : nPaper;
		}
	}
}

void CSpectrum::UpdateBorder (int nTstates)
{
	if (nTstates < m_nLastChgBorder || m_nLastChgBorder > m_nLastBorderUpdate)
	{
		return;
	}

	// con ULAplus, el borde usa los colores de papel de la subpaleta 0
	TPixel nColor = m_bULAplusActive ? m_ULAplusColor[8 + (m_nPortFE & 0x07)]
					 : m_StdColor[m_nPortFE & 0x07];

	nTstates += m_pTiming->nOutBorderOffset;
	nTstates &= ~3;

	while (m_nLastChgBorder < nTstates && m_nLastChgBorder < m_nLastBorderUpdate)
	{
		int nPix = m_States2Border[m_nLastChgBorder];
		m_nLastChgBorder += 4;

		if (nPix == NO_PIXEL)
		{
			continue;
		}

		TPixel *pPixel = &m_Screen[nPix];
		for (unsigned i = 0; i < 8; i++)
		{
			pPixel[i] = nColor;
		}
	}

	m_nLastChgBorder = nTstates;
}

////////////////////////////////////////////////////////////////////////////////
// Cuadro

void CSpectrum::RunFrame (void)
{
	const int nTstatesFrame = m_pTiming->nTstatesFrame;

	m_nStep = 0;
	m_nLastChgBorder = m_nFirstBorderUpdate;
	m_Audio.SetFrameStart (m_nFrameStart);

	while (m_nTstates < nTstatesFrame)
	{
		m_CPU.execute ();

		if (m_bNMIPending && m_nTstates >= nTstatesFrame / 2)
		{
			m_bNMIPending = FALSE;
			if (m_bSnet)
			{
				m_bSnetNMIPending = TRUE;	// la NMI la atiende la Spectranet (su menú)
			}
			m_CPU.triggerNMI ();
		}
	}

	UpdateScreen (0x7FFFFFFF);
	UpdateBorder (m_nLastBorderUpdate + 4);

	// grabación de la entrada EAR externa: sus cambios de este cuadro
	if (m_bRecording && m_bRecordEar && m_pEarInput != 0)
	{
		for (unsigned i = 0; i < m_pEarInput->GetEdgeCount (); i++)
		{
			RecordEdge (m_nFrameStart + m_pEarInput->GetEdgeTime (i));
		}
	}

	m_Audio.EndFrame (nTstatesFrame);
	if (HasDiskDrive ())
	{
		m_pDisk->Sync (m_nFrameStart + nTstatesFrame, m_pTiming->nClockFreq);
	}

	m_nTstates -= nTstatesFrame;
	m_nFrameStart += nTstatesFrame;
	// (con ULAplus activo no hay parpadeo: FLASH elige subpaleta)

	if ((++m_nFrames & 0x0F) == 0)
	{
		m_bFlash = !m_bFlash;
	}

	TapeFrameHousekeeping ();
}

////////////////////////////////////////////////////////////////////////////////
// Cinta

void CSpectrum::TapePlay (void)
{
	m_AccelMode = AccelNone;
	m_pTape->Play (GetAbsoluteTstates ());
	m_bAutoStopped = FALSE;
	m_nIdleFrames = 0;
	m_nLoadingFrames = 0;
}

void CSpectrum::TapeStop (void)
{
	m_pTape->Stop ();
	m_bAutoStopped = FALSE;		// parada manual: no arrancar sola
}

void CSpectrum::TapeInserted (void)
{
	m_pTape->Stop ();
	m_bAutoStopped = TRUE;
	m_nIdleFrames = 0;
	m_nLoadingFrames = 0;
}

boolean CSpectrum::IsTapeAccelerating (void) const
{
	return m_TapeMode != TapeModeNormal && m_pTape->IsPlaying ();
}

void CSpectrum::TapeFrameHousekeeping (void)
{
	// Parada pedida por la propia cinta (bloque de parada entre niveles de una
	// multicarga, como Basil the Great Mouse Detective): el cargador puede
	// volver a arrancarla. Antes contaba como parada manual y la cinta ya no
	// arrancaba sola, con el cargador esperando para siempre.
	if (!m_pTape->IsPlaying () && m_pTape->TakeStoppedByBlock ())
	{
		m_bAutoStopped = TRUE;
		m_nIdleFrames = 0;
		m_nLoadingFrames = 0;
	}

	if (m_pTape->IsPlaying ())
	{
		// Si el programa deja de leer la cinta durante un rato, se para. Pero
		// hay cargadores que leen poco: los acelerados (se saltan sus bucles
		// de espera) y los que pasan casi todo el tiempo en retardos, como el
		// de La Abadía del Crimen (unas 150 lecturas por cuadro). Lo que los
		// delata es que ven casi todos los flancos de la cinta: una lectura en
		// cada pulso. Un juego que solo mira el teclado ve muy pocos.
		//
		// Tampoco basta con leer mucho: los menús de algunos juegos miran el
		// teclado miles de veces por cuadro (Teenage Mutant Hero Turtles - The
		// Coin-Op) y la cinta seguía sonando por los bloques de los niveles
		// mientras se elegía en el menú; al pedir el nivel, la cinta ya estaba
		// en otro sitio. Por eso cuentan las lecturas con pinta de cargador
		// (ver CountLoaderRead), no todas.
		u32 nFrameEdges = m_pTape->GetEdgeCount () - m_nFrameEdges;
		boolean bFollowing = nFrameEdges > 0 && m_nObservedEdges * 2 >= nFrameEdges;
		boolean bLoading =    m_nLoaderReads >= LOADER_READS_ACTIVE
				   || m_nAccelEdges > 0
				   || (bFollowing && m_nLoaderReads > 0);
		if (!bLoading)
		{
			if (++m_nIdleFrames >= AUTOSTOP_FRAMES)
			{
				m_pTape->Stop ();
				m_bAutoStopped = TRUE;
				m_nIdleFrames = 0;
			}
		}
		else
		{
			m_nIdleFrames = 0;
		}
	}
	else if (   m_bAutoStopped
		 && m_pTape->IsInserted ()
		 && !m_pTape->IsAtEnd ())
	{
		// Un cargador propio está esperando la señal: se arranca la cinta.
		// Antes bastaba con muchas lecturas, y un menú que mira el teclado sin
		// parar la arrancaba sola (The Coin-Op, en carga instantánea).
		if (m_nLoaderReads > LOADER_READS_PLAY)
		{
			if (++m_nLoadingFrames >= AUTOPLAY_FRAMES)
			{
				TapePlay ();
			}
		}
		else
		{
			m_nLoadingFrames = 0;
		}
	}

	m_nFEReads = 0;
	m_nAccelEdges = 0;
	m_nObservedEdges = 0;
	m_nLoaderReads = 0;
	m_nFrameEdges = m_pTape->GetEdgeCount ();
}

uint8_t CSpectrum::breakpoint (uint16_t address, uint8_t opcode)
{
	if (m_bLECPaged)
	{
		return opcode;		// con la LEC paginada no hay ROM: ningún trap
	}

	// Interface 1: entra al leer 0x0008 (errores/extensiones) o 0x1708 (CLOSE#),
	// sale al leer 0x0700
	if (IF1Active ())
	{
		if ((address == 0x0008 || address == 0x1708) && !m_bIF1Paged && !m_bMFPaged)
		{
			m_bIF1Paged = TRUE;
			UpdatePaging ();
			return m_IF1Page[address];
		}

		if (address == 0x0700 && m_bIF1Paged)
		{
			m_bIF1Paged = FALSE;
			UpdatePaging ();
			return opcode;		// esta instrucción aún es de la ROM del IF1
		}
	}

	// NMI con Multiface: al leer la instrucción en 0x0066 entra la ROM del MF
	if (address == 0x0066 && !m_bMFPaged && IsMultifaceAvailable ())
	{
		PageMultiface ();
		if (m_bMFPaged)
		{
			m_bMFLocked = FALSE;
			return m_MFPage[0x0066];
		}
	}

	// SA-BYTES de la ROM de BASIC (en 0x04D0: el +3 no entra por 0x04C2).
	// En ese punto A = flag, IX = dirección, DE = longitud, y en la pila está
	// la dirección de SA/LD-RET, así que basta con un RET.
	if (   address == 0x04D0
	    && m_pSaveHandler != 0
	    && !m_bRecording
	    && m_pReadPage[0] == m_pBasicROM
	    && FlashSave ())
	{
		return 0xC9;	// RET
	}

	// Carga instantánea, como Fuse: en 0x0562 (el IN A,(FE) de LD-BYTES). A
	// esas alturas la ROM ya ha guardado el flag y LOAD/VERIFY en AF', ha
	// hecho DI, ha puesto el borde y ha apilado la vuelta por SA/LD-RET.
	// También coge a los cargadores que llaman directamente a 0x0562 para
	// usar otro flag con la rutina de la ROM (Cabal, de Ocean).
	if (   address == 0x0562
	    && m_TapeMode == TapeModeFlash
	    && m_pReadPage[0] == m_pBasicROM
	    && m_pTape->IsInserted ()
	    && !m_pTape->IsPlaying ()
	    && FlashLoad ())
	{
		// La divMMC pagina su memoria al leer la instrucción de 0x0562 (esxDOS
		// lo usa para sus cintas). Como la rutina de la ROM ya no se ejecuta,
		// se anula: si no, la vuelta por SA/LD-RET ejecutaba código de esxDOS
		// en lugar de la ROM y el programa se perdía tras el primer bloque.
		if (m_bDivAutomapLoad)
		{
			m_bDivAutomap = FALSE;
			m_bDivAutomapLoad = FALSE;
		}
		return 0xC9;	// RET (ver FlashLoad)
	}
	m_bDivAutomapLoad = FALSE;

	// LD-BYTES de la ROM de BASIC
	if (   address == 0x0556
	    && m_pReadPage[0] == m_pBasicROM
	    && m_pTape->IsInserted ()
	    && !m_pTape->IsPlaying ()
	    && !m_pTape->IsAtEnd ())
	{
		// Con carga instantánea y un bloque estándar, la ROM sigue hasta
		// 0x0562 y allí carga la trampa
		unsigned nLength;
		if (   m_TapeMode == TapeModeFlash
		    && m_pTape->GetStandardBlock (&nLength) != 0
		    && nLength > 0)
		{
			return opcode;
		}

		// Bloques no estándar (turbo, TZX especiales) o modos por pulsos
		TapePlay ();
	}

	return opcode;
}

#define REC_TSTATES_PER_SAMPLE	16
#define REC_MAX_SIZE		(TAPE_MAX_SIZE - 64)

boolean CSpectrum::IsEarInputActive (void) const
{
	return m_pEarInput != 0 && m_pEarInput->IsEnabled () && m_pEarInput->IsActive ();
}

boolean CSpectrum::StartRecording (boolean bEarInput)
{
	m_bRecordEar = bEarInput;
	FreeRecording ();

	m_nRecCapacity = 256 * 1024;
	m_pRecBuffer = new u8[m_nRecCapacity];
	if (m_pRecBuffer == 0)
	{
		m_nRecCapacity = 0;
		return FALSE;
	}

	m_nRecLength = 0;
	m_nRecPulses = 0;
	m_nRecRemainder = 0;
	m_nRecLastEdge = GetAbsoluteTstates ();
	m_bRecording = TRUE;

	return TRUE;
}

void CSpectrum::RecordEdge (u64 nTime)
{
	u64 nTotal = nTime - m_nRecLastEdge + m_nRecRemainder;
	m_nRecLastEdge = nTime;

	u64 nSamples = nTotal / REC_TSTATES_PER_SAMPLE;
	m_nRecRemainder = nTotal % REC_TSTATES_PER_SAMPLE;
	if (nSamples == 0)
	{
		nSamples = 1;
	}
	if (nSamples > 0xFFFFFFFF)
	{
		nSamples = 0xFFFFFFFF;
	}

	// crece el búfer si hace falta
	if (m_nRecLength + 5 > m_nRecCapacity)
	{
		unsigned nNew = m_nRecCapacity * 2;
		if (nNew > REC_MAX_SIZE)
		{
			nNew = REC_MAX_SIZE;
		}
		if (m_nRecLength + 5 > nNew)
		{
			return;		// grabación demasiado larga: se descarta el resto
		}

		u8 *pNew = new u8[nNew];
		if (pNew == 0)
		{
			return;
		}
		memcpy (pNew, m_pRecBuffer, m_nRecLength);
		delete [] m_pRecBuffer;
		m_pRecBuffer = pNew;
		m_nRecCapacity = nNew;
	}

	if (nSamples < 256)
	{
		m_pRecBuffer[m_nRecLength++] = (u8) nSamples;
	}
	else
	{
		m_pRecBuffer[m_nRecLength++] = 0;
		m_pRecBuffer[m_nRecLength++] = nSamples & 0xFF;
		m_pRecBuffer[m_nRecLength++] = (nSamples >> 8) & 0xFF;
		m_pRecBuffer[m_nRecLength++] = (nSamples >> 16) & 0xFF;
		m_pRecBuffer[m_nRecLength++] = (nSamples >> 24) & 0xFF;
	}

	m_nRecPulses++;
}

void CSpectrum::StopRecording (void)
{
	if (m_bRecording)
	{
		RecordEdge (GetAbsoluteTstates ());	// cierra el último pulso
		m_bRecording = FALSE;
	}
}

const u8 *CSpectrum::GetRecording (unsigned *pLength, unsigned *pPulses, unsigned *pSampleRate) const
{
	*pLength = m_nRecLength;
	*pPulses = m_nRecPulses;
	// muestras de 16 t-states del reloj del modelo (tiempo real, como una cinta)
	*pSampleRate = m_pTiming->nClockFreq / REC_TSTATES_PER_SAMPLE;
	return m_pRecBuffer;
}

void CSpectrum::FreeRecording (void)
{
	m_bRecording = FALSE;
	delete [] m_pRecBuffer;
	m_pRecBuffer = 0;
	m_nRecLength = m_nRecCapacity = m_nRecPulses = 0;
}

boolean CSpectrum::FlashSave (void)
{
	static u8 Block[0x10000 + 2];

	unsigned nBytes = m_CPU.getRegDE ();
	u16 nAddr = m_CPU.getRegIX ();
	if (nBytes > 0xFFFF - 2)
	{
		return FALSE;
	}

	u8 nParity = m_CPU.getRegA ();
	Block[0] = nParity;
	for (unsigned i = 0; i < nBytes; i++, nAddr++)
	{
		u8 nByte = m_pReadPage[nAddr >> 14][nAddr & 0x3FFF];
		Block[i + 1] = nByte;
		nParity ^= nByte;
	}
	Block[nBytes + 1] = nParity;

	return m_pSaveHandler->SaveTapeBlock (Block, nBytes + 2);
}

static u8 Parity (u8 nValue)
{
	nValue ^= nValue >> 4;
	nValue ^= nValue >> 2;
	nValue ^= nValue >> 1;
	return (nValue & 1) ? 0 : 0x04;		// P/V = 1 si paridad par
}

// Flags de CP n (A - n), con los bits 3 y 5 del operando (como el Z80)
static u8 FlagsCP (u8 nA, u8 nN)
{
	u8 nRes = nA - nN;
	u8 nF = (nRes & 0x80) | (nRes == 0 ? 0x40 : 0) | (nN & 0x28) | 0x02;
	if ((nA & 0x0F) < (nN & 0x0F))			nF |= 0x10;	// H
	if ((nA ^ nN) & (nA ^ nRes) & 0x80)		nF |= 0x04;	// V
	if (nA < nN)					nF |= 0x01;	// C
	return nF;
}

// Flags de XOR / AND-OR lógicos: S, Z, 5, 3, P; H = N = C = 0
static u8 FlagsLogic (u8 nRes)
{
	return (nRes & 0xA8) | (nRes == 0 ? 0x40 : 0) | Parity (nRes);
}

// Carga instantánea: hace lo que haría LD-BYTES de la ROM desde 0x0562 (con el
// flag pedido en A' y LOAD/VERIFY en el acarreo de F') y deja los registros
// como tape_trap_load_block de Fuse (tape_traps.c, GPL 2 o posterior):
//
//	A  = paridad calculada (o la diferencia con el flag pedido)
//	F  = como tras CP 1 / XOR; acarreo = cargado bien
//	B  = 0xB0 (bien) o 0x00 (fallo al final del bloque)
//	C  = 0x01; H = paridad; L = último byte leído (o 1)
//	DE = DE - bytes leídos; IX = IX + bytes leídos
//	AF'= 0x0145
//
// La vuelta la hace el RET que devuelve breakpoint (): si se entró por 0x0556,
// pasa por SA/LD-RET (0x053F), que restaura el borde y hace EI, como al cargar
// de verdad; si se entró directamente en 0x0562 (Cabal), vuelve a quien llamó.
// Antes la trampa estaba en 0x0556 y volvía sin pasar por ahí, sin tocar B, C,
// H, L ni AF': el cargador de Basil the Great Mouse Detective descifraba mal
// sus bloques turbo con lo que quedaba en esos registros.
boolean CSpectrum::FlashLoad (void)
{
	unsigned nLength;
	const u8 *pBlock = m_pTape->GetStandardBlock (&nLength);
	if (pBlock == 0)
	{
		return FALSE;
	}

	if (nLength == 0)
	{
		m_pTape->NextBlock ();
		return FALSE;
	}

	u8 nRequested = m_CPU.getRegAx ();
	boolean bVerify = (m_CPU.getRegFx () & 0x01) == 0;	// sin acarreo = VERIFY
	u16 nDE = m_CPU.getRegDE ();
	u16 nIX = m_CPU.getRegIX ();
	const u8 *pData = pBlock + 1;

	u8 nParity = pBlock[0];
	u8 nL = nParity;
	u8 nA = 0, nF = m_CPU.getFlags (), nB = m_CPU.getRegB ();
	unsigned nRead = nLength - 1;
	if (nRead > nDE)
	{
		nRead = nDE;
	}
	unsigned nProcessed = 0;
	boolean bFail = FALSE;

	if (nDE == 0)
	{
		// el fallo de la ROM con bloques de longitud 0: se lee un byte
		nB = 0xB0;
		nA = nParity;
		nF = FlagsCP (nA, 1);
	}
	else
	{
		m_CPU.setRegAFx (0x0145);

		nA = nRequested ^ nParity;
		nF = FlagsLogic (nA);
		if (nA != 0)
		{
			bFail = TRUE;			// el flag no coincide
		}
		else
		{
			if (nRead > 0)
			{
				nL = pData[nRead - 1];
			}

			for (nProcessed = 0; nProcessed < nRead; nProcessed++)
			{
				u8 nByte = pData[nProcessed];
				nParity ^= nByte;
				if (bVerify)
				{
					if (nByte != ReadMemory ((u16) (nIX + nProcessed)))
					{
						nL = nByte;
						bFail = TRUE;
						break;
					}
				}
				else
				{
					WriteByte ((u16) (nIX + nProcessed), nByte);
				}
			}

			if (!bFail)
			{
				if (nDE == nProcessed && nRead + 1 < nLength)
				{
					nParity ^= pData[nRead];	// byte de paridad
					nA = nParity;
					nF = FlagsCP (nA, 1);	// acarreo si la paridad da 0
					nB = 0xB0;
				}
				else
				{
					// faltan bytes: falla al leer el siguiente (ROM, 0x05EC)
					nB = 0x00;			// INC B desde 0xFF
					nL = 1;
					nF = (nF & 0x01) | 0x50;	// Z y H de INC B
					bFail = TRUE;
				}
			}
		}
	}

	if (bFail)
	{
		nF &= ~0x01;
	}

	m_CPU.setRegA (nA);
	m_CPU.setFlags (nF);
	m_CPU.setRegB (nB);
	m_CPU.setRegC (0x01);
	m_CPU.setRegH (nParity);
	m_CPU.setRegL (nL);
	m_CPU.setRegDE ((u16) (nDE - nProcessed));
	m_CPU.setRegIX ((u16) (nIX + nProcessed));
	m_pTape->NextBlock ();

	return TRUE;
}

//
// Aceleración de cargadores, adaptada de Fuse (loader.c y loader_detector.c,
// © 2006-2018 Philip Kendall y otros, GPL 2 o posterior).
//
// Casi todos los cargadores esperan cada flanco de la cinta en un bucle como
// el de la ROM (LD-SAMPLE, 0x05ED):
//
//	INC B / RET Z / LD A,7F / IN A,(FE) / RRA / [RET NC] / XOR C / AND 20h / JR Z,bucle
//
// B cuenta las vueltas, y al volver el cargador mira B para saber si el pulso
// era corto (bit 0) o largo (bit 1). Cuando el IN está en un bucle reconocido
// y la cinta sabe qué pulso está sonando (solo en los datos), en vez de dar
// todas las vueltas: el flanco llega ya, B queda con un valor de pulso corto o
// largo, el bit 5 de C con el nuevo nivel, el acarreo a 1 (flanco encontrado)
// y se simula el RET del bucle. El tono guía y los sincronismos se emulan
// normalmente, así que el cargador ve una cinta normal, solo que sin esperas.
//

struct TLoaderPatternByte
{
	u8 nAlternatives;		// 0 = cualquier byte
	u8 Values[4];
};

#define LP(v)			{1, {(v)}}
#define LP2(a, b)		{2, {(a), (b)}}
#define LP3(a, b, c)		{3, {(a), (b), (c)}}
#define LP4(a, b, c, d)		{4, {(a), (b), (c), (d)}}
#define LP_ANY			{0, {0}}

#define ROM_LOADER_PREFIX	LP (0x04), LP (0xC8), LP (0x3E), LP3 (0x00, 0x7F, 0xFF)	// INC B; RET Z; LD A,n
#define ROM_LOADER_INPUT	LP (0xDB), LP (0xFE)					// IN A,(FE)
#define ROM_LOADER_EDGE_TEST	LP (0x1F)						// RRA
#define ROM_LOADER_LEVEL_TEST	LP (0xA9), LP (0xE6), LP (0x20), LP (0x28)		// XOR C; AND 20h; JR Z
#define ROM_LOADER_EDGE_ACTION	LP4 (0x00, 0xA7, 0xC8, 0xD0)				// NOP/AND A/RET Z/RET NC

static const TLoaderPatternByte RomLoader[] =
	{ROM_LOADER_PREFIX, ROM_LOADER_INPUT, ROM_LOADER_EDGE_TEST, ROM_LOADER_LEVEL_TEST, LP (0xF4)};
static const TLoaderPatternByte RomLoaderWithEdgeAction[] =
	{ROM_LOADER_PREFIX, ROM_LOADER_INPUT, ROM_LOADER_EDGE_TEST, ROM_LOADER_EDGE_ACTION,
	 ROM_LOADER_LEVEL_TEST, LP (0xF3)};

#define MICROPROSE_PREFIX	ROM_LOADER_PREFIX, LP (0x3E), LP (0x7F), ROM_LOADER_INPUT, ROM_LOADER_EDGE_TEST
static const TLoaderPatternByte MicroproseLoader[] =
	{MICROPROSE_PREFIX, ROM_LOADER_LEVEL_TEST, LP (0xF2)};
static const TLoaderPatternByte MicroproseLoaderWithEdgeAction[] =
	{MICROPROSE_PREFIX, ROM_LOADER_EDGE_ACTION, ROM_LOADER_LEVEL_TEST, LP (0xF1)};

static const TLoaderPatternByte SearchLoader[] =
	{ROM_LOADER_PREFIX, ROM_LOADER_INPUT, LP (0xA9), LP (0xE6), LP (0x40), LP (0x28), LP (0xF5)};
static const TLoaderPatternByte SearchLoaderWithRet[] =
	{ROM_LOADER_PREFIX, ROM_LOADER_INPUT, LP (0xA9), LP (0xE6), LP (0x40), LP (0xD8), LP (0x00),
	 LP (0x28), LP (0xF3)};

#define ALKATRAZ_SUFFIX		ROM_LOADER_INPUT, ROM_LOADER_EDGE_TEST, LP (0xC8), ROM_LOADER_LEVEL_TEST, LP2 (0xF1, 0xF3)
static const TLoaderPatternByte AlkatrazLoader[] =
	{LP (0x03), LP (0xC3), LP_ANY, LP_ANY, ALKATRAZ_SUFFIX};
static const TLoaderPatternByte AlkatrazVariantLoader[] =
	{LP (0x04), LP (0x20), LP (0x01), LP (0xC9), ALKATRAZ_SUFFIX};

static const TLoaderPatternByte DigitalIntegrationLoader[] =
	{LP_ANY, LP_ANY, LP (0x05), LP (0xC8), ROM_LOADER_INPUT, LP (0xA9), LP (0xE6), LP (0x40),
	 LP (0xCA), LP_ANY, LP_ANY};

static const TLoaderPatternByte ModifiedROMPrefix[] =
	{LP (0xD9), LP (0x11), LP_ANY, LP_ANY, LP (0x21), LP_ANY, LP_ANY, LP (0x06), LP (0x05),
	 LP (0xCD), LP_ANY, LP_ANY, LP (0xD0), LP (0xD9)};
static const TLoaderPatternByte ModifiedROMEdge2Prefix[] = {LP (0xD9), LP (0xA7)};

#define PATTERN(p)	p, sizeof p / sizeof p[0]

struct TAccelPattern
{
	const TLoaderPatternByte *pBytes;
	unsigned nLength;
};

static const TAccelPattern IncreasingPatterns[] =
{
	{PATTERN (RomLoader)},
	{PATTERN (RomLoaderWithEdgeAction)},
	{PATTERN (MicroproseLoader)},
	{PATTERN (MicroproseLoaderWithEdgeAction)},
	{PATTERN (SearchLoader)},
	{PATTERN (SearchLoaderWithRet)},
	{PATTERN (AlkatrazLoader)},
	{PATTERN (AlkatrazVariantLoader)},
};

// Lectura de memoria sin contención ni efectos (para reconocer el código)
u8 CSpectrum::PeekLoader (u16 nAddress) const
{
	if (nAddress < 0x4000 && DivPaged ())
	{
		return DivRead (nAddress);
	}
	return m_pReadPage[nAddress >> 14][nAddress & 0x3FFF];
}

boolean CSpectrum::LoaderPatternMatches (u16 nAddress, const TLoaderPatternByte *pPattern,
					 unsigned nLength) const
{
	for (unsigned i = 0; i < nLength; i++)
	{
		const TLoaderPatternByte &P = pPattern[i];
		if (P.nAlternatives == 0)
		{
			continue;
		}

		u8 nByte = PeekLoader ((u16) (nAddress + i));
		boolean bMatch = FALSE;
		for (unsigned j = 0; j < P.nAlternatives; j++)
		{
			if (nByte == P.Values[j])
			{
				bMatch = TRUE;
				break;
			}
		}
		if (!bMatch)
		{
			return FALSE;
		}
	}

	return TRUE;
}

boolean CSpectrum::LoaderWordMatches (u16 nAddress, u16 nValue) const
{
	return    PeekLoader (nAddress) == (nValue & 0xFF)
	       && PeekLoader ((u16) (nAddress + 1)) == (nValue >> 8);
}

// nPC: dirección en la que empezaría el patrón (6 u 8 bytes antes del final del IN)
CSpectrum::TAccelMode CSpectrum::DetectLoader (u16 nPC) const
{
	// Digital Integration cuenta hacia abajo. Esos primeros bytes eligen otras
	// ramas en el detector original de Fuse, así que no pueden empezarlo.
	u8 nFirst = PeekLoader (nPC);
	if (   nFirst != 0x03 && nFirst != 0x04 && nFirst != 0x47
	    && LoaderPatternMatches (nPC, PATTERN (DigitalIntegrationLoader))
	    && LoaderWordMatches ((u16) (nPC + 10), (u16) (m_CPU.getRegPC () - 4)))
	{
		return AccelDecreasing;
	}

	for (unsigned i = 0; i < sizeof IncreasingPatterns / sizeof IncreasingPatterns[0]; i++)
	{
		if (LoaderPatternMatches (nPC, IncreasingPatterns[i].pBytes, IncreasingPatterns[i].nLength))
		{
			return AccelIncreasing;
		}
	}

	return AccelNone;
}

// Technician Ted cambia la espera de LD-EDGE-1 por trabajo útil que forma una
// sola unidad de tiempo con el muestreo: acelerarlo estropea la carga (Fuse)
boolean CSpectrum::ModifiedROMLoader (u16 nPC) const
{
	return    LoaderPatternMatches ((u16) (nPC - 40), PATTERN (ModifiedROMPrefix))
	       && LoaderWordMatches ((u16) (nPC - 30), (u16) (nPC - 24))
	       && LoaderPatternMatches ((u16) (nPC - 8), PATTERN (ModifiedROMEdge2Prefix))
	       && DetectLoader ((u16) (nPC - 6)) == AccelIncreasing;
}

// nPC: dirección siguiente al IN A,(FE)
CSpectrum::TAccelMode CSpectrum::DetectLoaderAtIn (u16 nPC) const
{
	if (ModifiedROMLoader (nPC))
	{
		return AccelNone;
	}

	TAccelMode Mode = DetectLoader ((u16) (nPC - 6));
	if (Mode == AccelNone)
	{
		Mode = DetectLoader ((u16) (nPC - 8));	// Microprose: otro LD A,7F antes del IN
	}

	return Mode;
}

// Se llama en cada lectura del puerto FE con la cinta en marcha, con PC ya
// detrás de la instrucción IN (como en Fuse)
void CSpectrum::CheckLoaderAcceleration (void)
{
	u16 nPC = m_CPU.getRegPC ();

	// un IN en otro sitio: se deja de acelerar y se mira si es otro cargador
	if (m_AccelMode != AccelNone && nPC != m_nAccelPC)
	{
		m_AccelMode = AccelNone;
	}
	if (m_AccelMode == AccelNone)
	{
		m_AccelMode = DetectLoaderAtIn (nPC);
		m_nAccelPC = nPC;
		if (m_AccelMode == AccelNone)
		{
			return;
		}
	}

	// Si el último flanco llegó a su hora, el cargador todavía no lo ha visto:
	// este IN lo verá. Adelantar ahora el siguiente haría perder un flanco.
	if (m_pTape->HadNaturalEdge ())
	{
		m_pTape->ClearNaturalEdge ();
		return;
	}

	CTape::TPulseClass Class = m_pTape->GetPulseClass ();
	if (Class == CTape::PulseUnknown)
	{
		return;				// tono guía, sincronismos, pausas...
	}

	// El pulso en curso termina ya
	m_pTape->ForceEdge (GetAbsoluteTstates ());
	m_nAccelEdges++;
	boolean bEarNow = m_pTape->GetEarBit (GetAbsoluteTstates ());

	// B: valor de un pulso largo o corto, según el sentido de la cuenta
	boolean bHigh = (Class == CTape::PulseLong) != (m_AccelMode == AccelDecreasing);
	m_CPU.setRegB (bHigh ? 0xFE : 0x00);

	// Tras el IN viene el final de la rutina de la ROM (o de una copia exacta):
	// RRA / RET NC / XOR C / AND 20h / JR Z,bucle y, al encontrar el flanco,
	// LD A,C / CPL / LD C,A / AND 7 / OR 8 / OUT (FE),A / SCF / RET. Entonces se
	// hace aquí lo mismo que haría (es lo más rápido).
	static const u8 RomEdgeTail[] = {0x1F, 0xD0, 0xA9, 0xE6, 0x20, 0x28, 0xF3, 0x79, 0x2F, 0x4F,
					 0xE6, 0x07, 0xF6, 0x08, 0xD3, 0xFE, 0x37, 0xC9};
	boolean bRomTail = TRUE;
	for (unsigned i = 0; i < sizeof RomEdgeTail; i++)
	{
		if (PeekLoader ((u16) (nPC + i)) != RomEdgeTail[i])
		{
			bRomTail = FALSE;
			break;
		}
	}
	if (bRomTail)
	{
		// C: nivel tras el flanco, invertido como hace CPL (el borde no importa
		// al cargador); acarreo = flanco encontrado; y el RET
		m_CPU.setRegC ((m_CPU.getRegC () & ~0x20) | (bEarNow ? 0x20 : 0x00));
		m_CPU.setFlags (m_CPU.getFlags () | 0x01);
		u16 nSP = m_CPU.getRegSP ();
		u16 nRet = PeekLoader (nSP) | (PeekLoader ((u16) (nSP + 1)) << 8);
		m_CPU.setRegSP ((u16) (nSP + 2));
		m_CPU.setRegPC (nRet);
		return;
	}

	// Cualquier otro bucle: el IN que acaba de hacer ve ya el nivel tras el
	// flanco (bit 6, EAR), así que el propio bucle encuentra el flanco y sigue
	// por su camino. Antes se hacía siempre el RET como en la ROM; pero hay
	// cargadores cuyo bucle no termina en RET al encontrar el flanco (Hollywood
	// Poker, TZX: espera los dos flancos del bit seguidos, con EXX y DJNZ), y el
	// RET los devolvía a quien los llamó a mitad de bit.
	m_CPU.setRegA ((m_CPU.getRegA () & ~0x40) | (bEarNow ? 0x40 : 0x00));
}

// Una lectura del puerto 0xFE "de cargador": en el mismo IN que la anterior
// (mismo PC y mismo puerto) y con un contador que ha avanzado una unidad entre
// las dos (B en la ROM y casi todos los cargadores, E en el de La Abadía del
// Crimen...). Es el bucle que espera un flanco de la cinta y cuenta lo que
// tarda; Fuse los detecta igual (loader.c, mirando B). Un menú que espera una
// tecla repite el IN sin contar nada, y el teclado se lee fila a fila (cambia
// el puerto), así que no cuentan.
void CSpectrum::CountLoaderRead (u16 nPort)
{
	u16 nPC = m_CPU.getRegPC ();
	u16 nBC = m_CPU.getRegBC (), nDE = m_CPU.getRegDE (), nHL = m_CPU.getRegHL ();
	u8 Regs[6] = {(u8) (nBC >> 8), (u8) nBC, (u8) (nDE >> 8), (u8) nDE, (u8) (nHL >> 8), (u8) nHL};

	if (nPC == m_nLastFEPC && nPort == m_nLastFEPort)
	{
		for (unsigned i = 0; i < 6; i++)
		{
			u8 nDiff = (u8) (Regs[i] - m_LastFERegs[i]);
			if (nDiff == 1 || nDiff == 0xFF)
			{
				m_nLoaderReads++;
				break;
			}
		}
	}

	m_nLastFEPC = nPC;
	m_nLastFEPort = nPort;
	memcpy (m_LastFERegs, Regs, sizeof Regs);
}
