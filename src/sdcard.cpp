//
// sdcard.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "sdcard.h"
#include <circle/util.h>

#define R1_IDLE			0x01
#define R1_ILLEGAL		0x04
#define R1_PARAMETER		0x40

#define TOKEN_DATA		0xFE	// CMD17/18/24
#define TOKEN_MULTI_WRITE	0xFC	// CMD25
#define TOKEN_STOP		0xFD	// fin de CMD25

CSDCard::CSDCard (void)
:	m_pDevice (0),
	m_bSelected (FALSE),
	m_bActivity (FALSE),
	m_bHighCapacity (FALSE)
{
	Reset ();
}

void CSDCard::Reset (void)
{
	m_bIdle = TRUE;
	m_bAppCmd = FALSE;
	m_State = StateCommand;
	m_bMultiWrite = FALSE;
	m_nCmdLen = 0;
	m_nOutHead = m_nOutTail = 0;
	m_nBlockPos = 0;
	m_nSector = 0;
}

void CSDCard::Insert (CBlockDevice *pDevice)
{
	m_pDevice = pDevice;
	m_bHighCapacity = pDevice != 0 && pDevice->GetSectors () > 4194304;	// > 2 GB
	Reset ();
}

void CSDCard::Select (boolean bSelected)
{
	if (bSelected == m_bSelected)
	{
		return;
	}

	m_bSelected = bSelected;
	if (!bSelected)
	{
		// al soltar CS se abandona lo que estuviera a medias (pero la tarjeta
		// sigue inicializada)
		m_nCmdLen = 0;
		m_nOutHead = m_nOutTail = 0;
		if (m_State == StateReadMulti)
		{
			m_State = StateCommand;
		}
	}
}

void CSDCard::QueueByte (u8 nByte)
{
	if (m_nOutTail < sizeof m_Out)
	{
		m_Out[m_nOutTail++] = nByte;
	}
}

void CSDCard::QueueR1 (u8 nR1)
{
	QueueByte (0xFF);		// un byte de espera (NCR) antes de la respuesta
	QueueByte (nR1 | (m_bIdle ? R1_IDLE : 0));
}

// Pone en la cola un bloque de lectura: espera, testigo, 512 bytes y CRC
void CSDCard::StartRead (void)
{
	if (!m_pDevice->ReadSector (m_nSector, m_Block))
	{
		QueueByte (0xFF);
		QueueByte (0x01);	// testigo de error: error genérico
		m_State = StateCommand;
		return;
	}

	QueueByte (0xFF);
	QueueByte (TOKEN_DATA);
	u16 nCRC = 0;
	for (unsigned i = 0; i < 512; i++)
	{
		QueueByte (m_Block[i]);

		// CRC16-CCITT (polinomio 0x1021, valor inicial 0), como una tarjeta real
		nCRC ^= m_Block[i] << 8;
		for (unsigned b = 0; b < 8; b++)
		{
			nCRC = (nCRC & 0x8000) ? (nCRC << 1) ^ 0x1021 : nCRC << 1;
		}
	}
	// El CRC de verdad, no 0xFFFF: algunos programas detectan la interfaz
	// leyendo el último byte recibido y esperan algo distinto de 0xFF (el
	// cargador de CP/M de Otivax, por ejemplo)
	QueueByte (nCRC >> 8);
	QueueByte (nCRC & 0xFF);
	m_bActivity = TRUE;
}

