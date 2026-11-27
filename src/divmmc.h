//
// divmmc.h - Interfaces de tarjeta SD: divMMC (con esxDOS y su reloj) y ZXMMC
//
// Fase 2c-2 de la capa de máquinas: salen de CSpectrum. Las dos usan la
// misma tarjeta (CSDCard, de CSpectrum), cada una con sus puertos:
//
//  - divMMC: 0xE3 (control: CONMEM, MAPRAM, banco), 0xE7 (selección de la
//    tarjeta) y 0xEB (datos SPI); 8 KB de ROM (esxmmc.bin) y 128 KB de RAM
//    que aparecen en 0x0000-0x3FFF al ejecutar los puntos de entrada
//    (0x0000, 0x0008, 0x0038, 0x0066, 0x04C6, 0x0562; 0x3D00-0x3DFF al
//    instante) y se retiran al ejecutar 0x1FF8-0x1FFF. Y el reloj para
//    esxDOS (RTC.SYS de ZXMetalCircle) en 0x7C3B.
//  - ZXMMC (+3e): 0x1F (selección) y 0x3F (datos SPI).
//
// En el +3e la divMMC solo pone sus puertos (sin ROM ni paginación), como
// en el ZX-Uno. Lo que se usa en cada acceso del Z80 (IsPaged, Read, Write,
// Fetch) es inline: va dentro de CSpectrum, sin funciones virtuales.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_divmmc_h
#define _zx_divmmc_h

#include "zxmemory.h"
#include "sdcard.h"
#include <circle/types.h>

class CDivMMC
{
public:
	CDivMMC (CSDCard *pCard);
	~CDivMMC (void);

	void SetROM (const u8 *pROM);		// 8 KB (0 = sin divMMC completa)
	boolean HasROM (void) const		{ return m_bROM; }
	void SetEnabled (boolean bOn);		// divmmc=esxdos
	boolean IsEnabled (void) const		{ return m_bEnabled; }
	// Conectada, con ROM y en un modelo sin ZXMMC (lo decide CSpectrum; se
	// guarda porque se mira en cada acceso)
	void SetActive (boolean bActive)	{ m_bActive = bActive; }
	boolean IsActive (void) const		{ return m_bActive; }

	// Reset: CONMEM y MAPRAM se borran, sin paginar
	void Reset (void)
	{
		m_bAutomap = FALSE;
		m_bAutomapLoad = FALSE;
		m_nControl = 0;
	}

	// Su memoria en 0x0000-0x3FFF
	boolean IsPaged (void) const
		{ return m_bActive && (m_bAutomap || (m_nControl & 0x80)); }

	//   CONMEM:              0x0000 ROM (esxDOS)       0x2000 banco elegido
	//   MAPRAM (sin CONMEM): 0x0000 banco 3 (lectura)  0x2000 banco elegido
	//   normal:              0x0000 ROM                0x2000 banco elegido
	u8 Read (u16 nAddress) const
	{
		if (nAddress < 0x2000)
		{
			if (!(m_nControl & 0x80) && (m_nControl & 0x40))
			{
				return m_pRAM[3 * 0x2000 + nAddress];
			}
			return m_ROM[nAddress];
		}
		return m_pRAM[(m_nControl & 0x0F) * 0x2000 + (nAddress - 0x2000)];
	}

	void Write (u16 nAddress, u8 nValue)
	{
		if (nAddress < 0x2000)
		{
			return;		// ROM protegida (el puente de escritura de la EEPROM, cerrado)
		}

		unsigned nBank = m_nControl & 0x0F;
		if (!(m_nControl & 0x80) && (m_nControl & 0x40) && nBank == 3)
		{
			return;		// con MAPRAM, el banco 3 es de solo lectura
		}
		m_pRAM[nBank * 0x2000 + (nAddress - 0x2000)] = nValue;
	}

