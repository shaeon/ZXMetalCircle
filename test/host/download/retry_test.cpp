//
// retry_test.cpp - Reintentos del cliente HTTPS (CHTTPSClient) con fallos pasajeros
//
// Un transporte de mentira contesta, intento a intento, lo que diga el guion:
// no conectar, un código HTTP (503, 429, 404...) o la respuesta buena. Se
// comprueba qué se reintenta, cuántas veces, cuánto se espera y el aviso.
//
// Se compila con Mbed TLS (como el emulador), aunque aquí solo se usa http://
// (ver .github/workflows/build.yml).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "download/httpsclient.h"
#include <stdio.h>
#include <string.h>

// Cada intento: 0 = no se puede conectar, -1 = se corta la conexión sin
// respuesta, o el código HTTP que se contesta (200 con el cuerpo "hola")
class CScriptTransport : public ITransport
{
public:
	CScriptTransport (const int *pScript, unsigned nCount)
	:	m_pScript (pScript), m_nCount (nCount), m_nAttempt (0), m_nPaused (0), m_nPauses (0) {}

	bool Connect (const char *, unsigned) override
	{
		m_nStep = m_nAttempt < m_nCount ? m_pScript[m_nAttempt] : 200;
		m_nAttempt++;
		m_nSent = 0;
		return m_nStep != 0;
	}
	int Send (const unsigned char *, size_t nLength) override	{ return (int) nLength; }
	int Receive (unsigned char *pBuffer, size_t nSize) override
	{
		if (m_nStep < 0 || m_nSent != 0)
		{
			return m_nStep < 0 ? -1 : 0;
		}
		char Response[128];
		const char *pBody = m_nStep == 200 ? "hola" : "error";
		snprintf (Response, sizeof Response, "HTTP/1.1 %d X\r\nContent-Length: %u\r\n\r\n%s",
			  m_nStep, (unsigned) strlen (pBody), pBody);
		size_t n = strlen (Response);
		if (n > nSize) n = nSize;
		memcpy (pBuffer, Response, n);
		m_nSent = n;
		return (int) n;
	}
	void Close (void) override {}
	void Pause (unsigned nMilliseconds) override	{ m_nPaused += nMilliseconds; m_nPauses++; }

	const int *m_pScript;
	unsigned m_nCount, m_nAttempt;
	int m_nStep;
	size_t m_nSent;
	unsigned m_nPaused, m_nPauses;
};

static unsigned s_nErrors = 0;
static unsigned s_nRetryCalls = 0;

static void OnRetry (void *, unsigned)	{ s_nRetryCalls++; }

static void Check (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) s_nErrors++;
}

// Devuelve el código de Get y comprueba intentos, esperas y avisos
static void Run (const char *pName, const int *pScript, unsigned nCount,
		 int nExpectedStatus, unsigned nExpectedAttempts)
{
	CScriptTransport Transport (pScript, nCount);
	CHTTPSClient Client (&Transport);
	s_nRetryCalls = 0;
	Client.SetRetry (OnRetry, 0);
	static unsigned char Buffer[1024];
	size_t nLength = 0;
	int nStatus = Client.Get ("http://ejemplo.test/fichero.zip", Buffer, sizeof Buffer, &nLength);

	unsigned nPausesExpected = nExpectedAttempts - 1;
	unsigned nPausedExpected = 0;
	for (unsigned i = 1; i < nExpectedAttempts; i++) nPausedExpected += 1000 * i;

	bool bOK =    nStatus == nExpectedStatus
		   && Transport.m_nAttempt == nExpectedAttempts
		   && Client.GetAttempts () == nExpectedAttempts
		   && Transport.m_nPauses == nPausesExpected
		   && Transport.m_nPaused == nPausedExpected
		   && s_nRetryCalls == nPausesExpected
		   && (nStatus != 200 || (nLength == 4 && memcmp (Buffer, "hola", 4) == 0));
	if (!bOK)
	{
		printf ("  -> código %d (esperado %d), intentos %u (esperados %u), esperas %u ms, avisos %u\n",
			nStatus, nExpectedStatus, Transport.m_nAttempt, nExpectedAttempts,
			Transport.m_nPaused, s_nRetryCalls);
	}
	Check (pName, bOK);
}

int main (void)
{
	HTTPSSetRootCerts (0, 0);

	static const int Good[] = {200};
	Run ("a la primera, sin reintentos", Good, 1, 200, 1);

	static const int ConnectThenOK[] = {0, 200};
	Run ("no conecta y luego sí (DNS o WiFi)", ConnectThenOK, 2, 200, 2);

	static const int BusyBusyOK[] = {503, 429, 200};
	Run ("503 y 429 (servidor ocupado) y luego bien", BusyBusyOK, 3, 200, 3);

	static const int CutThenOK[] = {-1, 200};
	Run ("se corta sin respuesta y luego bien", CutThenOK, 2, 200, 2);

	static const int AlwaysBusy[] = {503, 503, 503, 503};
	Run ("siempre 503: tres intentos y se rinde", AlwaysBusy, 4, 503, 3);

	static const int NotFound[] = {404, 200};
	Run ("404 no se reintenta", NotFound, 2, 404, 1);

	static const int Forbidden[] = {403, 200};
	Run ("403 no se reintenta", Forbidden, 2, 403, 1);

	if (s_nErrors == 0)
	{
		printf ("retry_test: todo OK\n");
	}
	return s_nErrors == 0 ? 0 : 1;
}
