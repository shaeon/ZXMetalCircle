//
// settings.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "settings.h"
#include "i18n.h"
#include <circle/util.h>

static const char *PageTitles[] =
{
	N_("Máquina"),
	N_("Imagen y sonido"),
	N_("Cintas y grabación"),
	N_("Teclado y mandos"),
	N_("Periféricos"),
	N_("Red, hora y casete"),
	N_("ROMs y carpetas"),
	N_("Servidor web")
};

// Etiquetas y nombres en español con N_ (): se traducen al dibujarlos. Las
// listas de nombres se traducen enteras ("sí|no" -> "yes|no").
static TSettingDef Settings[] =
{
	// 0 Máquina
	{"lang",	N_("Idioma"),		0, "es", "Español",					"es",	TRUE},
	{"model",	N_("Modelo"),		0, "16k|48k|128k|plus2|plus2a|plus3|plus3e",
					   "16K|48K|128K|+2|+2A|+3|+3e",			"128k",	TRUE},
	{"issue",	N_("Teclado"),		0, "3|2", "Issue 3|Issue 2",				"3",	TRUE},
	{"speed",	N_("Velocidad"),	0, "1|2|4|10", "x1|x2|x4|x10",				"1",	TRUE},
	{"ulaplus",	N_("ULAplus (64 colores)"), 0, "1|0", N_("sí|no"),			"1",	TRUE},
	{"p3elang",	N_("Idioma del +3e"),	0, "es|en", N_("español|inglés"),		"es",	FALSE},
	{"hdd",		N_("Disco duro al inicio"), 0, "none", N_("ninguno"),			"none",	FALSE},

	// 1 Imagen y sonido
	{"border",	N_("Borde"),		1, "none|standard|full|huge", N_("sin borde|estándar|completo|enorme"), "full", TRUE},
	{"scanlines",	N_("Scanlines"),	1, "0|1", N_("no|sí"),					"0",	TRUE},
	{"pureblack",	N_("Negro puro (paleta 48K)"), 1, "0|1", N_("no|sí"),			"0",	TRUE},
	{"aystereo",	N_("Sonido AY"),	1, "abc|acb|bac|mono", "ABC|ACB|BAC|mono",		"abc",	TRUE},
	{"tapesound",	N_("Oír la carga"),	1, "1|0", N_("sí|no"),					"1",	TRUE},
	{"speaker",	N_("Altavoz"),		1, "auto|beeper|tv|off", N_("automático|el del 48K|televisor|sin modelo"), "auto", TRUE},
	{"beeperfilter", N_("Quitar pitido 1 bit"), 1, "1|0", N_("sí|no"),			"1",	TRUE},
	{"sounddev",	N_("Salida de sonido"),	1, "sndhdmi|sndpwm|sndusb|none", N_("HDMI|jack (PWM)|USB|ninguna"), "sndhdmi", FALSE},
	{"gpioaudio",	N_("Sonido por GPIO12"), 1, "1|0", N_("sí|no"),				"1",	FALSE},
	{"multicore",	N_("Imagen en el núcleo 1"), 1, "1|0", N_("sí|no"),			"1",	FALSE},

	// 2 Cintas y grabación
	{"tape",	N_("Carga de cintas"),	2, "flash|fast|normal", N_("instantánea|rápida|real"), "flash", TRUE},
	{"tapeaccel",	N_("Acelerar cargadores"), 2, "1|0", N_("sí|no"),				"1",	TRUE},
	{"savetrap",	N_("SAVE a la SD"),	2, "1|0", N_("sí|no"),					"1",	TRUE},
	{"savefmt",	N_("Formato de SAVE"),	2, "tap|tzx", "TAP|TZX",				"tap",	TRUE},
	{"snapformat",	N_("Formato de F8"),	2, "z80|sna|szx", "Z80|SNA|SZX",			"z80",	TRUE},

	// 3 Teclado y mandos
	{"joystick",	N_("Joystick"),		3, "none|kempston|fuller|sinclair1|sinclair2|cursor|qaop",
				   N_("ninguno|Kempston|Fuller|Sinclair 1|Sinclair 2|cursor|QAOP"), "kempston", TRUE},
	{"cursors",	N_("Cursores del PC"),	3, "both|keys|joystick", N_("teclas y joystick|teclas|joystick"), "both", TRUE},
	{"f5",		N_("Tecla F5 (Alt+F5: la otra)"), 3, "nmi|files", N_("NMI / Multiface|ficheros"),	"nmi",	TRUE},
	{"recreated",	N_("Recreated ZX"),	3, "0|auto|1", N_("no|automático|sí"),			"auto",	TRUE},
	{"gpiokbd",	N_("Teclado por GPIO"),	3, "48|plus3|0", N_("48K/128|+2A/+3|ninguno"),		"48",	FALSE},

	// 4 Periféricos
	{"spectranet",	N_("Spectranet"),	4, "0|1", N_("no|sí"),					"0",	TRUE},
	{"divmmc",	N_("divMMC con esxDOS"), 4, "0|esxdos", N_("no|sí"),				"0",	TRUE},
	{"lec",		N_("Memoria LEC"),	4, "0|1", N_("no|sí"),					"0",	TRUE},
	{"if1",		N_("Interface 1"),	4, "auto|0|1", N_("al usarlo|nunca|siempre"),		"auto",	TRUE},
	{"multiface",	N_("Multiface"),	4, "1|0", N_("sí|no"),					"1",	TRUE},
	{"rs232",	N_("RS232 (TCP 2323)"),	4, "1|0", N_("sí|no"),					"1",	FALSE},
	{"zxnet",	N_("Red ZX Net (UDP 5050)"), 4, "1|0", N_("sí|no"),				"1",	FALSE},

	// 5 Red, hora y casete
	{"net",		N_("Red"),		5, "auto|wifi|eth|0", N_("automática|WiFi|cable|no"),	"auto",	FALSE},
	{"tz",		N_("Zona horaria"),	5, "-480|-300|-240|-180|0|60|120|180|240|330|480|540|600",
				   "UTC-8|UTC-5|UTC-4|UTC-3|UTC|UTC+1|UTC+2|UTC+3|UTC+4|UTC+5:30|UTC+8|UTC+9|UTC+10", "60", FALSE},
	{"dst",		N_("Horario de verano"), 5, "1|0", N_("sí|no"),				"1",	FALSE},
	{"ip",		N_("IP fija"),		5, "*ip", N_("no (DHCP)"),				"",	FALSE},
	{"mask",	N_("Máscara de red"),	5, "*ip", "255.255.255.0",				"",	FALSE},
	{"gw",		N_("Puerta de enlace"),	5, "*ip", N_("automática"),			"",	FALSE},
	{"dns",		N_("Servidor DNS"),	5, "*ip", N_("automático"),			"",	FALSE},
	{"tlsverify",	N_("Comprobar HTTPS"),	5, "1|0", N_("sí|no"),					"1",	FALSE},
	{"ear",		N_("Casete real"),	5, "0|gpio|usb", N_("no|GPIO|USB"),			"0",	FALSE},
	{"earinvert",	N_("Invertir casete"),	5, "0|1", N_("no|sí"),					"0",	FALSE},
	{"earpin",	N_("GPIO del casete"),	5, "auto|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27",
				   N_("automático|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27"), "auto", FALSE},

	// 6 ROMs y carpetas (las listas se rellenan al arrancar con lo que hay
	// en roms/ con el tamaño de cada una, y con las carpetas de la SD)
	{"rom48",	N_("ROM 16K/48K"),	6, "default", N_("la incluida"),			"default", FALSE},
	{"rom128",	N_("ROM 128K"),		6, "default", N_("la incluida"),			"default", FALSE},
	{"romplus2",	N_("ROM +2"),		6, "default", "plus2.rom",				"default", FALSE},
	{"romplus2a",	N_("ROM +2A"),		6, "default", "plus2a.rom",				"default", FALSE},
	{"romplus3",	N_("ROM +3"),		6, "default", "plus3.rom",				"default", FALSE},
	{"romplus3e",	N_("ROM +3e"),		6, "default", N_("según el idioma"),			"default", FALSE},
	{"romif1",	N_("ROM Interface 1"),	6, "default", "if1.rom",				"default", FALSE},
	{"romesx",	N_("ROM de esxDOS"),	6, "default", "esxmmc.bin",				"default", FALSE},
	{"romsnet",	N_("Flash Spectranet"),	6, "default", "spectranet.rom",				"default", FALSE},
	{"esxdir",	N_("Importar a esxDOS desde"), 6, "esxdos_i|0", N_("esxdos_i|no"),		"esxdos_i", FALSE},
	{"web",		N_("Página web"),	7, "1|ro|0", N_("sí|solo lectura|no"),			"1",	FALSE},
	{"webpass",	N_("Contraseña de la web"),	7, "*pass", N_("sin contraseña"),			"",	FALSE},
};