	// Lectura de una instrucción (ciclo M1) con la paginación automática
	u8 Fetch (u16 nAddress, const CMemory &rMem)
	{
		// 0x3D00-0x3DFF: se pagina antes de leer (la instrucción ya es de la divMMC)
		if (!m_bAutomap && (nAddress & 0xFF00) == 0x3D00)
		{
			m_bAutomap = TRUE;
		}

		u8 nOpcode = (nAddress < 0x4000 && IsPaged ()) ? Read (nAddress)
							       : rMem.ReadPage (nAddress >> 14)[nAddress & 0x3FFF];

		// los demás puntos de entrada paginan después de leer la instrucción,
		// y 0x1FF8-0x1FFF despagina también después
		if (!m_bAutomap)
		{
			if (   nAddress == 0x0000 || nAddress == 0x0008 || nAddress == 0x0038
			    || nAddress == 0x0066 || nAddress == 0x04C6 || nAddress == 0x0562)
			{
				m_bAutomap = TRUE;
				m_bAutomapLoad = nAddress == 0x0562;	// ver la carga instantánea
			}
		}
		else if (nAddress >= 0x1FF8 && nAddress <= 0x1FFF)
		{
			m_bAutomap = FALSE;
			m_bAutomapLoad = FALSE;
		}

		return nOpcode;
	}

	// La carga instantánea se salta la rutina de la ROM en 0x0562: si la
	// divMMC se paginó ahí, se retira (si no, la vuelta ejecutaría esxDOS)
	void CancelLoadAutomap (void)
	{
		if (m_bAutomapLoad)
		{
			m_bAutomap = FALSE;
		}
		m_bAutomapLoad = FALSE;
	}
	void ClearLoadAutomap (void)		{ m_bAutomapLoad = FALSE; }

	// Puertos (con IsActive o en el +3e; ver CSpectrum). 0xEB: lo recibido
	// en la transferencia anterior, y lanza otra con 0xFF
	u8 ReadData (void)
	{
		u8 nValue = m_nData;
		m_nData = m_pCard->Exchange (0xFF);
		return nValue;
	}
	void WritePort (u8 nPort, u8 nValue);	// 0xE3, 0xE7, 0xEB

	// Reloj para esxDOS: fecha y hora en formato MS-DOS (0 = sin hora). Un
	// OUT a 0x7C3B la fija y cuatro IN la leen (fecha byte bajo, alto, hora
	// byte bajo, alto). Solo con la divMMC activa.
	void SetRTC (u16 nDate, u16 nTime)	{ m_nRTCDate = nDate; m_nRTCTime = nTime; }
	void LatchRTC (void);
	u8 ReadRTC (void)			{ return m_nRTCIndex < 4 ? m_RTCLatch[m_nRTCIndex++] : 0xFF; }

private:
	CSDCard *m_pCard;

	boolean m_bEnabled;		// divmmc=esxdos
	boolean m_bROM;
	boolean m_bActive;
	boolean m_bAutomap;		// paginada por un punto de entrada
	boolean m_bAutomapLoad;		// la paginación automática ha sido en 0x0562
	u8 m_nControl;			// puerto 0xE3: CONMEM, MAPRAM, banco
	u8 m_nData;			// byte recibido en la transferencia anterior
	u8 m_ROM[0x2000];
	u8 *m_pRAM;			// 128 KB: 16 bancos de 8 KB

	u16 m_nRTCDate, m_nRTCTime;
	u8 m_RTCLatch[4];
	unsigned m_nRTCIndex;
};

// ZXMMC del +3e: 0x1F bit 0 = CS de la tarjeta (activa en bajo), 0x3F =
// datos SPI (la lectura devuelve lo recibido en la transferencia anterior y
// lanza otra con 0xFF, como el spi.v de zxp3)
class CZXMMC
{
public:
	CZXMMC (CSDCard *pCard) : m_pCard (pCard), m_nData (0) {}

	u8 ReadData (void)
	{
		u8 nValue = m_nData;
		m_nData = m_pCard->Exchange (0xFF);
		return nValue;
	}
	void WritePort (u8 nPort, u8 nValue)
	{
		if (nPort == 0x1F)
		{
			m_pCard->Select ((nValue & 0x01) == 0);
		}
		else if (nPort == 0x3F)
		{
			m_nData = m_pCard->Exchange (nValue);
		}
	}

private:
	CSDCard *m_pCard;
	u8 m_nData;		// a 0 al encender (como en zxp3 y el ZX-Uno; el
				// cargador de CP/M de Otivax detecta la interfaz por esto)
};

#endif
