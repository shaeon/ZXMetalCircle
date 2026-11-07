//
// kernelpad.cpp
//
// La pantalla "Botones del mando" (página 5 de la configuración): el perfil
// de un mando para el tipo de joystick de su jugador.
//
// - Primera fila: el perfil (por defecto o uno guardado con nombre); con las
//   flechas se pasa por los guardados, N crea uno nuevo (copiando el que se
//   ve), R le cambia el nombre y SUPR lo borra.
// - Las demás: un botón cada una, con su acción. Al pulsar un botón del mando
//   se marca su fila. Con las flechas se pasa por las acciones sencillas
//   (ninguno, disparo, disparo 2, direcciones, joystick, ayuda, configuración,
//   ficheros); ENTER abre la
//   lista de teclas (del Spectrum, con CAPS SHIFT o SYMBOL SHIFT, o del
//   emulador); SUPR deja el botón sin acción.
// - G guarda (joysticks/VVVV-PPPP/<tipo>[-<nombre>].txt y activo.txt) y el
//   perfil se aplica al momento. ESC sale (si hay cambios, pregunta).
//
// Mientras la pantalla está abierta, los botones de ese mando no hacen nada
// más que marcar su fila.
//
#include "kernel.h"
#include "padprofile.h"
#include "i18n.h"
#include <circle/string.h>
#include <circle/util.h>
#include <fatfs/ff.h>
#include <string.h>

enum
{
	PadEditList,		// la lista de botones
	PadEditChoosePad,	// qué mando (si hay varios)
	PadEditChooseKind,	// ENTER en un botón: qué clase de tecla
	PadEditChooseKey,	// la tecla
	PadEditInputName,	// nombre de un perfil nuevo o nuevo nombre
	PadEditConfirmExit,	// ESC con cambios
	PadEditConfirmDelete,	// SUPR en la fila del perfil
	PadEditInputAlias	// A: el alias del mando
};

static const char *JoyTypes[JoystickModeCount] =		// como joystick=
	{"none", "kempston", "fuller", "sinclair1", "sinclair2", "cursor", "qaop"};

static const char *SimpleActions[] =
	{"ninguno", "disparo", "disparo 2", "arriba", "abajo", "izquierda", "derecha", "joystick", "ayuda",
	 "configuracion", "ficheros"};
#define SIMPLE_ACTIONS	(sizeof SimpleActions / sizeof SimpleActions[0])

static const char *ZXKeys[] =
{
	"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S",
	"T", "U", "V", "W", "X", "Y", "Z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
	"ENTER", "ESPACIO", "CS", "SS"
};
#define ZX_KEYS		(sizeof ZXKeys / sizeof ZXKeys[0])

static const char *PCKeysList[] =
{
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
	"Alt+F1", "Alt+F2", "Alt+F3", "Alt+F4", "Alt+F5", "Alt+F6", "Alt+F7", "Alt+F8", "Alt+F9",
	"Alt+F10", "Alt+F11", "Alt+F12", "Enter", "Esc", "Tab", "Pausa", "RePag", "AvPag",
	"Inicio", "Fin", "Supr", "Arriba", "Abajo", "Izquierda", "Derecha"
};
#define PC_KEYS		(sizeof PCKeysList / sizeof PCKeysList[0])

// La carpeta de un mando
static void PadDir (CString *pDir, u16 nVID, u16 nPID)
{
	pDir->Format ("SD:/joysticks/%04x-%04x", nVID, nPID);
}