u8 CSDCard::Exchange (u8 nIn)
{
	if (m_pDevice == 0)
	{
		return 0xFF;		// sin tarjeta
	}

	if (!m_bSelected)
	{
		// Con la tarjeta sin seleccionar nadie mueve la línea MISO, que en el
		// ZX-Uno se lee a 0. Los controladores de CP/M de Otivax lo usan para
		// saber si hay tarjeta: tras unos ciclos sin seleccionarla, la
		// seleccionan y esperan leer algo distinto de 0xFF.
		return 0x00;
	}

	// Byte que sale hacia el Spectrum en esta transferencia
	u8 nOut = 0xFF;
	if (m_nOutHead < m_nOutTail)
	{
		nOut = m_Out[m_nOutHead++];
		if (m_nOutHead == m_nOutTail)
		{
			m_nOutHead = m_nOutTail = 0;

			// lectura múltiple: siguiente bloque
			if (m_State == StateReadMulti)
			{
				m_nSector++;
				if (m_nSector < m_pDevice->GetSectors ())
				{
					StartRead ();
				}
			}
		}
	}

	// Byte que entra desde el Spectrum
	switch (m_State)
	{
	case StateWriteWaitToken:
		if (nIn == TOKEN_DATA || nIn == TOKEN_MULTI_WRITE)
		{
			m_State = StateWriteData;
			m_nBlockPos = 0;
		}
		else if (nIn == TOKEN_STOP && m_bMultiWrite)
		{
			m_State = StateCommand;
			QueueByte (0xFF);
			QueueByte (0x00);	// ocupada un momento
		}
		break;

	case StateWriteData:
		if (m_nBlockPos < 512)
		{
			m_Block[m_nBlockPos] = nIn;
		}
		if (++m_nBlockPos == 514)	// datos + CRC
		{
			boolean bOK =    m_nSector < m_pDevice->GetSectors ()
				      && !m_pDevice->IsReadOnly ()
				      && m_pDevice->WriteSector (m_nSector, m_Block);
			m_bActivity = TRUE;

			QueueByte (bOK ? 0x05 : 0x0D);	// datos aceptados / error de escritura
			QueueByte (0x00);		// ocupada
			QueueByte (0x00);

			if (m_bMultiWrite && bOK)
			{
				m_nSector++;
				m_State = StateWriteWaitToken;
			}
			else
			{
				m_State = StateCommand;
			}
		}
		break;

	case StateCommand:
	case StateReadMulti:
		if (m_nCmdLen == 0 && (nIn & 0xC0) != 0x40)
		{
			break;			// relleno entre órdenes
		}
		m_Cmd[m_nCmdLen++] = nIn;
		if (m_nCmdLen == 6)
		{
			m_nCmdLen = 0;
			Command ();
		}
		break;
	}

	return nOut;
}

