//
// network.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "network.h"
#include "i18n.h"
#include "webserver.h"
#include "version.h"
#include <wlan/bcm4343.h>
#include <wlan/hostap/wpa_supplicant/wpasupplicant.h>
#include <circle/net/ntpdaemon.h>
#include <circle/net/mdnspublisher.h>
#include <circle/machineinfo.h>
#include <circle/timer.h>
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <fatfs/ff.h>

#define FIRMWARE_PATH		"SD:/firmware/"
#define WPA_CONFIG_FILE		"SD:/wpa_supplicant.conf"
#define NTP_SERVER		"pool.ntp.org"


#define WIFI_SLOW_SECONDS	120		// sin conectar en este tiempo: se avisa

static const char FromNet[] = "net";

// Lo que build.sh añade a wpa_supplicant (tools/circle/wpa_zxmc_*): en qué
// paso está la conexión y si la red ha rechazado la contraseña
int zxmc_wpa_get_state (char *pSSID, int nSize);
int zxmc_wpa_scan_start (void);
int zxmc_wpa_scanning (void);
int zxmc_wpa_get_bss (int nIndex, char *pSSID, int nSize, int *pLevel, int *pFreq, int *pOpen);
extern "C" int zxmc_wpa_psk_mismatches;
extern "C" char zxmc_wpa_psk_ssid[33];

CNetworkManager *CNetworkManager::s_pThis = 0;

////////////////////////////////////////////////////////////////////////////////
// Gestor de red

CNetworkManager::CNetworkManager (void)
:	m_State (NetStateOff),
	m_bWiFi (FALSE),
	m_bServicesStarted (FALSE),
	m_nTimeZone (60),
	m_SummerTime (SummerTimeEU),
	m_nLastDSTCheck (0),
	m_pWLAN (0),
	m_pNet (0),
	m_pSupplicant (0),
	m_pmDNS (0),
	m_bLoadPending (FALSE),
	m_bTLSVerify (TRUE),
	m_nWebMode (1),
	m_pBusyHandler (0),
	m_pBusyParam (0),
	m_bNoFirmware (FALSE),
	m_nWiFiNetworks (0),
	m_nConnectStart (0),
	m_bSlowWarned (FALSE),
	m_nPSKWarned (0),
	m_bWarningPending (FALSE)
{
	m_WiFiFirst[0] = '\0';
	m_WebPassword[0] = '\0';
	m_bWebProtect = true;
	s_pThis = this;
	m_IPAddress[0] = '\0';
}

boolean CNetworkManager::ModelHasWiFi (void)
{
	switch (CMachineInfo::Get ()->GetMachineModel ())
	{
	case MachineModelZeroW:
	case MachineModelZero2W:
	case MachineModel3B:
	case MachineModel3APlus:
	case MachineModel3BPlus:
	case MachineModel4B:
	case MachineModel400:
	case MachineModelCM4:
		return TRUE;

#if RASPPI >= 5
	// Pi 5: en pruebas. Circle avisa de que la WLAN es "delicada" en la Pi 5
	// con una pantalla HDMI a la vez (doc/issues.txt). Usa el firmware
	// brcmfmac43455-sdio.raspberrypi,5-model-b.* (build.sh lo descarga).
	// Algunas CM5 no llevan WiFi: entonces el chip no arranca y se avisa.
	case MachineModel5:
	case MachineModel500:
	case MachineModelCM5:
	case MachineModelCM5Lite:
		return TRUE;
#endif

	default:
		return FALSE;
	}
}

static boolean FileExists (const char *pPath)
{
	FILINFO Info;
	return f_stat (pPath, &Info) == FR_OK;
}

#define WPA_CONFIG_TEMP		"SD:/wpa_supplicant.tmp"
#define WPA_CONFIG_BACKUP	"SD:/wpa_supplicant.conf.bak"

