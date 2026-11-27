//
// kernelspectrum.cpp
//
// Lo que el kernel hace solo porque la máquina es un Spectrum. Fase 3 de la
// capa de máquinas: el bucle principal (kernel.cpp) es el de cualquier
// máquina (emular un cuadro, esperar, sonido e imagen) y llama aquí para lo
// demás: el reloj de esxDOS, la red del Interface 1 y sus cartuchos, la
// Spectranet, la carga de un .P en el ZX81 y la elección de bloque de la
// cinta. Y, de los snapshots, lo que depende del Spectrum: la pregunta por la
// paleta ULAplus al grabar y, al cargar, el .SP con ROM y el modelo sin ROM.
// Y las líneas del estado de F1 que hablan de la máquina.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include "i18n.h"
#include <circle/string.h>
#include <circle/timer.h>

// Tras emular cada cuadro (también a velocidad xN)
void CKernel::MachineFrame (void)
{
	ZX81Frame ();				// la carga de un .P, si hay una en marcha

	// Reloj para esxDOS (RTC.SYS): la hora de la Pi, si ya la tiene (NTP)
	if (m_pSpectrum->IsDivMMCActive ())
	{
		UpdateRTC ();
	}

	// Interface 1: RS232 y red ZX Net hacia la red de la Pi
	if (m_pSpectrum->IsIF1Connected ())
	{
		m_IF1Link.Poll (m_pSpectrum->GetInterface1 ());
	}

	// Cinta con bloque de selección (0x28): se pregunta qué cargar
	if (m_Tape.IsSelectPending () && !m_Menu.IsOpen ())
	{
		OpenTapeSelect ();
	}

	// Spectranet: atender sus sockets y guardar la flash cuando lleva 2 s sin
	// cambios (la configuración, módulos nuevos...)
	if (m_pSpectrum->IsSpectranetEnabled ())
	{
		CSpectranet *pSnet = m_pSpectrum->GetSpectranet ();
		pSnet->Poll ();
		if (pSnet->TakeFlashModified ())
		{
			m_nSnetSaveFrames = 100;
		}
		else if (m_nSnetSaveFrames > 0 && --m_nSnetSaveFrames == 0)
		{
			SaveSpectranetFlash ();
		}
	}
}

