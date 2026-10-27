//
// circleglue.cpp - Lo que Mbed TLS y el cliente HTTPS necesitan de Circle:
// sockets TCP con resolución DNS, números aleatorios del hardware y formato
// de texto (snprintf).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "../network.h"
#include "circleglue.h"
#include <circle/net/netsubsystem.h>
#include <circle/net/dnsclient.h>
#include <circle/net/in.h>
#include <circle/netdevice.h>
#include <circle/bcmrandom.h>
#include <circle/string.h>
#include <circle/util.h>
#include <stdarg.h>

CCircleTransport::CCircleTransport (void)
:	m_pSocket (0)
{
}

CCircleTransport::~CCircleTransport (void)
{
	Close ();
}

bool CCircleTransport::Connect (const char *pHost, unsigned nPort)
{
	Close ();

	CNetSubSystem *pNet = CNetworkManager::GetNet ();	// no CNetSubSystem::Get (): assert sin red
	if (pNet == 0 || !pNet->IsRunning ())
	{
		return false;
	}

	CIPAddress Address;
	CDNSClient DNS (pNet);
	if (!DNS.Resolve (pHost, &Address))
	{
		return false;
	}

	m_pSocket = new CSocket (pNet, IPPROTO_TCP);
	if (m_pSocket == 0 || m_pSocket->Connect (Address, (u16) nPort) < 0)
	{
		Close ();
		return false;
	}

	return true;
}

int CCircleTransport::Send (const unsigned char *pData, size_t nLength)
{
	if (m_pSocket == 0)
	{
		return -1;
	}
	// los paquetes de Circle tienen un tamaño máximo: se envía por trozos
	size_t nSent = 0;
	while (nSent < nLength)
	{
		size_t nChunk = nLength - nSent > 1400 ? 1400 : nLength - nSent;
		int r = m_pSocket->Send (pData + nSent, nChunk, 0);
		if (r <= 0)
		{
			return -1;
		}
		nSent += r;
	}
	return (int) nSent;
}

int CCircleTransport::Receive (unsigned char *pBuffer, size_t nSize)
{
	if (m_pSocket == 0)
	{
		return -1;
	}
	// Receive necesita espacio para un paquete entero (FRAME_BUFFER_SIZE)
	static u8 Frame[FRAME_BUFFER_SIZE];
	static size_t nFrameLen = 0, nFramePos = 0;
	static CSocket *pFrameSocket = 0;
	if (pFrameSocket != m_pSocket)
	{
		nFrameLen = nFramePos = 0;
		pFrameSocket = m_pSocket;
	}

	if (nFramePos >= nFrameLen)
	{
		int r = m_pSocket->Receive (Frame, sizeof Frame, 0);	// bloquea cediendo el control
		if (r <= 0)
		{
			return r < 0 ? -1 : 0;
		}
		nFrameLen = r;
		nFramePos = 0;
	}

	size_t n = nFrameLen - nFramePos;
	if (n > nSize)
	{
		n = nSize;
	}
	memcpy (pBuffer, Frame + nFramePos, n);
	nFramePos += n;
	return (int) n;
}

void CCircleTransport::Close (void)
{
	delete m_pSocket;
	m_pSocket = 0;
}

//------------------------------------------------------------------------------
// Para Mbed TLS (ver third_party/mbedtls/zxmc_config.h)

extern "C"
{

int zxmc_vsnprintf (char *s, size_t n, const char *fmt, va_list ap)
{
	CString Text;
	Text.FormatV (fmt, ap);
	size_t l = Text.GetLength ();
	if (n > 0)
	{
		size_t c = l < n - 1 ? l : n - 1;
		memcpy (s, (const char *) Text, c);
		s[c] = '\0';
	}
	return (int) l;
}

int zxmc_snprintf (char *s, size_t n, const char *fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	int r = zxmc_vsnprintf (s, n, fmt, ap);
	va_end (ap);
	return r;
}

int zxmc_printf (const char *, ...)		{ return 0; }
int zxmc_fprintf (void *, const char *, ...)	{ return 0; }
void zxmc_exit (int)				{ for (;;); }

int mbedtls_hardware_poll (void *, unsigned char *pOutput, size_t nLength, size_t *pnOut)
{
	static CBcmRandomNumberGenerator Random;
	for (size_t i = 0; i < nLength; i += 4)
	{
		u32 n = Random.GetNumber ();
		for (unsigned k = 0; k < 4 && i + k < nLength; k++)
		{
			pOutput[i + k] = (u8) (n >> (8 * k));
		}
	}
	*pnOut = nLength;
	return 0;
}

}
