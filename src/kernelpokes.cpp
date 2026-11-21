//
// kernelpokes.cpp
//
// Pokes y trucos:
//
// - Alt+C (o P en la ayuda; en el teclado GPIO, modo extendido + SYMBOL SHIFT
//   + C) abre la lista de trucos del juego cargado. Su .POK se busca solo en
//   SD:/pokes, por el nombre del juego (pokes/<juego>.pok o
//   pokes/<inicial>/<juego>.pok, como en la colección de ZXDB). Al elegir un
//   truco se aplica al momento y se marca [x]; otra vez, se quita y la
//   memoria vuelve a estar como antes. Los de valor 256 preguntan el valor.
// - Si no está en la SD y hay red, se busca en ZXDB: por la huella del
//   fichero cargado (SHA-512, /v5/filecheck de ZXInfo) o, si se descargó con
//   el buscador de ZXDB, por su ficha. Su .pok se descarga de Spectrum
//   Computing a pokes/<juego>.pok (la próxima vez ya está). Una vez por juego.
// - Las tres últimas líneas: "POKE a mano...", "Abrir un .pok..." (el
//   navegador, en SD:/pokes; un .pok elegido en el navegador también abre
//   esta pantalla) y "Buscar pokes en ZXDB..." (por el título, para los
//   ficheros que ZXDB no reconoce).
// - Alt+O (modo extendido + SYMBOL SHIFT + O): el POKE a mano directamente:
//   "35136 0" (POKE 35136,0), en hexadecimal con H al final ("8940H C9H"),
//   varios pares seguidos. Se quedan en la lista, para poder quitarlos.
//
// Al cargar otro juego se olvidan los trucos; con un reset, se quedan pero
// sin aplicar (la memoria se ha borrado).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include "i18n.h"
#include <circle/string.h>
#include <circle/util.h>
#include <fatfs/ff.h>
#include <string.h>
#include "webserver.h"		// ZXDBFetchPok, ZXDBSearch
#include "network.h"
#include "mbedtls/sha512.h"

#define POKES_DIR		"SD:/pokes"
#define POK_FILE_MAX		(64 * 1024)	// el mayor de ZXDB tiene 11 KB
#define POKE_MANUAL_PAIRS	4

enum
{
	PokeStageNone,
	PokeStageList,		// la lista de trucos
	PokeStageValue,		// un truco con valor 256: el valor
	PokeStageManual,	// el POKE a mano
	PokeStageSearch,	// buscar en ZXDB: el título
	PokeStageResults	// buscar en ZXDB: los resultados
};

// La memoria del Spectrum para CPokFile: las páginas del 128K, o la memoria
// tal como está paginada (en los 48K, siempre así)
class CPokSpectrumMemory : public CPokMemory
{
public:
	CPokSpectrumMemory (CSpectrum *pSpectrum) : m_pSpectrum (pSpectrum) {}

	u8 Read (unsigned nBank, u16 nAddress) override
	{
		if (nBank == POK_BANK_CURRENT || !HasPages ())
		{
			return m_pSpectrum->ReadMemory (nAddress);
		}
		return m_pSpectrum->ReadRAM (nBank, nAddress & 0x3FFF);
	}

	void Write (unsigned nBank, u16 nAddress, u8 nValue) override
	{
		if (nBank == POK_BANK_CURRENT || !HasPages ())
		{
			if (nAddress >= 0x4000)		// a la ROM, nada
			{
				m_pSpectrum->WriteMemory (nAddress, nValue);
			}
			return;
		}
		m_pSpectrum->WriteRAM (nBank, nAddress & 0x3FFF, nValue);
	}

private:
	boolean HasPages (void) const
	{
		return m_pSpectrum->GetTiming ()->CodeModel != CodeModel48K;
	}

	CSpectrum *m_pSpectrum;
};

