//
// kerneltape.cpp
//
// F2 en el navegador de la cinta (F3): copiar, mover, borrar y renombrar
// bloques. Los cambios se hacen en la cinta en memoria (CTape) y el fichero
// no se toca hasta guardar, al salir del navegador:
//
// - Una cabecera y sus datos van juntos (con "Solo este bloque" se elige uno).
// - Copiar y Mover marcan los bloques (abajo se ve "Para copiar: ..."); en el
//   bloque de destino, F2 > Pegar delante o detrás de este. Lo marcado se
//   guarda (una copia), así que se puede pegar también en otra cinta: se pone
//   la otra y F2 en su navegador. Mover, solo dentro de la misma; en otra se
//   pega una copia. De un TAP a un TZX, cada bloque pasa a ser uno estándar
//   (0x10, con 1 s de pausa); de un TZX a un TAP solo van los estándar: si
//   hay alguno más (turbo, tonos, textos...), no se pega nada y se avisa.
// - Borrar pregunta, con "No" elegido al entrar.
// - Al salir (ESC, F3 o ENTER) con cambios: seguir editando, guardar en el
//   mismo fichero, guardar como otro o descartarlos. Se guarda con un nombre
//   temporal y solo al terminar se le pone el suyo; el original se queda como
//   "nombre.tap.bak" (la primera vez: luego ya está guardado el original).
// - Solo TAP y TZX sin saltos, bucles ni llamadas (0x23-0x28: cuentan
//   bloques y moverlos los descolocaría). Ni CSW ni PZX.
// - filechanges=0 lo quita (F2 no hace nada), como en el navegador.
//
// Con el teclado GPIO: F2 es modo extendido + 2, y al escribir el nombre,
// CAPS SHIFT + letra es la mayúscula y SYMBOL SHIFT + tecla el símbolo.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include "i18n.h"
#include "webfiles.h"
#include "filecopy.h"
#include <circle/string.h>
#include <circle/util.h>
#include <fatfs/ff.h>
#include <string.h>

enum
{
	TapeStageNone,
	TapeStageOptions,	// la lista de F2
	TapeStageDelete,	// ¿borrar?
	TapeStageRename,	// el nombre de la cabecera
	TapeStageLeave,		// salir con cambios: ¿guardar?
	TapeStageSaveAs		// el nombre del fichero
};

enum				// las líneas de la lista de F2
{
	TapeOptionCopy,
	TapeOptionMove,
	TapeOptionRename,
	TapeOptionDelete,
	TapeOptionSingle,	// solo este bloque (no la cabecera con sus datos)
	TapeOptionPasteBefore,
	TapeOptionPasteAfter,
	TapeOptionForget
};

enum				// salir con cambios
{
	TapeLeaveContinue,
	TapeLeaveSave,
	TapeLeaveSaveAs,
	TapeLeaveDiscard
};

#define TAPE_TEMP_NAME		"~zxcinta.tmp"		// mientras se guarda

static boolean HasTapeExtension (const char *pPath, boolean bTZX)
{
	size_t nLen = strlen (pPath);
	return nLen > 4 && strcasecmp (pPath + nLen - 4, bTZX ? ".tzx" : ".tap") == 0;
}

// Se puede guardar en el mismo fichero: uno de la SD con su extensión (no
// un .zip ni una grabación de Alt+W)
boolean CKernel::TapeCanSaveInPlace (void)
{
	return    strncasecmp (m_TapePath, "SD:", 3) == 0
	       && HasTapeExtension (m_TapePath, m_Tape.IsTZX ())
	       && !FileProtected (m_TapePath);
}

// "Bloques 3-4" / "Bloque 5"
static void BlockRange (unsigned nFirst, unsigned nCount, CString *pText)
{
	if (nCount > 1)
	{
		pText->Format (T ("bloques %u-%u"), nFirst + 1, nFirst + nCount);
	}
	else
	{
		pText->Format (T ("bloque %u"), nFirst + 1);
	}
}

// Vuelta al navegador de la cinta con nBlock elegido y un aviso
void CKernel::TapeBack (unsigned nBlock, const char *pMessage)
{
	m_nTapeStage = TapeStageNone;
	m_Menu.OpenTape (&m_Tape, (int) nBlock, pMessage);
}

// Lo marcado es de la cinta puesta (y sus números siguen valiendo): solo
// así se puede mover; si no, se pega una copia
boolean CKernel::TapeClipSame (void)
{
	return m_nTapeClipCount > 0 && m_nTapeClipSerial == m_Tape.GetSerial ();
}

