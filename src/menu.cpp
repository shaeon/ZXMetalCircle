//
// menu.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "menu.h"
#include <circle/string.h>
#include "exclusions.h"
#include "version.h"
#include "i18n.h"
#include <fatfs/ff.h>
#include <circle/util.h>

#define ROOT_DIR	"SD:/"

// Geometría del panel (en caracteres de 8x8)
#define PANEL_X		1
#define PANEL_Y		1
#define PANEL_COLS	42
#define PANEL_ROWS	35
#define LIST_ROW	4
#define LIST_LINES	27
#define TAPE_LIST_LINES	(PANEL_ROWS - 8)	// filas de bloques en el navegador de la cinta
#define TEXT_COLS	(PANEL_COLS - 2)

static boolean HasExtension (const char *pName, const char *pExt)
{
	size_t nLen = strlen (pName);
	size_t nExtLen = strlen (pExt);
	return nLen > nExtLen && strcasecmp (pName + nLen - nExtLen, pExt) == 0;
}

static char ToLower (char ch)
{
	return (ch >= 'A' && ch <= 'Z') ? ch - 'A' + 'a' : ch;
}

CMenu::CMenu (CVideo *pVideo)
:	m_pVideo (pVideo),
	m_Mode (ModeClosed),
	m_pEntries (new TEntry[MENU_MAX_ENTRIES]),
	m_nEntries (0),
	m_nSelected (0),
	m_nTop (0),
	m_nScrollTick (0),
	m_nScrollSelected (0)
{
	m_pSettings = 0;
	m_nSettingsPage = 0;
	m_nSettingsRow = 0;
	m_nChangedSetting = 0;
	m_nChoices = 0;
	m_nChoiceSelected = 0;
	m_ChoiceTitle[0] = '\0';
	m_InputTitle[0] = m_InputHelp[0][0] = m_InputHelp[1][0] = m_InputBuf[0] = '\0';
	m_nAboutTop = 0;
	m_nUnitSelected = 0;
	m_nUnitTarget = -1;
	m_pTape = 0;
	m_nTapeSelected = 0;
	m_nTapeTop = 0;
	m_bEditing = FALSE;
	m_bEditBad = FALSE;
	m_EditBuf[0] = '\0';
	memset (m_UnitName, 0, sizeof m_UnitName);
	m_pRemote = 0;
	strcpy (m_CurrentDir, ROOT_DIR);
	m_SelectedPath[0] = '\0';
	m_Error[0] = '\0';
	memset (m_Status, 0, sizeof m_Status);
	memset (m_Help, 0, sizeof m_Help);
}

CMenu::~CMenu (void)
{
	delete [] m_pEntries;
	m_pEntries = 0;
}

void CMenu::SetStatusLine (unsigned nLine, const char *pText)
{
	if (nLine < MENU_STATUS_LINES)
	{
		strncpy (m_Status[nLine], pText, sizeof m_Status[nLine] - 1);
		m_Status[nLine][sizeof m_Status[nLine] - 1] = '\0';
	}
}

// Licencia y créditos (A desde la ayuda)
static const char *AboutLines[] =
{
	N_("Emulador de ZX Spectrum para Raspberry"),
	N_("Pi, sin sistema operativo (bare metal)."),
	"",
	"(c) 2026 Shaeon (Carlos Palmero)",
	"",
	N_("LICENCIA"),
	N_("Es software libre: puedes redistribuirlo"),
	N_("y modificarlo según la Licencia Pública"),
	N_("General de GNU (GPL), versión 3 o"),
	N_("posterior. Se distribuye SIN NINGUNA"),
	N_("GARANTÍA. Texto completo en el fichero"),
	N_("LICENSE y en www.gnu.org/licenses"),
	"",
	N_("AGRADECIMIENTOS"),
	N_("A José Luis Sánchez, por JSpeccy y por"),
	N_("ZXBaremulator, que inspiraron este"),
	N_("proyecto, y por z80cpp, la CPU Z80 que"),
	N_("usa. La ULA, la contención, la"),
	N_("paginación y el AY vienen de JSpeccy."),
	"",
	N_("A René Stange, por la librería Circle,"),
	N_("sobre la que está hecho todo."),
	"",
	N_("Al grupo de RetroWiki, por su ayuda y"),
	N_("por mantener viva la comunidad."),
	"",
	N_("A Jordi (jbinary.itch.io), por sus"),
	N_("consejos sobre el Spectrum."),
	"",
	N_("A Víctor Iborra (EremusOne), por"),
	N_("ESPectrum, el emulador de Spectrum para"),
	N_("ESP32: github.com/EremusOne/ESPectrum"),
	"",
	N_("CRÉDITOS"),
	N_("Fuse (P. Kendall, G. Szász y otros):"),
	N_("  Spectranet y RS232 del Interface 1."),
	N_("SpectRusty (Rafal Michalski): ZX Net."),
	N_("MAME (Olivier Galibert): uPD765 del +3."),
	N_("Spectranet: firmware (c) Dylan Smith."),
	N_("esxDOS (c) Papaya Dezign (uso no"),
	N_("  comercial)."),
	"+3e (c) Garry Lancaster.",
	N_("zxp3 de Kyp y MiST: tarjeta SD."),
	N_("ROMs del Spectrum (c) Amstrad, que"),
	N_("  permite usarlas en emuladores."),
	N_("ROMs del Multiface (c) Romantic Robot."),
	N_("FatFs (ChaN), Mbed TLS, jsmn, ZXInfo y"),
	"  Spectrum Computing (ZXDB).",
	"",
	N_("Más detalles en el README."),
};

#define ABOUT_LINES	(sizeof AboutLines / sizeof AboutLines[0])
#define ABOUT_VISIBLE	(PANEL_ROWS - 7)

void CMenu::OpenTape (const CTape *pTape)
{
	m_pTape = pTape;
	unsigned nCount = pTape->GetBlockCount ();
	m_nTapeSelected = pTape->GetBlock () < nCount ? pTape->GetBlock () : (nCount > 0 ? nCount - 1 : 0);
	m_nTapeTop = m_nTapeSelected > LIST_LINES / 2 ? m_nTapeSelected - LIST_LINES / 2 : 0;
	m_Mode = ModeTape;
}

// Una línea del navegador de la cinta: número y lo que hay en el bloque
static void FormatTapeBlock (const CTape *pTape, unsigned nBlock, CString *pLine, boolean *pHighlight)
{
	CTape::TBlockInfo Info;
	*pHighlight = FALSE;
	if (!pTape->GetBlockInfo (nBlock, &Info))
	{
		*pLine = "";
		return;
	}

	CString Text;
	switch (Info.Kind)
	{
	case CTape::BlockProgram:	Text.Format (T ("Programa: %s"), Info.Text);			break;
	case CTape::BlockNumArray:	Text.Format (T ("Matriz numérica: %s"), Info.Text);		break;
	case CTape::BlockCharArray:	Text.Format (T ("Matriz de caracteres: %s"), Info.Text);	break;
	case CTape::BlockBytes:		Text.Format (T ("Bytes: %s %u,%u"), Info.Text, Info.nStart, Info.nLength);	break;
	case CTape::BlockData:
		if (Info.nFlag == 0xFF)	Text.Format (T ("  datos (%u bytes)"), Info.nLength);
		else			Text.Format (T ("  datos, flag %02X (%u bytes)"), Info.nFlag, Info.nLength);
		break;
	case CTape::BlockTurbo:		Text.Format (T ("Turbo (%u bytes)"), Info.nLength);		break;
	case CTape::BlockTone:		Text.Format (T ("Tono (%u pulsos)"), Info.nLength);		break;
	case CTape::BlockPulses:	Text.Format (T ("Pulsos (%u)"), Info.nLength);			break;
	case CTape::BlockPureData:	Text.Format (T ("Datos puros (%u bytes)"), Info.nLength);	break;
	case CTape::BlockDirect:	Text = T ("Grabación directa");				break;
	case CTape::BlockCSW:		Text = T ("Grabación CSW");					break;
	case CTape::BlockGDB:		Text = T ("Datos generalizados");				break;
	case CTape::BlockPause:		Text.Format (T ("Pausa (%u ms)"), Info.nLength);		break;
	case CTape::BlockStop:		Text = T ("PARAR LA CINTA");	*pHighlight = TRUE;		break;
	case CTape::BlockGroup:		Text.Format ("== %s ==", Info.Text);	*pHighlight = TRUE;	break;
	case CTape::BlockGroupEnd:	Text = "==";							break;
	case CTape::BlockJump:		Text.Format (T ("Saltar %d bloques"), Info.nJump);		break;
	case CTape::BlockLoop:		Text.Format (T ("Repetir %u veces"), Info.nLength);		break;
	case CTape::BlockLoopEnd:	Text = T ("Fin de la repetición");				break;
	case CTape::BlockCall:		Text = T ("Secuencia de llamadas");				break;
	case CTape::BlockReturn:	Text = T ("Volver");						break;
	case CTape::BlockSelect:	Text.Format (T ("Elegir (%u opciones)"), Info.nLength);	*pHighlight = TRUE;	break;
	case CTape::BlockStop48K:	Text = T ("Parar si es un 48K");	*pHighlight = TRUE;	break;
	case CTape::BlockLevel:		Text = T ("Nivel de la señal");				break;
	case CTape::BlockText:		Text.Format ("\"%s\"", Info.Text);	*pHighlight = TRUE;	break;
	case CTape::BlockArchive:	Text.Format (T ("Información: %s"), Info.Text);		break;
	case CTape::BlockHardware:	Text = T ("Información del hardware");			break;
	case CTape::BlockCustom:	Text.Format (T ("Información: %s"), Info.Text);		break;
	case CTape::BlockGlue:		Text = T ("(unión de ficheros)");				break;
	default:			Text.Format (T ("Bloque %02X desconocido"), Info.nLength);	break;
	}

	pLine->Format ("%3u %s", nBlock + 1, (const char *) Text);
}

