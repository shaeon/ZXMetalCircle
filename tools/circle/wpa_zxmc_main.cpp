
/*
 * ZXMetalCircle: lo añade build.sh al final de
 * addon/wlan/hostap/wpa_supplicant/main_circle.cpp de Circle. Dice en qué
 * paso está la conexión de la WiFi, para que el emulador lo enseñe en vez de
 * solo "conectando...":
 *   0  wpa_supplicant no está en marcha
 *   1  buscando la red (o desconectada)
 *   2  conectando: asociándose o con la contraseña (4-way handshake)
 *   3  conectada (falta la IP: eso lo dice la red de Circle)
 * pSSID: la red con la que está, si la hay ("" si aún no ha elegido una).
 */
int zxmc_wpa_get_state (char *pSSID, int nSize)
{
	if (pSSID != NULL && nSize > 0)
	{
		pSSID[0] = '\0';
	}
	if (   global == NULL
	    || global->ifaces == NULL)
	{
		return 0;
	}

	struct wpa_supplicant *wpa_s = global->ifaces;
	if (pSSID != NULL && nSize > 0 && wpa_s->current_ssid != NULL)
	{
		size_t nLen = wpa_s->current_ssid->ssid_len;
		if (nLen > (size_t) nSize - 1)
		{
			nLen = nSize - 1;
		}
		os_memcpy (pSSID, wpa_s->current_ssid->ssid, nLen);
		pSSID[nLen] = '\0';
	}

	switch (wpa_s->wpa_state)
	{
	case WPA_COMPLETED:
		return 3;

	case WPA_AUTHENTICATING:
	case WPA_ASSOCIATING:
	case WPA_ASSOCIATED:
	case WPA_4WAY_HANDSHAKE:
	case WPA_GROUP_HANDSHAKE:
		return 2;

	default:
		return 1;
	}
}

/*
 * Buscar redes con wpa_supplicant en marcha (la WiFi ya arrancada): se le
 * pide una búsqueda como la del "scan" de wpa_cli, se espera a que termine
 * (zxmc_wpa_scanning) y se leen las redes que ha visto (zxmc_wpa_get_bss).
 */
extern "C" {
#include "bss.h"
#include "scan.h"
}

int zxmc_wpa_scan_start (void)
{
	if (   global == NULL
	    || global->ifaces == NULL)
	{
		return 0;
	}

	struct wpa_supplicant *wpa_s = global->ifaces;
	wpa_s->scan_req = wpa_supplicant::MANUAL_SCAN_REQ;	/* enum dentro del struct: en C++, con su nombre */
	wpa_supplicant_req_scan (wpa_s, 0, 0);

	return 1;
}

int zxmc_wpa_scanning (void)
{
	if (   global == NULL
	    || global->ifaces == NULL)
	{
		return 0;
	}

	return global->ifaces->scanning;
}

/*
 * La red nIndex de las vistas: nombre, señal (dBm), frecuencia (MHz) y si
 * es abierta (sin cifrado). 0 si no hay tantas.
 */
int zxmc_wpa_get_bss (int nIndex, char *pSSID, int nSize, int *pLevel, int *pFreq, int *pOpen)
{
	if (   global == NULL
	    || global->ifaces == NULL)
	{
		return 0;
	}

	struct wpa_supplicant *wpa_s = global->ifaces;
	struct wpa_bss *bss;
	int i = 0;
	dl_list_for_each (bss, &wpa_s->bss, struct wpa_bss, list)
	{
		if (i++ != nIndex)
		{
			continue;
		}

		size_t nLen = bss->ssid_len;
		if (nLen > (size_t) nSize - 1)
		{
			nLen = nSize - 1;
		}
		os_memcpy (pSSID, bss->ssid, nLen);
		pSSID[nLen] = '\0';
		*pLevel = bss->level;
		*pFreq = bss->freq;
		*pOpen = !(bss->caps & 0x0010);		/* IEEE80211_CAP_PRIVACY */

		return 1;
	}

	return 0;
}
