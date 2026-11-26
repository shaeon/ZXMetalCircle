//
// circleglue.h - Transporte TCP de Circle para el cliente HTTPS
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_circleglue_h
#define _zx_circleglue_h

#include "httpsclient.h"
#include <circle/net/socket.h>
#include <circle/netdevice.h>	// FRAME_BUFFER_SIZE

class CCircleTransport : public ITransport
{
public:
	CCircleTransport (void);
	~CCircleTransport (void);

	bool Connect (const char *pHost, unsigned nPort);
	int Send (const unsigned char *pData, size_t nLength);
	int Receive (unsigned char *pBuffer, size_t nSize);
	void Close (void);
	void Pause (unsigned nMilliseconds);

private:
	CSocket *m_pSocket;

	// Lo recibido de un paquete que aún no se ha leído. Es de cada conexión:
	// antes era estático y solo se vaciaba si el socket nuevo tenía otra
	// dirección, pero el nuevo suele ocupar la misma memoria que el anterior,
	// así que lo que sobraba de la conexión anterior (el aviso de cierre de
	// TLS, o el cuerpo de una redirección) llegaba al principio de la
	// siguiente, su TLS fallaba y había que reintentar.
	u8 m_Frame[FRAME_BUFFER_SIZE];
	size_t m_nFrameLen;
	size_t m_nFramePos;
};

#endif