void CMenu::OpenUnits (void)
{
	m_nUnitTarget = -1;
	m_Mode = ModeUnits;
}

void CMenu::SetUnitLine (unsigned nUnit, const char *pName)
{
	if (nUnit < UNIT_COUNT)
	{
		strncpy (m_UnitName[nUnit], pName != 0 ? pName : "", MENU_PATH_LEN - 1);
		m_UnitName[nUnit][MENU_PATH_LEN - 1] = '\0';
	}
}

void CMenu::OpenChoice (const char *pTitle)
{
	strncpy (m_ChoiceTitle, pTitle, sizeof m_ChoiceTitle - 1);
	m_ChoiceTitle[sizeof m_ChoiceTitle - 1] = '\0';
	m_nChoices = 0;
	m_nChoiceSelected = 0;
	m_Mode = ModeChoice;
}

void CMenu::OpenInput (const char *pTitle, const char *pHelp1, const char *pHelp2)
{
	strncpy (m_InputTitle, pTitle, sizeof m_InputTitle - 1);
	m_InputTitle[sizeof m_InputTitle - 1] = '\0';
	strncpy (m_InputHelp[0], pHelp1 != 0 ? pHelp1 : "", sizeof m_InputHelp[0] - 1);
	m_InputHelp[0][sizeof m_InputHelp[0] - 1] = '\0';
	strncpy (m_InputHelp[1], pHelp2 != 0 ? pHelp2 : "", sizeof m_InputHelp[1] - 1);
	m_InputHelp[1][sizeof m_InputHelp[1] - 1] = '\0';
	m_InputBuf[0] = '\0';
	m_Mode = ModeInput;
}

void CMenu::SetChoiceItem (unsigned nItem, const char *pText)
{
	if (nItem < MENU_CHOICE_ITEMS)
	{
		strncpy (m_Choice[nItem], pText, sizeof m_Choice[nItem] - 1);
		m_Choice[nItem][sizeof m_Choice[nItem] - 1] = '\0';
		if (nItem >= m_nChoices)
		{
			m_nChoices = nItem + 1;
		}
	}
}

void CMenu::OpenSettings (void)
{
	m_bEditing = FALSE;
	if (m_pSettings != 0)
	{
		m_Mode = ModeSettings;
		FixSettingsRow ();
	}
}

// Si el cursor está en una opción que no se usa (en gris), pasa a la siguiente
// que sí (o a la anterior, si no hay más abajo)
void CMenu::FixSettingsRow (void)
{
	unsigned Index[SETTINGS_MAX];
	unsigned nRows = SettingsOnPage (Index, SETTINGS_MAX);
	if (m_nSettingsRow >= nRows)
	{
		m_nSettingsRow = nRows > 0 ? nRows - 1 : 0;
	}
	if (nRows == 0 || m_pSettings->IsApplicable (Index[m_nSettingsRow]))
	{
		return;
	}
	for (unsigned i = m_nSettingsRow + 1; i < nRows; i++)
	{
		if (m_pSettings->IsApplicable (Index[i]))
		{
			m_nSettingsRow = i;
			return;
		}
	}
	for (unsigned i = m_nSettingsRow; i > 0; i--)
	{
		if (m_pSettings->IsApplicable (Index[i - 1]))
		{
			m_nSettingsRow = i - 1;
			return;
		}
	}
}

// índices (en la tabla de opciones) de las de la página actual
unsigned CMenu::SettingsOnPage (unsigned *pIndex, unsigned nMax) const
{
	unsigned n = 0;
	for (unsigned i = 0; i < m_pSettings->GetCount () && n < nMax; i++)
	{
		if (m_pSettings->GetDef (i).nPage == m_nSettingsPage)
		{
			pIndex[n++] = i;
		}
	}
	return n;
}

void CMenu::SetHelpLine (unsigned nLine, const char *pText)
{
	if (nLine < MENU_HELP_LINES)
	{
		strncpy (m_Help[nLine], pText, sizeof m_Help[nLine] - 1);
		m_Help[nLine][sizeof m_Help[nLine] - 1] = '\0';
	}
}

boolean CMenu::OpenBrowser (void)
{
	// se vuelve a leer excluir.txt por si ha cambiado (por ejemplo, desde la web)
	if (CExclusions::Get () != 0)
	{
		CExclusions::Get ()->Load ();
	}

	m_Mode = ModeBrowser;
	m_Error[0] = '\0';

	// Se recuerda dónde estaba el cursor (el fichero elegido la última vez y
	// su fila en pantalla) para volver a dejarlo ahí, y no al principio
	char LastName[MENU_NAME_LEN] = "";
	unsigned nLastRow = 0;
	if (m_nEntries > 0 && m_nSelected < m_nEntries)
	{
		strncpy (LastName, m_pEntries[m_Order[m_nSelected]].Name, sizeof LastName - 1);
		LastName[sizeof LastName - 1] = '\0';
		nLastRow = m_nSelected >= m_nTop ? m_nSelected - m_nTop : 0;
	}

	boolean bOK = ReadDirectory ();
	if (bOK)
	{
		for (unsigned i = 0; LastName[0] != '\0' && i < m_nEntries; i++)
		{
			if (strcmp (m_pEntries[m_Order[i]].Name, LastName) == 0)
			{
				m_nSelected = i;
				m_nTop = i >= nLastRow ? i - nLastRow : 0;
				EnsureVisible ();
				break;
			}
		}
	}
	else
	{
		// si el directorio ya no existe, se vuelve a la raíz
		strcpy (m_CurrentDir, ROOT_DIR);
		if (!ReadDirectory ())
		{
			strcpy (m_Error, T ("No se puede leer la tarjeta SD"));
			return FALSE;
		}
	}

	return TRUE;
}

void CMenu::OpenHelp (void)
{
	m_Mode = ModeHelp;
}

void CMenu::OpenKeyHelp (void)
{
	m_Mode = ModeKeyHelp;
}

static boolean IsLoadableName (const char *pName)
{
	static const char *Ext[] = {".tap", ".tzx", ".pzx", ".z80", ".sna", ".szx", ".csw", ".rom", ".mdr",
				    ".mdv", ".dsk", ".vhd", ".img", ".raw", ".hdf"};
	for (unsigned i = 0; i < sizeof Ext / sizeof Ext[0]; i++)
	{
		if (HasExtension (pName, Ext[i]))
		{
			return TRUE;
		}
	}
	return FALSE;
}

