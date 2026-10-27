//
// webserver.h - Servidor web de ZXMetalCircle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_webserver_h
#define _zx_webserver_h

#include <circle/net/httpdaemon.h>
#include <circle/string.h>
#include <circle/types.h>

// Enlace con el resto del emulador (los implementa network.cpp)
const char *WebGetStatus (void);		// estado del emulador para la página
void WebRequestLoad (const char *pPath);	// pide cargar un fichero en el Spectrum
boolean WebTLSVerify (void);			// comprobar certificados (tlsverify=1)
unsigned WebMode (void);			// 1 = completo, 2 = solo lectura
const char *WebPassword (void);		// "" = sin contraseña
void WebBusy (const char *pText);		// aviso en pantalla: red en uso (operación larga)

// Servidor web: navegador de la SD con subida, descarga, borrado y carga
class CZXWebServer : public CHTTPDaemon
{
public:
	CZXWebServer (CNetSubSystem *pNetSubSystem, CSocket *pSocket = 0);

	CHTTPDaemon *CreateWorker (CNetSubSystem *pNetSubSystem, CSocket *pSocket) override;

	THTTPStatus GetContent (const char *pPath, const char *pParams, const char *pFormData,
				u8 *pBuffer, unsigned *pLength, const char **ppContentType) override;

	void WriteAccessLog (const CIPAddress &rRemoteIP, THTTPRequestMethod RequestMethod,
			     const char *pRequestURI, THTTPStatus Status, unsigned nContentLength) override;

private:
	THTTPStatus SendFile (const char *pSDPath, u8 *pBuffer, unsigned *pLength,
			      const char **ppContentType);
	THTTPStatus SendDirectory (const char *pDir, const char *pMessage,
				   u8 *pBuffer, unsigned *pLength);
	CString Upload (const char *pDir);
	THTTPStatus SendSearch (const char *pQuery, u8 *pBuffer, unsigned *pLength);
	CString Download (const char *pPath);
	THTTPStatus SendLogin (const char *pMessage, u8 *pBuffer, unsigned *pLength);
	void AddToken (CString *pHtml);		// la clave de sesión en enlaces y formularios
	char m_Token[24];			// clave de esta petición ("" = sin contraseña)
};

#endif
