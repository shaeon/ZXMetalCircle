//
// kernelwifi.cpp - Elegir la red WiFi desde el emulador (Configuración,
//                  página 7 "Red y hora": "Red WiFi" y "País WiFi")
//
// Las redes se guardan en SD:/wpa_supplicant.conf (CWiFiConfig, con la
// contraseña en claro para que se pueda ver si está bien escrita). Nada es
// automático: el usuario abre la lista, elige o escribe una red, pone su
// contraseña y decide si reinicia ya; wpa_supplicant solo lee el fichero al
// arrancar, así que la red nueva se usa al reiniciar.
//
// Todo se hace con listas de elección (teclas 1-9) y la línea de texto, que
// también funcionan con el teclado GPIO.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include "i18n.h"
#include "wificonfig.h"
#include <circle/util.h>

// La red a la que está conectada ahora (tools/circle/ether4330_zxmc.c)
extern "C" int ether4330_get_essid (char *pBuffer, int nSize);

// Países para country= (el controlador de la WiFi lo exige para conectar: dice
// qué canales se pueden usar)
static const struct
{
	const char *pCode;
	const char *pName;
}
Countries[] =
{
	{"DE", N_("Alemania")},
	{"AR", N_("Argentina")},
	{"AU", N_("Australia")},
	{"AT", N_("Austria")},
	{"BE", N_("Bélgica")},
	{"BR", N_("Brasil")},
	{"CA", N_("Canadá")},
	{"CL", N_("Chile")},
	{"CO", N_("Colombia")},
	{"DK", N_("Dinamarca")},
	{"ES", N_("España")},
	{"US", N_("Estados Unidos")},
	{"FI", N_("Finlandia")},
	{"FR", N_("Francia")},
	{"IE", N_("Irlanda")},
	{"IT", N_("Italia")},
	{"JP", N_("Japón")},
	{"MX", N_("México")},
	{"NO", N_("Noruega")},
	{"NL", N_("Países Bajos")},
	{"PE", N_("Perú")},
	{"PL", N_("Polonia")},
	{"PT", N_("Portugal")},
	{"GB", N_("Reino Unido")},
	{"SE", N_("Suecia")},
	{"CH", N_("Suiza")},
	{"UY", N_("Uruguay")},
	{"VE", N_("Venezuela")}
};
#define NUM_COUNTRIES	(sizeof Countries / sizeof Countries[0])

static const char *CountryName (const char *pCode)
{
	for (unsigned i = 0; i < NUM_COUNTRIES; i++)
	{
		if (strcmp (Countries[i].pCode, pCode) == 0)
		{
			return T (Countries[i].pName);
		}
	}
	return pCode;				// uno puesto a mano que no está en la lista
}

// El país si no se ha elegido ninguno: el del idioma
static const char *DefaultCountry (void)
{
	CI18n *pI18n = CI18n::Get ();
	const char *pLang = pI18n != 0 ? pI18n->GetLanguage () : "es";
	if (strcmp (pLang, "pt-BR") == 0)	return "BR";
	if (strcmp (pLang, "pt") == 0)		return "PT";
	if (strcmp (pLang, "en") == 0)		return "GB";
	return "ES";
}

// Ancho en pantalla de un texto UTF-8
static unsigned Columns (const char *p)
{
	unsigned n = 0;
	for (; *p != '\0'; p++)
	{
		n += (*p & 0xC0) != 0x80;
	}
	return n;
}

// Lee las redes guardadas en m_pWiFiConfig; FALSE (y lo dice) si el fichero no
// se entiende: entonces no se toca
boolean CKernel::WiFiLoad (void)
{
	if (m_pWiFiConfig == 0)
	{
		m_pWiFiConfig = new CWiFiConfig;
		if (m_pWiFiConfig == 0)
		{
			return FALSE;
		}
	}
	if (!CNetworkManager::LoadWiFiConfig (m_pWiFiConfig))
	{
		ShowMessage (T ("wpa_supplicant.conf no se entiende"));
		return FALSE;
	}
	return TRUE;
}

