//
// wificonfig.cpp - Las redes WiFi guardadas (SD:/wpa_supplicant.conf)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "wificonfig.h"
#include <string.h>

// Cabecera de un fichero nuevo (cuando no había ninguno)
static const char DefaultHeader[] =
	"#\n"
	"# Configuración de la WiFi de ZXMetalCircle\n"
	"#\n"
	"# Se puede cambiar desde la Pi: Configuración, página 7 (Red y hora), \"Red WiFi\".\n"
	"# A mano: una red por bloque network={...}; se conecta a la de más prioridad\n"
	"# (priority=) que esté al alcance. Solo redes de 2,4 GHz con WPA o WPA2.\n"
	"#\n";

// Copia acotada que siempre termina en '\0'
static void CopyN (char *pTo, unsigned nSize, const char *pFrom, unsigned nLen)
{
	if (nSize == 0)
	{
		return;
	}
	if (nLen >= nSize)
	{
		nLen = nSize - 1;
	}
	memcpy (pTo, pFrom, nLen);
	pTo[nLen] = '\0';
}

// Añade texto al final de un búfer acotado; FALSE si no cabe
static bool Append (char *pBuffer, unsigned nSize, unsigned *pLen, const char *pText, unsigned nLen)
{
	if (*pLen + nLen >= nSize)
	{
		return false;
	}
	memcpy (pBuffer + *pLen, pText, nLen);
	*pLen += nLen;
	pBuffer[*pLen] = '\0';
	return true;
}

static bool Append (char *pBuffer, unsigned nSize, unsigned *pLen, const char *pText)
{
	return Append (pBuffer, nSize, pLen, pText, (unsigned) strlen (pText));
}

static bool AppendNumber (char *pBuffer, unsigned nSize, unsigned *pLen, int nValue)
{
	char Digits[12];
	unsigned n = 0;
	bool bNegative = nValue < 0;
	unsigned u = bNegative ? -nValue : nValue;
	do
	{
		Digits[n++] = '0' + u % 10;
		u /= 10;
	}
	while (u != 0);
	char Text[13];
	unsigned l = 0;
	if (bNegative)
	{
		Text[l++] = '-';
	}
	while (n > 0)
	{
		Text[l++] = Digits[--n];
	}
	return Append (pBuffer, nSize, pLen, Text, l);
}

static bool IsSpace (char c)
{
	return c == ' ' || c == '\t' || c == '\r';
}

// Valor de "clave=valor" (sin espacios alrededor): si va entre comillas, lo de
// dentro (hasta la última comilla) y *pQuoted = true
static void GetValue (const char *pValue, unsigned nLen, char *pOut, unsigned nSize, bool *pQuoted)
{
	*pQuoted = false;
	if (nLen >= 2 && pValue[0] == '"')
	{
		unsigned nEnd = nLen - 1;
		while (nEnd > 0 && pValue[nEnd] != '"')
		{
			nEnd--;
		}
		if (nEnd > 0)
		{
			*pQuoted = true;
			CopyN (pOut, nSize, pValue + 1, nEnd - 1);
			return;
		}
	}
	CopyN (pOut, nSize, pValue, nLen);
}

static bool IsHex64 (const char *p)
{
	unsigned n = 0;
	for (; p[n] != '\0'; n++)
	{
		char c = p[n];
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
		{
			return false;
		}
	}
	return n == 64;
}

static int ParseInt (const char *p)
{
	bool bNegative = *p == '-';
	if (bNegative)
	{
		p++;
	}
	int n = 0;
	while (*p >= '0' && *p <= '9')
	{
		n = n * 10 + (*p++ - '0');
	}
	return bNegative ? -n : n;
}

CWiFiConfig::CWiFiConfig (void)
{
	Clear ();
}

void CWiFiConfig::Clear (void)
{
	m_Country[0] = '\0';
	m_Header[0] = '\0';
	m_nCount = 0;
	memset (m_Networks, 0, sizeof m_Networks);
}

