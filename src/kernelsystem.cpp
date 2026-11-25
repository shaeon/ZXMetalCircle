//
// kernelsystem.cpp
//
// Pantalla Sistema (S en la ayuda; en el teclado GPIO, la S con la ayuda
// abierta): qué Pi es, su temperatura y si le ha faltado tensión, la pantalla,
// el kernel y la versión de Circle, la tarjeta, la red (con el nombre de la
// WiFi y su señal), la hora y los aparatos USB. Es lo que hay que mirar
// cuando alguien informa de un fallo ("mándame una foto de F1 > S").
//
// Las líneas se rehacen una vez por segundo mientras se ve la pantalla; lo que
// tarda (el espacio libre de la tarjeta y la frecuencia de la pantalla) solo
// al abrirla. Las marcas de las líneas son las de CMenu::DrawMarked: |valor|,
// ~aviso~ (amarillo) y ^alerta^ (rojo); las que empiezan por # son títulos.
//
// Además, CheckPowerWarning () mira cada pocos segundos si el firmware ha
// detectado bajo voltaje y, la primera vez, lo avisa en pantalla: una fuente
// floja es la causa típica de cuelgues y de aparatos USB que se pierden.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include "i18n.h"
#include "version.h"
#include <circle/machineinfo.h>
#include <circle/memory.h>
#include <circle/bcmpropertytags.h>
#include <circle/version.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <fatfs/ff.h>

static const char FromSystem[] = "system";

// Datos de la WiFi que añade build.sh al controlador de Circle (ether4330.c);
// 0 si no hay WiFi o no está conectada
extern "C" int ether4330_get_essid (char *pBuffer, int nSize);
extern "C" int ether4330_get_rssi (void);

// Estado del firmware (PROPTAG_GET_THROTTLED): lo que pasa ahora (bits 0-3) y
// lo que ha pasado desde el arranque (bits 16-19)
#define THROTTLED_UNDERVOLT_NOW		(1 << 0)
#define THROTTLED_CAPPED_NOW		(1 << 1)
#define THROTTLED_THROTTLED_NOW		(1 << 2)
#define THROTTLED_SOFTTEMP_NOW		(1 << 3)
#define THROTTLED_UNDERVOLT		(1 << 16)
#define THROTTLED_SLOWED		(0xE << 16)	// frecuencia limitada, frenada o límite de temperatura

#define SYSTEM_HOT_CELSIUS		80		// desde aquí, la temperatura en rojo
#define POWER_CHECK_SECONDS		5

// Nombre del kernel que arranca este modelo (el de config.txt)
static const char *KernelName (void)
{
#if RASPPI == 1
	return "kernel.img";
#elif RASPPI == 2
	return "kernel7.img";
#elif RASPPI == 3
	#if AARCH == 64
	return "kernel8.img";
	#else
	return "kernel8-32.img";
	#endif
#elif RASPPI == 4
	#if AARCH == 64
	return "kernel8-rpi4.img";
	#else
	return "kernel7l.img";
	#endif
#else
	return "kernel_2712.img";
#endif
}

// Estado del firmware; FALSE si no se puede preguntar (QEMU)
static boolean GetThrottled (unsigned *pState)
{
	CBcmPropertyTags Tags;
	TPropertyTagSimple Throttled;
	Throttled.nValue = 0xFFFF;		// como Circle (CCPUThrottle)
	if (!Tags.GetTag (PROPTAG_GET_THROTTLED, &Throttled, sizeof Throttled, 4))
	{
		return FALSE;
	}
	*pState = Throttled.nValue;
	return TRUE;
}

static unsigned GetCPUMHz (void)
{
	CBcmPropertyTags Tags;
	TPropertyTagClockRate Clock;
	Clock.nClockId = CLOCK_ID_ARM;
	if (!Tags.GetTag (PROPTAG_GET_CLOCK_RATE, &Clock, sizeof Clock, 4))
	{
		return 0;
	}
	return (Clock.nRate + 500000) / 1000000;
}

static unsigned GetCelsius (void)
{
	CBcmPropertyTags Tags;
	TPropertyTagTemperature Temperature;
	Temperature.nTemperatureId = 0;
	if (!Tags.GetTag (PROPTAG_GET_TEMPERATURE, &Temperature, sizeof Temperature, 4))
	{
		return 0;
	}
	return (Temperature.nValue + 500) / 1000;
}

