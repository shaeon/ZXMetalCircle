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
// Los fallos pasajeros (no se puede conectar o se corta la conexión, o el
// servidor contesta 429 o 5xx, "ahora no") se reintentan solos hasta tres
// veces en total, esperando 1 y 2 segundos: con la WiFi o con servidores
// detrás de Cloudflare pasa de vez en cuando, y al volver a pedirlo funciona.
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
	virtual void Pause (unsigned nMilliseconds) { (void) nMilliseconds; }	// antes de reintentar
};

#define HTTP_ATTEMPTS	3		// intentos de cada petición (con fallos pasajeros)

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

	// Código HTTP de la última respuesta (0 si no llegó ninguna) e intentos
	// que hicieron falta: para el registro
	int GetLastStatus (void) const		{ return m_nLastStatus; }
	unsigned GetAttempts (void) const	{ return m_nAttempts; }

	// Aviso antes de cada reintento (nAttempt = 2, 3...), p. ej. para poner
	// "Reintentando..." en pantalla
	typedef void TRetry (void *pParam, unsigned nAttempt);
	void SetRetry (TRetry *pRetry, void *pParam)	{ m_pRetry = pRetry; m_pRetryParam = pParam; }

	// Tras Get: la dirección final, después de las redirecciones
	const char *GetFinalURL (void) const;

	// Una sola petición, sin seguir la redirección: devuelve el código (302...)
	// y en pLocation adónde apunta. Rápido: no descarga la página de destino.
	int GetRedirect (const char *pURL, char *pLocation, size_t nLocationSize);

	// Progreso de la descarga: bytes recibidos y total (0 si no se sabe)
	typedef void TProgress (void *pParam, size_t nReceived, size_t nTotal);
	void SetProgress (TProgress *pProgress, void *pParam)	{ m_pProgress = pProgress; m_pProgressParam = pParam; }

private:
	int GetFollow (const char *pURL, unsigned char *pBuffer, size_t nBufferSize, size_t *pLength);
	bool RetryAfter (int nStatus, unsigned nAttempt);	// TRUE: se vuelve a intentar
	int GetOnce (const char *pURL, unsigned char *pBuffer, size_t nBufferSize, size_t *pLength,
		     char *pLocation, size_t nLocationSize);

	ITransport *m_pTransport;
	char m_Error[96];
	bool m_bTransient;		// el último fallo (< 0) puede ser pasajero
	int m_nLastStatus;
	unsigned m_nAttempts;
	TProgress *m_pProgress;
	void *m_pProgressParam;
	TRetry *m_pRetry;
	void *m_pRetryParam;
};

#endif
