//
// spectranet.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "spectranet.h"
#include <circle/util.h>

#define CPLD_VERSION		0x03

// Páginas de 4 KB
#define PAGE_FLASH_END		0x20
#define PAGE_W5100		0x40
#define PAGE_W5100_END		0x48
#define PAGE_RAM		0xC0
#define PAGE_RAM_END		0xE0

// W5100
#define SOCK_CLOSED		0x00
#define SOCK_INIT		0x13
#define SOCK_SYNSENT		0x15
#define SOCK_LISTEN		0x14
#define SOCK_ESTABLISHED	0x17
#define SOCK_CLOSE_WAIT		0x1C
#define SOCK_UDP		0x22

#define MODE_CLOSED		0
#define MODE_TCP		1
#define MODE_UDP		2

#define IR_CON			0x01
#define IR_DISCON		0x02
#define IR_RECV			0x04
#define IR_TIMEOUT		0x08
#define IR_SEND_OK		0x10

CSpectranet::CSpectranet (void)
:	m_bFlashModified (FALSE),
	m_FlashState (FlashReset),
	m_bTrapsDisabled (FALSE),
	m_pNet (0),
	m_bActivity (FALSE),
	m_nDHCPLength (0),
	m_nDHCPSocket (-1)
{
	m_pFlash = new u8[SNET_FLASH_SIZE];
	m_pRAM = new u8[SNET_RAM_SIZE];
	memset (m_pFlash, 0xFF, SNET_FLASH_SIZE);
	memset (m_pRAM, 0, SNET_RAM_SIZE);
	for (unsigned i = 0; i < 4; i++)
	{
		m_Socket[i].nHandle = -1;
	}
	Reset ();
}

CSpectranet::~CSpectranet (void)
{
	W5100Reset ();
	delete [] m_pFlash;
	delete [] m_pRAM;
}

void CSpectranet::LoadFlash (const u8 *pData, unsigned nSize)
{
	if (nSize > SNET_FLASH_SIZE)
	{
		nSize = SNET_FLASH_SIZE;
	}
	memcpy (m_pFlash, pData, nSize);
	m_bFlashModified = FALSE;
}

void CSpectranet::Reset (void)
{
	m_bPaged = !m_bTrapsDisabled;	// al encender corre el firmware de la tarjeta
	m_bPagedViaIO = FALSE;
	m_bNMIFlipFlop = FALSE;
	m_nTrap = 0;
	m_bTrapActive = FALSE;
	m_bTrapWriteMSB = FALSE;
	m_nControl = 0;
	m_FlashState = FlashReset;
	m_nDHCPSocket = -1;
	MapPage (0, 0x00);
	MapPage (1, 0x00);
	MapPage (2, 0x00);
	MapPage (3, PAGE_RAM);
	W5100Reset ();
}

////////////////////////////////////////////////////////////////////////////////
// Memoria

u8 CSpectranet::Read (u16 nAddress)
{
	return ReadPage (m_Page[(nAddress >> 12) & 3], nAddress & 0x0FFF);
}

void CSpectranet::Write (u16 nAddress, u8 nValue)
{
	unsigned nSlot = (nAddress >> 12) & 3;
	WritePage (nSlot, m_Page[nSlot], nAddress & 0x0FFF, nValue);
}

u8 CSpectranet::ReadPage (u8 nPage, u16 nOffset)
{
	if (nPage < PAGE_FLASH_END)
	{
		return FlashRead ((u32) nPage * 0x1000 + nOffset);
	}
	if (nPage >= PAGE_W5100 && nPage < PAGE_W5100_END)
	{
		return W5100Read ((nPage - PAGE_W5100) * 0x1000 + nOffset);
	}
	if (nPage >= PAGE_RAM && nPage < PAGE_RAM_END)
	{
		return m_pRAM[(u32) (nPage - PAGE_RAM) * 0x1000 + nOffset];
	}
	return 0xFF;
}

