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
#include <circle/logger.h>
#include <circle/util.h>
#include <fatfs/ff.h>

#define FIRMWARE_PATH		"SD:/firmware/"
#define WPA_CONFIG_FILE		"SD:/wpa_supplicant.conf"
#define NTP_SERVER		"pool.ntp.org"


static const char FromNet[] = "net";

CNetworkManager *CNetworkManager::s_pThis = 0;

////////////////////////////////////////////////////////////////////////////////
// Gestor de red

CNetworkManager::CNetworkManager (void)
:	m_State (NetStateOff),
	m_bWiFi (FALSE),
	m_bServicesStarted (FALSE),
	m_nTimeZone (60),
	m_bEUSummerTime (TRUE),
	m_nLastDSTCheck (0),
	m_pWLAN (0),
	m_pNet (0),
	m_pSupplicant (0),
	m_pmDNS (0),
	m_bLoadPending (FALSE),
	m_bTLSVerify (TRUE),
	m_nWebMode (1),
	m_pBusyHandler (0),
	m_pBusyParam (0)
{
	m_WebPassword[0] = '\0';
	s_pThis = this;
	m_IPAddress[0] = '\0';
}

static boolean ModelHasWiFi (void)
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

void CNetworkManager::Start (TNetMode Mode, const char *pHostname, int nTimeZoneMinutes,
			     boolean bEUSummerTime, const u8 *pIP, const u8 *pMask,
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
	m_bEUSummerTime = bEUSummerTime;
	CTimer::Get ()->SetTimeZone (m_nTimeZone);

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

	boolean bConfig = FileExists (WPA_CONFIG_FILE);
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
}

boolean CNetworkManager::Process (void)
{
	if (m_State == NetStateRunning)
	{
		UpdateSummerTime ();
		return FALSE;
	}

	if (m_State != NetStateConnecting || m_pNet == 0 || !m_pNet->IsRunning ())
	{
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

boolean CNetworkManager::HasTime (void) const
{
	return CTimer::Get ()->GetUniversalTime () > 1600000000;	// después de 2020
}

// Horario de verano europeo: del último domingo de marzo al último domingo de
// octubre, a la 01:00 UTC
void CNetworkManager::UpdateSummerTime (void)
{
	if (!m_bEUSummerTime || !HasTime ())
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

const char *CNetworkManager::GetStateText (void) const
{
	static CString Text;

	switch (m_State)
	{
	case NetStateOff:		return T ("desactivada");
	case NetStateNoHardware:	return m_bWiFi ? T ("sin WiFi") : T ("sin conexión");
	case NetStateNoConfig:		return T ("falta wpa_supplicant.conf");
	case NetStateConnecting:	return m_bWiFi ? T ("WiFi: conectando...") : T ("cable: esperando conexión");
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
