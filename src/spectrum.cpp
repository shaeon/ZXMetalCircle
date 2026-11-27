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

// Los ficheros del Spectrum (ver filetypes.h). Los .P los carga el ZX81 de
// Paul Farrow, que va como cartucho del Interface 2.
static const TFileType SpectrumFileTypes[] =
{
	{".tap", FileTape},
	{".tzx", FileTape},
	{".pzx", FileTape},
	{".csw", FileTape},
	{".z80", FileSnapshot},
	{".sna", FileSnapshot},
	{".szx", FileSnapshot},
	{".sp",  FileSnapshot},		// snapshots de Pedro Gimeno
	{".rom", FileCartridge},	// Interface 2
	{".mdr", FileMicrodrive},
	{".mdv", FileMicrodrive},
	{".dsk", FileDisk},		// +3
	{".vhd", FileHardDisk},		// +3e y divMMC
	{".img", FileHardDisk},
	{".raw", FileHardDisk},
	{".hdf", FileHardDisk},
	{".p",   FileProgramZX81},
	{".pok", FilePokes},
	{0,      FileUnknown}
};

const TFileType *CSpectrum::GetFileTypes (void) const
{
	return SpectrumFileTypes;
}

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
	m_Deck (pTape, m_CPU, m_Mem, m_Div, TapeWriteByte, this),
	m_bDiskAccel (FALSE),
	m_bTKDiode (FALSE),
	m_nTKULA (TKULA50Hz),
	m_bTKMicro (FALSE)
{
	m_nResetCount = 0;
	m_bIssue2 = FALSE;
	m_pDisk = new CPlus3Disk;
	m_bNMIPending = FALSE;
	m_FallbackFrom = ModelCount;
	for (unsigned i = 0; i < ModelCount; i++)
	{
		m_pModelROM[i] = 0;
	}

	m_bSnowEnabled = TRUE;
	m_bSnow = m_bSnow128 = FALSE;

	m_CPU.setBreakpoint (true);

	m_Audio.SetTape (m_pTape);

	SetModel (Model48K);
}