// El nombre del fichero sin carpeta ni extensión
static void BaseName (const char *pPath, char *pBase, unsigned nSize)
{
	const char *pName = pPath;
	for (const char *p = pPath; *p != '\0'; p++)
	{
		if (*p == '/' || *p == ':' || *p == '|')
		{
			pName = p + 1;
		}
	}
	strncpy (pBase, pName, nSize - 1);
	pBase[nSize - 1] = '\0';
	char *pDot = 0;				// el último punto (Circle no tiene strrchr)
	for (char *p = pBase; *p != '\0'; p++)
	{
		if (*p == '.')
		{
			pDot = p;
		}
	}
	if (pDot != 0 && pDot != pBase)
	{
		*pDot = '\0';
	}
}

void CKernel::PokeSetGame (const char *pPath)
{
	m_Pokes.Clear ();
	m_PokeTitle[0] = '\0';
	m_bPokeTried = FALSE;
	BaseName (pPath != 0 ? pPath : "", m_PokeGame, sizeof m_PokeGame);
	strncpy (m_PokeGamePath, pPath != 0 ? pPath : "", sizeof m_PokeGamePath - 1);
	m_PokeGamePath[sizeof m_PokeGamePath - 1] = '\0';
	m_PokeZXDBID[0] = '\0';
	m_bPokeOnline = FALSE;
	m_nPokeResets = m_pSpectrum->GetResetCount ();
}

void CKernel::PokeSetZXDBID (const char *pID)
{
	strncpy (m_PokeZXDBID, pID, sizeof m_PokeZXDBID - 1);
	m_PokeZXDBID[sizeof m_PokeZXDBID - 1] = '\0';
}

// Busca en ZXDB el .pok del juego cargado (por su ficha o por la huella del
// fichero) y lo carga; TRUE si lo ha encontrado
boolean CKernel::PokeFetchOnline (void)
{
	m_bPokeOnline = TRUE;			// una vez por juego

	char Hash[129] = "";
	if (m_PokeZXDBID[0] == '\0')
	{
		// SHA-512 del fichero, tal cual está en la SD (o en el ZIP)
		int nSize = SourceSize (m_PokeGamePath);
		if (nSize <= 0 || (unsigned) nSize > WORK_BUFFER_SIZE
		    || !ReadSource (m_PokeGamePath, m_pWork, (unsigned) nSize))
		{
			return FALSE;
		}
		u8 Digest[64];
		if (mbedtls_sha512 (m_pWork, (size_t) nSize, Digest, 0) != 0)
		{
			return FALSE;
		}
		static const char Hex[] = "0123456789abcdef";
		for (unsigned i = 0; i < 64; i++)
		{
			Hash[2 * i] = Hex[Digest[i] >> 4];
			Hash[2 * i + 1] = Hex[Digest[i] & 15];
		}
		Hash[128] = '\0';
	}

	CString SDPath, Message;
	int nResult = ZXDBFetchPok (Hash, m_PokeZXDBID, m_PokeGame, &SDPath, &Message);
	ShowMessage (Message);
	return nResult > 0 && PokeLoadFile (SDPath, FALSE);
}

boolean CKernel::PokeAvailable (void)
{
	if (IsZX81Active ())
	{
		ShowMessage (T ("Los pokes no están disponibles con el ZX81"));
		return FALSE;
	}

	// un reset desde que se aplicaron: la memoria se ha borrado
	if (m_nPokeResets != m_pSpectrum->GetResetCount ())
	{
		m_Pokes.Forget ();
		m_nPokeResets = m_pSpectrum->GetResetCount ();
	}
	return TRUE;
}

boolean CKernel::PokeLoadFile (const char *pPath, boolean bQuiet)
{
	static char Buffer[POK_FILE_MAX];
	unsigned nSize = ReadWholeFile (pPath, (u8 *) Buffer, sizeof Buffer);
	if (nSize == 0 || !m_Pokes.Parse (Buffer, nSize))
	{
		if (!bQuiet)
		{
			ShowMessage (T ("No es un fichero .POK válido"));
		}
		return FALSE;
	}
	BaseName (pPath, m_PokeTitle, sizeof m_PokeTitle);
	m_bPokeTried = TRUE;		// no se vuelve a buscar el del juego
	return TRUE;
}

