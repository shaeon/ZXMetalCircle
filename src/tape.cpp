//
// tape.cpp - Cinta virtual TAP/TZX
//
// Formato TZX: https://worldofspectrum.net/TZXformat.html
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "tape.h"
#include "inflate.h"
#include <circle/util.h>

// Temporizaciones estándar de la ROM (t-states a 3,5 MHz)
#define PILOT_PULSE		2168
#define PILOT_HEADER		8063
#define PILOT_DATA		3223
#define SYNC1_PULSE		667
#define SYNC2_PULSE		735
#define ZERO_PULSE		855
#define ONE_PULSE		1710
#define TAP_PAUSE_MS		1000

#define TSTATES_PER_MS		3500

CTape::CTape (void)
:	m_pData (0),
	m_nSize (0),
	m_bTZX (FALSE),
	m_bPZX (FALSE),
	m_bInverted (FALSE),
	m_nBlocks (0),
	m_nBlock (0),
	m_nUnsupported (0),
	m_nClockFreq (3500000),
	m_b48K (TRUE),
	m_State (StateEnd),
	m_bPlaying (FALSE),
	m_bEar (FALSE),
	m_nNextEdge (0),
	m_PulseClass (PulseUnknown),
	m_nEdges (0),
	m_bNaturalEdge (FALSE),
	m_bStoppedByBlock (FALSE),
	m_bForceLow (TRUE),
	m_pCSWBuffer (0),
	m_nLoopStart (0),
	m_nLoopCount (0),
	m_nCallBlock (0),
	m_nCallIndex (0),
	m_bInCall (FALSE),
	m_bSelectPending (FALSE),
	m_nSelectBlock (0)
{
	m_Name[0] = '\0';
}

CTape::~CTape (void)
{
	Eject ();
}

void CTape::SetMachine (unsigned nClockFreq, boolean b48K)
{
	m_nClockFreq = nClockFreq;
	m_b48K = b48K;
}

////////////////////////////////////////////////////////////////////////////////
// Carga e indexado

boolean CTape::InsertTAP (const u8 *pData, unsigned nSize, const char *pName)
{
	Eject ();

	if (nSize < 2 || nSize > TAPE_MAX_SIZE)
	{
		return FALSE;
	}

	m_pData = new u8[nSize];
	if (m_pData == 0)
	{
		return FALSE;
	}
	memcpy (m_pData, pData, nSize);
	m_nSize = nSize;
	m_bTZX = FALSE;

	if (!IndexTAP ())
	{
		Eject ();
		return FALSE;
	}

	strncpy (m_Name, pName, sizeof m_Name - 1);
	m_Name[sizeof m_Name - 1] = '\0';
	Rewind ();

	return TRUE;
}

boolean CTape::InsertTZX (const u8 *pData, unsigned nSize, const char *pName)
{
	Eject ();

	if (   nSize < 10 || nSize > TAPE_MAX_SIZE
	    || memcmp (pData, "ZXTape!\x1A", 8) != 0)
	{
		return FALSE;
	}

	m_pData = new u8[nSize];
	if (m_pData == 0)
	{
		return FALSE;
	}
	memcpy (m_pData, pData, nSize);
	m_nSize = nSize;
	m_bTZX = TRUE;

	if (!IndexTZX ())
	{
		Eject ();
		return FALSE;
	}

	strncpy (m_Name, pName, sizeof m_Name - 1);
	m_Name[sizeof m_Name - 1] = '\0';
	Rewind ();

	return TRUE;
}

// Un .CSW suelto se convierte en un TZX con un único bloque 0x18
// PZX: "PZXT" y bloques con etiqueta de 4 letras y longitud de 32 bits
// (especificación de pzxtools, de Patrik Rak). Bloques: PZXT (cabecera, con el
// título), PULS (pulsos), DATA (bits con sus pulsos y nivel inicial), PAUS
// (pausa con su nivel), BRWS (texto para el navegador) y STOP (parar).
#define PZX_TAG(a, b, c, d)	((u32) (a) | ((u32) (b) << 8) | ((u32) (c) << 16) | ((u32) (d) << 24))
#define PZX_PZXT		PZX_TAG ('P', 'Z', 'X', 'T')
#define PZX_PULS		PZX_TAG ('P', 'U', 'L', 'S')
#define PZX_DATA		PZX_TAG ('D', 'A', 'T', 'A')
#define PZX_PAUS		PZX_TAG ('P', 'A', 'U', 'S')
#define PZX_BRWS		PZX_TAG ('B', 'R', 'W', 'S')
#define PZX_STOP		PZX_TAG ('S', 'T', 'O', 'P')

boolean CTape::InsertPZX (const u8 *pData, unsigned nSize, const char *pName)
{
	Eject ();

	if (nSize < 8 || nSize > TAPE_MAX_SIZE || memcmp (pData, "PZXT", 4) != 0)
	{
		return FALSE;
	}

	m_pData = new u8[nSize];
	if (m_pData == 0)
	{
		return FALSE;
	}
	memcpy (m_pData, pData, nSize);
	m_nSize = nSize;
	m_bPZX = TRUE;

	if (!IndexPZX ())
	{
		Eject ();
		return FALSE;
	}

	strncpy (m_Name, pName, sizeof m_Name - 1);
	m_Name[sizeof m_Name - 1] = '\0';
	Rewind ();

	return TRUE;
}

boolean CTape::IndexPZX (void)
{
	unsigned nOffset = 0;
	m_nUnsupported = 0;

	while (nOffset + 8 <= m_nSize && m_nBlocks < TAPE_MAX_BLOCKS)
	{
		u32 nTag = DWord (m_pData + nOffset);
		u32 nLength = DWord (m_pData + nOffset + 4);
		if (nLength > m_nSize - nOffset - 8)
		{
			break;			// bloque cortado
		}

		if (   nTag != PZX_PZXT && nTag != PZX_PULS && nTag != PZX_DATA
		    && nTag != PZX_PAUS && nTag != PZX_BRWS && nTag != PZX_STOP)
		{
			m_nUnsupported++;	// desconocido: se salta
		}

		m_BlockOffset[m_nBlocks++] = nOffset;
		nOffset += 8 + nLength;
	}

	return m_nBlocks > 0;
}

