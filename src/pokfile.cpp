//
// pokfile.cpp
//
// Pokes de los ficheros .POK y los escritos a mano (ver pokfile.h)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "pokfile.h"
#include <string.h>

CPokFile::CPokFile (void)
:	m_nTrainers (0),
	m_nPokes (0),
	m_nManual (0)
{
}

void CPokFile::Clear (void)
{
	m_nTrainers = 0;
	m_nPokes = 0;
	m_nManual = 0;
}

CPokFile::TTrainer *CPokFile::Trainer (unsigned nTrainer)
{
	if (nTrainer < m_nTrainers)
	{
		return &m_Trainers[nTrainer];
	}
	nTrainer -= m_nTrainers;
	return nTrainer < m_nManual ? &m_Manual[nTrainer] : 0;
}

const CPokFile::TTrainer *CPokFile::Trainer (unsigned nTrainer) const
{
	return const_cast<CPokFile *> (this)->Trainer (nTrainer);
}

const char *CPokFile::GetName (unsigned nTrainer) const
{
	const TTrainer *pTrainer = Trainer (nTrainer);
	return pTrainer != 0 ? pTrainer->Name : "";
}

boolean CPokFile::IsActive (unsigned nTrainer) const
{
	const TTrainer *pTrainer = Trainer (nTrainer);
	return pTrainer != 0 && pTrainer->bActive;
}

boolean CPokFile::IsManual (unsigned nTrainer) const
{
	return nTrainer >= m_nTrainers && nTrainer < m_nTrainers + m_nManual;
}

boolean CPokFile::NeedsInput (unsigned nTrainer) const
{
	const TTrainer *pTrainer = Trainer (nTrainer);
	for (unsigned i = 0; pTrainer != 0 && i < pTrainer->nCount; i++)
	{
		if (pTrainer->pPokes[i].nValue == POK_ASK)
		{
			return TRUE;
		}
	}
	return FALSE;
}

unsigned CPokFile::GetPokeCount (unsigned nTrainer) const
{
	const TTrainer *pTrainer = Trainer (nTrainer);
	return pTrainer != 0 ? pTrainer->nCount : 0;
}

// Banco y dirección como los quiere CPokMemory
static void Where (unsigned nBank, u16 nAddress, unsigned *pBank, u16 *pAddress)
{
	if (nBank & 0x08)
	{
		*pBank = POK_BANK_CURRENT;
		*pAddress = nAddress;
	}
	else
	{
		*pBank = nBank & 0x07;		// la dirección entera: en los 48K hace falta
		*pAddress = nAddress;
	}
}

boolean CPokFile::Activate (unsigned nTrainer, CPokMemory *pMemory, u8 nInput)
{
	TTrainer *pTrainer = Trainer (nTrainer);
	if (pTrainer == 0 || pMemory == 0)
	{
		return FALSE;
	}

	for (unsigned i = 0; i < pTrainer->nCount; i++)
	{
		TPoke *pPoke = &pTrainer->pPokes[i];
		unsigned nBank;
		u16 nAddress;
		Where (pPoke->nBank, pPoke->nAddress, &nBank, &nAddress);
		if (!pTrainer->bActive)		// si ya estaba, lo de antes sigue guardado
		{
			pPoke->nSaved = pMemory->Read (nBank, nAddress);
		}
		pMemory->Write (nBank, nAddress, pPoke->nValue == POK_ASK ? nInput : (u8) pPoke->nValue);
	}
	pTrainer->bActive = TRUE;
	return TRUE;
}

void CPokFile::Deactivate (unsigned nTrainer, CPokMemory *pMemory)
{
	TTrainer *pTrainer = Trainer (nTrainer);
	if (pTrainer == 0 || pMemory == 0 || !pTrainer->bActive)
	{
		return;
	}

	// al revés, por si un truco toca dos veces la misma dirección
	for (unsigned i = pTrainer->nCount; i-- > 0; )
	{
		const TPoke *pPoke = &pTrainer->pPokes[i];
		unsigned nBank;
		u16 nAddress;
		Where (pPoke->nBank, pPoke->nAddress, &nBank, &nAddress);
		pMemory->Write (nBank, nAddress, pPoke->nOriginal != 0 ? pPoke->nOriginal : pPoke->nSaved);
	}
	pTrainer->bActive = FALSE;
}

void CPokFile::Forget (void)
{
	for (unsigned i = 0; i < GetCount (); i++)
	{
		Trainer (i)->bActive = FALSE;
	}
}

