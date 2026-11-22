//
// kernelfiles.cpp
//
// F2 en el navegador de ficheros: renombrar, borrar y crear carpetas en la SD,
// con las mismas comprobaciones que la web:
//
// - Lo del sistema (kernels, cmdline.txt, config.txt, las carpetas del
//   emulador...) no se toca mientras esté protegido (webprotect=1).
// - Lo que el emulador está usando (un disco del +3, un microdrive, el disco
//   duro, la cinta si SAVE graba en ella) no se renombra ni se borra.
// - Las carpetas solo se borran vacías, y borrar siempre pregunta (con "No"
//   elegido al entrar).
// - Solo en la SD de la Pi (no en TNFS) y no al elegir para una unidad.
//   filechanges=0 lo quita todo (F2 no hace nada).
//
// Con el teclado GPIO: F2 es modo extendido + 2; al escribir, CAPS SHIFT +
// letra es la mayúscula y SYMBOL SHIFT + tecla el símbolo (ver
// HandleGPIOKeyboard y CMenu::OpenText).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include "i18n.h"
#include "webfiles.h"
#include <circle/string.h>
#include <circle/util.h>
#include <fatfs/ff.h>
#include <string.h>

enum
{
	FileStageNone,
	FileStageOptions,	// la lista de F2
	FileStageRename,	// el nombre nuevo
	FileStageNewFolder,	// el nombre de la carpeta
	FileStageDelete		// ¿borrar?
};

enum				// las líneas de la lista de F2
{
	FileOptionRename,
	FileOptionDelete,
	FileOptionNewFolder
};

// La ruta de un nombre en una carpeta del navegador
static void JoinPath (const char *pDir, const char *pName, CString *pPath)
{
	size_t nLen = strlen (pDir);
	pPath->Format (nLen > 0 && pDir[nLen - 1] == '/' ? "%s%s" : "%s/%s", pDir, pName);
}

static boolean OnSD (const char *pPath)
{
	return strncasecmp (pPath, "SD:", 3) == 0;
}

// Lo del sistema, si está protegido (como en la web: webprotect)
boolean CKernel::FileProtected (const char *pPath)
{
	int nProtect = m_Settings.Find ("webprotect");
	boolean bProtect = nProtect < 0 || strcmp (m_Settings.GetValue (nProtect), "0") != 0;
	return bProtect && OnSD (pPath) && WebIsSystem (pPath);
}

// Por qué no se puede renombrar ni borrar pPath (0 si se puede)
const char *CKernel::FileCheckSource (const char *pPath)
{
	if (FileProtected (pPath))
	{
		return T ("Protegido: %s");
	}
	if (IsPathInUse (pPath))
	{
		return T ("En uso: %s (sácalo antes)");
	}
	return 0;
}

// Vuelve al navegador con un aviso (pFormat con un %s: el nombre)
void CKernel::FileBack (const char *pSelect, const char *pFormat, const char *pName)
{
	m_nFileStage = FileStageNone;
	CString Message;
	if (pFormat != 0)
	{
		char UTF8[MENU_NAME_LEN * 2];
		CP850ToUTF8 (pName != 0 ? pName : "", UTF8, sizeof UTF8);
		Message.Format (pFormat, UTF8);
	}
	m_Menu.ReturnToBrowser (pSelect, Message);
}

void CKernel::FileShowOptions (void)
{
	boolean bIsDir;
	m_bFileHasEntry = m_Menu.GetBrowserSelection (m_FileDir, sizeof m_FileDir, m_FileName,
						     sizeof m_FileName, &bIsDir);
	m_bFileIsDir = bIsDir;

	char UTF8[MENU_NAME_LEN * 2];
	CP850ToUTF8 (m_bFileHasEntry ? m_FileName : m_FileDir, UTF8, sizeof UTF8);

	m_nFileStage = FileStageOptions;
	m_Menu.OpenChoice (UTF8);
	m_nFileOptions = 0;
	if (m_bFileHasEntry)			// en ".." solo se puede crear una carpeta
	{
		m_FileOption[m_nFileOptions] = FileOptionRename;
		m_Menu.SetChoiceItem (m_nFileOptions++, T ("Renombrar..."));
		m_FileOption[m_nFileOptions] = FileOptionDelete;
		m_Menu.SetChoiceItem (m_nFileOptions++, T ("Borrar..."));
	}
	m_FileOption[m_nFileOptions] = FileOptionNewFolder;
	m_Menu.SetChoiceItem (m_nFileOptions++, T ("Nueva carpeta..."));
}

