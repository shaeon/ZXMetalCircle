//
// httpsclient.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "httpsclient.h"
#include "../i18n.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/platform.h"

#ifdef __circle__
#include <circle/util.h>
#else
#include <string.h>
#endif

#define MAX_REDIRECTS	5
#define HEADER_MAX	4096

//------------------------------------------------------------------------------
// Utilidades de texto (sin depender de la libc)

static bool StartsWithNoCase (const char *p, const char *pPrefix)
{
	for (; *pPrefix != '\0'; p++, pPrefix++)
	{
		char a = *p, b = *pPrefix;
		if (a >= 'A' && a <= 'Z') a += 32;
		if (b >= 'A' && b <= 'Z') b += 32;
		if (a != b)
		{
			return false;
		}
	}
	return true;
}

static unsigned long ParseNumber (const char *p, unsigned nBase)
{
	unsigned long n = 0;
	for (;; p++)
	{
		int d;
		if (*p >= '0' && *p <= '9')				d = *p - '0';
		else if (nBase == 16 && *p >= 'a' && *p <= 'f')	d = *p - 'a' + 10;
		else if (nBase == 16 && *p >= 'A' && *p <= 'F')	d = *p - 'A' + 10;
		else break;
		n = n * nBase + d;
	}
	return n;
}

static void CopyString (char *pDest, const char *pSrc, size_t nSize, size_t nMax = (size_t) -1)
{
	size_t i = 0;
	while (i + 1 < nSize && i < nMax && pSrc[i] != '\0')
	{
		pDest[i] = pSrc[i];
		i++;
	}
	pDest[i] = '\0';
}

//------------------------------------------------------------------------------
// TLS (configuración compartida)

static bool s_bTLSReady = false;
static mbedtls_entropy_context s_Entropy;
static mbedtls_ctr_drbg_context s_DRBG;
static mbedtls_x509_crt s_CACerts;
static bool s_bVerify = false;
static mbedtls_ssl_config s_Config;

void HTTPSSetRootCerts (const char *pPEM, size_t nLength)
{
	if (pPEM == 0)
	{
		s_bVerify = false;
		return;
	}

	mbedtls_x509_crt_init (&s_CACerts);
	// nLength incluye el '\0' final, como pide Mbed TLS para PEM; los
	// certificados que no se entiendan se saltan (devuelve > 0)
	int nResult = mbedtls_x509_crt_parse (&s_CACerts, (const unsigned char *) pPEM, nLength);
	s_bVerify = nResult >= 0 && s_CACerts.version != 0;
}

static bool InitTLS (void)
{
	if (s_bTLSReady)
	{
		return true;
	}

	mbedtls_entropy_init (&s_Entropy);
	mbedtls_ctr_drbg_init (&s_DRBG);
	static const char Personal[] = "ZXMetalCircle";
	if (mbedtls_ctr_drbg_seed (&s_DRBG, mbedtls_entropy_func, &s_Entropy,
				   (const unsigned char *) Personal, sizeof Personal - 1) != 0)
	{
		return false;
	}

	mbedtls_ssl_config_init (&s_Config);
	if (mbedtls_ssl_config_defaults (&s_Config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
					 MBEDTLS_SSL_PRESET_DEFAULT) != 0)
	{
		return false;
	}
	mbedtls_ssl_conf_rng (&s_Config, mbedtls_ctr_drbg_random, &s_DRBG);
	if (s_bVerify)
	{
		mbedtls_ssl_conf_ca_chain (&s_Config, &s_CACerts, 0);
		mbedtls_ssl_conf_authmode (&s_Config, MBEDTLS_SSL_VERIFY_REQUIRED);
	}
	else
	{
		mbedtls_ssl_conf_authmode (&s_Config, MBEDTLS_SSL_VERIFY_NONE);
	}

	s_bTLSReady = true;
	return true;
}

static int TLSSend (void *pCtx, const unsigned char *pBuf, size_t nLen)
{
	int n = static_cast<ITransport *> (pCtx)->Send (pBuf, nLen);
	return n < 0 ? MBEDTLS_ERR_SSL_INTERNAL_ERROR : n;
}

static int TLSRecv (void *pCtx, unsigned char *pBuf, size_t nLen)
{
	int n = static_cast<ITransport *> (pCtx)->Receive (pBuf, nLen);
	return n < 0 ? MBEDTLS_ERR_SSL_INTERNAL_ERROR : n;
}

// Una conexión: TCP a secas o TCP + TLS
class CConnection
{
public:
	CConnection (ITransport *pTransport) : m_pTransport (pTransport), m_bTLS (false), m_bOpen (false) {}
	~CConnection (void) { Close (); }