void CKernel::TapeUpdateClip (void)
{
	if (m_nTapeClipCount == 0)
	{
		m_Menu.SetTapeClip ("");
		return;
	}
	CString Text;
	if (TapeClipSame ())
	{
		CString Range;
		BlockRange (m_nTapeClipFirst, m_nTapeClipCount, &Range);
		Text.Format (m_bTapeClipMove ? T ("Para mover: %s") : T ("Para copiar: %s"), (const char *) Range);
	}
	else
	{
		char UTF8[sizeof m_TapeClipName * 2];
		CP850ToUTF8 (m_TapeClipName, UTF8, sizeof UTF8);
		if (m_nTapeClipCount > 1)
		{
			Text.Format (T ("Para copiar: %u bloques de %s"), m_nTapeClipCount, UTF8);
		}
		else
		{
			Text.Format (T ("Para copiar: un bloque de %s"), UTF8);
		}
	}
	m_Menu.SetTapeClip (Text);
}

void CKernel::TapeForgetClip (void)
{
	m_nTapeClipCount = 0;
	delete [] m_pTapeClipData;
	m_pTapeClipData = 0;
	m_nTapeClipSize = 0;
	TapeUpdateClip ();
}

// Copiar o Mover: se marca y se guarda una copia de los bloques (para
// pegarlos también en otra cinta)
void CKernel::TapeMark (boolean bMove)
{
	TapeForgetClip ();
	unsigned nCount = TapeEditCount ();
	m_pTapeClipData = m_Tape.ExtractBlocks (m_nTapeEditBlock, nCount, &m_nTapeClipSize);
	if (m_pTapeClipData == 0)
	{
		TapeBack (m_nTapeEditBlock, T ("No hay memoria para marcarlo"));
		return;
	}
	m_nTapeClipFirst = m_nTapeEditBlock;
	m_nTapeClipCount = nCount;
	m_bTapeClipMove = bMove;
	m_bTapeClipTZX = m_Tape.IsTZX ();
	m_nTapeClipSerial = m_Tape.GetSerial ();
	strncpy (m_TapeClipName, m_Tape.GetName (), sizeof m_TapeClipName - 1);
	m_TapeClipName[sizeof m_TapeClipName - 1] = '\0';
	TapeUpdateClip ();
	TapeBack (m_nTapeEditBlock, T ("Elige dónde ponerlo y pulsa F2"));
}

// Cuántos bloques van con este: la cabecera y sus datos, o solo él
unsigned CKernel::TapeEditCount (void)
{
	return m_bTapeSingle ? 1 : m_Tape.GetGroup (m_nTapeEditBlock);
}

void CKernel::TapeShowOptions (boolean bSingle)
{
	m_bTapeSingle = bSingle;
	unsigned nCount = TapeEditCount ();
	boolean bGroup = nCount > 1;

	CString Title;
	Title.Format (T ("Bloque %u de %u"), m_nTapeEditBlock + 1, m_Tape.GetBlockCount ());
	m_nTapeStage = TapeStageOptions;
	m_Menu.OpenChoice (Title);
	m_nTapeOptions = 0;

	if (m_nTapeClipCount > 0)		// lo marcado: pegarlo u olvidarlo
	{
		boolean bMove = m_bTapeClipMove && TapeClipSame ();	// en otra cinta, una copia
		m_TapeOption[m_nTapeOptions] = TapeOptionPasteBefore;
		m_Menu.SetChoiceItem (m_nTapeOptions++, bMove ? T ("Mover delante de este")
							      : T ("Pegar delante de este"));
		m_TapeOption[m_nTapeOptions] = TapeOptionPasteAfter;
		m_Menu.SetChoiceItem (m_nTapeOptions++, bMove ? T ("Mover detrás de este")
							      : T ("Pegar detrás de este"));
		m_TapeOption[m_nTapeOptions] = TapeOptionForget;
		m_Menu.SetChoiceItem (m_nTapeOptions++, T ("Olvidar lo marcado"));
	}

	m_TapeOption[m_nTapeOptions] = TapeOptionCopy;
	m_Menu.SetChoiceItem (m_nTapeOptions++, bGroup ? T ("Copiar cabecera y datos") : T ("Copiar este bloque"));
	m_TapeOption[m_nTapeOptions] = TapeOptionMove;
	m_Menu.SetChoiceItem (m_nTapeOptions++, bGroup ? T ("Mover cabecera y datos") : T ("Mover este bloque"));
	if (m_Tape.IsHeader (m_nTapeEditBlock))
	{
		m_TapeOption[m_nTapeOptions] = TapeOptionRename;
		m_Menu.SetChoiceItem (m_nTapeOptions++, T ("Renombrar la cabecera..."));
	}
	m_TapeOption[m_nTapeOptions] = TapeOptionDelete;
	m_Menu.SetChoiceItem (m_nTapeOptions++, bGroup ? T ("Borrar cabecera y datos...") : T ("Borrar este bloque..."));
	if (bGroup)
	{
		m_TapeOption[m_nTapeOptions] = TapeOptionSingle;
		m_Menu.SetChoiceItem (m_nTapeOptions++, T ("Solo la cabecera, sin sus datos..."));
	}
}