// Prepara un bloque PZX para reproducirlo. FALSE si no tiene señal (se salta).
boolean CTape::PreparePZX (const u8 *p)
{
	u32 nTag = DWord (p);
	u32 nLength = DWord (p + 4);
	const u8 *pBody = p + 8;
	const u8 *pEnd = pBody + nLength;

	if (nTag == PZX_PULS)
	{
		m_pPZX = pBody;
		m_pPZXEnd = pEnd;
		m_bPZXLevel = FALSE;		// PULS empieza siempre en nivel bajo
		m_nPZXCount = 0;
		m_State = StatePZXPulses;
		return TRUE;
	}

	if (nTag == PZX_DATA && nLength >= 8)
	{
		u32 nCount = DWord (pBody);
		m_bPZXLevel = (nCount & 0x80000000) != 0;
		m_nPZXBits = nCount & 0x7FFFFFFF;
		m_nPZXTail = Word (pBody + 4);
		m_nPZXSymLen[0] = pBody[6];
		m_nPZXSymLen[1] = pBody[7];
		m_pPZXSym[0] = pBody + 8;
		m_pPZXSym[1] = m_pPZXSym[0] + 2 * m_nPZXSymLen[0];
		m_pPZX = m_pPZXSym[1] + 2 * m_nPZXSymLen[1];
		if (m_pPZX + (m_nPZXBits + 7) / 8 > pEnd)
		{
			return FALSE;		// bloque mal formado
		}
		m_nPZXBit = 0;
		m_nPZXSym = 0;
		m_State = StatePZXData;
		return TRUE;
	}

	if (nTag == PZX_PAUS && nLength >= 4)
	{
		u32 nValue = DWord (pBody);
		m_bPZXLevel = (nValue & 0x80000000) != 0;
		m_nPZXDur = nValue & 0x7FFFFFFF;
		if (m_nPZXDur == 0)
		{
			return FALSE;
		}
		m_State = StatePZXTail;		// un pulso largo con ese nivel
		return TRUE;
	}

	if (nTag == PZX_STOP)
	{
		u16 nFlags = nLength >= 2 ? Word (pBody) : 0;
		if (nFlags == 1 && !m_b48K)
		{
			return FALSE;		// "parar solo en el 48K"
		}
		m_State = StateStop;
		return TRUE;
	}

	return FALSE;				// PZXT, BRWS y desconocidos
}

boolean CTape::InsertCSW (const u8 *pData, unsigned nSize, const char *pName)
{
	static const char Signature[] = "Compressed Square Wave\x1A";
	if (nSize < 0x20 || memcmp (pData, Signature, 23) != 0)
	{
		return FALSE;
	}

	u8 nMajor = pData[0x17];
	u32 nRate, nPulses = 0;
	u8 nCompression;
	unsigned nDataOffset;

	if (nMajor == 1)
	{
		nRate = Word (pData + 0x19);
		nCompression = pData[0x1B];
		nDataOffset = 0x20;

		// En la versión 1 no viene el número de pulsos: se cuentan
		for (unsigned i = nDataOffset; i < nSize; i++)
		{
			if (pData[i] == 0 && i + 4 < nSize)
			{
				i += 4;
			}
			nPulses++;
		}
	}
	else if (nMajor == 2)
	{
		if (nSize < 0x34)
		{
			return FALSE;
		}
		nRate = DWord (pData + 0x19);
		nPulses = DWord (pData + 0x1D);
		nCompression = pData[0x21];
		nDataOffset = 0x34 + pData[0x23];
	}
	else
	{
		return FALSE;
	}

	if (nDataOffset >= nSize || nRate == 0 || nRate > 0xFFFFFF || (nCompression != 1 && nCompression != 2))
	{
		return FALSE;
	}

	unsigned nDataLen = nSize - nDataOffset;
	unsigned nTZXSize = 10 + 15 + nDataLen;
	if (nTZXSize > TAPE_MAX_SIZE)
	{
		return FALSE;
	}

	u8 *pTZX = new u8[nTZXSize];
	if (pTZX == 0)
	{
		return FALSE;
	}

	memcpy (pTZX, "ZXTape!\x1A\x01\x14", 10);
	u8 *p = pTZX + 10;
	u32 nBlockLen = 10 + nDataLen;
	p[0] = 0x18;
	p[1] = nBlockLen; p[2] = nBlockLen >> 8; p[3] = nBlockLen >> 16; p[4] = nBlockLen >> 24;
	p[5] = 0; p[6] = 0;						// pausa
	p[7] = nRate; p[8] = nRate >> 8; p[9] = nRate >> 16;
	p[10] = nCompression;
	p[11] = nPulses; p[12] = nPulses >> 8; p[13] = nPulses >> 16; p[14] = nPulses >> 24;
	memcpy (p + 15, pData + nDataOffset, nDataLen);

	boolean bOK = InsertTZX (pTZX, nTZXSize, pName);
	delete [] pTZX;

	return bOK;
}

boolean CTape::IndexTAP (void)
{
	// [longitud LSB][longitud MSB][datos...]
	unsigned nOffset = 0;
	while (nOffset + 2 <= m_nSize && m_nBlocks < TAPE_MAX_BLOCKS)
	{
		unsigned nLen = Word (m_pData + nOffset);
		if (nOffset + 2 + nLen > m_nSize)
		{
			break;		// bloque truncado: se ignora el resto
		}

		m_BlockOffset[m_nBlocks++] = nOffset;
		nOffset += 2 + nLen;
	}

	return m_nBlocks > 0;
}

unsigned CTape::TZXBlockSize (unsigned nOffset) const
{
	const u8 *p = m_pData + nOffset;
	unsigned nRemain = m_nSize - nOffset;

	// Tamaño mínimo para leer la cabecera del bloque más largo (0x35: 21 bytes)
	if (nRemain < 1)
	{
		return 0;
	}

	#define NEED(n)	if (nRemain < (n)) return 0

	unsigned nSize;
	switch (p[0])
	{
	case 0x10: NEED (5);  nSize = 5 + Word (p + 3);		break;
	case 0x11: NEED (19); nSize = 19 + Word24 (p + 16);	break;
	case 0x12: nSize = 5;					break;
	case 0x13: NEED (2);  nSize = 2 + 2 * p[1];		break;
	case 0x14: NEED (11); nSize = 11 + Word24 (p + 8);	break;
	case 0x15: NEED (9);  nSize = 9 + Word24 (p + 6);	break;
	case 0x18:
	case 0x19: NEED (5);  nSize = 5 + DWord (p + 1);	break;
	case 0x20: nSize = 3;					break;
	case 0x21: NEED (2);  nSize = 2 + p[1];			break;
	case 0x22: nSize = 1;					break;
	case 0x23: nSize = 3;					break;
	case 0x24: nSize = 3;					break;
	case 0x25: nSize = 1;					break;
	case 0x26: NEED (3);  nSize = 3 + 2 * Word (p + 1);	break;
	case 0x27: nSize = 1;					break;
	case 0x28: NEED (3);  nSize = 3 + Word (p + 1);		break;
	case 0x2A: nSize = 5;					break;
	case 0x2B: nSize = 6;					break;
	case 0x30: NEED (2);  nSize = 2 + p[1];			break;
	case 0x31: NEED (3);  nSize = 3 + p[2];			break;
	case 0x32: NEED (3);  nSize = 3 + Word (p + 1);		break;
	case 0x33: NEED (2);  nSize = 2 + 3 * p[1];		break;
	case 0x35: NEED (21); nSize = 21 + DWord (p + 17);	break;
	case 0x5A: nSize = 10;					break;
	default:
		// Bloques de extensión: ID + longitud de 32 bits + datos
		NEED (5);
		nSize = 5 + DWord (p + 1);
		break;
	}

	#undef NEED

	return nSize <= nRemain ? nSize : 0;
}

