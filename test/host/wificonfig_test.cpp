//
// wificonfig_test.cpp - Lectura y escritura de wpa_supplicant.conf (CWiFiConfig)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "wificonfig.h"
#include <stdio.h>
#include <string.h>

static unsigned s_nErrors = 0;

static void Check (const char *pName, bool bOK)
{
	printf ("%s %s\n", bOK ? "ok   " : "FALLO", pName);
	if (!bOK) s_nErrors++;
}

static bool Contains (const char *pText, const char *pPart)
{
	return strstr (pText, pPart) != 0;
}

// El ejemplo que va en la SD (boot/wpa_supplicant.conf.ejemplo), ya rellenado
static const char Sample[] =
	"#\n"
	"# Configuración de la WiFi de ZXMetalCircle\n"
	"#\n"
	"\n"
	"country=ES\n"
	"\n"
	"network={\n"
	"\tssid=\"MiCasa\"\n"
	"\tpsk=\"clave-de-casa\"\n"
	"\tproto=WPA2\n"
	"\tkey_mgmt=WPA-PSK\n"
	"\t#scan_ssid=1\t\t# quita la almohadilla si tu red está oculta\n"
	"}\n";

static char s_Out[WIFI_FILE_MAX + 1];

int main (void)
{
	static CWiFiConfig Config;

	// 1. el ejemplo: una red, su país y lo demás conservado
	Check ("lee el ejemplo", Config.Parse (Sample));
	Check ("país ES", strcmp (Config.GetCountry (), "ES") == 0);
	Check ("una red", Config.GetCount () == 1);
	const TWiFiNetwork *pNet = Config.Get (0);
	Check ("nombre y contraseña en claro",
	       pNet != 0 && strcmp (pNet->SSID, "MiCasa") == 0 && strcmp (pNet->PSK, "clave-de-casa") == 0
	       && !pNet->bOpen && !pNet->bHexPSK && !pNet->bHidden);

	unsigned n = Config.Format (s_Out, sizeof s_Out);
	Check ("se escribe", n > 0 && n == strlen (s_Out));
	Check ("conserva la cabecera", Contains (s_Out, "# Configuración de la WiFi de ZXMetalCircle\n"));
	Check ("conserva proto y el comentario de la red",
	       Contains (s_Out, "\tproto=WPA2\n") && Contains (s_Out, "\t#scan_ssid=1\t\t# quita"));
	Check ("country= una sola vez",
	       Contains (s_Out, "country=ES\n") && strstr (strstr (s_Out, "country=") + 1, "country=") == 0);

	// releer lo escrito da lo mismo
	static CWiFiConfig Again;
	Check ("lo escrito se vuelve a leer igual",
	       Again.Parse (s_Out) && Again.GetCount () == 1 && strcmp (Again.Get (0)->PSK, "clave-de-casa") == 0);
	static char Out2[WIFI_FILE_MAX + 1];
	Again.Format (Out2, sizeof Out2);
	Check ("y se escribe igual (estable)", strcmp (s_Out, Out2) == 0);

	// 2. añadir redes: la última, la primera (más prioridad)
	Check ("añade Taller", Config.Add ("Taller", "otra clave 123", false));
	Check ("añade una abierta", Config.Add ("Biblioteca Libre", "", false));
	Check ("tres redes", Config.GetCount () == 3);
	Check ("la última añadida va primero",
	       strcmp (Config.Get (0)->SSID, "Biblioteca Libre") == 0 && strcmp (Config.Get (1)->SSID, "Taller") == 0
	       && strcmp (Config.Get (2)->SSID, "MiCasa") == 0);
	Check ("prioridades 3, 2, 1",
	       Config.Get (0)->nPriority == 3 && Config.Get (1)->nPriority == 2 && Config.Get (2)->nPriority == 1);
	Config.Format (s_Out, sizeof s_Out);
	Check ("la abierta lleva key_mgmt=NONE y sin psk",
	       Contains (s_Out, "ssid=\"Biblioteca Libre\"\n\tkey_mgmt=NONE\n\tpriority=3\n"));
	Check ("la nueva con contraseña lleva key_mgmt=WPA-PSK",
	       Contains (s_Out, "ssid=\"Taller\"\n\tpsk=\"otra clave 123\"\n\tpriority=2\n\tkey_mgmt=WPA-PSK\n"));

	// 3. conectar a una guardada: pasa a ser la primera
	Check ("MakeFirst MiCasa", Config.MakeFirst ("MiCasa") && strcmp (Config.Get (0)->SSID, "MiCasa") == 0
	       && Config.Get (0)->nPriority == 3);

	// 4. cambiar la contraseña de una guardada: no se duplica
	Check ("cambia la contraseña de Taller", Config.Add ("Taller", "clave nueva!", false));
	Check ("sigue habiendo tres", Config.GetCount () == 3);
	Check ("con la contraseña nueva y primera",
	       strcmp (Config.Get (0)->SSID, "Taller") == 0 && strcmp (Config.Get (0)->PSK, "clave nueva!") == 0);
	Config.Format (s_Out, sizeof s_Out);
	const char *pTaller = strstr (s_Out, "ssid=\"Taller\"");
	unsigned nKeyMgmt = 0;
	for (const char *p = pTaller; p != 0 && *p != '}'; p++)
	{
		nKeyMgmt += strncmp (p, "key_mgmt", 8) == 0;
	}
	Check ("un solo key_mgmt en Taller", pTaller != 0 && nKeyMgmt == 1);

	// 5. una abierta pasa a tener contraseña: fuera el key_mgmt=NONE
	Check ("la abierta pasa a tener contraseña", Config.Add ("Biblioteca Libre", "ahora con clave", false)
	       && !Config.Get (0)->bOpen);
	Config.Format (s_Out, sizeof s_Out);
	Check ("sin key_mgmt=NONE", !Contains (s_Out, "key_mgmt=NONE"));

	// 6. olvidar una red
	Check ("olvida MiCasa", Config.Remove ("MiCasa") && Config.GetCount () == 2 && Config.Find ("MiCasa") < 0);
	Check ("prioridades 2, 1", Config.Get (0)->nPriority == 2 && Config.Get (1)->nPriority == 1);
	Check ("olvidar una que no está falla", !Config.Remove ("NoEsta"));

	// 7. red oculta
	Check ("añade una oculta", Config.Add ("Oculta", "12345678", true) && Config.Get (0)->bHidden);
	Config.Format (s_Out, sizeof s_Out);
	Check ("con scan_ssid=1", Contains (s_Out, "ssid=\"Oculta\"\n\tpsk=\"12345678\"\n\tscan_ssid=1\n"));

	// 8. contraseñas y nombres que no valen
	Check ("contraseña de 7: no", !CWiFiConfig::IsValidPSK ("1234567"));
	Check ("contraseña de 8: sí", CWiFiConfig::IsValidPSK ("12345678"));
	char Long[70];
	memset (Long, 'a', 63); Long[63] = '\0';
	Check ("contraseña de 63: sí", CWiFiConfig::IsValidPSK (Long));
	Long[63] = 'a'; Long[64] = '\0';
	Check ("contraseña de 64: no", !CWiFiConfig::IsValidPSK (Long));
	Check ("contraseña con comillas: no", !CWiFiConfig::IsValidPSK ("abc\"defgh"));
	Check ("contraseña con ñ: no (no es ASCII)", !CWiFiConfig::IsValidPSK ("contraseña1"));
	Check ("Add con contraseña corta falla", !Config.Add ("Nueva", "corta", false) && Config.Find ("Nueva") < 0);
	Check ("nombre vacío: no", !CWiFiConfig::IsValidSSID (""));
	Check ("nombre de 33: no", !CWiFiConfig::IsValidSSID ("123456789012345678901234567890123"));
	Check ("nombre con espacios y acentos: sí", CWiFiConfig::IsValidSSID ("Casa de Ana ñ"));

	// 9. sin fichero: cabecera nueva, país y la red
	static CWiFiConfig New;
	New.SetCountry ("PT");
	Check ("nueva red en un fichero nuevo", New.Add ("Rede", "palavra-passe", false));
	New.Format (s_Out, sizeof s_Out);
	Check ("fichero nuevo con cabecera, país y red",
	       Contains (s_Out, "# Configuración de la WiFi de ZXMetalCircle") && Contains (s_Out, "country=PT\n")
	       && Contains (s_Out, "ssid=\"Rede\"") && Contains (s_Out, "priority=1"));

	// 10. psk de 64 cifras hexadecimales: se conserva tal cual
	static const char Hex[] =
		"country=ES\nnetwork={\n\tssid=\"Hex\"\n"
		"\tpsk=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n}\n";
	static CWiFiConfig HexConfig;
	Check ("psk hexadecimal", HexConfig.Parse (Hex) && HexConfig.Get (0)->bHexPSK);
	HexConfig.Format (s_Out, sizeof s_Out);
	Check ("se escribe sin comillas",
	       Contains (s_Out, "\tpsk=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n"));

	// 11. prioridades de un fichero a mano: se ordenan, y con la misma, en su orden
	static const char Manual[] =
		"network={\n\tssid=\"A\"\n\tpsk=\"aaaaaaaa\"\n}\n"
		"network={\n\tssid=\"B\"\n\tpsk=\"bbbbbbbb\"\n\tpriority=5\n}\n"
		"network={\n\tssid=\"C\"\n\tpsk=\"cccccccc\"\n}\n";
	static CWiFiConfig ManualConfig;
	Check ("orden por prioridad",
	       ManualConfig.Parse (Manual) && strcmp (ManualConfig.Get (0)->SSID, "B") == 0
	       && strcmp (ManualConfig.Get (1)->SSID, "A") == 0 && strcmp (ManualConfig.Get (2)->SSID, "C") == 0);
	ManualConfig.Format (s_Out, sizeof s_Out);
	Check ("sin priority= en las que no lo tenían",
	       Contains (s_Out, "ssid=\"A\"\n\tpsk=\"aaaaaaaa\"\n}") && Contains (s_Out, "priority=5"));

	// 12. ficheros que no se entienden: no se debe guardar encima
	static CWiFiConfig Bad;
	Check ("red sin cerrar", !Bad.Parse ("network={\n\tssid=\"X\"\n\tpsk=\"12345678\"\n"));
	Check ("red sin nombre", !Bad.Parse ("network={\n\tpsk=\"12345678\"\n}\n"));
	static char Many[4096];
	unsigned nMany = 0;
	for (unsigned i = 0; i <= WIFI_MAX_NETWORKS; i++)
	{
		nMany += (unsigned) snprintf (Many + nMany, sizeof Many - nMany,
					      "network={\n\tssid=\"R%u\"\n\tpsk=\"12345678\"\n}\n", i);
	}
	Check ("demasiadas redes", !Bad.Parse (Many));

	// 13. llena: no se añade una más
	static CWiFiConfig Full;
	bool bAllAdded = true;
	for (unsigned i = 0; i < WIFI_MAX_NETWORKS; i++)
	{
		char Name[8] = "Red0";
		Name[3] = (char) ('0' + i);
		bAllAdded = Full.Add (Name, "12345678", false) && bAllAdded;
	}
	Check ("caben WIFI_MAX_NETWORKS", bAllAdded && Full.GetCount () == WIFI_MAX_NETWORKS);
	Check ("una más no", !Full.Add ("Sobra", "12345678", false));
	Check ("pero cambiar una guardada sí", Full.Add ("Red3", "87654321", false));

	if (s_nErrors == 0)
	{
		printf ("wificonfig_test: todo OK\n");
	}
	return s_nErrors == 0 ? 0 : 1;
}
