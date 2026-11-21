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
// - Las dos últimas líneas: "POKE a mano..." y "Abrir un .pok..." (el
//   navegador, en SD:/pokes; un .pok elegido en el navegador también abre
//   esta pantalla).
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

#define POKES_DIR		"SD:/pokes"
#define POK_FILE_MAX		(64 * 1024)	// el mayor de ZXDB tiene 11 KB
#define POKE_MANUAL_PAIRS	4

enum
{
	PokeStageNone,
	PokeStageList,		// la lista de trucos
	PokeStageValue,		// un truco con valor 256: el valor
	PokeStageManual		// el POKE a mano
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
	m_nPokeResets = m_pSpectrum->GetResetCount ();
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
	m_Menu.SetChoiceSelected (nSelected <= nCount + 1 ? nSelected : 0);
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

	PokeShowList (0);
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
			else
			{
				m_nPokeStage = PokeStageNone;
				m_Menu.OpenBrowserIn (POKES_DIR);
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
	}

	return FALSE;
}
