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

CSpectrum::CSpectrum (CZXKeyboard *pKeyboard, CTape *pTape)
:	m_pKeyboard (pKeyboard),
	m_pTape (pTape),
	m_CPU (this),
	m_LEC (m_Mem),
	m_MF (m_Mem),
	m_Model (Model48K),
	m_pTiming (&MachineTiming[Model48K]),
	m_nTstates (0),
	m_nFrameStart (0),
	m_bPlus3Bus (FALSE),
	m_bBusValid (FALSE),
	m_nBusTstates (0),
	m_nBusValue (0xFF),
	m_Div (&m_SDCard),
	m_ZXMMC (&m_SDCard),
	m_TapeMode (TapeModeFast),
	m_nFEReads (0),
	m_nIdleFrames (0),
	m_nLoadingFrames (0),
	m_bAutoStopped (TRUE),
	m_bLoaderAccel (TRUE),
	m_bDiskAccel (FALSE),
	m_bTKDiode (FALSE),
	m_nTKULA (TKULA50Hz),
	m_bTKMicro (FALSE),
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
	m_nResetCount = 0;
	m_pSaveHandler = 0;
	m_bRecording = FALSE;
	m_bRecordEar = FALSE;
	m_pEarInput = 0;
	m_pRecBuffer = 0;
	m_nRecLength = m_nRecCapacity = m_nRecPulses = 0;
	m_nRecLastEdge = 0;
	m_nRecRemainder = 0;
	m_bIssue2 = FALSE;
	m_bIF1Connected = FALSE;
	m_bIF1Paged = FALSE;
	m_pDisk = new CPlus3Disk;
	m_bNMIPending = FALSE;
	m_bSnet = FALSE;
	m_bSnetNMIPending = FALSE;
	m_bPrevED = FALSE;
	m_bIF1ROM = FALSE;
	m_FallbackFrom = ModelCount;
	for (unsigned i = 0; i < ModelCount; i++)
	{
		m_pModelROM[i] = 0;
	}
	memset (m_IF1Page, 0xFF, sizeof m_IF1Page);	// sin ROM hasta SetIF1ROM ()

	m_bSnowEnabled = TRUE;
	m_bSnow = m_bSnow128 = FALSE;

	m_CPU.setBreakpoint (true);

	m_Audio.SetTape (m_pTape);

	SetModel (Model48K);
}

CSpectrum::~CSpectrum (void)
{
	FreeRecording ();
	delete m_pDisk;
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
	UpdateSnow ();
	m_pTiming = &MachineTiming[Model];
	// TK90X/TK95 con la ULA de Microdigital (50 o 60 Hz): sus propios tiempos
	m_bTKMicro = ModelHas (Model, MODEL_TK) && m_nTKULA != TKULAFerranti;
	if (m_bTKMicro)
	{
		m_pTiming = &TKMicroTiming[Model == ModelTK95][m_nTKULA == TKULA60Hz];
	}

	m_Mem.SetModel (Model, m_pTiming, m_pModelROM[Model]);

	UpdateDivActive ();

	m_ULA.SetTiming (m_pTiming);

	m_Audio.Configure (m_pTiming->nClockFreq, m_pTiming->bHasAY ? &m_AY : 0);
	m_pTape->SetMachine (m_pTiming->nClockFreq, m_pTiming->CodeModel == CodeModel48K);

	Reset ();

	m_bPlus3Bus = m_pTiming->CodeModel == CodeModelPlus3;	// bus flotante del +2A/+3
	m_bBusValid = FALSE;
}

