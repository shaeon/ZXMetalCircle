//
// inflate.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "inflate.h"

#define MAXBITS		15
#define MAXLCODES	286
#define MAXDCODES	30
#define FIXLCODES	288

namespace
{

struct TState
{
	const u8 *pIn;
	unsigned nInLength;
	unsigned nInPos;
	u32 nBitBuf;
	unsigned nBitCount;

	u8 *pOut;
	unsigned nOutLength;
	unsigned nOutPos;

	bool bError;
};

struct THuffman
{
	short Count[MAXBITS + 1];
	short Symbol[FIXLCODES];
};

int Bits (TState *s, unsigned n)
{
	u32 nValue = s->nBitBuf;
	while (s->nBitCount < n)
	{
		if (s->nInPos >= s->nInLength)
		{
			s->bError = true;
			return 0;
		}
		nValue |= (u32) s->pIn[s->nInPos++] << s->nBitCount;
		s->nBitCount += 8;
	}

	s->nBitBuf = nValue >> n;
	s->nBitCount -= n;

	return (int) (nValue & ((1UL << n) - 1));
}

bool Stored (TState *s)
{
	s->nBitBuf = 0;
	s->nBitCount = 0;

	if (s->nInPos + 4 > s->nInLength)
	{
		return false;
	}

	unsigned nLen = s->pIn[s->nInPos] | (s->pIn[s->nInPos + 1] << 8);
	unsigned nNLen = s->pIn[s->nInPos + 2] | (s->pIn[s->nInPos + 3] << 8);
	s->nInPos += 4;

	if (nLen != (~nNLen & 0xFFFF))
	{
		return false;
	}

	if (   s->nInPos + nLen > s->nInLength
	    || s->nOutPos + nLen > s->nOutLength)
	{
		return false;
	}

	while (nLen--)
	{
		s->pOut[s->nOutPos++] = s->pIn[s->nInPos++];
	}

	return true;
}

int Decode (TState *s, const THuffman *h)
{
	int nCode = 0, nFirst = 0, nIndex = 0;

	for (int nLen = 1; nLen <= MAXBITS; nLen++)
	{
		nCode |= Bits (s, 1);
		if (s->bError)
		{
			return -1;
		}

		int nCount = h->Count[nLen];
		if (nCode - nCount < nFirst)
		{
			return h->Symbol[nIndex + (nCode - nFirst)];
		}

		nIndex += nCount;
		nFirst += nCount;
		nFirst <<= 1;
		nCode <<= 1;
	}

	return -1;
}

// Devuelve 0 si el código está completo, >0 si está incompleto, <0 si sobra
int Construct (THuffman *h, const short *pLength, int n)
{
	for (int nLen = 0; nLen <= MAXBITS; nLen++)
	{
		h->Count[nLen] = 0;
	}

	for (int nSymbol = 0; nSymbol < n; nSymbol++)
	{
		h->Count[pLength[nSymbol]]++;
	}

	if (h->Count[0] == n)
	{
		return 0;
	}

	int nLeft = 1;
	for (int nLen = 1; nLen <= MAXBITS; nLen++)
	{
		nLeft <<= 1;
		nLeft -= h->Count[nLen];
		if (nLeft < 0)
		{
			return nLeft;
		}
	}

	short Offs[MAXBITS + 1];
	Offs[1] = 0;
	for (int nLen = 1; nLen < MAXBITS; nLen++)
	{
		Offs[nLen + 1] = Offs[nLen] + h->Count[nLen];
	}

	for (int nSymbol = 0; nSymbol < n; nSymbol++)
	{
		if (pLength[nSymbol] != 0)
		{
			h->Symbol[Offs[pLength[nSymbol]]++] = nSymbol;
		}
	}

	return nLeft;
}

bool Codes (TState *s, const THuffman *pLenCode, const THuffman *pDistCode)
{
	static const short LBase[29] =
	{
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
	};
	static const short LExt[29] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
	};
	static const short DBase[30] =
	{
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
		8193, 12289, 16385, 24577
	};
	static const short DExt[30] =
	{
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
	};

	for (;;)
	{
		int nSymbol = Decode (s, pLenCode);
		if (nSymbol < 0)
		{
			return false;
		}

		if (nSymbol < 256)
		{
			if (s->nOutPos >= s->nOutLength)
			{
				return false;
			}
			s->pOut[s->nOutPos++] = (u8) nSymbol;
		}
		else if (nSymbol == 256)
		{
			return true;
		}
		else
		{
			nSymbol -= 257;
			if (nSymbol >= 29)
			{
				return false;
			}
			unsigned nLen = LBase[nSymbol] + Bits (s, LExt[nSymbol]);

			nSymbol = Decode (s, pDistCode);
			if (nSymbol < 0 || nSymbol >= 30)
			{
				return false;
			}
			unsigned nDist = DBase[nSymbol] + Bits (s, DExt[nSymbol]);

			if (   s->bError
			    || nDist > s->nOutPos
			    || s->nOutPos + nLen > s->nOutLength)
			{
				return false;
			}

			while (nLen--)
			{
				s->pOut[s->nOutPos] = s->pOut[s->nOutPos - nDist];
				s->nOutPos++;
			}
		}
	}
}