#define NUM_SETTINGS	(sizeof Settings / sizeof Settings[0])

// n-ésimo elemento de una lista "a|b|c" (copiado en pOut)
static boolean GetItem (const char *pList, unsigned n, char *pOut, unsigned nSize)
{
	const char *p = pList;
	for (unsigned i = 0; i < n; i++)
	{
		while (*p != '\0' && *p != '|')
		{
			p++;
		}
		if (*p == '\0')
		{
			return FALSE;
		}
		p++;
	}
	unsigned k = 0;
	while (*p != '\0' && *p != '|' && k < nSize - 1)
	{
		pOut[k++] = *p++;
	}
	pOut[k] = '\0';
	return TRUE;
}

static int IndexOf (const char *pList, const char *pValue)
{
	char Item[SETTING_VALUE_LEN];
	for (unsigned i = 0; GetItem (pList, i, Item, sizeof Item); i++)
	{
		if (strcasecmp (Item, pValue) == 0)
		{
			return i;
		}
	}
	return -1;
}

CSettings::CSettings (void)
:	m_bDirty (FALSE)
{
	for (unsigned i = 0; i < NUM_SETTINGS; i++)
	{
		strcpy (m_Value[i], Settings[i].pDefault);
	}
}

unsigned CSettings::GetPageCount (void)
{
	return sizeof PageTitles / sizeof PageTitles[0];
}

