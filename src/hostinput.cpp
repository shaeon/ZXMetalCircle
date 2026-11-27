//
// hostinput.cpp - La entrada del PC, para cualquier máquina (ver hostinput.h)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "hostinput.h"
#include <circle/util.h>

CHostInput::CHostInput (void)
:	m_pMachine (0),
	m_bCaptured (FALSE),
	m_nEventIn (0),
	m_nEventOut (0)
{
	memset (m_PrevKeys, 0, sizeof m_PrevKeys);
	memset (m_KbdModifiers, 0, sizeof m_KbdModifiers);
	memset (m_KbdKeys, 0, sizeof m_KbdKeys);
}

void CHostInput::SetMachineInput (CMachineInput *pMachine)
{
	m_pMachine = pMachine;
	if (pMachine != 0)
	{
		pMachine->SetCaptured (m_bCaptured);	// por si ya había un menú abierto
	}
}

void CHostInput::SetCaptured (boolean bCaptured)
{
	m_bCaptured = bCaptured;
	if (m_pMachine != 0)
	{
		m_pMachine->SetCaptured (bCaptured);
	}
}

u8 CHostInput::GetHeldNavKey (void) const
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

void CHostInput::PushEvent (u8 nKey, u8 nModifiers)
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

boolean CHostInput::GetKeyEvent (TKeyEvent *pEvent)
{
	if (m_nEventOut == m_nEventIn)
	{
		return FALSE;
	}

	*pEvent = m_Events[m_nEventOut];
	m_nEventOut = (m_nEventOut + 1) % (sizeof m_Events / sizeof m_Events[0]);

	return TRUE;
}

void CHostInput::UpdateFromKeyboard (unsigned nKeyboard, u8 nModifiers, const u8 RawKeys[6])
{
	if (nKeyboard >= HOSTINPUT_KEYBOARDS)
	{
		UpdateFromReport (nModifiers, RawKeys);
		return;
	}

	// informe de error (demasiadas teclas): se mantiene lo que tenía ese teclado
	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] >= 0x01 && RawKeys[i] <= 0x03)
		{
			return;
		}
	}

	m_KbdModifiers[nKeyboard] = nModifiers;
	memcpy (m_KbdKeys[nKeyboard], RawKeys, 6);
	UpdateMerged ();
}

void CHostInput::ForgetKeyboard (unsigned nKeyboard)
{
	if (nKeyboard < HOSTINPUT_KEYBOARDS)
	{
		m_KbdModifiers[nKeyboard] = 0;
		memset (m_KbdKeys[nKeyboard], 0, 6);
	}
	UpdateMerged ();
}

// Las teclas de todos los teclados juntas: los modificadores de cualquiera y
// hasta 6 teclas (las primeras que haya, sin repetir)
void CHostInput::UpdateMerged (void)
{
	u8 nModifiers = 0;
	u8 Keys[6] = {0, 0, 0, 0, 0, 0};
	unsigned nKeys = 0;
	for (unsigned k = 0; k < HOSTINPUT_KEYBOARDS; k++)
	{
		nModifiers |= m_KbdModifiers[k];
		for (unsigned i = 0; i < 6 && nKeys < 6; i++)
		{
			u8 nKey = m_KbdKeys[k][i];
			if (nKey < 4)
			{
				continue;
			}
			boolean bDup = FALSE;
			for (unsigned j = 0; j < nKeys; j++)
			{
				if (Keys[j] == nKey)
				{
					bDup = TRUE;
				}
			}
			if (!bDup)
			{
				Keys[nKeys++] = nKey;
			}
		}
	}
	UpdateFromReport (nModifiers, Keys);
}

void CHostInput::UpdateFromReport (u8 nModifiers, const u8 RawKeys[6])
{
	// Con demasiadas teclas a la vez (o una combinación que su matriz no
	// distingue), el teclado manda un informe de error: ErrorRollOver (0x01)
	// en todas las posiciones. Según la norma HID se mantiene el estado
	// anterior hasta el siguiente informe válido; antes se soltaba todo y el
	// juego veía de golpe que no había ninguna tecla pulsada (p. ej. al
	// teclear a la vez VICTOR en Profanation).
	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] >= 0x01 && RawKeys[i] <= 0x03)
		{
			return;
		}
	}

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

	if (m_pMachine == 0)
	{
		return;
	}

	if (m_bCaptured)
	{
		m_pMachine->ReleaseAll ();
		return;
	}

	// Con Alt pulsado las teclas son atajos del emulador: no llegan a la máquina
	if (nModifiers & (HID_MOD_LALT | HID_MOD_RALT))
	{
		m_pMachine->ReleasePCKeys ();
		return;
	}

	m_pMachine->SetPCKeys (nModifiers, RawKeys);
}
