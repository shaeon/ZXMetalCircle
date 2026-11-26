
/*
 * ZXMetalCircle: lo añade build.sh al final de
 * addon/wlan/hostap/wpa_supplicant/events.c de Circle (y la llamada, después
 * de wpas_notify_psk_mismatch). Apunta que la red ha rechazado la contraseña
 * (el 4-way handshake ha fallado) y cuál era, para avisar en el emulador.
 */
int zxmc_wpa_psk_mismatches;
char zxmc_wpa_psk_ssid[33];

void zxmc_wpa_psk_mismatch (void *p)
{
	struct wpa_supplicant *wpa_s = p;
	size_t len = 0;

	zxmc_wpa_psk_mismatches++;
	if (wpa_s->current_ssid != NULL)
	{
		len = wpa_s->current_ssid->ssid_len;
		if (len > sizeof zxmc_wpa_psk_ssid - 1)
			len = sizeof zxmc_wpa_psk_ssid - 1;
		os_memcpy (zxmc_wpa_psk_ssid, wpa_s->current_ssid->ssid, len);
	}
	zxmc_wpa_psk_ssid[len] = '\0';
}