void CKernel::TapeAskDelete (void)
{
	CString Range, Title;
	BlockRange (m_nTapeEditBlock, TapeEditCount (), &Range);
	Title.Format (T ("¿Borrar %s?"), (const char *) Range);
	m_nTapeStage = TapeStageDelete;
	m_Menu.OpenChoice (Title);
	m_Menu.SetChoiceItem (0, T ("No, dejarlo"));
	m_Menu.SetChoiceItem (1, T ("Sí, borrar (en el fichero, al guardar)"));
	m_Menu.SetChoiceSelected (0);
}

void CKernel::TapeAskRename (void)
{
	char Name[12];
	if (!m_Tape.GetHeaderName (m_nTapeEditBlock, Name))
	{
		Name[0] = '\0';
	}
	m_nTapeStage = TapeStageRename;
	m_Menu.OpenText (T ("Nombre en la cinta"), T ("Hasta 10 letras (lo que sale al cargar)"),
			 T ("Mayúsculas: CAPS SHIFT + letra"), T ("ENTER cambiar   ESC volver"),
			 Name, strlen (Name), 10);
}

// Pegar lo marcado delante o detrás del bloque elegido (detrás de su grupo:
// nunca entre una cabecera y sus datos)
void CKernel::TapePaste (boolean bAfter)
{
	unsigned nBefore = m_nTapeEditBlock;
	if (bAfter)
	{
		nBefore += m_Tape.GetGroup (m_nTapeEditBlock);
	}
	else if (nBefore > 0 && m_Tape.GetGroup (nBefore - 1) == 2)
	{
		nBefore--;		// los datos de una cabecera: delante de ella
	}

	unsigned nFirst = m_nTapeClipFirst, nCount = m_nTapeClipCount;
	boolean bMove = m_bTapeClipMove && TapeClipSame ();
	if (bMove && nBefore > nFirst && nBefore < nFirst + nCount)
	{
		TapeBack (m_nTapeEditBlock, T ("Dentro de lo marcado: elige otro bloque"));
		return;
	}

	boolean bOK;
	if (bMove)
	{
		bOK = m_Tape.MoveBlocks (nFirst, nCount, nBefore);
	}
	else
	{
		// la copia guardada, en el formato de esta cinta (de TZX a TAP
		// solo los bloques estándar: si no, nada y se avisa)
		int nResult = m_Tape.InsertBlocks (m_pTapeClipData, m_nTapeClipSize, m_bTapeClipTZX, nBefore);
		if (nResult < 0)
		{
			TapeBack (m_nTapeEditBlock, T ("No caben en un TAP: turbo, tonos, textos"));
			return;			// lo marcado sigue, para pegarlo en un TZX
		}
		bOK = nResult > 0;
	}
	TapeForgetClip ();		// los números de los bloques ya no son los mismos
	if (!bOK)
	{
		TapeBack (m_nTapeEditBlock, bMove ? T ("No se pudo mover") : T ("No se pudo copiar (¿cinta llena?)"));
		return;
	}

	unsigned nNew = bMove && nBefore > nFirst ? nBefore - nCount : nBefore;
	CString Range, Message;
	BlockRange (nNew, nCount, &Range);
	Message.Format (bMove ? T ("Movido: %s") : T ("Copiado: %s"), (const char *) Range);
	TapeBack (nNew, Message);
}