// Lo que se ve a la derecha de "Red WiFi" y "País WiFi"
void CKernel::WiFiRows (void)
{
	if (!CNetworkManager::ModelHasWiFi ())
	{
		m_Settings.SetWiFiPresent (FALSE);
		m_Settings.SetActionText ("_wifinet", T ("sin WiFi"));
		m_Settings.SetActionText ("_wificountry", "-");
		return;
	}

	CWiFiConfig *pConfig = new CWiFiConfig;
	if (pConfig == 0)
	{
		return;
	}
	if (!CNetworkManager::LoadWiFiConfig (pConfig))
	{
		m_Settings.SetActionText ("_wifinet", T ("fichero con errores"));
		m_Settings.SetActionText ("_wificountry", "-");
		delete pConfig;
		return;
	}

	char Connected[WIFI_SSID_MAX + 1];
	if (   m_Network.IsWiFi () && CNetworkManager::IsNetRunning ()
	    && ether4330_get_essid (Connected, sizeof Connected))
	{
		m_Settings.SetActionText ("_wifinet", Connected);
	}
	else if (pConfig->GetCount () > 0)
	{
		m_Settings.SetActionText ("_wifinet", pConfig->Get (0)->SSID);
	}
	else
	{
		m_Settings.SetActionText ("_wifinet", T ("ninguna"));
	}

	CString Country;
	if (pConfig->GetCountry ()[0] != '\0')
	{
		Country = CountryName (pConfig->GetCountry ());
	}
	else if (m_WiFiCountry[0] != '\0')
	{
		Country = CountryName (m_WiFiCountry);
	}
	else
	{
		Country = CountryName (DefaultCountry ());	// el del idioma
	}
	m_Settings.SetActionText ("_wificountry", Country);
	delete pConfig;
}

// ENTER en "Red WiFi" o "País WiFi"
void CKernel::WiFiAction (const char *pKey)
{
	if (!CNetworkManager::ModelHasWiFi ())
	{
		ShowMessage (T ("Esta Raspberry Pi no tiene WiFi"));
		return;
	}
	if (!WiFiLoad ())
	{
		return;
	}

	if (strcmp (pKey, "_wifinet") == 0)
	{
		WiFiOpenList ();
		return;
	}

	// País: la lista, con el de ahora elegido
	const char *pCurrent = m_pWiFiConfig->GetCountry ()[0] != '\0' ? m_pWiFiConfig->GetCountry ()
			     : m_WiFiCountry[0] != '\0' ? m_WiFiCountry : DefaultCountry ();
	m_WiFiStep = WiFiStepCountry;
	m_Menu.OpenChoice (T ("País de la WiFi"));
	m_Menu.SetChoiceNote (T ("Dice qué canales se pueden usar"));
	for (unsigned i = 0; i < NUM_COUNTRIES; i++)
	{
		m_Menu.SetChoiceItem (i, T (Countries[i].pName));
		if (strcmp (Countries[i].pCode, pCurrent) == 0)
		{
			m_Menu.SetChoiceSelected (i);
		}
	}
}

// La lista de redes: las guardadas (la conectada con *) y "Otra red"
void CKernel::WiFiOpenList (void)
{
	char Connected[WIFI_SSID_MAX + 1] = "";
	if (!(   m_Network.IsWiFi () && CNetworkManager::IsNetRunning ()
	      && ether4330_get_essid (Connected, sizeof Connected)))
	{
		Connected[0] = '\0';
	}

	m_WiFiStep = WiFiStepList;
	m_Menu.OpenChoice (T ("Redes WiFi"));
	unsigned nCount = m_pWiFiConfig->GetCount ();
	boolean bAnyConnected = FALSE;
	for (unsigned i = 0; i < nCount; i++)
	{
		const TWiFiNetwork *pNet = m_pWiFiConfig->Get (i);
		boolean bConnected = strcmp (pNet->SSID, Connected) == 0;
		bAnyConnected = bAnyConnected || bConnected;

		// "*Nombre                    abierta", con el nombre a 26 columnas
		CString Item (bConnected ? "*" : " ");
		Item.Append (pNet->SSID);
		for (unsigned n = Columns (pNet->SSID); n < 26; n++)
		{
			Item.Append (" ");
		}
		Item.Append (" ");
		Item.Append (pNet->bOpen ? T ("abierta") : T ("clave"));
		m_Menu.SetChoiceItem (i, Item);
	}
	m_Menu.SetChoiceItem (nCount, T (" Otra red (escribir su nombre)..."));
	m_Menu.SetChoiceNote (  bAnyConnected ? T ("* conectada ahora")
			      : nCount == 0 ? T ("No hay ninguna red guardada")
					    : T ("Guardadas: se usa la primera que haya"));
}

// La contraseña de m_WiFiSSID (pInitial: lo que ya hay escrito)
void CKernel::WiFiAskPassword (const char *pInitial, const char *pError)
{
	CString Title;
	Title.Format (T ("Contraseña de %s"), m_WiFiSSID);
	m_WiFiStep = WiFiStepPassword;
	m_Menu.OpenText (Title, pError != 0 ? pError : T ("De 8 a 63 caracteres (vacía: abierta)"),
			 T ("CAPS SHIFT mayúsc.  SYMBOL SHIFT símb."),
			 T ("ENTER guardar  TAB ocultar  ESC cancelar"),
			 pInitial, strlen (pInitial), WIFI_PSK_MAX);
	m_Menu.SetTextPassword ();
}

