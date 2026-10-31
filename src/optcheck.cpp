//
// optcheck.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "optcheck.h"
#include "i18n.h"
#include <circle/util.h>

struct TOptionDef
{
	const char *pName;
	const char *pValues;	// valores admitidos separados por |; 0 = cualquiera
};

// Opciones del emulador (cmdline.hlp) y de Circle
static const TOptionDef Options[] =
{
	{"model",	"16k|48k|128k|plus2|+2|plus2a|+2a|plus3|+3|plus3e|+3e"},
	{"tape",	"flash|fast|normal"},
	{"tapeaccel",	"0|1"},
	{"diskaccel",	"0|1"},
	{"joystick",	"none|kempston|fuller|sinclair1|sinclair2|cursor|qaop"},
	{"cursors",	"both|keys|joystick"},
	{"f5",		"nmi|files"},
	{"recreated",	"0|1|auto"},
	{"multiface",	"0|1"},
	{"gpiokbd",	"0|off|1|48|128|plus3|plus2a|+3|+2a"},
	{"gpioaudio",	"0|1"},
	{"ulaplus",	"0|1"},
	{"pureblack",	"0|1"},
	{"aystereo",	"abc|acb|bac|mono"},
	{"tapesound",	"0|1"},
	{"beeperfilter", "0|1"},
	{"speaker",	"auto|beeper|tv|off"},
	{"issue",	"2|3"},
	{"speed",	"1|2|4|10"},
	{"border",	"none|standard|full|huge"},
	{"scanlines",	"0|1"},
	{"if1",		"auto|0|1"},
	{"rs232",	"0|1"},
	{"zxnet",	"0|1"},
	{"lec",		"0|1"},
	{"savetrap",	"0|1"},
	{"savefmt",	"tap|tzx"},
	{"snapformat",	"z80|sna|szx"},
	{"net",		"auto|wifi|eth|0"},
	{"tz",		0},
	{"dst",		"0|1"},
	{"ip",		0},
	{"mask",	0},
	{"gw",		0},
	{"dns",		0},
	{"tlsverify",	"0|1"},
	{"hdd",		0},
	{"p3elang",	"es|en"},
	{"divmmc",	"0|esxdos"},
	{"spectranet",	"0|1"},
	{"lang",	0},
	{"multicore",	"0|1"},
	{"esxdir",	0},
	{"web",		0},
	{"webpass",	0},
	{"romsnet",	0},
	{"ear",		"0|gpio|usb"},
	{"earpin",	0},
	{"earinvert",	"0|1"},
	{"rom48",	0},
	{"rom128",	0},
	{"romplus2",	0},
	{"romplus2a",	0},
	{"romplus3",	0},
	{"romplus3e",	0},
	{"romif1",	0},
	{"romesx",	0},
	{"sounddev",	"sndhdmi|sndpwm|sndusb|none"},
	// de Circle
	{"logdev",	0},
	{"loglevel",	0},
	{"width",	0},
	{"height",	0},
	{"keymap",	0},
	{"usbpowerdelay", 0},
	{"usbspeed",	0},
	{"usbboost",	0},
	{"usbignore",	0},
	{"usbsoundchannels", 0},
	{"soundopt",	0},
	{"fast",	0},
	{"socmaxtemp",	0},
	{"gpiofanpin",	0},
	{"touchscreen",	0},
	{"backlight",	0},
};

static boolean ValueAllowed (const char *pValues, const char *pValue, unsigned nLen)
{
	const char *p = pValues;
	while (*p != '\0')
	{
		const char *pEnd = p;
		while (*pEnd != '\0' && *pEnd != '|')
		{
			pEnd++;
		}
		if ((unsigned) (pEnd - p) == nLen && strncasecmp (p, pValue, nLen) == 0)
		{
			return TRUE;
		}
		p = *pEnd == '|' ? pEnd + 1 : pEnd;
	}
	return FALSE;
}

unsigned CheckOptions (const char *pCmdLine, char *pMessage, unsigned nSize, TOptionLogger *pLog)
{
	unsigned nProblems = 0;
	pMessage[0] = '\0';

	const char *p = pCmdLine;
	for (;;)
	{
		while (*p == ' ' || *p == '\t')
		{
			p++;
		}
		if (*p == '\0' || *p == '\r' || *p == '\n')
		{
			// solo cuenta la primera línea, como hace la Raspberry: si hay más
			// texto debajo, esas opciones no se están aplicando
			const char *pRest = p;
			while (*pRest == '\r' || *pRest == '\n' || *pRest == ' ' || *pRest == '\t')
			{
				pRest++;
			}
			if (*pRest != '\0')
			{
				const char *pProblem = T ("cmdline.txt: solo vale la 1.ª línea");
				if (nProblems == 0)
				{
					strncpy (pMessage, pProblem, nSize - 1);
					pMessage[nSize - 1] = '\0';
				}
				nProblems++;
				if (pLog != 0)
				{
					pLog (pProblem);
				}
			}
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
		const char *pValue = pEq < p ? pEq + 1 : p;
		unsigned nValue = p - pValue;

		char Problem[80];
		Problem[0] = '\0';

		const TOptionDef *pDef = 0;
		for (unsigned i = 0; i < sizeof Options / sizeof Options[0]; i++)
		{
			if (strlen (Options[i].pName) == nName && strncasecmp (Options[i].pName, pToken, nName) == 0)
			{
				pDef = &Options[i];
				break;
			}
		}

		char Token[48];
		unsigned nCopy = nToken < sizeof Token - 1 ? nToken : sizeof Token - 1;
		memcpy (Token, pToken, nCopy);
		Token[nCopy] = '\0';

		if (pDef == 0)
		{
			strcpy (Problem, T ("opción desconocida: "));
			strncat (Problem, Token, sizeof Problem - strlen (Problem) - 1);
		}
		else if (pDef->pValues != 0 && !ValueAllowed (pDef->pValues, pValue, nValue))
		{
			strcpy (Problem, Token);
			strncat (Problem, T (" no válido"), sizeof Problem - strlen (Problem) - 1);
		}

		if (Problem[0] != '\0')
		{
			if (nProblems == 0)
			{
				strncpy (pMessage, Problem, nSize - 1);
				pMessage[nSize - 1] = '\0';
			}
			nProblems++;
			if (pLog != 0)
			{
				pLog (Problem);
			}
		}
	}

	return nProblems;
}
