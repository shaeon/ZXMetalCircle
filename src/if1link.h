//
// if1link.h - RS232 y red del Interface 1 hacia la red de la Pi
//
//   - RS232: servidor TCP en el puerto 2323 de la Pi (un cliente: telnet,
//     netcat, PuTTY en modo "raw"...). Lo que manda el Spectrum va al cliente
//     y, si no hay ninguno, se añade a SD:/saves/rs232.txt (para imprimir
//     listados con LPRINT, por ejemplo). Lo que escribe el cliente le llega
//     al Spectrum.
//   - ZX Net: paquetes UDP por difusión en el puerto 5050 de la red local
//     (el formato de SpectRusty), así que varias Pi con ZXMetalCircle hacen
//     de una red de Spectrum.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_if1link_h
#define _zx_if1link_h

#include "interface1.h"
#include "zxnet.h"
#include <circle/net/socket.h>
#include <circle/types.h>

#define RS232_TCP_PORT		2323
#define ZXNET_UDP_PORT		5050

class CRS232AcceptTask;

class CIF1Link : public CZXNetTransport
{
public:
	CIF1Link (void);
	~CIF1Link (void);

	void Setup (boolean bRS232, boolean bZXNet)	{ m_bRS232 = bRS232; m_bZXNet = bZXNet; }

	// Una vez por cuadro con el Interface 1 conectado
	void Poll (CInterface1 *pIF1);

	boolean IsRS232Connected (void) const		{ return m_pClient != 0; }
	void Flush (void)				{ FlushFile (); }

	// CZXNetTransport
	void Send (const u8 *pData, unsigned nLength);
	unsigned Receive (u8 *pBuffer, unsigned nSize, boolean bWait);

private:
	void StartNetwork (void);
	void FlushFile (void);

	boolean m_bRS232;
	boolean m_bZXNet;
	boolean m_bStarted;

	CSocket *m_pListen;
	CRS232AcceptTask *m_pAccept;
	CSocket *m_pClient;

	CSocket *m_pUDP;
	u8 m_OwnIP[4];

	u8 m_FileBuffer[1024];
	unsigned m_nFileBuffer;
	unsigned m_nFileIdle;
};

#endif