void CKernel::WiFiAskName (const char *pInitial, const char *pError)
{
	m_WiFiStep = WiFiStepName;
	m_Menu.OpenText (T ("Nombre de la red"),
			 pError != 0 ? pError : T ("Tal cual: las mayúsculas cuentan"),
			 T ("CAPS SHIFT mayúsc.  SYMBOL SHIFT símb."),
			 T ("ENTER seguir   ESC cancelar"),
			 pInitial, strlen (pInitial), WIFI_SSID_MAX);
	m_Menu.SetTextPassword ();		// también valen * ? / : < > en un nombre
}

// Guardada (añadida, cambiada o puesta la primera): al fichero y la pregunta
// de reiniciar. Si la red está en "cable" o "no", pasa a "automática" (si no,
// la WiFi no se usaría) y se guarda con lo demás al reiniciar.
void CKernel::WiFiSaveAndAsk (void)
{
	if (m_pWiFiConfig->GetCountry ()[0] == '\0')
	{
		m_pWiFiConfig->SetCountry (m_WiFiCountry[0] != '\0' ? m_WiFiCountry : DefaultCountry ());
	}
	if (!CNetworkManager::SaveWiFiConfig (*m_pWiFiConfig))
	{
		m_WiFiStep = WiFiStepNone;
		m_Menu.OpenSettings ();
		ShowMessage (T ("No se pudo guardar wpa_supplicant.conf"));
		return;
	}
	WiFiRows ();

	boolean bNetChanged = FALSE;
	int nNet = m_Settings.Find ("net");
	if (nNet >= 0 && (strcmp (m_Settings.GetValue (nNet), "eth") == 0 || strcmp (m_Settings.GetValue (nNet), "0") == 0))
	{
		m_Settings.SetValue ("net", "auto");	// (con la clave: queda como cambio por guardar)
		bNetChanged = TRUE;
	}

	m_WiFiStep = WiFiStepReboot;
	m_Menu.OpenChoice (T ("Guardada. Para conectar, reiniciar"));
	m_Menu.SetChoiceNote (  bNetChanged ? T ("«Red» pasa a automática y se guarda")
			      : m_Settings.IsDirty () ? T ("Se guardan también los otros cambios")
						      : T ("Se perderá lo que haya en el Spectrum"));
	m_Menu.SetChoiceItem (0, T ("Reiniciar ahora"));
	m_Menu.SetChoiceItem (1, T ("Más tarde"));
}

