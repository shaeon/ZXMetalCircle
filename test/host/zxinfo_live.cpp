// Prueba en vivo (necesita red, no va en la Action): el cliente HTTPS y el
// instalador contra GitHub de verdad. Consulta la última versión (API y plan
// B), descarga el zip de la SD siguiendo las redirecciones, lo comprueba y
// lo instala en una carpeta temporal.
//
//   ./update_live [certificados.pem]
#include "download/httpsclient.h"
#include "download/zxinfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

// --- Lo que en la Pi pone Circle ---------------------------------------------

const char *T (const char *p) { return p; }
extern "C" {
int zxmc_snprintf (char *s, size_t n, const char *f, ...) { va_list a; va_start (a, f); int r = vsnprintf (s, n, f, a); va_end (a); return r; }
int zxmc_vsnprintf (char *s, size_t n, const char *f, va_list a) { return vsnprintf (s, n, f, a); }
int zxmc_printf (const char *, ...) { return 0; }
int zxmc_fprintf (FILE *, const char *, ...) { return 0; }
int mbedtls_hardware_poll (void *, unsigned char *pOut, size_t nLen, size_t *pnOut)
{
	FILE *f = fopen ("/dev/urandom", "rb");
	*pnOut = fread (pOut, 1, nLen, f);
	fclose (f);
	return 0;
}
}

class CSocket : public ITransport
{
	int m_fd = -1;
public:
	bool Connect (const char *pHost, unsigned nPort)
	{
		char Port[8]; snprintf (Port, sizeof Port, "%u", nPort);
		struct addrinfo Hints = {}, *pRes;
		Hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo (pHost, Port, &Hints, &pRes) != 0) return false;
		m_fd = socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);
		bool ok = m_fd >= 0 && connect (m_fd, pRes->ai_addr, pRes->ai_addrlen) == 0;
		freeaddrinfo (pRes);
		return ok;
	}
	int Send (const unsigned char *p, size_t n) { return send (m_fd, p, n, 0); }
	int Receive (unsigned char *p, size_t n) { return recv (m_fd, p, n, 0); }
	void Close (void) { if (m_fd >= 0) close (m_fd); m_fd = -1; }
};


// Prueba en vivo del buscador de ZXDB (la API de ZXInfo) y de la descarga,
// con el mismo código que el emulador. Necesita red: no va en la Action.
//   ./zxinfo_live [cacert.pem] [texto]
int main (int argc, char **argv)
{
	const char *pCerts = argc > 1 ? argv[1] : "../../boot/certs/cacert.pem";
	const char *pQuery = argc > 2 ? argv[2] : "manic miner";
	FILE *f = fopen (pCerts, "rb");
	if (!f) { printf ("no encuentro %s\n", pCerts); return 1; }
	static char PEM[512 * 1024];
	size_t nPEM = fread (PEM, 1, sizeof PEM - 1, f);
	fclose (f);
	PEM[nPEM] = '\0';
	HTTPSSetRootCerts (PEM, nPEM + 1);

	static unsigned char Work[16 * 1024 * 1024], File[16 * 1024 * 1024];
	TZXInfoResult Results[ZXINFO_MAX_RESULTS];
	const char *pError = "";
	int nFound;
	{
		CSocket Sock;
		CHTTPSClient Client (&Sock);
		nFound = ZXInfoSearch (&Client, pQuery, Results, 10, Work, sizeof Work, &pError);
	}
	printf ("buscar \"%s\": %d resultados %s\n", pQuery, nFound, pError);
	for (int i = 0; i < nFound; i++)
	{
		printf ("  %s  %-32s %s  %s  %s\n", Results[i].ID, Results[i].Title, Results[i].Machine,
			Results[i].Format, Results[i].Path);
	}
	if (nFound <= 0 || Results[0].Path[0] == '\0')
	{
		return 1;
	}
	char Name[64];
	size_t nSize;
	{
		CSocket Sock;
		CHTTPSClient Client (&Sock);
		nSize = ZXInfoDownload (&Client, Results[0].Path, File, sizeof File, Name, sizeof Name,
					Work, sizeof Work, &pError);
	}
	printf ("descargar %s: %s, %zu bytes %s\n", Results[0].Path, Name, nSize, pError);
	return nSize > 0 ? 0 : 1;
}
