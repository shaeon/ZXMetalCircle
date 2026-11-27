//
// kernelspectrum.cpp
//
// Lo que el kernel hace solo porque la máquina es un Spectrum. Fase 3 de la
// capa de máquinas: el bucle principal (kernel.cpp) es el de cualquier
// máquina (emular un cuadro, esperar, sonido e imagen) y llama aquí para lo
// demás: el reloj de esxDOS, la red del Interface 1 y sus cartuchos, la
// Spectranet, la carga de un .P en el ZX81 y la elección de bloque de la
// cinta.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
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
