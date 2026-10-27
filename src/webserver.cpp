//
// webserver.cpp - Servidor web de ZXMetalCircle: navegador de la SD con subida,
//                 descarga, borrado, creación de carpetas y carga en el Spectrum
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "webserver.h"
#include "i18n.h"
#include "exclusions.h"
#include "version.h"
#include "download/zxinfo.h"
#include "download/circleglue.h"
#include <circle/sched/scheduler.h>
#include <circle/sched/task.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/timer.h>
#include <fatfs/ff.h>

#define MAX_CONTENT_SIZE	(4 * 1024 * 1024)	// descargas de hasta 4 MB
#define MAX_UPLOAD_SIZE		(8 * 1024 * 1024)	// subidas de hasta 8 MB
#define REQUEST_TIMEOUT		60			// segundos

////////////////////////////////////////////////////////////////////////////////
// Utilidades para la web

static const char *FindLast (const char *pString, char ch)
{
	const char *pLast = 0;
	for (; *pString != '\0'; pString++)
	{
		if (*pString == ch)
		{
			pLast = pString;
		}
	}
	return pLast;
}

static int HexValue (char ch)
{
	if (ch >= '0' && ch <= '9') return ch - '0';
	if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
	return -1;
}

static void URLDecode (const char *pIn, unsigned nInLen, char *pOut, unsigned nOutSize)
{
	unsigned o = 0;
	for (unsigned i = 0; i < nInLen && pIn[i] != '\0' && o < nOutSize - 1; i++)
	{
		if (pIn[i] == '+')
		{
			pOut[o++] = ' ';
		}
		else if (   pIn[i] == '%' && i + 2 < nInLen
			 && HexValue (pIn[i + 1]) >= 0 && HexValue (pIn[i + 2]) >= 0)
		{
			pOut[o++] = (char) (HexValue (pIn[i + 1]) * 16 + HexValue (pIn[i + 2]));
			i += 2;
		}
		else
		{
			pOut[o++] = pIn[i];
		}
	}
	pOut[o] = '\0';
}

// Busca "nombre=valor" en los parámetros de la URL y lo decodifica
static boolean GetParam (const char *pParams, const char *pName, char *pOut, unsigned nOutSize)
{
	size_t nNameLen = strlen (pName);
	const char *p = pParams;
	while (p != 0 && *p != '\0')
	{
		const char *pEnd = strchr (p, '&');
		unsigned nLen = pEnd != 0 ? pEnd - p : strlen (p);
		if (nLen > nNameLen && strncmp (p, pName, nNameLen) == 0 && p[nNameLen] == '=')
		{
			URLDecode (p + nNameLen + 1, nLen - nNameLen - 1, pOut, nOutSize);
			return TRUE;
		}
		p = pEnd != 0 ? pEnd + 1 : 0;
	}

	pOut[0] = '\0';
	return FALSE;
}