#define NET_ROOT	"NET:"

boolean CMenu::IsRemote (void) const
{
	return strncmp (m_CurrentDir, NET_ROOT, 4) == 0;
}

// Extensiones de cada unidad (pantalla de unidades)
static boolean IsUnitName (int nUnit, const char *pName)
{
	static const char *Tape[] = {".tap", ".tzx", ".pzx", ".csw", 0};
	static const char *Disk[] = {".dsk", 0};
	static const char *Hard[] = {".vhd", ".img", ".raw", ".hdf", 0};
	static const char *Cart[] = {".rom", 0};
	static const char *Mdr[] = {".mdr", ".mdv", 0};
	const char **pExt =   nUnit == UNIT_TAPE ? Tape
			    : (nUnit == UNIT_DISK_A || nUnit == UNIT_DISK_B) ? Disk
			    : nUnit == UNIT_HARDDISK ? Hard
			    : nUnit == UNIT_CARTRIDGE ? Cart : Mdr;
	for (; *pExt != 0; pExt++)
	{
		if (HasExtension (pName, *pExt))
		{
			return TRUE;
		}
	}
	return FALSE;
}

void CMenu::OnEntry (const char *pName, boolean bDir)
{
	if (m_nEntries >= MENU_MAX_ENTRIES || pName[0] == '.')
	{
		return;
	}
	if (!bDir && !(m_nUnitTarget >= 0 ? IsUnitName (m_nUnitTarget, pName) : IsLoadableName (pName)))
	{
		return;
	}

	TEntry *pEntry = &m_pEntries[m_nEntries++];
	strncpy (pEntry->Name, pName, MENU_NAME_LEN - 1);
	pEntry->Name[MENU_NAME_LEN - 1] = '\0';
	pEntry->bDir = bDir;
	pEntry->bRemoteRoot = FALSE;
}

// "NET:" = lista de servidores; "NET:servidor/ruta" = carpeta remota
boolean CMenu::ReadRemoteDirectory (void)
{
	if (m_pRemote == 0 || !m_pRemote->IsAvailable ())
	{
		strcpy (m_Error, T ("Sin red"));
		return FALSE;
	}

	strcpy (m_pEntries[0].Name, "..");
	m_pEntries[0].bDir = TRUE;
	m_pEntries[0].bRemoteRoot = FALSE;
	m_nEntries = 1;

	if (strcmp (m_CurrentDir, NET_ROOT) == 0)
	{
		for (unsigned i = 0; i < m_pRemote->GetServerCount () && m_nEntries < MENU_MAX_ENTRIES; i++)
		{
			OnEntry (m_pRemote->GetServer (i), TRUE);
		}
		SortEntries ();
		return TRUE;
	}

	char Host[64];
	const char *pHost = m_CurrentDir + 4;
	const char *pPath = strchr (pHost, '/');
	unsigned nHostLen = pPath != 0 ? (unsigned) (pPath - pHost) : strlen (pHost);
	if (nHostLen >= sizeof Host)
	{
		return FALSE;
	}
	memcpy (Host, pHost, nHostLen);
	Host[nHostLen] = '\0';

	if (!m_pRemote->List (Host, pPath != 0 ? pPath : "/", this))
	{
		strncpy (m_Error, T (m_pRemote->GetError ()), sizeof m_Error - 1);
		m_Error[sizeof m_Error - 1] = '\0';
		return FALSE;
	}

	SortEntries ();
	return TRUE;
}

boolean CMenu::ReadDirectory (void)
{
	m_nEntries = 0;
	m_nSelected = 0;
	m_nTop = 0;

	if (IsRemote ())
	{
		return ReadRemoteDirectory ();
	}

	DIR Directory;
	if (f_opendir (&Directory, m_CurrentDir) != FR_OK)
	{
		return FALSE;
	}

	if (strcmp (m_CurrentDir, ROOT_DIR) != 0)
	{
		strcpy (m_pEntries[0].Name, "..");
		m_pEntries[0].bDir = TRUE;
		m_nEntries = 1;
	}

	FILINFO FileInfo;
	while (   m_nEntries < MENU_MAX_ENTRIES
	       && f_readdir (&Directory, &FileInfo) == FR_OK
	       && FileInfo.fname[0] != '\0')
	{
		if (   (FileInfo.fattrib & (AM_HID | AM_SYS))
		    || FileInfo.fname[0] == '.')
		{
			continue;
		}

		boolean bDir = (FileInfo.fattrib & AM_DIR) != 0;

		// excluir.txt: ficheros y carpetas que no se muestran
		if (   CExclusions::Get () != 0
		    && CExclusions::Get ()->IsExcluded (m_CurrentDir, FileInfo.fname, bDir))
		{
			continue;
		}
		if (   !bDir
		    && !(m_nUnitTarget >= 0 ? IsUnitName (m_nUnitTarget, FileInfo.fname)
					    : IsLoadableName (FileInfo.fname)))
		{
			continue;
		}

		TEntry *pEntry = &m_pEntries[m_nEntries++];
		strncpy (pEntry->Name, FileInfo.fname, MENU_NAME_LEN - 1);
		pEntry->Name[MENU_NAME_LEN - 1] = '\0';
		pEntry->bDir = bDir;
		pEntry->bRemoteRoot = FALSE;
	}

	f_closedir (&Directory);

	// raíz de la SD con red: entrada para los servidores TNFS
	boolean bRemote =    m_nUnitTarget < 0 && strcmp (m_CurrentDir, ROOT_DIR) == 0 && m_pRemote != 0
			  && m_pRemote->IsAvailable () && m_nEntries < MENU_MAX_ENTRIES;
	if (bRemote)
	{
		TEntry *pEntry = &m_pEntries[m_nEntries++];
		strcpy (pEntry->Name, "Internet (TNFS)");
		pEntry->bDir = TRUE;
		pEntry->bRemoteRoot = TRUE;
	}

	SortEntries ();

	// la entrada de Internet va la primera
	if (bRemote)
	{
		for (unsigned i = 0; i < m_nEntries; i++)
		{
			if (m_pEntries[m_Order[i]].bRemoteRoot)
			{
				u16 nIndex = m_Order[i];
				for (unsigned j = i; j > 0; j--)
				{
					m_Order[j] = m_Order[j - 1];
				}
				m_Order[0] = nIndex;
				break;
			}
		}
	}

	return TRUE;
}

void CMenu::SortEntries (void)
{
	for (unsigned i = 0; i < m_nEntries; i++)
	{
		m_Order[i] = i;
	}

	// Directorios primero, luego orden alfabético sin distinguir mayúsculas.
	// Ordenación Shell (no hay qsort sin librería C).
	for (unsigned nGap = m_nEntries / 2; nGap > 0; nGap /= 2)
	{
		for (unsigned i = nGap; i < m_nEntries; i++)
		{
			u16 nTemp = m_Order[i];
			const TEntry *pTemp = &m_pEntries[nTemp];

			unsigned j = i;
			while (j >= nGap)
			{
				const TEntry *pPrev = &m_pEntries[m_Order[j - nGap]];

				boolean bGreater;
				if (pPrev->bDir != pTemp->bDir)
				{
					bGreater = !pPrev->bDir;
				}
				else if (strcmp (pTemp->Name, "..") == 0)
				{
					bGreater = TRUE;
				}
				else if (strcmp (pPrev->Name, "..") == 0)
				{
					bGreater = FALSE;
				}
				else
				{
					bGreater = strcasecmp (pPrev->Name, pTemp->Name) > 0;
				}

				if (!bGreater)
				{
					break;
				}

				m_Order[j] = m_Order[j - nGap];
				j -= nGap;
			}

			m_Order[j] = nTemp;
		}
	}
}

void CMenu::EnsureVisible (void)
{
	if (m_nSelected < m_nTop)
	{
		m_nTop = m_nSelected;
	}
	else if (m_nSelected >= m_nTop + LIST_LINES)
	{
		m_nTop = m_nSelected - LIST_LINES + 1;
	}
}

