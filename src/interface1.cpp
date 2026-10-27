//
// interface1.cpp
//
// Portado a C++ desde JSpeccy (machine/Interface1.java, utilities/Microdrive.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "interface1.h"
#include "inflate.h"
#include <circle/util.h>

#define RAW_SECTOR_SIZE		792	// cartucho en blanco (valor empírico de JSpeccy)
#define MDR_SECTOR_SIZE		587	// 543 + 2 * preámbulo + 2 * 10 de hueco
#define MDR_GAP_LENGTH		10

#define CLOCK_DATA		0
#define CLOCK_GAP		4
#define DATA_FILLER		0x5A

#define STATUS_GAP		0x04
#define STATUS_SYNC		0x02
#define STATUS_WRITE_PROT_MASK	0xFE

#define CTRL_OUT_COMMSDATA	0x01
#define CTRL_OUT_COMMSCLK	0x02

static const u8 Preamble[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};

////////////////////////////////////////////////////////////////////////////////
// Microdrive

CMicrodrive::CMicrodrive (void)
:	m_pData (0),
	m_pGap (0),
	m_nLength (0),
	m_nPos (0),
	m_nLastData (0),
	m_bModified (FALSE),
	m_bWriteProtected (TRUE),
	m_bWrGapPending (FALSE),
	m_nStartGap (0)
{
}

CMicrodrive::~CMicrodrive (void)
{
	Eject ();
}

boolean CMicrodrive::Allocate (unsigned nLength)
{
	Eject ();

	m_pData = new u8[nLength];
	m_pGap = new u8[nLength];
	if (m_pData == 0 || m_pGap == 0)
	{
		Eject ();
		return FALSE;
	}

	m_nLength = nLength;
	m_nPos = 0;
	m_nLastData = 0;
	m_bWrGapPending = FALSE;

	return TRUE;
}

void CMicrodrive::Eject (void)
{
	delete [] m_pData;
	delete [] m_pGap;
	m_pData = 0;
	m_pGap = 0;
	m_nLength = 0;
	m_nPos = 0;
	m_bModified = FALSE;
	m_bWriteProtected = TRUE;
}

boolean CMicrodrive::InsertBlank (unsigned nSectors)
{
	if (nSectors < 10 || nSectors > MDR_MAX_SECTORS)
	{
		nSectors = 180;
	}

	if (!Allocate (nSectors * RAW_SECTOR_SIZE))
	{
		return FALSE;
	}

	memset (m_pData, DATA_FILLER, m_nLength);
	memset (m_pGap, CLOCK_GAP, m_nLength);
	m_bWriteProtected = FALSE;
	m_bModified = FALSE;

	return TRUE;
}

boolean CMicrodrive::InsertMDR (const u8 *pData, unsigned nSize)
{
	unsigned nSectors = (nSize - 1) / (MDR_HEADER_SIZE + MDR_RECORD_SIZE);
	if (nSize < 1 + MDR_HEADER_SIZE + MDR_RECORD_SIZE || nSectors > MDR_MAX_SECTORS)
	{
		return FALSE;
	}

	if (!Allocate (nSectors * MDR_SECTOR_SIZE))
	{
		return FALSE;
	}

	// hueco, preámbulo, cabecera, hueco, preámbulo, registro
	unsigned nPos = 0;
	const u8 *pIn = pData;
	for (unsigned nSector = 0; nSector < nSectors; nSector++)
	{
		for (unsigned nPart = 0; nPart < 2; nPart++)
		{
			for (unsigned i = 0; i < MDR_GAP_LENGTH; i++, nPos++)
			{
				m_pData[nPos] = DATA_FILLER;
				m_pGap[nPos] = CLOCK_GAP;
			}
			for (unsigned i = 0; i < sizeof Preamble; i++, nPos++)
			{
				m_pData[nPos] = Preamble[i];
				m_pGap[nPos] = CLOCK_DATA;
			}

			unsigned nBytes = nPart == 0 ? MDR_HEADER_SIZE : MDR_RECORD_SIZE;
			for (unsigned i = 0; i < nBytes; i++, nPos++)
			{
				m_pData[nPos] = *pIn++;
				m_pGap[nPos] = CLOCK_DATA;
			}
		}
	}

	// el último byte del .MDR indica la protección contra escritura
	m_bWriteProtected = pData[nSize - 1] != 0;
	m_bModified = FALSE;

	return TRUE;
}

