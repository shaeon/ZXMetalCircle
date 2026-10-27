//
// snetcircle.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "snetcircle.h"
#include "network.h"
#include <circle/net/in.h>
#include <circle/net/ipaddress.h>
#include <circle/net/netsubsystem.h>
#include <circle/sched/task.h>
#include <circle/util.h>

// Espera una conexión entrante sin detener la emulación
class CAcceptTask : public CTask
{
public:
	CAcceptTask (CSocket *pListen)
	:	m_pListen (pListen), m_pNew (0), m_bDone (FALSE), m_bStop (FALSE)
	{
	}

	void Run (void)
	{
		CIPAddress IP;
		u16 nPort;
		CSocket *pNew = m_pListen->Accept (&IP, &nPort);
		if (m_bStop)
		{
			delete pNew;
		}
		else
		{
			m_pNew = pNew;
		}
		m_bDone = TRUE;
	}

	CSocket *Take (void)
	{
		CSocket *p = m_pNew;
		m_pNew = 0;
		return p;
	}

	boolean IsDone (void) const	{ return m_bDone; }
	void Stop (void)		{ m_bStop = TRUE; }

private:
	CSocket *m_pListen;
	CSocket *volatile m_pNew;
	volatile boolean m_bDone;
	volatile boolean m_bStop;
};

// Conecta en segundo plano: el Connect de Circle bloquea (y con 127.0.0.1, que
// Circle no tiene, no vuelve nunca), así que la emulación sigue mientras tanto
class CConnectTask : public CTask
{
public:
	CConnectTask (CSocket *pSocket, const u8 *pIP, u16 nPort)
	:	m_pSocket (pSocket), m_nPort (nPort), m_nStatus (0), m_bAbandoned (FALSE)
	{
		memcpy (m_IP, pIP, 4);
	}

	void Run (void)
	{
		CIPAddress IP (m_IP);
		int nResult = m_pSocket->Connect (IP, m_nPort);
		if (m_bAbandoned)
		{
			delete m_pSocket;	// ya nadie lo quiere
		}
		m_nStatus = nResult >= 0 ? 1 : -1;
	}

	int GetStatus (void) const	{ return m_nStatus; }
	void Abandon (void)		{ m_bAbandoned = TRUE; }

private:
	CSocket *m_pSocket;
	u8 m_IP[4];
	u16 m_nPort;
	volatile int m_nStatus;
	volatile boolean m_bAbandoned;
};

CSpectranetCircleNet::CSpectranetCircleNet (void)
{
	for (unsigned i = 0; i < SNET_MAX_HANDLES; i++)
	{
		m_pSocket[i] = 0;
		m_pAccept[i] = 0;
		m_pConnect[i] = 0;
	}
}

