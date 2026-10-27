//
// zxnet.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "zxnet.h"
#include <circle/util.h>

// Tiempos en T-states (a 3,5 MHz), los de SpectRusty
#define INPAK_WAIT_MAX		8925
#define OUTPAK_START_DELAY	110
#define BIT_DELAY		60
#define REST_DELAY_THRESHOLD	64
#define PROBE_DELAY_MIN		65
#define PROBE_DELAY_MAX		130
#define BYTE_DELAY		120
#define BROADCAST_DATA_DELAY	530

#define KIND_NEW_DATA		0
#define KIND_ACCEPTED		1
#define ACCEPT_SIZE		(ZXNET_TAG_SIZE + ZXNET_HEAD_SIZE)

static u8 Rotr (u8 n)
{
	return (u8) ((n >> 1) | (n << 7));
}

////////////////////////////////////////////////////////////////////////////////

CZXNetSocket::CZXNetSocket (void)
:	m_pTransport (0),
	m_nLength (ZXNET_TAG_SIZE),
	m_nPosition (ZXNET_TAG_SIZE),
	m_bHaveAccepted (FALSE)
{
	memset (m_Packet, 0, sizeof m_Packet);
	memset (m_LastAccepted, 0, sizeof m_LastAccepted);
}

void CZXNetSocket::BeginPacket (void)
{
	memcpy (m_Packet, "ZXNET", 5);
	m_Packet[5] = KIND_NEW_DATA;
	m_nLength = ZXNET_TAG_SIZE;
	m_nPosition = ZXNET_TAG_SIZE;
}

unsigned CZXNetSocket::PushByte (u8 nByte)
{
	if (m_nLength < sizeof m_Packet)
	{
		m_Packet[m_nLength++] = nByte;
	}
	return m_nLength - ZXNET_TAG_SIZE;
}

void CZXNetSocket::SendPacket (void)
{
	if (m_pTransport != 0)
	{
		m_pTransport->Send (m_Packet, m_nLength);
	}
}