void CKernel::TapeAskLeave (void)
{
	m_nTapeStage = TapeStageLeave;
	m_Menu.OpenChoice (T ("La cinta tiene cambios sin guardar"));
	m_nTapeOptions = 0;
	m_TapeOption[m_nTapeOptions] = TapeLeaveContinue;
	m_Menu.SetChoiceItem (m_nTapeOptions++, T ("Seguir editando"));
	if (TapeCanSaveInPlace ())
	{
		char UTF8[MENU_NAME_LEN * 2];
		CP850ToUTF8 (FileBaseName (m_TapePath), UTF8, sizeof UTF8);
		CString Item;
		Item.Format (T ("Guardar en %s"), UTF8);
		m_TapeOption[m_nTapeOptions] = TapeLeaveSave;
		m_Menu.SetChoiceItem (m_nTapeOptions++, Item);
	}
	m_TapeOption[m_nTapeOptions] = TapeLeaveSaveAs;
	m_Menu.SetChoiceItem (m_nTapeOptions++, T ("Guardar como..."));
	m_TapeOption[m_nTapeOptions] = TapeLeaveDiscard;
	m_Menu.SetChoiceItem (m_nTapeOptions++, T ("Descartar los cambios"));
	m_Menu.SetChoiceSelected (0);
}

// La carpeta donde guardar como: la de la cinta si está en la SD (no dentro
// de un .zip), si no, saves
void CKernel::TapeSaveDir (char *pDir, unsigned nSize)
{
	if (strncasecmp (m_TapePath, "SD:", 3) == 0 && HasTapeExtension (m_TapePath, m_Tape.IsTZX ()))
	{
		strncpy (pDir, m_TapePath, nSize - 1);
		pDir[nSize - 1] = '\0';
		char *pName = (char *) FileBaseName (pDir);
		*pName = '\0';
	}
	else
	{
		f_mkdir ("SD:/saves");
		strncpy (pDir, "SD:/saves/", nSize - 1);
		pDir[nSize - 1] = '\0';
	}
}

void CKernel::TapeAskSaveAs (const char *pHelp, const char *pText)
{
	char Initial[MENU_NAME_LEN];
	if (pText != 0)
	{
		strncpy (Initial, pText, sizeof Initial - 1);
		Initial[sizeof Initial - 1] = '\0';
	}
	else
	{
		// "juego (2).tap": el primer nombre libre en la carpeta
		char Dir[MENU_PATH_LEN], Name[MENU_NAME_LEN];
		TapeSaveDir (Dir, sizeof Dir);
		if (HasTapeExtension (m_TapePath, m_Tape.IsTZX ()))
		{
			strncpy (Name, FileBaseName (m_TapePath), sizeof Name - 1);
			Name[sizeof Name - 1] = '\0';
		}
		else
		{
			strcpy (Name, m_Tape.IsTZX () ? "cinta.tzx" : "cinta.tap");
		}
		if (!CFileCopy::FreeName (Dir, Name, Initial, sizeof Initial))
		{
			strcpy (Initial, Name);
		}
	}

	// el cursor delante de la extensión
	unsigned nLen = strlen (Initial);
	unsigned nCursor = nLen;
	for (unsigned i = nLen; i-- > 1; )
	{
		if (Initial[i] == '.')
		{
			nCursor = i;
			break;
		}
	}
	m_nTapeStage = TapeStageSaveAs;
	m_Menu.OpenText (T ("Guardar la cinta como"), pHelp != 0 ? pHelp : T ("Mayúsculas: CAPS SHIFT + letra"),
			 T ("Símbolos ( ) . - _: SYMBOL SHIFT + tecla"), T ("ENTER guardar   ESC volver"),
			 Initial, nCursor);
}

// Escribe la cinta en pPath: primero con un nombre temporal y, si va bien,
// se le pone el suyo. bBackup: el fichero que había se queda como .bak (si no
// hay ya uno: ese es el original)
boolean CKernel::TapeWrite (const char *pPath, boolean bBackup)
{
	char Temp[MENU_PATH_LEN];
	strncpy (Temp, pPath, sizeof Temp - 1);
	Temp[sizeof Temp - 1] = '\0';
	char *pName = (char *) FileBaseName (Temp);
	if ((unsigned) (pName - Temp) + sizeof TAPE_TEMP_NAME > sizeof Temp)
	{
		return FALSE;
	}
	strcpy (pName, TAPE_TEMP_NAME);

	unsigned nSize;
	const u8 *pData = m_Tape.GetData (&nSize);
	if (!WriteFile (Temp, pData, nSize, FALSE))
	{
		f_unlink (Temp);		// tarjeta llena o protegida: el original sigue
		return FALSE;
	}

	FILINFO Info;
	if (f_stat (pPath, &Info) == FR_OK)
	{
		CString Backup;
		Backup.Format ("%s.bak", pPath);
		if (bBackup && f_stat (Backup, &Info) != FR_OK)
		{
			if (f_rename (pPath, Backup) != FR_OK)
			{
				f_unlink (Temp);
				return FALSE;
			}
		}
		else if (f_unlink (pPath) != FR_OK)
		{
			f_unlink (Temp);
			return FALSE;
		}
	}
	if (f_rename (Temp, pPath) != FR_OK)
	{
		return FALSE;			// queda el temporal: mejor que nada
	}
	m_Tape.KeepEdits ();
	return TRUE;
}