// "512 MB", "3,1 GB"
static void FormatBytes (CString *pText, u64 nBytes)
{
	if (nBytes < 1024ULL * 1024 * 1024)
	{
		pText->Format ("%u MB", (unsigned) ((nBytes + 512 * 1024) / (1024 * 1024)));
	}
	else
	{
		unsigned nTenths = (unsigned) ((nBytes * 10 + 512ULL * 1024 * 1024) / (1024ULL * 1024 * 1024));
		pText->Format ("%u,%u GB", nTenths / 10, nTenths % 10);
	}
}

// "25/11/2026 10:12" de __DATE__ ("Nov 25 2026") y __TIME__ ("10:12:33")
static void FormatBuildDate (CString *pText)
{
	static const char Months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	const char *pDate = __DATE__;
	unsigned nMonth = 1;
	for (unsigned i = 0; i < 12; i++)
	{
		if (strncmp (pDate, Months + i * 3, 3) == 0)
		{
			nMonth = i + 1;
		}
	}
	unsigned nDay = (pDate[4] == ' ' ? 0 : pDate[4] - '0') * 10 + pDate[5] - '0';
	char Time[6];
	memcpy (Time, __TIME__, 5);		// sin los segundos
	Time[5] = '\0';
	pText->Format ("%02u/%02u/%s %s", nDay, nMonth, pDate + 7, Time);
}

