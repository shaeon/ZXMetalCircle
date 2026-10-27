//
// if1link.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "if1link.h"
#include "network.h"
#include <circle/net/in.h>
#include <circle/net/ipaddress.h>
#include <circle/net/netsubsystem.h>
#include <circle/sched/task.h>
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <fatfs/ff.h>

static const char From[] = "if1link";

// Espera clientes del RS232 sin detener la emulación (el Accept de Circle bloquea)
class CRS232AcceptTask : public CTask
{
public:
	CRS232AcceptTask (CSocket *pListen) : m_pListen (pListen), m_pNew (0) {}

	void Run (void)
	{
		for (;;)
		{
			CIPAddress IP;
			u16 nPort;
			CSocket *pNew = m_pListen->Accept (&IP, &nPort);
			if (pNew == 0)
			{
				continue;
			}
			while (m_pNew != 0)		// el anterior aún no se ha recogido
			{
				CScheduler::Get ()->MsSleep (10);
			}
			m_pNew = pNew;
		}
	}

	CSocket *Take (void)
	{
		CSocket *p = m_pNew;
		m_pNew = 0;
		return p;
	}

private:
	CSocket *m_pListen;
	CSocket *volatile m_pNew;
};

CIF1Link::CIF1Link (void)
:	m_bRS232 (TRUE),
	m_bZXNet (TRUE),
	m_bStarted (FALSE),
	m_pListen (0),
	m_pAccept (0),
	m_pClient (0),
	m_pUDP (0),
	m_nFileBuffer (0),
	m_nFileIdle (0)
{
	memset (m_OwnIP, 0, sizeof m_OwnIP);
}

CIF1Link::~CIF1Link (void)
{
	delete m_pClient;
	delete m_pUDP;
}

void CIF1Link::StartNetwork (void)
{
	CNetSubSystem *pNet = CNetworkManager::GetNet ();
	if (pNet == 0 || !CNetworkManager::IsNetRunning ())
	{
		return;
	}
	m_bStarted = TRUE;

	pNet->GetConfig ()->GetIPAddress ()->CopyTo (m_OwnIP);

	if (m_bRS232)
	{
		m_pListen = new CSocket (pNet, IPPROTO_TCP);
		if (m_pListen->Bind (RS232_TCP_PORT) >= 0 && m_pListen->Listen (1) >= 0)
		{
			m_pAccept = new CRS232AcceptTask (m_pListen);
			CLogger::Get ()->Write (From, LogNotice, "RS232 en el puerto TCP %u", RS232_TCP_PORT);
		}
	}

	if (m_bZXNet)
	{
		m_pUDP = new CSocket (pNet, IPPROTO_UDP);
		if (m_pUDP->Bind (ZXNET_UDP_PORT) < 0)
		{
			delete m_pUDP;
			m_pUDP = 0;
		}
		else
		{
			CLogger::Get ()->Write (From, LogNotice, "ZX Net en el puerto UDP %u", ZXNET_UDP_PORT);
		}
	}
}

void CIF1Link::FlushFile (void)
{
	if (m_nFileBuffer == 0)
	{
		return;
	}
	f_mkdir ("SD:/saves");
	FIL File;
	if (f_open (&File, "SD:/saves/rs232.txt", FA_WRITE | FA_OPEN_APPEND) == FR_OK)
	{
		UINT nWritten;
		f_write (&File, m_FileBuffer, m_nFileBuffer, &nWritten);
		f_close (&File);
	}
	m_nFileBuffer = 0;
}

void CIF1Link::Poll (CInterface1 *pIF1)
{
	if (!m_bStarted)
	{
		StartNetwork ();
	}

	// RS232: cliente nuevo (sustituye al anterior)
	if (m_pAccept != 0)
	{
		CSocket *pNew = m_pAccept->Take ();
		if (pNew != 0)
		{
			delete m_pClient;
			m_pClient = pNew;
			CLogger::Get ()->Write (From, LogNotice, "RS232: cliente conectado");
		}
	}

	// RS232: salida del Spectrum
	CByteQueue *pOut = pIF1->GetRS232Out ();
	u8 Buffer[256];
	unsigned n = 0;
	while (n < sizeof Buffer && pOut->Get (&Buffer[n]))
	{
		n++;
	}
	if (n > 0)
	{
		if (m_pClient != 0 && m_pClient->Send (Buffer, n, 0) < 0)
		{
			delete m_pClient;		// el cliente se ha ido
			m_pClient = 0;
		}
		if (m_pClient == 0)
		{
			for (unsigned i = 0; i < n; i++)
			{
				if (m_nFileBuffer == sizeof m_FileBuffer)
				{
					FlushFile ();
				}
				m_FileBuffer[m_nFileBuffer++] = Buffer[i];
			}
			m_nFileIdle = 0;
		}
	}
	else if (m_nFileBuffer > 0 && ++m_nFileIdle >= 50)	// 1 s sin salida
	{
		FlushFile ();
	}

	// RS232: entrada desde el cliente
	if (m_pClient != 0)
	{
		CByteQueue *pIn = pIF1->GetRS232In ();
		int nRead = m_pClient->Receive (Buffer, sizeof Buffer, MSG_DONTWAIT);
		if (nRead < 0)
		{
			delete m_pClient;
			m_pClient = 0;
		}
		for (int i = 0; i < nRead; i++)
		{
			pIn->Put (Buffer[i]);
		}
	}
}

void CIF1Link::Send (const u8 *pData, unsigned nLength)
{
	if (m_pUDP != 0)
	{
		static const u8 Broadcast[4] = {255, 255, 255, 255};
		CIPAddress IP (Broadcast);
		m_pUDP->SendTo (pData, nLength, MSG_DONTWAIT, IP, ZXNET_UDP_PORT);
	}
}

unsigned CIF1Link::Receive (u8 *pBuffer, unsigned nSize, boolean bWait)
{
	if (m_pUDP == 0)
	{
		return 0;
	}

	unsigned nStart = CTimer::GetClockTicks ();
	for (;;)
	{
		CIPAddress IP;
		u16 nPort;
		int n = m_pUDP->ReceiveFrom (pBuffer, nSize, MSG_DONTWAIT, &IP, &nPort);
		if (n > 0)
		{
			u8 From[4];
			IP.CopyTo (From);
			if (memcmp (From, m_OwnIP, 4) == 0)
			{
				continue;		// nuestra propia difusión
			}
			return n;
		}
		// confirmación del otro lado: se espera hasta 50 ms (como SpectRusty)
		if (!bWait || CTimer::GetClockTicks () - nStart >= 50000)
		{
			return 0;
		}
		CScheduler::Get ()->Yield ();
	}
}