const char *CSettings::GetPageTitle (unsigned nPage)
{
	return nPage < GetPageCount () ? T (PageTitles[nPage]) : "";
}

// Valores que se conocen al arrancar (idiomas de SD:/locales)
void CSettings::SetValueList (const char *pKey, const char *pValues, const char *pNames)
{
	for (unsigned n = 0; n < sizeof Settings / sizeof Settings[0]; n++)
	{
		if (strcmp (Settings[n].pKey, pKey) == 0)
		{
			Settings[n].pValues = pValues;
			Settings[n].pNames = pNames;
		}
	}
}

unsigned CSettings::GetCount (void) const
{
	return NUM_SETTINGS;
}

const TSettingDef &CSettings::GetDef (unsigned nIndex) const
{
	return Settings[nIndex];
}

int CSettings::Find (const char *pKey) const
{
	for (unsigned i = 0; i < NUM_SETTINGS; i++)
	{
		if (strcasecmp (Settings[i].pKey, pKey) == 0)
		{
			return i;
		}
	}
	return -1;
}

boolean CSettings::IsEditable (unsigned nIndex)
{
	return    nIndex < NUM_SETTINGS
	       && (strcmp (Settings[nIndex].pValues, "*ip") == 0 || IsPassword (nIndex));
}

// Opción de texto libre (contraseña): letras minúsculas y números
boolean CSettings::IsPassword (unsigned nIndex)
{
	return nIndex < NUM_SETTINGS && strcmp (Settings[nIndex].pValues, "*pass") == 0;
}

// a.b.c.d con cada parte de 0 a 255 (o vacío)
boolean CSettings::IsValidIP (const char *p)
{
	if (*p == '\0')
	{
		return TRUE;
	}
	for (unsigned nPart = 0; nPart < 4; nPart++)
	{
		unsigned nValue = 0, nDigits = 0;
		while (*p >= '0' && *p <= '9')
		{
			nValue = nValue * 10 + (*p++ - '0');
			if (++nDigits > 3)
			{
				return FALSE;
			}
		}
		if (nDigits == 0 || nValue > 255)
		{
			return FALSE;
		}
		if (nPart < 3 && *p++ != '.')
		{
			return FALSE;
		}
	}
	return *p == '\0';
}

const char *CSettings::GetValueName (unsigned nIndex) const
{
	static char Name[64];
	if (IsPassword (nIndex) && m_Value[nIndex][0] != '\0')
	{
		return "********";		// la contraseña no se ve en pantalla
	}
	if (IsEditable (nIndex))
	{
		return m_Value[nIndex][0] != '\0' ? m_Value[nIndex] : T (Settings[nIndex].pNames);
	}
	int n = IndexOf (Settings[nIndex].pValues, m_Value[nIndex]);
	if (n < 0 || !GetItem (T (Settings[nIndex].pNames), n, Name, sizeof Name))
	{
		return m_Value[nIndex];		// un valor que no está en la lista
	}
	return Name;
}