// El alias de un modelo de mando (joysticks/VVVV-PPPP/alias.txt, la primera
// línea); lo comparten los mandos iguales
void CKernel::LoadPadAlias (unsigned nPad)
{
	m_PadAlias[nPad][0] = '\0';
	CString Dir, Path;
	PadDir (&Dir, m_PadVID[nPad], m_PadPID[nPad]);
	Path.Format ("%s/alias.txt", (const char *) Dir);
	FIL File;
	UINT nRead = 0;
	char Text[sizeof m_PadAlias[0]];
	if (f_open (&File, Path, FA_READ) != FR_OK)
	{
		return;
	}
	f_read (&File, Text, sizeof Text - 1, &nRead);
	f_close (&File);
	Text[nRead] = '\0';
	unsigned n = 0;
	while (Text[n] != '\0' && Text[n] != '\r' && Text[n] != '\n')
	{
		n++;
	}
	Text[n] = '\0';
	strcpy (m_PadAlias[nPad], Text);
}

// Qué número es entre los conectados de su mismo modelo (por orden de hueco,
// que es el de conexión): 1 el primero, 2 el segundo...
unsigned CKernel::PadInstance (unsigned nPad) const
{
	unsigned n = 1;
	for (unsigned i = 0; i < nPad; i++)
	{
		if (m_pGamePads[i] != 0 && m_PadVID[i] == m_PadVID[nPad] && m_PadPID[i] == m_PadPID[nPad])
		{
			n++;
		}
	}
	return n;
}

// Cómo se llama un mando en pantalla: su alias o "Mando N (VVVV:PPPP)"; si hay
// otro igual antes, con su número ("XBOX 2")
CString CKernel::PadName (unsigned nPad) const
{
	CString Name;
	unsigned nInstance = PadInstance (nPad);
	if (m_PadAlias[nPad][0] != '\0')
	{
		if (nInstance > 1)
		{
			Name.Format ("%s %u", m_PadAlias[nPad], nInstance);
		}
		else
		{
			Name = m_PadAlias[nPad];
		}
	}
	else
	{
		Name.Format (T ("Mando %u (%04X:%04X)"), nPad + 1, m_PadVID[nPad], m_PadPID[nPad]);
	}
	return Name;
}

void CKernel::PadEditStart (void)
{
	unsigned nPads = 0, nLast = 0;
	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		if (m_pGamePads[i] != 0)
		{
			nPads++;
			nLast = i;
		}
	}
	if (nPads == 0)
	{
		UpdateMessage (T ("No hay ningún mando conectado"));
		return;
	}
	if (nPads == 1)
	{
		PadEditOpen (nLast);
		return;
	}
	m_nPadEditStage = PadEditChoosePad;
	m_Menu.OpenChoice (T ("Elige el mando"));
	unsigned n = 0;
	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		if (m_pGamePads[i] != 0)
		{
			m_Menu.SetChoiceItem (n, PadName (i));
			m_PadEditChoice[n++] = i;
		}
	}
}

// Los perfiles guardados de este tipo: "" (el de por defecto) y los nombres
void CKernel::PadEditScanProfiles (void)
{
	m_nPadEditProfiles = 1;
	m_PadEditProfiles[0][0] = '\0';
	CString Dir;
	PadDir (&Dir, m_PadVID[m_nPadEdit], m_PadPID[m_nPadEdit]);
	const char *pType = JoyTypes[m_PadEditMode % JoystickModeCount];
	unsigned nType = strlen (pType);
	DIR Directory;
	FILINFO Info;
	if (f_opendir (&Directory, Dir) != FR_OK)
	{
		return;
	}
	while (f_readdir (&Directory, &Info) == FR_OK && Info.fname[0] != '\0')
	{
		// <tipo>-<nombre>.txt
		unsigned nLen = strlen (Info.fname);
		if (   nLen > nType + 5 && strncasecmp (Info.fname, pType, nType) == 0 && Info.fname[nType] == '-'
		    && strcasecmp (Info.fname + nLen - 4, ".txt") == 0
		    && m_nPadEditProfiles < PAD_EDIT_PROFILES && nLen - nType - 5 < PAD_PROFILE_NAME)
		{
			char *pName = m_PadEditProfiles[m_nPadEditProfiles++];
			memcpy (pName, Info.fname + nType + 1, nLen - nType - 5);
			pName[nLen - nType - 5] = '\0';
		}
	}
	f_closedir (&Directory);
}