// Guardado o descartado: fuera del navegador (con ENTER y guardado, la cinta
// al bloque elegido; descartado, los números ya no son esos)
void CKernel::TapeLeave (const char *pMessage, boolean bSeek)
{
	m_nTapeStage = TapeStageNone;
	if (!bSeek)
	{
		m_nTapeClipSerial = 0;		// descartado: lo marcado ya solo se puede copiar
		TapeUpdateClip ();
	}
	m_Menu.Close ();
	if (bSeek && m_Menu.IsTapeLeaveSeek ())
	{
		unsigned nBlock = m_Menu.GetSelectedTapeBlock ();
		if (nBlock < m_Tape.GetBlockCount ())
		{
			m_Tape.SeekBlock (nBlock);
		}
	}
	m_pSpectrum->TapeInserted ();
	ShowMessage (pMessage);
}

void CKernel::TapeSaveAs (const char *pName)
{
	if (pName[0] == '\0')
	{
		TapeAskLeave ();
		return;
	}
	if (!WebValidName (pName))
	{
		TapeAskSaveAs (T ("No vale: sin punto ni espacio al final"), pName);
		return;
	}

	// con la extensión de su formato
	char Name[MENU_NAME_LEN];
	strncpy (Name, pName, sizeof Name - 1);
	Name[sizeof Name - 1] = '\0';
	if (!HasTapeExtension (Name, m_Tape.IsTZX ()))
	{
		if (strlen (Name) + 5 > sizeof Name)
		{
			TapeAskSaveAs (T ("Demasiado largo"), pName);
			return;
		}
		strcat (Name, m_Tape.IsTZX () ? ".tzx" : ".tap");
	}

	char Dir[MENU_PATH_LEN];
	TapeSaveDir (Dir, sizeof Dir);
	CString Path;
	Path.Format ("%s%s", Dir, Name);
	if (FileProtected (Path))
	{
		TapeAskSaveAs (T ("Protegido: elige otro nombre"), Name);
		return;
	}
	FILINFO Info;
	if (f_stat (Path, &Info) == FR_OK)
	{
		TapeAskSaveAs (T ("Ya hay uno con ese nombre"), Name);
		return;
	}
	if (!TapeWrite (Path, FALSE))
	{
		TapeBack (m_Menu.GetSelectedTapeBlock (), T ("No se pudo guardar (¿tarjeta llena?)"));
		return;
	}

	// esta es ahora la cinta puesta
	strncpy (m_TapePath, Path, sizeof m_TapePath - 1);
	m_TapePath[sizeof m_TapePath - 1] = '\0';
	m_Tape.SetName (Name);
	m_bSaveTape = FALSE;

	char UTF8[MENU_NAME_LEN * 2];
	CP850ToUTF8 (Name, UTF8, sizeof UTF8);
	CString Message;
	Message.Format (T ("Cinta guardada: %s"), UTF8);
	TapeLeave (Message, TRUE);
}

