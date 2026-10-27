//
// zxkeyboard.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "zxkeyboard.h"
#include <circle/util.h>

// Posición en la matriz: (fila << 3) | bit. 0xFF = sin tecla
#define K(row, bit)	(((row) << 3) | (bit))
#define NK		0xFF

#define ZX_CS		K(0, 0)
#define ZX_SS		K(7, 1)
#define ZX_ENTER	K(6, 0)
#define ZX_SPACE	K(7, 0)

// HID 0x04..0x1D: letras a..z
static const u8 LetterMap[26] =
{
	K(1,0), K(7,4), K(0,3), K(1,2), K(2,2), K(1,3), K(1,4), K(6,4),	// a-h
	K(5,2), K(6,3), K(6,2), K(6,1), K(7,2), K(7,3), K(5,1), K(5,0),	// i-p
	K(2,0), K(2,3), K(1,1), K(2,4), K(5,3), K(0,4), K(2,1), K(0,2),	// q-x
	K(5,4), K(0,1)							// y-z
};

// HID 0x1E..0x27: 1..9, 0
static const u8 DigitMap[10] =
{
	K(3,0), K(3,1), K(3,2), K(3,3), K(3,4), K(4,4), K(4,3), K(4,2), K(4,1), K(4,0)
};

#define ZX_0	K(4,0)
#define ZX_1	K(3,0)
#define ZX_2	K(3,1)
#define ZX_5	K(3,4)
#define ZX_6	K(4,4)
#define ZX_7	K(4,3)
#define ZX_8	K(4,2)
#define ZX_9	K(4,1)
#define ZX_B	K(7,4)
#define ZX_C	K(0,3)
#define ZX_J	K(6,3)
#define ZX_K	K(6,2)
#define ZX_M	K(7,2)
#define ZX_N	K(7,3)
#define ZX_O	K(5,1)
#define ZX_P	K(5,0)
#define ZX_R	K(2,3)
#define ZX_T	K(2,4)
#define ZX_V	K(0,4)
#define ZX_Z	K(0,1)

// Teclas de puntuación de un teclado español -> SYMBOL SHIFT + tecla
struct TSymbolKey
{
	u8 nHID;
	u8 nNormal;	// sin mayúsculas
	u8 nShifted;	// con mayúsculas
};

static const TSymbolKey SymbolKeys[] =
{
	{0x2D, ZX_7, ZX_C},	// ' ?
	{0x30, ZX_K, ZX_B},	// + *
	{0x36, ZX_N, ZX_O},	// , ;
	{0x37, ZX_M, ZX_Z},	// . :
	{0x38, ZX_J, ZX_0},	// - _
	{0x64, ZX_R, ZX_T},	// < >
	{0x54, ZX_V, ZX_V},	// keypad /
	{0x55, ZX_B, ZX_B},	// keypad *
	{0x56, ZX_J, ZX_J},	// keypad -
	{0x57, ZX_K, ZX_K},	// keypad +
	{0x63, ZX_M, ZX_M},	// keypad .
};

CZXKeyboard::CZXKeyboard (void)
:	m_nKeyJoy (0),
	m_JoystickMode (JoystickKempston),
	m_CursorMode (CursorBoth),
	m_bBackspaceAsLeft (FALSE),
	m_bCaptured (FALSE),
	m_nEventIn (0),
	m_nEventOut (0)
{
	ReleaseAll ();
	memset (m_PrevKeys, 0, sizeof m_PrevKeys);
	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		m_PadJoy[i] = 0;
	}

	m_bMousePresent = FALSE;
	m_nMouseX = m_nMouseY = 0;
	m_nMouseWheel = 0x0F;
	m_nMouseButtons = 0;
}

void CZXKeyboard::SetGPIOMatrix (const u8 Matrix[8])
{
	for (unsigned i = 0; i < 8; i++)
	{
		m_GPIOMatrix[i] = Matrix[i];
	}
}