// Lee un perfil guardado ("" = el de por defecto: <tipo>.txt o el de siempre)
void CKernel::PadEditLoad (const char *pName)
{
	boolean bKnown = (m_GamePadProps[m_nPadEdit] & GamePadPropertyIsKnown) != 0;
	m_PadEditProfile.SetDefault (bKnown);
	strncpy (m_PadEditName, pName, sizeof m_PadEditName - 1);
	m_PadEditName[sizeof m_PadEditName - 1] = '\0';

	CString Dir, Path;
	PadDir (&Dir, m_PadVID[m_nPadEdit], m_PadPID[m_nPadEdit]);
	const char *pType = JoyTypes[m_PadEditMode % JoystickModeCount];
	if (pName[0] != '\0')
	{
		Path.Format ("%s/%s-%s.txt", (const char *) Dir, pType, pName);
	}
	else
	{
		Path.Format ("%s/%s.txt", (const char *) Dir, pType);
	}
	static char Buffer[4096];
	FIL File;
	UINT nRead = 0;
	if (f_open (&File, Path, FA_READ) == FR_OK)
	{
		f_read (&File, Buffer, sizeof Buffer, &nRead);
		f_close (&File);
		m_PadEditProfile.Parse (Buffer, nRead, bKnown);
	}
}

void CKernel::PadEditOpen (unsigned nPad)
{
	m_nPadEdit = nPad;
	m_PadEditMode = m_Keyboard.GetPadPlayer (nPad) == 1 ? m_Keyboard.GetJoystickMode2 ()
							     : m_Keyboard.GetJoystickMode ();
	m_nPadEditStage = PadEditList;
	m_bPadEditDirty = FALSE;
	m_nPadEditPressed = 0;

	// el perfil activo (activo.txt)
	CString Dir, Path;
	PadDir (&Dir, m_PadVID[nPad], m_PadPID[nPad]);
	Path.Format ("%s/activo.txt", (const char *) Dir);
	static char Buffer[2048];
	FIL File;
	UINT nRead = 0;
	if (f_open (&File, Path, FA_READ) == FR_OK)
	{
		f_read (&File, Buffer, sizeof Buffer, &nRead);
		f_close (&File);
	}
	const char *pType = JoyTypes[m_PadEditMode % JoystickModeCount];
	char Name[64];
	CPadProfile::ActiveFile (Buffer, nRead, pType, Name, sizeof Name);
	Name[strlen (Name) - 4] = '\0';				// sin ".txt"
	const char *pName = strchr (Name, '-');
	PadEditScanProfiles ();
	PadEditLoad (pName != 0 ? pName + 1 : "");

	// los botones que se ven: con nombre (conocidos) o tantos como tenga
	boolean bKnown = (m_GamePadProps[nPad] & GamePadPropertyIsKnown) != 0;
	m_nPadEditRows = 0;
	unsigned nButtons = bKnown ? PAD_BUTTONS : (m_PadButtonCount[nPad] > 0 ? m_PadButtonCount[nPad] : 12);
	for (unsigned i = 0; i < PAD_BUTTONS && i < nButtons; i++)
	{
		if (CPadProfile::ButtonName (i, bKnown) != 0)
		{
			m_PadEditRows[m_nPadEditRows++] = i;
		}
	}

	m_Menu.OpenList ("", T ("<- -> acción  ENTER teclas  G guardar"),
			 T ("N nuevo R nombre SUPR borrar A alias ESC"));
	PadEditRefresh ();
}