boolean CTape::IndexTZX (void)
{
	unsigned nOffset = 10;	// "ZXTape!" 0x1A, versión mayor, versión menor
	m_nUnsupported = 0;

	while (nOffset < m_nSize && m_nBlocks < TAPE_MAX_BLOCKS)
	{
		unsigned nSize = TZXBlockSize (nOffset);
		if (nSize == 0)
		{
			break;
		}

		switch (m_pData[nOffset])
		{
		case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
		case 0x18: case 0x19: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:
		case 0x26: case 0x27: case 0x28:
		case 0x2A: case 0x2B: case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x35: case 0x5A:
			break;

		default:		// desconocidos
			m_nUnsupported++;
			break;
		}

		m_BlockOffset[m_nBlocks++] = nOffset;
		nOffset += nSize;
	}

	return m_nBlocks > 0;
}

void CTape::Eject (void)
{
	m_bPlaying = FALSE;
	m_nBlocks = 0;
	m_nBlock = 0;
	m_nSize = 0;
	m_nUnsupported = 0;
	m_bTZX = FALSE;
	m_bPZX = FALSE;
	m_bInverted = FALSE;		// cada cinta empieza con la polaridad normal
	m_Name[0] = '\0';
	m_State = StateEnd;
	FreeCSWBuffer ();

	delete [] m_pData;
	m_pData = 0;
}

boolean CTape::AppendTZXBlock (const u8 *pBlock, unsigned nLength, const char *pName)
{
	if ((!m_bTZX && m_nBlocks > 0) || nLength > 0xFFFF)
	{
		return FALSE;
	}

	if (m_nBlocks == 0)
	{
		Eject ();
		strncpy (m_Name, pName, sizeof m_Name - 1);
		m_Name[sizeof m_Name - 1] = '\0';
		m_pData = new u8[10];
		if (m_pData == 0)
		{
			return FALSE;
		}
		memcpy (m_pData, "ZXTape!\x1A\x01\x14", 10);
		m_nSize = 10;
		m_bTZX = TRUE;
	}

	if (m_nBlocks >= TAPE_MAX_BLOCKS || m_nSize + 5 + nLength > TAPE_MAX_SIZE)
	{
		return FALSE;
	}

	u8 *pNew = new u8[m_nSize + 5 + nLength];
	if (pNew == 0)
	{
		return FALSE;
	}

	memcpy (pNew, m_pData, m_nSize);
	u8 *p = pNew + m_nSize;
	p[0] = 0x10;
	p[1] = TAP_PAUSE_MS & 0xFF;	// pausa de 1 s, como la ROM
	p[2] = TAP_PAUSE_MS >> 8;
	p[3] = nLength & 0xFF;
	p[4] = nLength >> 8;
	memcpy (p + 5, pBlock, nLength);

	delete [] m_pData;
	m_pData = pNew;

	m_BlockOffset[m_nBlocks++] = m_nSize;
	m_nSize += 5 + nLength;

	m_nBlock = m_nBlocks;
	m_bPlaying = FALSE;
	m_State = StateNextBlock;

	return TRUE;
}

void CTape::FreeCSWBuffer (void)
{
	delete [] m_pCSWBuffer;
	m_pCSWBuffer = 0;
}

boolean CTape::AppendTAPBlock (const u8 *pBlock, unsigned nLength, const char *pName)
{
	if (m_bTZX || nLength > 0xFFFF)
	{
		return FALSE;
	}

	if (m_nBlocks == 0)
	{
		// cinta nueva
		Eject ();
		strncpy (m_Name, pName, sizeof m_Name - 1);
		m_Name[sizeof m_Name - 1] = '\0';
		m_nBlock = 0;
		m_State = StateNextBlock;
	}

	if (m_nBlocks >= TAPE_MAX_BLOCKS || m_nSize + 2 + nLength > TAPE_MAX_SIZE)
	{
		return FALSE;
	}

	u8 *pNew = new u8[m_nSize + 2 + nLength];
	if (pNew == 0)
	{
		return FALSE;
	}

	if (m_pData != 0)
	{
		memcpy (pNew, m_pData, m_nSize);
	}
	pNew[m_nSize] = nLength & 0xFF;
	pNew[m_nSize + 1] = nLength >> 8;
	memcpy (pNew + m_nSize + 2, pBlock, nLength);

	delete [] m_pData;
	m_pData = pNew;

	m_BlockOffset[m_nBlocks++] = m_nSize;
	m_nSize += 2 + nLength;

	// la posición de la cinta queda después del bloque grabado
	m_nBlock = m_nBlocks;
	m_bPlaying = FALSE;
	m_State = StateNextBlock;

	return TRUE;
}

void CTape::Rewind (void)
{
	m_bPlaying = FALSE;
	m_nBlock = 0;
	m_nLoopCount = 0;
	m_bInCall = FALSE;
	m_bSelectPending = FALSE;
	m_State = StateNextBlock;
	m_bEar = FALSE;
}

int CTape::GetHardwareInfo (u8 nType, u8 nId) const
{
	if (!m_bTZX)
	{
		return -1;
	}
	int nResult = -1;
	for (unsigned i = 0; i < m_nBlocks; i++)
	{
		const u8 *p = m_pData + m_BlockOffset[i];
		if (p[0] != 0x33)
		{
			continue;
		}
		unsigned nCount = p[1];
		for (unsigned j = 0; j < nCount; j++)
		{
			const u8 *q = p + 2 + 3 * j;
			if (q + 3 > m_pData + m_nSize)
			{
				break;
			}
			if (q[0] == nType && q[1] == nId)
			{
				nResult = q[2];
			}
		}
	}
	return nResult;
}

void CTape::SeekBlock (unsigned nBlock)
{
	Rewind ();				// estado limpio (bucles, llamadas, selección)
	m_nBlock = nBlock < m_nBlocks ? nBlock : m_nBlocks;
}

// Copia un texto de la cinta a pDest, solo con caracteres imprimibles
static void CopyText (char *pDest, unsigned nSize, const u8 *pSrc, unsigned nLength)
{
	unsigned j = 0;
	for (unsigned i = 0; i < nLength && j + 1 < nSize; i++)
	{
		u8 c = pSrc[i];
		pDest[j++] = (c >= 32 && c < 127) ? (char) c : (c == 13 ? ' ' : '?');
	}
	while (j > 0 && pDest[j - 1] == ' ')
	{
		j--;			// sin espacios al final (los nombres son de 10)
	}
	pDest[j] = '\0';
}

// Un bloque de datos estándar (flag + datos + paridad): cabecera de la ROM o datos
static void DescribeStandard (const u8 *pBlock, unsigned nLength, CTape::TBlockInfo *pInfo)
{
	if (nLength == 19 && pBlock[0] == 0x00 && pBlock[1] <= 3)
	{
		static const CTape::TBlockKind Kinds[4] =
			{CTape::BlockProgram, CTape::BlockNumArray, CTape::BlockCharArray, CTape::BlockBytes};
		pInfo->Kind = Kinds[pBlock[1]];
		CopyText (pInfo->Text, sizeof pInfo->Text, pBlock + 2, 10);
		pInfo->nLength = pBlock[12] | (pBlock[13] << 8);
		pInfo->nStart = pBlock[14] | (pBlock[15] << 8);
		return;
	}

	pInfo->Kind = CTape::BlockData;
	pInfo->nFlag = nLength > 0 ? pBlock[0] : 0;
	pInfo->nLength = nLength >= 2 ? nLength - 2 : 0;	// sin flag ni paridad
}