bool CWiFiConfig::Parse (const char *pText)
{
	Clear ();
	unsigned nHeader = 0;
	TWiFiNetwork *pNet = 0;		// dentro de network={...}
	unsigned nExtra = 0;

	const char *p = pText;
	while (*p != '\0')
	{
		// una línea, sin el '\n'
		const char *pLine = p;
		while (*p != '\0' && *p != '\n')
		{
			p++;
		}
		unsigned nLineLen = (unsigned) (p - pLine);
		if (*p == '\n')
		{
			p++;
		}

		// sin espacios delante ni detrás
		const char *s = pLine;
		const char *e = pLine + nLineLen;
		while (s < e && IsSpace (*s)) s++;
		while (e > s && IsSpace (e[-1])) e--;
		unsigned nLen = (unsigned) (e - s);

		if (pNet == 0)
		{
			if (nLen >= 9 && strncmp (s, "network={", 9) == 0)
			{
				if (m_nCount == WIFI_MAX_NETWORKS)
				{
					return false;
				}
				pNet = &m_Networks[m_nCount];
				memset (pNet, 0, sizeof *pNet);
				nExtra = 0;
				continue;
			}
			if (nLen > 8 && strncmp (s, "country=", 8) == 0)
			{
				CopyN (m_Country, sizeof m_Country, s + 8, nLen - 8);
				continue;
			}
			// lo demás de fuera se conserva tal cual (comentarios, opciones)
			if (   !Append (m_Header, sizeof m_Header, &nHeader, pLine, nLineLen)
			    || !Append (m_Header, sizeof m_Header, &nHeader, "\n"))
			{
				return false;
			}
			continue;
		}

		// dentro de una red
		if (nLen == 1 && *s == '}')
		{
			if (pNet->SSID[0] == '\0')
			{
				return false;		// una red sin nombre: no se entiende
			}
			m_nCount++;
			pNet = 0;
			continue;
		}

		const char *pEqual = (const char *) memchr (s, '=', nLen);
		if (*s != '#' && pEqual != 0)
		{
			unsigned nKey = (unsigned) (pEqual - s);
			const char *pValue = pEqual + 1;
			unsigned nValue = (unsigned) (e - pValue);
			char Value[80];
			bool bQuoted;
			GetValue (pValue, nValue, Value, sizeof Value, &bQuoted);

			if (nKey == 4 && strncmp (s, "ssid", 4) == 0 && bQuoted)
			{
				CopyN (pNet->SSID, sizeof pNet->SSID, Value, (unsigned) strlen (Value));
				continue;
			}
			if (nKey == 3 && strncmp (s, "psk", 3) == 0)
			{
				pNet->bHexPSK = !bQuoted && IsHex64 (Value);
				if (bQuoted || pNet->bHexPSK)
				{
					CopyN (pNet->PSK, sizeof pNet->PSK, Value, (unsigned) strlen (Value));
					continue;
				}
			}
			if (nKey == 8 && strncmp (s, "key_mgmt", 8) == 0 && strcmp (Value, "NONE") == 0)
			{
				pNet->bOpen = true;
				continue;
			}
			if (nKey == 9 && strncmp (s, "scan_ssid", 9) == 0)
			{
				pNet->bHidden = ParseInt (Value) != 0;
				continue;
			}
			if (nKey == 8 && strncmp (s, "priority", 8) == 0)
			{
				pNet->nPriority = ParseInt (Value);
				continue;
			}
		}

		// lo demás de la red se conserva (con un tabulador delante)
		if (nLen > 0)
		{
			if (   !Append (pNet->Extra, sizeof pNet->Extra, &nExtra, "\t")
			    || !Append (pNet->Extra, sizeof pNet->Extra, &nExtra, s, nLen)
			    || !Append (pNet->Extra, sizeof pNet->Extra, &nExtra, "\n"))
			{
				return false;
			}
		}
	}

	if (pNet != 0)
	{
		return false;			// network={ sin cerrar
	}

	Sort ();
	return true;
}

