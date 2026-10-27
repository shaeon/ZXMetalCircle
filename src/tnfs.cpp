//
// tnfs.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "tnfs.h"
#include "i18n.h"
#include <circle/util.h>

#define CMD_MOUNT	0x00
#define CMD_UMOUNT	0x01
#define CMD_OPENDIR	0x10
#define CMD_READDIR	0x11
#define CMD_CLOSEDIR	0x12
#define CMD_OPENDIRX	0x17
#define CMD_READDIRX	0x18
#define CMD_READ	0x21
#define CMD_CLOSE	0x23
#define CMD_STAT	0x24
#define CMD_OPEN	0x29

#define STATUS_OK	0x00
#define STATUS_EOF	0x21

#define READ_CHUNK	512		// máximo por datagrama UDP
#define TIMEOUT_MS	1500
#define RETRIES		4

CTNFSClient::CTNFSClient (CTNFSTransport *pTransport)
:	m_pTransport (pTransport),
	m_bMounted (FALSE),
	m_nSession (0),
	m_nSeq (0),
	m_nRetryMs (TIMEOUT_MS),
	m_nDirHandle (-1),
	m_pProgress (0),
	m_pProgressParam (0),
	m_pError ("")
{
	m_Host[0] = '\0';
}

CTNFSClient::~CTNFSClient (void)
{
	Unmount ();
}

unsigned CTNFSClient::Request (u8 nCommand, const u8 *pData, unsigned nLength)
{
	if (nLength + 4 > sizeof m_Request)
	{
		m_pError = N_("Petición demasiado larga");
		return 0;
	}

	m_Request[0] = m_nSession & 0xFF;
	m_Request[1] = m_nSession >> 8;
	m_Request[2] = m_nSeq;
	m_Request[3] = nCommand;
	memcpy (m_Request + 4, pData, nLength);

	for (unsigned nTry = 0; nTry < RETRIES; nTry++)
	{
		if (!m_pTransport->Send (m_Request, nLength + 4))
		{
			m_pError = N_("No se pudo enviar");
			return 0;
		}

		// se descartan respuestas de peticiones anteriores (reintentos tardíos)
		for (;;)
		{
			unsigned nReply = m_pTransport->Receive (m_Reply, sizeof m_Reply, m_nRetryMs);
			if (nReply == 0)
			{
				break;		// tiempo agotado: reintento
			}
			if (nReply >= 5 && m_Reply[2] == m_nSeq && m_Reply[3] == nCommand)
			{
				m_nSeq++;
				return nReply;
			}
		}
	}

	m_pError = N_("El servidor no responde");
	return 0;
}

boolean CTNFSClient::Mount (const char *pHost)
{
	Unmount ();

	if (!m_pTransport->Open (pHost, TNFS_PORT))
	{
		m_pError = N_("No se encuentra el servidor");
		return FALSE;
	}

	m_nSession = 0;
	m_nSeq = 0;
	m_nRetryMs = TIMEOUT_MS;

	// versión 1.2, punto de montaje "/", sin usuario ni contraseña
	static const u8 Data[] = {0x02, 0x01, '/', 0, 0, 0};
	unsigned nReply = Request (CMD_MOUNT, Data, sizeof Data);
	if (nReply == 0)
	{
		m_pTransport->Close ();
		return FALSE;
	}

	if (m_Reply[4] != STATUS_OK)
	{
		m_pError = N_("El servidor rechaza la conexión");
		m_pTransport->Close ();
		return FALSE;
	}

	m_nSession = m_Reply[0] | (m_Reply[1] << 8);
	if (nReply >= 9)
	{
		unsigned nRetry = m_Reply[7] | (m_Reply[8] << 8);
		if (nRetry > m_nRetryMs && nRetry < 10000)
		{
			m_nRetryMs = nRetry;	// hay que respetar el mínimo del servidor
		}
	}

	strncpy (m_Host, pHost, sizeof m_Host - 1);
	m_Host[sizeof m_Host - 1] = '\0';
	m_bMounted = TRUE;
	return TRUE;
}

void CTNFSClient::Unmount (void)
{
	if (!m_bMounted)
	{
		return;
	}

	CloseDir ();
	Request (CMD_UMOUNT, 0, 0);
	m_pTransport->Close ();
	m_bMounted = FALSE;
	m_Host[0] = '\0';
}

boolean CTNFSClient::OpenDir (const char *pPath)
{
	CloseDir ();

	unsigned nLength = strlen (pPath) + 1;
	if (!m_bMounted || nLength > TNFS_MAX_PATH)
	{
		return FALSE;
	}

	unsigned nReply = Request (CMD_OPENDIR, (const u8 *) pPath, nLength);
	if (nReply < 6 || m_Reply[4] != STATUS_OK)
	{
		if (nReply != 0)
		{
			m_pError = N_("No es una carpeta");
		}
		return FALSE;
	}

	m_nDirHandle = m_Reply[5];
	return TRUE;
}

