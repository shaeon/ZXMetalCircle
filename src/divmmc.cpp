//
// divmmc.cpp - Interfaces de tarjeta SD: divMMC (con esxDOS y su reloj) y ZXMMC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "divmmc.h"
#include <circle/util.h>

CDivMMC::CDivMMC (CSDCard *pCard)
:	m_pCard (pCard),
	m_bEnabled (FALSE),
	m_bROM (FALSE),
	m_bActive (FALSE),
	m_bAutomap (FALSE),
	m_bAutomapLoad (FALSE),
	m_nControl (0),
	m_nData (0),		// a 0 al encender (ver CZXMMC)
	m_nRTCDate (0),
	m_nRTCTime (0),
	m_nRTCIndex (4)
{
	memset (m_ROM, 0, sizeof m_ROM);
	m_pRAM = new u8[128 * 1024];
	memset (m_pRAM, 0, 128 * 1024);
	memset (m_RTCLatch, 0, sizeof m_RTCLatch);
}

CDivMMC::~CDivMMC (void)
{
	delete [] m_pRAM;
}

void CDivMMC::SetROM (const u8 *pROM)
{
	m_bROM = pROM != 0;
	if (pROM != 0)
	{
		memcpy (m_ROM, pROM, sizeof m_ROM);
	}
}

void CDivMMC::SetEnabled (boolean bOn)
{
	m_bEnabled = bOn;
	Reset ();
}

// 0xE3 control (MAPRAM no se puede borrar salvo con reset; solo con la
// divMMC completa), 0xE7 bit 0 = CS de la tarjeta 0 (activa en bajo), 0xEB
// datos SPI
void CDivMMC::WritePort (u8 nPort, u8 nValue)
{
	switch (nPort)
	{
	case 0xE3:
		if (m_bActive)
		{
			m_nControl = nValue | (m_nControl & 0x40);
		}
		break;

	case 0xE7:
		m_pCard->Select ((nValue & 0x01) == 0);
		break;

	case 0xEB:
		m_nData = m_pCard->Exchange (nValue);
		break;
	}
}

void CDivMMC::LatchRTC (void)
{
	m_RTCLatch[0] = m_nRTCDate & 0xFF;
	m_RTCLatch[1] = m_nRTCDate >> 8;
	m_RTCLatch[2] = m_nRTCTime & 0xFF;
	m_RTCLatch[3] = m_nRTCTime >> 8;
	m_nRTCIndex = 0;
}