unsigned CWiFiConfig::Format (char *pOut, unsigned nSize) const
{
	unsigned n = 0;
	if (nSize == 0)
	{
		return 0;
	}
	pOut[0] = '\0';

	// la cabecera (o la de un fichero nuevo), sin líneas en blanco al final
	const char *pHeader = m_Header[0] != '\0' ? m_Header : DefaultHeader;
	unsigned nHeader = (unsigned) strlen (pHeader);
	while (nHeader > 0 && (pHeader[nHeader - 1] == '\n' || IsSpace (pHeader[nHeader - 1])))
	{
		nHeader--;
	}
	bool bOK = true;
	if (nHeader > 0)
	{
		bOK = Append (pOut, nSize, &n, pHeader, nHeader) && Append (pOut, nSize, &n, "\n\n");
	}
	if (bOK && m_Country[0] != '\0')
	{
		bOK =    Append (pOut, nSize, &n, "country=") && Append (pOut, nSize, &n, m_Country)
		      && Append (pOut, nSize, &n, "\n\n");
	}

	for (unsigned i = 0; bOK && i < m_nCount; i++)
	{
		const TWiFiNetwork *pNet = &m_Networks[i];
		bOK =    Append (pOut, nSize, &n, "network={\n\tssid=\"")
		      && Append (pOut, nSize, &n, pNet->SSID) && Append (pOut, nSize, &n, "\"\n");
		if (bOK && pNet->bOpen)
		{
			bOK = Append (pOut, nSize, &n, "\tkey_mgmt=NONE\n");
		}
		else if (bOK && pNet->bHexPSK)
		{
			bOK =    Append (pOut, nSize, &n, "\tpsk=") && Append (pOut, nSize, &n, pNet->PSK)
			      && Append (pOut, nSize, &n, "\n");
		}
		else if (bOK)
		{
			bOK =    Append (pOut, nSize, &n, "\tpsk=\"") && Append (pOut, nSize, &n, pNet->PSK)
			      && Append (pOut, nSize, &n, "\"\n");
		}
		if (bOK && pNet->bHidden)
		{
			bOK = Append (pOut, nSize, &n, "\tscan_ssid=1\n");
		}
		if (bOK && pNet->nPriority != 0)
		{
			bOK =    Append (pOut, nSize, &n, "\tpriority=") && AppendNumber (pOut, nSize, &n, pNet->nPriority)
			      && Append (pOut, nSize, &n, "\n");
		}
		if (bOK)
		{
			bOK =    Append (pOut, nSize, &n, pNet->Extra)
			      && Append (pOut, nSize, &n, i + 1 < m_nCount ? "}\n\n" : "}\n");
		}
	}

	return bOK ? n : 0;
}

void CWiFiConfig::SetCountry (const char *pCountry)
{
	CopyN (m_Country, sizeof m_Country, pCountry, (unsigned) strlen (pCountry));
}

const TWiFiNetwork *CWiFiConfig::Get (unsigned nIndex) const
{
	return nIndex < m_nCount ? &m_Networks[nIndex] : 0;
}

int CWiFiConfig::Find (const char *pSSID) const
{
	for (unsigned i = 0; i < m_nCount; i++)
	{
		if (strcmp (m_Networks[i].SSID, pSSID) == 0)
		{
			return (int) i;
		}
	}
	return -1;
}

// Quita de las líneas de más las de "clave=" (por ejemplo, el key_mgmt o el
// proto de antes, al cambiar una red de abierta a con contraseña)
static void RemoveExtraKey (char *pExtra, const char *pKey)
{
	unsigned nKey = (unsigned) strlen (pKey);
	char *p = pExtra;
	while (*p != '\0')
	{
		char *pEnd = strchr (p, '\n');
		pEnd = pEnd != 0 ? pEnd + 1 : p + strlen (p);
		const char *s = p;
		while (IsSpace (*s)) s++;
		if (strncmp (s, pKey, nKey) == 0 && s[nKey] == '=')
		{
			memmove (p, pEnd, strlen (pEnd) + 1);
		}
		else
		{
			p = pEnd;
		}
	}
}

