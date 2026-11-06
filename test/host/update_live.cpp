// Prueba en vivo (necesita red, no va en la Action): el cliente HTTPS y el
// instalador contra GitHub de verdad. Consulta la última versión (API y plan
// B), descarga el zip de la SD siguiendo las redirecciones, lo comprueba y
// lo instala en una carpeta temporal.
//
//   ./update_live [certificados.pem]
#include "download/httpsclient.h"
#include "updater.h"
#include "zipfile.h"
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

// --- La SD: una carpeta ------------------------------------------------------

class CDirFS : public CUpdateFS
{
public:
	std::string Root;
	std::string P (const char *p) { return Root + "/" + p; }
	boolean GetSize (const char *p, unsigned *n) { struct stat st; if (stat (P (p).c_str (), &st) || !S_ISREG (st.st_mode)) return FALSE; *n = st.st_size; return TRUE; }
	boolean Read (const char *p, u8 *b, unsigned n) { FILE *f = fopen (P (p).c_str (), "rb"); if (!f) return FALSE; bool ok = fread (b, 1, n, f) == n; fclose (f); return ok; }
	boolean Write (const char *p, const u8 *d, unsigned n) { FILE *f = fopen (P (p).c_str (), "wb"); if (!f) return FALSE; bool ok = fwrite (d, 1, n, f) == n; fclose (f); return ok; }
	boolean Remove (const char *p) { return unlink (P (p).c_str ()) == 0; }
	boolean Rename (const char *a, const char *b) { return rename (P (a).c_str (), P (b).c_str ()) == 0; }
	boolean MakeDir (const char *p) { mkdir (P (p).c_str (), 0755); struct stat st; return stat (P (p).c_str (), &st) == 0; }
};

static void Progress (void *, size_t nReceived, size_t nTotal)
{
	static size_t nLast = 0;
	if (nReceived - nLast >= 2 * 1024 * 1024 || nReceived == nTotal)
	{
		printf ("      %zu de %zu KB\n", nReceived / 1024, nTotal / 1024);
		nLast = nReceived;
	}
}

int main (int argc, char **argv)
{
	// certificados raíz: los de la SD (certs/cacert.pem) o los indicados
	const char *pCerts = argc > 1 ? argv[1] : "../../boot/certs/cacert.pem";
	FILE *f = fopen (pCerts, "rb");
	if (!f) { printf ("no encuentro %s\n", pCerts); return 1; }
	static char PEM[512 * 1024];
	size_t nPEM = fread (PEM, 1, sizeof PEM - 1, f);
	fclose (f);
	PEM[nPEM] = '\0';
	HTTPSSetRootCerts (PEM, nPEM + 1);

	CSocket Sock;
	CHTTPSClient Client (&Sock);
	static unsigned char Small[1024 * 1024];
	size_t nLen = 0;
	char Version[16] = "", URL[512] = "";

	// 1. La API
	int nStatus = Client.Get ("https://api.github.com/repos/shaeon/ZXMetalCircle/releases/latest", Small, sizeof Small, &nLen);
	bool bAPI = nStatus == 200 && CUpdater::ParseRelease ((const char *) Small, nLen, Version, sizeof Version, URL, sizeof URL);
	printf ("API: código %d, %s\n", nStatus, bAPI ? Version : (nStatus < 0 ? Client.GetError () : "sin versión (¿límite de consultas?)"));

	// 2. La redirección de la web (lo que se usa primero), siempre
	char V2[16] = "", U2[512] = "";
	static char Loc[1024];
	time_t t0 = time (0);
	nStatus = Client.GetRedirect ("https://github.com/shaeon/ZXMetalCircle/releases/latest", Loc, sizeof Loc);
	bool bB = CUpdater::ParseTagURL (Loc, V2, sizeof V2, U2, sizeof U2);
	printf ("redirección: código %d en %ld s, apunta a %s -> %s\n", nStatus, (long) (time (0) - t0), Loc, bB ? V2 : "sin versión");
	t0 = time (0);
	Client.Get ("https://github.com/shaeon/ZXMetalCircle/releases/latest", Small, sizeof Small, &nLen);
	printf ("(antes: seguir la redirección y bajar la página: %zu bytes en %ld s)\n", nLen, (long) (time (0) - t0));
	if (!bAPI)
	{
		if (!bB) { printf ("FALLO: no hay forma de saber la versión\n"); return 1; }
		strcpy (Version, V2); strcpy (URL, U2);
	}
	else if (bB && strcmp (Version, V2) != 0)
	{
		printf ("FALLO: la API dice %s y el plan B %s\n", Version, V2);
		return 1;
	}

	// 3. La descarga del zip
	printf ("descargando %s\n", URL);
	size_t nBig = 32 * 1024 * 1024;
	unsigned char *pZip = new unsigned char[nBig];
	Client.SetProgress (Progress, 0);
	nStatus = Client.Get (URL, pZip, nBig, &nLen);
	Client.SetProgress (0, 0);
	if (nStatus != 200) { printf ("FALLO: código %d (%s)\n", nStatus, Client.GetError ()); return 1; }
	printf ("descargados %zu bytes\n", nLen);

	// 4. Instalarlo en una carpeta
	char Tmp[] = "/tmp/updliveXXXXXX";
	CDirFS fs; fs.Root = mkdtemp (Tmp);
	TUpdateResult R;
	CUpdater::Install (&fs, pZip, nLen, 0, 0, false, Version, &R);
	printf ("instalación: estado %d, %u copiados, %u sin tocar\n", R.Status, R.nChanged, R.nUnchanged);
	unsigned n;
	bool bKernels = fs.GetSize ("kernel.img", &n) && fs.GetSize ("kernel8-rpi4.img", &n) && fs.GetSize ("kernel_2712.img", &n);
	printf ("%s\n", R.Status == UpdateOK && R.nChanged > 100 && bKernels ? "todo OK" : "FALLO");
	char Cmd[96]; snprintf (Cmd, sizeof Cmd, "rm -rf %s", fs.Root.c_str ());
	if (system (Cmd) != 0) {}
	return R.Status == UpdateOK ? 0 : 1;
}
