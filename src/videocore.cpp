//
// videocore.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "videocore.h"

#ifdef ARM_ALLOW_MULTI_CORE

#include <circle/memory.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <circle/util.h>

CVideoCore::CVideoCore (CVideo *pVideo)
:	CMultiCoreSupport (CMemorySystem::Get ()),
	m_pVideo (pVideo),
	m_pBuffer (new TPixel[SCREEN_WIDTH * SCREEN_HEIGHT]),
	m_bPending (FALSE),
	m_bFlipPending (FALSE),
	m_bPauseRequest (FALSE),
	m_bPaused (FALSE),
	m_nRenderTime (0)
{
}

CVideoCore::~CVideoCore (void)
{
	delete [] m_pBuffer;
}

boolean CVideoCore::Start (void)
{
	return Initialize ();		// arranca los núcleos 1-3 en Run ()
}

// Despierta al núcleo 1 si está esperando (WFE)
void CVideoCore::Wake (void)
{
	DataSyncBarrier ();
	asm volatile ("sev");
}

void CVideoCore::Poll (void)
{
	if (m_bFlipPending)
	{
		DataMemBarrier ();
		m_pVideo->Flip ();
		DataMemBarrier ();
		m_bFlipPending = FALSE;
		Wake ();
	}
}

boolean CVideoCore::Submit (const TPixel *pScreen)
{
	Poll ();

	if (m_bPending || m_bFlipPending || m_bPauseRequest)
	{
		return FALSE;			// el núcleo 1 sigue con el anterior
	}

	memcpy (m_pBuffer, pScreen, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
	DataMemBarrier ();
	m_bPending = TRUE;
	Wake ();
	return TRUE;
}

void CVideoCore::SubmitAndWait (const TPixel *pScreen, unsigned nTimeoutMs)
{
	unsigned nStart = CTimer::GetClockTicks ();
	unsigned nLimit = nTimeoutMs * (CLOCKHZ / 1000);

	while (!Submit (pScreen))
	{
		if (CTimer::GetClockTicks () - nStart > nLimit)
		{
			return;
		}
	}
	while (m_bPending || m_bFlipPending)
	{
		Poll ();			// cambia de página en cuanto el núcleo 1 acaba
		if (CTimer::GetClockTicks () - nStart > nLimit)
		{
			return;
		}
	}
}

void CVideoCore::Pause (void)
{
	m_bPauseRequest = TRUE;
	Wake ();
	while (!m_bPaused)
	{
		Poll ();		// por si el núcleo 1 acaba de terminar un cuadro
	}
	Poll ();
	m_bPending = FALSE;		// el cuadro a medias se descarta
}

void CVideoCore::Resume (void)
{
	DataMemBarrier ();
	m_bPauseRequest = FALSE;
	Wake ();
	while (m_bPaused)
	{
	}
}

void CVideoCore::Run (unsigned nCore)
{
	if (nCore != 1)
	{
		return;			// los núcleos 2 y 3 no hacen nada
	}

	for (;;)
	{
		if (m_bPauseRequest)
		{
			m_bPaused = TRUE;
			DataSyncBarrier ();
			while (m_bPauseRequest)
			{
				asm volatile ("wfe");
			}
			m_bPaused = FALSE;
			DataSyncBarrier ();
			continue;
		}

		if (!m_bPending || m_bFlipPending)
		{
			asm volatile ("wfe");	// hasta que el núcleo 0 avise (SEV)
			continue;
		}

		DataMemBarrier ();
		unsigned nStart = CTimer::GetClockTicks ();
		m_pVideo->Render (m_pBuffer);
		m_nRenderTime = CTimer::GetClockTicks () - nStart;

		DataSyncBarrier ();
		m_bFlipPending = TRUE;
		m_bPending = FALSE;
		DataSyncBarrier ();
	}
}

#endif