void CMenu::GoParent (void)
{
	if (strcmp (m_CurrentDir, ROOT_DIR) == 0)
	{
		return;
	}

	// "NET:" -> raíz de la SD; "NET:servidor" -> "NET:"
	if (IsRemote () && strchr (m_CurrentDir + 4, '/') == 0)
	{
		boolean bServerList = strcmp (m_CurrentDir, NET_ROOT) == 0;
		char OldName[MENU_NAME_LEN];
		strncpy (OldName, bServerList ? "Internet (TNFS)" : m_CurrentDir + 4, sizeof OldName - 1);
		OldName[sizeof OldName - 1] = '\0';
		strcpy (m_CurrentDir, bServerList ? ROOT_DIR : NET_ROOT);
		ReadDirectory ();
		for (unsigned i = 0; i < m_nEntries; i++)
		{
			if (strcmp (m_pEntries[m_Order[i]].Name, OldName) == 0)
			{
				m_nSelected = i;
				EnsureVisible ();
				break;
			}
		}
		return;
	}

	// "SD:/juegos/arcade" -> "SD:/juegos"
	char *pSlash = 0;
	for (char *p = m_CurrentDir; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pSlash = p;
		}
	}

	char OldName[MENU_NAME_LEN];
	strncpy (OldName, pSlash + 1, sizeof OldName - 1);
	OldName[sizeof OldName - 1] = '\0';

	if (!IsRemote () && pSlash == m_CurrentDir + strlen (ROOT_DIR) - 1)
	{
		strcpy (m_CurrentDir, ROOT_DIR);
	}
	else
	{
		*pSlash = '\0';
	}

	ReadDirectory ();

	// selecciona el directorio del que venimos
	for (unsigned i = 0; i < m_nEntries; i++)
	{
		if (strcmp (m_pEntries[m_Order[i]].Name, OldName) == 0)
		{
			m_nSelected = i;
			EnsureVisible ();
			break;
		}
	}
}

void CMenu::EnterSelected (TMenuResult *pResult)
{
	if (m_nEntries == 0)
	{
		return;
	}

	const TEntry *pEntry = &m_pEntries[m_Order[m_nSelected]];

	if (pEntry->bDir)
	{
		if (strcmp (pEntry->Name, "..") == 0)
		{
			GoParent ();
			return;
		}

		if (pEntry->bRemoteRoot)
		{
			strcpy (m_CurrentDir, NET_ROOT);
			if (!ReadDirectory ())
			{
				strcpy (m_CurrentDir, ROOT_DIR);
				char Error[sizeof m_Error];
				strcpy (Error, m_Error);
				ReadDirectory ();
				strcpy (m_Error, Error);
			}
			return;
		}

		if (strlen (m_CurrentDir) + strlen (pEntry->Name) + 2 >= MENU_PATH_LEN)
		{
			return;
		}

		if (strcmp (m_CurrentDir, ROOT_DIR) != 0 && strcmp (m_CurrentDir, NET_ROOT) != 0)
		{
			strcat (m_CurrentDir, "/");
		}
		strcat (m_CurrentDir, pEntry->Name);

		m_Error[0] = '\0';
		if (!ReadDirectory ())
		{
			char Error[sizeof m_Error];
			strcpy (Error, m_Error);
			GoParent ();
			strcpy (m_Error, Error);	// se muestra el error de la carpeta remota
		}
		return;
	}

	strcpy (m_SelectedPath, m_CurrentDir);
	if (strcmp (m_CurrentDir, ROOT_DIR) != 0 && strcmp (m_CurrentDir, NET_ROOT) != 0)
	{
		strcat (m_SelectedPath, "/");
	}
	strncat (m_SelectedPath, pEntry->Name, MENU_PATH_LEN - strlen (m_SelectedPath) - 1);

	*pResult = MenuResultFileSelected;
}

void CMenu::JumpToLetter (char chLetter)
{
	if (m_nEntries == 0)
	{
		return;
	}

	// Busca la siguiente entrada que empiece por esa letra (circular)
	for (unsigned n = 1; n <= m_nEntries; n++)
	{
		unsigned i = (m_nSelected + n) % m_nEntries;
		if (ToLower (m_pEntries[m_Order[i]].Name[0]) == chLetter)
		{
			m_nSelected = i;
			EnsureVisible ();
			return;
		}
	}
}