// Las respuestas de las pantallas de la WiFi; TRUE si eran suyas
boolean CKernel::WiFiMenuResult (TMenuResult Result)
{
	if (Result == MenuResultSettingAction)
	{
		const char *pKey = m_Settings.GetDef (m_Menu.GetChangedSetting ()).pKey;
		if (strcmp (pKey, "_wifinet") != 0 && strcmp (pKey, "_wificountry") != 0)
		{
			return FALSE;
		}
		WiFiAction (pKey);
		return TRUE;
	}

	if (m_WiFiStep == WiFiStepNone)
	{
		return FALSE;
	}
	boolean bChoice = Result == MenuResultChoice;
	boolean bInput = Result == MenuResultInput;
	if (   !bChoice && Result != MenuResultChoiceCancel
	    && !bInput && Result != MenuResultInputCancel)
	{
		return FALSE;
	}
	unsigned nChoice = m_Menu.GetChoice ();
	TWiFiStep Step = m_WiFiStep;
	m_WiFiStep = WiFiStepNone;

	switch (Step)
	{
	case WiFiStepList:
		if (!bChoice)
		{
			m_Menu.OpenSettings ();
		}
		else if (nChoice < m_pWiFiConfig->GetCount ())
		{
			// una guardada: conectar, cambiar la contraseña u olvidarla
			const TWiFiNetwork *pNet = m_pWiFiConfig->Get (nChoice);
			strcpy (m_WiFiSSID, pNet->SSID);
			CString Title;
			Title.Format (T ("%s (guardada)"), pNet->SSID);
			CString Note;
			if (pNet->bOpen)
			{
				Note = T ("Red abierta, sin contraseña");
			}
			else if (pNet->bHexPSK)
			{
				Note = T ("Contraseña en hexadecimal (64 cifras)");
			}
			else
			{
				Note.Format (T ("Contraseña: %s"), pNet->PSK);
			}
			m_WiFiStep = WiFiStepSaved;
			m_Menu.OpenChoice (Title);
			m_Menu.SetChoiceNote (Note);
			m_Menu.SetChoiceItem (0, T ("Conectar"));
			m_Menu.SetChoiceItem (1, pNet->bOpen ? T ("Ponerle contraseña") : T ("Cambiar la contraseña"));
			m_Menu.SetChoiceItem (2, T ("Olvidar esta red"));
		}
		else
		{
			m_bWiFiHidden = TRUE;		// escrita a mano: puede estar oculta
			WiFiAskName ("", 0);
		}
		break;

	case WiFiStepSaved:
		if (!bChoice)
		{
			WiFiOpenList ();
		}
		else if (nChoice == 0)			// conectar: pasa a ser la primera
		{
			m_pWiFiConfig->MakeFirst (m_WiFiSSID);
			WiFiSaveAndAsk ();
		}
		else if (nChoice == 1)			// la contraseña, con la que tenía
		{
			int n = m_pWiFiConfig->Find (m_WiFiSSID);
			const TWiFiNetwork *pNet = n >= 0 ? m_pWiFiConfig->Get (n) : 0;
			m_bWiFiHidden = pNet != 0 && pNet->bHidden;
			WiFiAskPassword (pNet != 0 && !pNet->bHexPSK && !pNet->bOpen ? pNet->PSK : "", 0);
		}
		else					// olvidarla
		{
			m_pWiFiConfig->Remove (m_WiFiSSID);
			if (CNetworkManager::SaveWiFiConfig (*m_pWiFiConfig))
			{
				CString Message;
				Message.Format (T ("Olvidada: %s"), m_WiFiSSID);
				ShowMessage (Message);
				WiFiRows ();
			}
			else
			{
				ShowMessage (T ("No se pudo guardar wpa_supplicant.conf"));
			}
			WiFiOpenList ();
		}
		break;

	case WiFiStepName:
		if (!bInput)
		{
			WiFiOpenList ();
		}
		else if (!CWiFiConfig::IsValidSSID (m_Menu.GetInput ()))
		{
			CString Typed (m_Menu.GetInput ());
			WiFiAskName (Typed, T ("Hace falta el nombre (hasta 32)"));
		}
		else
		{
			strcpy (m_WiFiSSID, m_Menu.GetInput ());
			int n = m_pWiFiConfig->Find (m_WiFiSSID);
			const TWiFiNetwork *pNet = n >= 0 ? m_pWiFiConfig->Get (n) : 0;
			WiFiAskPassword (pNet != 0 && !pNet->bHexPSK ? pNet->PSK : "", 0);
		}
		break;

	case WiFiStepPassword:
		if (!bInput)
		{
			WiFiOpenList ();
		}
		else
		{
			CString PSK (m_Menu.GetInput ());
			if (PSK.GetLength () > 0 && !CWiFiConfig::IsValidPSK (PSK))
			{
				WiFiAskPassword (PSK, T ("No vale: de 8 a 63, sin comillas"));
			}
			else if (!m_pWiFiConfig->Add (m_WiFiSSID, PSK, m_bWiFiHidden))
			{
				ShowMessage (T ("Ya hay 8 redes: olvida alguna antes"));
				WiFiOpenList ();
			}
			else
			{
				WiFiSaveAndAsk ();
			}
		}
		break;

	case WiFiStepReboot:
		if (bChoice && nChoice == 0)
		{
			RebootPi (m_Settings.IsDirty ());
		}
		else
		{
			m_Menu.OpenSettings ();
			ShowMessage (T ("La WiFi se conectará al reiniciar"));
		}
		break;

	case WiFiStepCountry:
		m_Menu.OpenSettings ();
		if (bChoice && nChoice < NUM_COUNTRIES)
		{
			strcpy (m_WiFiCountry, Countries[nChoice].pCode);
			if (m_pWiFiConfig->GetCount () == 0)
			{
				// sin redes no se crea el fichero (con él, la red
				// automática iría por WiFi): se guarda con la primera
				ShowMessage (T ("País elegido: se guarda con la red"));
			}
			else
			{
				m_pWiFiConfig->SetCountry (m_WiFiCountry);
				ShowMessage (CNetworkManager::SaveWiFiConfig (*m_pWiFiConfig)
					     ? T ("País de la WiFi: al reiniciar")
					     : T ("No se pudo guardar wpa_supplicant.conf"));
			}
			WiFiRows ();
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}