boolean CTNFSClient::ReadDir (char *pName, unsigned nSize)
{
	if (m_nDirHandle < 0)
	{
		return FALSE;
	}

	u8 Handle = (u8) m_nDirHandle;
	unsigned nReply = Request (CMD_READDIR, &Handle, 1);
	if (nReply < 6 || m_Reply[4] != STATUS_OK)
	{
		return FALSE;		// fin de la lista (EOF) o error
	}

	unsigned n = 0;
	for (unsigned i = 5; i < nReply && m_Reply[i] != '\0' && n < nSize - 1; i++)
	{
		pName[n++] = (char) m_Reply[i];
	}
	pName[n] = '\0';
	return TRUE;
}

void CTNFSClient::CloseDir (void)
{
	if (m_nDirHandle >= 0)
	{
		u8 Handle = (u8) m_nDirHandle;
		Request (CMD_CLOSEDIR, &Handle, 1);
		m_nDirHandle = -1;
	}
}

boolean CTNFSClient::Stat (const char *pPath, boolean *pIsDir, u32 *pSize)
{
	unsigned nLength = strlen (pPath) + 1;
	if (!m_bMounted || nLength > TNFS_MAX_PATH)
	{
		return FALSE;
	}

	unsigned nReply = Request (CMD_STAT, (const u8 *) pPath, nLength);
	if (nReply < 5 + 10 || m_Reply[4] != STATUS_OK)
	{
		return FALSE;
	}

	u16 nMode = m_Reply[5] | (m_Reply[6] << 8);
	*pIsDir = (nMode & 0xF000) == 0x4000;		// S_IFDIR
	*pSize = m_Reply[11] | (m_Reply[12] << 8) | (m_Reply[13] << 16) | ((u32) m_Reply[14] << 24);
	return TRUE;
}

unsigned CTNFSClient::ReadFile (const char *pPath, u8 *pBuffer, unsigned nSize)
{
	unsigned nLength = strlen (pPath) + 1;
	if (!m_bMounted || nLength + 4 > TNFS_MAX_PATH)
	{
		return 0;
	}

	// OPEN: flags O_RDONLY, modo 0, ruta
	u8 Data[TNFS_MAX_PATH + 4];
	Data[0] = 0x01; Data[1] = 0x00;
	Data[2] = 0x00; Data[3] = 0x00;
	memcpy (Data + 4, pPath, nLength);
	unsigned nReply = Request (CMD_OPEN, Data, nLength + 4);
	if (nReply < 6 || m_Reply[4] != STATUS_OK)
	{
		if (nReply != 0)
		{
			m_pError = N_("No se puede abrir el fichero");
		}
		return 0;
	}
	u8 nHandle = m_Reply[5];

	unsigned nTotal = 0;
	boolean bOK = TRUE;
	for (;;)
	{
		u8 ReadReq[3] = {nHandle, READ_CHUNK & 0xFF, READ_CHUNK >> 8};
		nReply = Request (CMD_READ, ReadReq, 3);
		if (nReply < 5)
		{
			bOK = FALSE;
			break;
		}
		if (m_Reply[4] == STATUS_EOF)
		{
			break;
		}
		if (m_Reply[4] != STATUS_OK || nReply < 7)
		{
			m_pError = N_("Error al leer el fichero");
			bOK = FALSE;
			break;
		}

		unsigned nBytes = m_Reply[5] | (m_Reply[6] << 8);
		if (nBytes > nReply - 7)
		{
			nBytes = nReply - 7;
		}
		if (nTotal + nBytes > nSize)
		{
			m_pError = N_("Fichero demasiado grande");
			bOK = FALSE;
			break;
		}
		memcpy (pBuffer + nTotal, m_Reply + 7, nBytes);
		nTotal += nBytes;
		if (m_pProgress != 0 && (nTotal >> 13) != ((nTotal - nBytes) >> 13))
		{
			m_pProgress (nTotal, m_pProgressParam);	// cada 8 KB
		}
		if (nBytes == 0)
		{
			break;
		}
	}

	Request (CMD_CLOSE, &nHandle, 1);

	return bOK ? nTotal : 0;
}

