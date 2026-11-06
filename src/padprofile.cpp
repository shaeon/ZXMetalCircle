//
// padprofile.cpp
//
// Perfil de un mando: qué hace cada botón (ver padprofile.h)
//
#include "padprofile.h"
#include "zxkeyboard.h"
#include <string.h>

// --- Textos -----------------------------------------------------------------

static char Lower (char c)	{ return c >= 'A' && c <= 'Z' ? c + 32 : c; }

static boolean Equal (const char *a, const char *b)
{
	for (; *a != '\0' && *b != '\0'; a++, b++)
	{
		if (Lower (*a) != Lower (*b))
		{
			return FALSE;
		}
	}
	return *a == *b;
}

// ¿Empieza por pPrefix (sin distinguir mayúsculas)? Devuelve lo que sigue, o 0
static const char *After (const char *p, const char *pPrefix)
{
	for (; *pPrefix != '\0'; p++, pPrefix++)
	{
		if (Lower (*p) != Lower (*pPrefix))
		{
			return 0;
		}
	}
	return p;
}

static void Append (char *pDest, unsigned nSize, const char *pText)
{
	unsigned n = strlen (pDest);
	while (*pText != '\0' && n + 1 < nSize)
	{
		pDest[n++] = *pText++;
	}
	pDest[n] = '\0';
}

// --- Botones ----------------------------------------------------------------

static const char *KnownButtons[PAD_BUTTONS] =
{
	"Guia", 0, 0, "LT", "RT", "LB", "RB", "Y", "B", "A", "X", "Select", "L3", "R3", "Start",
	"Arriba", "Derecha", "Abajo", "Izquierda", "Plus", "Minus", "Touchpad"
};

const char *CPadProfile::ButtonName (unsigned nButton, boolean bKnown)
{
	if (nButton >= PAD_BUTTONS)
	{
		return 0;
	}
	if (bKnown)
	{
		return KnownButtons[nButton];
	}
	static const char *Numbers[PAD_BUTTONS] =
	{
		"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16",
		"17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32"
	};
	return Numbers[nButton];
}

int CPadProfile::ButtonFromName (const char *pName, boolean bKnown)
{
	static const struct { const char *pAlias; const char *pName; } Aliases[] =
	{
		{"Up", "Arriba"}, {"Down", "Abajo"}, {"Left", "Izquierda"}, {"Right", "Derecha"},
		{"Guide", "Guia"}, {"Home", "Guia"}, {"L1", "LB"}, {"R1", "RB"}, {"L2", "LT"}, {"R2", "RT"},
		{"Cruz", "A"}, {"Cross", "A"}, {"Circulo", "B"}, {"Circle", "B"}, {"Cuadrado", "X"},
		{"Square", "X"}, {"Triangulo", "Y"}, {"Triangle", "Y"}, {"Back", "Select"}, {"Share", "Select"},
		{"Options", "Start"}
	};
	if (bKnown)
	{
		for (unsigned i = 0; i < sizeof Aliases / sizeof Aliases[0]; i++)
		{
			if (Equal (pName, Aliases[i].pAlias))
			{
				pName = Aliases[i].pName;
			}
		}
	}
	for (unsigned i = 0; i < PAD_BUTTONS; i++)
	{
		const char *p = ButtonName (i, bKnown);
		if (p != 0 && Equal (pName, p))
		{
			return i;
		}
	}
	return -1;
}

// --- Teclas del Spectrum -----------------------------------------------------

// Matriz: fila 0 CS Z X C V, 1 A S D F G, 2 Q W E R T, 3 1 2 3 4 5,
// 4 0 9 8 7 6, 5 P O I U Y, 6 ENTER L K J H, 7 ESPACIO SS M N B
static const char *ZXRows[8] = {"#ZXCV", "ASDFG", "QWERT", "12345", "09876", "POIUY", "#LKJH", "##MNB"};
#define ZX_POS(row, bit)	((row) << 3 | (bit))
#define ZX_CS			ZX_POS (0, 0)
#define ZX_SS			ZX_POS (7, 1)