bool CWiFiConfig::Add (const char *pSSID, const char *pPSK, bool bHidden)
{
	bool bOpen = pPSK == 0 || *pPSK == '\0';
	if (!IsValidSSID (pSSID) || (!bOpen && !IsValidPSK (pPSK)))
	{
		return false;
	}

	int nIndex = Find (pSSID);
	TWiFiNetwork *pNet;
	if (nIndex >= 0)
	{
		pNet = &m_Networks[nIndex];
		RemoveExtraKey (pNet->Extra, "key_mgmt");
		RemoveExtraKey (pNet->Extra, "proto");
	}
	else
	{
		if (m_nCount == WIFI_MAX_NETWORKS)
		{
			return false;
		}
		pNet = &m_Networks[m_nCount++];
		memset (pNet, 0, sizeof *pNet);
		CopyN (pNet->SSID, sizeof pNet->SSID, pSSID, (unsigned) strlen (pSSID));
	}

	pNet->bOpen = bOpen;
	pNet->bHexPSK = false;
	CopyN (pNet->PSK, sizeof pNet->PSK, bOpen ? "" : pPSK, bOpen ? 0 : (unsigned) strlen (pPSK));
	pNet->bHidden = bHidden || pNet->bHidden;
	if (!bOpen)
	{
		unsigned nExtra = (unsigned) strlen (pNet->Extra);
		Append (pNet->Extra, sizeof pNet->Extra, &nExtra, "\tkey_mgmt=WPA-PSK\n");
	}

	return MakeFirst (pSSID);
}

bool CWiFiConfig::MakeFirst (const char *pSSID)
{
	int nIndex = Find (pSSID);
	if (nIndex < 0)
	{
		return false;
	}
	TWiFiNetwork Net = m_Networks[nIndex];
	memmove (&m_Networks[1], &m_Networks[0], nIndex * sizeof m_Networks[0]);
	m_Networks[0] = Net;
	Renumber ();
	return true;
}

bool CWiFiConfig::Remove (const char *pSSID)
{
	int nIndex = Find (pSSID);
	if (nIndex < 0)
	{
		return false;
	}
	memmove (&m_Networks[nIndex], &m_Networks[nIndex + 1], (m_nCount - nIndex - 1) * sizeof m_Networks[0]);
	m_nCount--;
	Renumber ();
	return true;
}

bool CWiFiConfig::IsValidPSK (const char *pPSK)
{
	unsigned n = 0;
	for (; pPSK[n] != '\0'; n++)
	{
		unsigned char c = (unsigned char) pPSK[n];
		if (c < 0x20 || c > 0x7E || c == '"')
		{
			return false;		// las comillas cerrarían el psk="..."
		}
	}
	return n >= WIFI_PSK_MIN && n <= WIFI_PSK_MAX;
}

bool CWiFiConfig::IsValidSSID (const char *pSSID)
{
	unsigned n = 0;
	for (; pSSID[n] != '\0'; n++)
	{
		unsigned char c = (unsigned char) pSSID[n];
		if (c < 0x20 || c == '"' || c == 0x7F)
		{
			return false;
		}
	}
	return n >= 1 && n <= WIFI_SSID_MAX;
}

void CWiFiConfig::Sort (void)
{
	// inserción estable: con la misma prioridad, en el orden del fichero
	for (unsigned i = 1; i < m_nCount; i++)
	{
		TWiFiNetwork Net = m_Networks[i];
		unsigned j = i;
		while (j > 0 && m_Networks[j - 1].nPriority < Net.nPriority)
		{
			m_Networks[j] = m_Networks[j - 1];
			j--;
		}
		m_Networks[j] = Net;
	}
}

void CWiFiConfig::Renumber (void)
{
	for (unsigned i = 0; i < m_nCount; i++)
	{
		m_Networks[i].nPriority = (int) (m_nCount - i);
	}
}