// Direcciones a las que no se puede llegar: 127.x.x.x (Circle no tiene
// interfaz local), 0.x.x.x y la propia IP de la Pi
boolean CSpectranetCircleNet::IsUnreachable (const u8 *pIP)
{
	if (pIP[0] == 127 || pIP[0] == 0)
	{
		return TRUE;
	}
	CNetSubSystem *pNet = CNetworkManager::GetNet ();
	if (pNet != 0)
	{
		u8 Own[4];
		pNet->GetConfig ()->GetIPAddress ()->CopyTo (Own);
		if (memcmp (Own, pIP, 4) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

CSpectranetCircleNet::~CSpectranetCircleNet (void)
{
	for (int i = 0; i < SNET_MAX_HANDLES; i++)
	{
		Close (i);
	}
}

boolean CSpectranetCircleNet::IsAvailable (void)
{
	return CNetworkManager::IsNetRunning ();
}

int CSpectranetCircleNet::NewHandle (CSocket *pSocket)
{
	for (int i = 0; i < SNET_MAX_HANDLES; i++)
	{
		if (m_pSocket[i] == 0)
		{
			m_pSocket[i] = pSocket;
			return i;
		}
	}
	delete pSocket;
	return -1;
}

int CSpectranetCircleNet::Open (boolean bTCP)
{
	CNetSubSystem *pNet = CNetworkManager::GetNet ();
	if (pNet == 0)
	{
		return -1;
	}
	return NewHandle (new CSocket (pNet, bTCP ? IPPROTO_TCP : IPPROTO_UDP));
}

boolean CSpectranetCircleNet::Bind (int h, u16 nPort)
{
	CSocket *p = Get (h);
	return p != 0 && p->Bind (nPort) >= 0;
}

boolean CSpectranetCircleNet::ConnectStart (int h, const u8 *pIP, u16 nPort)
{
	CSocket *p = Get (h);
	if (p == 0 || IsUnreachable (pIP) || m_pConnect[h] != 0)
	{
		return FALSE;
	}
	m_pConnect[h] = new CConnectTask (p, pIP, nPort);
	return TRUE;
}

int CSpectranetCircleNet::ConnectStatus (int h)
{
	if (h < 0 || h >= SNET_MAX_HANDLES || m_pConnect[h] == 0)
	{
		return Get (h) != 0 ? 1 : -1;
	}
	int nStatus = m_pConnect[h]->GetStatus ();
	if (nStatus != 0)
	{
		m_pConnect[h] = 0;	// la tarea ha terminado (el planificador la libera)
	}
	return nStatus;
}

boolean CSpectranetCircleNet::Listen (int h)
{
	CSocket *p = Get (h);
	if (p == 0 || p->Listen (1) < 0)
	{
		return FALSE;
	}
	m_pAccept[h] = new CAcceptTask (p);
	return TRUE;
}

int CSpectranetCircleNet::Accept (int h)
{
	CAcceptTask *pTask = h >= 0 && h < SNET_MAX_HANDLES ? m_pAccept[h] : 0;
	if (pTask == 0 || !pTask->IsDone ())
	{
		return -1;
	}
	CSocket *pNew = pTask->Take ();
	m_pAccept[h] = 0;		// la tarea ya ha terminado (el planificador la libera)
	return pNew != 0 ? NewHandle (pNew) : -1;
}

int CSpectranetCircleNet::Send (int h, const u8 *pData, unsigned nLength)
{
	CSocket *p = Get (h);
	if (p == 0)
	{
		return -1;
	}
	int n = p->Send (pData, nLength, MSG_DONTWAIT);
	return n < 0 ? 0 : n;
}

int CSpectranetCircleNet::SendTo (int h, const u8 *pData, unsigned nLength, const u8 *pIP, u16 nPort)
{
	CSocket *p = Get (h);
	if (p == 0)
	{
		return -1;
	}
	if (IsUnreachable (pIP))
	{
		return nLength;		// se descarta: no hay nadie al otro lado
	}
	CIPAddress IP (pIP);
	return p->SendTo (pData, nLength, MSG_DONTWAIT, IP, nPort);
}

int CSpectranetCircleNet::Recv (int h, u8 *pBuffer, unsigned nSize)
{
	CSocket *p = Get (h);
	if (p == 0)
	{
		return -1;
	}
	return p->Receive (pBuffer, nSize, MSG_DONTWAIT);
}

int CSpectranetCircleNet::RecvFrom (int h, u8 *pBuffer, unsigned nSize, u8 *pIP, u16 *pPort)
{
	CSocket *p = Get (h);
	if (p == 0)
	{
		return 0;
	}
	CIPAddress IP;
	int n = p->ReceiveFrom (pBuffer, nSize, MSG_DONTWAIT, &IP, pPort);
	if (n > 0)
	{
		IP.CopyTo (pIP);
	}
	return n > 0 ? n : 0;
}

void CSpectranetCircleNet::Close (int h)
{
	if (h < 0 || h >= SNET_MAX_HANDLES)
	{
		return;
	}
	if (m_pConnect[h] != 0)
	{
		// conexión en curso: la tarea borrará el socket cuando termine
		if (m_pConnect[h]->GetStatus () == 0)
		{
			m_pConnect[h]->Abandon ();
			m_pConnect[h] = 0;
			m_pSocket[h] = 0;
			return;
		}
		m_pConnect[h] = 0;
	}
	if (m_pAccept[h] != 0)
	{
		// la tarea sigue esperando en Accept: se la deja terminar sola
		m_pAccept[h]->Stop ();
		m_pAccept[h] = 0;
		m_pSocket[h] = 0;	// el socket de escucha queda con la tarea
		return;
	}
	delete m_pSocket[h];
	m_pSocket[h] = 0;
}

boolean CSpectranetCircleNet::GetConfig (u8 *pIP, u8 *pMask, u8 *pGateway, u8 *pDNS)
{
	CNetSubSystem *pNet = CNetworkManager::GetNet ();
	if (pNet == 0 || !CNetworkManager::IsNetRunning ())
	{
		return FALSE;
	}
	CNetConfig *pConfig = pNet->GetConfig ();
	pConfig->GetIPAddress ()->CopyTo (pIP);
	memcpy (pMask, pConfig->GetNetMask (), 4);
	pConfig->GetDefaultGateway ()->CopyTo (pGateway);
	pConfig->GetDNSServer ()->CopyTo (pDNS);
	return TRUE;
}