boolean CZXNetSocket::RecvAccept (void)
{
	if (m_pTransport == 0 || m_nLength <= ACCEPT_SIZE)
	{
		return FALSE;
	}

	u8 Buffer[ZXNET_MAX_PACKET];
	unsigned n;
	while ((n = m_pTransport->Receive (Buffer, sizeof Buffer, TRUE)) > 0)
	{
		if (   n == ACCEPT_SIZE && memcmp (Buffer, "ZXNET", 5) == 0
		    && Buffer[5] == KIND_ACCEPTED
		    && memcmp (Buffer + ZXNET_TAG_SIZE, m_Packet + ZXNET_TAG_SIZE, ZXNET_HEAD_SIZE) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

boolean CZXNetSocket::RecvPacket (void)
{
	if (m_pTransport == 0)
	{
		return FALSE;
	}

	u8 Buffer[ZXNET_MAX_PACKET];
	unsigned n;
	while ((n = m_pTransport->Receive (Buffer, sizeof Buffer, FALSE)) > 0)
	{
		if (n <= ACCEPT_SIZE || memcmp (Buffer, "ZXNET", 5) != 0 || Buffer[5] != KIND_NEW_DATA)
		{
			continue;
		}
		// un paquete repetido que ya aceptamos: se vuelve a confirmar
		if (   m_bHaveAccepted
		    && memcmp (m_LastAccepted + ZXNET_TAG_SIZE, Buffer + ZXNET_TAG_SIZE, ZXNET_HEAD_SIZE) == 0)
		{
			SendLastAccepted ();
			continue;
		}
		memcpy (m_Packet, Buffer, n);
		m_nLength = n;
		m_nPosition = ZXNET_TAG_SIZE;
		return TRUE;
	}
	return FALSE;
}

boolean CZXNetSocket::PullByte (u8 *pByte)
{
	if (m_nPosition >= m_nLength)
	{
		return FALSE;
	}
	*pByte = m_Packet[m_nPosition++];
	return TRUE;
}

void CZXNetSocket::SendAccept (void)
{
	memcpy (m_LastAccepted, m_Packet, ACCEPT_SIZE);
	m_LastAccepted[5] = KIND_ACCEPTED;
	m_bHaveAccepted = TRUE;
	SendLastAccepted ();
}

void CZXNetSocket::SendLastAccepted (void)
{
	if (m_pTransport != 0)
	{
		m_pTransport->Send (m_LastAccepted, ACCEPT_SIZE);
	}
}

////////////////////////////////////////////////////////////////////////////////

CZXNet::CZXNet (void)
{
	Reset ();
}

void CZXNet::Reset (void)
{
	m_nEventTime = 0;
	m_bOutbound = FALSE;
	m_State = Idle;
	m_nArg = 0;
	m_bNetState = FALSE;
}

void CZXNet::SendState (boolean bNet, u64 nTime)
{
	switch (m_State)
	{
	case Idle:
		m_bNetState = bNet;
		m_State = OutputScout;
		return;

	case OutputScout:
		if (bNet && !m_bNetState)	// empieza OUTPAK
		{
			m_Socket.BeginPacket ();
			m_bOutbound = TRUE;
			m_nEventTime = nTime + OUTPAK_START_DELAY;
			m_State = OutputStart;
		}
		else
		{
			m_bNetState = FALSE;
			m_State = Idle;
			m_nArg = 0;
		}
		return;

	case InputData:
		if (m_nArg == 0 && bNet && nTime >= m_nEventTime)	// respuesta o datos: OUTPAK
		{
			m_nEventTime = nTime + OUTPAK_START_DELAY;
			m_State = OutputStart;
			return;
		}
		break;

	case OutputStart:
		if (!bNet && nTime < m_nEventTime)
		{
			m_nEventTime = nTime + BIT_DELAY;
			m_State = OutputData;
			m_nArg = 0x80;
			return;
		}
		break;

	case OutputData:
		if (nTime < m_nEventTime)
		{
			u8 nBits = m_nArg;
			u8 nNext = Rotr ((u8) ((nBits & ~1) | (bNet ? 1 : 0)));
			m_nEventTime = nTime + BIT_DELAY;
			m_State = (nBits & 1) ? OutputStop : OutputData;
			m_nArg = nNext;
			return;
		}
		break;

	case OutputStop:
		if (bNet && nTime < m_nEventTime)
		{
			u8 nByte = m_nArg;
			if (!m_bOutbound)
			{
				if (nByte == 1)
				{
					if (m_Socket.InboundIndex () == ZXNET_HEAD_SIZE)
					{
						m_Socket.SendAccept ();
					}
					m_State = OutputEnd;
				}
				else
				{
					m_bNetState = FALSE;
					m_State = Idle;		// fin del paquete
					m_nArg = 0;
				}
			}
			else
			{
				unsigned nLen = m_Socket.PushByte (nByte);
				const u8 *pHead = m_Socket.PacketData ();
				if (nLen == ZXNET_HEAD_SIZE)
				{
					m_State = OutputEnd;
				}
				else if (nLen > ZXNET_HEAD_SIZE && nLen - ZXNET_HEAD_SIZE == pHead[5])
				{
					m_Socket.SendPacket ();
					m_State = OutputEnd;
				}
				else
				{
					m_State = OutputStart;
				}
			}
			m_nEventTime = nTime + BYTE_DELAY;
			return;
		}
		break;

	case OutputEnd:
		if (!bNet && nTime < m_nEventTime)	// fin de OUTPAK
		{
			m_nEventTime = nTime;
			const u8 *pHead = m_Socket.PacketData ();
			if (!m_bOutbound)
			{
				if (m_Socket.InboundIndex () == pHead[5] + (unsigned) ZXNET_HEAD_SIZE)
				{
					m_bNetState = FALSE;
					m_State = Idle;
					m_nArg = 0;
				}
				else
				{
					m_bNetState = TRUE;
					m_State = InputScout;	// vienen los datos
				}
			}
			else
			{
				if (m_Socket.OutboundIndex () == pHead[5] + (unsigned) ZXNET_HEAD_SIZE)
				{
					m_bNetState = FALSE;
					if (pHead[0] == 0)		// difusión: sin confirmación
					{
						m_State = Idle;
						m_nArg = 0;
					}
					else
					{
						m_State = InputScout;	// espera la confirmación
					}
				}
				else if (pHead[0] == 0)
				{
					m_bNetState = FALSE;
					m_State = InputData;	// siguiente OUTPAK
					m_nArg = 0;
				}
				else
				{
					m_bNetState = TRUE;
					m_State = InputScout;	// espera la confirmación de la cabecera
				}
			}
			return;
		}
		break;

	default:
		break;
	}

	// algo no cuadra: la rutina de sondeo sabrá que no es el momento
	m_State = Idle;
	m_nArg = 0;
	m_bNetState = FALSE;
}

boolean CZXNet::PollState (u64 nTime)
{
	switch (m_State)
	{
	case Idle:
		if (nTime >= m_nEventTime)
		{
			u64 nDiff = nTime - m_nEventTime;
			if (nDiff <= REST_DELAY_THRESHOLD)
			{
				m_nEventTime = nTime;
				if (m_nArg < 255)
				{
					m_nArg++;
				}
			}
			else if (nDiff >= PROBE_DELAY_MIN && nDiff <= PROBE_DELAY_MAX && m_nArg < 191)	// espera de exploración
			{
				m_nEventTime = nTime;
				if (m_Socket.RecvPacket ())
				{
					m_State = InputScout;
					m_bOutbound = FALSE;
					m_bNetState = TRUE;
				}
			}
			else
			{
				m_nEventTime = nTime;
				m_nArg = 0;
				m_bNetState = FALSE;
			}
		}
		else if (m_nArg != 0)
		{
			m_nArg = 0;
		}
		break;

	case OutputScout:
		m_State = Idle;
		m_nArg = 0;
		break;

	case InputScout:
		if (m_bNetState)
		{
			m_nEventTime = nTime;
			m_State = InputStart;
		}
		else if (m_Socket.RecvAccept ())
		{
			m_nEventTime = nTime;
			m_State = InputStart;
			m_bNetState = TRUE;
		}
		else
		{
			m_nEventTime = nTime;
			m_State = Idle;
			m_nArg = 0;
		}
		break;

	case InputStart:		// no debería pasar: se descarta y a reposo
		m_nEventTime = nTime + INPAK_WAIT_MAX;
		m_State = Idle;
		m_nArg = 0;
		m_bNetState = FALSE;
		break;

	case InputData:
		if (nTime < m_nEventTime)
		{
			u8 nByte = m_nArg;
			m_nEventTime = nTime + BIT_DELAY;
			m_nArg = nByte >> 1;
			m_bNetState = (nByte & 1) != 0;
		}
		else if (nTime - m_nEventTime <= BROADCAST_DATA_DELAY)
		{
			m_nEventTime = nTime;
			m_State = InputStart;
			m_bNetState = TRUE;
		}
		else
		{
			m_State = Idle;
			m_nArg = 0;
			m_bNetState = FALSE;
		}
		break;

	default:			// inesperado durante la salida
		m_State = Idle;
		m_nArg = 0;
		m_bNetState = FALSE;
		break;
	}

	return m_bNetState;
}

void CZXNet::WaitData (u64 nTime)
{
	u8 nByte = 0;
	boolean bHave = FALSE;

	if (m_State == InputStart && m_bOutbound)
	{
		nByte = 1;
		bHave = TRUE;
	}
	else if (m_State == InputStart && !m_bOutbound)
	{
		bHave = m_Socket.PullByte (&nByte);
	}
	else if (   m_State == InputData && m_nArg == 0 && !m_bOutbound
		 && nTime > m_nEventTime && nTime < m_nEventTime + BIT_DELAY)
	{
		bHave = m_Socket.PullByte (&nByte);
	}

	if (bHave)
	{
		m_nEventTime = nTime + 2 * BIT_DELAY;
		m_State = InputData;
		m_nArg = nByte;
	}
	else
	{
		m_nEventTime = nTime + INPAK_WAIT_MAX;
		m_State = Idle;
		m_nArg = 0;
		m_bNetState = FALSE;
	}
}
