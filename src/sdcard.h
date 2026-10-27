//
// sdcard.h - Tarjeta SD en modo SPI (para la ZXMMC del +3e) sobre una imagen
//
// Hasta 2 GB se comporta como una tarjeta SD normal (SDSC: las órdenes llevan
// la dirección en bytes); a partir de ahí, como una SDHC (dirección = número de
// sector), igual que una tarjeta real de ese tamaño. Con una imagen de 1 GB
// funcionan tanto la ROM del +3e (que mira el tipo con CMD58) como programas
// que solo conocen las tarjetas SDSC, como el CP/M 3 de Otivax.
// Órdenes: CMD0, CMD1, CMD8, CMD9, CMD10, CMD12, CMD13, CMD16, CMD17, CMD18,
// CMD24, CMD25, CMD55, CMD58, CMD59 y ACMD41. Referencias: sd_card.v de MiST
// (Till Harbaum, GPL) y mmc.c de ZEsarUX (César Hernández, GPLv3).
//
// Los sectores se leen y escriben a través de CBlockDevice, que en la Pi es un
// fichero de la tarjeta SD (ver CHardDiskImage en el kernel).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_sdcard_h
#define _zx_sdcard_h

#include <circle/types.h>

class CBlockDevice
{
public:
	virtual ~CBlockDevice (void) {}
	virtual u32 GetSectors (void) const = 0;			// sectores de 512 bytes
	virtual boolean ReadSector (u32 nSector, u8 *pBuffer) = 0;
	virtual boolean WriteSector (u32 nSector, const u8 *pBuffer) = 0;
	virtual boolean IsReadOnly (void) const		{ return FALSE; }
};

class CSDCard
{
public:
	CSDCard (void);

	void Insert (CBlockDevice *pDevice);	// 0 = sin tarjeta
	boolean IsInserted (void) const		{ return m_pDevice != 0; }

	void Select (boolean bSelected);	// línea CS (activa en bajo en la interfaz)
	u8 Exchange (u8 nOut);			// un byte por SPI en cada sentido

	boolean IsActive (void)			// actividad reciente (para el indicador)
	{
		boolean b = m_bActivity;
		m_bActivity = FALSE;
		return b;
	}

private:
	void Reset (void);
	void Command (void);
	void QueueByte (u8 nByte);
	void QueueR1 (u8 nR1);
	void StartRead (void);

	enum TState
	{
		StateCommand,		// esperando una orden
		StateReadMulti,		// CMD18: se envían bloques hasta CMD12
		StateWriteWaitToken,	// CMD24/25: esperando el testigo de datos
		StateWriteData,		// recibiendo 512 bytes + CRC
	};

	CBlockDevice *m_pDevice;
	boolean m_bSelected;
	boolean m_bIdle;		// tras CMD0 hasta ACMD41
	boolean m_bAppCmd;		// la orden anterior fue CMD55
	TState m_State;
	boolean m_bMultiWrite;
	boolean m_bActivity;
	boolean m_bHighCapacity;	// SDHC (más de 2 GB): direcciones en sectores; si no, en bytes
	u32 Sector (u32 nArg) const	{ return m_bHighCapacity ? nArg : nArg / 512; }

	u8 m_Cmd[6];
	unsigned m_nCmdLen;

	u32 m_nSector;			// sector en curso (lectura o escritura múltiple)

	// Bytes pendientes de enviar al Spectrum
	u8 m_Out[600];
	unsigned m_nOutHead, m_nOutTail;

	u8 m_Block[512];
	unsigned m_nBlockPos;		// bytes de datos (y CRC) recibidos al escribir
};

#endif
