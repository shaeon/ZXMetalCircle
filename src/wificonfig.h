//
// wificonfig.h - Las redes WiFi guardadas (SD:/wpa_supplicant.conf)
//
// Lee y escribe el fichero que usa wpa_supplicant: el país (country=) y una
// lista de redes, cada una con su nombre, su contraseña (en claro, para que se
// pueda comprobar si está bien escrita) y su prioridad. La red elegida la
// última es la de más prioridad: al arrancar, wpa_supplicant se conecta a la
// de más prioridad que esté al alcance.
//
// Lo que no se entiende se conserva tal cual: las líneas de más de cada red
// (por ejemplo, un "proto=" o un "bssid=" puestos a mano) y las de fuera de
// las redes. Así, un fichero escrito a mano no pierde nada al guardar desde
// la Pi; solo cambia lo que se ha tocado.
//
// Sin nada de Circle (solo cadenas): se prueba en el PC (test/host/wificonfig_test.cpp).
// Leer y escribir el fichero, con FatFs, lo hace CNetworkManager
// (LoadWiFiConfig / SaveWiFiConfig).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_wificonfig_h
#define _zx_wificonfig_h

#define WIFI_MAX_NETWORKS	8
#define WIFI_SSID_MAX		32		// bytes del nombre de una red
#define WIFI_PSK_MIN		8		// contraseña WPA: de 8 a 63 caracteres
#define WIFI_PSK_MAX		63
#define WIFI_EXTRA_MAX		256		// líneas de más de una red
#define WIFI_HEADER_MAX		2048		// líneas de fuera de las redes (comentarios...)
#define WIFI_FILE_MAX		(WIFI_HEADER_MAX + WIFI_MAX_NETWORKS * (WIFI_EXTRA_MAX + 256))

struct TWiFiNetwork
{
	char SSID[WIFI_SSID_MAX + 1];
	char PSK[64 + 1];		// en claro (8..63) o la clave de 64 cifras hexadecimales
	bool bHexPSK;			// psk=<64 hex>, sin comillas: no se puede mostrar
	bool bOpen;			// key_mgmt=NONE (sin contraseña)
	bool bHidden;			// scan_ssid=1 (red oculta)
	int nPriority;			// priority= (0 si no lo pone)
	char Extra[WIFI_EXTRA_MAX];	// líneas que no se tocan, cada una con su '\n'
};

class CWiFiConfig
{
public:
	CWiFiConfig (void);

	void Clear (void);

	// Lee el texto del fichero (terminado en '\0'). FALSE si hay algo que no
	// se entiende de forma segura (una red sin cerrar, demasiadas redes...):
	// entonces no se debe guardar encima.
	bool Parse (const char *pText);

	// Escribe el fichero completo en pOut; devuelve los bytes (0 si no cabe)
	unsigned Format (char *pOut, unsigned nSize) const;

	const char *GetCountry (void) const	{ return m_Country; }
	void SetCountry (const char *pCountry);	// dos letras ("ES"); "" = no se pone

	unsigned GetCount (void) const		{ return m_nCount; }
	const TWiFiNetwork *Get (unsigned nIndex) const;	// por prioridad, la primera la más alta
	int Find (const char *pSSID) const;	// índice o -1

	// Añade la red o, si ya está, le cambia la contraseña. Pasa a ser la de
	// más prioridad. pPSK = 0 o "" para una red abierta. FALSE si la
	// contraseña no vale o ya hay WIFI_MAX_NETWORKS redes.
	bool Add (const char *pSSID, const char *pPSK, bool bHidden);

	// La red pasa a ser la de más prioridad (para "Conectar" a una guardada)
	bool MakeFirst (const char *pSSID);

	bool Remove (const char *pSSID);

	// ¿Vale como contraseña WPA? 8 a 63 caracteres ASCII imprimibles
	static bool IsValidPSK (const char *pPSK);
	// ¿Vale como nombre de red? 1 a 32 bytes, sin comillas ni saltos de línea
	static bool IsValidSSID (const char *pSSID);

private:
	void Sort (void);			// por prioridad, de mayor a menor
	void Renumber (void);			// prioridades N..1 según el orden

	char m_Country[3];
	char m_Header[WIFI_HEADER_MAX];		// lo de fuera de las redes, menos country=
	unsigned m_nCount;
	TWiFiNetwork m_Networks[WIFI_MAX_NETWORKS];
};

#endif
