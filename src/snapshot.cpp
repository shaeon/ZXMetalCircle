//
// snapshot.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "snapshot.h"
#include "i18n.h"
#include "inflate.h"
#include <circle/util.h>

static inline u16 Word (const u8 *p)	{ return p[0] | (p[1] << 8); }
static inline u32 DWord (const u8 *p)	{ return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32) p[3] << 24); }
static inline void PutWord (u8 *p, u16 n)	{ p[0] = n & 0xFF; p[1] = n >> 8; }
static inline void PutDWord (u8 *p, u32 n)	{ p[0] = n; p[1] = n >> 8; p[2] = n >> 16; p[3] = n >> 24; }

static inline boolean Is48 (TModel Model)
{
	return MachineTiming[Model].CodeModel == CodeModel48K;
}

static boolean HasExtension (const char *pName, const char *pExt)
{
	size_t nLen = strlen (pName);
	size_t nExtLen = strlen (pExt);
	return nLen > nExtLen && strcasecmp (pName + nLen - nExtLen, pExt) == 0;
}

const char *CSnapshot::Load (CSpectrum *pSpectrum, const u8 *pData, unsigned nSize,
			     const char *pFileName)
{
	if (HasExtension (pFileName, ".sna"))	return LoadSNA (pSpectrum, pData, nSize);
	if (HasExtension (pFileName, ".z80"))	return LoadZ80 (pSpectrum, pData, nSize);
	if (HasExtension (pFileName, ".szx"))	return LoadSZX (pSpectrum, pData, nSize);

	return N_("Formato de snapshot desconocido");
}

void CSnapshot::SetModel (CSpectrum *pSpectrum, TModel Model)
{
	// un snapshot de +3 cargado en el +3e se queda en el +3e (mismo hardware)
	if (Model == ModelPlus3 && pSpectrum->m_Model == ModelPlus3E)
	{
		Model = ModelPlus3E;
	}

	pSpectrum->m_bIF2Inserted = FALSE;	// el snapshot trae su propia memoria
	pSpectrum->SetModel (Model);		// incluye reset
	pSpectrum->UnpageMultiface ();
}

void CSnapshot::Finish (CSpectrum *pSpectrum)
{
	u8 nIssueMask = (pSpectrum->m_bIssue2 && pSpectrum->m_pTiming->CodeModel == CodeModel48K) ? 0x18 : 0x10;
	pSpectrum->m_bEarOut = (pSpectrum->m_nPortFE & nIssueMask) != 0;
	pSpectrum->UpdatePaging ();
	pSpectrum->m_Audio.Reset ();
	pSpectrum->m_CPU.setHalted (false);
	pSpectrum->m_CPU.setPendingEI (false);
	pSpectrum->m_CPU.setNMI (false);
}

////////////////////////////////////////////////////////////////////////////////
// .SNA