boolean CTape::GetBlockInfo (unsigned nBlock, TBlockInfo *pInfo) const
{
	if (nBlock >= m_nBlocks)
	{
		return FALSE;
	}

	memset (pInfo, 0, sizeof *pInfo);
	pInfo->Kind = BlockUnknown;
	const u8 *p = m_pData + m_BlockOffset[nBlock];

	if (m_bPZX)
	{
		u32 nTag = DWord (p);
		u32 nLength = DWord (p + 4);
		const u8 *pBody = p + 8;
		if (nTag == PZX_PZXT)
		{
			pInfo->Kind = BlockArchive;	// el primer texto tras la versión es el título
			if (nLength > 2)
			{
				unsigned n = 0;
				while (n < nLength - 2 && pBody[2 + n] != 0)
				{
					n++;
				}
				CopyText (pInfo->Text, sizeof pInfo->Text, pBody + 2, n);
			}
		}
		else if (nTag == PZX_PULS)
		{
			pInfo->Kind = BlockPulses;	// número de pulsos
			const u8 *q = pBody, *pEnd = pBody + nLength;
			while (q + 2 <= pEnd)
			{
				u32 nDur = Word (q), nCount = 1;
				q += 2;
				if (nDur > 0x8000 && q + 2 <= pEnd)	{ nCount = nDur & 0x7FFF; nDur = Word (q); q += 2; }
				if (nDur >= 0x8000)			{ q += 2; }
				pInfo->nLength += nCount;
			}
		}
		else if (nTag == PZX_DATA && nLength >= 8)
		{
			u32 nBits = DWord (pBody) & 0x7FFFFFFF;
			const u8 *pData = pBody + 8 + 2 * pBody[6] + 2 * pBody[7];
			if (nBits % 8 == 0 && pData + nBits / 8 <= pBody + nLength)
			{
				DescribeStandard (pData, nBits / 8, pInfo);	// cabecera o datos
			}
			else
			{
				pInfo->Kind = BlockPureData;
				pInfo->nLength = (nBits + 7) / 8;
			}
		}
		else if (nTag == PZX_PAUS && nLength >= 4)
		{
			pInfo->Kind = BlockPause;
			pInfo->nLength = (DWord (pBody) & 0x7FFFFFFF) / 3500;	// ms
		}
		else if (nTag == PZX_BRWS)
		{
			pInfo->Kind = BlockText;
			CopyText (pInfo->Text, sizeof pInfo->Text, pBody, nLength);
		}
		else if (nTag == PZX_STOP)
		{
			pInfo->Kind = (nLength >= 2 && Word (pBody) == 1) ? BlockStop48K : BlockStop;
		}
		else
		{
			pInfo->Kind = BlockUnknown;
		}
		return TRUE;
	}

	if (!m_bTZX)
	{
		DescribeStandard (p + 2, Word (p), pInfo);
		return TRUE;
	}

	switch (p[0])
	{
	case 0x10:	DescribeStandard (p + 5, Word (p + 3), pInfo);			break;
	case 0x11:	pInfo->Kind = BlockTurbo;	pInfo->nLength = Word24 (p + 16);
			pInfo->nFlag = pInfo->nLength > 0 ? p[19] : 0;			break;
	case 0x12:	pInfo->Kind = BlockTone;	pInfo->nLength = Word (p + 3);	break;
	case 0x13:	pInfo->Kind = BlockPulses;	pInfo->nLength = p[1];		break;
	case 0x14:	pInfo->Kind = BlockPureData;	pInfo->nLength = Word24 (p + 8);	break;
	case 0x15:	pInfo->Kind = BlockDirect;	pInfo->nLength = Word24 (p + 6);	break;
	case 0x18:	pInfo->Kind = BlockCSW;		pInfo->nLength = DWord (p + 1);	break;
	case 0x19:	pInfo->Kind = BlockGDB;		pInfo->nLength = DWord (p + 1);	break;
	case 0x20:	pInfo->nLength = Word (p + 1);
			pInfo->Kind = pInfo->nLength == 0 ? BlockStop : BlockPause;	break;
	case 0x21:	pInfo->Kind = BlockGroup;
			CopyText (pInfo->Text, sizeof pInfo->Text, p + 2, p[1]);	break;
	case 0x22:	pInfo->Kind = BlockGroupEnd;					break;
	case 0x23:	pInfo->Kind = BlockJump;	pInfo->nJump = (s16) Word (p + 1);	break;
	case 0x24:	pInfo->Kind = BlockLoop;	pInfo->nLength = Word (p + 1);	break;
	case 0x25:	pInfo->Kind = BlockLoopEnd;					break;
	case 0x26:	pInfo->Kind = BlockCall;	pInfo->nLength = Word (p + 1);	break;
	case 0x27:	pInfo->Kind = BlockReturn;					break;
	case 0x28:	pInfo->Kind = BlockSelect;	pInfo->nLength = p[3];		break;
	case 0x2A:	pInfo->Kind = BlockStop48K;					break;
	case 0x2B:	pInfo->Kind = BlockLevel;	pInfo->nLength = p[5];		break;
	case 0x30:	pInfo->Kind = BlockText;
			CopyText (pInfo->Text, sizeof pInfo->Text, p + 2, p[1]);	break;
	case 0x31:	pInfo->Kind = BlockText;
			CopyText (pInfo->Text, sizeof pInfo->Text, p + 3, p[2]);	break;
	case 0x32:
		{
			// información del archivo: el título (campo 0x00), si lo hay
			pInfo->Kind = BlockArchive;
			unsigned nCount = p[3];
			const u8 *q = p + 4;
			const u8 *pEnd = p + 3 + Word (p + 1);
			for (unsigned i = 0; i < nCount && q + 2 <= pEnd; i++)
			{
				if (q[0] == 0x00)
				{
					CopyText (pInfo->Text, sizeof pInfo->Text, q + 2, q[1]);
					break;
				}
				q += 2 + q[1];
			}
		}
		break;
	case 0x33:	pInfo->Kind = BlockHardware;					break;
	case 0x35:	pInfo->Kind = BlockCustom;
			CopyText (pInfo->Text, sizeof pInfo->Text, p + 1, 16);	break;
	case 0x5A:	pInfo->Kind = BlockGlue;					break;
	default:	pInfo->Kind = BlockUnknown;	pInfo->nLength = p[0];		break;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Carga instantánea

// ¿Duración parecida a la estándar? (±15 %, como la comprobación de los PZX)
static boolean Near (unsigned nValue, unsigned nStd)
{
	return nValue >= nStd * 85 / 100 && nValue <= nStd * 115 / 100;
}

const u8 *CTape::GetStandardBlock (unsigned *pLength)
{
	while (m_nBlock < m_nBlocks)
	{
		const u8 *p = m_pData + m_BlockOffset[m_nBlock];

		if (m_bPZX)
		{
			// Un bloque DATA con los bits de la ROM (dos pulsos de 855 para
			// el 0 y dos de 1710 para el 1, con margen) y bytes enteros; si va
			// detrás de su tono guía (PULS), se salta el tono.
			u32 nTag = DWord (p);
			if (nTag == PZX_PZXT || nTag == PZX_BRWS || nTag == PZX_PAUS)
			{
				m_nBlock++;
				continue;
			}
			unsigned nData = m_nBlock;
			if (nTag == PZX_PULS && m_nBlock + 1 < m_nBlocks)
			{
				nData = m_nBlock + 1;
			}
			const u8 *q = m_pData + m_BlockOffset[nData];
			if (DWord (q) != PZX_DATA || DWord (q + 4) < 8)
			{
				return 0;
			}
			const u8 *pBody = q + 8;
			u32 nBits = DWord (pBody) & 0x7FFFFFFF;
			if (nBits == 0 || nBits % 8 != 0 || pBody[6] != 2 || pBody[7] != 2)
			{
				return 0;
			}
			for (unsigned i = 0; i < 4; i++)
			{
				unsigned nDur = Word (pBody + 8 + 2 * i);
				unsigned nStd = i < 2 ? ZERO_PULSE : ONE_PULSE;
				if (nDur < nStd * 85 / 100 || nDur > nStd * 115 / 100)
				{
					return 0;	// turbo: hace falta la carga por pulsos
				}
			}
			const u8 *pData = pBody + 16;
			if (pData + nBits / 8 > pBody + DWord (q + 4))
			{
				return 0;
			}
			m_nBlock = nData;
			*pLength = nBits / 8;
			return pData;
		}

		if (!m_bTZX)
		{
			*pLength = Word (p);
			return p + 2;
		}

		switch (p[0])
		{
		case 0x10:
			*pLength = Word (p + 3);
			return p + 5;

		case 0x11:
			// "Turbo" con los tiempos de la ROM (con margen) y bytes enteros:
			// hay TZX que guardan así bloques normales (JSpeccy los carga al
			// instante por lo mismo; cita Midnight Resistance 128K)
			if (   Near (Word (p + 1), PILOT_PULSE) && Near (Word (p + 3), SYNC1_PULSE)
			    && Near (Word (p + 5), SYNC2_PULSE) && Near (Word (p + 7), ZERO_PULSE)
			    && Near (Word (p + 9), ONE_PULSE) && p[13] == 8)
			{
				*pLength = Word24 (p + 16);
				return p + 19;
			}
			return 0;

		// bloques sin señal que se pueden saltar
		case 0x20: case 0x21: case 0x22: case 0x2B: case 0x30: case 0x31:
		case 0x32: case 0x33: case 0x35: case 0x5A:
			m_nBlock++;
			break;

		case 0x2A:		// "parar si 48K"
			m_nBlock++;
			break;

		default:		// turbo, tonos, bucles...: hace falta la carga por pulsos
			return 0;
		}
	}

	return 0;
}

void CTape::NextBlock (void)
{
	if (m_nBlock < m_nBlocks)
	{
		m_nBlock++;
	}

	m_State = StateNextBlock;
	m_bPlaying = FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Reproducción por pulsos

boolean CTape::PrepareBlock (void)
{
	FreeCSWBuffer ();

	for (unsigned nGuard = 0; nGuard < 2 * TAPE_MAX_BLOCKS && m_nBlock < m_nBlocks; nGuard++)
	{
		const u8 *p = m_pData + m_BlockOffset[m_nBlock];

		if (m_bPZX)
		{
			if (PreparePZX (p))
			{
				return TRUE;
			}
			m_nBlock++;
			continue;
		}

		// valores estándar
		m_nPilotLen = PILOT_PULSE;
		m_nSync1 = SYNC1_PULSE;
		m_nSync2 = SYNC2_PULSE;
		m_nZeroLen = ZERO_PULSE;
		m_nOneLen = ONE_PULSE;
		m_nUsedBits = 8;

		if (!m_bTZX)
		{
			m_nDataLen = Word (p);
			m_pBlockData = p + 2;
			m_nPauseMs = TAP_PAUSE_MS;
			m_nPilotPulses = (m_nDataLen > 0 && m_pBlockData[0] < 0x80) ? PILOT_HEADER : PILOT_DATA;
			m_nCount = m_nPilotPulses;
			m_State = StatePilot;
			return TRUE;
		}

		switch (p[0])
		{
		case 0x10:		// datos estándar
			m_nPauseMs = Word (p + 1);
			m_nDataLen = Word (p + 3);
			m_pBlockData = p + 5;
			m_nPilotPulses = (m_nDataLen > 0 && m_pBlockData[0] < 0x80) ? PILOT_HEADER : PILOT_DATA;
			m_nCount = m_nPilotPulses;
			m_State = StatePilot;
			return TRUE;

		case 0x11:		// datos turbo
			m_nPilotLen = Word (p + 1);
			m_nSync1 = Word (p + 3);
			m_nSync2 = Word (p + 5);
			m_nZeroLen = Word (p + 7);
			m_nOneLen = Word (p + 9);
			m_nPilotPulses = Word (p + 11);
			m_nUsedBits = p[13];
			m_nPauseMs = Word (p + 14);
			m_nDataLen = Word24 (p + 16);
			m_pBlockData = p + 19;
			m_nCount = m_nPilotPulses;
			m_State = StatePilot;
			return TRUE;

		case 0x12:		// tono puro
			m_nPilotLen = Word (p + 1);
			m_nPilotPulses = Word (p + 3);
			m_nSync1 = m_nSync2 = 0;
			m_nDataLen = 0;
			m_pBlockData = p;
			m_nPauseMs = 0;
			m_nCount = m_nPilotPulses;
			m_State = StatePilot;
			return TRUE;

		case 0x13:		// secuencia de pulsos
			m_nCount = p[1];
			m_pPulses = p + 2;
			m_State = StatePulseSeq;
			return TRUE;

		case 0x14:		// datos puros (sin tono guía ni sincronismo)
			m_nPilotPulses = 0;
			m_nSync1 = m_nSync2 = 0;
			m_nZeroLen = Word (p + 1);
			m_nOneLen = Word (p + 3);
			m_nUsedBits = p[5];
			m_nPauseMs = Word (p + 6);
			m_nDataLen = Word24 (p + 8);
			m_pBlockData = p + 11;
			m_nCount = 0;
			m_State = StatePilot;
			return TRUE;

		case 0x15:		// grabación directa
			m_nSampleLen = Word (p + 1);
			m_nPauseMs = Word (p + 3);
			m_nUsedBits = p[5];
			m_nDataLen = Word24 (p + 6);
			m_pBlockData = p + 9;
			m_nByte = 0;
			m_nBit = 0;
			m_State = StateDirect;
			return TRUE;

		case 0x18:		// grabación CSW
			if (PrepareCSW (p))
			{
				return TRUE;
			}
			m_nBlock++;
			break;

		case 0x19:		// datos generalizados
			if (PrepareGDB (p))
			{
				return TRUE;
			}
			m_nBlock++;
			break;

		case 0x20:		// pausa (0 = parar la cinta)
			m_nPauseMs = Word (p + 1);
			m_State = m_nPauseMs == 0 ? StateStop : StatePause;
			return TRUE;

		case 0x2A:		// parar la cinta si es un 48K
			if (m_b48K)
			{
				m_State = StateStop;
				return TRUE;
			}
			m_nBlock++;
			break;

		case 0x2B:		// nivel de la señal
			m_bEar = p[5] != 0;
			m_nBlock++;
			break;

		case 0x23:		// salto relativo
		{
			int nRel = (s16) Word (p + 1);
			int nTarget = (int) m_nBlock + (nRel == 0 ? 1 : nRel);
			if (nTarget < 0 || nTarget >= (int) m_nBlocks)
			{
				m_nBlock = m_nBlocks;
			}
			else
			{
				m_nBlock = nTarget;
			}
			break;
		}

		case 0x24:		// inicio de bucle
			m_nLoopCount = Word (p + 1);
			m_nLoopStart = m_nBlock + 1;
			m_nBlock++;
			break;

		case 0x25:		// fin de bucle
			if (m_nLoopCount > 1)
			{
				m_nLoopCount--;
				m_nBlock = m_nLoopStart;
			}
			else
			{
				m_nLoopCount = 0;
				m_nBlock++;
			}
			break;

		case 0x26:		// llamada a secuencias: N desplazamientos (relativos a este bloque)
			if (Word (p + 1) == 0)
			{
				m_nBlock++;
			}
			else
			{
				m_nCallBlock = m_nBlock;
				m_nCallIndex = 0;
				m_bInCall = TRUE;
				JumpRelative (m_nCallBlock, (s16) Word (p + 3));
			}
			break;

		case 0x27:		// vuelta de la secuencia: la siguiente, o tras el 0x26
			if (!m_bInCall)
			{
				m_nBlock++;
				break;
			}
			{
				const u8 *pCall = m_pData + m_BlockOffset[m_nCallBlock];
				if (++m_nCallIndex < Word (pCall + 1))
				{
					JumpRelative (m_nCallBlock, (s16) Word (pCall + 3 + 2 * m_nCallIndex));
				}
				else
				{
					m_bInCall = FALSE;
					m_nBlock = m_nCallBlock + 1;
				}
			}
			break;

		case 0x28:		// selección: la cinta se para hasta que se elige
			if (GetSelectCountAt (p) == 0)
			{
				m_nBlock++;
				break;
			}
			m_bSelectPending = TRUE;
			m_nSelectBlock = m_nBlock;
			m_State = StateNextBlock;
			return FALSE;

		default:		// informativos y no soportados
			m_nBlock++;
			break;
		}
	}

	m_State = StateEnd;
	return FALSE;
}

// Salto relativo a un bloque (0 o fuera de la cinta: siguiente / final)
void CTape::JumpRelative (unsigned nFrom, int nRel)
{
	int nTarget = (int) nFrom + (nRel == 0 ? 1 : nRel);
	m_nBlock = (nTarget < 0 || nTarget >= (int) m_nBlocks) ? m_nBlocks : (unsigned) nTarget;
}

// Bloque 0x28: [longitud:2][N:1] y N veces [desplazamiento:2][longitud:1][texto]
unsigned CTape::GetSelectCountAt (const u8 *p) const
{
	return p[3];
}

unsigned CTape::GetSelectCount (void) const
{
	return m_bSelectPending ? GetSelectCountAt (m_pData + m_BlockOffset[m_nSelectBlock]) : 0;
}

static const u8 *SelectEntry (const u8 *pBlock, unsigned nItem, unsigned nBlockSize)
{
	const u8 *p = pBlock + 4;
	const u8 *pEnd = pBlock + nBlockSize;
	for (unsigned i = 0; i < nItem && p + 3 <= pEnd; i++)
	{
		p += 3 + p[2];
	}
	return p + 3 <= pEnd ? p : 0;
}

boolean CTape::GetSelectItem (unsigned nItem, char *pText, unsigned nSize) const
{
	if (!m_bSelectPending || nItem >= GetSelectCount ())
	{
		return FALSE;
	}
	const u8 *pBlock = m_pData + m_BlockOffset[m_nSelectBlock];
	const u8 *pEntry = SelectEntry (pBlock, nItem, TZXBlockSize (m_BlockOffset[m_nSelectBlock]));
	if (pEntry == 0)
	{
		return FALSE;
	}
	unsigned nLen = pEntry[2];
	if (nLen > nSize - 1)
	{
		nLen = nSize - 1;
	}
	for (unsigned i = 0; i < nLen; i++)
	{
		char c = pEntry[3 + i];
		pText[i] = (c >= 32 && c < 127) ? c : ' ';
	}
	pText[nLen] = '\0';
	return TRUE;
}

void CTape::Select (unsigned nItem)
{
	if (!m_bSelectPending)
	{
		return;
	}
	m_bSelectPending = FALSE;
	const u8 *pBlock = m_pData + m_BlockOffset[m_nSelectBlock];
	const u8 *pEntry = SelectEntry (pBlock, nItem, TZXBlockSize (m_BlockOffset[m_nSelectBlock]));
	if (pEntry == 0)
	{
		m_nBlock = m_nSelectBlock + 1;
	}
	else
	{
		JumpRelative (m_nSelectBlock, (s16) Word (pEntry));
	}
	m_State = StateNextBlock;
}

void CTape::CancelSelect (void)
{
	if (m_bSelectPending)
	{
		m_bSelectPending = FALSE;
		m_nBlock = m_nSelectBlock + 1;
		m_State = StateNextBlock;
	}
}

boolean CTape::PrepareCSW (const u8 *p)
{
	FreeCSWBuffer ();

	u32 nBlockLen = DWord (p + 1);
	if (nBlockLen < 10)
	{
		return FALSE;
	}

	const u8 *q = p + 5;
	m_nPauseMs = Word (q);
	m_nCSWRate = Word24 (q + 2);
	u8 nCompression = q[5];
	u32 nPulses = DWord (q + 6);
	const u8 *pData = q + 10;
	unsigned nDataLen = nBlockLen - 10;

	if (m_nCSWRate == 0)
	{
		return FALSE;
	}

	if (nCompression == 1)		// RLE
	{
		m_pCSW = pData;
		m_nCSWLen = nDataLen;
	}
	else if (nCompression == 2)	// Z-RLE: RLE comprimido con zlib
	{
		unsigned nMax = nPulses * 5 + 16;	// como mucho 5 bytes por pulso
		if (nMax > 32 * 1024 * 1024)
		{
			nMax = 32 * 1024 * 1024;
		}

		m_pCSWBuffer = new u8[nMax];
		if (m_pCSWBuffer == 0)
		{
			return FALSE;
		}

		int nLen = ZlibInflate (pData, nDataLen, m_pCSWBuffer, nMax);
		if (nLen < 0)
		{
			FreeCSWBuffer ();
			return FALSE;
		}

		m_pCSW = m_pCSWBuffer;
		m_nCSWLen = nLen;
	}
	else
	{
		return FALSE;
	}

	m_nCSWPos = 0;
	m_nCSWFrac = 0;
	m_State = StateCSW;

	return TRUE;
}

boolean CTape::PrepareGDB (const u8 *p)
{
	u32 nBlockLen = DWord (p + 1);
	const u8 *q = p + 5;
	const u8 *pEnd = q + nBlockLen;

	if (nBlockLen < 14)
	{
		return FALSE;
	}

	m_nPauseMs = Word (q);
	m_nTOTP = DWord (q + 2);
	m_nNPP = q[6];
	unsigned nASP = q[7] ? q[7] : 256;
	m_nTOTD = DWord (q + 8);
	m_nNPD = q[12];
	m_nASD = q[13] ? q[13] : 256;
	q += 14;

	m_pSymDefPilot = m_pPRLE = m_pSymDefData = m_pGDBData = 0;

	if (m_nTOTP > 0)
	{
		m_pSymDefPilot = q;
		q += nASP * (2 * m_nNPP + 1);
		m_pPRLE = q;
		q += 3 * m_nTOTP;
	}

	m_nNB = 0;
	while ((1U << m_nNB) < m_nASD)
	{
		m_nNB++;
	}

	if (m_nTOTD > 0)
	{
		m_pSymDefData = q;
		q += m_nASD * (2 * m_nNPD + 1);
		m_pGDBData = q;
		q += ((u64) m_nNB * m_nTOTD + 7) / 8;
	}

	if (q > pEnd)
	{
		return FALSE;		// bloque mal formado
	}

	m_nPRLEIndex = 0;
	m_nRepeat = 0;
	m_nSymIndex = 0;
	m_pSym = 0;
	m_State = StateGDBPilot;

	return TRUE;
}

void CTape::StartSymbol (const u8 *pSymDef, unsigned nMaxPulses)
{
	m_pSym = pSymDef;
	m_nSymPulse = 0;
	m_nSymMax = nMaxPulses;
}

unsigned CTape::SymbolPulse (void)
{
	if (m_nSymPulse >= m_nSymMax)
	{
		return 0;
	}

	unsigned nLen = Word (m_pSym + 1 + 2 * m_nSymPulse);
	if (nLen == 0)
	{
		return 0;
	}

	if (m_nSymPulse == 0)
	{
		switch (m_pSym[0] & 0x03)
		{
		case 0: Edge ();		break;	// flanco (nivel bajo si empieza el bloque)
		case 1: m_bForceLow = FALSE;	break;	// mismo nivel
		case 2: m_bEar = FALSE;		m_bForceLow = FALSE;	break;	// nivel bajo
		case 3: m_bEar = TRUE;		m_bForceLow = FALSE;	break;	// nivel alto
		}
	}
	else
	{
		m_bEar = !m_bEar;
	}

	m_nSymPulse++;

	return nLen;
}

// Un flanco al empezar un pulso. Como libspectrum (Fuse): el primer pulso de
// un bloque que va tras una pausa, o al empezar la cinta, es nivel bajo; los
// demás cambian de nivel. Antes ese primer pulso cambiaba desde el nivel bajo
// de la pausa y salía alto: todo el bloque iba con la polaridad invertida. A
// los cargadores que solo miran los flancos les da igual, pero los que miden
// cada bit como nivel bajo más nivel alto (Basil the Great Mouse Detective,
// de Gremlin) juntaban medio bit con medio del siguiente y no cargaban.
void CTape::Edge (void)
{
	if (m_bForceLow)
	{
		m_bEar = FALSE;
		m_bForceLow = FALSE;
	}
	else
	{
		m_bEar = !m_bEar;
	}
}

unsigned CTape::Step (void)
{
	m_PulseClass = PulseUnknown;		// solo los pulsos de datos se conocen

	for (unsigned nGuard = 0; nGuard < 4 * TAPE_MAX_BLOCKS; nGuard++)
	{
		switch (m_State)
		{
		case StateNextBlock:
			if (!PrepareBlock ())
			{
				return 0;
			}
			continue;

		case StatePilot:
			if (m_nCount > 0)
			{
				m_nCount--;
				Edge ();
				return m_nPilotLen;
			}
			m_State = StateSync1;
			continue;

		case StateSync1:
			m_State = StateSync2;
			if (m_nSync1 > 0)
			{
				Edge ();
				return m_nSync1;
			}
			continue;

		case StateSync2:
			m_State = StateDataStart;
			if (m_nSync2 > 0)
			{
				Edge ();
				return m_nSync2;
			}
			continue;

		case StateDataStart:
			m_nByte = 0;
			m_nBit = 0;
			m_nHalf = 0;
			m_State = StateData;
			continue;

		case StateData:
		{
			if (   m_nByte >= m_nDataLen
			    || (m_nByte == m_nDataLen - 1 && m_nBit >= m_nUsedBits))
			{
				m_State = StatePause;
				continue;
			}

			boolean bOne = (m_pBlockData[m_nByte] & (0x80 >> m_nBit)) != 0;
			unsigned nLen = bOne ? m_nOneLen : m_nZeroLen;
			Edge ();
			// Para acelerar hace falta que un bit 1 dure más que un bit 0 (como
			// en la ROM y casi todos los turbos)
			if (m_nOneLen > m_nZeroLen)
			{
				m_PulseClass = bOne ? PulseLong : PulseShort;
			}

			if (++m_nHalf == 2)
			{
				m_nHalf = 0;
				if (++m_nBit == 8)
				{
					m_nBit = 0;
					m_nByte++;
				}
			}

			return nLen;
		}

		case StatePulseSeq:
			if (m_nCount > 0)
			{
				m_nCount--;
				Edge ();
				unsigned nLen = Word (m_pPulses);
				m_pPulses += 2;
				return nLen;
			}
			m_nBlock++;
			m_State = StateNextBlock;
			continue;

		case StateDirect:
			if (   m_nByte >= m_nDataLen
			    || (m_nByte == m_nDataLen - 1 && m_nBit >= m_nUsedBits))
			{
				m_State = StatePause;
				continue;
			}

			m_bEar = (m_pBlockData[m_nByte] & (0x80 >> m_nBit)) != 0;
			m_bForceLow = FALSE;		// la grabación marca el nivel
			if (++m_nBit == 8)
			{
				m_nBit = 0;
				m_nByte++;
			}
			return m_nSampleLen;

		case StateCSW:
		{
			if (m_nCSWPos >= m_nCSWLen)
			{
				FreeCSWBuffer ();
				m_State = StatePause;
				continue;
			}

			u32 nSamples = m_pCSW[m_nCSWPos++];
			if (nSamples == 0 && m_nCSWPos + 4 <= m_nCSWLen)
			{
				nSamples = DWord (m_pCSW + m_nCSWPos);
				m_nCSWPos += 4;
			}

			m_bEar = !m_bEar;
			m_bForceLow = FALSE;		// CSW: el primer pulso sigue el nivel que había (libspectrum)

			u64 nTotal = (u64) nSamples * 3500000 + m_nCSWFrac;
			u64 nLen = nTotal / m_nCSWRate;
			m_nCSWFrac = nTotal % m_nCSWRate;
			if (nLen == 0)
			{
				nLen = 1;
			}
			if (nLen > 0x7FFFFFFF)
			{
				nLen = 0x7FFFFFFF;
			}
			return (unsigned) nLen;
		}

		case StateGDBPilot:
			if (m_pSym == 0)
			{
				if (m_nRepeat == 0)
				{
					if (m_nPRLEIndex >= m_nTOTP)
					{
						m_State = StateGDBData;
						continue;
					}
					const u8 *pEntry = m_pPRLE + 3 * m_nPRLEIndex++;
					m_nPRLESymbol = pEntry[0];
					m_nRepeat = Word (pEntry + 1);
					continue;
				}

				m_nRepeat--;
				StartSymbol (m_pSymDefPilot + m_nPRLESymbol * (2 * m_nNPP + 1), m_nNPP);
			}

			{
				unsigned nLen = SymbolPulse ();
				if (nLen > 0)
				{
					return nLen;
				}
				m_pSym = 0;
			}
			continue;

		case StateGDBData:
			if (m_pSym == 0)
			{
				if (m_nSymIndex >= m_nTOTD)
				{
					m_State = StatePause;
					continue;
				}

				// NB bits por símbolo, el más significativo primero
				unsigned nSymbol = 0;
				u64 nBit = (u64) m_nSymIndex++ * m_nNB;
				for (unsigned i = 0; i < m_nNB; i++, nBit++)
				{
					nSymbol <<= 1;
					if (m_pGDBData[nBit >> 3] & (0x80 >> (nBit & 7)))
					{
						nSymbol |= 1;
					}
				}

				if (nSymbol >= m_nASD)
				{
					continue;
				}

				StartSymbol (m_pSymDefData + nSymbol * (2 * m_nNPD + 1), m_nNPD);
			}

			{
				unsigned nLen = SymbolPulse ();
				if (nLen > 0)
				{
					return nLen;
				}
				m_pSym = 0;
			}
			continue;

		case StatePZXPulses:
			{
				boolean bEnd = FALSE;
				while (m_nPZXCount == 0 || m_nPZXDur == 0)
				{
					if (m_nPZXCount > 0)
					{
						// pulsos de duración 0: solo cambian el nivel
						if (m_nPZXCount & 1)
						{
							m_bPZXLevel = !m_bPZXLevel;
						}
						m_nPZXCount = 0;
					}
					if (m_pPZX + 2 > m_pPZXEnd)
					{
						bEnd = TRUE;
						break;
					}
					u32 nDur = Word (m_pPZX);
					m_pPZX += 2;
					u32 nCount = 1;
					if (nDur > 0x8000)
					{
						if (m_pPZX + 2 > m_pPZXEnd)
						{
							bEnd = TRUE;
							break;
						}
						nCount = nDur & 0x7FFF;
						nDur = Word (m_pPZX);
						m_pPZX += 2;
					}
					if (nDur >= 0x8000)
					{
						if (m_pPZX + 2 > m_pPZXEnd)
						{
							bEnd = TRUE;
							break;
						}
						nDur = ((nDur & 0x7FFF) << 16) | Word (m_pPZX);
						m_pPZX += 2;
					}
					m_nPZXCount = nCount;
					m_nPZXDur = nDur;
				}
				if (bEnd)
				{
					m_nBlock++;
					m_State = StateNextBlock;
					continue;
				}
				m_nPZXCount--;
				m_bEar = m_bPZXLevel;
				m_bPZXLevel = !m_bPZXLevel;
				return m_nPZXDur;
			}

		case StatePZXData:
			while (m_nPZXBit < m_nPZXBits)
			{
				unsigned nBit = (m_pPZX[m_nPZXBit >> 3] >> (7 - (m_nPZXBit & 7))) & 1;
				if (m_nPZXSym < m_nPZXSymLen[nBit])
				{
					unsigned nDur = Word (m_pPZXSym[nBit] + 2 * m_nPZXSym);
					m_nPZXSym++;
					if (nDur == 0)
					{
						m_bPZXLevel = !m_bPZXLevel;
						continue;
					}
					m_bEar = m_bPZXLevel;
					m_bPZXLevel = !m_bPZXLevel;
					m_PulseClass = nBit ? PulseLong : PulseShort;
					return nDur;
				}
				m_nPZXSym = 0;
				m_nPZXBit++;
			}
			m_nPZXDur = m_nPZXTail;
			m_State = StatePZXTail;
			continue;

		case StatePZXTail:		// pulso final de DATA, o una pausa PAUS
			m_nBlock++;
			m_State = StateNextBlock;
			if (m_nPZXDur == 0)
			{
				continue;
			}
			m_bEar = m_bPZXLevel;
			m_bPZXLevel = !m_bPZXLevel;
			return m_nPZXDur;

		case StatePause:
			if (m_nPauseMs == 0)
			{
				m_nBlock++;
				m_State = StateNextBlock;
				continue;
			}
			// Según la especificación TZX: flanco final, 1 ms de nivel opuesto
			// y el resto de la pausa en nivel bajo
			m_bEar = !m_bEar;
			m_State = StatePauseLow;
			return TSTATES_PER_MS;

		case StatePauseLow:
			m_bEar = FALSE;
			m_bForceLow = TRUE;		// el bloque siguiente empieza en nivel bajo
			m_nBlock++;
			m_State = StateNextBlock;
			return (m_nPauseMs > 1 ? m_nPauseMs - 1 : 1) * TSTATES_PER_MS;

		case StateStop:
			m_nBlock++;
			m_State = StateNextBlock;
			m_bStoppedByBlock = TRUE;
			return 0;

		case StateEnd:
		default:
			return 0;
		}
	}

	m_State = StateEnd;
	return 0;
}

void CTape::Play (u64 nNow)
{
	if (m_nBlocks == 0 || m_nBlock >= m_nBlocks)
	{
		m_bPlaying = FALSE;
		return;
	}

	// Siempre desde el comienzo del bloque actual, y su primer pulso en
	// nivel bajo, como al empezar la cinta en libspectrum. Tiene que ponerse
	// antes de generar ese pulso: antes se ponía después, y si la cinta se
	// arrancaba en un bloque intermedio (tras una carga instantánea, el
	// cargador la arranca solo) el primer pulso salía alto y todo el bloque
	// con la polaridad invertida. El cargador de Basil the Great Mouse
	// Detective mide cada bit como ciclo completo y leía medio bit de cada uno.
	m_State = StateNextBlock;
	m_bForceLow = TRUE;

	unsigned nNext = Step ();
	if (nNext == 0)
	{
		m_bPlaying = FALSE;
		return;
	}

	m_bPlaying = TRUE;
	m_nNextEdge = nNow + (u64) nNext * m_nClockFreq / 3500000;
	m_bNaturalEdge = TRUE;		// sin acelerar hasta que el cargador vea un flanco
	m_bStoppedByBlock = FALSE;
}

void CTape::Stop (void)
{
	m_bPlaying = FALSE;
}

boolean CTape::GetEarBit (u64 nNow)
{
	while (m_bPlaying && nNow >= m_nNextEdge)
	{
		m_bNaturalEdge = TRUE;
		m_nEdges++;
		unsigned nNext = Step ();
		if (nNext == 0)
		{
			m_bPlaying = FALSE;	// fin de la cinta o bloque de parada
			break;
		}

		m_nNextEdge += (u64) nNext * m_nClockFreq / 3500000;
	}

	return m_bEar != m_bInverted;
}

// Aceleración de cargadores: el flanco que cierra el pulso actual llega ahora,
// y el siguiente pulso empieza a contar desde aquí. Este flanco no es "natural":
// el cargador lo da por visto (CSpectrum ha simulado su bucle de espera).
void CTape::ForceEdge (u64 nNow)
{
	if (!m_bPlaying)
	{
		return;
	}

	m_nNextEdge = nNow;
	GetEarBit (nNow);
	m_bNaturalEdge = FALSE;
}