// Abre el menú, si no lo estaba (como F1)
void CKernel::PokeEnterMenu (void)
{
	if (!m_Menu.IsOpen ())
	{
		memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
		UpdateStatusLines ();
		OpenMenu ();
		m_Keyboard.SetCaptured (TRUE);
	}
}

void CKernel::PokeShowList (unsigned nSelected)
{
	PokeEnterMenu ();

	CString Title;
	if (m_PokeTitle[0] != '\0')
	{
		Title.Format ("%s: %s", T ("Pokes"), m_PokeTitle);
	}
	else if (m_PokeGame[0] != '\0')
	{
		Title.Format (T ("Pokes: no hay .pok de %s"), m_PokeGame);
	}
	else
	{
		Title = T ("Pokes");
	}

	m_nPokeStage = PokeStageList;
	m_Menu.OpenChoice (Title);
	unsigned nCount = m_Pokes.GetCount ();
	for (unsigned i = 0; i < nCount; i++)
	{
		CString Item;
		Item.Format ("[%c] %s", m_Pokes.IsActive (i) ? 'x' : ' ', m_Pokes.GetName (i));
		m_Menu.SetChoiceItem (i, Item);
	}
	m_Menu.SetChoiceItem (nCount, T ("POKE a mano..."));
	m_Menu.SetChoiceItem (nCount + 1, T ("Abrir un .pok..."));
	m_Menu.SetChoiceItem (nCount + 2, T ("Buscar pokes en ZXDB..."));
	m_Menu.SetChoiceSelected (nSelected <= nCount + 2 ? nSelected : 0);
}

void CKernel::PokeOpen (void)
{
	if (!PokeAvailable ())
	{
		return;
	}

	// la primera vez, el .POK del juego: pokes/<juego>.pok o
	// pokes/<inicial>/<juego>.pok (las carpetas por letra de la colección)
	if (!m_bPokeTried && m_PokeGame[0] != '\0')
	{
		m_bPokeTried = TRUE;
		CString Path;
		Path.Format ("%s/%s.pok", POKES_DIR, m_PokeGame);
		if (!PokeLoadFile (Path, TRUE))
		{
			char cFirst = m_PokeGame[0];
			if (cFirst >= 'A' && cFirst <= 'Z')
			{
				cFirst += 'a' - 'A';
			}
			if (cFirst >= 'a' && cFirst <= 'z')
			{
				Path.Format ("%s/%c/%s.pok", POKES_DIR, cFirst, m_PokeGame);
			}
			else
			{
				Path.Format ("%s/123/%s.pok", POKES_DIR, m_PokeGame);
			}
			PokeLoadFile (Path, TRUE);
		}
	}

	// si no está en la SD, en ZXDB (una vez por juego, y con red)
	if (   m_Pokes.GetFileCount () == 0 && !m_bPokeOnline && m_PokeGamePath[0] != '\0'
	    && CNetworkManager::IsNetRunning ())
	{
		PokeEnterMenu ();		// el aviso de la red sale ya con el menú
		PokeFetchOnline ();
	}

	PokeShowList (0);
}

void CKernel::PokeAskSearch (void)
{
	m_nPokeStage = PokeStageSearch;
	m_Menu.OpenInput (T ("Buscar pokes en ZXDB"), T ("Escribe el título del juego"),
			  T ("(letras, números y espacios)"), T ("ENTER buscar   ESC volver"));
}

void CKernel::PokeShowResults (unsigned nSelected)
{
	m_nPokeStage = PokeStageResults;
	CString Title;
	Title.Format (T ("Pokes en ZXDB: %s"), m_ZXDBQuery);
	m_Menu.OpenChoice (Title);
	for (unsigned i = 0; i < m_nZXDBResults; i++)
	{
		const TZXInfoResult &R = m_ZXDBResults[i];
		CString Item;
		if (R.nYear != 0)
		{
			Item.Format ("%s (%u, %s)", R.Title, R.nYear, R.Publisher);
		}
		else
		{
			Item.Format ("%s (%s)", R.Title, R.Publisher);
		}
		m_Menu.SetChoiceItem (i, Item);
	}
	m_Menu.SetChoiceSelected (nSelected);
}