static int ZXKeyFromName (const char *p)
{
	if (Equal (p, "ENTER"))					return ZX_POS (6, 0);
	if (Equal (p, "ESPACIO") || Equal (p, "SPACE"))		return ZX_POS (7, 0);
	if (Equal (p, "CS"))					return ZX_CS;
	if (Equal (p, "SS"))					return ZX_SS;
	if (p[0] != '\0' && p[1] == '\0')
	{
		char c = p[0] >= 'a' && p[0] <= 'z' ? p[0] - 32 : p[0];
		for (unsigned nRow = 0; nRow < 8; nRow++)
		{
			for (unsigned nBit = 0; nBit < 5; nBit++)
			{
				if (ZXRows[nRow][nBit] == c && c != '#')
				{
					return ZX_POS (nRow, nBit);
				}
			}
		}
	}
	return -1;
}

static void ZXKeyName (u8 nPos, char *pBuffer, unsigned nSize)
{
	const char *p = 0;
	char One[2] = {0, 0};
	if (nPos == ZX_POS (6, 0))		p = "ENTER";
	else if (nPos == ZX_POS (7, 0))		p = "ESPACIO";
	else if (nPos == ZX_CS)			p = "CS";
	else if (nPos == ZX_SS)			p = "SS";
	else if ((nPos >> 3) < 8 && (nPos & 7) < 5)
	{
		One[0] = ZXRows[nPos >> 3][nPos & 7];
		p = One;
	}
	Append (pBuffer, nSize, p != 0 ? p : "?");
}

// --- Teclas del emulador (HID) -----------------------------------------------

static const struct { const char *pName; u8 nHID; } PCKeys[] =
{
	{"Enter", 0x28}, {"Esc", 0x29}, {"Retroceso", 0x2A}, {"Backspace", 0x2A}, {"Tab", 0x2B},
	{"Espacio", 0x2C}, {"Space", 0x2C}, {"Pausa", 0x48}, {"Pause", 0x48}, {"Inicio", 0x4A},
	{"Home", 0x4A}, {"RePag", 0x4B}, {"PgUp", 0x4B}, {"Supr", 0x4C}, {"Delete", 0x4C},
	{"Fin", 0x4D}, {"End", 0x4D}, {"AvPag", 0x4E}, {"PgDn", 0x4E}, {"Derecha", 0x4F},
	{"Right", 0x4F}, {"Izquierda", 0x50}, {"Left", 0x50}, {"Abajo", 0x51}, {"Down", 0x51},
	{"Arriba", 0x52}, {"Up", 0x52}
};

static int PCKeyFromName (const char *p)
{
	if ((p[0] == 'F' || p[0] == 'f') && p[1] >= '1' && p[1] <= '9')
	{
		int n = p[1] - '0';
		if (p[2] >= '0' && p[2] <= '2' && p[1] == '1' && p[3] == '\0')
		{
			n = 10 + p[2] - '0';
		}
		else if (p[2] != '\0')
		{
			n = 0;
		}
		if (n >= 1 && n <= 12)
		{
			return 0x3A + n - 1;			// F1-F12
		}
	}
	for (unsigned i = 0; i < sizeof PCKeys / sizeof PCKeys[0]; i++)
	{
		if (Equal (p, PCKeys[i].pName))
		{
			return PCKeys[i].nHID;
		}
	}
	if (p[0] != '\0' && p[1] == '\0')
	{
		char c = Lower (p[0]);
		if (c >= 'a' && c <= 'z')	return 0x04 + c - 'a';
		if (c >= '1' && c <= '9')	return 0x1E + c - '1';
		if (c == '0')			return 0x27;
	}
	return -1;
}

static void PCKeyName (u8 nHID, char *pBuffer, unsigned nSize)
{
	char Text[4] = {0, 0, 0, 0};
	if (nHID >= 0x3A && nHID <= 0x45)
	{
		unsigned n = nHID - 0x3A + 1;
		Text[0] = 'F';
		Text[1] = n >= 10 ? '1' : '0' + n;
		Text[2] = n >= 10 ? '0' + n - 10 : '\0';
		Append (pBuffer, nSize, Text);
		return;
	}
	if (nHID >= 0x04 && nHID <= 0x1D)	{ Text[0] = 'A' + nHID - 0x04; Append (pBuffer, nSize, Text); return; }
	if (nHID >= 0x1E && nHID <= 0x26)	{ Text[0] = '1' + nHID - 0x1E; Append (pBuffer, nSize, Text); return; }
	if (nHID == 0x27)			{ Append (pBuffer, nSize, "0"); return; }
	for (unsigned i = 0; i < sizeof PCKeys / sizeof PCKeys[0]; i += 1)
	{
		if (PCKeys[i].nHID == nHID)
		{
			Append (pBuffer, nSize, PCKeys[i].pName);	// el primero: en español
			return;
		}
	}
	Append (pBuffer, nSize, "?");
}