static void URLEncode (const char *pIn, CString *pOut)
{
	static const char Hex[] = "0123456789ABCDEF";
	*pOut = "";
	for (const unsigned char *p = (const unsigned char *) pIn; *p != '\0'; p++)
	{
		char Buf[4];
		if (   (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9')
		    || *p == '-' || *p == '_' || *p == '.' || *p == '/')
		{
			Buf[0] = *p;
			Buf[1] = '\0';
		}
		else
		{
			Buf[0] = '%';
			Buf[1] = Hex[*p >> 4];
			Buf[2] = Hex[*p & 15];
			Buf[3] = '\0';
		}
		pOut->Append (Buf);
	}
}

static void HTMLEscape (const char *pIn, CString *pOut)
{
	*pOut = "";
	for (const char *p = pIn; *p != '\0'; p++)
	{
		switch (*p)
		{
		case '<':  pOut->Append ("&lt;");	break;
		case '>':  pOut->Append ("&gt;");	break;
		case '&':  pOut->Append ("&amp;");	break;
		case '"':  pOut->Append ("&quot;");	break;
		default:
			{
				char Buf[2] = {*p, '\0'};
				pOut->Append (Buf);
			}
			break;
		}
	}
}

// Ruta relativa a la SD, sin ".." ni barras invertidas; devuelve "" si no vale
static boolean CleanPath (const char *pIn, char *pOut, unsigned nOutSize)
{
	if (strstr (pIn, "..") != 0 || strchr (pIn, '\\') != 0 || strchr (pIn, ':') != 0)
	{
		return FALSE;
	}

	while (*pIn == '/')
	{
		pIn++;
	}

	strncpy (pOut, pIn, nOutSize - 1);
	pOut[nOutSize - 1] = '\0';

	size_t nLen = strlen (pOut);
	while (nLen > 0 && pOut[nLen - 1] == '/')
	{
		pOut[--nLen] = '\0';
	}

	return TRUE;
}

static boolean HasExtension (const char *pName, const char *pExt)
{
	size_t nLen = strlen (pName);
	size_t nExtLen = strlen (pExt);
	return nLen > nExtLen && strcasecmp (pName + nLen - nExtLen, pExt) == 0;
}

static boolean IsLoadable (const char *pName)
{
	static const char *Ext[] = {".tap", ".tzx", ".pzx", ".csw", ".z80", ".sna", ".szx",
				    ".rom", ".mdr", ".mdv", ".dsk",
				    ".vhd", ".img", ".raw", ".hdf"};
	for (unsigned i = 0; i < sizeof Ext / sizeof Ext[0]; i++)
	{
		if (HasExtension (pName, Ext[i]))
		{
			return TRUE;
		}
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Servidor web

CZXWebServer::CZXWebServer (CNetSubSystem *pNetSubSystem, CSocket *pSocket)
:	CHTTPDaemon (pNetSubSystem, pSocket, MAX_CONTENT_SIZE, HTTP_PORT, MAX_UPLOAD_SIZE,
		     REQUEST_TIMEOUT)
{
	m_Token[0] = '\0';
}

CHTTPDaemon *CZXWebServer::CreateWorker (CNetSubSystem *pNetSubSystem, CSocket *pSocket)
{
	return new CZXWebServer (pNetSubSystem, pSocket);
}

void CZXWebServer::WriteAccessLog (const CIPAddress &rRemoteIP, THTTPRequestMethod RequestMethod,
				   const char *pRequestURI, THTTPStatus Status, unsigned nContentLength)
{
	// sin registro: el puerto serie va lento y no aporta nada al usuario
}

////////////////////////////////////////////////////////////////////////////////
// Contraseña (webpass=): el servidor web de Circle no da las cabeceras de la
// petición (ni cookies ni Authorization) ni deja poner cabeceras en la
// respuesta, así que no hay contraseña "del navegador". Tras entrar con la
// contraseña se crea una clave de sesión al azar, que va en cada enlace y
// formulario de las páginas (parámetro k). Las claves se pierden al apagar.
// Va sin cifrar (HTTP): protege de curiosos de la red local, no de espías.

#define MAX_SESSIONS		6
#define LOGIN_MAX_FAILS		5
#define LOGIN_LOCK_SECONDS	30

static char s_Sessions[MAX_SESSIONS][17];
static unsigned s_nNextSession = 0;
static unsigned s_nLoginFails = 0;
static unsigned s_nLoginLockUntil = 0;		// segundos (CTimer::GetUptime)
static u64 s_nRandom = 0;

static u64 Random64 (void)
{
	if (s_nRandom == 0)
	{
		s_nRandom = CTimer::GetClockTicks64 () * 0x9E3779B97F4A7C15ULL | 1;
	}
	s_nRandom ^= CTimer::GetClockTicks64 ();	// mezcla el momento de cada petición
	s_nRandom ^= s_nRandom << 13;
	s_nRandom ^= s_nRandom >> 7;
	s_nRandom ^= s_nRandom << 17;
	return s_nRandom;
}

static boolean ValidSession (const char *pToken)
{
	if (strlen (pToken) != 16)
	{
		return FALSE;
	}
	for (unsigned i = 0; i < MAX_SESSIONS; i++)
	{
		if (s_Sessions[i][0] != '\0' && strcmp (s_Sessions[i], pToken) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

static const char *NewSession (void)
{
	char *pSlot = s_Sessions[s_nNextSession];
	s_nNextSession = (s_nNextSession + 1) % MAX_SESSIONS;
	u64 nValue = Random64 ();
	static const char Hex[] = "0123456789abcdef";
	for (unsigned i = 0; i < 16; i++)
	{
		pSlot[i] = Hex[(nValue >> (4 * i)) & 15];
	}
	pSlot[16] = '\0';
	return pSlot;
}

// Añade la clave de sesión a la página: a los enlaces internos (href="/...")
// y a la acción de los formularios POST como ?k=, y a todos los formularios
// como campo oculto (el navegador quita la parte ?... de la acción de un GET).
// Se escribe en un búfer reservado de una vez: añadir carácter a carácter a
// un CString lo copia entero cada vez y una carpeta grande tardaría mucho.
void CZXWebServer::AddToken (CString *pHtml)
{
	if (m_Token[0] == '\0')
	{
		return;
	}

	const char *pIn = *pHtml;
	unsigned nInLen = strlen (pIn);

	CString HiddenText;
	HiddenText.Format ("<input type=\"hidden\" name=\"k\" value=\"%s\">", m_Token);
	const char *Hidden = HiddenText;
	unsigned nHidden = strlen (Hidden);
	unsigned nToken = strlen (m_Token);

	// cuántos sitios: cada uno añade como mucho la clave o el campo oculto
	unsigned nPlaces = 0;
	for (const char *q = pIn; (q = strchr (q, '<')) != 0; q++)
	{
		nPlaces++;
	}
	unsigned nMax = nInLen + nPlaces * (nHidden + nToken + 4) + 1;
	char *pOut = new char[nMax];
	if (pOut == 0)
	{
		return;
	}
	unsigned nOut = 0;
	auto Put = [&] (const char *pText, unsigned nLen)
	{
		if (nOut + nLen < nMax)
		{
			memcpy (pOut + nOut, pText, nLen);
			nOut += nLen;
		}
	};

	const char *p = pIn;
	boolean bInForm = FALSE;		// dentro de una etiqueta <form ...>
	boolean bPostForm = FALSE;
	while (*p != '\0')
	{
		if (strncmp (p, "<form", 5) == 0)
		{
			const char *pClose = strchr (p, '>');
			bInForm = pClose != 0;
			bPostForm = FALSE;
			for (const char *q = p; pClose != 0 && q + 13 <= pClose; q++)
			{
				if (strncmp (q, "method=\"post\"", 13) == 0)
				{
					bPostForm = TRUE;
				}
			}
		}

		boolean bHref = strncmp (p, "href=\"/", 7) == 0;
		boolean bAction = bInForm && bPostForm && strncmp (p, "action=\"/", 9) == 0;
		if (bHref || bAction)
		{
			const char *pOpen = strchr (p, '"');
			const char *pEnd = pOpen != 0 ? strchr (pOpen + 1, '"') : 0;
			if (pEnd != 0)
			{
				boolean bQuery = FALSE;
				for (const char *q = p; q < pEnd; q++)
				{
					bQuery |= *q == '?';
				}
				Put (p, pEnd - p);
				Put (bQuery ? "&k=" : "?k=", 3);
				Put (m_Token, nToken);
				p = pEnd;
				continue;
			}
		}

		if (*p == '>' && bInForm)
		{
			Put (">", 1);
			Put (Hidden, nHidden);	// detrás de la etiqueta <form ...>
			bInForm = FALSE;
			p++;
			continue;
		}

		Put (p, 1);
		p++;
	}
	pOut[nOut] = '\0';
	*pHtml = pOut;
	delete [] pOut;
}

THTTPStatus CZXWebServer::SendLogin (const char *pMessage, u8 *pBuffer, unsigned *pLength)
{
	CString Html, Line;
	Html.Format ("<!DOCTYPE html><html lang=\"%s\"><head><meta charset=\"utf-8\">"
		     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
		     "<title>ZXMetalCircle</title><style>"
		     "body{font-family:sans-serif;background:#10123a;color:#e8e8e8;margin:0;padding:12px}"
		     "h1{color:#eeeb46;font-size:1.3em}.msg{background:#36efde;color:#000;padding:6px;margin:8px 0}"
		     "form{margin:10px 0;padding:8px;background:#1c1f55}"
		     "</style></head><body>", CI18n::Get ()->GetLanguage ());
	Line.Format ("<h1>ZXMetalCircle %s</h1>", ZXCIRCLE_VERSION);
	Html.Append (Line);
	if (pMessage != 0 && *pMessage != '\0')
	{
		Line.Format ("<div class=\"msg\">%s</div>", pMessage);
		Html.Append (Line);
	}
	Line.Format ("<form method=\"post\" action=\"/login\">%s <input type=\"password\" name=\"p\" autofocus> "
		     "<input type=\"submit\" value=\"%s\"></form></body></html>",
		     T ("Contraseña:"), T ("Entrar"));
	Html.Append (Line);

	unsigned nLen = Html.GetLength ();
	if (nLen > *pLength)
	{
		return HTTPContentBufferTooSmall;
	}
	memcpy (pBuffer, (const char *) Html, nLen);
	*pLength = nLen;
	return HTTPOK;
}

THTTPStatus CZXWebServer::GetContent (const char *pPath, const char *pParams, const char *pFormData,
				      u8 *pBuffer, unsigned *pLength, const char **ppContentType)
{
	char Arg[HTTP_MAX_PATH + 1];
	char Clean[HTTP_MAX_PATH + 1];

	// contraseña: sin una clave de sesión válida, solo la página de entrada
	m_Token[0] = '\0';
	const char *pPassword = WebPassword ();
	if (*pPassword != '\0')
	{
		if (strcmp (pPath, "/login") == 0)
		{
			char Try[40];
			GetParam (pFormData, "p", Try, sizeof Try);
			unsigned nNow = CTimer::Get ()->GetUptime ();
			if (nNow < s_nLoginLockUntil)
			{
				return SendLogin (T ("Demasiados intentos: espera unos segundos"), pBuffer, pLength);
			}
			if (strcmp (Try, pPassword) != 0)
			{
				if (++s_nLoginFails >= LOGIN_MAX_FAILS)
				{
					s_nLoginFails = 0;
					s_nLoginLockUntil = nNow + LOGIN_LOCK_SECONDS;
				}
				return SendLogin (T ("Contraseña incorrecta"), pBuffer, pLength);
			}
			s_nLoginFails = 0;
			strcpy (m_Token, NewSession ());
			return SendDirectory ("", "", pBuffer, pLength);
		}
		char Token[24];
		GetParam (pParams, "k", Token, sizeof Token);
		if (!ValidSession (Token))
		{
			if (strncmp (pPath, "/sd/", 4) == 0)
			{
				return HTTPNotFound;		// (Circle no tiene 403)
			}
			return SendLogin ("", pBuffer, pLength);
		}
		strcpy (m_Token, Token);
	}

	// solo lectura: nada de subir, borrar, crear carpetas ni descargar a la SD
	boolean bReadOnly = WebMode () == 2;

	// /sd/<ruta>: descarga de un fichero
	if (strncmp (pPath, "/sd/", 4) == 0)
	{
		URLDecode (pPath + 4, strlen (pPath + 4), Arg, sizeof Arg);
		if (!CleanPath (Arg, Clean, sizeof Clean))
		{
			return HTTPBadRequest;
		}
		return SendFile (Clean, pBuffer, pLength, ppContentType);
	}

	GetParam (pParams, "d", Arg, sizeof Arg);
	if (!CleanPath (Arg, Clean, sizeof Clean))
	{
		Clean[0] = '\0';
	}

	CString Message;

	if (   bReadOnly
	    && (   strcmp (pPath, "/up") == 0 || strcmp (pPath, "/del") == 0
		|| strcmp (pPath, "/mkdir") == 0 || strcmp (pPath, "/bajar") == 0))
	{
		Message = T ("La web está en modo solo lectura");
	}
	else if (strcmp (pPath, "/up") == 0)
	{
		Message = Upload (Clean);
	}
	else if (strcmp (pPath, "/del") == 0 || strcmp (pPath, "/load") == 0)
	{
		char File[HTTP_MAX_PATH + 1], CleanFile[HTTP_MAX_PATH + 1];
		GetParam (pParams, "f", File, sizeof File);
		if (File[0] != '\0' && CleanPath (File, CleanFile, sizeof CleanFile))
		{
			CString SDPath;
			SDPath.Format ("SD:/%s", CleanFile);

			if (pPath[1] == 'd')
			{
				Message.Format (f_unlink (SDPath) == FR_OK ? T ("Borrado: %s") : T ("No se pudo borrar %s"),
						CleanFile);
			}
			else
			{
				WebRequestLoad (SDPath);
				Message.Format (T ("Cargando en el Spectrum: %s"), CleanFile);
			}
		}
	}
	else if (strcmp (pPath, "/buscar") == 0)
	{
		char Query[128];
		GetParam (pParams, "q", Query, sizeof Query);
		return SendSearch (Query, pBuffer, pLength);
	}
	else if (strcmp (pPath, "/bajar") == 0)
	{
		char DownPath[HTTP_MAX_PATH + 1];
		GetParam (pParams, "p", DownPath, sizeof DownPath);
		Message = Download (DownPath);
		strcpy (Clean, "descargas");
	}
	else if (strcmp (pPath, "/mkdir") == 0)
	{
		char Name[HTTP_MAX_PATH + 1];
		GetParam (pParams, "n", Name, sizeof Name);
		if (   Name[0] != '\0' && strchr (Name, '/') == 0 && strchr (Name, '\\') == 0
		    && strstr (Name, "..") == 0 && strchr (Name, ':') == 0)
		{
			CString SDPath;
			SDPath.Format (Clean[0] != '\0' ? "SD:/%s/%s" : "SD:/%s%s", Clean, Name);
			Message.Format (f_mkdir (SDPath) == FR_OK ? T ("Carpeta creada: %s") : T ("No se pudo crear %s"),
					Name);
		}
	}
	else if (strcmp (pPath, "/") != 0 && strcmp (pPath, "/index.html") != 0)
	{
		return HTTPNotFound;
	}

	return SendDirectory (Clean, Message, pBuffer, pLength);
}

// Aviso en pantalla para las operaciones largas con ficheros (desde 32 KB):
// mientras se leen o escriben enteros en la SD, la emulación se para
#define BUSY_MIN_SIZE	(32 * 1024)

static void BusyFile (const char *pFormat, const char *pPath, unsigned nSize)
{
	if (nSize < BUSY_MIN_SIZE)
	{
		return;
	}
	const char *pName = pPath;
	for (const char *p = pPath; *p != '\0'; p++)
	{
		if (*p == '/' || *p == '\\')
		{
			pName = p + 1;
		}
	}
	CString Text;
	Text.Format (pFormat, pName, (nSize + 1023) / 1024);
	WebBusy (Text);
}

THTTPStatus CZXWebServer::SendFile (const char *pRelPath, u8 *pBuffer, unsigned *pLength,
				    const char **ppContentType)
{
	CString SDPath;
	SDPath.Format ("SD:/%s", pRelPath);

	FIL File;
	if (f_open (&File, SDPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		return HTTPNotFound;
	}

	unsigned nSize = f_size (&File);
	unsigned nRead = 0;
	if (nSize > *pLength)
	{
		f_close (&File);
		return HTTPRequestEntityTooLarge;
	}

	BusyFile (T ("Red: enviando %s (%u KB)"), pRelPath, nSize);

	FRESULT Result = f_read (&File, pBuffer, nSize, &nRead);
	f_close (&File);
	if (Result != FR_OK || nRead != nSize)
	{
		return HTTPInternalServerError;
	}

	*pLength = nSize;
	*ppContentType = "application/octet-stream";

	return HTTPOK;
}

CString CZXWebServer::Upload (const char *pDir)
{
	unsigned nSaved = 0;
	CString Message;

	const char *pHeader;
	const u8 *pData;
	unsigned nLength;
	while (GetMultipartFormPart (&pHeader, &pData, &nLength))
	{
		// Content-Disposition: form-data; name="f"; filename="juego.tap"
		const char *pFileName = strstr (pHeader, "filename=\"");
		if (pFileName == 0)
		{
			continue;
		}
		pFileName += 10;
		const char *pEnd = strchr (pFileName, '"');
		if (pEnd == 0 || pEnd == pFileName)
		{
			continue;
		}

		char Name[HTTP_MAX_PATH + 1];
		unsigned nNameLen = pEnd - pFileName;
		if (nNameLen > HTTP_MAX_PATH)
		{
			nNameLen = HTTP_MAX_PATH;
		}
		memcpy (Name, pFileName, nNameLen);
		Name[nNameLen] = '\0';

		// algunos navegadores mandan la ruta completa: solo vale el nombre
		const char *pBase = Name;
		for (const char *p = Name; *p != '\0'; p++)
		{
			if (*p == '/' || *p == '\\' || *p == ':')
			{
				pBase = p + 1;
			}
		}
		if (*pBase == '\0' || strstr (pBase, "..") != 0)
		{
			continue;
		}

		CString SDPath;
		SDPath.Format (pDir[0] != '\0' ? "SD:/%s/%s" : "SD:/%s%s", pDir, pBase);

		BusyFile (T ("Red: guardando %s (%u KB)"), pBase, nLength);

		FIL File;
		unsigned nWritten = 0;
		boolean bOK =    f_open (&File, SDPath, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK
			      && f_write (&File, pData, nLength, &nWritten) == FR_OK
			      && nWritten == nLength;
		f_close (&File);

		if (bOK)
		{
			nSaved++;
		}
		else
		{
			Message.Format (T ("No se pudo guardar %s"), pBase);
			return Message;
		}
	}

	if (nSaved == 0)
	{
		Message = T ("No se ha recibido ningún fichero");
	}
	else
	{
		Message.Format (T ("Subidos %u fichero(s)"), nSaved);
	}

	return Message;
}

struct TDirEntry
{
	char Name[128];
	boolean bDir;
	unsigned nSize;
};

static int CompareEntries (const TDirEntry *a, const TDirEntry *b)
{
	if (a->bDir != b->bDir)
	{
		return a->bDir ? -1 : 1;
	}
	return strcasecmp (a->Name, b->Name);
}

THTTPStatus CZXWebServer::SendDirectory (const char *pDir, const char *pMessage,
					 u8 *pBuffer, unsigned *pLength)
{
	// Lee el directorio
	const unsigned nMaxEntries = 2000;
	TDirEntry *pEntries = new TDirEntry[nMaxEntries];
	if (pEntries == 0)
	{
		return HTTPInternalServerError;
	}

	unsigned nEntries = 0;
	CString SDDir;
	SDDir.Format ("SD:/%s", pDir);
	DIR Dir;
	FILINFO Info;
	if (f_opendir (&Dir, SDDir) == FR_OK)
	{
		while (   nEntries < nMaxEntries
		       && f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
		{
			if (Info.fattrib & (AM_HID | AM_SYS))
			{
				continue;
			}
			if (   CExclusions::Get () != 0		// excluir.txt
			    && CExclusions::Get ()->IsExcluded (SDDir, Info.fname, (Info.fattrib & AM_DIR) != 0))
			{
				continue;
			}
			strncpy (pEntries[nEntries].Name, Info.fname, sizeof pEntries[0].Name - 1);
			pEntries[nEntries].Name[sizeof pEntries[0].Name - 1] = '\0';
			pEntries[nEntries].bDir = (Info.fattrib & AM_DIR) != 0;
			pEntries[nEntries].nSize = Info.fsize;
			nEntries++;
		}
		f_closedir (&Dir);
	}

	// orden: carpetas primero, luego por nombre (inserción: listas pequeñas)
	for (unsigned i = 1; i < nEntries; i++)
	{
		TDirEntry Tmp = pEntries[i];
		unsigned j = i;
		while (j > 0 && CompareEntries (&pEntries[j - 1], &Tmp) > 0)
		{
			pEntries[j] = pEntries[j - 1];
			j--;
		}
		pEntries[j] = Tmp;
	}

	CString Html;
	CString Esc, Enc, Line;

	Html.Format ("<!DOCTYPE html><html lang=\"%s\"><head><meta charset=\"utf-8\">", CI18n::Get ()->GetLanguage ());
	Html.Append ("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
	       "<title>ZXMetalCircle</title><style>"
	       "body{font-family:sans-serif;background:#10123a;color:#e8e8e8;margin:0;padding:12px}"
	       "h1{color:#eeeb46;font-size:1.3em;margin:0 0 4px}"
	       "a{color:#7fd6ff}.st{color:#8a8ca0;font-size:.9em}"
	       ".msg{background:#36efde;color:#000;padding:6px;margin:8px 0}"
	       "table{border-collapse:collapse;width:100%}td{padding:4px 6px;border-bottom:1px solid #2a2c5a}"
	       "td.s{text-align:right;color:#8a8ca0;white-space:nowrap}"
	       "form{margin:10px 0;padding:8px;background:#1c1f55}"
	       "</style></head><body>");

	Line.Format ("<h1>ZXMetalCircle %s</h1><div class=\"st\">", ZXCIRCLE_VERSION);
	Html.Append (Line);
	HTMLEscape (WebGetStatus (), &Esc);
	Html.Append (Esc);
	Html.Append ("</div>");

	Line.Format ("<form action=\"/buscar\">%s "
		     "<input name=\"q\"> <input type=\"submit\" value=\"%s\"></form>",
		     T ("Buscar programas en internet (ZXDB):"), T ("Buscar"));
	Html.Append (Line);

	if (pMessage != 0 && *pMessage != '\0')
	{
		HTMLEscape (pMessage, &Esc);
		Html.Append ("<div class=\"msg\">");
		Html.Append (Esc);
		Html.Append ("</div>");
	}

	// ruta actual con enlaces a cada nivel
	Html.Append ("<p><a href=\"/\">SD:</a>");
	{
		char Partial[HTTP_MAX_PATH + 1];
		unsigned n = 0;
		for (const char *p = pDir; *p != '\0'; p++)
		{
			Partial[n++] = *p;
			if (p[1] == '/' || p[1] == '\0')
			{
				Partial[n] = '\0';
				const char *pLast = FindLast (Partial, '/');
				HTMLEscape (pLast != 0 ? pLast + 1 : Partial, &Esc);
				URLEncode (Partial, &Enc);
				Line.Format (" / <a href=\"/?d=%s\">%s</a>", (const char *) Enc, (const char *) Esc);
				Html.Append (Line);
			}
		}
	}
	Html.Append ("</p>");

	URLEncode (pDir, &Enc);
	CString DirEnc = Enc;

	boolean bReadOnly = WebMode () == 2;
	if (!bReadOnly)
	{
		Line.Format ("<form method=\"post\" enctype=\"multipart/form-data\" action=\"/up?d=%s\">"
			     "%s <input type=\"file\" name=\"f\" multiple> "
			     "<input type=\"submit\" value=\"%s\"></form>", (const char *) DirEnc,
			     T ("Subir a esta carpeta:"), T ("Subir"));
		Html.Append (Line);
	}

	Html.Append ("<table>");
	if (pDir[0] != '\0')
	{
		char Parent[HTTP_MAX_PATH + 1];
		strncpy (Parent, pDir, sizeof Parent - 1);
		Parent[sizeof Parent - 1] = '\0';
		char *pSlash = (char *) FindLast (Parent, '/');
		if (pSlash != 0)
		{
			*pSlash = '\0';
		}
		else
		{
			Parent[0] = '\0';
		}
		URLEncode (Parent, &Enc);
		Line.Format ("<tr><td><a href=\"/?d=%s\">[..]</a></td><td></td><td></td></tr>",
			     (const char *) Enc);
		Html.Append (Line);
	}

	for (unsigned i = 0; i < nEntries; i++)
	{
		CString Rel;
		Rel.Format (pDir[0] != '\0' ? "%s/%s" : "%s%s", pDir, pEntries[i].Name);
		URLEncode (Rel, &Enc);
		HTMLEscape (pEntries[i].Name, &Esc);

		if (pEntries[i].bDir)
		{
			Line.Format ("<tr><td><a href=\"/?d=%s\">[%s]</a></td><td></td><td></td></tr>",
				     (const char *) Enc, (const char *) Esc);
		}
		else
		{
			CString Actions;
			if (IsLoadable (pEntries[i].Name))
			{
				Actions.Format ("<a href=\"/load?d=%s&f=%s\">%s</a> ",
						(const char *) DirEnc, (const char *) Enc, T ("cargar"));
			}
			CString Del;
			CString Ask;
			Ask.Format (T ("¿Borrar %s?"), (const char *) Esc);
			CString AskJS;		// para JavaScript: sin comillas simples
			for (const char *p = Ask; *p != '\0'; p++)
			{
				char c[2] = {*p == '\'' ? ' ' : *p, '\0'};
				AskJS.Append (c);
			}
			if (!bReadOnly)
			{
				Del.Format ("<a href=\"/del?d=%s&f=%s\" onclick=\"return confirm('%s')\">%s</a>",
					    (const char *) DirEnc, (const char *) Enc, (const char *) AskJS, T ("borrar"));
				Actions.Append (Del);
			}

			Line.Format ("<tr><td><a href=\"/sd/%s\" download>%s</a></td>"
				     "<td class=\"s\">%u KB</td><td>%s</td></tr>",
				     (const char *) Enc, (const char *) Esc,
				     (pEntries[i].nSize + 1023) / 1024, (const char *) Actions);
		}
		Html.Append (Line);
	}
	Html.Append ("</table>");

	HTMLEscape (pDir, &Esc);
	if (!bReadOnly)
	{
		Line.Format ("<form action=\"/mkdir\"><input type=\"hidden\" name=\"d\" value=\"%s\">"
			     "%s <input name=\"n\"> <input type=\"submit\" value=\"%s\"></form>",
			     (const char *) Esc, T ("Nueva carpeta:"), T ("Crear"));
		Html.Append (Line);
	}
	const char *pNote = *WebPassword () == '\0' ? T ("Sin contraseña: úsala solo en tu red local.")
						     : T ("Con contraseña, pero sin cifrar: úsala en tu red local.");
	Line.Format ("<p class=\"st\">%s%s</p></body></html>", pNote,
		     bReadOnly ? T (" Modo solo lectura.") : "");
	Html.Append (Line);
	AddToken (&Html);

	delete [] pEntries;

	unsigned nLen = Html.GetLength ();
	if (nLen > *pLength)
	{
		return HTTPInternalServerError;
	}
	memcpy (pBuffer, (const char *) Html, nLen);
	*pLength = nLen;

	return HTTPOK;
}

//------------------------------------------------------------------------------
// Búsqueda y descarga de programas (ZXDB / Spectrum Computing, por HTTPS)

#define DOWNLOAD_WORK_SIZE	(4 * 1024 * 1024)
#define DOWNLOAD_MAX_FILE	(2 * 1024 * 1024)

// Certificados raíz: SD:/certs/cacert.pem (se cargan la primera vez)
static void EnsureCertificates (void)
{
	static boolean bDone = FALSE;
	if (bDone)
	{
		return;
	}
	bDone = TRUE;

	if (!WebTLSVerify ())
	{
		HTTPSSetRootCerts (0, 0);
		CLogger::Get ()->Write ("web", LogWarning, "HTTPS sin comprobar certificados (tlsverify=0)");
		return;
	}

	FIL File;
	if (f_open (&File, "SD:/certs/cacert.pem", FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		CLogger::Get ()->Write ("web", LogWarning, "Falta SD:/certs/cacert.pem: no se comprueban certificados");
		return;
	}
	unsigned nSize = f_size (&File), nRead = 0;
	char *pPEM = new char[nSize + 1];
	if (pPEM != 0 && f_read (&File, pPEM, nSize, &nRead) == FR_OK && nRead == nSize)
	{
		pPEM[nSize] = '\0';
		HTTPSSetRootCerts (pPEM, nSize + 1);
	}
	f_close (&File);
	delete [] pPEM;		// Mbed TLS se queda con su propia copia
}

// Las conexiones HTTPS se hacen en una tarea propia con 128 KB de pila: Mbed
// TLS necesita más de los 32 KB de una tarea normal de Circle. La tarea del
// servidor web espera (cediendo el control) a que termine.
struct TNetJob
{
	boolean bSearch;
	char Arg[256];
	TZXInfoResult Results[ZXINFO_MAX_RESULTS];
	int nCount;
	char Error[96];
	char Message[160];
	volatile boolean bDone;
};

static void DoSearch (TNetJob *pJob);
static void DoDownload (TNetJob *pJob);

class CNetJobTask : public CTask
{
public:
	CNetJobTask (TNetJob *pJob) : CTask (0x20000), m_pJob (pJob) {}
	void Run (void)
	{
		EnsureCertificates ();
		if (m_pJob->bSearch)
		{
			DoSearch (m_pJob);
		}
		else
		{
			DoDownload (m_pJob);
		}
		m_pJob->bDone = TRUE;		// la tarea se borra sola al volver
	}
private:
	TNetJob *m_pJob;
};

static void RunJob (TNetJob *pJob)
{
	pJob->bDone = FALSE;
	new CNetJobTask (pJob);
	while (!pJob->bDone)
	{
		CScheduler::Get ()->MsSleep (20);
	}
}

static void DoSearch (TNetJob *pJob)
{
	WebBusy (T ("Red: buscando en ZXDB..."));
	u8 *pWork = new u8[DOWNLOAD_WORK_SIZE];
	CCircleTransport Transport;
	CHTTPSClient Client (&Transport);
	const char *pError = N_("No hay memoria");
	pJob->nCount = pWork != 0
		       ? ZXInfoSearch (&Client, pJob->Arg, pJob->Results, ZXINFO_MAX_RESULTS, pWork,
				       DOWNLOAD_WORK_SIZE, &pError)
		       : -1;
	strncpy (pJob->Error, T (pError), sizeof pJob->Error - 1);
	pJob->Error[sizeof pJob->Error - 1] = '\0';
	delete [] pWork;
}

THTTPStatus CZXWebServer::SendSearch (const char *pQuery, u8 *pBuffer, unsigned *pLength)
{

	CString Html, Line, Esc, Enc;
	Html.Format ("<!DOCTYPE html><html lang=\"%s\"><head><meta charset=\"utf-8\">"
	       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
	       "<title>ZXMetalCircle - %s</title><style>", CI18n::Get ()->GetLanguage (), T ("Buscar"));
	Html.Append (
	       "body{font-family:sans-serif;background:#10123a;color:#e8e8e8;margin:0;padding:12px}"
	       "h1{color:#eeeb46;font-size:1.3em}a{color:#7fd6ff}.st{color:#8a8ca0;font-size:.9em}"
	       "table{border-collapse:collapse;width:100%}td{padding:4px 6px;border-bottom:1px solid #2a2c5a}"
	       "form{margin:10px 0;padding:8px;background:#1c1f55}"
	       "</style></head><body>");
	Line.Format ("<h1>%s</h1><p><a href=\"/\">%s</a></p>", T ("Buscar programas"), T ("Volver a la SD"));
	Html.Append (Line);

	HTMLEscape (pQuery, &Esc);
	Line.Format ("<form action=\"/buscar\"><input name=\"q\" value=\"%s\"> "
		     "<input type=\"submit\" value=\"%s\"></form>", (const char *) Esc, T ("Buscar"));
	Html.Append (Line);

	TNetJob *pJob = pQuery[0] != '\0' ? new TNetJob : 0;
	if (pJob != 0)
	{
		pJob->bSearch = TRUE;
		strncpy (pJob->Arg, pQuery, sizeof pJob->Arg - 1);
		pJob->Arg[sizeof pJob->Arg - 1] = '\0';
		RunJob (pJob);
		int nCount = pJob->nCount;
		const char *pError = pJob->Error;
		const TZXInfoResult *pResults = pJob->Results;

		if (nCount < 0)
		{
			HTMLEscape (pError, &Esc);
			Line.Format (T ("<p>No se pudo buscar: %s</p>"), (const char *) Esc);
			Html.Append (Line);
		}
		else if (nCount == 0)
		{
			Html.Append (T ("<p>No se ha encontrado nada.</p>"));
		}
		else
		{
			Html.Append ("<table>");
			for (int i = 0; i < nCount; i++)
			{
				const TZXInfoResult &R = pResults[i];
				CString Title, Pub, Action;
				HTMLEscape (R.Title, &Title);
				HTMLEscape (R.Publisher, &Pub);
				if (R.Path[0] != '\0' && WebMode () != 2)
				{
					URLEncode (R.Path, &Enc);
					Action.Format ("<a href=\"/bajar?p=%s\">%s (%s)</a>",
						       (const char *) Enc, T ("descargar y cargar"), R.Format);
				}
				else
				{
					Action.Format ("<span class=\"st\">%s</span>", T ("sin fichero"));
				}
				Line.Format ("<tr><td><a href=\"https://spectrumcomputing.co.uk/entry/%s\" target=\"_blank\">%s</a>"
					     "<br><span class=\"st\">%u %s - %s</span></td><td>%s</td></tr>",
					     R.ID, (const char *) Title, R.nYear, (const char *) Pub, R.Machine,
					     (const char *) Action);
				Html.Append (Line);
			}
			Html.Append ("</table>");
		}

		delete pJob;
	}

	Line.Format ("<p class=\"st\">%s</p></body></html>", T ("Datos de ZXDB a través de ZXInfo; ficheros de Spectrum Computing."));
	Html.Append (Line);

	AddToken (&Html);
	unsigned nLen = Html.GetLength ();
	if (nLen > *pLength)
	{
		return HTTPInternalServerError;
	}
	memcpy (pBuffer, (const char *) Html, nLen);
	*pLength = nLen;
	return HTTPOK;
}

CString CZXWebServer::Download (const char *pPath)
{
	CString Message;
	TNetJob *pJob = new TNetJob;
	if (pJob == 0)
	{
		Message = T ("No hay memoria para la descarga");
		return Message;
	}
	pJob->bSearch = FALSE;
	strncpy (pJob->Arg, pPath, sizeof pJob->Arg - 1);
	pJob->Arg[sizeof pJob->Arg - 1] = '\0';
	RunJob (pJob);
	Message = pJob->Message;
	delete pJob;
	return Message;
}

// Descarga a SD:/descargas y pide cargarlo en el Spectrum (en la tarea)
static void DoDownload (TNetJob *pJob)
{
	WebBusy (T ("Red: descargando de ZXDB..."));

	const char *pPath = pJob->Arg;
	CString Message;
	u8 *pWork = new u8[DOWNLOAD_WORK_SIZE];
	u8 *pFile = new u8[DOWNLOAD_MAX_FILE];
	if (pWork == 0 || pFile == 0)
	{
		delete [] pWork;
		delete [] pFile;
		Message = T ("No hay memoria para la descarga");
		strncpy (pJob->Message, Message, sizeof pJob->Message - 1);
		pJob->Message[sizeof pJob->Message - 1] = '\0';
		return;
	}

	CCircleTransport Transport;
	CHTTPSClient Client (&Transport);
	char Name[128];
	const char *pError = "";
	size_t nSize = ZXInfoDownload (&Client, pPath, pFile, DOWNLOAD_MAX_FILE, Name, sizeof Name,
				       pWork, DOWNLOAD_WORK_SIZE, &pError);
	delete [] pWork;

	if (nSize == 0)
	{
		Message.Format (T ("No se pudo descargar: %s"), T (pError));
		delete [] pFile;
		strncpy (pJob->Message, Message, sizeof pJob->Message - 1);
		pJob->Message[sizeof pJob->Message - 1] = '\0';
		return;
	}

	// nombre seguro para FAT
	for (char *p = Name; *p != '\0'; p++)
	{
		if (*p == '/' || *p == '\\' || *p == ':' || *p == '*' || *p == '?' || *p == '"'
		    || *p == '<' || *p == '>' || *p == '|')
		{
			*p = '_';
		}
	}

	f_mkdir ("SD:/descargas");
	CString SDPath;
	SDPath.Format ("SD:/descargas/%s", Name);
	FIL File;
	unsigned nWritten = 0;
	boolean bOK =    f_open (&File, SDPath, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK
		      && f_write (&File, pFile, nSize, &nWritten) == FR_OK && nWritten == nSize;
	f_close (&File);
	delete [] pFile;

	if (!bOK)
	{
		Message.Format (T ("No se pudo guardar %s"), Name);
		strncpy (pJob->Message, Message, sizeof pJob->Message - 1);
		pJob->Message[sizeof pJob->Message - 1] = '\0';
		return;
	}

	WebRequestLoad (SDPath);
	Message.Format (T ("Descargado y cargando en el Spectrum: %s (%u KB)"), Name, (unsigned) (nSize + 1023) / 1024);
	strncpy (pJob->Message, Message, sizeof pJob->Message - 1);
	pJob->Message[sizeof pJob->Message - 1] = '\0';
}