void CSettings::SetValue (unsigned nIndex, const char *pValue)
{
	if (nIndex >= NUM_SETTINGS)
	{
		return;
	}

	// sinónimos de cmdline.txt (+3, 1 para gpiokbd...)
	int n = IndexOf (Settings[nIndex].pValues, pValue);
	if (n < 0)
	{
		if (strcmp (Settings[nIndex].pKey, "model") == 0)
		{
			if (strcasecmp (pValue, "+2") == 0)		pValue = "plus2";
			else if (strcasecmp (pValue, "+2a") == 0)	pValue = "plus2a";
			else if (strcasecmp (pValue, "+3") == 0)	pValue = "plus3";
			else if (strcasecmp (pValue, "+3e") == 0)	pValue = "plus3e";
		}
		else if (strcmp (Settings[nIndex].pKey, "gpiokbd") == 0)
		{
			if (strcmp (pValue, "1") == 0 || strcmp (pValue, "128") == 0)		pValue = "48";
			else if (strcasecmp (pValue, "off") == 0)				pValue = "0";
			else if (   strcasecmp (pValue, "plus2a") == 0 || strcasecmp (pValue, "+3") == 0
				 || strcasecmp (pValue, "+2a") == 0)				pValue = "plus3";
		}
		else if (strcmp (Settings[nIndex].pKey, "net") == 0 && strcasecmp (pValue, "off") == 0)
		{
			pValue = "0";
		}
		else if (strcmp (Settings[nIndex].pKey, "hdd") == 0 && (*pValue == '\0' || strcmp (pValue, "0") == 0))
		{
			pValue = "none";
		}
		else if (strncmp (Settings[nIndex].pKey, "rom", 3) == 0 && *pValue == '\0')
		{
			pValue = "default";
		}
	}

	strncpy (m_Value[nIndex], pValue, SETTING_VALUE_LEN - 1);
	m_Value[nIndex][SETTING_VALUE_LEN - 1] = '\0';
}

void CSettings::SetValue (const char *pKey, const char *pValue)
{
	int n = Find (pKey);
	if (n >= 0)
	{
		SetValue (n, pValue);
		m_bDirty = TRUE;
	}
}

void CSettings::Step (unsigned nIndex, int nDirection)
{
	if (IsEditable (nIndex))
	{
		return;				// se escriben (ver CMenu)
	}
	const char *pList = Settings[nIndex].pValues;
	unsigned nCount = 1;
	for (const char *p = pList; *p != '\0'; p++)
	{
		if (*p == '|')
		{
			nCount++;
		}
	}

	int n = IndexOf (pList, m_Value[nIndex]);
	if (n < 0)
	{
		n = 0;
	}
	else
	{
		n = (n + nCount + (nDirection < 0 ? -1 : 1)) % nCount;
	}

	GetItem (pList, n, m_Value[nIndex], SETTING_VALUE_LEN);
	m_bDirty = TRUE;
}

void CSettings::Merge (const char *pOld, char *pOut, unsigned nOutSize) const
{
	boolean Done[NUM_SETTINGS];
	for (unsigned i = 0; i < NUM_SETTINGS; i++)
	{
		Done[i] = FALSE;
	}

	unsigned n = 0;
	pOut[0] = '\0';

	// Opciones que ya están en el fichero, en su orden (solo la 1.ª línea)
	const char *p = pOld;
	for (;;)
	{
		while (*p == ' ' || *p == '\t')
		{
			p++;
		}
		if (*p == '\0' || *p == '\r' || *p == '\n')
		{
			break;
		}
		const char *pToken = p;
		while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
		{
			p++;
		}
		unsigned nToken = p - pToken;

		const char *pEq = pToken;
		while (pEq < p && *pEq != '=')
		{
			pEq++;
		}
		unsigned nName = pEq - pToken;

		char Item[160];
		Item[0] = '\0';
		for (unsigned i = 0; i < NUM_SETTINGS; i++)
		{
			if (   strlen (Settings[i].pKey) == nName
			    && strncasecmp (Settings[i].pKey, pToken, nName) == 0)
			{
				if (!Done[i])
				{
					strcpy (Item, Settings[i].pKey);
					strcat (Item, "=");
					strcat (Item, m_Value[i]);
					Done[i] = TRUE;
				}
				nToken = 0;		// (repetida: se quita)
				break;
			}
		}
		if (nToken > 0 && nToken < sizeof Item)
		{
			memcpy (Item, pToken, nToken);
			Item[nToken] = '\0';
		}

		if (Item[0] != '\0' && n + strlen (Item) + 2 < nOutSize)
		{
			if (n > 0)
			{
				pOut[n++] = ' ';
			}
			strcpy (pOut + n, Item);
			n += strlen (Item);
		}
	}

	// Opciones de la tabla que no estaban y no tienen el valor por defecto
	for (unsigned i = 0; i < NUM_SETTINGS; i++)
	{
		if (Done[i] || strcasecmp (m_Value[i], Settings[i].pDefault) == 0)
		{
			continue;
		}
		char Item[160];
		strcpy (Item, Settings[i].pKey);
		strcat (Item, "=");
		strcat (Item, m_Value[i]);
		if (n + strlen (Item) + 2 < nOutSize)
		{
			if (n > 0)
			{
				pOut[n++] = ' ';
			}
			strcpy (pOut + n, Item);
			n += strlen (Item);
		}
	}

	if (n + 1 < nOutSize)
	{
		pOut[n++] = '\n';
	}
	pOut[n] = '\0';
}