boolean CNetworkManager::LoadWiFiConfig (CWiFiConfig *pConfig)
{
	pConfig->Clear ();
	FIL File;
	if (f_open (&File, WPA_CONFIG_FILE, FA_READ) != FR_OK)
	{
		return TRUE;			// sin fichero: ninguna red
	}
	char *pText = new char[WIFI_FILE_MAX + 1];
	UINT nRead = 0;
	boolean bOK =    pText != 0
		      && f_size (&File) <= WIFI_FILE_MAX
		      && f_read (&File, pText, WIFI_FILE_MAX, &nRead) == FR_OK;
	f_close (&File);
	if (bOK)
	{
		pText[nRead] = '\0';
		bOK = pConfig->Parse (pText);
	}
	if (!bOK)
	{
		CLogger::Get ()->Write (FromNet, LogWarning, "%s: no se entiende, no se toca", WPA_CONFIG_FILE);
	}
	delete [] pText;
	return bOK;
}

boolean CNetworkManager::SaveWiFiConfig (const CWiFiConfig &Config)
{
	char *pText = new char[WIFI_FILE_MAX + 1];
	if (pText == 0)
	{
		return FALSE;
	}
	unsigned nLength = Config.Format (pText, WIFI_FILE_MAX + 1);
	FIL File;
	UINT nWritten = 0;
	boolean bOK =    nLength != 0
		      && f_open (&File, WPA_CONFIG_TEMP, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK;
	if (bOK)
	{
		bOK = f_write (&File, pText, nLength, &nWritten) == FR_OK && nWritten == nLength;
		bOK = f_close (&File) == FR_OK && bOK;
	}
	delete [] pText;

	// el temporal ya está entero: el de antes pasa a .bak y el nuevo a su sitio
	if (bOK)
	{
		if (FileExists (WPA_CONFIG_FILE))
		{
			f_unlink (WPA_CONFIG_BACKUP);
			bOK = f_rename (WPA_CONFIG_FILE, WPA_CONFIG_BACKUP) == FR_OK;
		}
		bOK = bOK && f_rename (WPA_CONFIG_TEMP, WPA_CONFIG_FILE) == FR_OK;
	}
	if (!bOK)
	{
		f_unlink (WPA_CONFIG_TEMP);
		CLogger::Get ()->Write (FromNet, LogError, "No se pudo guardar %s", WPA_CONFIG_FILE);
	}
	return bOK;
}

void CNetworkManager::Start (TNetMode Mode, const char *pHostname, int nTimeZoneMinutes,
			     TSummerTime SummerTime, const u8 *pIP, const u8 *pMask,
			     const u8 *pGateway, const u8 *pDNS)
{
	if (pIP != 0)
	{
		memcpy (m_StaticIP, pIP, 4);
		memcpy (m_StaticMask, pMask, 4);
		memcpy (m_StaticGateway, pGateway, 4);
		memcpy (m_StaticDNS, pDNS, 4);
	}

	m_nTimeZone = nTimeZoneMinutes;
	m_SummerTime = SummerTime;
	CTimer::Get ()->SetTimeZone (m_nTimeZone + (m_SummerTime == SummerTimeAlways ? 60 : 0));

	if (Mode == NetModeOff)
	{
		m_State = NetStateOff;
		return;
	}

#ifdef ZX_TEST_NET_ERROR
	// solo para pruebas en QEMU: la red falla antes de crear CNetSubSystem
	m_pErrorReason = "prueba";
	m_State = NetStateError;
	return;
#endif

	// Las redes guardadas: sin ninguna, en automático va por cable (como sin
	// el fichero) y con WiFi se avisa de que hay que elegir una
	boolean bConfig = FALSE;
	{
		CWiFiConfig *pConfig = new CWiFiConfig;
		if (pConfig != 0)
		{
			if (FileExists (WPA_CONFIG_FILE) && !LoadWiFiConfig (pConfig))
			{
				bConfig = TRUE;		// no se entiende: que lo intente wpa_supplicant
			}
			else if (pConfig->GetCount () > 0)
			{
				bConfig = TRUE;
				m_nWiFiNetworks = pConfig->GetCount ();
				strcpy (m_WiFiFirst, pConfig->Get (0)->SSID);
			}
			delete pConfig;
		}
	}
	if (Mode == NetModeAuto)
	{
		Mode = (ModelHasWiFi () && bConfig) ? NetModeWiFi : NetModeEthernet;
	}

	if (Mode == NetModeWiFi)
	{
		if (!ModelHasWiFi ())
		{
			m_State = NetStateNoHardware;
			return;
		}
		if (!bConfig || !FileExists (FIRMWARE_PATH "brcmfmac43430-sdio.bin"))
		{
			m_bWiFi = TRUE;
			m_bNoFirmware = bConfig;
			m_State = NetStateNoConfig;
			return;
		}

		m_bWiFi = TRUE;
		m_pWLAN = new CBcm4343Device (FIRMWARE_PATH);
		if (m_pWLAN == 0 || !m_pWLAN->Initialize ())
		{
			CLogger::Get ()->Write (FromNet, LogError, "No se pudo iniciar el chip WiFi (firmware en %s)",
						FIRMWARE_PATH);
			m_pErrorReason = N_("no arranca el chip WiFi");
			m_State = NetStateError;
			return;
		}
	}

	m_pNet = new CNetSubSystem (pIP != 0 ? m_StaticIP : 0,
				    pIP != 0 ? m_StaticMask : 0,
				    pIP != 0 ? m_StaticGateway : 0,
				    pIP != 0 ? m_StaticDNS : 0, pHostname,
				    m_bWiFi ? NetDeviceTypeWLAN : NetDeviceTypeEthernet);
	if (m_pNet == 0 || !m_pNet->Initialize (FALSE))
	{
		CLogger::Get ()->Write (FromNet, LogNotice, "Sin dispositivo de red");
		delete m_pNet;
		m_pNet = 0;
		m_State = NetStateNoHardware;
		return;
	}

	if (m_bWiFi)
	{
		m_pSupplicant = new CWPASupplicant (WPA_CONFIG_FILE);
		if (m_pSupplicant == 0 || !m_pSupplicant->Initialize ())
		{
			CLogger::Get ()->Write (FromNet, LogError, "No se pudo iniciar WPA: revisa %s",
						WPA_CONFIG_FILE);
			m_pErrorReason = N_("revisa wpa_supplicant.conf");
			m_State = NetStateError;
			return;
		}
	}

	m_State = NetStateConnecting;
	m_nConnectStart = CTimer::Get ()->GetUptime ();
}

boolean CNetworkManager::Process (void)
{
	if (m_State == NetStateRunning)
	{
		UpdateSummerTime ();
		return FALSE;
	}

	if (m_State != NetStateConnecting || m_pNet == 0)
	{
		return FALSE;
	}
	if (!m_pNet->IsRunning ())
	{
		CheckWiFiProgress ();
		return FALSE;
	}

	CString IP;
	m_pNet->GetConfig ()->GetIPAddress ()->Format (&IP);
	strncpy (m_IPAddress, IP, sizeof m_IPAddress - 1);
	m_IPAddress[sizeof m_IPAddress - 1] = '\0';

	if (!m_bServicesStarted)
	{
		m_bServicesStarted = TRUE;

		new CNTPDaemon (NTP_SERVER, m_pNet);
		if (m_nWebMode != 0)			// web=0: sin servidor web
		{
			new CZXWebServer (m_pNet);
		}

		m_pmDNS = new CmDNSPublisher (m_pNet);
		if (m_pmDNS != 0)
		{
			static const char *Text[] = {"path=/", nullptr};
			m_pmDNS->PublishService ("ZXMetalCircle", "_http._tcp", 80, Text);
		}
	}

	CLogger::Get ()->Write (FromNet, LogNotice, "Red en marcha: http://%s/", m_IPAddress);
	m_State = NetStateRunning;

	return TRUE;
}

// Zona horaria y horario de verano cambiados en la configuración: se aplican
// ya (el horario de verano, en la próxima comprobación, si ya hay hora)
void CNetworkManager::SetTimeZone (int nMinutes, TSummerTime SummerTime)
{
	m_nTimeZone = nMinutes;
	m_SummerTime = SummerTime;
	m_nLastDSTCheck = 0;
	CTimer::Get ()->SetTimeZone (m_nTimeZone + (m_SummerTime == SummerTimeAlways ? 60 : 0));
	if (m_State == NetStateRunning)
	{
		UpdateSummerTime ();
	}
}

// dst=: 0 no, on siempre; 1 (y auto), el europeo automático, como hasta ahora
CNetworkManager::TSummerTime CNetworkManager::ParseSummerTime (const char *pValue)
{
	if (strcmp (pValue, "0") == 0 || strcasecmp (pValue, "off") == 0)
	{
		return SummerTimeOff;
	}
	return strcasecmp (pValue, "on") == 0 ? SummerTimeAlways : SummerTimeEU;
}

boolean CNetworkManager::HasTime (void) const
{
	return CTimer::Get ()->GetUniversalTime () > 1600000000;	// después de 2020
}

// Horario de verano europeo: del último domingo de marzo al último domingo de
// octubre, a la 01:00 UTC
void CNetworkManager::UpdateSummerTime (void)
{
	if (m_SummerTime != SummerTimeEU || !HasTime ())
	{
		return;
	}

	unsigned nUTC = CTimer::Get ()->GetUniversalTime ();
	if (nUTC - m_nLastDSTCheck < 60)
	{
		return;
	}
	m_nLastDSTCheck = nUTC;

	unsigned nDays = nUTC / 86400;
	unsigned nYear = 1970;
	while (nDays >= (unsigned) (CTimer::IsLeapYear (nYear) ? 366 : 365))
	{
		nDays -= CTimer::IsLeapYear (nYear) ? 366 : 365;
		nYear++;
	}

	// días desde 1970 hasta el 1 de enero de nYear
	unsigned nYearStart = (nUTC / 86400) - nDays;

	// día del año (desde 0) del último domingo de un mes
	auto LastSunday = [nYear, nYearStart] (unsigned nMonth) -> unsigned
	{
		unsigned nDay = 0;
		for (unsigned m = 0; m <= nMonth; m++)
		{
			nDay += CTimer::GetDaysOfMonth (m, nYear);
		}
		nDay--;					// último día del mes
		unsigned nWeekday = (nYearStart + nDay + 4) % 7;	// 1/1/1970 fue jueves; 0 = domingo
		return nDay - nWeekday;
	};

	unsigned nStart = (nYearStart + LastSunday (2)) * 86400 + 3600;	// marzo
	unsigned nEnd = (nYearStart + LastSunday (9)) * 86400 + 3600;	// octubre

	int nZone = m_nTimeZone + ((nUTC >= nStart && nUTC < nEnd) ? 60 : 0);
	if (CTimer::Get ()->GetTimeZone () != nZone)
	{
		CTimer::Get ()->SetTimeZone (nZone);
	}
}

// La WiFi sin conectar: si la red rechaza la contraseña se avisa al momento,
// y si pasan WIFI_SLOW_SECONDS sin conexión, una vez (lo enseña el kernel)
void CNetworkManager::CheckWiFiProgress (void)
{
	if (!m_bWiFi)
	{
		return;
	}
	if (zxmc_wpa_get_state (0, 0) == 3)
	{
		zxmc_wpa_psk_mismatches = 0;		// conectada: lo de antes ya no vale
		m_nPSKWarned = 0;
	}
	if (zxmc_wpa_psk_mismatches > 0 && m_nPSKWarned == 0)	// solo la primera vez
	{
		m_nPSKWarned = zxmc_wpa_psk_mismatches;
		m_Warning.Format (T ("WiFi: %s rechaza la contraseña"), zxmc_wpa_psk_ssid);
		m_bWarningPending = TRUE;
		CLogger::Get ()->Write (FromNet, LogWarning, "%s: contrasena rechazada", zxmc_wpa_psk_ssid);
	}
	if (   !m_bSlowWarned
	    && CTimer::Get ()->GetUptime () - m_nConnectStart >= WIFI_SLOW_SECONDS)
	{
		m_bSlowWarned = TRUE;
		m_Warning = T ("WiFi sin conectar: mira F1 o reinicia");
		m_bWarningPending = TRUE;
		CLogger::Get ()->Write (FromNet, LogWarning, "Sin conexion WiFi tras %u s: %s",
					WIFI_SLOW_SECONDS, GetStateText ());
	}
}

boolean CNetworkManager::TakeWarning (CString *pText)
{
	if (!m_bWarningPending)
	{
		return FALSE;
	}
	m_bWarningPending = FALSE;
	*pText = m_Warning;
	return TRUE;
}

// En qué paso está la WiFi (conectando): buscando la red, conectando a ella,
// esperando la IP o con la contraseña rechazada
const char *CNetworkManager::GetWiFiStepText (void) const
{
	static CString Text;
	char SSID[WIFI_SSID_MAX + 1];
	int nStep = zxmc_wpa_get_state (SSID, sizeof SSID);
	if (nStep == 3)
	{
		Text.Format (T ("WiFi: conectada a %s, esperando IP"), SSID);
	}
	else if (zxmc_wpa_psk_mismatches > 0)
	{
		Text.Format (T ("WiFi: %s rechaza la contraseña"), zxmc_wpa_psk_ssid);
	}
	else if (nStep == 2)
	{
		Text.Format (T ("WiFi: conectando a %s..."), SSID[0] != '\0' ? SSID : m_WiFiFirst);
	}
	else if (m_nWiFiNetworks > 1)
	{
		Text.Format (T ("WiFi: buscando tus %u redes..."), m_nWiFiNetworks);
	}
	else if (m_nWiFiNetworks == 1)
	{
		Text.Format (T ("WiFi: buscando %s..."), m_WiFiFirst);
	}
	else
	{
		return T ("WiFi: conectando...");	// fichero a mano que no se entiende
	}
	return Text;
}

const char *CNetworkManager::GetStateText (void) const
{
	static CString Text;

	switch (m_State)
	{
	case NetStateOff:		return T ("desactivada");
	case NetStateNoHardware:	return m_bWiFi ? T ("sin WiFi") : T ("sin conexión");
	case NetStateNoConfig:		return m_bNoFirmware ? T ("falta el firmware de la WiFi")
								     : T ("WiFi sin redes: Alt+F1, pág. 7");
	case NetStateConnecting:	return m_bWiFi ? GetWiFiStepText () : T ("cable: esperando conexión");
	case NetStateError:
		Text.Format (T ("error: %s"), T (m_pErrorReason));
		return Text;
	case NetStateRunning:
		if (m_nWebMode == 0)
		{
			Text.Format (T ("%s %s (web apagada)"), m_bWiFi ? "WiFi" : T ("Cable"), m_IPAddress);
		}
		else
		{
			Text.Format ("%s http://%s%s", m_bWiFi ? "WiFi" : T ("Cable"), m_IPAddress,
				     m_nWebMode == 2 ? T (" (solo lectura)") : "");
		}
		return Text;
	}

	return "";
}

void CNetworkManager::RequestLoad (const char *pPath)
{
	m_LoadPath = pPath;
	m_bLoadPending = TRUE;
}

boolean CNetworkManager::TakeLoadRequest (CString *pPath)
{
	if (!m_bLoadPending)
	{
		return FALSE;
	}

	*pPath = m_LoadPath;
	m_bLoadPending = FALSE;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Enlace con el servidor web

const char *WebGetStatus (void)
{
	return CNetworkManager::Get ()->GetEmulatorStatus ();
}

void WebRequestLoad (const char *pPath)
{
	CNetworkManager::Get ()->RequestLoad (pPath);
}

void CNetworkManager::SetWebOptions (unsigned nMode, const char *pPassword)
{
	m_nWebMode = nMode;
	strncpy (m_WebPassword, pPassword != 0 ? pPassword : "", sizeof m_WebPassword - 1);
	m_WebPassword[sizeof m_WebPassword - 1] = '\0';
}

unsigned WebMode (void)
{
	CNetworkManager *p = CNetworkManager::Get ();
	return p != 0 ? p->GetWebMode () : 1;
}

bool WebProtect (void)
{
	CNetworkManager *p = CNetworkManager::Get ();
	return p == 0 || p->GetWebProtect ();
}

const char *WebPassword (void)
{
	CNetworkManager *p = CNetworkManager::Get ();
	return p != 0 ? p->GetWebPassword () : "";
}

void WebBusy (const char *pText)
{
	CNetworkManager *p = CNetworkManager::Get ();
	if (p != 0)
	{
		p->Busy (pText);
	}
}

boolean WebTLSVerify (void)
{
	return CNetworkManager::Get ()->GetTLSVerify ();
}

////////////////////////////////////////////////////////////////////////////////
// Buscar redes WiFi

// Lo que da el chip al buscar sin wpa_supplicant (como en driver_circle.cpp
// de Circle: mismos campos y la misma alineación)
struct TBrcmBSSInfo
{
	u32 nVersion;
	u32 nLength;			// de este registro, con sus IE
	u8 BSSID[6];
	u16 nBeaconPeriod;
	u16 nCapability;		// bit 4: cifrada
	u8 nSSIDLen;
	u8 SSID[32];
	struct
	{
		u32 nCount;
		u8 Rates[16];
	} RateSet;
	u16 nChanSpec;			// canal en los 8 bits bajos
	u16 nATIMWindow;
	u8 nDTIMPeriod;
	u16 nRSSI;			// dBm (con signo)
};

struct TBrcmEScanResult
{
	u32 nBufLen;
	u32 nVersion;
	u16 nSyncID;
	u16 nBSSCount;
	TBrcmBSSInfo BSSInfo;		// nBSSCount de ellos, cada uno de nLength bytes
};

// Añade una red a la lista (si ya está, se queda con la de más señal)
static void AddScanEntry (TWiFiScanEntry *pEntries, unsigned nMax, unsigned *pCount,
			  const char *pSSID, int nRSSI, boolean bOpen)
{
	if (pSSID[0] == '\0')
	{
		return;				// oculta
	}
	for (unsigned i = 0; i < *pCount; i++)
	{
		if (strcmp (pEntries[i].SSID, pSSID) == 0)
		{
			if (nRSSI > pEntries[i].nRSSI)
			{
				pEntries[i].nRSSI = nRSSI;
				pEntries[i].bOpen = bOpen;
			}
			return;
		}
	}
	if (*pCount >= nMax)
	{
		return;
	}
	TWiFiScanEntry *p = &pEntries[(*pCount)++];
	strncpy (p->SSID, pSSID, sizeof p->SSID - 1);
	p->SSID[sizeof p->SSID - 1] = '\0';
	p->nRSSI = nRSSI;
	p->bOpen = bOpen;
}

boolean CNetworkManager::ScanWiFi (TWiFiScanEntry *pEntries, unsigned nMax, unsigned *pCount,
				   const char *pCountry, const char **ppError)
{
	*pCount = 0;
	*ppError = "";
	if (!ModelHasWiFi ())
	{
		*ppError = N_("esta Pi no tiene WiFi");
		return FALSE;
	}
	CScheduler *pScheduler = CScheduler::Get ();

	if (m_pSupplicant != 0 && m_State != NetStateError)
	{
		// WiFi en marcha: que busque wpa_supplicant (unos 3 s) y leer lo que ha visto
		zxmc_wpa_scan_start ();
		unsigned nStart = CTimer::Get ()->GetClockTicks ();
		boolean bStarted = FALSE;
		while (CTimer::Get ()->GetClockTicks () - nStart < 10 * CLOCKHZ)
		{
			pScheduler->MsSleep (100);
			if (zxmc_wpa_scanning ())
			{
				bStarted = TRUE;
			}
			else if (bStarted || CTimer::Get ()->GetClockTicks () - nStart > 2 * CLOCKHZ)
			{
				break;			// terminada (o no empezó: lo que haya)
			}
		}
		char SSID[WIFI_SSID_MAX + 1];
		int nLevel, nFreq, bOpen;
		for (int i = 0; zxmc_wpa_get_bss (i, SSID, sizeof SSID, &nLevel, &nFreq, &bOpen); i++)
		{
			if (nFreq < 3000)		// solo 2,4 GHz
			{
				AddScanEntry (pEntries, nMax, pCount, SSID, nLevel, bOpen);
			}
		}
	}
	else
	{
		// Sin WiFi arrancada: solo el chip (la primera vez carga su firmware)
		if (m_pWLAN == 0)
		{
			if (!FileExists (FIRMWARE_PATH "brcmfmac43430-sdio.bin"))
			{
				*ppError = N_("falta el firmware de la WiFi");
				return FALSE;
			}
			m_pWLAN = new CBcm4343Device (FIRMWARE_PATH);
			if (m_pWLAN == 0 || !m_pWLAN->Initialize ())
			{
				CLogger::Get ()->Write (FromNet, LogError, "Buscar redes: no arranca el chip WiFi");
				delete m_pWLAN;
				m_pWLAN = 0;
				*ppError = N_("no arranca el chip WiFi");
				return FALSE;
			}
		}
		if (pCountry != 0 && pCountry[0] != '\0')
		{
			m_pWLAN->Control ("country %s", pCountry);
		}
		u8 *pBuffer = new u8[FRAME_BUFFER_SIZE];
		if (pBuffer == 0)
		{
			return FALSE;
		}
		unsigned nLength;
		while (m_pWLAN->ReceiveScanResult (pBuffer, &nLength))
		{
			// lo que quedara de antes
		}
		if (!m_pWLAN->Control ("escan 4"))
		{
			delete [] pBuffer;
			*ppError = N_("el chip WiFi no busca");
			return FALSE;
		}
		unsigned nStart = CTimer::Get ()->GetClockTicks ();
		while (CTimer::Get ()->GetClockTicks () - nStart < 5 * CLOCKHZ)
		{
			pScheduler->MsSleep (50);
			while (m_pWLAN->ReceiveScanResult (pBuffer, &nLength))
			{
				if (nLength < sizeof (TBrcmEScanResult))
				{
					continue;
				}
				const TBrcmEScanResult *pResult = (const TBrcmEScanResult *) pBuffer;
				const u8 *pBSS = (const u8 *) &pResult->BSSInfo;
				for (unsigned i = 0; i < pResult->nBSSCount; i++)
				{
					const TBrcmBSSInfo *pInfo = (const TBrcmBSSInfo *) pBSS;
					if (   pBSS + sizeof (TBrcmBSSInfo) > pBuffer + nLength
					    || pInfo->nLength < sizeof (TBrcmBSSInfo))
					{
						break;
					}
					unsigned nChannel = pInfo->nChanSpec & 0xFF;
					if (nChannel >= 1 && nChannel <= 14 && pInfo->nSSIDLen <= 32)
					{
						char SSID[WIFI_SSID_MAX + 1];
						memcpy (SSID, pInfo->SSID, pInfo->nSSIDLen);
						SSID[pInfo->nSSIDLen] = '\0';
						AddScanEntry (pEntries, nMax, pCount, SSID, (s16) pInfo->nRSSI,
							      !(pInfo->nCapability & 0x0010));
					}
					pBSS += pInfo->nLength;
				}
			}
		}
		m_pWLAN->Control ("escan 0");		// por si sigue
		delete [] pBuffer;
	}

	// de más a menos señal
	for (unsigned i = 1; i < *pCount; i++)
	{
		TWiFiScanEntry Entry = pEntries[i];
		unsigned j = i;
		while (j > 0 && pEntries[j - 1].nRSSI < Entry.nRSSI)
		{
			pEntries[j] = pEntries[j - 1];
			j--;
		}
		pEntries[j] = Entry;
	}
	CLogger::Get ()->Write (FromNet, LogNotice, "Buscar redes: %u", *pCount);
	return TRUE;
}
