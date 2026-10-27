//
// tnfs.h - Cliente TNFS (Trivial Network Filesystem, protocolo de Spectranet)
//
// Permite recorrer y descargar ficheros de los servidores TNFS de Spectrum
// (vexed4.alioth.net, tnfs.markround.com, tnfs.millhill.org...) por UDP,
// puerto 16384. Solo lectura: MOUNT, OPENDIR/READDIR/CLOSEDIR, STAT,
// OPEN/READ/CLOSE y UMOUNT. Especificación: tnfs-protocol.md de
// https://github.com/spectrumero/tnfsd
//
// El transporte (UDP) va aparte para poder probar el cliente en un PC.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_tnfs_h
#define _zx_tnfs_h

#include <circle/types.h>

#define TNFS_PORT	16384
#define TNFS_MAX_PATH	256

class CTNFSTransport
{
public:
	virtual ~CTNFSTransport (void) {}
	virtual boolean Open (const char *pHost, u16 nPort) = 0;	// resuelve y "conecta"
	virtual boolean Send (const u8 *pData, unsigned nLength) = 0;
	// devuelve los bytes recibidos o 0 si vence el tiempo
	virtual unsigned Receive (u8 *pBuffer, unsigned nSize, unsigned nTimeoutMs) = 0;
	virtual void Close (void) = 0;
};

class CTNFSListener
{
public:
	virtual ~CTNFSListener (void) {}
	virtual void OnEntry (const char *pName, boolean bDir) = 0;
};

class CTNFSClient
{
public:
	CTNFSClient (CTNFSTransport *pTransport);
	~CTNFSClient (void);

	boolean Mount (const char *pHost);
	void Unmount (void);
	boolean IsMounted (void) const		{ return m_bMounted; }
	const char *GetHost (void) const	{ return m_Host; }

	boolean OpenDir (const char *pPath);
	boolean ReadDir (char *pName, unsigned nSize);	// FALSE al terminar
	void CloseDir (void);

	// Lista una carpeta entera (sin "." ni ".."). Usa OPENDIRX/READDIRX, que
	// devuelven muchas entradas por datagrama y si son carpetas; si el servidor
	// no las conoce, OPENDIR/READDIR y STAT para lo que no parezca un fichero.
	boolean ListDir (const char *pPath, CTNFSListener *pListener, boolean bForceOld = FALSE);

	// Información de un fichero (TRUE si existe)
	boolean Stat (const char *pPath, boolean *pIsDir, u32 *pSize);

	// Descarga un fichero entero en pBuffer (devuelve los bytes o 0 si falla)
	unsigned ReadFile (const char *pPath, u8 *pBuffer, unsigned nSize);

	// Progreso de ReadFile: se llama cada 8 KB con los bytes leídos hasta ahora
	typedef void TProgressHandler (unsigned nBytes, void *pParam);
	void SetProgressHandler (TProgressHandler *pHandler, void *pParam)
		{ m_pProgress = pHandler; m_pProgressParam = pParam; }

	const char *GetError (void) const	{ return m_pError; }

private:
	// Envía una orden y espera la respuesta (con reintentos). Devuelve la
	// longitud de la respuesta en m_Reply o 0 si no hay respuesta.
	unsigned Request (u8 nCommand, const u8 *pData, unsigned nLength);

	CTNFSTransport *m_pTransport;
	boolean m_bMounted;
	char m_Host[128];
	u16 m_nSession;
	u8 m_nSeq;
	unsigned m_nRetryMs;
	int m_nDirHandle;
	TProgressHandler *m_pProgress;
	void *m_pProgressParam;
	const char *m_pError;

	u8 m_Request[600];
	u8 m_Reply[600];
};

#endif