// --- Acciones -----------------------------------------------------------------

boolean CPadProfile::ParseAction (const char *pText, TPadAction *pAction)
{
	while (*pText == ' ' || *pText == '\t')
	{
		pText++;
	}
	pAction->Type = PadActNone;
	pAction->nArg = 0;
	pAction->nArg2 = PAD_NO_KEY;

	static const struct { const char *pName; u8 Type; u8 nArg; } Simple[] =
	{
		{"ninguno", PadActNone, 0}, {"none", PadActNone, 0},
		{"disparo", PadActJoy, JOY_FIRE}, {"fire", PadActJoy, JOY_FIRE},
		{"disparo 2", PadActJoy, JOY_FIRE2}, {"disparo2", PadActJoy, JOY_FIRE2}, {"fire 2", PadActJoy, JOY_FIRE2},
		{"fire2", PadActJoy, JOY_FIRE2},
		{"arriba", PadActJoy, JOY_UP}, {"up", PadActJoy, JOY_UP},
		{"abajo", PadActJoy, JOY_DOWN}, {"down", PadActJoy, JOY_DOWN},
		{"izquierda", PadActJoy, JOY_LEFT}, {"left", PadActJoy, JOY_LEFT},
		{"derecha", PadActJoy, JOY_RIGHT}, {"right", PadActJoy, JOY_RIGHT},
		{"joystick", PadActNextJoy, 0}, {"menu", PadActMenu, 0}, {"menú", PadActMenu, 0}
	};
	for (unsigned i = 0; i < sizeof Simple / sizeof Simple[0]; i++)
	{
		if (Equal (pText, Simple[i].pName))
		{
			pAction->Type = Simple[i].Type;
			pAction->nArg = Simple[i].nArg;
			return TRUE;
		}
	}

	const char *p;
	if ((p = After (pText, "tecla ")) != 0 || (p = After (pText, "key ")) != 0)
	{
		const char *q;
		u8 nShift = PAD_NO_KEY;
		if ((q = After (p, "CS+")) != 0)	{ nShift = ZX_CS; p = q; }
		else if ((q = After (p, "SS+")) != 0)	{ nShift = ZX_SS; p = q; }
		int nKey = ZXKeyFromName (p);
		if (nKey < 0)
		{
			return FALSE;
		}
		pAction->Type = PadActZXKey;
		pAction->nArg = nKey;
		pAction->nArg2 = nShift;
		return TRUE;
	}
	if ((p = After (pText, "pc ")) != 0)
	{
		u8 nMods = 0;
		for (;;)
		{
			const char *q;
			if ((q = After (p, "Alt+")) != 0)		{ nMods |= 0x04; p = q; }
			else if ((q = After (p, "Ctrl+")) != 0)		{ nMods |= 0x01; p = q; }
			else if ((q = After (p, "Shift+")) != 0)	{ nMods |= 0x02; p = q; }
			else break;
		}
		int nKey = PCKeyFromName (p);
		if (nKey < 0)
		{
			return FALSE;
		}
		pAction->Type = PadActPCKey;
		pAction->nArg = nKey;
		pAction->nArg2 = nMods;
		return TRUE;
	}
	return FALSE;
}

void CPadProfile::FormatAction (const TPadAction &Action, char *pBuffer, unsigned nSize)
{
	pBuffer[0] = '\0';
	switch (Action.Type)
	{
	case PadActJoy:
		Append (pBuffer, nSize,   Action.nArg == JOY_FIRE ? "disparo" : Action.nArg == JOY_FIRE2 ? "disparo 2"
					: Action.nArg == JOY_UP ? "arriba" : Action.nArg == JOY_DOWN ? "abajo"
					: Action.nArg == JOY_LEFT ? "izquierda" : "derecha");
		break;
	case PadActZXKey:
		Append (pBuffer, nSize, "tecla ");
		if (Action.nArg2 == ZX_CS)	Append (pBuffer, nSize, "CS+");
		if (Action.nArg2 == ZX_SS)	Append (pBuffer, nSize, "SS+");
		ZXKeyName (Action.nArg, pBuffer, nSize);
		break;
	case PadActPCKey:
		Append (pBuffer, nSize, "pc ");
		if (Action.nArg2 & 0x01)	Append (pBuffer, nSize, "Ctrl+");
		if (Action.nArg2 & 0x02)	Append (pBuffer, nSize, "Shift+");
		if (Action.nArg2 & 0x04)	Append (pBuffer, nSize, "Alt+");
		PCKeyName (Action.nArg, pBuffer, nSize);
		break;
	case PadActNextJoy:	Append (pBuffer, nSize, "joystick");	break;
	case PadActMenu:	Append (pBuffer, nSize, "menu");	break;
	default:		Append (pBuffer, nSize, "ninguno");	break;
	}
}