const char *CSnapshot::LoadSNA (CSpectrum *p, const u8 *pData, unsigned nSize)
{
	boolean b128K;
	if (nSize == 49179)
	{
		b128K = FALSE;
	}
	else if (nSize == 131103 || nSize == 147487)
	{
		b128K = TRUE;
	}
	else
	{
		return N_("Tamaño de .SNA no válido");
	}

	if (b128K)
	{
		// En un +2A se queda en +2A; en otro caso, 128K
		SetModel (p, (p->m_Model == ModelPlus2A || p->m_Model == ModelPlus2 || p->m_Model == ModelPlus3
			      || p->m_Model == ModelPlus3E)
			     ? p->m_Model : Model128K);
	}
	else
	{
		SetModel (p, Model48K);
	}

	Z80 &CPU = p->m_CPU;
	CPU.setRegI (pData[0]);
	CPU.setRegHLx (Word (pData + 1));
	CPU.setRegDEx (Word (pData + 3));
	CPU.setRegBCx (Word (pData + 5));
	CPU.setRegAFx (Word (pData + 7));
	CPU.setRegHL (Word (pData + 9));
	CPU.setRegDE (Word (pData + 11));
	CPU.setRegBC (Word (pData + 13));
	CPU.setRegIY (Word (pData + 15));
	CPU.setRegIX (Word (pData + 17));
	CPU.setIFF2 ((pData[19] & 0x04) != 0);
	CPU.setIFF1 ((pData[19] & 0x04) != 0);
	CPU.setRegR (pData[20]);
	CPU.setRegAF (Word (pData + 21));
	CPU.setRegSP (Word (pData + 23));
	switch (pData[25] & 3)
	{
	case 0:  CPU.setIM (Z80::IM0); break;
	case 1:  CPU.setIM (Z80::IM1); break;
	default: CPU.setIM (Z80::IM2); break;
	}
	p->m_nPortFE = pData[26] & 0x07;

	const u8 *pRAM = pData + 27;

	if (!b128K)
	{
		memcpy (p->m_RAM[5], pRAM, 0x4000);
		memcpy (p->m_RAM[2], pRAM + 0x4000, 0x4000);
		memcpy (p->m_RAM[0], pRAM + 0x8000, 0x4000);

		// El PC está en la pila
		u16 nSP = CPU.getRegSP ();
		u8 nLow = 0, nHigh = 0;
		if (nSP >= 0x4000)
		{
			nLow = p->m_RAM[nSP < 0x8000 ? 5 : (nSP < 0xC000 ? 2 : 0)][nSP & 0x3FFF];
		}
		u16 nSP1 = nSP + 1;
		if (nSP1 >= 0x4000)
		{
			nHigh = p->m_RAM[nSP1 < 0x8000 ? 5 : (nSP1 < 0xC000 ? 2 : 0)][nSP1 & 0x3FFF];
		}
		CPU.setRegPC (nLow | (nHigh << 8));
		CPU.setRegSP (nSP + 2);

		p->m_nPort7FFD = 0;
	}
	else
	{
		const u8 *pExtra = pData + 27 + 0xC000;
		CPU.setRegPC (Word (pExtra));
		p->m_nPort7FFD = pExtra[2];
		// pExtra[3]: TR-DOS (se ignora)

		unsigned nPaged = p->m_nPort7FFD & 0x07;
		memcpy (p->m_RAM[5], pRAM, 0x4000);
		memcpy (p->m_RAM[2], pRAM + 0x4000, 0x4000);
		memcpy (p->m_RAM[nPaged], pRAM + 0x8000, 0x4000);

		const u8 *pBank = pExtra + 4;
		for (unsigned nBank = 0; nBank < 8; nBank++)
		{
			if (nBank == 2 || nBank == 5 || nBank == nPaged)
			{
				continue;
			}
			memcpy (p->m_RAM[nBank], pBank, 0x4000);
			pBank += 0x4000;
		}
	}

	p->m_nTstates = 0;
	Finish (p);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// .Z80

boolean CSnapshot::DecompressZ80 (const u8 *pIn, unsigned nInSize, u8 *pOut, unsigned nOutSize,
				  boolean bEndMarker)
{
	unsigned i = 0, o = 0;
	while (i < nInSize && o < nOutSize)
	{
		if (   bEndMarker && i + 3 < nInSize
		    && pIn[i] == 0x00 && pIn[i + 1] == 0xED && pIn[i + 2] == 0xED && pIn[i + 3] == 0x00)
		{
			break;
		}

		if (i + 3 < nInSize && pIn[i] == 0xED && pIn[i + 1] == 0xED)
		{
			unsigned nCount = pIn[i + 2];
			u8 nByte = pIn[i + 3];
			i += 4;
			while (nCount-- && o < nOutSize)
			{
				pOut[o++] = nByte;
			}
		}
		else
		{
			pOut[o++] = pIn[i++];
		}
	}

	return o == nOutSize;
}

unsigned CSnapshot::CompressZ80 (const u8 *pIn, unsigned nInSize, u8 *pOut, unsigned nOutSize)
{
	unsigned i = 0, o = 0;
	while (i < nInSize)
	{
		u8 nByte = pIn[i];
		unsigned nRun = 1;
		while (i + nRun < nInSize && pIn[i + nRun] == nByte && nRun < 255)
		{
			nRun++;
		}

		if (nRun >= 5 || (nByte == 0xED && nRun >= 2))
		{
			if (o + 4 > nOutSize) return 0;
			pOut[o++] = 0xED;
			pOut[o++] = 0xED;
			pOut[o++] = nRun;
			pOut[o++] = nByte;
			i += nRun;
		}
		else if (nByte == 0xED)
		{
			// tras un ED suelto, el byte siguiente va siempre literal
			if (o + 2 > nOutSize) return 0;
			pOut[o++] = 0xED;
			i++;
			if (i < nInSize)
			{
				pOut[o++] = pIn[i++];
			}
		}
		else
		{
			if (o + 1 > nOutSize) return 0;
			pOut[o++] = nByte;
			i++;
		}
	}

	return o;
}

const char *CSnapshot::LoadZ80 (CSpectrum *p, const u8 *pData, unsigned nSize)
{
	if (nSize < 30)
	{
		return N_("Fichero .Z80 demasiado corto");
	}

	u8 nFlags1 = pData[12] == 0xFF ? 1 : pData[12];
	u16 nPC = Word (pData + 6);
	boolean bV1 = nPC != 0;

	TModel Model = Model48K;
	unsigned nHeaderLen = 30;
	u8 n7FFD = 0, n1FFD = 0;
	unsigned nTstates = 0;
	boolean bHasTstates = FALSE;
	boolean bAY = FALSE;

	if (!bV1)
	{
		if (nSize < 32)
		{
			return N_("Fichero .Z80 truncado");
		}

		unsigned nExtra = Word (pData + 30);
		nHeaderLen = 32 + nExtra;
		if (nSize < nHeaderLen || nExtra < 23)
		{
			return N_("Fichero .Z80 truncado");
		}

		nPC = Word (pData + 32);
		u8 nHardware = pData[34];
		boolean bV2 = nExtra == 23;

		if (bV2)
		{
			if (nHardware <= 1)		Model = Model48K;
			else if (nHardware == 3 || nHardware == 4)	Model = Model128K;
			else return N_("Hardware del .Z80 no soportado");
		}
		else
		{
			switch (nHardware)
			{
			case 0: case 1: case 3:		Model = Model48K;	break;
			case 4: case 5: case 6:		Model = Model128K;	break;
			case 9:				Model = Model128K;	break;	// Pentagon: lo más parecido
			case 12:			Model = ModelPlus2;	break;
			case 7: case 8:			Model = ModelPlus3;	break;
			case 13:			Model = ModelPlus2A;	break;
			default: return N_("Hardware del .Z80 no soportado");
			}
		}

		// bit 7 del byte 37: hardware modificado (48K -> 16K, 128K -> +2)
		if (pData[37] & 0x80)
		{
			if (Model == Model48K)		Model = Model16K;
			else if (Model == Model128K)	Model = ModelPlus2;
		}

		n7FFD = pData[35];
		bAY = !Is48 (Model);

		if (!bV2)
		{
			// contador de t-states (esquema de Fuse)
			unsigned nLow = Word (pData + 55);
			unsigned nHigh = pData[57];
			unsigned nFrame = MachineTiming[Model].nTstatesFrame;
			unsigned nQuarter = nFrame / 4;
			int nT = (int) ((((nHigh + 1) % 4) + 1) * nQuarter) - (int) (nLow + 1);
			if (nT >= 0 && (unsigned) nT < nFrame)
			{
				nTstates = nT;
				bHasTstates = TRUE;
			}

			if (nExtra >= 55)
			{
				n1FFD = pData[86];
			}
		}
	}

	SetModel (p, Model);

	Z80 &CPU = p->m_CPU;
	CPU.setRegA (pData[0]);
	CPU.setFlags (pData[1]);
	CPU.setRegBC (Word (pData + 2));
	CPU.setRegHL (Word (pData + 4));
	CPU.setRegPC (nPC);
	CPU.setRegSP (Word (pData + 8));
	CPU.setRegI (pData[10]);
	CPU.setRegR ((pData[11] & 0x7F) | ((nFlags1 & 0x01) << 7));
	p->m_nPortFE = (nFlags1 >> 1) & 0x07;
	CPU.setRegDE (Word (pData + 13));
	CPU.setRegBCx (Word (pData + 15));
	CPU.setRegDEx (Word (pData + 17));
	CPU.setRegHLx (Word (pData + 19));
	CPU.setRegAFx ((pData[21] << 8) | pData[22]);
	CPU.setRegIY (Word (pData + 23));
	CPU.setRegIX (Word (pData + 25));
	CPU.setIFF1 (pData[27] != 0);
	CPU.setIFF2 (pData[28] != 0);
	p->m_bIssue2 = (pData[29] & 0x04) != 0;		// bit 2: emulación Issue 2
	switch (pData[29] & 3)
	{
	case 0:  CPU.setIM (Z80::IM0); break;
	case 1:  CPU.setIM (Z80::IM1); break;
	default: CPU.setIM (Z80::IM2); break;
	}

	if (bV1)
	{
		const u8 *pMem = pData + 30;
		unsigned nMemSize = nSize - 30;
		static u8 Buffer[0xC000];

		if (nFlags1 & 0x20)
		{
			if (!DecompressZ80 (pMem, nMemSize, Buffer, sizeof Buffer, TRUE))
			{
				return N_("Datos del .Z80 corruptos");
			}
		}
		else
		{
			if (nMemSize < sizeof Buffer)
			{
				return N_("Fichero .Z80 truncado");
			}
			memcpy (Buffer, pMem, sizeof Buffer);
		}

		memcpy (p->m_RAM[5], Buffer, 0x4000);
		memcpy (p->m_RAM[2], Buffer + 0x4000, 0x4000);
		memcpy (p->m_RAM[0], Buffer + 0x8000, 0x4000);
	}
	else
	{
		unsigned nPos = nHeaderLen;
		while (nPos + 3 <= nSize)
		{
			unsigned nLen = Word (pData + nPos);
			u8 nPage = pData[nPos + 2];
			nPos += 3;

			boolean bCompressed = nLen != 0xFFFF;
			if (!bCompressed)
			{
				nLen = 0x4000;
			}
			if (nPos + nLen > nSize)
			{
				return N_("Fichero .Z80 truncado");
			}

			int nBank = -1;
			if (Is48 (Model))
			{
				if (nPage == 4)		nBank = 2;
				else if (nPage == 5)	nBank = 0;
				else if (nPage == 8)	nBank = 5;
			}
			else if (nPage >= 3 && nPage <= 10)
			{
				nBank = nPage - 3;
			}

			if (nBank >= 0)
			{
				if (bCompressed)
				{
					if (!DecompressZ80 (pData + nPos, nLen, p->m_RAM[nBank], 0x4000, FALSE))
					{
						return N_("Datos del .Z80 corruptos");
					}
				}
				else
				{
					memcpy (p->m_RAM[nBank], pData + nPos, 0x4000);
				}
			}

			nPos += nLen;
		}

		if (!Is48 (Model))
		{
			p->m_nPort7FFD = n7FFD;
			p->m_nPort1FFD = MachineTiming[Model].CodeModel == CodeModelPlus3 ? n1FFD : 0;
		}

		if (bAY && nHeaderLen >= 55)
		{
			for (unsigned nReg = 0; nReg < 16; nReg++)
			{
				p->m_AY.SetAddressLatch (nReg);
				p->m_AY.WriteRegister (pData[39 + nReg]);
			}
			p->m_AY.SetAddressLatch (pData[38]);
		}
	}

	p->m_nTstates = bHasTstates ? nTstates : 0;
	Finish (p);

	return 0;
}

unsigned CSnapshot::SaveZ80 (CSpectrum *p, u8 *pBuffer, unsigned nBufferSize)
{
	const unsigned nExtra = MachineTiming[p->m_Model].CodeModel == CodeModelPlus3 ? 55 : 54;
	const unsigned nHeaderLen = 32 + nExtra;
	if (nBufferSize < nHeaderLen)
	{
		return 0;
	}

	memset (pBuffer, 0, nHeaderLen);
	Z80 &CPU = p->m_CPU;

	pBuffer[0] = CPU.getRegA ();
	pBuffer[1] = CPU.getFlags ();
	PutWord (pBuffer + 2, CPU.getRegBC ());
	PutWord (pBuffer + 4, CPU.getRegHL ());
	PutWord (pBuffer + 6, 0);			// PC = 0 -> versión 2/3
	PutWord (pBuffer + 8, CPU.getRegSP ());
	pBuffer[10] = CPU.getRegI ();
	pBuffer[11] = CPU.getRegR () & 0x7F;
	pBuffer[12] = ((CPU.getRegR () >> 7) & 0x01) | ((p->m_nPortFE & 0x07) << 1);
	PutWord (pBuffer + 13, CPU.getRegDE ());
	PutWord (pBuffer + 15, CPU.getRegBCx ());
	PutWord (pBuffer + 17, CPU.getRegDEx ());
	PutWord (pBuffer + 19, CPU.getRegHLx ());
	pBuffer[21] = CPU.getRegAFx () >> 8;
	pBuffer[22] = CPU.getRegAFx () & 0xFF;
	PutWord (pBuffer + 23, CPU.getRegIY ());
	PutWord (pBuffer + 25, CPU.getRegIX ());
	pBuffer[27] = CPU.isIFF1 () ? 1 : 0;
	pBuffer[28] = CPU.isIFF2 () ? 1 : 0;
	pBuffer[29] = (CPU.getIM () == Z80::IM0 ? 0 : (CPU.getIM () == Z80::IM1 ? 1 : 2))
		      | (p->m_bIssue2 ? 0x04 : 0);

	PutWord (pBuffer + 30, nExtra);
	PutWord (pBuffer + 32, CPU.getRegPC ());
	switch (p->m_Model)
	{
	case Model48K:
	case Model16K:		pBuffer[34] = 0;	break;
	case Model128K:		pBuffer[34] = 4;	break;
	case ModelPlus2:	pBuffer[34] = 12;	break;
	case ModelPlus2A:	pBuffer[34] = 13;	break;
	case ModelPlus3:
	case ModelPlus3E:	pBuffer[34] = 7;	break;
	default:					break;
	}
	pBuffer[35] = Is48 (p->m_Model) ? 0 : p->m_nPort7FFD;
	pBuffer[37] = 0x03 | (p->m_pTiming->bHasAY ? 0x04 : 0)
		      | (p->m_Model == Model16K ? 0x80 : 0);	// 16K: 48K con hardware modificado
	pBuffer[38] = p->m_AY.GetAddressLatch ();
	for (unsigned nReg = 0; nReg < 16; nReg++)
	{
		pBuffer[39 + nReg] = p->m_AY.GetRegister (nReg);
	}

	// contador de t-states (esquema de Fuse)
	unsigned nQuarter = p->m_pTiming->nTstatesFrame / 4;
	unsigned nT = p->m_nTstates >= 0 ? (unsigned) p->m_nTstates : 0;
	if (nT >= (unsigned) p->m_pTiming->nTstatesFrame)
	{
		nT %= p->m_pTiming->nTstatesFrame;
	}
	PutWord (pBuffer + 55, nQuarter - (nT % nQuarter) - 1);
	pBuffer[57] = (nT / nQuarter + 3) % 4;
	pBuffer[61] = 0xFF;	// 0x0000-0x1FFF es ROM
	pBuffer[62] = 0xFF;	// 0x2000-0x3FFF es ROM
	if (nExtra == 55)
	{
		pBuffer[86] = p->m_nPort1FFD;
	}

	unsigned nPos = nHeaderLen;

	static const u8 Pages48K[3][2] = {{4, 2}, {5, 0}, {8, 5}};	// página .Z80, banco
	unsigned nPages = p->m_Model == Model16K ? 1 : (Is48 (p->m_Model) ? 3 : 8);

	for (unsigned i = 0; i < nPages; i++)
	{
		u8 nPage, nBank;
		if (p->m_Model == Model16K)
		{
			nPage = 8;		// solo 0x4000-0x7FFF
			nBank = 5;
		}
		else if (Is48 (p->m_Model))
		{
			nPage = Pages48K[i][0];
			nBank = Pages48K[i][1];
		}
		else
		{
			nPage = i + 3;
			nBank = i;
		}

		if (nPos + 3 + 0x4000 > nBufferSize)
		{
			return 0;
		}

		unsigned nLen = CompressZ80 (p->m_RAM[nBank], 0x4000, pBuffer + nPos + 3, 0x4000 - 1);
		if (nLen == 0)
		{
			// no merece la pena comprimir
			PutWord (pBuffer + nPos, 0xFFFF);
			memcpy (pBuffer + nPos + 3, p->m_RAM[nBank], 0x4000);
			nLen = 0x4000;
		}
		else
		{
			PutWord (pBuffer + nPos, nLen);
		}
		pBuffer[nPos + 2] = nPage;

		nPos += 3 + nLen;
	}

	return nPos;
}

////////////////////////////////////////////////////////////////////////////////
// .SZX

const char *CSnapshot::LoadSZX (CSpectrum *p, const u8 *pData, unsigned nSize)
{
	if (nSize < 8 || memcmp (pData, "ZXST", 4) != 0)
	{
		return N_("Fichero .SZX no válido");
	}

	TModel Model;
	switch (pData[6])
	{
	case 0:			Model = Model16K;	break;
	case 1:			Model = Model48K;	break;
	case 2:			Model = Model128K;	break;
	case 3:			Model = ModelPlus2;	break;
	case 4:			Model = ModelPlus2A;	break;
	case 5:			Model = ModelPlus3;	break;
	case 7:			Model = Model128K;	break;	// Pentagon: lo más parecido
	default:		return N_("Modelo del .SZX no soportado");
	}

	SetModel (p, Model);

	static u8 PageBuffer[0x4000];
	Z80 &CPU = p->m_CPU;
	boolean bHaveZ80 = FALSE;

	unsigned nPos = 8;
	while (nPos + 8 <= nSize)
	{
		const u8 *pID = pData + nPos;
		u32 nLen = DWord (pData + nPos + 4);
		const u8 *pBlock = pData + nPos + 8;
		nPos += 8;
		if (nPos + nLen > nSize)
		{
			return N_("Fichero .SZX truncado");
		}

		if (memcmp (pID, "Z80R", 4) == 0 && nLen >= 37)
		{
			CPU.setRegAF (Word (pBlock + 0));
			CPU.setRegBC (Word (pBlock + 2));
			CPU.setRegDE (Word (pBlock + 4));
			CPU.setRegHL (Word (pBlock + 6));
			CPU.setRegAFx (Word (pBlock + 8));
			CPU.setRegBCx (Word (pBlock + 10));
			CPU.setRegDEx (Word (pBlock + 12));
			CPU.setRegHLx (Word (pBlock + 14));
			CPU.setRegIX (Word (pBlock + 16));
			CPU.setRegIY (Word (pBlock + 18));
			CPU.setRegSP (Word (pBlock + 20));
			CPU.setRegPC (Word (pBlock + 22));
			CPU.setRegI (pBlock[24]);
			CPU.setRegR (pBlock[25]);
			CPU.setIFF1 (pBlock[26] != 0);
			CPU.setIFF2 (pBlock[27] != 0);
			switch (pBlock[28] & 3)
			{
			case 0:  CPU.setIM (Z80::IM0); break;
			case 1:  CPU.setIM (Z80::IM1); break;
			default: CPU.setIM (Z80::IM2); break;
			}
			u32 nCycles = DWord (pBlock + 29);
			p->m_nTstates = nCycles < (u32) p->m_pTiming->nTstatesFrame ? nCycles : 0;
			if (nLen >= 37)
			{
				CPU.setMemPtr (Word (pBlock + 35));
			}
			bHaveZ80 = TRUE;
		}
		else if (memcmp (pID, "SPCR", 4) == 0 && nLen >= 4)
		{
			p->m_nPortFE = (pBlock[3] & 0x18) | (pBlock[0] & 0x07);
			if (!Is48 (Model))
			{
				p->m_nPort7FFD = pBlock[1];
			}
			if (MachineTiming[Model].CodeModel == CodeModelPlus3)
			{
				p->m_nPort1FFD = pBlock[2];
			}
		}
		else if (memcmp (pID, "RAMP", 4) == 0 && nLen >= 3)
		{
			u16 nFlags = Word (pBlock);
			u8 nPage = pBlock[2];
			const u8 *pPageData = pBlock + 3;
			unsigned nDataLen = nLen - 3;

			if (nPage < 8)
			{
				if (nFlags & 0x01)
				{
					if (ZlibInflate (pPageData, nDataLen, PageBuffer, sizeof PageBuffer) != 0x4000)
					{
						return N_("Datos del .SZX corruptos");
					}
					memcpy (p->m_RAM[nPage], PageBuffer, 0x4000);
				}
				else if (nDataLen >= 0x4000)
				{
					memcpy (p->m_RAM[nPage], pPageData, 0x4000);
				}
			}
		}
		else if (memcmp (pID, "AY\0\0", 4) == 0 && nLen >= 18 && p->m_pTiming->bHasAY)
		{
			for (unsigned nReg = 0; nReg < 16; nReg++)
			{
				p->m_AY.SetAddressLatch (nReg);
				p->m_AY.WriteRegister (pBlock[2 + nReg]);
			}
			p->m_AY.SetAddressLatch (pBlock[1]);
		}
		else if (memcmp (pID, "PLTT", 4) == 0 && nLen >= 66 && p->m_bULAplusEnabled)
		{
			// ULAplus: flags (bit 0 = activa), registro actual y 64 colores
			for (unsigned i = 0; i < 64; i++)
			{
				p->SetULAplusColor (i, pBlock[2 + i]);
			}
			p->m_bULAplusActive = (pBlock[0] & 0x01) != 0;
			p->m_nULAplusRegister = pBlock[1] & 0x7F;
			if (p->m_nULAplusRegister > 0x40)
			{
				p->m_nULAplusRegister = 0x40;
			}
		}
		else if (memcmp (pID, "IF2R", 4) == 0 && nLen >= 4)
		{
			// cartucho del Interface 2: tamaño comprimido (4 bytes) + datos zlib
			static u8 CartBuffer[0x4000];
			u32 nCompressed = DWord (pBlock);
			if (   nCompressed <= nLen - 4
			    && ZlibInflate (pBlock + 4, nCompressed, CartBuffer, sizeof CartBuffer) == 0x4000)
			{
				memcpy (p->m_IF2ROM, CartBuffer, 0x4000);
				p->m_bIF2Inserted = MachineTiming[Model].CodeModel != CodeModelPlus3;
			}
		}
		// el resto de bloques (teclado, joystick, etc.) se ignoran

		nPos += nLen;
	}

	if (!bHaveZ80)
	{
		return N_("El .SZX no tiene registros del Z80");
	}

	Finish (p);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Grabación .SNA

unsigned CSnapshot::SaveSNA (CSpectrum *p, u8 *pBuffer, unsigned nBufferSize)
{
	Z80 &CPU = p->m_CPU;
	boolean b48 = Is48 (p->m_Model);
	unsigned nPaged = p->m_nPort7FFD & 0x07;

	unsigned nSize = 27 + 0xC000;
	if (!b48)
	{
		unsigned nRemaining = 0;
		for (unsigned nBank = 0; nBank < 8; nBank++)
		{
			if (nBank != 2 && nBank != 5 && nBank != nPaged)
			{
				nRemaining++;
			}
		}
		nSize += 4 + nRemaining * 0x4000;
	}
	if (nSize > nBufferSize)
	{
		return 0;
	}

	u16 nSP = CPU.getRegSP ();
	pBuffer[0] = CPU.getRegI ();
	PutWord (pBuffer + 1, CPU.getRegHLx ());
	PutWord (pBuffer + 3, CPU.getRegDEx ());
	PutWord (pBuffer + 5, CPU.getRegBCx ());
	PutWord (pBuffer + 7, CPU.getRegAFx ());
	PutWord (pBuffer + 9, CPU.getRegHL ());
	PutWord (pBuffer + 11, CPU.getRegDE ());
	PutWord (pBuffer + 13, CPU.getRegBC ());
	PutWord (pBuffer + 15, CPU.getRegIY ());
	PutWord (pBuffer + 17, CPU.getRegIX ());
	pBuffer[19] = CPU.isIFF2 () ? 0x04 : 0x00;
	pBuffer[20] = CPU.getRegR ();
	PutWord (pBuffer + 21, CPU.getRegAF ());
	pBuffer[25] = CPU.getIM () == Z80::IM0 ? 0 : (CPU.getIM () == Z80::IM1 ? 1 : 2);
	pBuffer[26] = p->m_nPortFE & 0x07;

	u8 *pRAM = pBuffer + 27;
	memcpy (pRAM, p->m_RAM[5], 0x4000);
	memcpy (pRAM + 0x4000, p->m_RAM[2], 0x4000);
	memcpy (pRAM + 0x8000, p->m_RAM[b48 ? 0 : nPaged], 0x4000);

	if (b48)
	{
		// el PC va a la pila (en la copia; la máquina no se toca)
		u16 nPC = CPU.getRegPC ();
		nSP -= 2;
		if (nSP >= 0x4000)
		{
			pRAM[nSP - 0x4000] = nPC & 0xFF;
		}
		if ((u16) (nSP + 1) >= 0x4000)
		{
			pRAM[(u16) (nSP + 1) - 0x4000] = nPC >> 8;
		}
		PutWord (pBuffer + 23, nSP);
	}
	else
	{
		PutWord (pBuffer + 23, nSP);

		u8 *pExtra = pBuffer + 27 + 0xC000;
		PutWord (pExtra, CPU.getRegPC ());
		pExtra[2] = p->m_nPort7FFD;
		pExtra[3] = 0;			// TR-DOS no paginada

		u8 *pBank = pExtra + 4;
		for (unsigned nBank = 0; nBank < 8; nBank++)
		{
			if (nBank != 2 && nBank != 5 && nBank != nPaged)
			{
				memcpy (pBank, p->m_RAM[nBank], 0x4000);
				pBank += 0x4000;
			}
		}
	}

	return nSize;
}

////////////////////////////////////////////////////////////////////////////////
// Grabación .SZX (sin comprimir)

unsigned CSnapshot::SaveSZX (CSpectrum *p, u8 *pBuffer, unsigned nBufferSize)
{
	Z80 &CPU = p->m_CPU;
	unsigned nPos = 0;

	#define NEED(n)	if (nPos + (n) > nBufferSize) return 0

	NEED (8);
	memcpy (pBuffer, "ZXST", 4);
	pBuffer[4] = 1;		// versión 1.4
	pBuffer[5] = 4;
	switch (p->m_Model)
	{
	case Model16K:		pBuffer[6] = 0;	break;
	case Model48K:		pBuffer[6] = 1;	break;
	case Model128K:		pBuffer[6] = 2;	break;
	case ModelPlus2:	pBuffer[6] = 3;	break;
	case ModelPlus2A:	pBuffer[6] = 4;	break;
	case ModelPlus3:
	case ModelPlus3E:	pBuffer[6] = 5;	break;
	default:		pBuffer[6] = 1;	break;
	}
	pBuffer[7] = 0;
	nPos = 8;

	// Creador
	NEED (8 + 36);
	memcpy (pBuffer + nPos, "CRTR", 4);
	PutDWord (pBuffer + nPos + 4, 36);
	memset (pBuffer + nPos + 8, 0, 36);
	strcpy ((char *) pBuffer + nPos + 8, "ZXCircle");
	nPos += 8 + 36;

	// Registros del Z80
	NEED (8 + 37);
	memcpy (pBuffer + nPos, "Z80R", 4);
	PutDWord (pBuffer + nPos + 4, 37);
	u8 *q = pBuffer + nPos + 8;
	PutWord (q + 0, CPU.getRegAF ());
	PutWord (q + 2, CPU.getRegBC ());
	PutWord (q + 4, CPU.getRegDE ());
	PutWord (q + 6, CPU.getRegHL ());
	PutWord (q + 8, CPU.getRegAFx ());
	PutWord (q + 10, CPU.getRegBCx ());
	PutWord (q + 12, CPU.getRegDEx ());
	PutWord (q + 14, CPU.getRegHLx ());
	PutWord (q + 16, CPU.getRegIX ());
	PutWord (q + 18, CPU.getRegIY ());
	PutWord (q + 20, CPU.getRegSP ());
	PutWord (q + 22, CPU.getRegPC ());
	q[24] = CPU.getRegI ();
	q[25] = CPU.getRegR ();
	q[26] = CPU.isIFF1 () ? 1 : 0;
	q[27] = CPU.isIFF2 () ? 1 : 0;
	q[28] = CPU.getIM () == Z80::IM0 ? 0 : (CPU.getIM () == Z80::IM1 ? 1 : 2);
	PutDWord (q + 29, p->m_nTstates >= 0 ? p->m_nTstates : 0);
	q[33] = p->m_pTiming->nLengthINT;
	q[34] = (CPU.isPendingEI () ? 0x01 : 0) | (CPU.isHalted () ? 0x02 : 0);
	PutWord (q + 35, CPU.getMemPtr ());
	nPos += 8 + 37;

	// Puertos
	NEED (8 + 8);
	memcpy (pBuffer + nPos, "SPCR", 4);
	PutDWord (pBuffer + nPos + 4, 8);
	q = pBuffer + nPos + 8;
	memset (q, 0, 8);
	q[0] = p->m_nPortFE & 0x07;
	q[1] = Is48 (p->m_Model) ? 0 : p->m_nPort7FFD;
	q[2] = MachineTiming[p->m_Model].CodeModel == CodeModelPlus3 ? p->m_nPort1FFD : 0;
	q[3] = p->m_nPortFE;
	nPos += 8 + 8;

	// AY
	if (p->m_pTiming->bHasAY)
	{
		NEED (8 + 18);
		memcpy (pBuffer + nPos, "AY\0\0", 4);
		PutDWord (pBuffer + nPos + 4, 18);
		q = pBuffer + nPos + 8;
		q[0] = 0;
		q[1] = p->m_AY.GetAddressLatch ();
		for (unsigned nReg = 0; nReg < 16; nReg++)
		{
			q[2 + nReg] = p->m_AY.GetRegister (nReg);
		}
		nPos += 8 + 18;
	}

	// ULAplus
	if (p->m_bULAplusEnabled)
	{
		NEED (8 + 66);
		memcpy (pBuffer + nPos, "PLTT", 4);
		PutDWord (pBuffer + nPos + 4, 66);
		q = pBuffer + nPos + 8;
		q[0] = p->m_bULAplusActive ? 1 : 0;
		q[1] = p->m_nULAplusRegister;
		memcpy (q + 2, p->m_ULAplusPalette, 64);
		nPos += 8 + 66;
	}

	// Páginas de RAM, sin comprimir
	static const u8 Pages48K[] = {5, 2, 0};
	static const u8 Pages16K[] = {5};
	static const u8 Pages128K[] = {0, 1, 2, 3, 4, 5, 6, 7};
	const u8 *pPages;
	unsigned nPages;
	if (p->m_Model == Model16K)	{ pPages = Pages16K;  nPages = 1; }
	else if (Is48 (p->m_Model))	{ pPages = Pages48K;  nPages = 3; }
	else				{ pPages = Pages128K; nPages = 8; }

	for (unsigned i = 0; i < nPages; i++)
	{
		NEED (8 + 3 + 0x4000);
		memcpy (pBuffer + nPos, "RAMP", 4);
		PutDWord (pBuffer + nPos + 4, 3 + 0x4000);
		q = pBuffer + nPos + 8;
		PutWord (q, 0);			// sin comprimir
		q[2] = pPages[i];
		memcpy (q + 3, p->m_RAM[pPages[i]], 0x4000);
		nPos += 8 + 3 + 0x4000;
	}

	#undef NEED

	return nPos;
}