void CKernel::FileAskRename (const char *pHelp, const char *pText)
{
	// lo escrito (si no valía) o el nombre de ahora, con el cursor delante de
	// la extensión (lo normal es cambiar el nombre)
	const char *pInitial = pText != 0 ? pText : m_FileName;
	unsigned nLen = strlen (pInitial);
	unsigned nCursor = nLen;
	if (!m_bFileIsDir && pText == 0)
	{
		for (unsigned i = nLen; i-- > 1; )
		{
			if (pInitial[i] == '.')
			{
				nCursor = i;
				break;
			}
		}
	}
	m_nFileStage = FileStageRename;
	m_Menu.OpenText (T ("Renombrar"), pHelp != 0 ? pHelp : T ("Mayúsculas: CAPS SHIFT + letra"),
			 T ("Símbolos ( ) . - _: SYMBOL SHIFT + tecla"), T ("ENTER cambiar   ESC volver"),
			 pInitial, nCursor);
}

void CKernel::FileAskNewFolder (const char *pHelp, const char *pText)
{
	m_nFileStage = FileStageNewFolder;
	m_Menu.OpenText (T ("Nueva carpeta"), pHelp != 0 ? pHelp : T ("Mayúsculas: CAPS SHIFT + letra"),
			 T ("Símbolos ( ) . - _: SYMBOL SHIFT + tecla"), T ("ENTER crear   ESC volver"),
			 pText != 0 ? pText : "", pText != 0 ? strlen (pText) : 0);
}

void CKernel::FileAskDelete (void)
{
	char UTF8[MENU_NAME_LEN * 2];
	CP850ToUTF8 (m_FileName, UTF8, sizeof UTF8);
	CString Title;
	Title.Format (m_bFileIsDir ? T ("¿Borrar la carpeta %s?") : T ("¿Borrar %s?"), UTF8);
	m_nFileStage = FileStageDelete;
	m_Menu.OpenChoice (Title);
	m_Menu.SetChoiceItem (0, T ("No, dejarlo"));
	m_Menu.SetChoiceItem (1, T ("Sí, borrarlo (no se puede deshacer)"));
	m_Menu.SetChoiceSelected (0);
}

void CKernel::FileRename (const char *pNewName)
{
	if (strcmp (pNewName, m_FileName) == 0)
	{
		FileBack (m_FileName, 0, 0);		// sin cambios
		return;
	}
	if (!WebValidName (pNewName))
	{
		FileAskRename (T ("No vale: sin punto ni espacio al final"), pNewName);
		return;
	}

	CString From, To;
	JoinPath (m_FileDir, m_FileName, &From);
	JoinPath (m_FileDir, pNewName, &To);

	const char *pError = FileCheckSource (From);
	if (pError != 0)
	{
		FileBack (m_FileName, pError, m_FileName);
		return;
	}
	if (FileProtected (To))
	{
		FileBack (m_FileName, T ("Protegido: %s"), pNewName);
		return;
	}
	// ya hay otro con ese nombre (si solo cambian mayúsculas, es el mismo)
	FILINFO Info;
	if (strcasecmp (pNewName, m_FileName) != 0 && f_stat (To, &Info) == FR_OK)
	{
		FileAskRename (T ("Ya hay uno con ese nombre"), pNewName);
		return;
	}

	if (f_rename (From, To) != FR_OK)
	{
		FileBack (m_FileName, T ("No se pudo renombrar %s"), m_FileName);
		return;
	}
	FileBack (pNewName, T ("Renombrado: %s"), pNewName);
}

