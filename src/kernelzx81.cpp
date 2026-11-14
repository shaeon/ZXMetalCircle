//
// kernelzx81.cpp
//
// El ZX81 con el emulador ZX81+ de Paul Farrow (ver zx81.h): la ROM se carga
// de roms/zx81.rom, se pone como cartucho del Interface 2 en el 128K y se
// arranca; al salir (F11, o elegir un modelo con F7) se quita el cartucho.
// La distribución del teclado se pone según la opción zx81kbd, sin tocar el
// fichero. Los .P se cargan con CZX81Loader.
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

boolean CKernel::IsZX81ROMPresent (void)
{
	if (m_pZX81ROM != 0)
	{
		return TRUE;
	}

	FILINFO Info;
	return    m_bSDMounted
	       && f_stat ("SD:/roms/" ZX81_ROM_NAME, &Info) == FR_OK
	       && Info.fsize == ZX81_ROM_SIZE;
}

boolean CKernel::IsZX81Active (void) const
{
	return m_bZX81 && m_pSpectrum->IsCartridgeInserted ();
}

boolean CKernel::StartZX81 (const char *pProgram)
{
	if (m_pZX81ROM == 0)
	{
		m_pZX81ROM = LoadROMFile (ZX81_ROM_NAME, ZX81_ROM_SIZE);
		if (m_pZX81ROM == 0)
		{
			ShowMessage (T ("Falta roms/zx81.rom (ver LEEME)"));
			return FALSE;
		}
	}

	// El programa se lee ya: la entrada de un ZIP solo vale ahora
	static u8 Program[ZX81_PROG_MAX];
	unsigned nProgSize = 0;
	if (pProgram != 0)
	{
		int nSize = SourceSize (pProgram);
		if (nSize < 0)
		{
			ShowMessage (T ("No se puede abrir el fichero"));
			return FALSE;
		}
		if (nSize == 0 || nSize > ZX81_PROG_MAX)
		{
			ShowMessage (T ("Programa del ZX81 no válido"));
			return FALSE;
		}
		if (!ReadSource (pProgram, Program, nSize))
		{
			ShowMessage (T ("No se puede abrir el fichero"));
			return FALSE;
		}
		nProgSize = nSize;
	}

	// Solo funciona en el 128K (el +2 es igual); los demás modelos, al 128K
	TModel Model = m_pSpectrum->GetModel ();
	if (Model != Model128K && Model != ModelPlus2)
	{
		SetModel (Model128K);		// (ya quita el ZX81 si estaba)
	}
	else
	{
		StopZX81 ();			// si ya estaba, se vuelve a arrancar
	}

	// La distribución del teclado, en una copia de la ROM
	static u8 ROM[ZX81_ROM_SIZE];
	memcpy (ROM, m_pZX81ROM, sizeof ROM);
	int nKbd = m_Settings.Find ("zx81kbd");
	CZX81Loader::SetKeyboardLayout (ROM, nKbd < 0 || strcmp (m_Settings.GetValue (nKbd), "128") != 0);

	if (!m_pSpectrum->InsertCartridge (ROM, sizeof ROM))		// con reset
	{
		ShowMessage (T ("Cartucho no válido (máximo 16 KB)"));
		return FALSE;
	}
	m_bZX81 = TRUE;
	strcpy (m_CartridgeName, "ZX81");
	m_nNextFrameTime = 0;

	if (pProgram != 0)
	{
		m_ZX81Loader.Start (Program, nProgSize);	// la carga, cuadro a cuadro

		const char *pName = pProgram;
		for (const char *p = pProgram; *p != '\0'; p++)
		{
			if (*p == '/' || *p == ':')
			{
				pName = p + 1;
			}
		}
		CString Message;
		Message.Format (T ("ZX81: %s"), pName);
		ShowMessage (Message);
	}
	else
	{
		ShowMessage (T ("ZX81 (emulador de Paul Farrow)"));
	}

	return TRUE;
}

void CKernel::StopZX81 (void)
{
	m_ZX81Loader.Cancel (&m_Keyboard);

	if (IsZX81Active ())
	{
		m_pSpectrum->EjectCartridge ();
		m_CartridgeName[0] = '\0';
	}
	m_bZX81 = FALSE;
}

// Tras cada cuadro emulado: la carga del .P, si hay una en marcha
void CKernel::ZX81Frame (void)
{
	if (!m_ZX81Loader.IsActive ())
	{
		return;
	}

	if (!IsZX81Active ())		// se ha cargado otra cosa (un snapshot...)
	{
		StopZX81 ();
		return;
	}

	m_ZX81Loader.Frame (m_pSpectrum, &m_Keyboard);
}