boolean CMicrodrive::IsPreamble (unsigned nPos) const
{
	unsigned nPrev = nPos == 0 ? m_nLength - 1 : nPos - 1;
	if (m_pGap[nPrev] == CLOCK_DATA)
	{
		return FALSE;		// un preámbulo siempre va detrás de un hueco
	}

	for (unsigned i = 0; i < sizeof Preamble; i++)
	{
		unsigned k = (nPos + i) % m_nLength;
		if (m_pGap[k] != CLOCK_DATA || m_pData[k] != Preamble[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}

unsigned CMicrodrive::ExportMDR (u8 *pBuffer, unsigned nBufferSize) const
{
	if (m_pData == 0 || nBufferSize < MDR_FILE_SIZE)
	{
		return 0;
	}

	// Se empieza justo después de un hueco, para no partir un sector
	unsigned nStart = 0;
	for (unsigned i = 0; i < m_nLength; i++)
	{
		if (IsPreamble (i))
		{
			nStart = i;
			break;
		}
	}

	unsigned nSectors = 0;
	unsigned nScanned = 0;
	unsigned k = nStart;
	while (nScanned < m_nLength && nSectors < MDR_MAX_SECTORS)
	{
		if (!IsPreamble (k))
		{
			k = (k + 1) % m_nLength;
			nScanned++;
			continue;
		}

		unsigned nHeader = (k + sizeof Preamble) % m_nLength;
		if ((m_pData[nHeader] & 0x01) == 0)	// no es una cabecera
		{
			k = (k + 1) % m_nLength;
			nScanned++;
			continue;
		}

		u8 *pOut = pBuffer + nSectors * (MDR_HEADER_SIZE + MDR_RECORD_SIZE);
		for (unsigned i = 0; i < MDR_HEADER_SIZE; i++)
		{
			pOut[i] = m_pData[(nHeader + i) % m_nLength];
		}
		memset (pOut + MDR_HEADER_SIZE, 0, MDR_RECORD_SIZE);

		// el registro de datos viene después del siguiente hueco
		unsigned nAfter = (nHeader + MDR_HEADER_SIZE) % m_nLength;
		unsigned nStep = MDR_HEADER_SIZE + sizeof Preamble;
		for (unsigned j = 0; j < 64; j++)
		{
			unsigned r = (nAfter + j) % m_nLength;
			if (IsPreamble (r))
			{
				unsigned nRecord = (r + sizeof Preamble) % m_nLength;
				if ((m_pData[nRecord] & 0x01) == 0)
				{
					for (unsigned i = 0; i < MDR_RECORD_SIZE; i++)
					{
						pOut[MDR_HEADER_SIZE + i] = m_pData[(nRecord + i) % m_nLength];
					}
					nStep += j + sizeof Preamble + MDR_RECORD_SIZE;
				}
				break;
			}
		}

		nSectors++;
		k = (k + nStep) % m_nLength;
		nScanned += nStep;
	}

	if (nSectors == 0)
	{
		return 0;
	}

	unsigned nSize = nSectors * (MDR_HEADER_SIZE + MDR_RECORD_SIZE);
	pBuffer[nSize++] = m_bWriteProtected ? 1 : 0;

	return nSize;
}

unsigned CMicrodrive::CountSectors (void) const
{
	u8 *pBuffer = new u8[MDR_FILE_SIZE];
	if (pBuffer == 0)
	{
		return 0;
	}
	unsigned nSize = ExportMDR (pBuffer, MDR_FILE_SIZE);
	delete [] pBuffer;
	return nSize > 0 ? (nSize - 1) / (MDR_HEADER_SIZE + MDR_RECORD_SIZE) : 0;
}

// Compara la secuencia de bloques de datos (lo que hay entre huecos) de la cinta
// con la que resulta de guardarla en .MDR y volver a cargarla. Si coinciden, el
// .MDR conserva todo; si no (como con el CP/M de LEC, que usa otra estructura de
// sectores), hay que guardar la cinta en bruto (.MDV).
boolean CMicrodrive::IsSinclairFormat (void) const
{
	if (m_pData == 0)
	{
		return FALSE;
	}

	u8 *pMDR = new u8[MDR_FILE_SIZE];
	if (pMDR == 0)
	{
		return FALSE;
	}
	unsigned nSize = ExportMDR (pMDR, MDR_FILE_SIZE);

	CMicrodrive Copy;
	boolean bOK = nSize > 0 && Copy.InsertMDR (pMDR, nSize);
	delete [] pMDR;
	if (!bOK)
	{
		return FALSE;
	}

	// primer bloque de cada cinta: el primer preámbulo
	unsigned nA = 0, nB = 0;
	while (nA < m_nLength && !IsPreamble (nA))	nA++;
	while (nB < Copy.m_nLength && !Copy.IsPreamble (nB))	nB++;
	if (nA >= m_nLength || nB >= Copy.m_nLength)
	{
		return FALSE;
	}

	// recorre las dos cintas bloque a bloque
	unsigned nScannedA = 0, nScannedB = 0;
	while (nScannedA < m_nLength)
	{
		// salta huecos
		while (m_pGap[nA] != CLOCK_DATA && nScannedA < m_nLength)
		{
			nA = (nA + 1) % m_nLength;
			nScannedA++;
		}
		while (Copy.m_pGap[nB] != CLOCK_DATA && nScannedB < Copy.m_nLength)
		{
			nB = (nB + 1) % Copy.m_nLength;
			nScannedB++;
		}
		if (nScannedA >= m_nLength)
		{
			break;
		}
		if (nScannedB >= Copy.m_nLength)
		{
			return FALSE;		// la copia tiene menos bloques
		}

		// compara el bloque de datos
		while (m_pGap[nA] == CLOCK_DATA && nScannedA < m_nLength)
		{
			if (   Copy.m_pGap[nB] != CLOCK_DATA
			    || Copy.m_pData[nB] != m_pData[nA])
			{
				return FALSE;
			}
			nA = (nA + 1) % m_nLength;
			nB = (nB + 1) % Copy.m_nLength;
			nScannedA++;
			nScannedB++;
		}
		if (Copy.m_pGap[nB] == CLOCK_DATA)
		{
			return FALSE;		// el bloque de la copia es más largo
		}
	}

	return TRUE;
}

// Cabecera MDVT: "MDVT", longitud (12), versión 1.0, flags, tamaño de sector,
// número de sectores, longitud de hueco, primer estado, número de tramos de huecos
#define MDVT_WR_PROT		0x01
#define MDVT_COMPRESSED		0x02
#define MDVT_MDR_CONVERTED	0x04
#define MDVT_UNFORMATTED	0x08

unsigned CMicrodrive::GetMDVSize (void) const
{
	// cabecera + textos + datos + tramos (como mucho, 2 por byte y los partidos)
	return 8 + 12 + 3 + 16 + m_nLength + 4 * m_nLength + 64;
}

unsigned CMicrodrive::ExportMDV (u8 *pBuffer, unsigned nBufferSize) const
{
	if (m_pData == 0 || nBufferSize < GetMDVSize ())
	{
		return 0;
	}

	u8 *p = pBuffer;
	memcpy (p, "MDVT", 4);
	p[4] = 12; p[5] = 0; p[6] = 0; p[7] = 0;
	p += 8;

	u8 *pHeader = p;
	memset (pHeader, 0, 12);
	pHeader[0] = 1;						// versión 1.0
	pHeader[2] = m_bWriteProtected ? MDVT_WR_PROT : 0;	// sin comprimir
	pHeader[4] = m_nLength & 0xFF;				// "sector" = toda la cinta
	pHeader[5] = (m_nLength >> 8) & 0xFF;
	pHeader[6] = 1;						// un único "sector"
	pHeader[9] = m_pGap[0] == CLOCK_DATA ? 0 : CLOCK_GAP;
	p += 12;

	// Tamaño de sector como JSpeccy: 792 en los cartuchos en bruto y 587 en
	// los que vienen de un .MDR (con el flag y la longitud de hueco, 10). Al
	// grabar, JSpeccy escribe siempre ese tamaño con el número de sectores
	// que leyó: con otro tamaño, la cabecera dejaría de cuadrar con los datos.
	// Si la cinta no es múltiplo de ninguno, se trocea (16 bits por sector).
	unsigned nSectorSize, nSectors;
	if (m_nLength % RAW_SECTOR_SIZE == 0 && m_nLength / RAW_SECTOR_SIZE <= 0xFFFF)
	{
		nSectorSize = RAW_SECTOR_SIZE;
	}
	else if (m_nLength % MDR_SECTOR_SIZE == 0 && m_nLength / MDR_SECTOR_SIZE <= 0xFFFF)
	{
		nSectorSize = MDR_SECTOR_SIZE;
		pHeader[2] |= MDVT_MDR_CONVERTED;
		pHeader[8] = 10;			// longitud de hueco de los .MDR convertidos
	}
	else
	{
		nSectors = 1;
		while (m_nLength / nSectors > 0xFFFF || m_nLength % nSectors != 0)
		{
			nSectors++;
		}
		nSectorSize = m_nLength / nSectors;
	}
	nSectors = m_nLength / nSectorSize;
	pHeader[4] = nSectorSize & 0xFF;
	pHeader[5] = nSectorSize >> 8;
	pHeader[6] = nSectors & 0xFF;
	pHeader[7] = nSectors >> 8;

	// textos: creador y dos vacíos
	static const char Creator[] = "ZXCircle";
	*p++ = sizeof Creator - 1;
	memcpy (p, Creator, sizeof Creator - 1);
	p += sizeof Creator - 1;
	*p++ = 0;
	*p++ = 0;

	memcpy (p, m_pData, m_nLength);
	p += m_nLength;

	// tramos alternos de dato/hueco (16 bits; los largos se parten con tramos de 0)
	unsigned nEntries = 0;
	u8 nState = m_pGap[0] == CLOCK_DATA ? 0 : 1;
	unsigned nRun = 0;
	for (unsigned i = 0; i <= m_nLength; i++)
	{
		u8 nNow = i < m_nLength ? (m_pGap[i] == CLOCK_DATA ? 0 : 1) : 2;
		if (nNow == nState && nRun < 0xFFFF)
		{
			nRun++;
			continue;
		}

		*p++ = nRun & 0xFF;
		*p++ = nRun >> 8;
		nEntries++;

		if (nNow == nState)		// tramo partido: uno vacío del otro estado
		{
			*p++ = 0;
			*p++ = 0;
			nEntries++;
			nRun = 1;
			continue;
		}

		nState = nNow;
		nRun = 1;
	}

	pHeader[10] = nEntries & 0xFF;
	pHeader[11] = nEntries >> 8;

	return p - pBuffer;
}

boolean CMicrodrive::InsertMDV (const u8 *pData, unsigned nSize)
{
	if (nSize < 8 || memcmp (pData, "MDVT", 4) != 0)
	{
		return FALSE;
	}

	unsigned nHeaderSize = pData[4] | (pData[5] << 8) | (pData[6] << 16) | ((u32) pData[7] << 24);
	if (nHeaderSize < 12 || 8 + nHeaderSize > nSize)
	{
		return FALSE;
	}

	const u8 *pHeader = pData + 8;
	unsigned nFlags = pHeader[2] | (pHeader[3] << 8);
	unsigned nSectorSize = pHeader[4] | (pHeader[5] << 8);
	unsigned nSectors = pHeader[6] | (pHeader[7] << 8);
	u8 nFirstGap = pHeader[9];
	unsigned nEntries = pHeader[10] | (pHeader[11] << 8);
	unsigned nLength = nSectorSize * nSectors;

	if (nLength == 0 || nLength > 4 * 1024 * 1024)
	{
		return FALSE;
	}

	// tres textos de longitud variable
	unsigned nPos = 8 + nHeaderSize;
	for (unsigned i = 0; i < 3; i++)
	{
		if (nPos >= nSize)
		{
			return FALSE;
		}
		nPos += 1 + pData[nPos];
	}

	if (!Allocate (nLength))
	{
		return FALSE;
	}

	m_bWriteProtected = (nFlags & MDVT_WR_PROT) != 0;
	m_bModified = FALSE;

	if (nFlags & MDVT_UNFORMATTED)
	{
		memset (m_pData, DATA_FILLER, m_nLength);
		memset (m_pGap, CLOCK_GAP, m_nLength);
		return TRUE;
	}

	unsigned nRawSize = nLength + 2 * nEntries;
	const u8 *pRaw;
	u8 *pInflated = 0;

	if (nFlags & MDVT_COMPRESSED)
	{
		pInflated = new u8[nRawSize];
		if (   pInflated == 0
		    || ZlibInflate (pData + nPos, nSize - nPos, pInflated, nRawSize) != (int) nRawSize)
		{
			delete [] pInflated;
			Eject ();
			return FALSE;
		}
		pRaw = pInflated;
	}
	else
	{
		if (nPos + nRawSize > nSize)
		{
			Eject ();
			return FALSE;
		}
		pRaw = pData + nPos;
	}

	memcpy (m_pData, pRaw, nLength);

	// reconstruye los huecos a partir de los tramos
	u8 nState = nFirstGap == 0 ? CLOCK_DATA : CLOCK_GAP;
	unsigned nIndex = 0;
	const u8 *pRuns = pRaw + nLength;
	for (unsigned i = 0; i < nEntries; i++)
	{
		unsigned nRun = pRuns[2 * i] | (pRuns[2 * i + 1] << 8);
		while (nRun-- > 0 && nIndex < m_nLength)
		{
			m_pGap[nIndex++] = nState;
		}
		nState = nState == CLOCK_DATA ? CLOCK_GAP : CLOCK_DATA;
	}

	delete [] pInflated;

	if (nIndex != m_nLength)
	{
		Eject ();
		return FALSE;
	}

	return TRUE;
}

void CMicrodrive::Selected (void)
{
	if (m_pData == 0)
	{
		return;
	}

	m_bWrGapPending = FALSE;

	// avanza hasta el final del bloque de datos en curso
	unsigned nStop = m_nPos == 0 ? m_nLength - 1 : m_nPos - 1;
	while (m_pGap[m_nPos] == CLOCK_DATA && m_nPos != nStop)
	{
		Advance ();
	}
	Advance ();
}

u8 CMicrodrive::ReadStatus (void)
{
	if (m_pData == 0)
	{
		return 0xF5;
	}

	if (m_bWrGapPending)
	{
		return 0xFB;
	}

	u8 nStatus = 0xF9;		// sin hueco ni sincronismo
	if (m_pGap[m_nPos] != CLOCK_DATA)
	{
		nStatus |= STATUS_GAP;
	}
	if (m_pGap[m_nPos] == CLOCK_DATA && (m_nLastData & m_pData[m_nPos]) == 0)
	{
		nStatus |= STATUS_SYNC;
	}

	m_nLastData = m_pData[m_nPos];
	Advance ();

	if (m_bWriteProtected)
	{
		nStatus &= STATUS_WRITE_PROT_MASK;
	}

	return nStatus;
}

u8 CMicrodrive::ReadData (void)
{
	if (m_pData == 0)
	{
		return 0xFF;
	}

	m_nLastData = m_pData[m_nPos];
	Advance ();

	return m_nLastData;
}

void CMicrodrive::WriteControl (u8 nValue, u64 nNow)
{
	if (m_pData == 0)
	{
		return;
	}

	u8 nOp = nValue & 0x0C;
	if (nOp == 0x04 && !m_bWrGapPending)
	{
		m_nStartGap = nNow;
		m_bWrGapPending = TRUE;
		return;
	}

	if ((nOp == 0x00 || nOp == 0x0C) && m_bWrGapPending)
	{
		// se escribe un hueco tan largo como el tiempo transcurrido
		u64 nGap = (nNow - m_nStartGap) / MDR_BYTE_TSTATES;
		if (nGap > m_nLength)
		{
			nGap = m_nLength;
		}
		while (nGap-- > 0)
		{
			m_pData[m_nPos] = DATA_FILLER;
			m_pGap[m_nPos] = CLOCK_GAP;
			Advance ();
		}
		m_bWrGapPending = FALSE;
	}
}

void CMicrodrive::WriteData (u8 nValue)
{
	if (m_pData == 0 || m_bWriteProtected)
	{
		return;
	}

	m_pGap[m_nPos] = CLOCK_DATA;
	m_pData[m_nPos] = nValue;
	m_bModified = TRUE;
	Advance ();
}

////////////////////////////////////////////////////////////////////////////////
// Interface 1

CInterface1::CInterface1 (void)
:	m_nFlipFlop (0),
	m_nSelected (0),
	m_bCommsClk (FALSE)
{
	Reset ();
}

void CInterface1::Reset (void)
{
	m_nFlipFlop = 0;
	m_bCommsClk = FALSE;
	m_bCommsData = FALSE;
	m_bCTS = FALSE;
	m_nCountOut = 0;
	m_nDataOut = 0;
	m_nCountIn = 0;
	m_nDataIn = 0;
	m_bTX = FALSE;
	m_ZXNet.Reset ();
}

// Puerto 0xF7 (lectura): bit 7 = TX DATA del RS232, bit 0 = cable de red
u8 CInterface1::ReadLanPort (u64 nNow)
{
	u8 nRet = 0xFF;

	// RS232: con CTS, el byte pendiente se manda bit a bit (como en Fuse:
	// 1 lectura en reposo, 4 de bit de inicio y 8 de datos invertidos)
	if (m_bCTS)
	{
		if (m_nCountIn == 0)
		{
			if (m_RS232In.Get (&m_nDataIn))
			{
				m_nCountIn++;
			}
			m_bTX = FALSE;
		}
		else if (m_nCountIn < 5)
		{
			m_bTX = TRUE;			// bit de inicio
			m_nCountIn++;
		}
		else if (m_nCountIn < 13)
		{
			m_bTX = (m_nDataIn & 0x01) == 0;
			m_nDataIn >>= 1;
			m_nCountIn++;
		}
		else
		{
			m_nCountIn = 0;
		}
	}
	else
	{
		m_nCountIn = 0;
		m_bTX = FALSE;
	}

	// Red (ZX Net): con CTS inactivo (bit 4 de 0xEF a 1), el bit 0 es el cable
	boolean bNet = !m_bCTS && m_ZXNet.PollState (nNow);

	if (!m_bTX)
	{
		nRet &= 0x7F;
	}
	if (!bNet)
	{
		nRet &= 0xFE;
	}
	return nRet;
}

// Puerto 0xF7 (escritura): bit 0 = RX DATA (RS232) o cable de red
void CInterface1::WriteLanPort (u8 nValue, u64 nNow)
{
	unsigned nBit = nValue & 0x01;

	if (m_bCommsData)		// RS232
	{
		if (m_nCountOut == 0 && !nBit)
		{
			m_nCountOut++;			// esperando el flanco ~~\__
		}
		else if (m_nCountOut == 1)
		{
			if (m_bCTS || !nBit)
			{
				m_nCountOut = -1;
			}
			else
			{
				m_nCountOut++;		// bit de inicio __/~~
			}
		}
		else if (m_nCountOut >= 2 && m_nCountOut <= 9)
		{
			m_nDataOut >>= 1;
			m_nDataOut |= nBit ? 0 : 0x80;
			m_nCountOut++;
		}
		else if (m_nCountOut >= 10 && m_nCountOut <= 11)
		{
			m_nCountOut = nBit ? -1 : m_nCountOut + 1;
		}
		else if (m_nCountOut == 12)
		{
			m_nCountOut = !nBit ? -1 : m_nCountOut + 1;
		}
		else if (m_nCountOut == 13)
		{
			if (nBit)
			{
				m_nCountOut = -1;
			}
		}

		if (m_nCountOut == -1)
		{
			m_nCountOut = 13;
			m_nDataOut = '?';		// error de trama
		}
		if (m_nCountOut == 13)
		{
			m_RS232Out.Put (m_nDataOut);
			m_nCountOut = 0;
		}
	}
	else				// red: el cable va invertido (bit 0 a 0 = señal)
	{
		m_ZXNet.SendState (nBit == 0, nNow);
	}
}

int CInterface1::GetRunningDrive (void) const
{
	return m_nFlipFlop != 0 ? (int) m_nSelected : -1;
}

u8 CInterface1::ReadControlPort (void)
{
	if (m_nFlipFlop != 0 && m_Drive[m_nSelected].IsCartridge ())
	{
		return m_Drive[m_nSelected].ReadStatus ();
	}

	// bit 2 a 0: protección contra escritura activa; bit 3 a 1: DTR (hay un
	// "terminal" en el RS232 listo para recibir: la salida siempre se recoge)
	return 0xFC;
}

u8 CInterface1::ReadDataPort (void)
{
	if (m_nFlipFlop != 0 && m_Drive[m_nSelected].IsCartridge ())
	{
		return m_Drive[m_nSelected].ReadData ();
	}

	return 0xFF;
}

void CInterface1::WriteControlPort (u8 nValue, u64 nNow)
{
	// Flanco de bajada de COMMS CLK: el bit COMMS DATA entra en el registro que
	// elige el motor (un bit por unidad)
	if ((nValue & CTRL_OUT_COMMSCLK) == 0 && m_bCommsClk)
	{
		m_nFlipFlop <<= 1;
		if ((nValue & CTRL_OUT_COMMSDATA) == 0)
		{
			m_nFlipFlop |= 0x01;
		}

		for (unsigned i = 0; i < MICRODRIVES; i++)
		{
			if (m_nFlipFlop == (1 << i))
			{
				m_nSelected = i;
			}
		}

		if (m_nFlipFlop != 0)
		{
			m_Drive[m_nSelected].Selected ();
		}
	}

	if (m_nFlipFlop != 0)
	{
		m_Drive[m_nSelected].WriteControl (nValue, nNow);
	}

	// RS232: al pasar COMMS DATA a 1 se empieza de cero; CTS en el bit 4
	if ((nValue & CTRL_OUT_COMMSDATA) && !m_bCommsData)
	{
		m_nCountOut = 0;
		m_nDataOut = 0;
		m_nCountIn = 0;
		m_nDataIn = 0;
	}
	m_bCommsData = (nValue & CTRL_OUT_COMMSDATA) != 0;
	m_bCTS = (nValue & 0x10) != 0;

	// red: con WAIT (bit 5 a 0) la ROM espera un byte del cable
	if ((nValue & 0x20) == 0)
	{
		m_ZXNet.WaitData (nNow);
	}

	m_bCommsClk = (nValue & CTRL_OUT_COMMSCLK) != 0;
}

void CInterface1::WriteDataPort (u8 nValue)
{
	if (m_nFlipFlop != 0 && m_Drive[m_nSelected].IsCartridge ())
	{
		m_Drive[m_nSelected].WriteData (nValue);
	}
}