void CKernel::PokeToggle (unsigned nTrainer)
{
	CPokSpectrumMemory Memory (m_pSpectrum);

	if (m_Pokes.IsActive (nTrainer))
	{
		m_Pokes.Deactivate (nTrainer, &Memory);
		PokeShowList (nTrainer);
		return;
	}

	if (m_Pokes.NeedsInput (nTrainer))
	{
		m_nPokePending = nTrainer;
		m_nPokeStage = PokeStageValue;
		m_Menu.OpenInput (m_Pokes.GetName (nTrainer), T ("Escribe el valor (de 0 a 255)"),
				  T ("En hexadecimal, con H al final: C9H"), T ("ENTER aplicar   ESC volver"));
		return;
	}

	m_Pokes.Activate (nTrainer, &Memory);
	m_nPokeResets = m_pSpectrum->GetResetCount ();
	PokeShowList (nTrainer);
}

void CKernel::PokeOpenManual (boolean bFromList)
{
	if (!bFromList && !PokeAvailable ())
	{
		return;
	}
	PokeEnterMenu ();
	m_bPokeManualFromList = bFromList;
	m_nPokeStage = PokeStageManual;
	m_Menu.OpenInput (T ("POKE a mano"), T ("Dirección y valor: 35136 0"),
			  T ("Hexadecimal con H al final: 8940H C9H"), T ("ENTER aplicar   ESC volver"));
}