int CPokFile::AddManual (u16 nAddress, u8 nValue)
{
	if (m_nManual >= POK_MAX_MANUAL || nAddress < 0x4000)
	{
		return -1;
	}

	TTrainer *pTrainer = &m_Manual[m_nManual];
	TPoke *pPoke = &m_ManualPokes[m_nManual];
	pPoke->nAddress = nAddress;
	pPoke->nValue = nValue;
	pPoke->nBank = POK_BANK_CURRENT;
	pPoke->nOriginal = 0;
	pPoke->nSaved = 0;

	// "POKE 35136,0" (a mano, sin snprintf: también en el PC y en Circle)
	char *p = pTrainer->Name;
	memcpy (p, "POKE ", 5);
	p += 5;
	char Digits[8];
	unsigned n = 0;
	unsigned nNumber = nAddress;
	do { Digits[n++] = '0' + nNumber % 10; nNumber /= 10; } while (nNumber > 0);
	while (n > 0) { *p++ = Digits[--n]; }
	*p++ = ',';
	nNumber = nValue;
	do { Digits[n++] = '0' + nNumber % 10; nNumber /= 10; } while (nNumber > 0);
	while (n > 0) { *p++ = Digits[--n]; }
	*p = '\0';

	pTrainer->pPokes = pPoke;
	pTrainer->nCount = 1;
	pTrainer->bActive = FALSE;

	return (int) (m_nTrainers + m_nManual++);
}

