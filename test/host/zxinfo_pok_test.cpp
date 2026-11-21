// Pokes de ZXDB (src/download/zxinfo.cpp): qué se pide a ZXInfo y cómo se lee
// la respuesta, con un cliente HTTPS de mentira que devuelve JSON preparado
// (la forma de /v5/filecheck y /v5/entries de zxinfo-api-v5)
//
//   g++ -O2 -std=c++17 -I. -I../../src -o zxinfo_pok_test zxinfo_pok_test.cpp
//       ../../src/download/zxinfo.cpp ../../src/inflate.cpp t_stub.cpp
#include "download/zxinfo.h"
#include <stdio.h>
#include <string.h>
#include <string>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

// El cliente de mentira: la última URL pedida y lo que se contesta
static std::string s_LastURL;
static int s_nStatus = 200;
static std::string s_Body;

CHTTPSClient::CHTTPSClient (ITransport *pTransport) : m_pTransport (pTransport), m_pProgress (0), m_pProgressParam (0)
{
	m_Error[0] = '\0';
}

int CHTTPSClient::Get (const char *pURL, unsigned char *pBuffer, size_t nBufferSize, size_t *pLength)
{
	s_LastURL = pURL;
	*pLength = 0;
	if (s_nStatus == 200)
	{
		size_t n = s_Body.size () < nBufferSize ? s_Body.size () : nBufferSize;
		memcpy (pBuffer, s_Body.data (), n);
		*pLength = n;
	}
	if (s_nStatus < 0)
	{
		strcpy (m_Error, "sin red");
	}
	return s_nStatus;
}

int main ()
{
	static unsigned char Work[256 * 1024];
	CHTTPSClient Client (0);
	const char *pError;
	char ID[12], Title[64], Path[192];

	// 1. filecheck: encontrado
	const char *pHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
			    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
	s_nStatus = 200;
	s_Body = "{\"entry_id\":\"0003534\",\"title\":\"Operation Wolf\",\"zxinfoVersion\":\"x\","
		 "\"publishers\":[{\"name\":\"Ocean Software Ltd\"}],"
		 "\"file\":[{\"filename\":\"OperationWolf.tzx\",\"md5\":\"abc\",\"sha512\":\"def\"}]}";
	int n = ZXInfoFileCheck (&Client, pHash, ID, sizeof ID, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("filecheck: la URL", s_LastURL == std::string ("https://api.zxinfo.dk/v5/filecheck/") + pHash);
	chk ("filecheck: encontrado", n == 1 && strcmp (ID, "0003534") == 0 && strcmp (Title, "Operation Wolf") == 0);

	// no lo conoce (404) y error del servidor
	s_nStatus = 404;
	n = ZXInfoFileCheck (&Client, pHash, ID, sizeof ID, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("filecheck: 404 es no encontrado", n == 0 && ID[0] == '\0');
	s_nStatus = 503;
	n = ZXInfoFileCheck (&Client, pHash, ID, sizeof ID, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("filecheck: 503 es error", n < 0 && pError[0] != '\0');
	s_nStatus = -1;
	n = ZXInfoFileCheck (&Client, pHash, ID, sizeof ID, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("filecheck: sin red es error", n < 0 && strcmp (pError, "sin red") == 0);

	// 2. la ficha con su .pok entre las descargas
	s_nStatus = 200;
	s_Body = "{\"_index\":\"zxinfo\",\"_id\":\"0003534\",\"found\":true,\"_source\":{"
		 "\"title\":\"Operation Wolf\",\"machineType\":\"ZX-Spectrum 48K\","
		 "\"publishers\":[{\"name\":\"Ocean\"}],"
		 "\"additionalDownloads\":["
		 "{\"path\":\"/pub/sinclair/games-inlays/o/OperationWolf.jpg\",\"type\":\"Cassette inlay\",\"format\":\"Picture (JPG)\"},"
		 "{\"path\":\"/zxdb/sinclair/pokes/o/Operation Wolf (1988)(Ocean Software).pok\",\"type\":\"POK pokes file\",\"format\":\"POK\"},"
		 "{\"path\":\"/pub/sinclair/games-info/o/OperationWolf.txt\",\"type\":\"Instructions\",\"format\":\"Text\"}"
		 "],\"screens\":[]}}";
	n = ZXInfoPokPath (&Client, "0003534", Path, sizeof Path, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("ficha: la URL", s_LastURL == "https://api.zxinfo.dk/v5/entries/0003534?mode=tiny");
	chk ("ficha: su .pok", n == 1 && strcmp (Path, "/zxdb/sinclair/pokes/o/Operation Wolf (1988)(Ocean Software).pok") == 0);
	chk ("ficha: el título", strcmp (Title, "Operation Wolf") == 0);

	// sin .pok
	s_Body = "{\"_id\":\"0000001\",\"_source\":{\"title\":\"Sin trucos\",\"additionalDownloads\":["
		 "{\"path\":\"/pub/x.jpg\",\"type\":\"Cassette inlay\"}]}}";
	n = ZXInfoPokPath (&Client, "0000001", Path, sizeof Path, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("ficha sin .pok", n == 0 && Path[0] == '\0' && strcmp (Title, "Sin trucos") == 0);
	s_Body = "{\"_id\":\"0000002\",\"_source\":{\"title\":\"Nada\"}}";
	n = ZXInfoPokPath (&Client, "0000002", Path, sizeof Path, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("ficha sin descargas", n == 0);

	// dos .pok: mejor el que no es una variante
	s_Body = "{\"_id\":\"0045787\",\"_source\":{\"title\":\"Shadows of Belial\",\"additionalDownloads\":["
		 "{\"path\":\"/zxdb/sinclair/entries/0045787/ShadowsOfBelial(ES).pok\",\"type\":\"POK pokes file\"},"
		 "{\"path\":\"/zxdb/sinclair/entries/0045787/ShadowsOfBelial.POK\",\"type\":\"POK pokes file\"}]}}";
	n = ZXInfoPokPath (&Client, "0045787", Path, sizeof Path, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("dos .pok: el que no es variante", n == 1 && strcmp (Path, "/zxdb/sinclair/entries/0045787/ShadowsOfBelial.POK") == 0);

	// un id que no es un número no se pide
	s_LastURL = "";
	n = ZXInfoPokPath (&Client, "12/../x", Path, sizeof Path, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("id raro: no se pide", n == 0 && s_LastURL == "");

	// 404 de la ficha
	s_nStatus = 404;
	n = ZXInfoPokPath (&Client, "9999999", Path, sizeof Path, Title, sizeof Title, Work, sizeof Work, &pError);
	chk ("ficha 404", n == 0);

	// 3. la descarga del .pok va sin comprimir y con la ruta codificada
	s_nStatus = 200;
	s_Body = "NInfinite Lives\r\nZ 8 40000 0 0\r\nY\r\n";
	static unsigned char Out[64 * 1024];
	char Name[128];
	size_t nSize = ZXInfoDownload (&Client, "/zxdb/sinclair/pokes/o/Operation Wolf (1988)(Ocean Software).pok",
				       Out, sizeof Out, Name, sizeof Name, Work, sizeof Work, &pError);
	chk ("descarga: la URL", s_LastURL == "https://spectrumcomputing.co.uk/zxdb/sinclair/pokes/o/"
					      "Operation%20Wolf%20(1988)(Ocean%20Software).pok");
	chk ("descarga: el fichero", nSize == s_Body.size () && memcmp (Out, s_Body.data (), nSize) == 0
				      && strcmp (Name, "Operation Wolf (1988)(Ocean Software).pok") == 0);

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails;
}