void CSpectranet::WritePage (unsigned nSlot, u8 nPage, u16 nOffset, u8 nValue)
{
	if (nPage < PAGE_FLASH_END)
	{
		// Como en Fuse, la escritura en cualquier zona con flash va a la
		// página de flash que tenga la página B (así funciona, por ejemplo,
		// el F0 de "reset" que el firmware escribe en 0x0000)
		u8 nFlashPage = m_Page[2] < PAGE_FLASH_END ? m_Page[2] : nPage;
		FlashWrite (nFlashPage / 4, (nFlashPage % 4) * 0x1000 + nOffset, nValue);
	}
	else if (nPage >= PAGE_W5100 && nPage < PAGE_W5100_END)
	{
		W5100Write ((nPage - PAGE_W5100) * 0x1000 + nOffset, nValue);
	}
	else if (nPage >= PAGE_RAM && nPage < PAGE_RAM_END)
	{
		m_pRAM[(u32) (nPage - PAGE_RAM) * 0x1000 + nOffset] = nValue;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Paginación, NMI y puertos

boolean CSpectranet::M1Early (u16 nAddress)
{
	if (m_bTrapsDisabled)
	{
		return FALSE;
	}

	if (nAddress == 0x0008 || (nAddress & 0xFFF8) == 0x3FF8)
	{
		m_bPaged = TRUE;
		m_bPagedViaIO = FALSE;
	}

	return m_bTrapActive && nAddress == m_nTrap && (!m_bPaged || nAddress >= 0x4000);
}

void CSpectranet::M1Late (u16 nAddress)
{
	if (nAddress == 0x007C && !m_bTrapsDisabled)
	{
		m_bPaged = FALSE;
		m_bPagedViaIO = FALSE;
	}
}

void CSpectranet::NMI (void)
{
	if (m_bTrapsDisabled)
	{
		return;
	}
	m_bNMIFlipFlop = TRUE;
	if (!m_bPaged)
	{
		m_bPaged = TRUE;
		m_bPagedViaIO = FALSE;
	}
}

void CSpectranet::RETN (void)
{
	m_bNMIFlipFlop = FALSE;
}

boolean CSpectranet::IsPort (u16 nPort) const
{
	return nPort == 0x003B || nPort == 0x013B || nPort == 0x023B || nPort == 0x033B;
}

u8 CSpectranet::ReadPort (u16 nPort, u8 nULALast, boolean b128Bank)
{
	switch (nPort)
	{
	case 0x023B:
		return CPLD_VERSION;

	case 0x033B:
	{
		u8 b = nULALast & 0x07;
		if (m_bTrapActive)
		{
			b |= 0x08;
		}
		if (b128Bank)
		{
			b |= 0x10;
		}
		return b;
	}
	}
	return 0xFF;
}

void CSpectranet::WritePort (u16 nPort, u8 nValue)
{
	switch (nPort)
	{
	case 0x003B:
		MapPage (1, nValue);
		break;

	case 0x013B:
		MapPage (2, nValue);
		break;

	case 0x023B:
		if (m_bTrapWriteMSB)
		{
			m_nTrap = (m_nTrap & 0x00FF) | (nValue << 8);
		}
		else
		{
			m_nTrap = (m_nTrap & 0xFF00) | nValue;
		}
		m_bTrapWriteMSB = !m_bTrapWriteMSB;
		break;

	case 0x033B:
		m_nControl = nValue;
		if (nValue & 0x01)
		{
			m_bPaged = TRUE;
			m_bPagedViaIO = TRUE;
		}
		else if (m_bPagedViaIO)
		{
			m_bPaged = FALSE;
			m_bPagedViaIO = FALSE;
		}
		m_bTrapActive = (nValue & 0x08) != 0;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Flash AM29F010 (sectores de 16 KB)

u8 CSpectranet::FlashRead (u32 nOffset)
{
	if (m_FlashState == FlashAutoselect)
	{
		switch (nOffset & 0xFF)
		{
		case 0x00:	return 0x01;	// AMD
		case 0x01:	return 0x20;	// Am29F010B
		case 0x02:	return 0x00;	// sin protección
		default:	return 0xFF;
		}
	}
	return m_pFlash[nOffset & (SNET_FLASH_SIZE - 1)];
}

void CSpectranet::FlashWrite (u8 nSector, u16 nAddress, u8 nValue)
{
	u16 nCmd = nAddress & 0x7FF;

	switch (m_FlashState)
	{
	case FlashReset:
		if (nCmd == 0x555 && nValue == 0xAA)	m_FlashState = FlashC2;
		break;

	case FlashC2:
		m_FlashState = (nCmd == 0x2AA && nValue == 0x55) ? FlashC3 : FlashReset;
		break;

	case FlashC3:
		m_FlashState = FlashReset;
		if (nCmd == 0x555)
		{
			if (nValue == 0xA0)		m_FlashState = FlashProgram;
			else if (nValue == 0x80)	m_FlashState = FlashC4;
			else if (nValue == 0x90)	m_FlashState = FlashAutoselect;
		}
		break;

	case FlashC4:
		m_FlashState = (nCmd == 0x555 && nValue == 0xAA) ? FlashC5 : FlashReset;
		break;

	case FlashC5:
		m_FlashState = (nCmd == 0x2AA && nValue == 0x55) ? FlashC6 : FlashReset;
		break;

	case FlashC6:
		if (nCmd == 0x555 && nValue == 0x10)		// borrar todo
		{
			memset (m_pFlash, 0xFF, SNET_FLASH_SIZE);
			m_bFlashModified = TRUE;
		}
		else if (nValue == 0x30)			// borrar un sector
		{
			memset (m_pFlash + (u32) nSector * 0x4000, 0xFF, 0x4000);
			m_bFlashModified = TRUE;
		}
		m_FlashState = FlashReset;
		break;

	case FlashProgram:
	{
		u32 nOffset = ((u32) nSector * 0x4000 + nAddress) & (SNET_FLASH_SIZE - 1);
		m_pFlash[nOffset] &= nValue;		// programar solo pasa bits de 1 a 0
		m_bFlashModified = TRUE;
		m_FlashState = FlashReset;
		break;
	}

	case FlashAutoselect:
		break;
	}

	if (nValue == 0xF0)
	{
		m_FlashState = FlashReset;
	}
}

////////////////////////////////////////////////////////////////////////////////
// W5100

void CSpectranet::SocketClean (TSocket *pS)
{
	pS->nIR = 0;
	memset (pS->Port, 0, sizeof pS->Port);
	memset (pS->DIP, 0, sizeof pS->DIP);
	memset (pS->DPort, 0, sizeof pS->DPort);
	pS->nTxRR = pS->nTxWR = 0;
	pS->nRxRSR = pS->nRxRD = pS->nOldRxRD = 0;
	if (pS->nHandle >= 0 && m_pNet != 0)
	{
		m_pNet->Close (pS->nHandle);
	}
	pS->nHandle = -1;
	pS->bBound = FALSE;
	pS->nBindCount = 0;
	pS->bWritePending = FALSE;
	pS->nLastSend = 0;
	pS->nDatagrams = 0;
}

void CSpectranet::W5100Reset (void)
{
	memset (m_GW, 0, sizeof m_GW);
	memset (m_Sub, 0, sizeof m_Sub);
	memset (m_SHA, 0, sizeof m_SHA);
	memset (m_SIP, 0, sizeof m_SIP);
	for (unsigned i = 0; i < 4; i++)
	{
		TSocket *pS = &m_Socket[i];
		SocketClean (pS);
		pS->nMode = MODE_CLOSED;
		pS->nState = SOCK_CLOSED;
		memset (pS->MSS, 0, sizeof pS->MSS);
		pS->nProto = 0;
	}
}

u8 CSpectranet::W5100Read (u16 nReg)
{
	if (nReg < 0x30)
	{
		if (nReg >= 0x01 && nReg <= 0x04)	return m_GW[nReg - 0x01];
		if (nReg >= 0x05 && nReg <= 0x08)	return m_Sub[nReg - 0x05];
		if (nReg >= 0x09 && nReg <= 0x0E)	return m_SHA[nReg - 0x09];
		if (nReg >= 0x0F && nReg <= 0x12)	return m_SIP[nReg - 0x0F];
		if (nReg == 0x16)			return 0xEF;	// IMR
		if (nReg == 0x1A || nReg == 0x1B)	return 0x55;	// 2 KB por socket
		if (nReg == 0x00)			return 0x00;	// MR
		return 0xFF;
	}

	if (nReg >= 0x400 && nReg < 0x800)
	{
		TSocket *pS = &m_Socket[(nReg >> 8) - 4];
		unsigned r = nReg & 0xFF;
		u16 nFSR = 0x800 - (u16) (pS->nTxWR - pS->nTxRR);
		switch (r)
		{
		case 0x00:	return pS->nMode;
		case 0x02:	return pS->nIR;
		case 0x03:	return pS->nState;
		case 0x04: case 0x05:	return pS->Port[r - 0x04];
		case 0x0C: case 0x0D: case 0x0E: case 0x0F:	return pS->DIP[r - 0x0C];
		case 0x10: case 0x11:	return pS->DPort[r - 0x10];
		case 0x12: case 0x13:	return pS->MSS[r - 0x12];
		case 0x14:	return pS->nProto;
		case 0x20:	return nFSR >> 8;
		case 0x21:	return nFSR & 0xFF;
		case 0x22:	return pS->nTxRR >> 8;
		case 0x23:	return pS->nTxRR & 0xFF;
		case 0x24:	return pS->nTxWR >> 8;
		case 0x25:	return pS->nTxWR & 0xFF;
		case 0x26:	return pS->nRxRSR >> 8;
		case 0x27:	return pS->nRxRSR & 0xFF;
		case 0x28:	return pS->nRxRD >> 8;
		case 0x29:	return pS->nRxRD & 0xFF;
		}
		return 0xFF;
	}

	if (nReg >= 0x4000 && nReg < 0x6000)
	{
		return m_Socket[(nReg - 0x4000) / 0x800].TxBuf[nReg & 0x7FF];
	}

	if (nReg >= 0x6000)
	{
		return m_Socket[(nReg - 0x6000) / 0x800].RxBuf[nReg & 0x7FF];
	}

	return 0xFF;
}

void CSpectranet::W5100Write (u16 nReg, u8 nValue)
{
	if (nReg < 0x30)
	{
		if (nReg == 0x00 && (nValue & 0x80))	W5100Reset ();
		else if (nReg >= 0x01 && nReg <= 0x04)	m_GW[nReg - 0x01] = nValue;
		else if (nReg >= 0x05 && nReg <= 0x08)	m_Sub[nReg - 0x05] = nValue;
		else if (nReg >= 0x09 && nReg <= 0x0E)	m_SHA[nReg - 0x09] = nValue;
		else if (nReg >= 0x0F && nReg <= 0x12)	m_SIP[nReg - 0x0F] = nValue;
		return;
	}

	if (nReg >= 0x400 && nReg < 0x800)
	{
		TSocket *pS = &m_Socket[(nReg >> 8) - 4];
		unsigned r = nReg & 0xFF;
		switch (r)
		{
		case 0x00:
		{
			u8 nMode = nValue & 0x0F;
			pS->nMode = (nMode == MODE_TCP || nMode == MODE_UDP) ? nMode : MODE_CLOSED;
			break;
		}
		case 0x01:	SocketCommand (pS, nValue);	break;
		case 0x02:	pS->nIR &= ~nValue;		break;
		case 0x04: case 0x05:
			pS->Port[r - 0x04] = nValue;
			if (++pS->nBindCount == 2)
			{
				if (pS->nState == SOCK_UDP && !pS->bBound)
				{
					SocketBind (pS);
				}
				pS->nBindCount = 0;
			}
			break;
		case 0x0C: case 0x0D: case 0x0E: case 0x0F:	pS->DIP[r - 0x0C] = nValue;	break;
		case 0x10: case 0x11:	pS->DPort[r - 0x10] = nValue;	break;
		case 0x12: case 0x13:	pS->MSS[r - 0x12] = nValue;	break;
		case 0x14:	pS->nProto = nValue;		break;
		case 0x24:	pS->nTxWR = (pS->nTxWR & 0x00FF) | (nValue << 8);	break;
		case 0x25:	pS->nTxWR = (pS->nTxWR & 0xFF00) | nValue;		break;
		case 0x28:	pS->nRxRD = (pS->nRxRD & 0x00FF) | (nValue << 8);	break;
		case 0x29:	pS->nRxRD = (pS->nRxRD & 0xFF00) | nValue;		break;
		}
		if (r != 0x04 && r != 0x05)
		{
			pS->nBindCount = 0;
		}
		return;
	}

	if (nReg >= 0x4000 && nReg < 0x6000)
	{
		m_Socket[(nReg - 0x4000) / 0x800].TxBuf[nReg & 0x7FF] = nValue;
	}
}

boolean CSpectranet::SocketBind (TSocket *pS)
{
	if (pS->bBound)
	{
		return TRUE;
	}
	if (pS->nHandle < 0 || m_pNet == 0
	    || !m_pNet->Bind (pS->nHandle, (pS->Port[0] << 8) | pS->Port[1]))
	{
		// el puerto 68 (cliente DHCP) no hace falta: el DHCP se responde aquí
		u16 nPort = (pS->Port[0] << 8) | pS->Port[1];
		if (nPort == 68)
		{
			pS->bBound = TRUE;
			return TRUE;
		}
		pS->nIR |= IR_TIMEOUT;
		pS->nState = SOCK_CLOSED;
		return FALSE;
	}
	pS->bBound = TRUE;
	return TRUE;
}

void CSpectranet::SocketCommand (TSocket *pS, u8 nCommand)
{
	switch (nCommand)
	{
	case 0x01:		// OPEN
		if ((pS->nMode == MODE_TCP || pS->nMode == MODE_UDP) && pS->nState == SOCK_CLOSED)
		{
			u8 Port[2] = {pS->Port[0], pS->Port[1]};
			u8 DIP[4], DPort[2];
			memcpy (DIP, pS->DIP, 4);
			memcpy (DPort, pS->DPort, 2);
			SocketClean (pS);
			memcpy (pS->Port, Port, 2);		// el puerto se escribe antes de OPEN
			memcpy (pS->DIP, DIP, 4);
			memcpy (pS->DPort, DPort, 2);
			if (m_pNet != 0 && m_pNet->IsAvailable ())
			{
				pS->nHandle = m_pNet->Open (pS->nMode == MODE_TCP);
			}
			// sin red, el socket UDP se abre igual (para que el DHCP funcione)
			if (pS->nHandle >= 0 || pS->nMode == MODE_UDP)
			{
				pS->nState = pS->nMode == MODE_TCP ? SOCK_INIT : SOCK_UDP;
			}
		}
		break;

	case 0x02:		// LISTEN
		if (pS->nState == SOCK_INIT && SocketBind (pS) && m_pNet->Listen (pS->nHandle))
		{
			pS->nState = SOCK_LISTEN;
		}
		break;

	case 0x04:		// CONNECT: se empieza y se termina en Poll (SYNSENT)
		if (pS->nState == SOCK_INIT)
		{
			if (   pS->nHandle >= 0
			    && m_pNet->ConnectStart (pS->nHandle, pS->DIP, (pS->DPort[0] << 8) | pS->DPort[1]))
			{
				pS->nState = SOCK_SYNSENT;
				pS->nConnectPolls = 0;
			}
			else
			{
				pS->nIR |= IR_TIMEOUT;
				pS->nState = SOCK_CLOSED;
			}
			m_bActivity = TRUE;
		}
		break;

	case 0x08:		// DISCON
		if (pS->nState == SOCK_ESTABLISHED || pS->nState == SOCK_CLOSE_WAIT)
		{
			if (pS->nHandle >= 0)
			{
				m_pNet->Close (pS->nHandle);
				pS->nHandle = -1;
			}
			pS->bBound = FALSE;
			pS->bWritePending = FALSE;
			pS->nIR |= IR_DISCON;
			pS->nState = SOCK_CLOSED;
		}
		break;

	case 0x10:		// CLOSE
		if (pS->nHandle >= 0 && m_pNet != 0)
		{
			m_pNet->Close (pS->nHandle);
		}
		pS->nHandle = -1;
		pS->bBound = FALSE;
		pS->bWritePending = FALSE;
		pS->nState = SOCK_CLOSED;
		break;

	case 0x20:		// SEND
		if (pS->nState == SOCK_UDP)
		{
			if (pS->nDatagrams < 32)
			{
				pS->DatagramLen[pS->nDatagrams++] = pS->nTxWR - pS->nLastSend;
			}
			pS->nLastSend = pS->nTxWR;
			pS->bWritePending = TRUE;
		}
		else if (pS->nState == SOCK_ESTABLISHED)
		{
			pS->bWritePending = TRUE;
		}
		Poll ();		// se envía ya (el firmware espera SEND_OK)
		break;

	case 0x40:		// RECV
		if (pS->nState == SOCK_UDP || pS->nState == SOCK_ESTABLISHED)
		{
			pS->nRxRSR -= (u16) (pS->nRxRD - pS->nOldRxRD);
			pS->nOldRxRD = pS->nRxRD;
			if (pS->nRxRSR != 0)
			{
				pS->nIR |= IR_RECV;
			}
		}
		break;
	}
}

void CSpectranet::SocketRxAppend (TSocket *pS, const u8 *pData, unsigned nLength)
{
	unsigned nOffset = (pS->nOldRxRD + pS->nRxRSR) & 0x7FF;
	for (unsigned i = 0; i < nLength; i++)
	{
		pS->RxBuf[(nOffset + i) & 0x7FF] = pData[i];
	}
	pS->nRxRSR += nLength;
	pS->nIR |= IR_RECV;
	m_bActivity = TRUE;
}

// DHCP: el firmware pide configuración por DHCP (UDP 68 -> 67). Se responde
// aquí mismo con la de la Pi: la Spectranet usa la red de la Pi, así que su
// IP es la de la Pi.
boolean CSpectranet::DHCPReply (TSocket *pS, const u8 *pReq, unsigned nLength)
{
	if (nLength < 240 || pReq[0] != 1)
	{
		return FALSE;
	}

	// tipo de mensaje (opción 53)
	u8 nType = 0;
	for (unsigned i = 240; i + 1 < nLength && pReq[i] != 0xFF; )
	{
		if (pReq[i] == 0)
		{
			i++;
			continue;
		}
		if (pReq[i] == 53 && i + 2 < nLength)
		{
			nType = pReq[i + 2];
		}
		i += 2 + pReq[i + 1];
	}
	if (nType != 1 && nType != 3)		// DISCOVER o REQUEST
	{
		return FALSE;
	}

	u8 IP[4], Mask[4], GW[4], DNS[4];
	if (m_pNet == 0 || !m_pNet->GetConfig (IP, Mask, GW, DNS))
	{
		// la red de la Pi aún no está lista: se contesta cuando lo esté
		unsigned nCopy = nLength < sizeof m_DHCPRequest ? nLength : sizeof m_DHCPRequest;
		memcpy (m_DHCPRequest, pReq, nCopy);
		m_nDHCPLength = nCopy;
		m_nDHCPSocket = pS - m_Socket;
		return FALSE;
	}
	m_nDHCPSocket = -1;

	static u8 Reply[300];
	memset (Reply, 0, sizeof Reply);
	unsigned n = 0;
	// cabecera UDP del W5100: IP y puerto de origen y longitud de los datos
	u8 *p = Reply + 8;
	p[0] = 2;			// BOOTREPLY
	p[1] = 1;			// Ethernet
	p[2] = 6;
	memcpy (p + 4, pReq + 4, 4);	// xid
	memcpy (p + 10, pReq + 10, 2);	// flags
	memcpy (p + 16, IP, 4);		// yiaddr
	memcpy (p + 20, GW, 4);		// siaddr
	memcpy (p + 28, pReq + 28, 16);	// chaddr
	p[236] = 0x63; p[237] = 0x82; p[238] = 0x53; p[239] = 0x63;
	n = 240;
	p[n++] = 53; p[n++] = 1; p[n++] = nType == 1 ? 2 : 5;	// OFFER o ACK
	p[n++] = 54; p[n++] = 4; memcpy (p + n, GW, 4); n += 4;
	p[n++] = 51; p[n++] = 4; p[n++] = 0; p[n++] = 1; p[n++] = 0x51; p[n++] = 0x80;	// 1 día
	p[n++] = 1; p[n++] = 4; memcpy (p + n, Mask, 4); n += 4;
	p[n++] = 3; p[n++] = 4; memcpy (p + n, GW, 4); n += 4;
	p[n++] = 6; p[n++] = 4; memcpy (p + n, DNS, 4); n += 4;
	p[n++] = 255;
	while (n < 300 - 8 && n < 272)
	{
		p[n++] = 0;
	}

	memcpy (Reply, GW, 4);		// "servidor" DHCP
	Reply[4] = 0; Reply[5] = 67;
	Reply[6] = n >> 8; Reply[7] = n & 0xFF;

	if ((unsigned) (0x800 - pS->nRxRSR) < n + 8)
	{
		return FALSE;
	}
	SocketRxAppend (pS, Reply, n + 8);
	return TRUE;
}

void CSpectranet::Poll (void)
{
	static u8 Buffer[0x800];

	// DHCP que esperaba a la red de la Pi
	if (m_nDHCPSocket >= 0)
	{
		TSocket *pS = &m_Socket[m_nDHCPSocket];
		if (pS->nState != SOCK_UDP)
		{
			m_nDHCPSocket = -1;
		}
		else if (m_pNet != 0 && m_pNet->IsAvailable ())
		{
			DHCPReply (pS, m_DHCPRequest, m_nDHCPLength);
		}
	}

	for (unsigned i = 0; i < 4; i++)
	{
		TSocket *pS = &m_Socket[i];

		// envíos pendientes
		if (pS->bWritePending && pS->nState == SOCK_UDP)
		{
			while (pS->nDatagrams > 0)
			{
				u16 nLength = pS->DatagramLen[0];
				for (unsigned k = 0; k < nLength; k++)
				{
					Buffer[k] = pS->TxBuf[(pS->nTxRR + k) & 0x7FF];
				}
				u16 nDPort = (pS->DPort[0] << 8) | pS->DPort[1];
				if (nDPort == 67)
				{
					DHCPReply (pS, Buffer, nLength);	// DHCP: respuesta local
				}
				else if (   pS->nHandle >= 0 && SocketBind (pS)
					 && m_pNet->SendTo (pS->nHandle, Buffer, nLength, pS->DIP, nDPort) < 0)
				{
					break;
				}
				pS->nTxRR += nLength;
				for (unsigned k = 1; k < pS->nDatagrams; k++)
				{
					pS->DatagramLen[k - 1] = pS->DatagramLen[k];
				}
				pS->nDatagrams--;
				m_bActivity = TRUE;
			}
			if (pS->nDatagrams == 0)
			{
				pS->bWritePending = FALSE;
				pS->nIR |= IR_SEND_OK;
			}
		}
		else if (pS->bWritePending && pS->nState == SOCK_ESTABLISHED && pS->nHandle >= 0)
		{
			while (pS->nTxRR != pS->nTxWR)
			{
				unsigned nOffset = pS->nTxRR & 0x7FF;
				unsigned nLength = (u16) (pS->nTxWR - pS->nTxRR);
				if (nOffset + nLength > 0x800)
				{
					nLength = 0x800 - nOffset;
				}
				int nSent = m_pNet->Send (pS->nHandle, pS->TxBuf + nOffset, nLength);
				if (nSent <= 0)
				{
					break;
				}
				pS->nTxRR += nSent;
				m_bActivity = TRUE;
			}
			if (pS->nTxRR == pS->nTxWR)
			{
				pS->bWritePending = FALSE;
				pS->nIR |= IR_SEND_OK;
			}
		}

		if (pS->nHandle < 0 || m_pNet == 0)
		{
			continue;
		}

		// conexión saliente en curso (máximo unos 10 s)
		if (pS->nState == SOCK_SYNSENT)
		{
			int nStatus = m_pNet->ConnectStatus (pS->nHandle);
			if (nStatus == 0 && ++pS->nConnectPolls >= 500)
			{
				nStatus = -1;
			}
			if (nStatus > 0)
			{
				pS->nIR |= IR_CON;
				pS->nState = SOCK_ESTABLISHED;
			}
			else if (nStatus < 0)
			{
				m_pNet->Close (pS->nHandle);
				pS->nHandle = -1;
				pS->nIR |= IR_TIMEOUT;
				pS->nState = SOCK_CLOSED;
			}
			continue;
		}

		// conexiones entrantes
		if (pS->nState == SOCK_LISTEN)
		{
			int nNew = m_pNet->Accept (pS->nHandle);
			if (nNew >= 0)
			{
				m_pNet->Close (pS->nHandle);
				pS->nHandle = nNew;
				pS->nState = SOCK_ESTABLISHED;
				pS->nIR |= IR_CON;
			}
			continue;
		}

		// recepción
		unsigned nFree = 0x800 - pS->nRxRSR;
		if (pS->nState == SOCK_UDP && nFree >= 9)
		{
			u8 IP[4];
			u16 nPort;
			int nRead = m_pNet->RecvFrom (pS->nHandle, Buffer + 8, nFree - 8, IP, &nPort);
			if (nRead > 0)
			{
				memcpy (Buffer, IP, 4);
				Buffer[4] = nPort >> 8;
				Buffer[5] = nPort & 0xFF;
				Buffer[6] = nRead >> 8;
				Buffer[7] = nRead & 0xFF;
				SocketRxAppend (pS, Buffer, nRead + 8);
			}
		}
		else if (pS->nState == SOCK_ESTABLISHED && nFree >= 1)
		{
			int nRead = m_pNet->Recv (pS->nHandle, Buffer, nFree);
			if (nRead > 0)
			{
				SocketRxAppend (pS, Buffer, nRead);
			}
			else if (nRead < 0)
			{
				pS->nState = SOCK_CLOSE_WAIT;
				pS->nIR |= IR_DISCON;
			}
		}
	}
}