static boolean LooksLikeFile (const char *pName)
{
	// extensiones de ficheros de Spectrum: no hace falta preguntar si son carpetas
	static const char *Ext[] = {".tap", ".tzx", ".z80", ".sna", ".szx", ".csw", ".rom",
				    ".mdr", ".mdv", ".dsk", ".scr", ".txt", ".zip", ".bin",
				    ".pzx", ".trd", ".scl", ".hdf", ".img", ".vhd", ".raw"};
	size_t nLen = strlen (pName);
	for (unsigned i = 0; i < sizeof Ext / sizeof Ext[0]; i++)
	{
		size_t nExt = strlen (Ext[i]);
		if (nLen > nExt && strcasecmp (pName + nLen - nExt, Ext[i]) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

boolean CTNFSClient::ListDir (const char *pPath, CTNFSListener *pListener, boolean bForceOld)
{
	CloseDir ();
	unsigned nPathLen = strlen (pPath) + 1;
	if (!m_bMounted || nPathLen + 6 > TNFS_MAX_PATH)
	{
		return FALSE;
	}

	if (!bForceOld)
	{
		// OPENDIRX: opciones 0, orden 0 (por nombre, carpetas primero), sin
		// límite, sin patrón
		u8 Data[TNFS_MAX_PATH + 8];
		Data[0] = 0; Data[1] = 0; Data[2] = 0; Data[3] = 0;
		Data[4] = 0;			// patrón vacío
		memcpy (Data + 5, pPath, nPathLen);
		unsigned nReply = Request (CMD_OPENDIRX, Data, nPathLen + 5);
		if (nReply == 0)
		{
			return FALSE;		// el servidor no responde
		}
		if (nReply >= 6 && m_Reply[4] == STATUS_OK)
		{
			u8 nHandle = m_Reply[5];
			boolean bOK = TRUE;
			for (;;)
			{
				u8 Req[2] = {nHandle, 0};	// tantas entradas como quepan
				nReply = Request (CMD_READDIRX, Req, 2);
				if (nReply < 5 || m_Reply[4] == STATUS_EOF)
				{
					bOK = nReply != 0;
					break;
				}
				if (m_Reply[4] != STATUS_OK || nReply < 9)
				{
					bOK = FALSE;
					break;
				}

				unsigned nCount = m_Reply[5];
				boolean bLast = (m_Reply[6] & 0x01) != 0;
				unsigned nPos = 9;
				for (unsigned i = 0; i < nCount && nPos + 13 < nReply; i++)
				{
					u8 nFlags = m_Reply[nPos];
					const char *pName = (const char *) &m_Reply[nPos + 13];
					unsigned nNameLen = strnlen (pName, nReply - nPos - 13);
					if (nPos + 13 + nNameLen >= nReply)
					{
						break;
					}
					m_Reply[nPos + 13 + nNameLen] = '\0';
					if (   strcmp (pName, ".") != 0 && strcmp (pName, "..") != 0
					    && !(nFlags & 0x02))			// oculto
					{
						pListener->OnEntry (pName, (nFlags & 0x01) != 0);
					}
					nPos += 13 + nNameLen + 1;
				}
				if (bLast || nCount == 0)
				{
					break;
				}
			}
			Request (CMD_CLOSEDIR, &nHandle, 1);
			return bOK;
		}
		// el servidor no conoce OPENDIRX: se usa la forma clásica
	}

	if (!OpenDir (pPath))
	{
		return FALSE;
	}

	// primero los nombres (STAT con la carpeta abierta podría confundir a
	// servidores sencillos), luego se pregunta por lo que no parezca fichero
	static char Names[512][TNFS_MAX_PATH / 2];
	unsigned nNames = 0;
	char Name[TNFS_MAX_PATH];
	while (nNames < 512 && ReadDir (Name, sizeof Name))
	{
		if (strcmp (Name, ".") == 0 || strcmp (Name, "..") == 0 || Name[0] == '.')
		{
			continue;
		}
		strncpy (Names[nNames], Name, sizeof Names[0] - 1);
		Names[nNames][sizeof Names[0] - 1] = '\0';
		nNames++;
	}
	CloseDir ();

	for (unsigned i = 0; i < nNames; i++)
	{
		boolean bDir = FALSE;
		if (!LooksLikeFile (Names[i]))
		{
			char Full[TNFS_MAX_PATH];
			unsigned nBase = strlen (pPath);
			if (nBase + strlen (Names[i]) + 2 < sizeof Full)
			{
				strcpy (Full, pPath);
				if (nBase == 0 || Full[nBase - 1] != '/')
				{
					strcat (Full, "/");
				}
				strcat (Full, Names[i]);
				u32 nSize;
				if (!Stat (Full, &bDir, &nSize))
				{
					bDir = FALSE;
				}
			}
		}
		pListener->OnEntry (Names[i], bDir);
	}

	return TRUE;
}