void CSDCard::Command (void)
{
	unsigned nCmd = m_Cmd[0] & 0x3F;
	u32 nArg = (m_Cmd[1] << 24) | (m_Cmd[2] << 16) | (m_Cmd[3] << 8) | m_Cmd[4];
	boolean bApp = m_bAppCmd;
	m_bAppCmd = FALSE;

	// en lectura múltiple solo vale CMD12 (y lo demás la interrumpe igual)
	if (m_State == StateReadMulti)
	{
		m_nOutHead = m_nOutTail = 0;
		m_State = StateCommand;
	}

	switch (nCmd)
	{
	case 0:			// GO_IDLE_STATE
		Reset ();
		QueueR1 (0);
		break;

	case 1:			// SEND_OP_COND (MMC)
		m_bIdle = FALSE;
		QueueR1 (0);
		break;

	case 8:			// SEND_IF_COND: tarjeta SD v2
		QueueR1 (0);
		QueueByte (0x00);
		QueueByte (0x00);
		QueueByte (0x01);	// 2,7-3,6 V
		QueueByte (nArg & 0xFF);	// patrón de comprobación
		break;

	case 9:			// SEND_CSD
	{
		u8 CSD[16];
		if (m_bHighCapacity)
		{
			// versión 2.0 (SDHC): C_SIZE en unidades de 512 KB
			u32 nSize = m_pDevice->GetSectors () / 1024;
			if (nSize > 0)
			{
				nSize--;
			}
			const u8 CSD2[16] =
			{
				0x40, 0x0E, 0x00, 0x32, 0x5B, 0x59, 0x00,
				(u8) ((nSize >> 16) & 0x3F), (u8) (nSize >> 8), (u8) nSize,
				0x7F, 0x80, 0x0A, 0x40, 0x00, 0x01
			};
			memcpy (CSD, CSD2, sizeof CSD);
		}
		else
		{
			// versión 1.0 (SDSC): capacidad = (C_SIZE + 1) * 2^(C_SIZE_MULT + 2)
			// bloques de 2^READ_BL_LEN bytes (512, o 1024 a partir de 1 GB)
			u32 nBlocks = m_pDevice->GetSectors ();
			unsigned nBlockLen = 9;
			if (nBlocks > 4096u * 512)
			{
				nBlockLen = 10;
				nBlocks /= 2;
			}
			unsigned nMult = 0;
			while (nMult < 7 && (nBlocks >> (nMult + 2)) > 4096)
			{
				nMult++;
			}
			u32 nCSize = (nBlocks >> (nMult + 2));
			if (nCSize > 0)
			{
				nCSize--;
			}
			const u8 CSD1[16] =
			{
				0x00, 0x26, 0x00, 0x32, 0x5B, (u8) (0x50 | nBlockLen),
				(u8) (0x80 | ((nCSize >> 10) & 0x03)), (u8) (nCSize >> 2),
				(u8) (((nCSize & 0x03) << 6) | 0x3F),
				(u8) (0xFC | ((nMult >> 1) & 0x03)),
				(u8) (((nMult & 1) << 7) | 0x7F),
				0x80, 0x0A, 0x40, 0x00, 0x01
			};
			memcpy (CSD, CSD1, sizeof CSD);
		}
		QueueR1 (0);
		QueueByte (0xFF);
		QueueByte (TOKEN_DATA);
		for (unsigned i = 0; i < 16; i++)
		{
			QueueByte (CSD[i]);
		}
		QueueByte (0xFF);
		QueueByte (0xFF);
		break;
	}

	case 10:		// SEND_CID
	{
		static const u8 CID[16] =
		{
			0x5A, 'Z', 'X', 'M', 'e', 't', 'a', 'l', 0x10,
			0x00, 0x00, 0x00, 0x01, 0x01, 0x9A, 0x01
		};
		QueueR1 (0);
		QueueByte (0xFF);
		QueueByte (TOKEN_DATA);
		for (unsigned i = 0; i < 16; i++)
		{
			QueueByte (CID[i]);
		}
		QueueByte (0xFF);
		QueueByte (0xFF);
		break;
	}

	case 12:		// STOP_TRANSMISSION
		QueueByte (0xFF);	// byte de relleno tras CMD12
		QueueR1 (0);
		QueueByte (0x00);	// ocupada
		break;

	case 13:		// SEND_STATUS (R2)
		QueueR1 (0);
		QueueByte (0x00);
		break;

	case 16:		// SET_BLOCKLEN (siempre 512 en SDHC)
	case 59:		// CRC_ON_OFF
		QueueR1 (0);
		break;

	case 17:		// READ_SINGLE_BLOCK
	case 18:		// READ_MULTIPLE_BLOCK
		if (Sector (nArg) >= m_pDevice->GetSectors ())
		{
			QueueR1 (R1_PARAMETER);
			break;
		}
		QueueR1 (0);
		m_nSector = Sector (nArg);
		StartRead ();
		if (nCmd == 18)
		{
			m_State = StateReadMulti;
		}
		break;

	case 24:		// WRITE_BLOCK
	case 25:		// WRITE_MULTIPLE_BLOCK
		if (Sector (nArg) >= m_pDevice->GetSectors ())
		{
			QueueR1 (R1_PARAMETER);
			break;
		}
		QueueR1 (0);
		m_nSector = Sector (nArg);
		m_bMultiWrite = nCmd == 25;
		m_State = StateWriteWaitToken;
		break;

	case 41:		// ACMD41 (SD_SEND_OP_COND) o CMD41 a secas
		m_bIdle = FALSE;
		QueueR1 (bApp ? 0 : R1_ILLEGAL);
		break;

	case 55:		// APP_CMD
		m_bAppCmd = TRUE;
		QueueR1 (0);
		break;

	case 58:		// READ_OCR: bit 31 = lista, bit 30 = alta capacidad (SDHC)
		QueueR1 (0);
		QueueByte ((m_bIdle ? 0x00 : 0x80) | (m_bHighCapacity ? 0x40 : 0x00));
		QueueByte (0xFF);
		QueueByte (0x80);
		QueueByte (0x00);
		break;

	default:
		QueueR1 (R1_ILLEGAL);
		break;
	}
}