	bool Open (const char *pHost, unsigned nPort, bool bTLS, char *pError, size_t nErrorSize)
	{
		if (!m_pTransport->Connect (pHost, nPort))
		{
			CopyString (pError, T ("No se pudo conectar con el servidor"), nErrorSize);
			return false;
		}
		m_bOpen = true;
		m_bTLS = bTLS;
		if (!bTLS)
		{
			return true;
		}

		if (!InitTLS ())
		{
			CopyString (pError, T ("Error al iniciar TLS"), nErrorSize);
			return false;
		}
		mbedtls_ssl_init (&m_SSL);
		if (   mbedtls_ssl_setup (&m_SSL, &s_Config) != 0
		    || mbedtls_ssl_set_hostname (&m_SSL, pHost) != 0)
		{
			CopyString (pError, T ("Error al preparar TLS"), nErrorSize);
			return false;
		}
		mbedtls_ssl_set_bio (&m_SSL, m_pTransport, TLSSend, TLSRecv, 0);

		int nResult;
		while ((nResult = mbedtls_ssl_handshake (&m_SSL)) != 0)
		{
			if (nResult != MBEDTLS_ERR_SSL_WANT_READ && nResult != MBEDTLS_ERR_SSL_WANT_WRITE)
			{
				CopyString (pError,
					    nResult == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED
					    ? "Certificado del servidor no valido" : "Fallo en la conexion segura",
					    nErrorSize);
				return false;
			}
		}
		return true;
	}

	int Send (const unsigned char *p, size_t n)
	{
		size_t nSent = 0;
		while (nSent < n)
		{
			int r = m_bTLS ? mbedtls_ssl_write (&m_SSL, p + nSent, n - nSent)
				       : m_pTransport->Send (p + nSent, n - nSent);
			if (r == MBEDTLS_ERR_SSL_WANT_WRITE || r == MBEDTLS_ERR_SSL_WANT_READ)
			{
				continue;
			}
			if (r <= 0)
			{
				return -1;
			}
			nSent += r;
		}
		return (int) nSent;
	}