// Las respuestas del menú mientras se ven los pokes; TRUE si eran suyas
boolean CKernel::PokeMenuResult (TMenuResult Result)
{
	if (Result == MenuResultOpenPokes)
	{
		PokeOpen ();
		return TRUE;
	}

	switch (m_nPokeStage)
	{
	case PokeStageList:
		if (Result == MenuResultChoice)
		{
			unsigned nChoice = m_Menu.GetChoice ();
			unsigned nCount = m_Pokes.GetCount ();
			if (nChoice < nCount)
			{
				PokeToggle (nChoice);
			}
			else if (nChoice == nCount)
			{
				PokeOpenManual (TRUE);
			}
			else if (nChoice == nCount + 1)
			{
				m_nPokeStage = PokeStageNone;
				m_Menu.OpenBrowserIn (POKES_DIR);
			}
			else if (!CNetworkManager::IsNetRunning ())
			{
				ShowMessage (T ("Sin red"));
				PokeShowList (nChoice);
			}
			else
			{
				PokeAskSearch ();
			}
			return TRUE;
		}
		if (Result == MenuResultChoiceCancel)
		{
			m_nPokeStage = PokeStageNone;		// ESC: al Spectrum
			return TRUE;
		}
		return FALSE;

	case PokeStageValue:
		if (Result == MenuResultInput)
		{
			u8 nValue;
			if (!CPokFile::ParseValue (m_Menu.GetInput (), &nValue))
			{
				m_Menu.OpenInput (m_Pokes.GetName (m_nPokePending), T ("No vale: escribe de 0 a 255"),
						  T ("En hexadecimal, con H al final: C9H"), T ("ENTER aplicar   ESC volver"));
				return TRUE;
			}
			CPokSpectrumMemory Memory (m_pSpectrum);
			m_Pokes.Activate (m_nPokePending, &Memory, nValue);
			m_nPokeResets = m_pSpectrum->GetResetCount ();
			PokeShowList (m_nPokePending);
			return TRUE;
		}
		if (Result == MenuResultInputCancel)
		{
			PokeShowList (m_nPokePending);
			return TRUE;
		}
		return FALSE;

	case PokeStageManual:
		if (Result == MenuResultInput)
		{
			u16 Address[POKE_MANUAL_PAIRS];
			u8 Value[POKE_MANUAL_PAIRS];
			const char *pInput = m_Menu.GetInput ();
			unsigned nPairs = CPokFile::ParseManual (pInput, Address, Value, POKE_MANUAL_PAIRS);
			if (nPairs == 0)
			{
				// vacío: se vuelve sin hacer nada; mal escrito: se dice
				if (pInput[0] == '\0')
				{
					Result = MenuResultInputCancel;
				}
				else
				{
					m_Menu.OpenInput (T ("POKE a mano"), T ("No vale: dirección de 16384 a 65535"),
							  T ("y valor de 0 a 255 (35136 0, 8940H C9H)"),
							  T ("ENTER aplicar   ESC volver"));
					return TRUE;
				}
			}
			else
			{
				CPokSpectrumMemory Memory (m_pSpectrum);
				int nLast = -1;
				for (unsigned i = 0; i < nPairs; i++)
				{
					int nTrainer = m_Pokes.AddManual (Address[i], Value[i]);
					if (nTrainer >= 0)
					{
						m_Pokes.Activate ((unsigned) nTrainer, &Memory);
						nLast = nTrainer;
					}
				}
				m_nPokeResets = m_pSpectrum->GetResetCount ();

				if (nLast < 0)
				{
					m_Menu.OpenInput (T ("POKE a mano"), T ("No caben más: quita alguno"),
							  T ("(o carga otra vez el juego)"), T ("ENTER aplicar   ESC volver"));
					return TRUE;
				}
				if (m_bPokeManualFromList)
				{
					PokeShowList ((unsigned) nLast);
				}
				else
				{
					m_nPokeStage = PokeStageNone;	// el menú ya está cerrado
					CString Message;
					Message.Format ("%s", m_Pokes.GetName ((unsigned) nLast));
					ShowMessage (Message);
				}
				return TRUE;
			}
		}
		if (Result == MenuResultInputCancel)
		{
			if (m_bPokeManualFromList)
			{
				PokeShowList (m_Pokes.GetCount ());
			}
			else
			{
				m_nPokeStage = PokeStageNone;
			}
			return TRUE;
		}
		return FALSE;

	case PokeStageSearch:
		if (Result == MenuResultInput && m_Menu.GetInput ()[0] != '\0')
		{
			strncpy (m_ZXDBQuery, m_Menu.GetInput (), sizeof m_ZXDBQuery - 1);
			m_ZXDBQuery[sizeof m_ZXDBQuery - 1] = '\0';
			CString Error;
			int nCount = ZXDBSearch (m_ZXDBQuery, m_ZXDBResults, ZXINFO_MAX_RESULTS, &Error);
			if (nCount <= 0)
			{
				ShowMessage (nCount < 0 ? (const char *) Error : T ("ZXDB: no se ha encontrado nada"));
				PokeAskSearch ();
				return TRUE;
			}
			m_nZXDBResults = nCount;
			PokeShowResults (0);
			return TRUE;
		}
		if (Result == MenuResultInput || Result == MenuResultInputCancel)
		{
			PokeShowList (m_Pokes.GetCount () + 2);		// vacío o ESC: a la lista
			return TRUE;
		}
		return FALSE;

	case PokeStageResults:
		if (Result == MenuResultChoice && m_Menu.GetChoice () < m_nZXDBResults)
		{
			unsigned nChoice = m_Menu.GetChoice ();
			const TZXInfoResult &R = m_ZXDBResults[nChoice];
			CString SDPath, Message;
			int nResult = ZXDBFetchPok ("", R.ID, m_PokeGame[0] != '\0' ? m_PokeGame : R.Title,
						    &SDPath, &Message);
			ShowMessage (Message);
			if (nResult > 0 && PokeLoadFile (SDPath, FALSE))
			{
				PokeShowList (0);
			}
			else
			{
				PokeShowResults (nChoice);		// otro de la lista
			}
			return TRUE;
		}
		if (Result == MenuResultChoiceCancel)
		{
			PokeAskSearch ();				// ESC: otra búsqueda
			return TRUE;
		}
		return FALSE;
	}

	return FALSE;
}