void CKernel::PadEditRefresh (void)
{
	CString Title;
	Title.Format ("%s: %s", (const char *) PadName (m_nPadEdit), T (JoystickNames[m_PadEditMode % JoystickModeCount]));
	m_Menu.SetListTitle (Title);

	m_Menu.SetListRow (0, T ("Perfil"), m_PadEditName[0] != '\0' ? m_PadEditName : T ("por defecto"));
	boolean bKnown = (m_GamePadProps[m_nPadEdit] & GamePadPropertyIsKnown) != 0;
	for (unsigned i = 0; i < m_nPadEditRows; i++)
	{
		CString Label;
		Label.Format (T ("Botón %s"), CPadProfile::ButtonName (m_PadEditRows[i], bKnown));
		char Action[48];
		CPadProfile::FormatAction (m_PadEditProfile.GetAction (m_PadEditRows[i]), Action, sizeof Action);
		m_Menu.SetListRow (i + 1, Label, Action);
	}
	m_Menu.SetListCount (m_nPadEditRows + 1);
	m_Menu.SetListNote (m_bPadEditDirty ? T ("Hay cambios sin guardar (G)") : "");
}

// Guarda el perfil que se ve y lo deja activo
boolean CKernel::PadEditSave (void)
{
	CString Dir, Path;
	PadDir (&Dir, m_PadVID[m_nPadEdit], m_PadPID[m_nPadEdit]);
	f_mkdir ("SD:/joysticks");
	f_mkdir (Dir);
	const char *pType = JoyTypes[m_PadEditMode % JoystickModeCount];

	boolean bKnown = (m_GamePadProps[m_nPadEdit] & GamePadPropertyIsKnown) != 0;
	static char Text[4096];
	m_PadEditProfile.SetName (m_PadEditName);
	m_PadEditProfile.Format (Text, sizeof Text, bKnown);
	if (m_PadEditName[0] != '\0')
	{
		Path.Format ("%s/%s-%s.txt", (const char *) Dir, pType, m_PadEditName);
	}
	else
	{
		Path.Format ("%s/%s.txt", (const char *) Dir, pType);
	}
	FIL File;
	UINT nWritten = 0;
	boolean bOK = f_open (&File, Path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK;
	if (bOK)
	{
		bOK = f_write (&File, Text, strlen (Text), &nWritten) == FR_OK && nWritten == strlen (Text);
		bOK = f_close (&File) == FR_OK && bOK;
	}

	// activo.txt: la línea de este tipo, cambiada (o quitada, si es el de por defecto)
	static char Old[2048], New[2048];
	UINT nRead = 0;
	Path.Format ("%s/activo.txt", (const char *) Dir);
	if (f_open (&File, Path, FA_READ) == FR_OK)
	{
		f_read (&File, Old, sizeof Old - 1, &nRead);
		f_close (&File);
	}
	Old[nRead] = '\0';
	New[0] = '\0';
	unsigned nType = strlen (pType);
	for (char *p = Old; *p != '\0'; )
	{
		char *pLine = p;
		while (*p != '\0' && *p != '\n') p++;
		if (*p == '\n') *p++ = '\0';
		char *q = pLine;
		while (*q == ' ') q++;
		boolean bThisType = strncasecmp (q, pType, nType) == 0;
		if (bThisType)
		{
			char *r = q + nType;
			while (*r == ' ') r++;
			bThisType = *r == '=';
		}
		if (!bThisType && *q != '\0' && *q != '\r')
		{
			strncat (New, pLine, sizeof New - strlen (New) - 2);
			strcat (New, "\n");
		}
	}
	if (m_PadEditName[0] != '\0' && strlen (New) + nType + strlen (m_PadEditName) + 5 < sizeof New)
	{
		strcat (New, pType);
		strcat (New, " = ");
		strcat (New, m_PadEditName);
		strcat (New, "\n");
	}
	if (f_open (&File, Path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
	{
		f_write (&File, New, strlen (New), &nWritten);
		f_close (&File);
	}

	// que se aplique ya, en todos los mandos de ese modelo
	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		if (m_pGamePads[i] != 0 && m_PadVID[i] == m_PadVID[m_nPadEdit] && m_PadPID[i] == m_PadPID[m_nPadEdit])
		{
			m_PadProfileMode[i] = 0xFF;
		}
	}
	UpdatePadPlayers ();

	m_bPadEditDirty = !bOK;
	UpdateMessage (bOK ? T ("Perfil guardado") : T ("No se pudo guardar el perfil"));
	return bOK;
}

void CKernel::PadEditClose (void)
{
	m_nPadEdit = -1;
	m_nPadEditStage = PadEditList;
	m_Menu.OpenSettings ();
}

// Una tecla en la lista
void CKernel::PadEditKey (u8 nKey, unsigned nRow)
{
	if (nRow == 0)						// fila del perfil
	{
		if ((nKey == HID_KEY_LEFT || nKey == HID_KEY_RIGHT) && m_nPadEditProfiles > 0)
		{
			unsigned n = 0;
			for (unsigned i = 0; i < m_nPadEditProfiles; i++)
			{
				if (strcmp (m_PadEditProfiles[i], m_PadEditName) == 0)
				{
					n = i;
				}
			}
			n = (n + m_nPadEditProfiles + (nKey == HID_KEY_LEFT ? -1 : 1)) % m_nPadEditProfiles;
			PadEditLoad (m_PadEditProfiles[n]);
			m_bPadEditDirty = TRUE;			// el activo cambia al guardar
		}
		else if (nKey == 0x4C && m_PadEditName[0] != '\0')	// SUPR: borrar
		{
			CString Title;
			Title.Format (T ("¿Borrar el perfil %s?"), m_PadEditName);
			m_nPadEditStage = PadEditConfirmDelete;
			m_Menu.OpenChoice (Title);
			m_Menu.SetChoiceItem (0, T ("No"));
			m_Menu.SetChoiceItem (1, T ("Sí, borrarlo"));
			return;
		}
	}
	else if (nRow - 1 < m_nPadEditRows)			// un botón
	{
		unsigned nButton = m_PadEditRows[nRow - 1];
		TPadAction Action = m_PadEditProfile.GetAction (nButton);
		if (nKey == HID_KEY_LEFT || nKey == HID_KEY_RIGHT)
		{
			char Current[48];
			CPadProfile::FormatAction (Action, Current, sizeof Current);
			int n = -1;
			for (unsigned i = 0; i < SIMPLE_ACTIONS; i++)
			{
				if (strcmp (Current, SimpleActions[i]) == 0)
				{
					n = i;
				}
			}
			n = n < 0 ? 0 : (n + SIMPLE_ACTIONS + (nKey == HID_KEY_LEFT ? -1 : 1)) % SIMPLE_ACTIONS;
			CPadProfile::ParseAction (SimpleActions[n], &Action);
			m_PadEditProfile.SetAction (nButton, Action);
			m_bPadEditDirty = TRUE;
		}
		else if (nKey == 0x4C)				// SUPR: sin acción
		{
			CPadProfile::ParseAction ("ninguno", &Action);
			m_PadEditProfile.SetAction (nButton, Action);
			m_bPadEditDirty = TRUE;
		}
		else if (nKey == HID_KEY_ENTER)			// teclas
		{
			m_nPadEditButton = nButton;
			m_nPadEditStage = PadEditChooseKind;
			m_Menu.OpenChoice (T ("Qué tecla hace el botón"));
			m_Menu.SetChoiceItem (0, T ("Una tecla del Spectrum"));
			m_Menu.SetChoiceItem (1, T ("CAPS SHIFT + una tecla"));
			m_Menu.SetChoiceItem (2, T ("SYMBOL SHIFT + una tecla"));
			m_Menu.SetChoiceItem (3, T ("Una tecla del emulador"));
			return;
		}
	}

	if (nKey == 0x04)					// A: alias del mando
	{
		m_nPadEditStage = PadEditInputAlias;
		m_Menu.OpenInput (T ("Alias del mando"), T ("Letras, números y espacios"), T ("Vacío: quitar el alias"),
				  T ("ENTER aceptar  BORRAR corrige  ESC salir"));
		return;
	}
	else if (nKey == 0x0A)					// G: guardar
	{
		PadEditSave ();
	}
	else if (nKey == 0x11 || (nKey == 0x15 && m_PadEditName[0] != '\0'))	// N: nuevo; R: nombre
	{
		m_bPadEditRename = nKey == 0x15;
		m_nPadEditStage = PadEditInputName;
		m_Menu.OpenInput (m_bPadEditRename ? T ("Nuevo nombre del perfil") : T ("Nombre del perfil nuevo"),
				  T ("Letras, números y espacios"), "",
				  T ("ENTER aceptar  BORRAR corrige  ESC salir"));
		return;
	}
	else if (nKey == HID_KEY_ESC)
	{
		if (!m_bPadEditDirty)
		{
			PadEditClose ();
			return;
		}
		m_nPadEditStage = PadEditConfirmExit;
		m_Menu.OpenChoice (T ("Hay cambios sin guardar"));
		m_Menu.SetChoiceItem (0, T ("Guardar y salir"));
		m_Menu.SetChoiceItem (1, T ("Salir sin guardar"));
		m_Menu.SetChoiceItem (2, T ("Seguir"));
		return;
	}
	PadEditRefresh ();
}

// Vuelve a la lista, en la fila nRow
void CKernel::PadEditBackToList (unsigned nRow)
{
	unsigned nSelected = nRow;
	m_nPadEditStage = PadEditList;
	m_Menu.OpenList ("", T ("<- -> acción  ENTER teclas  G guardar"),
			 T ("N nuevo R nombre SUPR borrar A alias ESC"));
	PadEditRefresh ();
	m_Menu.SetListSelected (nSelected);
}

// Las respuestas del menú mientras la pantalla está abierta; TRUE si eran suyas
boolean CKernel::PadEditMenuResult (TMenuResult Result)
{
	if (Result == MenuResultSettingAction)
	{
		if (strcmp (m_Settings.GetDef (m_Menu.GetChangedSetting ()).pKey, "_padbuttons") != 0)
		{
			return FALSE;
		}
		PadEditStart ();
		return TRUE;
	}
	if (m_nPadEdit < 0 && m_nPadEditStage != PadEditChoosePad)
	{
		return FALSE;
	}

	switch (m_nPadEditStage)
	{
	case PadEditList:
		if (Result != MenuResultListKey)
		{
			return FALSE;
		}
		PadEditKey (m_Menu.GetListKey (), m_Menu.GetListSelected ());
		return TRUE;

	case PadEditChoosePad:
		if (Result == MenuResultChoice && m_Menu.GetChoice () < MAX_GAMEPADS)
		{
			PadEditOpen (m_PadEditChoice[m_Menu.GetChoice ()]);
		}
		else if (Result == MenuResultChoiceCancel)
		{
			m_nPadEditStage = PadEditList;
			m_Menu.OpenSettings ();
		}
		else
		{
			return FALSE;
		}
		return TRUE;

	case PadEditChooseKind:
		if (Result == MenuResultChoice)
		{
			m_nPadEditKind = m_Menu.GetChoice ();
			m_nPadEditStage = PadEditChooseKey;
			m_Menu.OpenChoice (T ("Elige la tecla"));
			if (m_nPadEditKind == 3)
			{
				for (unsigned i = 0; i < PC_KEYS; i++)
				{
					m_Menu.SetChoiceItem (i, PCKeysList[i]);
				}
			}
			else
			{
				unsigned n = 0;
				for (unsigned i = 0; i < ZX_KEYS; i++)
				{
					// con CS o SS delante, no la propia CS ni SS
					if (m_nPadEditKind == 0 || (strcmp (ZXKeys[i], "CS") != 0 && strcmp (ZXKeys[i], "SS") != 0))
					{
						m_Menu.SetChoiceItem (n++, ZXKeys[i]);
					}
				}
			}
		}
		else if (Result == MenuResultChoiceCancel)
		{
			PadEditBackToList (m_Menu.GetListSelected ());
		}
		else
		{
			return FALSE;
		}
		return TRUE;

	case PadEditChooseKey:
		if (Result == MenuResultChoice)
		{
			unsigned nChoice = m_Menu.GetChoice ();
			CString Text;
			if (m_nPadEditKind == 3)
			{
				Text.Format ("pc %s", nChoice < PC_KEYS ? PCKeysList[nChoice] : "F1");
			}
			else
			{
				// la n-ésima de la lista que se vio (sin CS ni SS con prefijo)
				unsigned n = 0;
				const char *pKey = "A";
				for (unsigned i = 0; i < ZX_KEYS; i++)
				{
					if (m_nPadEditKind == 0 || (strcmp (ZXKeys[i], "CS") != 0 && strcmp (ZXKeys[i], "SS") != 0))
					{
						if (n++ == nChoice)
						{
							pKey = ZXKeys[i];
						}
					}
				}
				Text.Format ("tecla %s%s", m_nPadEditKind == 1 ? "CS+" : (m_nPadEditKind == 2 ? "SS+" : ""), pKey);
			}
			TPadAction Action;
			if (CPadProfile::ParseAction (Text, &Action))
			{
				m_PadEditProfile.SetAction (m_nPadEditButton, Action);
				m_bPadEditDirty = TRUE;
			}
		}
		else if (Result != MenuResultChoiceCancel)
		{
			return FALSE;
		}
		for (unsigned i = 0; i < m_nPadEditRows; i++)
		{
			if (m_PadEditRows[i] == m_nPadEditButton)
			{
				PadEditBackToList (i + 1);
			}
		}
		return TRUE;

	case PadEditInputName:
		if (Result == MenuResultInput)
		{
			// letras, números, espacios y guiones bajos; sin espacios a los lados
			char Name[PAD_PROFILE_NAME];
			unsigned n = 0;
			for (const char *p = m_Menu.GetInput (); *p != '\0' && n + 1 < sizeof Name; p++)
			{
				char c = *p;
				if (   (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
				    || c == '_' || (c == ' ' && n > 0))
				{
					Name[n++] = c;
				}
			}
			while (n > 0 && Name[n - 1] == ' ')
			{
				n--;
			}
			Name[n] = '\0';
			if (n == 0)
			{
				UpdateMessage (T ("Nombre no válido"));
			}
			else
			{
				CString Old (m_PadEditName);
				strcpy (m_PadEditName, Name);
				if (PadEditSave () && m_bPadEditRename && Old.GetLength () > 0 && strcmp (Old, Name) != 0)
				{
					// el fichero con el nombre de antes sobra
					CString Dir, Path;
					PadDir (&Dir, m_PadVID[m_nPadEdit], m_PadPID[m_nPadEdit]);
					Path.Format ("%s/%s-%s.txt", (const char *) Dir,
						     JoyTypes[m_PadEditMode % JoystickModeCount], (const char *) Old);
					f_unlink (Path);
				}
				PadEditScanProfiles ();
			}
		}
		else if (Result != MenuResultInputCancel)
		{
			return FALSE;
		}
		PadEditBackToList (0);
		return TRUE;

	case PadEditConfirmExit:
		if (Result == MenuResultChoice && m_Menu.GetChoice () == 0)
		{
			if (PadEditSave ())
			{
				PadEditClose ();
				return TRUE;
			}
		}
		else if (Result == MenuResultChoice && m_Menu.GetChoice () == 1)
		{
			PadEditClose ();
			return TRUE;
		}
		else if (Result != MenuResultChoice && Result != MenuResultChoiceCancel)
		{
			return FALSE;
		}
		PadEditBackToList (0);
		return TRUE;

	case PadEditInputAlias:
		if (Result == MenuResultInput)
		{
			// letras, números y espacios; vacío = sin alias
			char Alias[sizeof m_PadAlias[0]];
			unsigned n = 0;
			for (const char *p = m_Menu.GetInput (); *p != '\0' && n + 1 < sizeof Alias; p++)
			{
				char c = *p;
				if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == ' ' && n > 0))
				{
					Alias[n++] = c;
				}
			}
			while (n > 0 && Alias[n - 1] == ' ')
			{
				n--;
			}
			Alias[n] = '\0';

			CString Dir, Path;
			PadDir (&Dir, m_PadVID[m_nPadEdit], m_PadPID[m_nPadEdit]);
			Path.Format ("%s/alias.txt", (const char *) Dir);
			boolean bOK;
			if (n == 0)
			{
				f_unlink (Path);
				bOK = TRUE;
			}
			else
			{
				f_mkdir ("SD:/joysticks");
				f_mkdir (Dir);
				FIL File;
				UINT nWritten = 0;
				bOK = f_open (&File, Path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK;
				if (bOK)
				{
					bOK = f_write (&File, Alias, n, &nWritten) == FR_OK && nWritten == n;
					bOK = f_close (&File) == FR_OK && bOK;
				}
			}
			// en todos los mandos de ese modelo
			for (unsigned i = 0; i < MAX_GAMEPADS; i++)
			{
				if (m_pGamePads[i] != 0 && m_PadVID[i] == m_PadVID[m_nPadEdit] && m_PadPID[i] == m_PadPID[m_nPadEdit])
				{
					LoadPadAlias (i);
				}
			}
			UpdatePadPlayers ();			// los nombres de la página 5
			UpdateMessage (!bOK ? T ("No se pudo guardar el alias") : (n == 0 ? T ("Alias quitado") : T ("Alias guardado")));
		}
		else if (Result != MenuResultInputCancel)
		{
			return FALSE;
		}
		PadEditBackToList (0);
		return TRUE;

	case PadEditConfirmDelete:
		if (Result == MenuResultChoice && m_Menu.GetChoice () == 1)
		{
			CString Dir, Path;
			PadDir (&Dir, m_PadVID[m_nPadEdit], m_PadPID[m_nPadEdit]);
			Path.Format ("%s/%s-%s.txt", (const char *) Dir,
				     JoyTypes[m_PadEditMode % JoystickModeCount], m_PadEditName);
			f_unlink (Path);
			PadEditLoad ("");			// vuelve el de por defecto
			PadEditSave ();				// y queda activo
			PadEditScanProfiles ();
			UpdateMessage (T ("Perfil borrado"));
		}
		else if (Result != MenuResultChoice && Result != MenuResultChoiceCancel)
		{
			return FALSE;
		}
		PadEditBackToList (0);
		return TRUE;
	}
	return FALSE;
}

// Desde el bucle principal: si se pulsó un botón del mando que se edita, su
// fila queda marcada
void CKernel::PadEditPoll (void)
{
	if (m_nPadEdit >= 0 && m_pGamePads[m_nPadEdit] == 0)
	{
		m_nPadEdit = -1;				// se ha desconectado
		m_nPadEditStage = PadEditList;
		if (m_Menu.IsOpen ())
		{
			m_Menu.OpenSettings ();
		}
		UpdateMessage (T ("Se ha desconectado el mando"));
		return;
	}
	unsigned nPressed = m_nPadEditPressed;
	if (nPressed == 0 || m_nPadEdit < 0 || m_nPadEditStage != PadEditList || !m_Menu.IsListOpen ())
	{
		return;
	}
	m_nPadEditPressed = 0;
	for (unsigned i = 0; i < m_nPadEditRows; i++)
	{
		if (m_PadEditRows[i] == nPressed - 1)
		{
			m_Menu.SetListSelected (i + 1);
		}
	}

	// y abajo, cuál es (también si no está en la lista)
	boolean bKnown = (m_GamePadProps[m_nPadEdit] & GamePadPropertyIsKnown) != 0;
	const char *pName = CPadProfile::ButtonName (nPressed - 1, bKnown);
	CString Number, Note;
	Number.Format ("%u", nPressed);
	Note.Format (T ("Has pulsado: Botón %s"), pName != 0 ? pName : (const char *) Number);
	m_Menu.SetListNote (Note);
}