void CKernel::FileNewFolder (const char *pName)
{
	if (pName[0] == '\0')
	{
		FileBack (0, 0, 0);
		return;
	}
	if (!WebValidName (pName))
	{
		FileAskNewFolder (T ("No vale: sin punto ni espacio al final"), pName);
		return;
	}
	CString Path;
	JoinPath (m_FileDir, pName, &Path);
	if (OnSD (m_FileDir) && FileProtected (Path))
	{
		FileBack (0, T ("Protegido: aquí no se puede crear %s"), pName);
		return;
	}
	{
		// dentro de una carpeta del sistema (locales, firmware...), nada nuevo
		int nProtect = m_Settings.Find ("webprotect");
		if (   (nProtect < 0 || strcmp (m_Settings.GetValue (nProtect), "0") != 0)
		    && OnSD (m_FileDir) && WebIsSystemFolder (m_FileDir))
		{
			FileBack (0, T ("Protegido: aquí no se puede crear %s"), pName);
			return;
		}
	}
	FILINFO Info;
	if (f_stat (Path, &Info) == FR_OK)
	{
		FileAskNewFolder (T ("Ya hay uno con ese nombre"), pName);
		return;
	}
	if (f_mkdir (Path) != FR_OK)
	{
		FileBack (0, T ("No se pudo crear %s"), pName);
		return;
	}
	FileBack (pName, T ("Carpeta creada: %s"), pName);
}

void CKernel::FileDelete (void)
{
	CString Path;
	JoinPath (m_FileDir, m_FileName, &Path);
	const char *pError = FileCheckSource (Path);
	if (pError != 0)
	{
		FileBack (m_FileName, pError, m_FileName);
		return;
	}

	FRESULT Result = f_unlink (Path);
	if (Result == FR_OK)
	{
		FileBack (0, T ("Borrado: %s"), m_FileName);
	}
	else if (m_bFileIsDir && Result == FR_DENIED)
	{
		FileBack (m_FileName, T ("La carpeta no está vacía: %s"), m_FileName);
	}
	else
	{
		FileBack (m_FileName, T ("No se pudo borrar %s"), m_FileName);
	}
}

// Las respuestas del menú de F2; TRUE si eran suyas
boolean CKernel::FileMenuResult (TMenuResult Result)
{
	if (Result == MenuResultFileOptions)
	{
		FileShowOptions ();
		return TRUE;
	}

	switch (m_nFileStage)
	{
	case FileStageOptions:
		if (Result == MenuResultChoice && m_Menu.GetChoice () < m_nFileOptions)
		{
			switch (m_FileOption[m_Menu.GetChoice ()])
			{
			case FileOptionRename:
				{
					CString Path;
					JoinPath (m_FileDir, m_FileName, &Path);
					const char *pError = FileCheckSource (Path);
					if (pError != 0)
					{
						FileBack (m_FileName, pError, m_FileName);	// antes de escribir nada
					}
					else
					{
						FileAskRename (0, 0);
					}
				}
				break;

			case FileOptionDelete:
				{
					CString Path;
					JoinPath (m_FileDir, m_FileName, &Path);
					const char *pError = FileCheckSource (Path);
					if (pError != 0)
					{
						FileBack (m_FileName, pError, m_FileName);
					}
					else
					{
						FileAskDelete ();
					}
				}
				break;

			default:
				FileAskNewFolder (0, 0);
				break;
			}
			return TRUE;
		}
		if (Result == MenuResultChoiceCancel)
		{
			FileBack (m_bFileHasEntry ? m_FileName : 0, 0, 0);	// ESC: al navegador
			return TRUE;
		}
		return FALSE;

	case FileStageRename:
		if (Result == MenuResultInput)
		{
			char Name[MENU_NAME_LEN];
			strncpy (Name, m_Menu.GetInput (), sizeof Name - 1);
			Name[sizeof Name - 1] = '\0';
			FileRename (Name);
			return TRUE;
		}
		if (Result == MenuResultInputCancel)
		{
			FileBack (m_FileName, 0, 0);
			return TRUE;
		}
		return FALSE;

	case FileStageNewFolder:
		if (Result == MenuResultInput)
		{
			char Name[MENU_NAME_LEN];
			strncpy (Name, m_Menu.GetInput (), sizeof Name - 1);
			Name[sizeof Name - 1] = '\0';
			FileNewFolder (Name);
			return TRUE;
		}
		if (Result == MenuResultInputCancel)
		{
			FileBack (m_bFileHasEntry ? m_FileName : 0, 0, 0);
			return TRUE;
		}
		return FALSE;

	case FileStageDelete:
		if (Result == MenuResultChoice && m_Menu.GetChoice () == 1)
		{
			FileDelete ();
			return TRUE;
		}
		if (Result == MenuResultChoice || Result == MenuResultChoiceCancel)
		{
			FileBack (m_FileName, 0, 0);		// "No" o ESC: no se borra
			return TRUE;
		}
		return FALSE;
	}

	return FALSE;
}
