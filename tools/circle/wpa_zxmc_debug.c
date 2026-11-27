

/*
 * ZXMetalCircle: lo añade build.sh al final de
 * addon/wlan/hostap/src/utils/wpa_debug_circle.c de Circle. Registro de la
 * WiFi (wifilog=): con zxmc_wpa_log_on (lo pone el emulador), se copian con
 * su hora a un búfer que el emulador va pasando a SD:/wifi-diag.txt
 * (zxmc_wpa_log_take):
 *
 *  - los mensajes de wpa_supplicant: wpa_printf y wpa_msg llaman a
 *    zxmc_wpa_vprintf en vez de a vprintf ("wpa:"; "wpa!" los errores;
 *    "wpa " los de depuración, con wifilog=2). A la consola solo va lo de
 *    siempre (MSG_INFO y más): con wifilog=2 el emulador baja
 *    wpa_debug_level a MSG_DEBUG y lo de más solo va al fichero.
 *  - lo que escribe el controlador del chip (print de p9util.cpp, por
 *    ejemplo por qué falla un "join": el controlador de Circle no se lo
 *    dice a wpa_supplicant), con "chip".
 *  - cada evento del firmware del chip (bcmevent de ether4330.c): nombre,
 *    estado y motivo (el código que manda el router al desconectar), con
 *    "chip"; menos los resultados parciales de las búsquedas, que son
 *    muchos.
 *
 * wpa_supplicant, el chip y el emulador son tareas del mismo núcleo, que se
 * turnan (no hay dos a la vez en el búfer).
 */
#define ZXMC_WPA_LOG_SIZE	65536

int zxmc_wpa_log_on;
static char zxmc_wpa_log[ZXMC_WPA_LOG_SIZE];
static unsigned zxmc_wpa_log_len;
static unsigned zxmc_wpa_log_lost;

/* Una línea al búfer: la hora, la etiqueta y el texto (fmt, ap) */
static void zxmc_log_line (const char *tag, const char *fmt, va_list ap)
{
	char line[512];
	struct os_reltime t;
	int n, m;

	os_get_reltime (&t);
	n = snprintf (line, sizeof line, "%6ld.%02ld %s ", (long) t.sec, (long) (t.usec / 10000), tag);
	m = vsnprintf (line + n, sizeof line - n, fmt, ap);
	if (m < 0)
		m = 0;
	n += m;
	if (n > (int) sizeof line - 3)
		n = sizeof line - 3;
	while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
		n--;
	line[n++] = '\r';
	line[n++] = '\n';

	if (zxmc_wpa_log_len + n <= ZXMC_WPA_LOG_SIZE)
	{
		os_memcpy (zxmc_wpa_log + zxmc_wpa_log_len, line, n);
		zxmc_wpa_log_len += n;
	}
	else
	{
		zxmc_wpa_log_lost++;
	}
}

static void zxmc_log (const char *tag, const char *fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	zxmc_log_line (tag, fmt, ap);
	va_end (ap);
}

void zxmc_wpa_vprintf (int level, const char *fmt, va_list ap)
{
	if (zxmc_wpa_log_on)
	{
		va_list ap2;
		__builtin_va_copy (ap2, ap);	/* (la stdarg.h de Circle no tiene va_copy) */
		zxmc_log_line (level >= MSG_ERROR ? "wpa!" : (level >= MSG_INFO ? "wpa:" : "wpa "), fmt, ap2);
		va_end (ap2);
	}

	if (level >= MSG_INFO)
	{
		vprintf (fmt, ap);
	}
}

/* Lo que escribe el controlador del chip (una línea) */
void zxmc_wlan_log (const char *text)
{
	if (zxmc_wpa_log_on)
		zxmc_log ("chip", "%s", text);
}

/* Un evento del firmware del chip */
void zxmc_wlan_event (const char *name, long event, long status, int flags, long reason)
{
	if (!zxmc_wpa_log_on)
		return;
	if (event == 69 && status == 8)		/* E_ESCAN_RESULT parcial (una red) */
		return;
	zxmc_log ("chip", "evento %s (%ld): estado %ld, motivo %ld, flags 0x%x",
		  name != NULL ? name : "?", event, status, reason, flags);
}

/* Pasa lo apuntado a pBuffer (hasta nSize) y lo deja vacío; *pLost: las
   líneas que no cupieron desde la vez anterior */
unsigned zxmc_wpa_log_take (char *pBuffer, unsigned nSize, unsigned *pLost)
{
	unsigned n = zxmc_wpa_log_len;
	if (n > nSize)
		n = nSize;
	os_memcpy (pBuffer, zxmc_wpa_log, n);
	if (n < zxmc_wpa_log_len)
		os_memmove (zxmc_wpa_log, zxmc_wpa_log + n, zxmc_wpa_log_len - n);
	zxmc_wpa_log_len -= n;
	*pLost = zxmc_wpa_log_lost;
	zxmc_wpa_log_lost = 0;
	return n;
}