bool Fixed (TState *s)
{
	static bool bVirgin = true;
	static THuffman LenCode, DistCode;

	if (bVirgin)
	{
		short Lengths[FIXLCODES];
		int nSymbol = 0;
		for (; nSymbol < 144; nSymbol++) Lengths[nSymbol] = 8;
		for (; nSymbol < 256; nSymbol++) Lengths[nSymbol] = 9;
		for (; nSymbol < 280; nSymbol++) Lengths[nSymbol] = 7;
		for (; nSymbol < FIXLCODES; nSymbol++) Lengths[nSymbol] = 8;
		Construct (&LenCode, Lengths, FIXLCODES);

		for (nSymbol = 0; nSymbol < MAXDCODES; nSymbol++)
		{
			Lengths[nSymbol] = 5;
		}
		Construct (&DistCode, Lengths, MAXDCODES);

		bVirgin = false;
	}

	return Codes (s, &LenCode, &DistCode);
}

bool Dynamic (TState *s)
{
	static const short Order[19] =
	{
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	int nLen = Bits (s, 5) + 257;
	int nDist = Bits (s, 5) + 1;
	int nCode = Bits (s, 4) + 4;
	if (s->bError || nLen > MAXLCODES || nDist > MAXDCODES)
	{
		return false;
	}

	short Lengths[MAXLCODES + MAXDCODES];
	int nIndex;
	for (nIndex = 0; nIndex < nCode; nIndex++)
	{
		Lengths[Order[nIndex]] = Bits (s, 3);
	}
	for (; nIndex < 19; nIndex++)
	{
		Lengths[Order[nIndex]] = 0;
	}

	THuffman LenCode, DistCode;
	if (Construct (&LenCode, Lengths, 19) != 0)
	{
		return false;
	}

	nIndex = 0;
	while (nIndex < nLen + nDist)
	{
		int nSymbol = Decode (s, &LenCode);
		if (nSymbol < 0)
		{
			return false;
		}

		if (nSymbol < 16)
		{
			Lengths[nIndex++] = nSymbol;
		}
		else
		{
			int nRepeatLen = 0;
			int nRepeat;
			if (nSymbol == 16)
			{
				if (nIndex == 0)
				{
					return false;
				}
				nRepeatLen = Lengths[nIndex - 1];
				nRepeat = 3 + Bits (s, 2);
			}
			else if (nSymbol == 17)
			{
				nRepeat = 3 + Bits (s, 3);
			}
			else
			{
				nRepeat = 11 + Bits (s, 7);
			}

			if (nIndex + nRepeat > nLen + nDist)
			{
				return false;
			}
			while (nRepeat--)
			{
				Lengths[nIndex++] = nRepeatLen;
			}
		}
	}

	if (s->bError || Lengths[256] == 0)
	{
		return false;
	}

	int nErr = Construct (&LenCode, Lengths, nLen);
	if (nErr < 0 || (nErr > 0 && nLen - LenCode.Count[0] != 1))
	{
		return false;
	}

	nErr = Construct (&DistCode, Lengths + nLen, nDist);
	if (nErr < 0 || (nErr > 0 && nDist - DistCode.Count[0] != 1))
	{
		return false;
	}

	return Codes (s, &LenCode, &DistCode);
}

}	// namespace

int ZlibInflate (const u8 *pSource, unsigned nSourceLength, u8 *pDest, unsigned nDestLength)
{
	// Cabecera zlib: CMF (método 8 = deflate) y FLG (sin diccionario)
	if (   nSourceLength < 2
	    || (pSource[0] & 0x0F) != 8
	    || ((pSource[0] << 8) | pSource[1]) % 31 != 0
	    || (pSource[1] & 0x20) != 0)
	{
		return -1;
	}

	TState s;
	s.pIn = pSource + 2;
	s.nInLength = nSourceLength - 2;
	s.nInPos = 0;
	s.nBitBuf = 0;
	s.nBitCount = 0;
	s.pOut = pDest;
	s.nOutLength = nDestLength;
	s.nOutPos = 0;
	s.bError = false;

	int nLast;
	do
	{
		nLast = Bits (&s, 1);
		int nType = Bits (&s, 2);
		if (s.bError)
		{
			return -1;
		}

		bool bOK;
		switch (nType)
		{
		case 0:  bOK = Stored (&s);	break;
		case 1:  bOK = Fixed (&s);	break;
		case 2:  bOK = Dynamic (&s);	break;
		default: bOK = false;		break;
		}

		if (!bOK || s.bError)
		{
			return -1;
		}
	}
	while (!nLast);

	return (int) s.nOutPos;
}

// Deflate sin cabecera zlib (ficheros ZIP)
int RawInflate (const u8 *pSource, unsigned nSourceLength, u8 *pDest, unsigned nDestLength)
{
	TState s;
	s.pIn = pSource;
	s.nInLength = nSourceLength;
	s.nInPos = 0;
	s.nBitBuf = 0;
	s.nBitCount = 0;
	s.pOut = pDest;
	s.nOutLength = nDestLength;
	s.nOutPos = 0;
	s.bError = false;

	int nLast;
	do
	{
		nLast = Bits (&s, 1);
		int nType = Bits (&s, 2);
		if (s.bError)
		{
			return -1;
		}

		bool bOK;
		switch (nType)
		{
		case 0:  bOK = Stored (&s);	break;
		case 1:  bOK = Fixed (&s);	break;
		case 2:  bOK = Dynamic (&s);	break;
		default: bOK = false;		break;
		}

		if (!bOK || s.bError)
		{
			return -1;
		}
	}
	while (!nLast);

	return (int) s.nOutPos;
}