// Una vez por vuelta del bucle principal (tras los cuadros de esa vuelta)
void CKernel::MachineHousekeeping (void)
{
	// Los cartuchos modificados se graban cuando el motor lleva al menos 2 s
	// parado, y como mucho una vez cada 3 s: grabar cuesta (convertir y
	// escribir ~200 KB) y el CP/M arranca y para el motor muy a menudo
	if (m_pSpectrum->IsIF1Connected ())
	{
		u64 nNow = CTimer::GetClockTicks64 ();
		if (m_pSpectrum->GetInterface1 ()->GetRunningDrive () >= 0)
		{
			m_nMotorStop = nNow;
		}
		else if (   nNow - m_nMotorStop >= 2000000
			 && nNow - m_nLastMDRSave >= 3000000)
		{
			m_nLastMDRSave = nNow;
			SaveMicrodrives (FALSE);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Snapshots

// F8 con el formato de snapformat: Z80 y SNA no guardan la paleta de la
// ULAplus (al cargarlos, el juego saldría con otros colores). Si está en uso,
// se pregunta (SZX, elegido al entrar) y se devuelve TRUE; la respuesta llega
// a SnapMenuResult.
boolean CKernel::MachineAskStateFormat (unsigned nFormat)
{
	if (nFormat == 2 || !m_pSpectrum->IsULAplusInUse ())
	{
		return FALSE;
	}

	if (!m_Menu.IsOpen ())
	{
		memcpy (m_pFrozen, m_pMachine->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
		UpdateStatusLines ();
		OpenMenu ();
		m_Input.SetCaptured (TRUE);
	}
	m_nSnapStage = 1;
	m_Menu.OpenChoice (nFormat == 1 ? T ("SNA no guarda la paleta ULAplus")
					: T ("Z80 no guarda la paleta ULAplus"));
	m_Menu.SetChoiceItem (0, T ("Grabar en SZX (con la paleta)"));
	m_Menu.SetChoiceItem (1, nFormat == 1 ? T ("Grabar en SNA igualmente")
					      : T ("Grabar en Z80 igualmente"));
	m_Menu.SetChoiceItem (2, T ("No grabar"));
	m_Menu.SetChoiceSelected (0);
	return TRUE;
}

boolean CKernel::SnapMenuResult (TMenuResult Result)
{
	if (m_nSnapStage == 0 || (Result != MenuResultChoice && Result != MenuResultChoiceCancel))
	{
		return FALSE;
	}
	m_nSnapStage = 0;
	unsigned nChoice = Result == MenuResultChoice ? m_Menu.GetChoice () : 2;
	if (m_Menu.IsOpen ())
	{
		m_Menu.Close ();
	}
	if (nChoice == 0)
	{
		SaveSnapshot (2);			// SZX
	}
	else if (nChoice == 1)
	{
		SaveSnapshot ((int) m_nSnapFormat);	// el elegido, sin la paleta
	}
	else
	{
		ShowMessage (T ("Snapshot no grabado"));
	}
	return TRUE;
}

// Tras cargar un snapshot: el nombre del cartucho (un .SP con su propia ROM)
// y, si el snapshot pedía un modelo sin ROM (se ha cargado en el 128K), el
// mensaje que lo dice
boolean CKernel::MachineStateLoaded (CString *pMessage)
{
	if (m_pSpectrum->IsCartridgeInserted ())
	{
		strcpy (m_CartridgeName, ".SP (ROM)");
	}
	else
	{
		m_CartridgeName[0] = '\0';
	}

	TModel Missing = m_pSpectrum->TakeFallback ();
	if (Missing != ModelCount)
	{
		pMessage->Format (T ("Falta la ROM de %s: cargado en 128K"), MachineTiming[Missing].pShortName);
		return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// F1: las líneas 0-6 del estado (modelo, joystick, carga, cinta, sonido y
// periféricos). Las de debajo (imagen, red y hora) son del kernel.
void CKernel::MachineStatusLines (void)
{
	CString Line;

	Line.Format (T ("Modelo: |%s%s|  Vel. |x%u|"), m_pMachine->GetName (),
		     (m_pSpectrum->IsIssue2 () && m_pSpectrum->GetTiming ()->CodeModel == CodeModel48K)
		     ? (m_pSpectrum->IsLECConnected () ? " +LEC" : " (Issue 2)")
		     : (m_pSpectrum->IsLECConnected () && m_pSpectrum->GetModel () == Model48K ? " +LEC" : ""),
		     m_nSpeed);
	m_Menu.SetStatusLine (0, Line);

	CString Joy (T (JoystickNames[m_Keyboard.GetJoystickMode ()]));
	if (m_Keyboard.GetJoystickMode2 () != JoystickNone)
	{
		Joy.Append (" + ");			// el del jugador 2
		Joy.Append (T (JoystickNames[m_Keyboard.GetJoystickMode2 ()]));
	}
	Line.Format (T ("Joystick: |%s%s|  |%u.%02u c/s|"), (const char *) Joy,
		     m_Keyboard.IsMouseActive () ? T (" +ratón") : "", m_nFPSx100 / 100, m_nFPSx100 % 100);
	m_Menu.SetStatusLine (1, Line);

	if (m_pSpectrum->IsIF1Connected ())
	{
		Line.Format (T ("Carga: |%s|  Microdrives: |%u|"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]),
			     CountMicrodrives ());
	}
	else if (m_pSpectrum->HasDiskDrive ())
	{
		CPlus3Disk *pDisk = m_pSpectrum->GetDisk ();
		Line.Format (T ("Carga: |%s|  Disq. A:|%s| B:|%s%s|"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]),
			     pDisk->HasDisk (0) ? T ("sí") : "-", pDisk->HasDisk (1) ? T ("sí") : "-",
			     m_pSpectrum->HasZXMMC () && m_HardDisk.IsOpen () ? T (" HD:sí") : "");
	}
	else if (m_pEarSource != 0)
	{
		Line.Format (T ("Carga: |%s|  Casete: |%s|"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]),
			     m_pSpectrum->IsEarInputActive () ? T ("con señal") : T ("sin señal"));
	}
	else
	{
		Line.Format (T ("Carga: |%s|"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]));
	}
	m_Menu.SetStatusLine (2, Line);

	if (m_Tape.IsInserted ())
	{
		Line.Format (T ("Cinta: |%s|"), m_Tape.GetName ());
		m_Menu.SetStatusLine (3, Line);
		Line.Format (T ("|Bloque %u de %u%s|%s"), m_Tape.GetBlock () + 1 > m_Tape.GetBlockCount ()
						? m_Tape.GetBlockCount () : m_Tape.GetBlock () + 1,
			     m_Tape.GetBlockCount (), m_Tape.IsPlaying () ? T (" (en marcha)") : "",
			     m_Tape.GetUnsupportedBlocks () > 0 ? T (" *no soportados") : "");
		m_Menu.SetStatusLine (4, Line);
	}
	else
	{
		m_Menu.SetStatusLine (3, T ("Cinta: |(ninguna)|"));
		m_Menu.SetStatusLine (4, "");
	}

	CString Volume;
	if (m_bMute)
	{
		Volume = T ("silencio");
	}
	else
	{
		Volume.Format ("%u%%", m_nVolume);
	}
	Line.Format (T ("Sonido: |%s|, AY |%s|, vol. |%s|"), T (m_pSoundName),
		     AYStereoNames[m_pSpectrum->GetAY ()->GetStereoMode ()], (const char *) Volume);
	m_Menu.SetStatusLine (5, Line);

	if (m_pSpectrum->IsCartridgeInserted ())
	{
		Line.Format (T ("Cartucho: |%s|"), m_CartridgeName[0] != '\0' ? m_CartridgeName : T ("(snapshot)"));
	}
	else
	{
		if (m_pSpectrum->IsSpectranetEnabled ())
		{
			Line.Format (T ("Spectranet: |sí|  ULAplus: |%s|"), m_pSpectrum->IsULAplusActive () ? T ("activo") : T ("no"));
		}
		else if (m_pSpectrum->IsDivMMCActive ())
		{
			Line.Format (T ("divMMC: |esxDOS|  ULAplus: |%s|"), m_pSpectrum->IsULAplusActive () ? T ("activo") : T ("no"));
		}
		else
		{
			Line.Format (T ("Multiface: |%s|  ULAplus: |%s|"),
				     m_pSpectrum->IsMultifaceAvailable () ? T ("sí") : T ("no"),
				     m_pSpectrum->IsULAplusActive () ? T ("activo") : T ("no"));
		}
	}
	m_Menu.SetStatusLine (6, Line);
}
