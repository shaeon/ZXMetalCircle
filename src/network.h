//
// network.h - Red para ZXMetalCircle: WiFi (o Ethernet), hora por NTP, mDNS y
//             servidor web para gestionar los ficheros de la SD
//
// La red de Circle es cooperativa: sus tareas (DHCP, WPA, servidor web...) solo
// avanzan cuando el bucle principal cede el control (CScheduler::Yield). El
// emulador lo hace mientras espera al siguiente cuadro, así que todo lo que
// hacen las tareas de red ocurre entre dos cuadros y no hay problemas de
// concurrencia con la emulación ni con el sistema de ficheros.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_network_h
#define _zx_network_h

#include <circle/net/netsubsystem.h>
#include <circle/net/httpdaemon.h>
#include <circle/string.h>
#include <circle/types.h>

class CBcm4343Device;
class CWPASupplicant;
class CmDNSPublisher;

enum TNetMode
{
	NetModeOff,
	NetModeAuto,		// WiFi si el modelo la tiene y hay configuración; si no, cable
	NetModeWiFi,
	NetModeEthernet
};

enum TNetState
{
	NetStateOff,
	NetStateNoHardware,	// el modelo no tiene WiFi ni hay adaptador de red
	NetStateNoConfig,	// falta wpa_supplicant.conf o el firmware
	NetStateConnecting,
	NetStateRunning,
	NetStateError
};

class CNetworkManager
{
public:
	CNetworkManager (void);

	// Arranca la red sin esperar a que conecte (la conexión sigue en segundo plano)
	// pIP == 0: DHCP; si no, IP fija (máscara, puerta de enlace y DNS)
	void Start (TNetMode Mode, const char *pHostname, int nTimeZoneMinutes, boolean bEUSummerTime,
		    const u8 *pIP = 0, const u8 *pMask = 0, const u8 *pGateway = 0, const u8 *pDNS = 0);

	// Llamar una vez por cuadro: arranca los servicios cuando hay conexión y
	// mantiene el horario de verano. Devuelve TRUE la primera vez que conecta.
	boolean Process (void);

	TNetState GetState (void) const		{ return m_State; }
	boolean IsWiFi (void) const		{ return m_bWiFi; }
	const char *GetIPAddress (void) const	{ return m_IPAddress; }
	const char *GetStateText (void) const;	// para la ayuda (F1)
	boolean HasTime (void) const;		// hora puesta por NTP

	// Estado del emulador para la web (lo actualiza el kernel)
	void SetEmulatorStatus (const char *pText)	{ m_EmulatorStatus = pText; }
	const char *GetEmulatorStatus (void) const	{ return m_EmulatorStatus; }

	// Petición de carga desde la web (la atiende el kernel en su bucle)
	void RequestLoad (const char *pPath);
	boolean TakeLoadRequest (CString *pPath);

	static CNetworkManager *Get (void)	{ return s_pThis; }

	// La red de Circle o 0 si no se ha creado. Hay que usar esto y no
	// CNetSubSystem::Get (), que se detiene con un assert si no existe.
	static CNetSubSystem *GetNet (void)
		{ return s_pThis != 0 ? s_pThis->m_pNet : 0; }
	static boolean IsNetRunning (void)
		{ CNetSubSystem *p = GetNet (); return p != 0 && p->IsRunning (); }

	const char *GetErrorReason (void) const	{ return m_pErrorReason; }

	// Aviso de "red en uso" (lo pinta el kernel al momento, porque mientras la
	// tarea de red trabaja la emulación está parada)
	typedef void TBusyHandler (const char *pText, void *pParam);
	void SetBusyHandler (TBusyHandler *pHandler, void *pParam)
		{ m_pBusyHandler = pHandler; m_pBusyParam = pParam; }
	void Busy (const char *pText)
		{ if (m_pBusyHandler != 0) m_pBusyHandler (pText, m_pBusyParam); }

	void SetTLSVerify (boolean bVerify)	{ m_bTLSVerify = bVerify; }
	// Servidor web: 0 = apagado, 1 = completo, 2 = solo lectura; contraseña ("" = sin)
	void SetWebOptions (unsigned nMode, const char *pPassword);
	unsigned GetWebMode (void) const	{ return m_nWebMode; }
	const char *GetWebPassword (void) const	{ return m_WebPassword; }
	boolean GetTLSVerify (void) const	{ return m_bTLSVerify; }

private:
	void UpdateSummerTime (void);

private:
	TNetState m_State;
	boolean m_bWiFi;
	boolean m_bServicesStarted;
	int m_nTimeZone;
	boolean m_bEUSummerTime;
	unsigned m_nLastDSTCheck;

	CBcm4343Device *m_pWLAN;
	CNetSubSystem *m_pNet;
	CWPASupplicant *m_pSupplicant;
	CmDNSPublisher *m_pmDNS;

	char m_IPAddress[20];
	const char *m_pErrorReason;	// por qué falló (para la ayuda y el registro)
	u8 m_StaticIP[4], m_StaticMask[4], m_StaticGateway[4], m_StaticDNS[4];
	CString m_EmulatorStatus;

	boolean m_bLoadPending;
	boolean m_bTLSVerify;
	unsigned m_nWebMode;
	char m_WebPassword[33];
	TBusyHandler *m_pBusyHandler;
	void *m_pBusyParam;
	CString m_LoadPath;

	static CNetworkManager *s_pThis;
};

#endif
