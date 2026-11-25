
/*
 * ZXMetalCircle: lo añade build.sh al final de addon/wlan/ether4330.c de
 * Circle (y una línea en etherbcmpnp que guarda el controlador). Da a la
 * pantalla Sistema del emulador el nombre de la red WiFi y la intensidad de
 * la señal. Las dos funciones devuelven 0 si no hay WiFi o no está conectada.
 */
void *zxmc_ether4330_ctlr;

int
ether4330_get_essid(char *buf, int len)
{
	Ctlr *ctl = (Ctlr*)zxmc_ether4330_ctlr;

	if(ctl == nil || ctl->status != Connected || ctl->essid[0] == 0 || len <= 1)
		return 0;
	strncpy(buf, ctl->essid, len - 1);
	buf[len - 1] = 0;
	return 1;
}

int
ether4330_get_rssi(void)
{
	Ctlr *ctl = (Ctlr*)zxmc_ether4330_ctlr;
	uchar buf[4];

	if(ctl == nil || ctl->status != Connected)
		return 0;
	if(waserror())
		return 0;
	wlcmd(ctl, 0, 127, nil, 0, buf, 4);	/* WLC_GET_RSSI: dBm */
	poperror();
	return (int)(u32int)get4(buf);
}