u8 CZXKeyboard::MatrixKeyToHID (unsigned nRow, unsigned nBit)
{
	// 0 = sin equivalente; las letras/dígitos se toman de las tablas de arriba
	static const char Keys[8][5] =
	{
		{0,   'z', 'x', 'c', 'v'},
		{'a', 's', 'd', 'f', 'g'},
		{'q', 'w', 'e', 'r', 't'},
		{'1', '2', '3', '4', '5'},
		{'0', '9', '8', '7', '6'},
		{'p', 'o', 'i', 'u', 'y'},
		{'\n', 'l', 'k', 'j', 'h'},
		{' ', 0,   'm', 'n', 'b'},
	};

	if (nRow >= 8 || nBit >= 5)
	{
		return 0;
	}

	char ch = Keys[nRow][nBit];
	if (ch >= 'a' && ch <= 'z')	return 0x04 + ch - 'a';
	if (ch >= '1' && ch <= '9')	return 0x1E + ch - '1';
	if (ch == '0')			return 0x27;
	if (ch == '\n')			return HID_KEY_ENTER;
	if (ch == ' ')			return HID_KEY_SPACE;

	return 0;
}

void CZXKeyboard::ReleaseAll (void)
{
	for (unsigned i = 0; i < 8; i++)
	{
		m_Matrix[i] = 0;
		m_GPIOMatrix[i] = 0;
	}
	m_nKeyJoy = 0;
}

void CZXKeyboard::SetJoystickMode (TJoystickMode Mode)
{
	m_JoystickMode = Mode;
	ReleaseAll ();
}

static inline void Press (u8 Matrix[8], u8 nPos)
{
	if (nPos != NK)
	{
		Matrix[nPos >> 3] |= 1 << (nPos & 7);
	}
}

u8 CZXKeyboard::GetHeldNavKey (void) const
{
	for (unsigned i = 0; i < 6; i++)
	{
		u8 nKey = m_PrevKeys[i];
		if (   nKey == HID_KEY_UP || nKey == HID_KEY_DOWN || nKey == HID_KEY_LEFT
		    || nKey == HID_KEY_RIGHT || nKey == HID_KEY_PAGEUP || nKey == HID_KEY_PAGEDOWN)
		{
			return nKey;
		}
	}
	return 0;
}

void CZXKeyboard::PushEvent (u8 nKey, u8 nModifiers)
{
	unsigned nNext = (m_nEventIn + 1) % (sizeof m_Events / sizeof m_Events[0]);
	if (nNext == m_nEventOut)
	{
		return;		// cola llena
	}

	m_Events[m_nEventIn].nKey = nKey;
	m_Events[m_nEventIn].nModifiers = nModifiers;
	m_nEventIn = nNext;
}

boolean CZXKeyboard::GetKeyEvent (TKeyEvent *pEvent)
{
	if (m_nEventOut == m_nEventIn)
	{
		return FALSE;
	}

	*pEvent = m_Events[m_nEventOut];
	m_nEventOut = (m_nEventOut + 1) % (sizeof m_Events / sizeof m_Events[0]);

	return TRUE;
}

