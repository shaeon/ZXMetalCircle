//
// tnfsbrowser.h - Servidores TNFS en el selector de ficheros (F1)
//
// El selector muestra en la raíz de la SD una entrada "Internet (TNFS)" con
// los servidores de SD:/tnfs.txt (o unos por defecto). Se recorren como si
// fueran carpetas y, al elegir un fichero, se descarga a SD:/descargas/<servidor>
// y se carga como cualquier otro.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_tnfsbrowser_h
#define _zx_tnfsbrowser_h

#include "tnfs.h"
#include <circle/types.h>

class CNetSubSystem;
class CSocket;

// Interfaz que usa el menú (no depende de la red de Circle)
class CRemoteSource
{
public:
	virtual ~CRemoteSource (void) {}
	virtual boolean IsAvailable (void) = 0;
	virtual unsigned GetServerCount (void) = 0;
	virtual const char *GetServer (unsigned nIndex) = 0;
	virtual boolean List (const char *pHost, const char *pPath, CTNFSListener *pListener) = 0;
	virtual const char *GetError (void) = 0;
};

// Transporte UDP de Circle
class CCircleUDPTransport : public CTNFSTransport
{
public:
	CCircleUDPTransport (void);
	~CCircleUDPTransport (void);

	boolean Open (const char *pHost, u16 nPort);
	boolean Send (const u8 *pData, unsigned nLength);
	unsigned Receive (u8 *pBuffer, unsigned nSize, unsigned nTimeoutMs);
	void Close (void);

private:
	CSocket *m_pSocket;
};

class CTNFSBrowser : public CRemoteSource
{
public:
	typedef void TBusyHandler (const char *pText, void *pParam);

	CTNFSBrowser (void);

	void LoadServers (void);		// SD:/tnfs.txt o la lista por defecto
	void SetBusyHandler (TBusyHandler *pHandler, void *pParam)
		{ m_pBusy = pHandler; m_pBusyParam = pParam; }

	boolean IsAvailable (void);
	unsigned GetServerCount (void)		{ return m_nServers; }
	const char *GetServer (unsigned nIndex)	{ return m_Servers[nIndex]; }
	boolean List (const char *pHost, const char *pPath, CTNFSListener *pListener);
	const char *GetError (void)		{ return m_pError; }

	// Descarga "NET:servidor/ruta" en pBuffer; devuelve los bytes (0 si falla)
	unsigned Download (const char *pNetPath, u8 *pBuffer, unsigned nSize);

private:
	boolean Connect (const char *pHost);
	void Busy (const char *pText)		{ if (m_pBusy) m_pBusy (pText, m_pBusyParam); }
	static void DownloadProgress (unsigned nBytes, void *pParam);

	CCircleUDPTransport m_Transport;
	CTNFSClient m_Client;

	enum { MaxServers = 16 };
	char m_Servers[MaxServers][64];
	unsigned m_nServers;
	const char *m_pError;

	TBusyHandler *m_pBusy;
	void *m_pBusyParam;
};

#endif