// Un número decimal sin signo, o hexadecimal con H al final ("8940H");
// pText apunta a la palabra (sin espacios delante); *pEnd, a lo que sigue
static boolean ParseNumber (const char *pText, const char **pEnd, unsigned *pValue)
{
	const char *p = pText;
	while (   (*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')
	       || *p == 'H' || *p == 'h')
	{
		p++;
	}
	unsigned nLength = p - pText;
	if (nLength == 0 || nLength > 8)
	{
		return FALSE;
	}

	boolean bHex = pText[nLength - 1] == 'H' || pText[nLength - 1] == 'h';
	if (bHex)
	{
		nLength--;
		if (nLength == 0)
		{
			return FALSE;
		}
	}

	unsigned nValue = 0;
	for (unsigned i = 0; i < nLength; i++)
	{
		char c = pText[i];
		unsigned nDigit;
		if (c >= '0' && c <= '9')	nDigit = c - '0';
		else if (c >= 'A' && c <= 'F')	nDigit = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')	nDigit = c - 'a' + 10;
		else				return FALSE;	// otra H en medio
		if (!bHex && nDigit > 9)
		{
			return FALSE;
		}
		nValue = nValue * (bHex ? 16 : 10) + nDigit;
	}

	*pEnd = p;
	*pValue = nValue;
	return TRUE;
}

static const char *SkipBlanks (const char *p)
{
	while (*p == ' ' || *p == '\t' || *p == ',')
	{
		p++;
	}
	return p;
}

unsigned CPokFile::ParseManual (const char *pText, u16 *pAddress, u8 *pValue, unsigned nMax)
{
	unsigned nPairs = 0;
	const char *p = SkipBlanks (pText);
	while (*p != '\0')
	{
		unsigned nAddress, nValue;
		if (   nPairs >= nMax
		    || !ParseNumber (p, &p, &nAddress) || nAddress < 0x4000 || nAddress > 0xFFFF)
		{
			return 0;
		}
		p = SkipBlanks (p);
		if (!ParseNumber (p, &p, &nValue) || nValue > 255)
		{
			return 0;
		}
		p = SkipBlanks (p);
		pAddress[nPairs] = (u16) nAddress;
		pValue[nPairs] = (u8) nValue;
		nPairs++;
	}
	return nPairs;
}

boolean CPokFile::ParseValue (const char *pText, u8 *pValue)
{
	const char *p = SkipBlanks (pText);
	unsigned nValue;
	if (!ParseNumber (p, &p, &nValue) || nValue > 255 || *SkipBlanks (p) != '\0')
	{
		return FALSE;
	}
	*pValue = (u8) nValue;
	return TRUE;
}

// Copia el nombre: sin espacios al final y con '?' en lo que no sea UTF-8
// válido (los .POK antiguos pueden traer Latin-1)
static void CopyName (char *pDest, const char *pSrc, unsigned nLength)
{
	while (nLength > 0 && (pSrc[nLength - 1] == ' ' || pSrc[nLength - 1] == '\t'))
	{
		nLength--;
	}

	unsigned nOut = 0;
	for (unsigned i = 0; i < nLength; )
	{
		u8 c = (u8) pSrc[i];
		unsigned nSeq =   c < 0x80 ? 1
				: (c & 0xE0) == 0xC0 ? 2
				: (c & 0xF0) == 0xE0 ? 3
				: (c & 0xF8) == 0xF0 ? 4 : 0;
		boolean bValid = nSeq > 0 && i + nSeq <= nLength && (c >= 0x20 || c == '\t');
		for (unsigned j = 1; bValid && j < nSeq; j++)
		{
			bValid = ((u8) pSrc[i + j] & 0xC0) == 0x80;
		}
		if (!bValid)
		{
			nSeq = 1;
		}
		if (nOut + (bValid ? nSeq : 1) >= POK_NAME_LEN)
		{
			break;
		}
		if (bValid)
		{
			memcpy (pDest + nOut, pSrc + i, nSeq);
			nOut += nSeq;
		}
		else
		{
			pDest[nOut++] = '?';
		}
		i += nSeq;
	}
	pDest[nOut] = '\0';
}

// Recorre el texto. Con pStore == 0 solo comprueba y cuenta; si no, guarda los
// trucos en pStore (que ya se sabe que caben)
boolean CPokFile::ParseText (const char *pText, unsigned nLength, CPokFile *pStore,
			     unsigned *pTrainers, unsigned *pPokes)
{
	unsigned nTrainers = 0;
	unsigned nPokes = 0;
	boolean bOpen = FALSE;		// hay un truco empezado (N) sin su Z
	unsigned nTrainerPokes = 0;	// los pokes del último N
	TTrainer *pTrainer = 0;

	unsigned nPos = 0;
	while (nPos < nLength)
	{
		// una línea, sin el \r\n
		unsigned nStart = nPos;
		while (nPos < nLength && pText[nPos] != '\n')
		{
			nPos++;
		}
		unsigned nEnd = nPos;
		nPos++;
		if (nEnd > nStart && pText[nEnd - 1] == '\r')
		{
			nEnd--;
		}
		if (nEnd == nStart)
		{
			continue;
		}

		const char *pLine = pText + nStart;
		char cType = pLine[0];

		if (cType == 'Y')
		{
			break;
		}

		if (cType == 'N')
		{
			// un truco sin pokes (otro N detrás) no se cuenta: su sitio es
			// para este
			if (nTrainers > 0 && nTrainerPokes == 0)
			{
				nTrainers--;
			}
			if (nTrainers >= POK_MAX_TRAINERS)
			{
				return FALSE;
			}
			if (pStore != 0)
			{
				pTrainer = &pStore->m_Trainers[nTrainers];
				CopyName (pTrainer->Name, pLine + 1, nEnd - nStart - 1);
				pTrainer->pPokes = &pStore->m_Pokes[nPokes];
				pTrainer->nCount = 0;
				pTrainer->bActive = FALSE;
			}
			nTrainers++;
			nTrainerPokes = 0;
			bOpen = TRUE;
			continue;
		}

		if (cType == 'M' || cType == 'Z')
		{
			if (!bOpen)
			{
				return FALSE;		// un poke sin su N delante
			}

			// banco dirección valor original
			char Line[64];
			unsigned nLineLen = nEnd - nStart - 1;
			if (nLineLen >= sizeof Line)
			{
				return FALSE;
			}
			memcpy (Line, pLine + 1, nLineLen);
			Line[nLineLen] = '\0';

			unsigned Field[4];
			const char *p = Line;
			for (unsigned i = 0; i < 4; i++)
			{
				while (*p == ' ' || *p == '\t')
				{
					p++;
				}
				if (*p < '0' || *p > '9')
				{
					return FALSE;
				}
				unsigned nValue = 0;
				while (*p >= '0' && *p <= '9')
				{
					nValue = nValue * 10 + (*p - '0');
					if (nValue > 0xFFFF)
					{
						return FALSE;
					}
					p++;
				}
				Field[i] = nValue;
			}
			while (*p == ' ' || *p == '\t')
			{
				p++;
			}
			if (   *p != '\0' || Field[0] > 255 || Field[2] > POK_ASK || Field[3] > 255
			    || nPokes >= POK_MAX_POKES)
			{
				return FALSE;
			}

			if (pStore != 0)
			{
				TPoke *pPoke = &pStore->m_Pokes[nPokes];
				pPoke->nBank = (u8) Field[0];
				pPoke->nAddress = (u16) Field[1];
				pPoke->nValue = (u16) Field[2];
				pPoke->nOriginal = (u8) Field[3];
				pPoke->nSaved = 0;
				pTrainer->nCount++;
			}
			nPokes++;
			nTrainerPokes++;
			if (cType == 'Z')
			{
				bOpen = FALSE;
			}
			continue;
		}

		// otra cosa: se pasa por alto
	}

	// el último N sin pokes
	if (nTrainers > 0 && nTrainerPokes == 0)
	{
		nTrainers--;
	}

	*pTrainers = nTrainers;
	*pPokes = nPokes;
	return nTrainers > 0;
}

boolean CPokFile::Parse (const char *pText, unsigned nLength)
{
	if (pText == 0)
	{
		return FALSE;
	}

	// primero se comprueba, para no perder los trucos que hay si está mal
	unsigned nTrainers, nPokes;
	if (!ParseText (pText, nLength, 0, &nTrainers, &nPokes))
	{
		return FALSE;
	}

	// los que había se quitan (sin tocar la memoria: es otro .POK)
	m_nTrainers = 0;
	m_nPokes = 0;
	if (!ParseText (pText, nLength, this, &nTrainers, &nPokes))
	{
		return FALSE;
	}
	m_nTrainers = nTrainers;
	m_nPokes = nPokes;
	return TRUE;
}