void CKernel::UpdateSystemLines (boolean bOpening)
{
	unsigned nNow = CTimer::Get ()->GetUptime ();
	if (!bOpening && nNow == m_nSystemLast)
	{
		return;				// una vez por segundo
	}
	m_nSystemLast = nNow;

	// Lo que tarda, solo al abrir: la tarjeta (FatFs ya suele saber lo libre,
	// pero si no, recorre la FAT) y la frecuencia de la pantalla (cinco
	// refrescos)
	if (bOpening)
	{
		m_SystemSD = T ("|no hay|");
		DWORD nFreeClusters;
		FATFS *pFS;
		if (m_bSDMounted && f_getfree ("SD:", &nFreeClusters, &pFS) == FR_OK)
		{
			u64 nCluster = (u64) pFS->csize * 512;
			CString Total, Free;
			FormatBytes (&Total, (u64) (pFS->n_fatent - 2) * nCluster);
			FormatBytes (&Free, (u64) nFreeClusters * nCluster);
			m_SystemSD.Format (T ("|%s|, libres |%s|"), (const char *) Total, (const char *) Free);
		}
		m_nDisplayHz = m_Video.MeasureRefresh ();
	}

	unsigned nLine = 0;
	CString Line;

	// La Pi
	m_Menu.SetSystemLine (nLine++, T ("#RASPBERRY PI"));
	CMachineInfo *pInfo = CMachineInfo::Get ();
	Line.Format (T ("Modelo:    |%s|"), pInfo->GetMachineName ());
	m_Menu.SetSystemLine (nLine++, Line);

	unsigned nRAM = pInfo->GetRAMSize ();
	CString RAM, Free;
	if (nRAM >= 1024)
	{
		RAM.Format ("%u GB", nRAM / 1024);
	}
	else
	{
		RAM.Format ("%u MB", nRAM);
	}
	FormatBytes (&Free, CMemorySystem::Get ()->GetHeapFreeSpace (HEAP_ANY));
	Line.Format (T ("RAM:       |%s| (libres: |%s|)"), (const char *) RAM, (const char *) Free);
	m_Menu.SetSystemLine (nLine++, Line);

	unsigned nMHz = GetCPUMHz ();
	unsigned nCelsius = GetCelsius ();
	CString Temperature;
	if (nCelsius == 0)
	{
		Temperature = "|-|";
	}
	else
	{
		Temperature.Format (nCelsius >= SYSTEM_HOT_CELSIUS ? "^%u °C^" : "|%u °C|", nCelsius);
	}
	Line.Format (T ("CPU:       |%u MHz|   Temp.: %s"), nMHz, (const char *) Temperature);
	m_Menu.SetSystemLine (nLine++, Line);

	unsigned nThrottled;
	if (!GetThrottled (&nThrottled))
	{
		Line = T ("Avisos:    |-|");
	}
	else if ((nThrottled & (THROTTLED_UNDERVOLT | THROTTLED_UNDERVOLT_NOW | THROTTLED_SLOWED
				| THROTTLED_CAPPED_NOW | THROTTLED_THROTTLED_NOW | THROTTLED_SOFTTEMP_NOW)) == 0)
	{
		Line = T ("Avisos:    |ninguno|");
	}
	else
	{
		Line = T ("Avisos:    ");
		if (nThrottled & (THROTTLED_UNDERVOLT | THROTTLED_UNDERVOLT_NOW))
		{
			Line.Append ("^");
			Line.Append (T ("bajo voltaje"));
			Line.Append ("^");
		}
		if (nThrottled & (THROTTLED_SLOWED | THROTTLED_CAPPED_NOW | THROTTLED_THROTTLED_NOW
				  | THROTTLED_SOFTTEMP_NOW))
		{
			if (nThrottled & (THROTTLED_UNDERVOLT | THROTTLED_UNDERVOLT_NOW))
			{
				Line.Append (", ");
			}
			Line.Append ("~");
			Line.Append (T ("CPU frenada"));
			Line.Append ("~");
		}
	}
	m_Menu.SetSystemLine (nLine++, Line);

	if (m_nDisplayHz != 0)
	{
		Line.Format (T ("Pantalla:  |%ux%u| a |%u Hz|"), m_Video.GetDisplayWidth (),
			     m_Video.GetDisplayHeight (), m_nDisplayHz);
	}
	else
	{
		Line.Format (T ("Pantalla:  |%ux%u|"), m_Video.GetDisplayWidth (), m_Video.GetDisplayHeight ());
	}
	m_Menu.SetSystemLine (nLine++, Line);

	Line.Format (T ("Sonido:    |%s|"), T (m_pSoundName));
	m_Menu.SetSystemLine (nLine++, Line);
	m_Menu.SetSystemLine (nLine++, "");

	// El emulador
	m_Menu.SetSystemLine (nLine++, T ("#EMULADOR"));
	CString Build;
	FormatBuildDate (&Build);
	Line.Format (T ("Versión:   |%s| (|%s|)"), ZXCIRCLE_VERSION, (const char *) Build);
	m_Menu.SetSystemLine (nLine++, Line);
	Line.Format (T ("Kernel:    |%s|, |%u bits|"), KernelName (), AARCH);
	m_Menu.SetSystemLine (nLine++, Line);
	Line.Format ("Circle |%s|%s, gcc |%u.%u|", CIRCLE_VERSION_STRING,
#if STDLIB_SUPPORT >= 2
		     " + newlib",
#else
		     "",
#endif
		     __GNUC__, __GNUC_MINOR__);
	m_Menu.SetSystemLine (nLine++, Line);
	Line.Format (m_bSDOnUSB ? T ("SD (USB): %s") : T ("SD:        %s"), (const char *) m_SystemSD);
	m_Menu.SetSystemLine (nLine++, Line);
	m_Menu.SetSystemLine (nLine++, "");

	// La red y la hora
	m_Menu.SetSystemLine (nLine++, T ("#RED Y HORA"));
	boolean bConnected = CNetworkManager::IsNetRunning () && m_Network.GetIPAddress ()[0] != '\0';
	if (bConnected)
	{
		Line.Format (T ("Red:       |%s|, |%s|"), m_Network.IsWiFi () ? "WiFi" : T ("cable"),
			     m_Network.GetIPAddress ());
	}
	else
	{
		Line.Format (T ("Red:       |%s|"), m_Network.GetStateText ());
	}
	m_Menu.SetSystemLine (nLine++, Line);

	char SSID[33];
	if (bConnected && m_Network.IsWiFi () && ether4330_get_essid (SSID, sizeof SSID))
	{
		Line.Format (T ("WiFi:      |%s|"), SSID);
		m_Menu.SetSystemLine (nLine++, Line);

		// la señal, cada tres segundos (es una orden al chip)
		if (bOpening || nNow - m_nSystemRSSITime >= 3)
		{
			m_nSystemRSSI = ether4330_get_rssi ();
			m_nSystemRSSITime = nNow;
		}
		int nRSSI = m_nSystemRSSI;
		if (nRSSI < 0)
		{
			const char *pQuality = nRSSI > -60 ? T ("buena") : nRSSI >= -70 ? T ("regular") : T ("mala");
			char chMark = nRSSI > -60 ? '|' : nRSSI >= -70 ? '~' : '^';
			Line.Format (T ("Señal:     %c%s (%d dBm)%c"), chMark, pQuality, nRSSI, chMark);
		}
		else
		{
			Line = T ("Señal:     |-|");
		}
		m_Menu.SetSystemLine (nLine++, Line);
	}
	if (bConnected)
	{
		m_Menu.SetSystemLine (nLine++, T ("Nombre:    |zxmetalcircle.local|"));
	}

	char Now[20];
	if (CMenu::FormatNow (Now, sizeof Now))
	{
		Line.Format (T ("Hora:      |%s| (internet)"), Now);
	}
	else
	{
		Line = T ("Hora:      |todavía no (de internet)|");
	}
	m_Menu.SetSystemLine (nLine++, Line);

	int nZone = m_Network.GetTimeZone ();
	CString Zone;
	unsigned nAbsZone = nZone < 0 ? -nZone : nZone;
	if (nAbsZone % 60 == 0)
	{
		Zone.Format ("UTC%c%u", nZone < 0 ? '-' : '+', nAbsZone / 60);
	}
	else
	{
		Zone.Format ("UTC%c%u:%02u", nZone < 0 ? '-' : '+', nAbsZone / 60, nAbsZone % 60);
	}
	const char *pSummer;
	switch (m_Network.GetSummerTime ())
	{
	case CNetworkManager::SummerTimeOff:	pSummer = T ("no");	break;
	case CNetworkManager::SummerTimeAlways:	pSummer = T ("siempre");	break;
	default:
		pSummer = CTimer::Get ()->GetTimeZone () != nZone ? T ("auto, ahora sí") : T ("auto, ahora no");
		break;
	}
	Line.Format (T ("Zona:      |%s|, verano: |%s|"), (const char *) Zone, pSummer);
	m_Menu.SetSystemLine (nLine++, Line);

	unsigned nMinutes = nNow / 60;
	if (nMinutes < 60)
	{
		Line.Format (T ("Encendida: |%u min|"), nMinutes);
	}
	else
	{
		Line.Format (T ("Encendida: |%u h %u min|"), nMinutes / 60, nMinutes % 60);
	}
	m_Menu.SetSystemLine (nLine++, Line);
	m_Menu.SetSystemLine (nLine++, "");

	// Lo que hay conectado
	unsigned nKeyboards = 0, nPads = 0;
	for (unsigned i = 0; i < MAX_KEYBOARDS; i++)
	{
		nKeyboards += m_pKeyboards[i] != 0;
	}
	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		nPads += m_pGamePads[i] != 0;
	}
	Line.Format (T ("USB:       teclados |%u|, ratón |%u|, mandos |%u|"),
		     nKeyboards, m_pMouse != 0 ? 1 : 0, nPads);
	m_Menu.SetSystemLine (nLine++, Line);
	m_Menu.SetSystemLine (nLine++, m_bGPIOKeyboard ? T ("Teclado GPIO: |sí|") : T ("Teclado GPIO: |no|"));

	while (nLine < MENU_SYSTEM_LINES)
	{
		m_Menu.SetSystemLine (nLine++, "");
	}
}

// Bajo voltaje: aviso en pantalla la primera vez (se mira cada pocos segundos)
void CKernel::CheckPowerWarning (void)
{
	if (m_bPowerWarned)
	{
		return;
	}
	unsigned nNow = CTimer::Get ()->GetUptime ();
	if (nNow - m_nPowerCheckTime < POWER_CHECK_SECONDS)
	{
		return;
	}
	m_nPowerCheckTime = nNow;

	unsigned nThrottled;
	if (   GetThrottled (&nThrottled)
	    && (nThrottled & (THROTTLED_UNDERVOLT | THROTTLED_UNDERVOLT_NOW)))
	{
		m_bPowerWarned = TRUE;
		m_Logger.Write (FromSystem, LogWarning, "Under-voltage detected (0x%X)", nThrottled);
		size_t nLen = strnlen (T ("Bajo voltaje: la fuente no da bastante"), sizeof m_OptionWarning - 1);
		memcpy (m_OptionWarning, T ("Bajo voltaje: la fuente no da bastante"), nLen);
		m_OptionWarning[nLen] = '\0';
		ShowMessage (m_OptionWarning);
		m_nWarningFrames = 500;			// unos 10 s
		m_nMessageFrames = m_nWarningFrames;
	}
}
