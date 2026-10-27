//
// recreated.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "recreated.h"
#include <circle/util.h>

#define K(row, bit)	(((row) << 3) | (bit))

// Tabla de la capa de juego: carácter de pulsación, carácter de liberación y tecla
struct TRecreatedKey
{
	char chPress;
	char chRelease;
	u8 nPos;	// (fila << 3) | bit en la matriz
};

static const TRecreatedKey KeyTable[40] =
{
	{'a', 'b', K(3,0)}, {'c', 'd', K(3,1)}, {'e', 'f', K(3,2)}, {'g', 'h', K(3,3)},	// 1 2 3 4
	{'i', 'j', K(3,4)}, {'k', 'l', K(4,4)}, {'m', 'n', K(4,3)}, {'o', 'p', K(4,2)},	// 5 6 7 8
	{'q', 'r', K(4,1)}, {'s', 't', K(4,0)},						// 9 0
	{'u', 'v', K(2,0)}, {'w', 'x', K(2,1)}, {'y', 'z', K(2,2)}, {'A', 'B', K(2,3)},	// Q W E R
	{'C', 'D', K(2,4)}, {'E', 'F', K(5,4)}, {'G', 'H', K(5,3)}, {'I', 'J', K(5,2)},	// T Y U I
	{'K', 'L', K(5,1)}, {'M', 'N', K(5,0)},						// O P
	{'O', 'P', K(1,0)}, {'Q', 'R', K(1,1)}, {'S', 'T', K(1,2)}, {'U', 'V', K(1,3)},	// A S D F
	{'W', 'X', K(1,4)}, {'Y', 'Z', K(6,4)}, {'0', '1', K(6,3)}, {'2', '3', K(6,2)},	// G H J K
	{'4', '5', K(6,1)}, {'6', '7', K(6,0)},						// L ENTER
	{'8', '9', K(0,0)}, {'<', '>', K(0,1)}, {'-', '=', K(0,2)}, {'[', ']', K(0,3)},	// CAPS Z X C
	{';', ':', K(0,4)}, {',', '.', K(7,4)}, {'/', '?', K(7,3)}, {'{', '}', K(7,2)},	// V B N M
	{'!', '$', K(7,1)}, {'%', '^', K(7,0)},						// SYMBOL SPACE
};

#define DETECT_MAX_HOLD_US	30000	// el Recreated suelta la tecla en menos de 30 ms
#define DETECT_HITS		3	// pares pulsar/soltar completos (p. ej. 'a' y luego 'b')
#define NORMAL_HOLDS		8	// pulsaciones normales para descartarlo

CRecreatedKeyboard::CRecreatedKeyboard (void)
{
	Reset (RecreatedAuto);
}

void CRecreatedKeyboard::Reset (TRecreatedMode Mode)
{
	m_Mode = Mode;
	m_bActive = Mode == RecreatedForced;
	m_bJustDetected = FALSE;
	for (unsigned i = 0; i < 8; i++)
	{
		m_Matrix[i] = 0;
	}
	memset (m_PrevKeys, 0, sizeof m_PrevKeys);
	m_bNormal = Mode == RecreatedOff;
	m_bPending = FALSE;
	m_nDownTime = 0;
	m_nHits = 0;
	m_nNormalHolds = 0;
	m_bPrevEmpty = TRUE;
	memset (m_TapDown, 0, sizeof m_TapDown);
}

boolean CRecreatedKeyboard::CheckJustDetected (void)
{
	if (m_bJustDetected)
	{
		m_bJustDetected = FALSE;
		return TRUE;
	}

	return FALSE;
}

char CRecreatedKeyboard::HIDToChar (u8 nKey, boolean bShift)
{
	if (nKey >= 0x04 && nKey <= 0x1D)
	{
		return (bShift ? 'A' : 'a') + nKey - 0x04;
	}

	if (nKey >= 0x1E && nKey <= 0x27)
	{
		static const char Normal[] = "1234567890";
		static const char Shifted[] = "!@#$%^&*()";
		return bShift ? Shifted[nKey - 0x1E] : Normal[nKey - 0x1E];
	}

	switch (nKey)
	{
	case 0x2D: return bShift ? '_' : '-';
	case 0x2E: return bShift ? '+' : '=';
	case 0x2F: return bShift ? '{' : '[';
	case 0x30: return bShift ? '}' : ']';
	case 0x33: return bShift ? ':' : ';';
	case 0x36: return bShift ? '<' : ',';
	case 0x37: return bShift ? '>' : '.';
	case 0x38: return bShift ? '?' : '/';
	default:   return 0;
	}
}

static const TRecreatedKey *FindChar (char ch, boolean *pPressed)
{
	if (ch == 0)
	{
		return 0;
	}

	for (unsigned i = 0; i < sizeof KeyTable / sizeof KeyTable[0]; i++)
	{
		if (KeyTable[i].chPress == ch)
		{
			*pPressed = TRUE;
			return &KeyTable[i];
		}

		if (KeyTable[i].chRelease == ch)
		{
			*pPressed = FALSE;
			return &KeyTable[i];
		}
	}

	return 0;
}

