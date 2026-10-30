//
// menu.h - Navegador de ficheros y pantalla de ayuda
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_menu_h
#define _zx_menu_h

#include "video.h"
#include "zxkeyboard.h"
#include "tnfsbrowser.h"
#include <circle/types.h>
#include "settings.h"
#include "tape.h"

#define MENU_MAX_ENTRIES	2048
#define MENU_NAME_LEN		128
#define MENU_PATH_LEN		512
#define MENU_STATUS_LINES	9
#define MENU_HELP_LINES		23
#define MENU_OPTION_LINES	6
#define MENU_CHOICE_ITEMS	14

// Unidades de la pantalla de unidades
#define UNIT_TAPE		0
#define UNIT_DISK_A		1
#define UNIT_DISK_B		2
#define UNIT_HARDDISK		3
#define UNIT_CARTRIDGE		4
#define UNIT_MICRODRIVE		5	// 5-12: microdrives 1-8
#define UNIT_COUNT		13

enum TMenuResult
{
	MenuResultNone,
	MenuResultClose,
	MenuResultFileSelected,
	MenuResultOpenSettings,		// C en la ayuda: el kernel actualiza y abre la configuración
	MenuResultOpenUnits,		// U en la ayuda: el kernel pone al día las unidades y las abre
	MenuResultSetting,		// se ha cambiado una opción (GetChangedSetting ())
	MenuResultSaveSettings,		// G en la configuración: guardar en cmdline.txt
	MenuResultReboot,		// R en la configuración: reiniciar la Pi (se pide confirmación)
	MenuResultChoice,		// lista de elección: ENTER (GetChoice ())
	MenuResultChoiceCancel,		// lista de elección: ESC
	MenuResultUnitEject,		// SUPR en la pantalla de unidades (GetSelectedUnit ())
	MenuResultTapeSeek,		// ENTER en el navegador de la cinta (GetSelectedTapeBlock ())
	MenuResultTapeInvert,		// P en el navegador de la cinta: invertir la polaridad
	MenuResultInput,		// línea de texto: ENTER (GetInput ())
	MenuResultInputCancel		// línea de texto: ESC
};

class CMenu : public CTNFSListener
{
public:
	CMenu (CVideo *pVideo);
	~CMenu (void);

	boolean OpenBrowser (void);
	void OpenHelp (void);			// ayuda general (F1); ENTER abre el selector
	void OpenKeyHelp (void);		// ayuda del teclado (Alt+K)
	void SetSettings (CSettings *pSettings)	{ m_pSettings = pSettings; }
	void OpenSettings (void);		// configuración (C desde la ayuda)
	unsigned GetChangedSetting (void) const	{ return m_nChangedSetting; }

	// Pantalla de unidades (U desde la ayuda, Alt+U): cinta, discos A: y B:,
	// disco duro, cartucho del Interface 2 y microdrives 1-8. ENTER abre el
	// selector solo con los ficheros de esa unidad (lo elegido se devuelve con
	// MenuResultFileSelected y TakeUnitTarget () dice la unidad); SUPR la vacía.
	void OpenUnits (void);
	void OpenTape (const CTape *pTape);	// navegador de bloques de la cinta (F3)
	unsigned GetSelectedTapeBlock (void) const	{ return m_nTapeSelected; }
	void SetUnitLine (unsigned nUnit, const char *pName);
	unsigned GetSelectedUnit (void) const	{ return m_nUnitSelected; }
	int TakeUnitTarget (void)		{ int n = m_nUnitTarget; m_nUnitTarget = -1; return n; }

	// Lista para elegir una opción (por ejemplo, el bloque 0x28 de una cinta)
	void OpenChoice (const char *pTitle);
	void SetChoiceItem (unsigned nItem, const char *pText);
	unsigned GetChoice (void) const		{ return m_nChoiceSelected; }

	// Línea de texto (letras, números y espacio): título y dos líneas de
	// explicación; ENTER da MenuResultInput y ESC MenuResultInputCancel
	void OpenInput (const char *pTitle, const char *pHelp1, const char *pHelp2);
	const char *GetInput (void) const	{ return m_InputBuf; }
	void Close (void)		{ m_Mode = ModeClosed; }
	boolean IsOpen (void) const	{ return m_Mode != ModeClosed; }

	TMenuResult HandleKey (const TKeyEvent &rEvent);
	void Draw (TPixel *pScreen);

	const char *GetSelectedPath (void) const	{ return m_SelectedPath; }

	// Servidores TNFS (entrada "Internet (TNFS)" en la raíz de la SD)
	void SetRemoteSource (CRemoteSource *pRemote)	{ m_pRemote = pRemote; }
	void OnEntry (const char *pName, boolean bDir) override;	// CTNFSListener

	// Líneas de estado que se muestran en la ayuda (modelo, joystick, cinta...)
	void SetStatusLine (unsigned nLine, const char *pText);

	// Líneas de la ayuda general (teclas de función), generadas por el kernel
	void SetHelpLine (unsigned nLine, const char *pText);

private:
	boolean ReadDirectory (void);
	boolean ReadRemoteDirectory (void);
	boolean IsRemote (void) const;
	void SortEntries (void);
	void EnterSelected (TMenuResult *pResult);
	void GoParent (void);
	void JumpToLetter (char chLetter);
	void EnsureVisible (void);
	unsigned ScrollOffset (unsigned nExtra);

private:
	CVideo *m_pVideo;

	enum TMode { ModeClosed, ModeBrowser, ModeHelp, ModeKeyHelp, ModeSettings, ModeChoice, ModeUnits,
		     ModeAbout, ModeTape, ModeInput };
	unsigned SettingsOnPage (unsigned *pIndex, unsigned nMax) const;
	void FixSettingsRow (void);
	TMode m_Mode;

	struct TEntry
	{
		char Name[MENU_NAME_LEN];
		boolean bDir;
		boolean bRemoteRoot;	// "Internet (TNFS)" en la raíz de la SD
	};
	CRemoteSource *m_pRemote;

	TEntry *m_pEntries;
	u16 m_Order[MENU_MAX_ENTRIES];
	unsigned m_nEntries;
	unsigned m_nSelected;
	unsigned m_nTop;
	unsigned m_nScrollTick;		// desplazamiento de nombres largos
	unsigned m_nScrollSelected;

	char m_CurrentDir[MENU_PATH_LEN];
	char m_SelectedPath[MENU_PATH_LEN];
	char m_Error[64];

	char m_Status[MENU_STATUS_LINES][48];
	char m_Help[MENU_HELP_LINES][48];
	CSettings *m_pSettings;
	unsigned m_nSettingsPage;
	unsigned m_nSettingsRow;
	unsigned m_nChangedSetting;
	char m_ChoiceTitle[48];
	char m_InputTitle[48];
	char m_InputHelp[2][48];
	char m_InputBuf[20];		// lo escrito en la línea de texto
	char m_Choice[MENU_CHOICE_ITEMS][48];
	unsigned m_nChoices;
	unsigned m_nChoiceSelected;
	unsigned m_nAboutTop;		// primera línea visible de los créditos
	char m_UnitName[UNIT_COUNT][MENU_PATH_LEN];
	unsigned m_nUnitSelected;
	int m_nUnitTarget;		// unidad para la que se ha abierto el selector (-1 = ninguna)
	const CTape *m_pTape;		// navegador de la cinta
	unsigned m_nTapeSelected;
	unsigned m_nTapeTop;
	boolean m_bEditing;		// escribiendo una opción (IP) de la configuración
	boolean m_bEditBad;		// lo escrito no es una IP válida
	char m_EditBuf[16];
};

#endif
