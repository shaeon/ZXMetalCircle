//
// tnfsbrowser.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "tnfsbrowser.h"
#include "i18n.h"
#include "network.h"
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/net/dnsclient.h>
#include <circle/net/ipaddress.h>
#include <circle/net/in.h>
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <circle/string.h>
#include <circle/util.h>
#include <fatfs/ff.h>

#define SERVERS_FILE	"SD:/tnfs.txt"

// Servidores públicos de Spectrum conocidos (se pueden cambiar en tnfs.txt)
static const char *DefaultServers[] =
{
	"tnfs.markround.com",		// más de 16.000 juegos, demos y utilidades
	"vexed4.alioth.net",		// el del autor de Spectranet
	"tnfs.millhill.org",		// archivo del antiguo zx.kupo.be
	"tnfs.bytedelight.com",
};

////////////////////////////////////////////////////////////////////////////////

CCircleUDPTransport::CCircleUDPTransport (void)
:	m_pSocket (0)
{
}

CCircleUDPTransport::~CCircleUDPTransport (void)
{
	Close ();
}

boolean CCircleUDPTransport::Open (const char *pHost, u16 nPort)
{
	Close ();

	CNetSubSystem *pNet = CNetworkManager::GetNet ();
	if (pNet == 0 || !pNet->IsRunning ())
	{
		return FALSE;
	}

	CIPAddress IP;
	CDNSClient DNS (pNet);
	if (!DNS.Resolve (pHost, &IP))
	{
		return FALSE;
	}

	m_pSocket = new CSocket (pNet, IPPROTO_UDP);
	if (m_pSocket == 0 || m_pSocket->Connect (IP, nPort) < 0)
	{
		Close ();
		return FALSE;
	}

	return TRUE;
}

boolean CCircleUDPTransport::Send (const u8 *pData, unsigned nLength)
{
	return m_pSocket != 0 && m_pSocket->Send (pData, nLength, 0) == (int) nLength;
}

unsigned CCircleUDPTransport::Receive (u8 *pBuffer, unsigned nSize, unsigned nTimeoutMs)
{
	if (m_pSocket == 0)
	{
		return 0;
	}

	u64 nEnd = CTimer::GetClockTicks64 () + (u64) nTimeoutMs * 1000;
	do
	{
		int nResult = m_pSocket->Receive (pBuffer, nSize, MSG_DONTWAIT);
		if (nResult > 0)
		{
			return nResult;
		}
		CScheduler::Get ()->MsSleep (1);	// deja trabajar a la red
	}
	while (CTimer::GetClockTicks64 () < nEnd);

	return 0;
}

void CCircleUDPTransport::Close (void)
{
	delete m_pSocket;
	m_pSocket = 0;
}

////////////////////////////////////////////////////////////////////////////////

CTNFSBrowser::CTNFSBrowser (void)
:	m_Client (&m_Transport),
	m_nServers (0),
	m_pError (""),
	m_pBusy (0),
	m_pBusyParam (0)
{
}

void CTNFSBrowser::LoadServers (void)
{
	m_nServers = 0;

	FIL File;
	if (f_open (&File, SERVERS_FILE, FA_READ | FA_OPEN_EXISTING) == FR_OK)
	{
		char Line[128];
		while (m_nServers < MaxServers && f_gets (Line, sizeof Line, &File) != 0)
		{
			// sin espacios ni comentarios
			char *p = Line;
			while (*p == ' ' || *p == '\t')
			{
				p++;
			}
			unsigned n = 0;
			while (p[n] != '\0' && p[n] != '\r' && p[n] != '\n' && p[n] != ' '
			       && p[n] != '\t' && p[n] != '#')
			{
				n++;
			}
			if (n == 0 || n >= sizeof m_Servers[0])
			{
				continue;
			}
			memcpy (m_Servers[m_nServers], p, n);
			m_Servers[m_nServers][n] = '\0';
			m_nServers++;
		}
		f_close (&File);
	}

	if (m_nServers == 0)
	{
		for (unsigned i = 0; i < sizeof DefaultServers / sizeof DefaultServers[0]; i++)
		{
			strcpy (m_Servers[m_nServers++], DefaultServers[i]);
		}
	}
}

boolean CTNFSBrowser::IsAvailable (void)
{
	return CNetworkManager::IsNetRunning () && m_nServers > 0;
}

boolean CTNFSBrowser::Connect (const char *pHost)
{
	if (m_Client.IsMounted () && strcmp (m_Client.GetHost (), pHost) == 0)
	{
		return TRUE;
	}

	CString Text;
	Text.Format (T ("Conectando con %s..."), pHost);
	Busy (Text);

	if (!m_Client.Mount (pHost))
	{
		m_pError = m_Client.GetError ();
		return FALSE;
	}
	return TRUE;
}

boolean CTNFSBrowser::List (const char *pHost, const char *pPath, CTNFSListener *pListener)
{
	m_pError = "";
	if (!Connect (pHost))
	{
		return FALSE;
	}

	Busy (T ("Leyendo la lista..."));
	if (!m_Client.ListDir (pPath, pListener))
	{
		m_pError = m_Client.GetError ();
		// la sesión puede haber caducado: se reintenta una vez
		m_Client.Unmount ();
		if (!Connect (pHost) || !m_Client.ListDir (pPath, pListener))
		{
			m_pError = m_Client.GetError ();
			return FALSE;
		}
	}
	return TRUE;
}

unsigned CTNFSBrowser::Download (const char *pNetPath, u8 *pBuffer, unsigned nSize)
{
	// "NET:servidor/ruta/fichero"
	if (strncmp (pNetPath, "NET:", 4) != 0)
	{
		return 0;
	}
	const char *pHost = pNetPath + 4;
	const char *pSlash = strchr (pHost, '/');
	if (pSlash == 0)
	{
		return 0;
	}

	char Host[64];
	unsigned nHostLen = pSlash - pHost;
	if (nHostLen >= sizeof Host)
	{
		return 0;
	}
	memcpy (Host, pHost, nHostLen);
	Host[nHostLen] = '\0';

	if (!Connect (Host))
	{
		return 0;
	}

	Busy (T ("Descargando..."));
	m_Client.SetProgressHandler (DownloadProgress, this);
	unsigned nBytes = m_Client.ReadFile (pSlash, pBuffer, nSize);
	m_Client.SetProgressHandler (0, 0);
	if (nBytes == 0)
	{
		m_pError = m_Client.GetError ();
	}
	return nBytes;
}

// Mientras se descarga, la emulación está parada: se va diciendo cuánto lleva
void CTNFSBrowser::DownloadProgress (unsigned nBytes, void *pParam)
{
	CTNFSBrowser *pThis = static_cast<CTNFSBrowser *> (pParam);
	CString Text;
	Text.Format (T ("Descargando... %u KB"), nBytes / 1024);
	pThis->Busy (Text);
}