TMenuResult CMenu::HandleKey (const TKeyEvent &rEvent)
{
	if (m_Mode == ModeBrowser)
	{
		m_Error[0] = '\0';	// el error se ve hasta la siguiente tecla
	}

	TMenuResult Result = MenuResultNone;
	u8 nKey = rEvent.nKey;

	if (m_Mode == ModeHelp)
	{
		if (nKey == HID_KEY_ENTER || nKey == 0x58)
		{
			OpenBrowser ();		// selector de cintas
		}
		else if (nKey == 0x06)		// C: configuración
		{
			Result = MenuResultOpenSettings;
		}
		else if (nKey == 0x04)		// A: licencia y créditos
		{
			m_nAboutTop = 0;
			m_Mode = ModeAbout;
		}
		else if (nKey == 0x18)		// U: unidades (las líneas las rellena el kernel)
		{
			Result = MenuResultOpenUnits;
		}
		else if (nKey == HID_KEY_ESC || nKey == HID_KEY_F1 || nKey == HID_KEY_SPACE)
		{
			m_Mode = ModeClosed;
			Result = MenuResultClose;
		}
		return Result;
	}

	if (m_Mode == ModeUnits)
	{
		if (nKey == HID_KEY_UP && m_nUnitSelected > 0)
		{
			m_nUnitSelected--;
		}
		else if (nKey == HID_KEY_DOWN && m_nUnitSelected + 1 < UNIT_COUNT)
		{
			m_nUnitSelected++;
		}
		else if (nKey == HID_KEY_ENTER || nKey == 0x58 || nKey == HID_KEY_RIGHT)
		{
			m_nUnitTarget = (int) m_nUnitSelected;	// selector solo con esos ficheros
			OpenBrowser ();
		}
		else if (nKey == 0x4C || nKey == HID_KEY_BACKSPACE)	// SUPR o retroceso: vaciar
		{
			Result = MenuResultUnitEject;
		}
		else if (nKey == HID_KEY_ESC)
		{
			m_Mode = ModeHelp;
		}
		else if (nKey == HID_KEY_F1)
		{
			m_Mode = ModeClosed;
			Result = MenuResultClose;
		}
		return Result;
	}

	if (m_Mode == ModeTape)
	{
		unsigned nCount = m_pTape != 0 ? m_pTape->GetBlockCount () : 0;
		if (nCount == 0)
		{
			m_Mode = ModeClosed;
			return MenuResultClose;
		}
		switch (nKey)
		{
		case HID_KEY_UP:	if (m_nTapeSelected > 0) m_nTapeSelected--;			break;
		case HID_KEY_DOWN:	if (m_nTapeSelected + 1 < nCount) m_nTapeSelected++;		break;
		case 0x4B:		m_nTapeSelected = m_nTapeSelected > LIST_LINES ? m_nTapeSelected - LIST_LINES : 0;	break;	// RePág
		case 0x4E:		m_nTapeSelected = m_nTapeSelected + LIST_LINES < nCount ? m_nTapeSelected + LIST_LINES : nCount - 1;	break;	// AvPág
		case HID_KEY_HOME:	m_nTapeSelected = 0;							break;
		case HID_KEY_END:	m_nTapeSelected = nCount - 1;						break;
		case HID_KEY_ENTER:
		case 0x58:		// ENTER del teclado numérico
			m_Mode = ModeClosed;
			return MenuResultTapeSeek;
		case 0x13:		// P: invertir la polaridad (el menú sigue abierto)
			return MenuResultTapeInvert;
		case HID_KEY_ESC:
		case HID_KEY_F1 + 2:	// F3 otra vez: cerrar
			m_Mode = ModeClosed;
			return MenuResultClose;
		default:
			break;
		}
		if (m_nTapeSelected < m_nTapeTop)
		{
			m_nTapeTop = m_nTapeSelected;
		}
		else if (m_nTapeSelected >= m_nTapeTop + TAPE_LIST_LINES)
		{
			m_nTapeTop = m_nTapeSelected - TAPE_LIST_LINES + 1;
		}
		return Result;
	}

	if (m_Mode == ModeAbout)
	{
		unsigned nMaxTop = ABOUT_LINES > ABOUT_VISIBLE ? ABOUT_LINES - ABOUT_VISIBLE : 0;
		if (nKey == HID_KEY_UP && m_nAboutTop > 0)
		{
			m_nAboutTop--;
		}
		else if (nKey == HID_KEY_DOWN && m_nAboutTop < nMaxTop)
		{
			m_nAboutTop++;
		}
		else if (nKey == 0x4B)				// RePág
		{
			m_nAboutTop = m_nAboutTop > ABOUT_VISIBLE ? m_nAboutTop - ABOUT_VISIBLE : 0;
		}
		else if (nKey == 0x4E)				// AvPág
		{
			m_nAboutTop = m_nAboutTop + ABOUT_VISIBLE < nMaxTop ? m_nAboutTop + ABOUT_VISIBLE : nMaxTop;
		}
		else if (nKey == HID_KEY_ESC || nKey == HID_KEY_BACKSPACE || nKey == HID_KEY_ENTER)
		{
			m_Mode = ModeHelp;
		}
		else if (nKey == HID_KEY_F1)
		{
			m_Mode = ModeClosed;
			Result = MenuResultClose;
		}
		return Result;
	}

	if (m_Mode == ModeInput)
	{
		unsigned nLen = strlen (m_InputBuf);
		char c = '\0';
		if (nKey >= 0x04 && nKey <= 0x1D)
		{
			c = 'A' + (nKey - 0x04);
		}
		else if (nKey >= 0x1E && nKey <= 0x26)
		{
			c = '1' + (nKey - 0x1E);
		}
		else if (nKey == 0x27)
		{
			c = '0';
		}
		else if (nKey == HID_KEY_SPACE)
		{
			c = ' ';
		}

		if (c != '\0')
		{
			if (nLen + 1 < sizeof m_InputBuf)
			{
				m_InputBuf[nLen] = c;
				m_InputBuf[nLen + 1] = '\0';
			}
		}
		else if (nKey == HID_KEY_BACKSPACE && nLen > 0)
		{
			m_InputBuf[nLen - 1] = '\0';
		}
		else if (nKey == HID_KEY_ENTER || nKey == 0x58)
		{
			m_Mode = ModeClosed;
			Result = MenuResultInput;
		}
		else if (nKey == HID_KEY_ESC || nKey == HID_KEY_F1)
		{
			m_Mode = ModeClosed;
			Result = MenuResultInputCancel;
		}
		return Result;
	}

	if (m_Mode == ModeChoice)
	{
		if (nKey == HID_KEY_UP && m_nChoiceSelected > 0)
		{
			m_nChoiceSelected--;
		}
		else if (nKey == HID_KEY_DOWN && m_nChoiceSelected + 1 < m_nChoices)
		{
			m_nChoiceSelected++;
		}
		else if (nKey >= 0x1E && nKey <= 0x26 && (unsigned) (nKey - 0x1E) < m_nChoices)
		{
			m_nChoiceSelected = nKey - 0x1E;		// 1-9: elegir directamente
			m_Mode = ModeClosed;
			Result = MenuResultChoice;
		}
		else if (nKey == HID_KEY_ENTER || nKey == 0x58)
		{
			m_Mode = ModeClosed;
			Result = MenuResultChoice;
		}
		else if (nKey == HID_KEY_ESC || nKey == HID_KEY_F1)
		{
			m_Mode = ModeClosed;
			Result = MenuResultChoiceCancel;
		}
		return Result;
	}

	if (m_Mode == ModeSettings)
	{
		unsigned Index[SETTINGS_MAX];
		unsigned nRows = SettingsOnPage (Index, SETTINGS_MAX);
		unsigned nPages = CSettings::GetPageCount ();

		// escribiendo una IP: números, punto, retroceso, SUPR (vaciar),
		// ENTER (aceptar si es válida) y ESC (dejarla como estaba)
		if (m_bEditing)
		{
			unsigned nLen = strlen (m_EditBuf);
			boolean bPass = CSettings::IsPassword (Index[m_nSettingsRow]);
			char c = 0;
			if (bPass && nKey >= 0x04 && nKey <= 0x1D)	c = 'a' + nKey - 0x04;	// contraseña: letras
			else if (nKey >= 0x1E && nKey <= 0x26)	c = '1' + nKey - 0x1E;
			else if (nKey == 0x27)			c = '0';
			else if (nKey >= 0x59 && nKey <= 0x61)	c = '1' + nKey - 0x59;	// teclado numérico
			else if (nKey == 0x62)			c = '0';
			else if ((nKey == 0x37 || nKey == 0x63) && !bPass)	c = '.';

			m_bEditBad = FALSE;
			if (c != 0 && nLen < (bPass ? 16u : 15u))
			{
				m_EditBuf[nLen] = c;
				m_EditBuf[nLen + 1] = '\0';
			}
			else if (nKey == HID_KEY_BACKSPACE && nLen > 0)
			{
				m_EditBuf[nLen - 1] = '\0';
			}
			else if (nKey == 0x4C)			// SUPR: vacía (automática)
			{
				m_EditBuf[0] = '\0';
			}
			else if (nKey == HID_KEY_ENTER || nKey == 0x58)
			{
				if (bPass || CSettings::IsValidIP (m_EditBuf))
				{
					m_bEditing = FALSE;
					m_nChangedSetting = Index[m_nSettingsRow];
					m_pSettings->SetValue (m_pSettings->GetDef (m_nChangedSetting).pKey, m_EditBuf);
					Result = MenuResultSetting;
				}
				else
				{
					m_bEditBad = TRUE;
				}
			}
			else if (nKey == HID_KEY_ESC)
			{
				m_bEditing = FALSE;
			}
			return Result;
		}

		// Las opciones que no se usan con lo elegido (en gris) no se pueden
		// elegir ni cambiar: el cursor se las salta
		boolean bApplies =    m_nSettingsRow < nRows
				   && m_pSettings->IsApplicable (Index[m_nSettingsRow]);

		if (nKey == HID_KEY_UP)
		{
			for (unsigned i = m_nSettingsRow; i > 0; i--)
			{
				if (m_pSettings->IsApplicable (Index[i - 1]))
				{
					m_nSettingsRow = i - 1;
					break;
				}
			}
		}
		else if (nKey == HID_KEY_DOWN)
		{
			for (unsigned i = m_nSettingsRow + 1; i < nRows; i++)
			{
				if (m_pSettings->IsApplicable (Index[i]))
				{
					m_nSettingsRow = i;
					break;
				}
			}
		}
		else if (   (nKey == HID_KEY_RIGHT || nKey == HID_KEY_ENTER || nKey == 0x58)
			 && bApplies && CSettings::IsEditable (Index[m_nSettingsRow]))
		{
			// opción que se escribe: se empieza con lo que tiene
			strncpy (m_EditBuf, m_pSettings->GetValue (Index[m_nSettingsRow]), sizeof m_EditBuf - 1);
			m_EditBuf[sizeof m_EditBuf - 1] = '\0';
			m_bEditing = TRUE;
			m_bEditBad = FALSE;
		}
		else if (   (nKey == HID_KEY_LEFT || nKey == HID_KEY_RIGHT || nKey == HID_KEY_ENTER || nKey == 0x58)
			 && bApplies && !CSettings::IsEditable (Index[m_nSettingsRow]))
		{
			m_nChangedSetting = Index[m_nSettingsRow];
			m_pSettings->Step (m_nChangedSetting, nKey == HID_KEY_LEFT ? -1 : 1);
			Result = MenuResultSetting;
		}
		else if (nKey == 0x4E || nKey == HID_KEY_TAB)		// AvPág o Tab: página siguiente
		{
			m_nSettingsPage = (m_nSettingsPage + 1) % nPages;
			m_nSettingsRow = 0;
		}
		else if (nKey == 0x4B)					// RePág: página anterior
		{
			m_nSettingsPage = (m_nSettingsPage + nPages - 1) % nPages;
			m_nSettingsRow = 0;
		}
		else if (nKey == 0x0A)					// G: guardar
		{
			Result = MenuResultSaveSettings;
		}
		else if (nKey == 0x15)					// R: reiniciar la Pi
		{
			Result = MenuResultReboot;
		}
		else if (nKey == HID_KEY_ESC || nKey == HID_KEY_BACKSPACE)
		{
			m_Mode = ModeHelp;
		}
		else if (nKey == HID_KEY_F1)
		{
			m_Mode = ModeClosed;
			Result = MenuResultClose;
		}
		if (m_Mode == ModeSettings)
		{
			FixSettingsRow ();	// al cambiar de página o de modelo
		}
		return Result;
	}

	if (m_Mode == ModeKeyHelp)
	{
		if (   nKey == HID_KEY_ESC || nKey == HID_KEY_ENTER
		    || nKey == HID_KEY_SPACE || nKey == 0x0E)	// K
		{
			m_Mode = ModeClosed;
			Result = MenuResultClose;
		}
		return Result;
	}

	switch (nKey)
	{
	case HID_KEY_ESC:
	case HID_KEY_F1:
		if (m_nUnitTarget >= 0 && nKey == HID_KEY_ESC)
		{
			m_nUnitTarget = -1;		// vuelta a la pantalla de unidades
			m_Mode = ModeUnits;
			break;
		}
		m_nUnitTarget = -1;
		m_Mode = ModeClosed;
		Result = MenuResultClose;
		break;

	case HID_KEY_UP:
		if (m_nSelected > 0) m_nSelected--;
		break;

	case HID_KEY_DOWN:
		if (m_nSelected + 1 < m_nEntries) m_nSelected++;
		break;

	case HID_KEY_PAGEUP:
	case HID_KEY_LEFT:
		m_nSelected = m_nSelected > LIST_LINES ? m_nSelected - LIST_LINES : 0;
		break;

	case HID_KEY_PAGEDOWN:
	case HID_KEY_RIGHT:
		m_nSelected += LIST_LINES;
		if (m_nSelected >= m_nEntries)
		{
			m_nSelected = m_nEntries > 0 ? m_nEntries - 1 : 0;
		}
		break;

	case HID_KEY_HOME:
		m_nSelected = 0;
		break;

	case HID_KEY_END:
		m_nSelected = m_nEntries > 0 ? m_nEntries - 1 : 0;
		break;

	case HID_KEY_ENTER:
	case 0x58:
		EnterSelected (&Result);
		if (Result == MenuResultFileSelected)
		{
			m_Mode = ModeClosed;
		}
		break;

	case HID_KEY_BACKSPACE:
		GoParent ();
		break;

	default:
		if (nKey >= 0x04 && nKey <= 0x1D)
		{
			JumpToLetter ('a' + nKey - 0x04);
		}
		else if (nKey >= 0x1E && nKey <= 0x27)
		{
			JumpToLetter (nKey == 0x27 ? '0' : '1' + nKey - 0x1E);
		}
		break;
	}

	EnsureVisible ();

	return Result;
}

