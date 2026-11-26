
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