// --- Perfil -------------------------------------------------------------------

CPadProfile::CPadProfile (void)
{
	SetDefault (TRUE);
}

void CPadProfile::SetName (const char *pName)
{
	strncpy (m_Name, pName, sizeof m_Name - 1);
	m_Name[sizeof m_Name - 1] = '\0';
}

void CPadProfile::SetAction (unsigned nButton, const TPadAction &Action)
{
	if (nButton < PAD_BUTTONS)
	{
		m_Action[nButton] = Action;
	}
}

void CPadProfile::SetDefault (boolean bKnown)
{
	SetName ("");
	for (unsigned i = 0; i < PAD_BUTTONS; i++)
	{
		m_Action[i].Type = PadActNone;
		m_Action[i].nArg = 0;
		m_Action[i].nArg2 = PAD_NO_KEY;
	}
	const char *Default[][2] =
	{
		{"A", "disparo"}, {"Y", "disparo"}, {"LB", "disparo"}, {"RB", "disparo"},
		{"B", "disparo 2"}, {"X", "disparo 2"}, {"LT", "disparo 2"}, {"RT", "disparo 2"},
		{"Arriba", "arriba"}, {"Abajo", "abajo"}, {"Izquierda", "izquierda"}, {"Derecha", "derecha"},
		{"Select", "joystick"}, {"Minus", "joystick"}, {"Start", "menu"}, {"Plus", "menu"}
	};
	const char *Generic[][2] =
	{
		{"1", "disparo"}, {"4", "disparo"}, {"5", "disparo"}, {"6", "disparo"},
		{"2", "disparo 2"}, {"3", "disparo 2"}, {"7", "disparo 2"}, {"8", "disparo 2"},
		{"9", "joystick"}, {"10", "menu"}
	};
	unsigned nCount = bKnown ? sizeof Default / sizeof Default[0] : sizeof Generic / sizeof Generic[0];
	for (unsigned i = 0; i < nCount; i++)
	{
		const char *pButton = bKnown ? Default[i][0] : Generic[i][0];
		const char *pAction = bKnown ? Default[i][1] : Generic[i][1];
		int n = ButtonFromName (pButton, bKnown);
		if (n >= 0)
		{
			ParseAction (pAction, &m_Action[n]);
		}
	}
}

boolean CPadProfile::Parse (const char *pText, unsigned nLength, boolean bKnown)
{
	boolean bOK = TRUE;
	unsigned i = 0;
	while (i < nLength)
	{
		// una línea, sin espacios al principio ni al final
		char Line[96];
		unsigned n = 0;
		while (i < nLength && pText[i] != '\n')
		{
			if (pText[i] != '\r' && n + 1 < sizeof Line)
			{
				Line[n++] = pText[i];
			}
			i++;
		}
		i++;
		Line[n] = '\0';
		while (n > 0 && (Line[n - 1] == ' ' || Line[n - 1] == '\t'))
		{
			Line[--n] = '\0';
		}
		char *p = Line;
		while (*p == ' ' || *p == '\t')
		{
			p++;
		}
		if (*p == '\0' || *p == '#')
		{
			continue;			// vacía o comentario
		}

		char *pEqual = strchr (p, '=');
		if (pEqual == 0)
		{
			bOK = FALSE;
			continue;
		}
		char *pEnd = pEqual;
		while (pEnd > p && (pEnd[-1] == ' ' || pEnd[-1] == '\t'))
		{
			pEnd--;
		}
		*pEnd = '\0';
		const char *pValue = pEqual + 1;
		while (*pValue == ' ' || *pValue == '\t')
		{
			pValue++;
		}

		if (Equal (p, "nombre") || Equal (p, "name"))
		{
			SetName (pValue);
			continue;
		}
		int nButton = ButtonFromName (p, bKnown);
		TPadAction Action;
		if (nButton < 0 || !ParseAction (pValue, &Action))
		{
			bOK = FALSE;
			continue;
		}
		m_Action[nButton] = Action;
	}
	return bOK;
}