CSpectrum::~CSpectrum (void)
{
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
	m_IF1ROM.Reset ();
	m_IF1.Reset ();
	m_LEC.Reset ();
	m_pDisk->Reset ();
	m_SDCard.Select (FALSE);
	m_Div.Reset ();			// CONMEM y MAPRAM se borran con el reset
	m_Snet.MachineReset ();
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
	m_IF1ROM.Map (m_Mem);

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
	return m_MF.IsReady () && !m_Div.IsActive () && !m_Snet.IsEnabled ();
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
	if (!m_IF1ROM.HasROM () || m_Div.IsActive () || m_Snet.IsEnabled ())
	{
		bConnected = FALSE;		// sin roms/if1.rom, o con la divMMC (puertos 0xE7/0xEB), no hay IF1
	}
	if (m_IF1ROM.SetConnected (bConnected))
	{
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

	if (nAddress < 0x4000 && m_Snet.IsEnabled () && m_Snet.IsPaged ())
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

// La carga instantánea escribe como el Z80 (la imagen, la divMMC...)
void CSpectrum::TapeWriteByte (void *pParam, u16 nAddress, u8 nValue)
{
	static_cast<CSpectrum *> (pParam)->WriteByte (nAddress, nValue);
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
	if (m_Snet.IsEnabled ())
	{
		boolean bNMI;
		u8 nOpcode = m_Snet.Fetch (address, m_Mem, &bNMI);
		if (bNMI)
		{
			m_CPU.triggerNMI ();
		}
		return nOpcode;
	}
	return m_Mem.ReadPage (address >> 14)[address & 0x3FFF];
}

void CSpectrum::SetSpectranetEnabled (boolean bOn)
{
	m_Snet.SetEnabled (bOn);
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
	if (address < 0x4000 && m_Snet.IsEnabled () && m_Snet.IsPaged ())
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
		     : (address < 0x4000 && m_Snet.IsEnabled () && m_Snet.IsPaged ()) ? m_Snet.Read (address)
							: m_Mem.ReadPage (address >> 14)[address & 0x3FFF];

	address++;
	Contend (address, 3);
	uint8_t nMSB = (address < 0x4000 && m_Div.IsPaged ()) ? m_Div.Read (address)
		     : (address < 0x4000 && m_Snet.IsEnabled () && m_Snet.IsPaged ()) ? m_Snet.Read (address)
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
	if (m_Snet.IsEnabled () && m_Snet.IsPort (port))
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
		m_Deck.CountLoaderRead (port);

		boolean bEar = m_bEarOut;
		if (m_pTape->IsPlaying ())
		{
			// el audio avanza primero, para que oiga la cinta en el momento justo
			m_Audio.AdvanceTo (m_nTstates);
			bEar = m_Deck.ReadEar (GetAbsoluteTstates ());
		}
		else if (m_Deck.IsEarInputActive ())
		{
			// casete real: el nivel de su señal en este instante
			m_Audio.AdvanceTo (m_nTstates);
			bEar = m_Deck.ReadEarInput (m_nTstates);
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

	if (m_Snet.IsEnabled () && m_Snet.IsPort (port))
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

		if ((m_nPortFE ^ value) & 0x08)
		{
			m_Deck.MICChanged (GetAbsoluteTstates ());	// para la grabación por pulsos
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
			if (m_Snet.IsEnabled ())
			{
				m_Snet.TakeNMI ();		// la NMI la atiende la Spectranet (su menú)
			}
			m_CPU.triggerNMI ();
		}
	}

	m_ULA.EndFrame (m_nPortFE);

	// grabación de la entrada EAR externa: sus cambios de este cuadro
	m_Deck.RecordEarInput (m_nFrameStart);

	m_Audio.EndFrame (nTstatesFrame);
	if (HasDiskDrive ())
	{
		m_pDisk->Sync (m_nFrameStart + nTstatesFrame, m_pTiming->nClockFreq);
	}

	m_nTstates -= nTstatesFrame;
	m_nFrameStart += nTstatesFrame;
	m_Deck.EndFrame (GetAbsoluteTstates ());
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
		if ((address == 0x0008 || address == 0x1708) && !m_IF1ROM.IsPaged () && !m_MF.IsPaged ())
		{
			m_IF1ROM.Page ();
			UpdatePaging ();
			return m_IF1ROM.Read (address);
		}

		if (address == 0x0700 && m_IF1ROM.IsPaged ())
		{
			m_IF1ROM.Unpage ();
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
	    && m_Mem.ReadPage (0) == m_Mem.GetBasicROM ()
	    && m_Deck.SaveTrap ())
	{
		return 0xC9;	// RET
	}

	// Carga instantánea, como Fuse: en 0x0562 (el IN A,(FE) de LD-BYTES). A
	// esas alturas la ROM ya ha guardado el flag y LOAD/VERIFY en AF', ha
	// hecho DI, ha puesto el borde y ha apilado la vuelta por SA/LD-RET.
	// También coge a los cargadores que llaman directamente a 0x0562 para
	// usar otro flag con la rutina de la ROM (Cabal, de Ocean).
	if (   address == 0x0562
	    && m_Mem.ReadPage (0) == m_Mem.GetBasicROM ()
	    && m_Deck.LoadTrap ())
	{
		// La divMMC pagina su memoria al leer la instrucción de 0x0562 (esxDOS
		// lo usa para sus cintas). Como la rutina de la ROM ya no se ejecuta,
		// se anula: si no, la vuelta por SA/LD-RET ejecutaba código de esxDOS
		// en lugar de la ROM y el programa se perdía tras el primer bloque.
		m_Div.CancelLoadAutomap ();
		return 0xC9;	// RET (ver FlashLoad)
	}
	m_Div.ClearLoadAutomap ();

	// LD-BYTES de la ROM de BASIC: arranca la cinta si hace falta
	if (   address == 0x0556
	    && m_Mem.ReadPage (0) == m_Mem.GetBasicROM ())
	{
		m_Deck.LoadBytes (GetAbsoluteTstates ());
	}

	return opcode;
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
