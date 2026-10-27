//
// earsource.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "earsource.h"
#include <circle/timer.h>
#include <circle/util.h>

////////////////////////////////////////////////////////////////////////////////
// GPIO

CGPIOEarSource::CGPIOEarSource (unsigned nPin, boolean bInvert, CGPIOManager *pManager)
:	m_nPin (nPin),
	m_bInvert (bInvert),
	m_Pin (nPin, GPIOModeInputPullUp, pManager),
	m_nHead (0),
	m_nTail (0),
	m_bSynced (FALSE),
	m_nWindowNs (0)
{
}

CGPIOEarSource::~CGPIOEarSource (void)
{
	m_Pin.DisableInterrupt ();
	m_Pin.DisableInterrupt2 ();
	m_Pin.DisconnectInterrupt ();
}

boolean CGPIOEarSource::Initialize (void)
{
	m_Pin.ConnectInterrupt (InterruptHandler, this);
	m_Pin.EnableInterrupt (GPIOInterruptOnRisingEdge);
	m_Pin.EnableInterrupt2 (GPIOInterruptOnFallingEdge);
	return TRUE;
}

// En la interrupción: hora y nivel del flanco
void CGPIOEarSource::InterruptHandler (void *pParam)
{
	CGPIOEarSource *pThis = static_cast<CGPIOEarSource *> (pParam);

	u32 nTime = CTimer::GetClockTicks ();
	unsigned nLevel = pThis->m_Pin.Read ();

	unsigned nHead = pThis->m_nHead;
	unsigned nNext = (nHead + 1) % QueueSize;
	if (nNext != pThis->m_nTail)		// si se llena, se pierden flancos
	{
		pThis->m_Time[nHead] = nTime;
		pThis->m_Level[nHead] = (nLevel != 0) ^ pThis->m_bInvert;
		pThis->m_nHead = nNext;
	}
}

void CGPIOEarSource::FillFrame (CEarInput *pInput, u64 nFrameNs, unsigned nTstatesFrame,
				unsigned nClock)
{
	u64 nNowNs = CTimer::GetClockTicks64 () * 1000;
	u64 nTarget = nNowNs - EAR_LATENCY_US * 1000ULL;

	// La ventana del cuadro avanza exactamente un cuadro cada vez, para que el
	// tiempo del casete y el del Spectrum no se separen. Si se ha separado mucho
	// (la emulación se paró), se vuelve a alinear y se descarta lo antiguo.
	if (   !m_bSynced
	    || m_nWindowNs + 100000000ULL < nTarget || m_nWindowNs > nTarget + 100000000ULL)
	{
		m_nWindowNs = nTarget;
		m_bSynced = TRUE;
	}

	u64 nStart = m_nWindowNs;
	u64 nEnd = nStart + nFrameNs;
	m_nWindowNs = nEnd;

	// El reloj de 32 bits da la vuelta cada 71 minutos: se comparan diferencias
	u32 nStartUs = (u32) (nStart / 1000);
	u32 nLengthUs = (u32) (nFrameNs / 1000);

	while (m_nTail != m_nHead)
	{
		s32 nOffset = (s32) (m_Time[m_nTail] - nStartUs);
		if (nOffset >= (s32) nLengthUs)
		{
			break;			// es del cuadro siguiente
		}

		if (nOffset >= 0)
		{
			unsigned nTstate = (unsigned) ((u64) nOffset * nClock / 1000000);
			if (nTstate >= nTstatesFrame)
			{
				nTstate = nTstatesFrame - 1;
			}
			pInput->AddEdge (nTstate, m_Level[m_nTail] != 0);
		}
		else
		{
			pInput->AddEdge (0, m_Level[m_nTail] != 0);	// llegó tarde: al principio
		}

		m_nTail = (m_nTail + 1) % QueueSize;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Tarjeta de sonido USB

CUSBEarSource::CUSBEarSource (CSoundBaseDevice *pDevice)
:	m_pDevice (pDevice),
	m_nBuffered (0),
	m_nPhase (0),
	m_bSynced (FALSE),
	m_nAverage (0),
	m_nPeak (2000),
	m_bLevel (TRUE)
{
}

boolean CUSBEarSource::Initialize (void)
{
	if (m_pDevice == 0 || !m_pDevice->AllocateReadQueue (200))
	{
		return FALSE;
	}
	m_pDevice->SetReadFormat (SoundFormatSigned16, 1);	// mono
	return TRUE;
}

void CUSBEarSource::FillFrame (CEarInput *pInput, u64 nFrameNs, unsigned nTstatesFrame,
			       unsigned nClock)
{
	// Lo que haya llegado de la tarjeta
	if (m_nBuffered < BufferSize)
	{
		int nBytes = m_pDevice->Read (m_Buffer + m_nBuffered,
					      (BufferSize - m_nBuffered) * sizeof (s16));
		if (nBytes > 0)
		{
			m_nBuffered += nBytes / sizeof (s16);
		}
	}

	// Retraso fijo: se mantienen unas EAR_LATENCY_US de muestras en reserva. Si
	// hay muchas más (la emulación se paró o la tarjeta va algo más rápida), se
	// descartan; si faltan, este cuadro espera.
	const unsigned nTarget = SampleRate / 1000 * (EAR_LATENCY_US / 1000);
	if (!m_bSynced || m_nBuffered > nTarget * 3)
	{
		if (m_nBuffered > nTarget)
		{
			unsigned nDrop = m_nBuffered - nTarget;
			memmove (m_Buffer, m_Buffer + nDrop, nTarget * sizeof (s16));
			m_nBuffered = nTarget;
		}
		m_bSynced = TRUE;
	}

	// Muestras que corresponden a este cuadro (con la fracción acumulada)
	m_nPhase += nFrameNs * SampleRate;
	unsigned nSamples = (unsigned) (m_nPhase / 1000000000ULL);
	m_nPhase %= 1000000000ULL;
	if (nSamples > m_nBuffered)
	{
		nSamples = m_nBuffered;		// faltan datos: el cuadro va algo corto
	}

	for (unsigned i = 0; i < nSamples; i++)
	{
		int nSample = m_Buffer[i];

		// nivel medio (para quitar la continua) y amplitud, que se adaptan solos:
		// la amplitud sube deprisa y baja muy despacio, y la histéresis tiene un
		// mínimo, para que el ruido de los silencios no invente flancos
		m_nAverage += ((nSample << 8) - m_nAverage) >> 12;
		int nCentered = nSample - (m_nAverage >> 8);
		int nAbs = nCentered < 0 ? -nCentered : nCentered;
		if (nAbs > m_nPeak)
		{
			m_nPeak += (nAbs - m_nPeak) >> 6;
		}
		else
		{
			m_nPeak -= (m_nPeak - nAbs) >> 14;
		}
		int nHysteresis = m_nPeak / 3;
		if (nHysteresis < 300)
		{
			nHysteresis = 300;
		}

		boolean bLevel = m_bLevel;
		if (nCentered > nHysteresis)
		{
			bLevel = TRUE;
		}
		else if (nCentered < -nHysteresis)
		{
			bLevel = FALSE;
		}

		if (bLevel != m_bLevel)
		{
			m_bLevel = bLevel;
			unsigned nTstate = (unsigned) ((u64) i * nTstatesFrame / (nSamples ? nSamples : 1));
			pInput->AddEdge (nTstate, bLevel);
		}
	}

	memmove (m_Buffer, m_Buffer + nSamples, (m_nBuffered - nSamples) * sizeof (s16));
	m_nBuffered -= nSamples;
}
