//
// snetcircle.h - Red de la Pi (Circle) para los sockets de la Spectranet
//
// Cada socket del W5100 es un CSocket de Circle. Todo es sin bloquear salvo
// Connect (TCP), que detiene la emulación hasta que se conecta o falla. Las
// conexiones entrantes (Listen/Accept) se esperan en una tarea aparte, porque
// el Accept de Circle bloquea.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_snetcircle_h
#define _zx_snetcircle_h

#include "spectranet.h"
#include <circle/net/socket.h>

#define SNET_MAX_HANDLES	8

class CAcceptTask;
class CConnectTask;

class CSpectranetCircleNet : public CSpectranetNet
{
public:
	CSpectranetCircleNet (void);
	~CSpectranetCircleNet (void);

	boolean IsAvailable (void);
	int Open (boolean bTCP);
	boolean Bind (int h, u16 nPort);
	boolean ConnectStart (int h, const u8 *pIP, u16 nPort);
	int ConnectStatus (int h);
	boolean Listen (int h);
	int Accept (int h);
	int Send (int h, const u8 *pData, unsigned nLength);
	int SendTo (int h, const u8 *pData, unsigned nLength, const u8 *pIP, u16 nPort);
	int Recv (int h, u8 *pBuffer, unsigned nSize);
	int RecvFrom (int h, u8 *pBuffer, unsigned nSize, u8 *pIP, u16 *pPort);
	void Close (int h);
	boolean GetConfig (u8 *pIP, u8 *pMask, u8 *pGateway, u8 *pDNS);

private:
	CSocket *Get (int h)	{ return h >= 0 && h < SNET_MAX_HANDLES ? m_pSocket[h] : 0; }
	int NewHandle (CSocket *pSocket);

	CSocket *m_pSocket[SNET_MAX_HANDLES];
	CAcceptTask *m_pAccept[SNET_MAX_HANDLES];
	CConnectTask *m_pConnect[SNET_MAX_HANDLES];
	static boolean IsUnreachable (const u8 *pIP);
};

#endif