// Reglas de las opciones que solo se usan con un modelo o con otra opción.
// Se mira lo elegido en la propia configuración (aunque no esté guardado).
boolean CSettings::IsApplicable (unsigned nIndex) const
{
	if (nIndex >= GetCount ())
	{
		return TRUE;
	}

	const char *pKey = GetDef (nIndex).pKey;
	auto Is = [this] (const char *pOther, const char *pValue) -> boolean
	{
		int n = Find (pOther);
		return n >= 0 && strcmp (GetValue (n), pValue) == 0;
	};

	// el teclado Issue 2/3 solo cambia algo en el 16K y el 48K
	if (strcmp (pKey, "issue") == 0)
	{
		return Is ("model", "16k") || Is ("model", "48k");
	}
	// idioma y disco duro del +3e
	if (strcmp (pKey, "p3elang") == 0 || strcmp (pKey, "hdd") == 0)
	{
		return Is ("model", "plus3e");
	}
	// la aceleración de cargadores no actúa en la carga real
	if (strcmp (pKey, "tapeaccel") == 0)
	{
		return !Is ("tape", "normal");
	}
	// el formato de SAVE solo importa si SAVE va a la SD
	if (strcmp (pKey, "savefmt") == 0)
	{
		return Is ("savetrap", "1");
	}
	// sonido por el GPIO12: solo con la salida PWM, en las Pi 1 a 3 (en la 4 el
	// PWM del jack no llega al conector; en la 5 el PWM ya sale por el GPIO12 y
	// el GPIO13) y sin el teclado del +2A/+3, que usa el GPIO12 como fila. Ver
	// GPIO.md.
	if (strcmp (pKey, "gpioaudio") == 0)
	{
#if RASPPI <= 3
		return Is ("sounddev", "sndpwm") && !Is ("gpiokbd", "plus3");
#else
		return FALSE;
#endif
	}
	// la imagen en el núcleo 1: solo en las Pi con varios núcleos (2, 3 y 4)
	if (strcmp (pKey, "multicore") == 0)
	{
#ifdef ARM_ALLOW_MULTI_CORE
		return TRUE;
#else
		return FALSE;
#endif
	}
	// red: con la IP automática (DHCP) no se usan máscara, puerta ni DNS
	if (strcmp (pKey, "mask") == 0 || strcmp (pKey, "gw") == 0 || strcmp (pKey, "dns") == 0)
	{
		return !Is ("ip", "") && !Is ("net", "0");
	}
	if (   strcmp (pKey, "ip") == 0 || strcmp (pKey, "tz") == 0 || strcmp (pKey, "dst") == 0
	    || strcmp (pKey, "tlsverify") == 0)
	{
		return !Is ("net", "0");
	}
	// casete real
	if (strcmp (pKey, "earinvert") == 0)
	{
		return !Is ("ear", "0");
	}
	if (strcmp (pKey, "earpin") == 0)
	{
		return Is ("ear", "gpio");
	}
	// negro puro: solo cambia la paleta del 16K/48K (la de los demás ya lo es)
	if (strcmp (pKey, "pureblack") == 0)
	{
		return Is ("model", "16k") || Is ("model", "48k");
	}
	// contraseña de la web: solo con la web y la red
	if (strcmp (pKey, "webpass") == 0)
	{
		return !Is ("web", "0") && !Is ("net", "0");
	}
	if (strcmp (pKey, "web") == 0)
	{
		return !Is ("net", "0");
	}
	// carpeta de esxDOS: solo con la divMMC
	if (strcmp (pKey, "esxdir") == 0)
	{
		return Is ("divmmc", "esxdos");
	}

	return TRUE;
}
