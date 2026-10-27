//
// httpsclient.h - Cliente HTTP/HTTPS sencillo para ZXMetalCircle
//
// GET con redirecciones (301/302/303/307/308), "Content-Length" o
// "Transfer-Encoding: chunked", a un búfer en memoria. HTTPS con Mbed TLS
// (TLS 1.2, ver third_party/mbedtls/zxmc_config.h).
//
// El transporte TCP es una interfaz (ITransport) para poder usar el mismo
// código en la Raspberry (sockets de Circle) y en las pruebas en el PC.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_httpsclient_h
#define _zx_httpsclient_h

#include <stddef.h>

class ITransport
{
public:
	virtual ~ITransport (void) {}
	virtual bool Connect (const char *pHost, unsigned nPort) = 0;
	virtual int Send (const unsigned char *pData, size_t nLength) = 0;	// < 0: error
	virtual int Receive (unsigned char *pBuffer, size_t nSize) = 0;	// 0: fin, < 0: error
	virtual void Close (void) = 0;
};

// Certificados raíz (PEM, terminado en '\0'); 0 = no se comprueban
void HTTPSSetRootCerts (const char *pPEM, size_t nLength);

class CHTTPSClient
{
public:
	CHTTPSClient (ITransport *pTransport);

	// Descarga la URL en pBuffer. Devuelve el código HTTP (200...) o un número
	// negativo si falla la conexión; *pLength = bytes recibidos.
	int Get (const char *pURL, unsigned char *pBuffer, size_t nBufferSize, size_t *pLength);

	const char *GetError (void) const	{ return m_Error; }

private:
	int GetOnce (const char *pURL, unsigned char *pBuffer, size_t nBufferSize, size_t *pLength,
		     char *pLocation, size_t nLocationSize);

	ITransport *m_pTransport;
	char m_Error[96];
};

#endif