void CZXKeyboard::UpdateFromReport (u8 nModifiers, const u8 RawKeys[6])
{
	// Eventos para el emulador: teclas recién pulsadas
	for (unsigned i = 0; i < 6; i++)
	{
		u8 nKey = RawKeys[i];
		if (nKey < 4)
		{
			continue;	// 0 = nada, 1-3 = errores
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

		if (   bNew
		    && (   m_bCaptured
			|| (nKey >= HID_KEY_F1 && nKey <= HID_KEY_F12)
			|| nKey == HID_KEY_PAUSE
			|| (nModifiers & (HID_MOD_LALT | HID_MOD_RALT))))
		{
			PushEvent (nKey, nModifiers);
		}
	}
	memcpy (m_PrevKeys, RawKeys, 6);

	if (m_bCaptured)
	{
		ReleaseAll ();
		return;
	}

	// Con Alt pulsado las teclas son atajos del emulador: no llegan al Spectrum
	if (nModifiers & (HID_MOD_LALT | HID_MOD_RALT))
	{
		for (unsigned i = 0; i < 8; i++)
		{
			m_Matrix[i] = 0;
		}
		m_nKeyJoy = 0;
		return;
	}

	u8 Matrix[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	boolean bShift = (nModifiers & (HID_MOD_LSHIFT | HID_MOD_RSHIFT)) != 0;
	boolean bSuppressCS = FALSE;
	boolean bJoystick = m_JoystickMode != JoystickNone;

	// Joystick: cursores + Ctrl derecho (o 0 del teclado numérico) como disparo
	u8 nJoyDirs = 0;	// bits JOY_*
	if (bJoystick && (nModifiers & HID_MOD_RCTRL))
	{
		nJoyDirs |= 0x10;
	}

	for (unsigned i = 0; i < 6; i++)
	{
		u8 nKey = RawKeys[i];

		if (nKey >= 0x04 && nKey <= 0x1D)
		{
			Press (Matrix, LetterMap[nKey - 0x04]);
		}
		else if (nKey >= 0x1E && nKey <= 0x27)
		{
			Press (Matrix, DigitMap[nKey - 0x1E]);
		}
		else if (nKey >= 0x59 && nKey <= 0x61)	// teclado numérico 1-9
		{
			Press (Matrix, DigitMap[nKey - 0x59]);
		}
		else switch (nKey)
		{
		case HID_KEY_ENTER:
		case 0x58:			// Enter numérico
			Press (Matrix, ZX_ENTER);
			break;

		case HID_KEY_SPACE:
			Press (Matrix, ZX_SPACE);
			break;

		case HID_KEY_BACKSPACE:		// DELETE (CAPS SHIFT + 0)
			// En el CP/M de LEC, DELETE es el "rubout" clásico del CP/M 2.2, que
			// borra pero repite en pantalla lo borrado (escribe la palabra al
			// revés). Allí se envía cursor izquierda (CAPS SHIFT + 5), que es el
			// retroceso de verdad.
			Press (Matrix, ZX_CS);
			Press (Matrix, m_bBackspaceAsLeft ? ZX_5 : ZX_0);
			break;

		case HID_KEY_ESC:		// BREAK
			Press (Matrix, ZX_CS);
			Press (Matrix, ZX_SPACE);
			break;

		case HID_KEY_TAB:		// EXTENDED MODE
			Press (Matrix, ZX_CS);
			Press (Matrix, ZX_SS);
			break;

		case 0x39:			// Bloq Mayús -> CAPS LOCK
			Press (Matrix, ZX_CS);
			Press (Matrix, ZX_2);
			break;

		case 0x62:			// 0 del teclado numérico
			if (bJoystick)
			{
				nJoyDirs |= 0x10;
			}
			else
			{
				Press (Matrix, ZX_0);
			}
			break;

		// Cursores del PC: teclas de cursor del Spectrum (CAPS SHIFT + 5/6/7/8)
		// y/o joystick, según el modo (cursors=both|keys|joystick)
		case HID_KEY_RIGHT:
		case HID_KEY_LEFT:
		case HID_KEY_DOWN:
		case HID_KEY_UP:
		{
			static const u8 Dirs[4] = {0x01, 0x02, 0x04, 0x08};
			static const u8 Digit[4] = {ZX_8, ZX_5, ZX_6, ZX_7};
			unsigned i = nKey - HID_KEY_RIGHT;
			boolean bAsKeys = !bJoystick || m_CursorMode != CursorJoystickOnly;
			boolean bAsJoy = bJoystick && m_CursorMode != CursorKeysOnly;
			if (bAsKeys)
			{
				Press (Matrix, ZX_CS);
				Press (Matrix, Digit[i]);
			}
			if (bAsJoy)
			{
				nJoyDirs |= Dirs[i];
			}
			break;
		}

		default:
			for (unsigned j = 0; j < sizeof SymbolKeys / sizeof SymbolKeys[0]; j++)
			{
				if (SymbolKeys[j].nHID == nKey)
				{
					Press (Matrix, ZX_SS);
					Press (Matrix, bShift ? SymbolKeys[j].nShifted : SymbolKeys[j].nNormal);
					bSuppressCS = TRUE;
					break;
				}
			}
			break;
		}
	}

	if (bShift && !bSuppressCS)
	{
		Press (Matrix, ZX_CS);
	}

	if (nModifiers & HID_MOD_LCTRL)
	{
		Press (Matrix, ZX_SS);
	}
	if ((nModifiers & HID_MOD_RCTRL) && !bJoystick)
	{
		Press (Matrix, ZX_SS);
	}

	for (unsigned i = 0; i < 8; i++)
	{
		m_Matrix[i] = Matrix[i];
	}
	m_nKeyJoy = nJoyDirs;
}

void CZXKeyboard::SetGamePadState (unsigned nIndex, u8 nState)
{
	if (nIndex < MAX_GAMEPADS)
	{
		m_PadJoy[nIndex] = nState;
	}
}

void CZXKeyboard::SetMousePresent (boolean bPresent)
{
	m_bMousePresent = bPresent;
	m_nMouseButtons = 0;
}

void CZXKeyboard::MouseUpdate (unsigned nButtons, int nDX, int nDY, int nWheel)
{
	if (m_bCaptured)
	{
		m_nMouseButtons = 0;
		return;
	}

	// X crece hacia la derecha; Y crece hacia arriba (en USB crece hacia abajo)
	m_nMouseX = (u8) (m_nMouseX + nDX);
	m_nMouseY = (u8) (m_nMouseY - nDY);
	m_nMouseWheel = (m_nMouseWheel - nWheel) & 0x0F;
	m_nMouseButtons = nButtons;
}

u8 CZXKeyboard::ReadMouseButtons (void) const
{
	// bit 0 derecho, bit 1 izquierdo, bit 2 central (activos a nivel bajo);
	// bits 4-7: rueda (Kempston Mouse Turbo)
	u8 nValue = (m_nMouseWheel << 4) | 0x0F;
	unsigned nButtons = m_nMouseButtons;
	if (nButtons & 0x01)	nValue &= ~0x02;	// MOUSE_BUTTON_LEFT
	if (nButtons & 0x02)	nValue &= ~0x01;	// MOUSE_BUTTON_RIGHT
	if (nButtons & 0x04)	nValue &= ~0x04;	// MOUSE_BUTTON_MIDDLE
	return nValue;
}

u8 CZXKeyboard::GetJoystickState (void) const
{
	if (m_bCaptured)
	{
		return 0;
	}

	u8 nState = m_nKeyJoy;
	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		nState |= m_PadJoy[i];
	}

	return nState;
}

u8 CZXKeyboard::ReadKempston (void) const
{
	return GetJoystickState () & 0x3F;
}

u8 CZXKeyboard::ReadFuller (void) const
{
	// activo a nivel bajo: arriba 0, abajo 1, izquierda 2, derecha 3, disparo 7
	u8 nJoy = GetJoystickState ();
	u8 nValue = 0xFF;
	if (nJoy & JOY_UP)			nValue &= ~0x01;
	if (nJoy & JOY_DOWN)			nValue &= ~0x02;
	if (nJoy & JOY_LEFT)			nValue &= ~0x04;
	if (nJoy & JOY_RIGHT)			nValue &= ~0x08;
	if (nJoy & (JOY_FIRE | JOY_FIRE2))	nValue &= ~0x80;
	return nValue;
}

u8 CZXKeyboard::ReadPort (u8 nHighByte) const
{
	u8 Matrix[8];
	for (unsigned nRow = 0; nRow < 8; nRow++)
	{
		Matrix[nRow] = m_Matrix[nRow] | m_GPIOMatrix[nRow];
	}

	// Joysticks que se leen como teclas
	u8 nJoy = GetJoystickState ();
	if (nJoy != 0)
	{
		// posición en la matriz para: derecha, izquierda, abajo, arriba, disparo, disparo 2
		static const u8 JoyKeys[JoystickModeCount][6] =
		{
			{NK, NK, NK, NK, NK, NK},				// ninguno
			{NK, NK, NK, NK, NK, NK},				// Kempston (puerto)
			{NK, NK, NK, NK, NK, NK},				// Fuller (puerto)
			{ZX_7, ZX_6, ZX_8, ZX_9, ZX_0, K(0,2)},		// Sinclair 1, X
			{K(3,1), K(3,0), K(3,2), K(3,3), K(3,4), ZX_Z},	// Sinclair 2, Z
			{ZX_8, ZX_5, ZX_6, ZX_7, ZX_0, ZX_9},		// Cursor, 9
			{ZX_P, ZX_O, K(1,0), K(2,0), ZX_SPACE, ZX_M},	// QAOP, SPACE, M
		};

		const u8 *pKeys = JoyKeys[m_JoystickMode];
		for (unsigned i = 0; i < 6; i++)
		{
			if (nJoy & (1 << i))
			{
				Press (Matrix, pKeys[i]);
			}
		}
	}

	u8 nKeys = 0;
	for (unsigned nRow = 0; nRow < 8; nRow++)
	{
		if ((nHighByte & (1 << nRow)) == 0)
		{
			nKeys |= Matrix[nRow];
		}
	}

	return ~nKeys | 0xE0;	// bits 5-7 a 1; el bit 6 (EAR) lo ajusta el llamante
}