// Desplazamiento del nombre largo seleccionado: pausa al principio, avanza un
// carácter cada 5 cuadros, pausa al final y vuelve a empezar
unsigned CMenu::ScrollOffset (unsigned nExtra)
{
	const unsigned nPauseStart = 40, nStep = 5, nPauseEnd = 60;
	unsigned nCycle = nPauseStart + nExtra * nStep + nPauseEnd;
	unsigned t = m_nScrollTick % nCycle;

	if (t < nPauseStart)
	{
		return 0;
	}
	t -= nPauseStart;
	if (t < nExtra * nStep)
	{
		return t / nStep;
	}
	return nExtra;
}

void CMenu::Draw (TPixel *pScreen)
{
	if (m_nSelected != m_nScrollSelected)
	{
		m_nScrollSelected = m_nSelected;
		m_nScrollTick = 0;
	}
	m_nScrollTick++;

	if (m_Mode == ModeClosed)
	{
		return;
	}

	// panel centrado en la imagen (cabe en los bordes "completo" y "enorme")
	const int x0 = (SCREEN_WIDTH - PANEL_COLS * 8) / 2;
	const int y0 = (SCREEN_HEIGHT - PANEL_ROWS * 8) / 2;

	m_pVideo->FillRect (pScreen, x0 - 2, y0 - 2, PANEL_COLS * 8 + 4, PANEL_ROWS * 8 + 4, COLOR_UI_TEXT);
	m_pVideo->FillRect (pScreen, x0, y0, PANEL_COLS * 8, PANEL_ROWS * 8, COLOR_UI_PANEL);

	const int tx = x0 + 8;
	#define ROW_Y(n)	(y0 + (n) * 8)

	if (m_Mode == ModeHelp)
	{
		CString HelpTitle;
		HelpTitle.Format ("ZXMetalCircle %s - %s", ZXCIRCLE_VERSION, T ("Ayuda"));
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), HelpTitle, COLOR_UI_TITLE, COLOR_UI_PANEL);

		unsigned nRow = 3;
		for (unsigned i = 0; i < MENU_HELP_LINES; i++)
		{
			if (m_Help[i][0] != '\0')
			{
				m_pVideo->DrawText (pScreen, tx, ROW_Y (nRow++), m_Help[i],
						    COLOR_UI_TEXT, COLOR_UI_PANEL, TEXT_COLS);
			}
		}

		nRow++;
		for (unsigned i = 0; i < MENU_STATUS_LINES; i++)
		{
			if (m_Status[i][0] != '\0')
			{
				m_pVideo->DrawText (pScreen, tx, ROW_Y (nRow++), m_Status[i],
						    COLOR_UI_HILITE, COLOR_UI_PANEL, TEXT_COLS);
			}
		}

		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("ENTER cintas C conf. U unidades A acerca"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		return;
	}

	if (m_Mode == ModeUnits)
	{
		static const char *Labels[UNIT_MICRODRIVE] =
			{N_("Cinta"), N_("Disco A:"), N_("Disco B:"), N_("Disco duro"), N_("Cartucho IF2")};
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), T ("Unidades"), COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		for (unsigned i = 0; i < UNIT_COUNT; i++)
		{
			CString Label;
			if (i < UNIT_MICRODRIVE)
			{
				Label = T (Labels[i]);
			}
			else
			{
				Label.Format (T ("Microdrive %u"), i - UNIT_MICRODRIVE + 1);
			}
			// nombre sin la carpeta, en UTF-8 (la SD usa la página 850)
			const char *pName = m_UnitName[i];
			for (const char *p = m_UnitName[i]; *p != '\0'; p++)
			{
				if (*p == '/')
				{
					pName = p + 1;
				}
			}
			char Name[MENU_PATH_LEN];
			if (*pName == '\0')
			{
				strcpy (Name, i == UNIT_TAPE ? T ("(vacía)") : T ("(vacío)"));
			}
			else
			{
				CP850ToUTF8 (pName, Name, sizeof Name);
			}

			CString Line (Label);
			for (unsigned n = UTF8Length (Label); n < 14; n++)
			{
				Line.Append (" ");
			}
			Line.Append (Name);

			unsigned nRow = 3 + i + (i >= UNIT_MICRODRIVE ? 1 : 0);
			boolean bSel = i == m_nUnitSelected;
			if (bSel)
			{
				m_pVideo->FillRect (pScreen, tx - 2, ROW_Y (nRow) - 1, TEXT_COLS * 8 + 4, 10, COLOR_UI_HILITE);
			}
			m_pVideo->DrawText (pScreen, tx, ROW_Y (nRow), Line,
					    bSel ? COLOR_UI_PANEL : COLOR_UI_TEXT,
					    bSel ? COLOR_UI_HILITE : COLOR_UI_PANEL, TEXT_COLS);
		}
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 4), T ("Lo montado aquí vale para esta sesión."),
				    COLOR_UI_DIM, COLOR_UI_PANEL, TEXT_COLS);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("ENTER montar  SUPR vaciar  ESC volver"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		return;
	}

	if (m_Mode == ModeTape && m_pTape != 0)
	{
		char Name[MENU_PATH_LEN];
		CP850ToUTF8 (m_pTape->GetName (), Name, sizeof Name);
		CString Title;
		Title.Format (T ("Cinta: %s"), Name);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), Title, COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);

		unsigned nCount = m_pTape->GetBlockCount ();
		unsigned nCurrent = m_pTape->GetBlock ();
		for (unsigned i = 0; i < TAPE_LIST_LINES && m_nTapeTop + i < nCount; i++)
		{
			unsigned nBlock = m_nTapeTop + i;
			CString Line;
			boolean bHighlight;
			FormatTapeBlock (m_pTape, nBlock, &Line, &bHighlight);

			// ">" marca dónde está la cinta ahora
			CString Row;
			Row.Format ("%c%s", nBlock == nCurrent ? '>' : ' ', (const char *) Line);

			unsigned nRow = 3 + i;
			boolean bSel = nBlock == m_nTapeSelected;
			if (bSel)
			{
				m_pVideo->FillRect (pScreen, tx - 2, ROW_Y (nRow) - 1, TEXT_COLS * 8 + 4, 10, COLOR_UI_HILITE);
			}
			m_pVideo->DrawText (pScreen, tx, ROW_Y (nRow), Row,
					    bSel ? COLOR_UI_PANEL : (bHighlight ? COLOR_UI_TITLE : COLOR_UI_TEXT),
					    bSel ? COLOR_UI_HILITE : COLOR_UI_PANEL, TEXT_COLS);
		}

		CString Where;
		if (nCurrent < nCount)
		{
			Where.Format (T ("La cinta está en el bloque %u de %u"), nCurrent + 1, nCount);
		}
		else
		{
			Where.Format (T ("La cinta está al final (%u bloques)"), nCount);
		}
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 4), Where, COLOR_UI_DIM, COLOR_UI_PANEL, TEXT_COLS);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 3),
				    m_pTape->IsInverted () ? T ("P polaridad: INVERTIDA") : T ("P polaridad: normal"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);	// tecla que se puede pulsar
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("ENTER poner aquí  Inicio/Fin  ESC volver"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		return;
	}

	if (m_Mode == ModeAbout)
	{
		CString Title;
		Title.Format ("ZXMetalCircle %s", ZXCIRCLE_VERSION);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), Title, COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		for (unsigned i = 0; i < ABOUT_VISIBLE && m_nAboutTop + i < ABOUT_LINES; i++)
		{
			const char *pLine = T (AboutLines[m_nAboutTop + i]);
			boolean bHeading = pLine[0] != '\0';	// títulos: todo en mayúsculas
			for (const char *p = pLine; *p != '\0'; p++)
			{
				if (*p >= 'a' && *p <= 'z')
				{
					bHeading = FALSE;
				}
			}
			m_pVideo->DrawText (pScreen, tx, ROW_Y (3 + i), pLine,
					    bHeading ? COLOR_UI_TITLE : COLOR_UI_TEXT, COLOR_UI_PANEL, TEXT_COLS);
		}
		CString Pos;
		Pos.Format ("%u-%u/%u", m_nAboutTop + 1,
			    m_nAboutTop + ABOUT_VISIBLE < ABOUT_LINES ? m_nAboutTop + ABOUT_VISIBLE : (unsigned) ABOUT_LINES,
			    (unsigned) ABOUT_LINES);
		m_pVideo->DrawText (pScreen, tx + (TEXT_COLS - 9) * 8, ROW_Y (1), Pos, COLOR_UI_TEXT, COLOR_UI_PANEL, 9);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("Cursores y RePág/AvPág   ESC volver"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		return;
	}

	if (m_Mode == ModeInput)
	{
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), m_InputTitle, COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		CString Line;
		Line.Format ("> %s_", m_InputBuf);
		m_pVideo->FillRect (pScreen, tx - 2, ROW_Y (4) - 1, TEXT_COLS * 8 + 4, 10, COLOR_UI_HILITE);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (4), Line, COLOR_UI_PANEL, COLOR_UI_HILITE, TEXT_COLS);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (7), m_InputHelp[0], COLOR_UI_TEXT, COLOR_UI_PANEL, TEXT_COLS);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (8), m_InputHelp[1], COLOR_UI_TEXT, COLOR_UI_PANEL, TEXT_COLS);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("ENTER pulsar  BORRAR corregir  ESC salir"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		return;
	}

	if (m_Mode == ModeChoice)
	{
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), m_ChoiceTitle, COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		for (unsigned i = 0; i < m_nChoices; i++)
		{
			CString Line;
			Line.Format ("%u %s", i + 1, m_Choice[i]);
			boolean bSel = i == m_nChoiceSelected;
			if (bSel)
			{
				m_pVideo->FillRect (pScreen, tx - 2, ROW_Y (3 + 2 * i) - 1, TEXT_COLS * 8 + 4, 10, COLOR_UI_HILITE);
			}
			m_pVideo->DrawText (pScreen, tx, ROW_Y (3 + 2 * i), Line,
					    bSel ? COLOR_UI_PANEL : COLOR_UI_TEXT,
					    bSel ? COLOR_UI_HILITE : COLOR_UI_PANEL, TEXT_COLS);
		}
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("ENTER o 1-9: elegir   ESC: seguir"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		return;
	}

	if (m_Mode == ModeSettings)
	{
		CString Title;
		Title.Format (T ("Configuración %u/%u: %s"), m_nSettingsPage + 1, CSettings::GetPageCount (),
			      CSettings::GetPageTitle (m_nSettingsPage));
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), Title, COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);

		unsigned Index[SETTINGS_MAX];
		unsigned nRows = SettingsOnPage (Index, SETTINGS_MAX);
		boolean bRestart = FALSE;
		boolean bSelApplies = TRUE;
		for (unsigned i = 0; i < nRows; i++)
		{
			const TSettingDef &Def = m_pSettings->GetDef (Index[i]);
			// etiqueta rellenada a 21 caracteres (no bytes: puede llevar acentos)
			const char *pLabel = T (Def.pLabel);
			CString Line;
			Line.Format ("%c%s", Def.bLive ? ' ' : '*', pLabel);
			for (unsigned n = UTF8Length (pLabel); n < 22; n++)
			{
				Line.Append (" ");
			}
			if (m_bEditing && i == m_nSettingsRow)
			{
				Line.Append (m_EditBuf);	// escribiendo: lo escrito y el cursor
				Line.Append ("_");
			}
			else
			{
				Line.Append (m_pSettings->GetValueName (Index[i]));
			}
			bRestart |= !Def.bLive;
			boolean bSel = i == m_nSettingsRow;
			// en gris lo que no se usa con lo elegido (se puede cambiar igual)
			boolean bApplies = m_pSettings->IsApplicable (Index[i]);
			if (bSel)
			{
				m_pVideo->FillRect (pScreen, tx - 2, ROW_Y (3 + 2 * i) - 1, TEXT_COLS * 8 + 4, 10, COLOR_UI_HILITE);
				bSelApplies = bApplies;
			}
			m_pVideo->DrawText (pScreen, tx, ROW_Y (3 + 2 * i), Line,
					    bSel ? COLOR_UI_PANEL : (bApplies ? COLOR_UI_TEXT : COLOR_UI_DIM),
					    bSel ? COLOR_UI_HILITE : COLOR_UI_PANEL, TEXT_COLS);
		}

		unsigned nRow = 4 + 2 * nRows;
		if (bRestart)
		{
			m_pVideo->DrawText (pScreen, tx, ROW_Y (nRow), T ("* se aplica al reiniciar la Pi"),
					    COLOR_UI_TEXT, COLOR_UI_PANEL, TEXT_COLS);
		}
		if (m_pSettings->IsDirty ())
		{
			m_pVideo->DrawText (pScreen, tx, ROW_Y (nRow + 2), T ("Hay cambios sin guardar (G)"),
					    COLOR_UI_HILITE, COLOR_UI_PANEL, TEXT_COLS);
		}
		if (!bSelApplies && !m_bEditing)
		{
			m_pVideo->DrawText (pScreen, tx, ROW_Y (nRow + 4), T ("No se usa con lo elegido ahora"),
					    COLOR_UI_DIM, COLOR_UI_PANEL, TEXT_COLS);
		}

		if (m_bEditing)
		{
			boolean bPass = m_nSettingsRow < nRows && CSettings::IsPassword (Index[m_nSettingsRow]);
			m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 6),
					    bPass ? T ("Letras y números; SUPR la quita")
					    : m_bEditBad ? T ("No es una IP válida (p. ej. 192.168.1.50)")
							 : T ("Números y punto; SUPR deja automática"),
					    m_bEditBad ? COLOR_UI_TITLE : COLOR_UI_DIM, COLOR_UI_PANEL, TEXT_COLS);
			m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 4), T ("ENTER aceptar   ESC dejarla como estaba"),
					    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		}
		else
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 4), T ("<- -> cambiar   RePág/AvPág página"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("G guardar  R reiniciar la Pi  ESC volver"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
		return;
	}

	if (m_Mode == ModeKeyHelp)
	{
		static const char *KeyHelpLines[] =
		{
			N_("Teclado del PC     Spectrum"),
			"",
			N_("Mayúsculas         CAPS SHIFT"),
			"Ctrl               SYMBOL SHIFT",
			"Tab                EXTENDED MODE",
			"Esc                BREAK",
			N_("Retroceso          DELETE"),
			N_("Bloq Mayús         CAPS LOCK"),
			N_("Cursores           CAPS SHIFT + 5-8"),
			N_(", . - + ' < >      SYMBOL SHIFT + tecla"),
			N_("Teclado numérico   números y + - * / ."),
			"",
			N_("Con joystick activo, los cursores"),
			N_("son a la vez CS+5-8 y joystick"),
			N_("Ctrl der. / 0 num. disparo"),
			"",
			N_("Teclado real (GPIO) o Recreated ZX:"),
			N_("botón o CS+SS+ENTER = modo extendido"),
			"(1-0 = F1-F10, SYMBOL SHIFT = Alt)",
			N_("Botón NMI: GPIO16 (pin 36)"),
			"",
			N_("Mando USB: Select cambia de joystick,"),
			N_("Start abre la ayuda"),
			N_("Ratón USB = ratón Kempston (si el"),
			N_("joystick no es Kempston)"),
		};

		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), T ("ZXMetalCircle - Ayuda de teclado"), COLOR_UI_TITLE, COLOR_UI_PANEL);

		unsigned nRow = 3;
		for (unsigned i = 0; i < sizeof KeyHelpLines / sizeof KeyHelpLines[0]; i++)
		{
			m_pVideo->DrawText (pScreen, tx, ROW_Y (nRow++), T (KeyHelpLines[i]),
					    i == 0 ? COLOR_UI_HILITE : COLOR_UI_TEXT, COLOR_UI_PANEL, TEXT_COLS);
		}

		m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("ESC/ENTER: volver"),
				    COLOR_UI_DIM, COLOR_UI_PANEL);
		return;
	}

	// Navegador
	if (m_nUnitTarget >= 0)
	{
		static const char *Titles[UNIT_MICRODRIVE + 1] =
			{N_("Elige una cinta"), N_("Elige un disco para A:"), N_("Elige un disco para B:"),
			 N_("Elige una imagen de disco duro"), N_("Elige un cartucho del IF2"),
			 N_("Elige un cartucho de microdrive")};
		unsigned n = m_nUnitTarget < UNIT_MICRODRIVE ? m_nUnitTarget : UNIT_MICRODRIVE;
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), T (Titles[n]), COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
	}
	else
	{
		m_pVideo->DrawText (pScreen, tx, ROW_Y (1), T ("ZXMetalCircle - Cintas y snapshots"),
				    COLOR_UI_TITLE, COLOR_UI_PANEL);
	}

	// Ruta actual (se recorta por la izquierda si es larga)
	const char *pDir = m_CurrentDir;
	size_t nDirLen = strlen (pDir);
	if (nDirLen > TEXT_COLS)
	{
		pDir += nDirLen - TEXT_COLS;
	}
	// los errores (de la SD o de un servidor TNFS) se muestran en lugar de la ruta
	if (m_Error[0] != '\0')
	{
		m_pVideo->DrawText (pScreen, tx, ROW_Y (2), m_Error, COLOR_UI_TITLE, COLOR_UI_PANEL, TEXT_COLS);
	}
	else
	{
		char DirUTF8[MENU_PATH_LEN * 2];
		CP850ToUTF8 (pDir, DirUTF8, sizeof DirUTF8);
		m_pVideo->DrawText (pScreen, tx, ROW_Y (2), DirUTF8, COLOR_UI_DIM, COLOR_UI_PANEL, TEXT_COLS);
	}

	if (m_nEntries == 0)
	{
		m_pVideo->DrawText (pScreen, tx, ROW_Y (LIST_ROW), T ("(no hay ficheros que cargar)"),
				    COLOR_UI_DIM, COLOR_UI_PANEL);
	}

	for (unsigned nLine = 0; nLine < LIST_LINES; nLine++)
	{
		unsigned i = m_nTop + nLine;
		if (i >= m_nEntries)
		{
			break;
		}

		const TEntry *pEntry = &m_pEntries[m_Order[i]];
		boolean bSelected = i == m_nSelected;

		u8 nPaper = bSelected ? COLOR_UI_HILITE : COLOR_UI_PANEL;
		u8 nInk = bSelected ? COLOR_UI_HILITE_TEXT : (pEntry->bDir ? COLOR_UI_DIR : COLOR_UI_TEXT);

		// Texto de la entrada; los directorios van entre corchetes
		char Full[MENU_NAME_LEN + 3];
		unsigned nFull = 0;
		if (pEntry->bDir)
		{
			Full[nFull++] = '[';
		}
		for (const char *p = pEntry->Name; *p != '\0'; p++)
		{
			Full[nFull++] = *p;
		}
		if (pEntry->bDir)
		{
			Full[nFull++] = ']';
		}
		Full[nFull] = '\0';

		// Si no cabe, la entrada seleccionada se desplaza poco a poco
		unsigned nStart = 0;
		if (bSelected && nFull > TEXT_COLS)
		{
			nStart = ScrollOffset (nFull - TEXT_COLS);
		}

		char Line[TEXT_COLS + 1];
		unsigned n = 0;
		for (const char *p = Full + nStart; *p != '\0' && n < TEXT_COLS; p++)
		{
			Line[n++] = *p;
		}
		if (!bSelected && nFull > TEXT_COLS)
		{
			Line[TEXT_COLS - 1] = '>';	// indica que el nombre sigue
		}
		while (n < TEXT_COLS)
		{
			Line[n++] = ' ';	// rellena para que la barra ocupe todo el ancho
		}
		Line[n] = '\0';

		// los nombres de la SD vienen en la página de códigos 850 de FatFs
		char UTF8[TEXT_COLS * 3 + 1];
		if (strncmp (m_CurrentDir, NET_ROOT, 4) != 0)
		{
			CP850ToUTF8 (Line, UTF8, sizeof UTF8);
		}
		else
		{
			strcpy (UTF8, Line);
		}
		m_pVideo->DrawText (pScreen, tx, ROW_Y (LIST_ROW + nLine), UTF8, nInk, nPaper);
	}

	// Barra de posición
	if (m_nEntries > LIST_LINES)
	{
		char Pos[16];
		unsigned nPos = m_nSelected + 1;
		unsigned nLen = 0;
		char Digits[8];
		unsigned nDigits = 0;
		do { Digits[nDigits++] = '0' + nPos % 10; nPos /= 10; } while (nPos > 0);
		while (nDigits > 0) Pos[nLen++] = Digits[--nDigits];
		Pos[nLen++] = '/';
		nPos = m_nEntries;
		do { Digits[nDigits++] = '0' + nPos % 10; nPos /= 10; } while (nPos > 0);
		while (nDigits > 0) Pos[nLen++] = Digits[--nDigits];
		Pos[nLen] = '\0';

		m_pVideo->DrawText (pScreen, x0 + (PANEL_COLS - 1 - nLen) * 8, ROW_Y (2), Pos,
				    COLOR_UI_DIM, COLOR_UI_PANEL);
	}

	m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 3), T ("ENTER cargar  ESC salir  BORRAR subir"),
			    COLOR_UI_DIM, COLOR_UI_PANEL, TEXT_COLS);
	m_pVideo->DrawText (pScreen, tx, ROW_Y (PANEL_ROWS - 2), T ("Letra: saltar  RePág/AvPág: páginas"),
			    COLOR_UI_DIM, COLOR_UI_PANEL, TEXT_COLS);
}