void CSpectrum::Reset (void)
{
	m_Mem.Reset ();
	m_nResetCount++;

	m_nPortFE = 0;
	m_bEarOut = FALSE;
	m_MF.Reset ();
	m_ULA.Reset ();
	m_bIF1Paged = FALSE;
	m_IF1.Reset ();
	m_LEC.Reset ();
	m_pDisk->Reset ();
	m_SDCard.Select (FALSE);
	m_Div.Reset ();			// CONMEM y MAPRAM se borran con el reset
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
	m_Mem.UpdatePaging ();
	m_ULA.SetScreenPage (m_Mem.GetScreenPage ());

	// LEC paginada: no hay ROM ni periféricos que se superpongan
	if (m_LEC.Map (m_Mem))
	{
		return;
	}

	// Cartucho del Interface 2 (no en el +2A/+3)
	m_IF2.Map (m_Mem);

	// ROM del Interface 1 (8 KB, repetida en los 16 KB)
	if (m_bIF1Paged && IF1Active ())
	{
		m_Mem.MapSlot (0, m_IF1Page, m_Mem.GetROMSink ());
	}

	// Multiface paginado: su ROM y su RAM ocupan los primeros 16 KB
	m_MF.Map (m_Mem);
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

boolean CSpectrum::IsMultifaceAvailable (void) const
{
	// con la divMMC, la NMI es suya (el navegador de esxDOS)
	return m_MF.IsReady () && !m_Div.IsActive () && !m_bSnet;
}

boolean CSpectrum::InsertCartridge (const u8 *pROM, unsigned nSize)
{
	if (!m_IF2.Insert (pROM, nSize, m_pTiming->CodeModel))
	{
		return FALSE;
	}
	Reset ();

	return TRUE;
}

void CSpectrum::EjectCartridge (void)
{
	if (m_IF2.Eject ())
	{
		Reset ();
	}
}

boolean CSpectrum::SetLECConnected (boolean bConnected)
{
	boolean bWasConnected = m_LEC.IsConnected ();
	if (!m_LEC.SetConnected (bConnected))
	{
		return FALSE;
	}
	if (bWasConnected && !bConnected)
	{
		UpdatePaging ();
	}

	return TRUE;
}

void CSpectrum::SetIF1Connected (boolean bConnected)
{
	if (!m_bIF1ROM || m_Div.IsActive () || m_bSnet)
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

void CSpectrum::PageMultiface (void)
{
	if (IsMultifaceAvailable () && m_MF.Page ())
	{
		UpdatePaging ();
	}
}

void CSpectrum::UnpageMultiface (void)
{
	if (m_MF.Unpage ())
	{
		UpdatePaging ();
	}
}

inline void CSpectrum::Contend (uint16_t nAddress, int nTstates)
{
	if (m_Mem.IsContended (nAddress >> 14))
	{
		m_nTstates += m_ULA.Delay (m_nTstates);
	}
	m_nTstates += nTstates;
}

inline boolean CSpectrum::IsScreenWrite (uint16_t nAddress) const
{
	return    m_Mem.WritePage (nAddress >> 14) == m_ULA.GetScreenPage ()
	       && (nAddress & 0x3FFF) < 0x1B00;
}

inline void CSpectrum::WriteByte (uint16_t nAddress, uint8_t nValue)
{
	if (m_bPlus3Bus && (nAddress & 0xE000) == 0x4000)
	{
		NoteBus (nValue);
	}
	if (nAddress < 0x4000 && m_Div.IsPaged ())
	{
		m_Div.Write (nAddress, nValue);
		return;
	}

	if (nAddress < 0x4000 && m_bSnet && m_Snet.IsPaged ())
	{
		m_Snet.Write (nAddress, nValue);
		return;
	}

	if (m_MF.IsROM (nAddress))
	{
		return;		// ROM del Multiface
	}

	if (IsScreenWrite (nAddress))
	{
		m_ULA.UpdateScreen (m_nTstates);
	}

	m_Mem.WritePage (nAddress >> 14)[nAddress & 0x3FFF] = nValue;
}

uint8_t CSpectrum::fetchOpcode (uint16_t address)
{
	Contend (address, 4);
	if (m_bSnow || m_bSnow128)
	{
		u8 nI = m_CPU.getRegI () & 0xC0;
		if (nI == 0x40)
		{
			Snow (m_ULA.GetScreenPage ());
		}
		else if (nI == 0xC0 && m_bSnow128 && (m_Mem.GetPort7FFD () & 0x01))
		{
			// 128K y +2 con una página lenta (impar) arriba: la fila
			// equivocada sale del banco 1 o 3 (si es la 1 o la 3) o del 5
			// o 7 (si es la 5 o la 7), el de la pantalla que se ve (Weiv,
			// como ESPectrum)
			Snow (m_Mem.Bank ((m_Mem.GetPort7FFD () & 0x04) | ((m_Mem.GetPort7FFD () & 0x08) ? 2 : 0) | 1));
		}
	}
	if (m_Div.IsActive ())
	{
		return m_Div.Fetch (address, m_Mem);
	}
	if (m_bSnet)
	{
		return SnetFetch (address);
	}
	return m_Mem.ReadPage (address >> 14)[address & 0x3FFF];
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
							      : m_Mem.ReadPage (nAddress >> 14)[nAddress & 0x3FFF];

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
	if (m_bPlus3Bus && (address & 0xE000) == 0x4000)
	{
		NoteBus (m_Mem.ReadPage (1)[address & 0x3FFF]);
	}
	if (address < 0x4000 && m_Div.IsPaged ())
	{
		return m_Div.Read (address);
	}
	if (address < 0x4000 && m_bSnet && m_Snet.IsPaged ())
	{
		return m_Snet.Read (address);
	}
	return m_Mem.ReadPage (address >> 14)[address & 0x3FFF];
}

void CSpectrum::poke8 (uint16_t address, uint8_t value)
{
	Contend (address, 3);
	WriteByte (address, value);
}

uint16_t CSpectrum::peek16 (uint16_t address)
{
	Contend (address, 3);
	uint8_t nLSB = (address < 0x4000 && m_Div.IsPaged ()) ? m_Div.Read (address)
		     : (address < 0x4000 && m_bSnet && m_Snet.IsPaged ()) ? m_Snet.Read (address)
							: m_Mem.ReadPage (address >> 14)[address & 0x3FFF];

	address++;
	Contend (address, 3);
	uint8_t nMSB = (address < 0x4000 && m_Div.IsPaged ()) ? m_Div.Read (address)
		     : (address < 0x4000 && m_bSnet && m_Snet.IsPaged ()) ? m_Snet.Read (address)
							: m_Mem.ReadPage (address >> 14)[address & 0x3FFF];

	if (m_bPlus3Bus)
	{
		if (((address - 1) & 0xE000) == 0x4000) NoteBus (nLSB);
		if ((address & 0xE000) == 0x4000) NoteBus (nMSB);
	}

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
	if (   m_Mem.IsContended (address >> 14)
	    && m_pTiming->CodeModel != CodeModelPlus3)
	{
		for (int32_t i = 0; i < wstates; i++)
		{
			m_nTstates += m_ULA.Delay (m_nTstates) + 1;
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
// Los de ULAplus, como los de la ULA (bit 0 = 0) aunque sean impares.
void CSpectrum::PreIO (uint16_t nPort)
{
	if (m_Mem.IsContendedIO (nPort >> 14))
	{
		m_nTstates += m_ULA.Delay (m_nTstates);
	}
	m_nTstates++;
}

void CSpectrum::PostIO (uint16_t nPort)
{
	if (m_bTKMicro)
	{
		m_nTstates += 3 + TKIOContention (nPort);	// ULA de Microdigital
		return;
	}

	if (m_pTiming->CodeModel == CodeModelPlus3)
	{
		m_nTstates += 3;
		return;
	}

	// Los puertos de ULAplus los atiende la propia ULA: tienen su contención
	// aunque sean impares (José Luis Sánchez, JSpeccy)
	boolean bULAPort = !(nPort & 0x0001) || m_ULA.IsULAplusPort (nPort);

	if (!bULAPort)
	{
		if (m_Mem.IsContendedIO (nPort >> 14))
		{
			for (unsigned i = 0; i < 3; i++)
			{
				m_nTstates += m_ULA.Delay (m_nTstates) + 1;
			}
		}
		else
		{
			m_nTstates += 3;
		}
	}
	else
	{
		m_nTstates += m_ULA.Delay (m_nTstates) + 3;
	}
}

// Contención de E/S de la ULA de Microdigital (TK90X/TK95 a 50 y 60 Hz), como
// ESPectrum (Ports.cpp, tkIOcon): en las 192 líneas del papel, si el puerto
// está en 0x4000-0x7FFF o es par, un retardo según el T-state de la línea.
static const u8 TKContention2[8] = {6, 6, 5, 4, 3, 2, 1, 0};
static const u8 TKContention3[129] = {
	6,  6,  12, 12, 11, 10, 9,  8,  7,  6,  12, 12, 11, 10, 9,  8,  7,  6,  12,
	12, 11, 10, 9,  8,  7,  6,  12, 12, 11, 10, 9,  8,  7,  6,  12, 12, 11, 10,
	9,  8,  7,  6,  12, 12, 11, 10, 9,  8,  7,  6,  12, 12, 11, 10, 9,  8,  7,
	6,  12, 12, 11, 10, 9,  8,  7,  6,  12, 12, 11, 10, 9,  8,  7,  6,  12, 12,
	11, 10, 9,  8,  7,  6,  12, 12, 11, 10, 9,  8,  7,  6,  12, 12, 11, 10, 9,
	8,  7,  6,  12, 12, 11, 10, 9,  8,  7,  6,  12, 12, 11, 10, 9,  8,  7,  6,
	12, 12, 11, 10, 9,  8,  7,  6,  6,  6,  5,  4,  3,  2,  1};

unsigned CSpectrum::TKIOContention (u16 nPort) const
{
	const TMachineTiming *T = m_pTiming;
	int nLine = m_nTstates / T->nTstatesLine - T->nUpBorderWidth;
	int nT = m_nTstates % T->nTstatesLine - (T->nPaperCol - 3);	// 93
	if (nT < 0)
	{
		nT += T->nTstatesLine;
		nLine--;
	}
	if (nLine < 0 || nLine >= 192)
	{
		return 0;
	}
	if ((nPort & 0xC000) == 0x4000)
	{
		return nT < 129 ? TKContention3[nT] : 0;
	}
	if (nPort & 0x0001)
	{
		return 0;
	}
	return nT < 128 ? TKContention2[nT & 7] : 0;
}

// +2A/+3: el último valor que pasó por el bus de datos. Lo dejan la Gate Array
// al leer la memoria de vídeo (bitmap y atributo, cuatro lecturas de cada
// ocho T-states en la zona de pantalla) y la CPU al leer o escribir entre
// 0x4000 y 0x5FFF; se queda con el más reciente. Fuera de esos momentos el
// valor se mantiene (no vuelve a 0xFF como en el 48K/128K).
u8 CSpectrum::Plus3FloatingBus (void)
{
	const TMachineTiming *T = m_pTiming;
	int nLine = (int) T->nTstatesLine;
	int nRow = (int) (m_nTstates / T->nTstatesLine) - (int) T->nUpBorderWidth;
	int nCol = (int) (m_nTstates % T->nTstatesLine) - (int) T->nOutOffset;
	s64 nFrame = (s64) m_nFrameStart;

	// última lectura de la Gate Array antes de ahora: fila y columna (0-123)
	if (nRow > 191 || (nRow == 191 && nCol > 123))
	{
		nRow = 191; nCol = 123;
	}
	else if (nRow < 0 || (nRow == 0 && nCol < 0))
	{
		nRow = 191; nCol = 123;			// la del cuadro anterior
		nFrame -= (s64) T->nTstatesFrame;
	}
	else if (nCol < 0)
	{
		nRow--; nCol = 123;
	}
	else if (nCol > 123)
	{
		nCol = 123;
	}
	else if (nCol % 8 > 3)
	{
		nCol = nCol - nCol % 8 + 3;
	}

	s64 nGA = nFrame + (s64) (nRow + (int) T->nUpBorderWidth) * nLine + (int) T->nOutOffset + nCol;
	if (m_bBusValid && (s64) m_nBusTstates >= nGA)
	{
		return m_nBusValue;			// la CPU tocó el bus después
	}

	return m_ULA.VideoFetch (nRow, nCol);
}

uint8_t CSpectrum::inPort (uint16_t port)
{
	PreIO (port);
	PostIO (port);

	if (IsMultifaceAvailable ())
	{
		u8 nMFValue;
		boolean bPaging = FALSE;
		boolean bValue = m_MF.In (port, &nMFValue, &bPaging);
		if (bPaging)
		{
			UpdatePaging ();
		}
		if (bValue)
		{
			return nMFValue;
		}
	}

	// Reloj de esxDOS (RTC.SYS de ZXMetalCircle)
	if (port == 0x7C3B && m_Div.IsActive ())
	{
		return m_Div.ReadRTC ();
	}

	// Spectranet: 0x003B, 0x013B, 0x023B y 0x033B (dirección completa)
	if (m_bSnet && m_Snet.IsPort (port))
	{
		return m_Snet.ReadPort (port, m_nPortFE,
					m_pTiming->CodeModel != CodeModel48K && (m_Mem.GetPort7FFD () & 0x08));
	}

	// divMMC: 0xEB igual que el 0x3F de la ZXMMC
	if (HasDivMMCPorts () && (port & 0xFF) == 0xEB)
	{
		return m_Div.ReadData ();
	}

	// ZXMMC: la lectura de 0x3F devuelve lo recibido en la transferencia
	// anterior y lanza otra con 0xFF (como el spi.v de zxp3)
	if (HasZXMMC () && (port & 0xFF) == 0x3F)
	{
		return m_ZXMMC.ReadData ();
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
		if (ModelHas (m_Model, MODEL_TK))
		{
			// TK: los bits 5 y 7 no están conectados y dan lo que haya en el
			// bus: el byte que lee la ULA de la memoria de vídeo, o 1 cuando
			// no lee (Cantinho do TK90X, "ROM e Porta 254"). En los TK90X
			// vendidos en Brasil el diodo D1 pone el bit 7 a 0, y la ROM v1
			// sale en portugués; sin él, en español.
			u8 nBus = m_ULA.FloatingBus (m_nTstates);
			nValue = (nValue & 0x5F) | (nBus & 0xA0);
			if (m_Model == ModelTK90X && m_bTKDiode)
			{
				nValue &= 0x7F;
			}
		}

		return nValue;
	}

	if (m_ULA.IsULAplusPort (port) && (port & 0x4000))	// datos (0xFF3B)
	{
		return m_ULA.ULAplusRead ();
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
		// +2A/+3: el bus flotante solo se ve en la familia del 0x0FFD
		// (A15-A12 = 0, A1 = 0) y sin bloqueo de paginación, con el bit 0
		// a 1; los demás puertos sin dueño dan 0xFF. Como el core zxp3 de
		// Kyp (zxp3.v).
		if ((port & 0xF002) == 0x0000 && !m_Mem.IsPagingLocked ())
		{
			return Plus3FloatingBus () | 0x01;
		}
		return 0xFF;
	}

	u8 nFloatBus = m_ULA.FloatingBus (m_nTstates);

	// Solo en el 128K (no +2/+2A/+3): leer 0x7FFD escribe el valor leído en el puerto
	if (   m_Model == Model128K
	    && (port & 0x8002) == 0
	    && !m_Mem.IsPagingLocked ())
	{
		if ((m_Mem.GetPort7FFD () ^ nFloatBus) & 0x08)
		{
			m_ULA.UpdateScreen (m_nTstates);
		}
		m_Mem.SetPort7FFD (nFloatBus);
		UpdatePaging ();
	}

	return nFloatBus;
}

void CSpectrum::outPort (uint16_t port, uint8_t value)
{
	PreIO (port);

	// LEC: en el 48K, cualquier puerto con A1 = 0 (tiene preferencia, como en
	// JSpeccy); en los 128K, solo el OUT (0xFD),A del CP/M (ver CLEC::IsPort)
	if (m_LEC.IsActive () && m_LEC.IsPort (port, value))
	{
		m_LEC.Write (value);
		UpdatePaging ();
		PostIO (port);
		return;
	}

	if (IsMultifaceAvailable ())
	{
		m_MF.Out (port, m_CPU.getRegPC ());
	}

	if (m_bSnet && m_Snet.IsPort (port))
	{
		m_Snet.WritePort (port, value);
	}

	if (port == 0x7C3B && m_Div.IsActive ())		// reloj de esxDOS: se fija la hora
	{
		m_Div.LatchRTC ();
	}

	// divMMC: 0xE3 control, 0xE7 selección de la tarjeta, 0xEB datos SPI
	if (HasDivMMCPorts ())
	{
		m_Div.WritePort (port & 0xFF, value);
	}

	// ZXMMC: 0x1F selección de la tarjeta, 0x3F datos SPI
	if (HasZXMMC ())
	{
		m_ZXMMC.WritePort (port & 0xFF, value);
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
			m_ULA.UpdateBorder (m_nTstates, m_nPortFE);
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

	// ULAplus: 0xBF3B (registro) y 0xFF3B (datos), con sus espejos (ver
	// IsULAplusPort). Antes respondía a cualquier puerto con A2 = 0 (como
	// JSpeccy), también a los de la divMMC (0xE3, 0xEB) y a los del reloj de
	// esxDOS (0x7C3B, 0x7D3B), que dejaban ULAplus encendido al salir de la NMI
	if (m_ULA.IsULAplusPort (port))
	{
		m_ULA.ULAplusWrite (port, value, m_nTstates, m_nPortFE);
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

	if (m_Mem.IsPagingLocked ())		// paginación bloqueada
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
			m_Mem.SetPort1FFD (value);
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
		if ((m_Mem.GetPort7FFD () ^ value) & 0x08)
		{
			m_ULA.UpdateScreen (m_nTstates);
		}

		m_Mem.SetPort7FFD (value);
		UpdatePaging ();
	}
}

////////////////////////////////////////////////////////////////////////////////
// Cuadro

void CSpectrum::RunFrame (void)
{
	const int nTstatesFrame = m_pTiming->nTstatesFrame;

	m_ULA.StartFrame ();
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

	m_ULA.EndFrame (m_nPortFE);

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
	if (m_LEC.IsPaged ())
	{
		return opcode;		// con la LEC paginada no hay ROM: ningún trap
	}

	// Interface 1: entra al leer 0x0008 (errores/extensiones) o 0x1708 (CLOSE#),
	// sale al leer 0x0700
	if (IF1Active ())
	{
		if ((address == 0x0008 || address == 0x1708) && !m_bIF1Paged && !m_MF.IsPaged ())
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
	if (address == 0x0066 && !m_MF.IsPaged () && IsMultifaceAvailable ())
	{
		PageMultiface ();
		if (m_MF.IsPaged ())
		{
			m_MF.Unlock ();
			return m_MF.Read (0x0066);
		}
	}

	// SA-BYTES de la ROM de BASIC (en 0x04D0: el +3 no entra por 0x04C2).
	// En ese punto A = flag, IX = dirección, DE = longitud, y en la pila está
	// la dirección de SA/LD-RET, así que basta con un RET.
	if (   address == 0x04D0
	    && m_pSaveHandler != 0
	    && !m_bRecording
	    && m_Mem.ReadPage (0) == m_Mem.GetBasicROM ()
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
	    && m_Mem.ReadPage (0) == m_Mem.GetBasicROM ()
	    && m_pTape->IsInserted ()
	    && !m_pTape->IsPlaying ()
	    && FlashLoad ())
	{
		// La divMMC pagina su memoria al leer la instrucción de 0x0562 (esxDOS
		// lo usa para sus cintas). Como la rutina de la ROM ya no se ejecuta,
		// se anula: si no, la vuelta por SA/LD-RET ejecutaba código de esxDOS
		// en lugar de la ROM y el programa se perdía tras el primer bloque.
		m_Div.CancelLoadAutomap ();
		return 0xC9;	// RET (ver FlashLoad)
	}
	m_Div.ClearLoadAutomap ();

	// LD-BYTES de la ROM de BASIC
	if (   address == 0x0556
	    && m_Mem.ReadPage (0) == m_Mem.GetBasicROM ()
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
		u8 nByte = m_Mem.ReadPage (nAddr >> 14)[nAddr & 0x3FFF];
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
	if (nAddress < 0x4000 && m_Div.IsPaged ())
	{
		return m_Div.Read (nAddress);
	}
	return m_Mem.ReadPage (nAddress >> 14)[nAddress & 0x3FFF];
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

// Disco acelerado (diskaccel=1): mientras el programa usa la disquetera, el
// kernel emula sin esperar al reloj, como con la cinta en carga rápida. El
// disco y el controlador siguen en tiempo del Spectrum, así que no cambia
// nada para las protecciones; solo pasa más deprisa en la realidad.
boolean CSpectrum::IsDiskAccelerating (void) const
{
	return m_bDiskAccel && HasDiskDrive ()
	       && m_pDisk->RecentlyUsed (m_nFrameStart + m_nTstates, m_pTiming->nClockFreq);
}