void CRecreatedKeyboard::Decode (u8 nModifiers, const u8 RawKeys[6])
{
	boolean bShift = (nModifiers & 0x22) != 0;	// Mayúsculas izquierda o derecha

	for (unsigned i = 0; i < 6; i++)
	{
		u8 nKey = RawKeys[i];
		if (nKey < 4)
		{
			continue;
		}

		boolean bNew = TRUE;
		for (unsigned j = 0; j < 6; j++)
		{
			if (m_PrevKeys[j] == nKey)
			{
				bNew = FALSE;
				break;
			}
		}

		if (!bNew)
		{
			continue;
		}

		boolean bPressed;
		const TRecreatedKey *pEntry = FindChar (HIDToChar (nKey, bShift), &bPressed);
		if (pEntry == 0)
		{
			continue;
		}

		u8 nRow = pEntry->nPos >> 3;
		u8 nMask = 1 << (pEntry->nPos & 7);
		if (bPressed)
		{
			m_Matrix[nRow] |= nMask;
		}
		else
		{
			m_Matrix[nRow] &= ~nMask;
		}
	}

	memcpy (m_PrevKeys, RawKeys, 6);
}

void CRecreatedKeyboard::Emit (TOutput *pOut, u8 nModifiers, const u8 RawKeys[6])
{
	if (pOut->nCount < 2)
	{
		pOut->Modifiers[pOut->nCount] = nModifiers;
		memcpy (pOut->Keys[pOut->nCount], RawKeys, 6);
		pOut->nCount++;
	}
}

void CRecreatedKeyboard::FlushPending (TOutput *pOut, boolean bNormalHold)
{
	if (!m_bPending)
	{
		return;
	}

	m_bPending = FALSE;
	Emit (pOut, m_nPendingModifiers, m_PendingKeys);

	if (bNormalHold)
	{
		m_nHits = 0;
		memset (m_TapDown, 0, sizeof m_TapDown);
		if (++m_nNormalHolds >= NORMAL_HOLDS)
		{
			m_bNormal = TRUE;	// teclado normal: no se vuelve a retener nada
		}
	}
}

void CRecreatedKeyboard::CheckTimeout (unsigned nNowUs, TOutput *pOut)
{
	pOut->nCount = 0;

	if (m_bPending && nNowUs - m_nDownTime > DETECT_MAX_HOLD_US)
	{
		FlushPending (pOut, TRUE);
	}
}

void CRecreatedKeyboard::Detect (u8 nModifiers, const u8 RawKeys[6], unsigned nNowUs, TOutput *pOut)
{
	unsigned nKeys = 0;
	u8 nKey = 0;
	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] >= 4)
		{
			nKeys++;
			nKey = RawKeys[i];
		}
	}

	boolean bEmpty = nKeys == 0;
	boolean bPrevEmpty = m_bPrevEmpty;
	m_bPrevEmpty = bEmpty;

	if (m_bPending)
	{
		if (bEmpty && nNowUs - m_nDownTime <= DETECT_MAX_HOLD_US)
		{
			// pulsación y liberación casi instantáneas: patrón Recreated
			m_bPending = FALSE;
			m_nNormalHolds = 0;

			// Solo cuenta un par completo: el carácter de "soltar" de una tecla
			// cuya "pulsación" ya se vio. Un teclado normal tecleando deprisa
			// casi nunca produce esas parejas (a-b, c-d, 8-9...).
			boolean bPressed;
			boolean bShift = (m_nPendingModifiers & 0x22) != 0;
			const TRecreatedKey *pEntry = FindChar (HIDToChar (m_PendingKeys[0], bShift), &bPressed);
			unsigned nIndex = pEntry != 0 ? pEntry - KeyTable : 0;
			boolean bPair = FALSE;
			if (pEntry != 0)
			{
				if (bPressed)
				{
					m_TapDown[nIndex] = TRUE;
				}
				else if (m_TapDown[nIndex])
				{
					m_TapDown[nIndex] = FALSE;
					bPair = TRUE;
				}
			}

			if (bPair && ++m_nHits >= DETECT_HITS)
			{
				m_bActive = TRUE;
				m_bJustDetected = TRUE;
				for (unsigned i = 0; i < 8; i++)
				{
					m_Matrix[i] = 0;
				}
				memset (m_PrevKeys, 0, sizeof m_PrevKeys);
			}
			return;		// no llega al Spectrum
		}

		if (   nKeys == 1
		    && nKey == m_PendingKeys[0]
		    && nModifiers == m_nPendingModifiers
		    && nNowUs - m_nDownTime <= DETECT_MAX_HOLD_US)
		{
			return;		// mismo informe repetido: se sigue esperando
		}

		// cualquier otra cosa: era una pulsación normal
		FlushPending (pOut, TRUE);
	}

	if (nKeys == 1 && bPrevEmpty)
	{
		boolean bPressed;
		boolean bShift = (nModifiers & 0x22) != 0;
		if (FindChar (HIDToChar (nKey, bShift), &bPressed) != 0)
		{
			// candidata: se retiene hasta ver cuánto dura
			m_bPending = TRUE;
			m_nPendingModifiers = nModifiers;
			memset (m_PendingKeys, 0, sizeof m_PendingKeys);
			m_PendingKeys[0] = nKey;
			m_nDownTime = nNowUs;
			return;
		}
	}

	if (!bEmpty)
	{
		m_nHits = 0;
	}

	Emit (pOut, nModifiers, RawKeys);
}

void CRecreatedKeyboard::ProcessReport (u8 nModifiers, const u8 RawKeys[6], unsigned nNowUs,
					TOutput *pOut)
{
	pOut->nCount = 0;

	if (m_bActive)
	{
		Decode (nModifiers, RawKeys);
		return;
	}

	if (m_bNormal)
	{
		Emit (pOut, nModifiers, RawKeys);
		return;
	}

	Detect (nModifiers, RawKeys, nNowUs, pOut);
}