void CPadProfile::Format (char *pBuffer, unsigned nSize, boolean bKnown) const
{
	pBuffer[0] = '\0';
	if (m_Name[0] != '\0')
	{
		Append (pBuffer, nSize, "nombre = ");
		Append (pBuffer, nSize, m_Name);
		Append (pBuffer, nSize, "\n");
	}
	for (unsigned i = 0; i < PAD_BUTTONS; i++)
	{
		const char *pButton = ButtonName (i, bKnown);
		if (pButton == 0)
		{
			continue;
		}
		char Action[40];
		FormatAction (m_Action[i], Action, sizeof Action);
		Append (pBuffer, nSize, pButton);
		Append (pBuffer, nSize, " = ");
		Append (pBuffer, nSize, Action);
		Append (pBuffer, nSize, "\n");
	}
}

void CPadProfile::Evaluate (u32 nButtons, u32 nPrevButtons, TPadOutput *pOut) const
{
	memset (pOut, 0, sizeof *pOut);
	for (unsigned i = 0; i < PAD_BUTTONS; i++)
	{
		if ((nButtons & (1U << i)) == 0)
		{
			continue;
		}
		boolean bNew = (nPrevButtons & (1U << i)) == 0;
		const TPadAction &A = m_Action[i];
		switch (A.Type)
		{
		case PadActJoy:
			pOut->nJoy |= A.nArg;
			break;
		case PadActZXKey:			// mantenidas mientras se pulsa el botón
			pOut->Matrix[A.nArg >> 3] |= 1 << (A.nArg & 7);
			if (A.nArg2 != PAD_NO_KEY)
			{
				pOut->Matrix[A.nArg2 >> 3] |= 1 << (A.nArg2 & 7);
			}
			break;
		case PadActPCKey:			// una pulsación al pulsar el botón
			if (bNew && pOut->nPCKeys < PAD_BUTTONS)
			{
				pOut->PCKey[pOut->nPCKeys] = A.nArg;
				pOut->PCMods[pOut->nPCKeys] = A.nArg2;
				pOut->nPCKeys++;
			}
			break;
		case PadActNextJoy:
			pOut->bNextJoy |= bNew;
			break;
		case PadActMenu:
			pOut->bMenu |= bNew;
			break;
		default:
			break;
		}
	}
}

void CPadProfile::ActiveFile (const char *pActive, unsigned nLength, const char *pType,
			      char *pFile, unsigned nSize)
{
	char Name[PAD_PROFILE_NAME] = "";
	unsigned i = 0;
	while (pActive != 0 && i < nLength)
	{
		// una línea: "<tipo> = <nombre>"
		char Line[96];
		unsigned n = 0;
		while (i < nLength && pActive[i] != '\n')
		{
			if (pActive[i] != '\r' && n + 1 < sizeof Line)
			{
				Line[n++] = pActive[i];
			}
			i++;
		}
		i++;
		Line[n] = '\0';
		const char *p = Line;
		while (*p == ' ' || *p == '\t') p++;
		p = After (p, pType);
		if (p == 0)
		{
			continue;
		}
		while (*p == ' ' || *p == '\t') p++;
		if (*p++ != '=')
		{
			continue;
		}
		while (*p == ' ' || *p == '\t') p++;
		strncpy (Name, p, sizeof Name - 1);
		Name[sizeof Name - 1] = '\0';
		unsigned nEnd = strlen (Name);
		while (nEnd > 0 && (Name[nEnd - 1] == ' ' || Name[nEnd - 1] == '\t'))
		{
			Name[--nEnd] = '\0';
		}
	}
	pFile[0] = '\0';
	Append (pFile, nSize, pType);
	if (Name[0] != '\0')
	{
		Append (pFile, nSize, "-");
		Append (pFile, nSize, Name);
	}
	Append (pFile, nSize, ".txt");
}