	int Receive (unsigned char *p, size_t n)
	{
		if (!m_bTLS)
		{
			return m_pTransport->Receive (p, n);
		}
		for (;;)
		{
			int r = mbedtls_ssl_read (&m_SSL, p, n);
			if (r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE)
			{
				continue;
			}
			if (r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
			{
				return 0;
			}
			return r;
		}
	}

	void Close (void)
	{
		if (m_bTLS && m_bOpen)
		{
			mbedtls_ssl_close_notify (&m_SSL);
			mbedtls_ssl_free (&m_SSL);
		}
		if (m_bOpen)
		{
			m_pTransport->Close ();
		}
		m_bOpen = m_bTLS = false;
	}

private:
	ITransport *m_pTransport;
	bool m_bTLS;
	bool m_bOpen;
	mbedtls_ssl_context m_SSL;
};

//------------------------------------------------------------------------------
// HTTP

CHTTPSClient::CHTTPSClient (ITransport *pTransport)
:	m_pTransport (pTransport)
{
	m_Error[0] = '\0';
}

int CHTTPSClient::Get (const char *pURL, unsigned char *pBuffer, size_t nBufferSize, size_t *pLength)
{
	static char URL[1024];
	static char Location[1024];
	CopyString (URL, pURL, sizeof URL);

	for (unsigned i = 0; i <= MAX_REDIRECTS; i++)
	{
		Location[0] = '\0';
		int nStatus = GetOnce (URL, pBuffer, nBufferSize, pLength, Location, sizeof Location);
		if (nStatus < 300 || nStatus >= 400 || Location[0] == '\0')
		{
			return nStatus;
		}

		// redirección: absoluta o relativa al mismo servidor
		if (StartsWithNoCase (Location, "http://") || StartsWithNoCase (Location, "https://"))
		{
			CopyString (URL, Location, sizeof URL);
		}
		else
		{
			const char *pHostEnd = URL;
			pHostEnd += StartsWithNoCase (URL, "https://") ? 8 : 7;
			while (*pHostEnd != '\0' && *pHostEnd != '/')
			{
				pHostEnd++;
			}
			size_t nPrefix = pHostEnd - URL;
			char Tmp[1024];
			CopyString (Tmp, URL, sizeof Tmp, nPrefix);
			size_t nLen = strlen (Tmp);
			CopyString (Tmp + nLen, Location, sizeof Tmp - nLen);
			CopyString (URL, Tmp, sizeof URL);
		}
	}

	CopyString (m_Error, T ("Demasiadas redirecciones"), sizeof m_Error);
	return -1;
}

int CHTTPSClient::GetOnce (const char *pURL, unsigned char *pBuffer, size_t nBufferSize, size_t *pLength,
			   char *pLocation, size_t nLocationSize)
{
	*pLength = 0;
	m_Error[0] = '\0';

	// http[s]://servidor[:puerto]/ruta
	bool bTLS;
	const char *p = pURL;
	if (StartsWithNoCase (p, "https://"))	{ bTLS = true; p += 8; }
	else if (StartsWithNoCase (p, "http://"))	{ bTLS = false; p += 7; }
	else
	{
		CopyString (m_Error, T ("URL no válida"), sizeof m_Error);
		return -1;
	}

	char Host[128];
	size_t nHost = 0;
	while (*p != '\0' && *p != '/' && *p != ':' && nHost < sizeof Host - 1)
	{
		Host[nHost++] = *p++;
	}
	Host[nHost] = '\0';
	unsigned nPort = bTLS ? 443 : 80;
	if (*p == ':')
	{
		nPort = (unsigned) ParseNumber (++p, 10);
		while (*p >= '0' && *p <= '9') p++;
	}
	const char *pPath = *p != '\0' ? p : "/";

	CConnection Conn (m_pTransport);
	if (!Conn.Open (Host, nPort, bTLS, m_Error, sizeof m_Error))
	{
		return -1;
	}

	static char Request[1400];
	size_t n = 0;
	const char *Parts[] = {"GET ", pPath, " HTTP/1.1\r\nHost: ", Host,
			       "\r\nUser-Agent: ZXMetalCircle\r\nAccept: */*\r\nConnection: close\r\n\r\n"};
	for (unsigned i = 0; i < sizeof Parts / sizeof Parts[0]; i++)
	{
		size_t l = strlen (Parts[i]);
		if (n + l >= sizeof Request)
		{
			CopyString (m_Error, T ("URL demasiado larga"), sizeof m_Error);
			return -1;
		}
		memcpy (Request + n, Parts[i], l);
		n += l;
	}
	if (Conn.Send ((const unsigned char *) Request, n) < 0)
	{
		CopyString (m_Error, T ("Error al enviar la petición"), sizeof m_Error);
		return -1;
	}

	// Cabeceras
	static char Header[HEADER_MAX + 1];
	size_t nHeader = 0;
	size_t nBodyStart = 0;
	for (;;)
	{
		int r = Conn.Receive ((unsigned char *) Header + nHeader, HEADER_MAX - nHeader);
		if (r <= 0)
		{
			CopyString (m_Error, T ("Respuesta incompleta"), sizeof m_Error);
			return -1;
		}
		nHeader += r;
		Header[nHeader] = '\0';

		for (size_t i = 3; i < nHeader; i++)
		{
			if (Header[i - 3] == '\r' && Header[i - 2] == '\n' && Header[i - 1] == '\r' && Header[i] == '\n')
			{
				nBodyStart = i + 1;
				break;
			}
		}
		if (nBodyStart != 0)
		{
			break;
		}
		if (nHeader == HEADER_MAX)
		{
			CopyString (m_Error, T ("Cabeceras demasiado largas"), sizeof m_Error);
			return -1;
		}
	}

	// HTTP/1.x NNN ...
	const char *pStatus = Header;
	while (*pStatus != ' ' && *pStatus != '\0') pStatus++;
	int nStatus = (int) ParseNumber (pStatus + 1, 10);

	long nContentLength = -1;
	bool bChunked = false;
	for (const char *pLine = Header; pLine < Header + nBodyStart; )
	{
		if (StartsWithNoCase (pLine, "Content-Length:"))
		{
			const char *v = pLine + 15;
			while (*v == ' ') v++;
			nContentLength = (long) ParseNumber (v, 10);
		}
		else if (StartsWithNoCase (pLine, "Transfer-Encoding:") && strstr (pLine, "chunked") != 0)
		{
			bChunked = true;
		}
		else if (StartsWithNoCase (pLine, "Location:"))
		{
			const char *v = pLine + 9;
			while (*v == ' ') v++;
			size_t l = 0;
			while (v[l] != '\r' && v[l] != '\n' && v[l] != '\0') l++;
			CopyString (pLocation, v, nLocationSize, l);
		}
		while (pLine < Header + nBodyStart && *pLine != '\n') pLine++;
		pLine++;
	}

	if (nStatus >= 300 && nStatus < 400)
	{
		return nStatus;		// se sigue la redirección sin leer el cuerpo
	}

	// Cuerpo
	size_t nBody = nHeader - nBodyStart;
	if (nBody > nBufferSize)
	{
		CopyString (m_Error, T ("Fichero demasiado grande"), sizeof m_Error);
		return -1;
	}
	memcpy (pBuffer, Header + nBodyStart, nBody);

	while (nContentLength < 0 || (long) nBody < nContentLength)
	{
		if (nBody == nBufferSize)
		{
			CopyString (m_Error, T ("Fichero demasiado grande"), sizeof m_Error);
			return -1;
		}
		int r = Conn.Receive (pBuffer + nBody, nBufferSize - nBody);
		if (r == 0)
		{
			break;			// fin de la conexión
		}
		if (r < 0)
		{
			if (nContentLength < 0)
			{
				break;		// algunos servidores cortan sin aviso
			}
			CopyString (m_Error, T ("Conexión interrumpida"), sizeof m_Error);
			return -1;
		}
		nBody += r;
	}

	// "chunked": se quitan las cabeceras de cada trozo
	if (bChunked)
	{
		size_t nIn = 0, nOut = 0;
		while (nIn < nBody)
		{
			unsigned long nChunk = ParseNumber ((const char *) pBuffer + nIn, 16);
			while (nIn < nBody && pBuffer[nIn] != '\n') nIn++;
			nIn++;
			if (nChunk == 0 || nIn + nChunk > nBody)
			{
				break;
			}
			memmove (pBuffer + nOut, pBuffer + nIn, nChunk);
			nOut += nChunk;
			nIn += nChunk + 2;	// datos + CRLF
		}
		nBody = nOut;
	}

	*pLength = nBody;
	return nStatus;
}