boolean CKernel::TapeMenuResult (TMenuResult Result)
{
	if (Result == MenuResultTapeOptions)
	{
		m_nTapeEditBlock = m_Menu.GetSelectedTapeBlock ();
		switch (m_Tape.CanEdit ())
		{
		case CTape::EditJumps:
			TapeBack (m_nTapeEditBlock, T ("Tiene saltos o bucles: no se edita"));
			break;
		case CTape::EditNotBlocks:
			TapeBack (m_nTapeEditBlock, T ("Esta cinta no tiene bloques que editar"));
			break;
		default:
			TapeShowOptions (FALSE);
			break;
		}
		return TRUE;
	}
	if (Result == MenuResultTapeLeave)
	{
		TapeAskLeave ();
		return TRUE;
	}

	switch (m_nTapeStage)
	{
	case TapeStageOptions:
		if (Result == MenuResultChoice && m_Menu.GetChoice () < m_nTapeOptions)
		{
			switch (m_TapeOption[m_Menu.GetChoice ()])
			{
			case TapeOptionCopy:
			case TapeOptionMove:
				TapeMark (m_TapeOption[m_Menu.GetChoice ()] == TapeOptionMove);
				break;

			case TapeOptionRename:
				TapeAskRename ();
				break;

			case TapeOptionDelete:
				TapeAskDelete ();
				break;

			case TapeOptionSingle:
				TapeShowOptions (TRUE);
				break;

			case TapeOptionPasteBefore:
				TapePaste (FALSE);
				break;

			case TapeOptionPasteAfter:
				TapePaste (TRUE);
				break;

			default:
				TapeForgetClip ();
				TapeBack (m_nTapeEditBlock, 0);
				break;
			}
			return TRUE;
		}
		if (Result == MenuResultChoiceCancel)
		{
			TapeBack (m_nTapeEditBlock, 0);		// ESC: a la cinta
			return TRUE;
		}
		return FALSE;

	case TapeStageDelete:
		if (Result == MenuResultChoice && m_Menu.GetChoice () == 1)
		{
			unsigned nCount = TapeEditCount ();
			if (!m_Tape.DeleteBlocks (m_nTapeEditBlock, nCount))
			{
				TapeBack (m_nTapeEditBlock, T ("No se puede dejar la cinta vacía"));
				return TRUE;
			}
			TapeForgetClip ();
			CString Range, Message;
			BlockRange (m_nTapeEditBlock, nCount, &Range);
			Message.Format (T ("Borrado: %s"), (const char *) Range);
			unsigned nBlocks = m_Tape.GetBlockCount ();
			TapeBack (m_nTapeEditBlock < nBlocks ? m_nTapeEditBlock : nBlocks - 1, Message);
			return TRUE;
		}
		if (Result == MenuResultChoice || Result == MenuResultChoiceCancel)
		{
			TapeBack (m_nTapeEditBlock, 0);
			return TRUE;
		}
		return FALSE;

	case TapeStageRename:
		if (Result == MenuResultInput)
		{
			char Old[12];
			m_Tape.GetHeaderName (m_nTapeEditBlock, Old);
			if (strcmp (Old, m_Menu.GetInput ()) == 0)
			{
				TapeBack (m_nTapeEditBlock, 0);		// sin cambios
			}
			else if (m_Tape.RenameHeader (m_nTapeEditBlock, m_Menu.GetInput ()))
			{
				TapeForgetClip ();
				TapeBack (m_nTapeEditBlock, T ("Cabecera renombrada"));
			}
			else
			{
				TapeBack (m_nTapeEditBlock, T ("No se pudo renombrar"));
			}
			return TRUE;
		}
		if (Result == MenuResultInputCancel)
		{
			TapeBack (m_nTapeEditBlock, 0);
			return TRUE;
		}
		return FALSE;

	case TapeStageLeave:
		if (Result == MenuResultChoice && m_Menu.GetChoice () < m_nTapeOptions)
		{
			switch (m_TapeOption[m_Menu.GetChoice ()])
			{
			case TapeLeaveSave:
				if (!TapeWrite (m_TapePath, TRUE))
				{
					TapeBack (m_Menu.GetSelectedTapeBlock (), T ("No se pudo guardar (¿tarjeta llena?)"));
				}
				else
				{
					char UTF8[MENU_NAME_LEN * 2];
					CP850ToUTF8 (FileBaseName (m_TapePath), UTF8, sizeof UTF8);
					CString Message;
					Message.Format (T ("Guardada: %s (el original, en .bak)"), UTF8);
					TapeLeave (Message, TRUE);
				}
				break;

			case TapeLeaveSaveAs:
				TapeAskSaveAs (0, 0);
				break;

			case TapeLeaveDiscard:
				m_Tape.DiscardEdits ();
				TapeLeave (T ("Cambios descartados"), FALSE);
				break;

			default:
				TapeBack (m_Menu.GetSelectedTapeBlock (), 0);
				break;
			}
			return TRUE;
		}
		if (Result == MenuResultChoiceCancel)
		{
			TapeBack (m_Menu.GetSelectedTapeBlock (), 0);	// ESC: seguir editando
			return TRUE;
		}
		return FALSE;

	case TapeStageSaveAs:
		if (Result == MenuResultInput)
		{
			TapeSaveAs (m_Menu.GetInput ());
			return TRUE;
		}
		if (Result == MenuResultInputCancel)
		{
			TapeAskLeave ();
			return TRUE;
		}
		return FALSE;

	default:
		return FALSE;
	}
}
