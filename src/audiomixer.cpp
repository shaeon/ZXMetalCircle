//
// audiomixer.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "audiomixer.h"

#define AY_STEP_TSTATES	16
#define TAPE_LEVEL	2500	// la cinta suena más baja que el beeper

CAudioMixer::CAudioMixer (void)
:	m_nClockFreq (3500000),
	m_pAY (0),
	m_pTape (0),
	m_pEarInput (0),
	m_bTapeSound (TRUE),
	m_nFrameStart (0),
	m_bBeeperFilter (TRUE),
	m_Speaker (SpeakerBeeper)
{
	Reset ();
}

void CAudioMixer::Configure (unsigned nClockFreq, CAY8912 *pAY)
{
	m_nClockFreq = nClockFreq;
	m_pAY = pAY;
	Reset ();
}

void CAudioMixer::Reset (void)
{
	m_nLastTstate = 0;
	m_nPhase = 0;
	m_nBeeper = 0;
	m_nAccLeft = m_nAccRight = m_nAccCount = 0;
	m_nAccBeeper = 0;
	m_nDCInL = m_nDCOutL = m_nDCInR = m_nDCOutR = 0;
	for (unsigned i = 0; i < 2; i++)
	{
		for (unsigned j = 0; j < 4; j++)
		{
			m_LPState[i][j] = 0.0f;
		}
	}
	m_fULA = 0.0f;
	m_bULAInit = FALSE;
	for (unsigned j = 0; j < 4; j++)
	{
		m_HPState[j] = 0.0f;
	}
	m_fTVIn = m_fTVHigh = m_fTVLow = 0.0f;
	m_nFrames = 0;
}

void CAudioMixer::SetSpeaker (TSpeaker Speaker)
{
	if (Speaker != m_Speaker)
	{
		m_Speaker = Speaker;
		m_bULAInit = FALSE;		// sin saltos al cambiar de modelo
		for (unsigned j = 0; j < 4; j++)
		{
			m_HPState[j] = 0.0f;
		}
		m_fTVIn = m_fTVHigh = m_fTVLow = 0.0f;
	}
}

void CAudioMixer::AdvanceTo (int nTstates)
{
	while (m_nLastTstate + AY_STEP_TSTATES <= nTstates)
	{
		m_nLastTstate += AY_STEP_TSTATES;

		int nBeeper = m_nBeeper;
		if (m_bTapeSound && m_pTape != 0 && m_pTape->IsPlaying ()
		    && m_pTape->GetEarBit (m_nFrameStart + m_nLastTstate))
		{
			nBeeper += TAPE_LEVEL;
		}
		else if (   m_bTapeSound && m_pEarInput != 0 && m_pEarInput->IsEnabled ()
			 && m_pEarInput->IsActive () && m_pEarInput->GetLevel (m_nLastTstate))
		{
			nBeeper += TAPE_LEVEL;	// se oye el casete real
		}

		// el beeper se acumula aparte (lleva su propio filtro) y el AY aparte
		int nLeft = 0, nRight = 0;
		if (m_pAY != 0)
		{
			m_pAY->Tick (&nLeft, &nRight);
		}

		m_nAccBeeper += nBeeper;
		m_nAccLeft += nLeft;
		m_nAccRight += nRight;
		m_nAccCount++;

		m_nPhase += AY_STEP_TSTATES * AUDIO_SAMPLE_RATE;
		if (m_nPhase >= m_nClockFreq)
		{
			m_nPhase -= m_nClockFreq;
			Emit ();
		}
	}
}

// Paso bajo de 4.º orden, Butterworth, corte en 6 kHz a 48 kHz (dos biquads)
static const float LPCoef[2][5] =
{
	// b0, b1, b2, a1, a2
	{0.08857936f, 0.17715871f, 0.08857936f, -0.85539793f, 0.20971536f},
	{0.11525802f, 0.23051603f, 0.11525802f, -1.11302986f, 0.57406192f},
};

// Modelo del altavoz de Fuse, a 48 kHz
#define ULA_ALPHA_RISE	0.43459915f	// constante de tiempo 36,5 us
#define ULA_ALPHA_FALL	0.26106330f	// constante de tiempo 68,9 us
static const float HP750[5] =		// paso alto de 750 Hz, Q 0,707: b0, b1, b2, a1, a2
	{0.93293216f, -1.86586431f, 0.93293216f, -1.86136115f, 0.87036748f};
#define TV_HP_DECAY	0.98699533f	// paso alto de 100 Hz
#define TV_LP_ALPHA	0.72990916f	// paso bajo de 10 kHz

void CAudioMixer::Emit (void)
{
	// beeper: media del periodo de la muestra (ya quita algo de agudos)
	float fBeeper = (float) m_nAccBeeper / m_nAccCount;

	if (m_Speaker != SpeakerOff)
	{
		// respuesta eléctrica de la ULA (sube más rápido que baja)
		if (!m_bULAInit)
		{
			m_fULA = fBeeper;
			m_bULAInit = TRUE;
			m_HPState[0] = m_HPState[1] = fBeeper;	// sin escalón al empezar
			m_HPState[2] = m_HPState[3] = 0.0f;
			m_fTVIn = fBeeper;
		}
		m_fULA += (fBeeper > m_fULA ? ULA_ALPHA_RISE : ULA_ALPHA_FALL) * (fBeeper - m_fULA);
		fBeeper = m_fULA;

		if (m_Speaker == SpeakerBeeper)		// altavoz interno: sin graves
		{
			float *st = m_HPState;
			float y =   HP750[0] * fBeeper + HP750[1] * st[0] + HP750[2] * st[1]
				  - HP750[3] * st[2] - HP750[4] * st[3];
			st[1] = st[0]; st[0] = fBeeper;
			st[3] = st[2]; st[2] = y;
			fBeeper = y * 2.5f;		// compensa los graves que quita
		}
		else					// televisor: 100 Hz - 10 kHz
		{
			m_fTVHigh = TV_HP_DECAY * (m_fTVHigh + fBeeper - m_fTVIn);
			m_fTVIn = fBeeper;
			m_fTVLow += TV_LP_ALPHA * (m_fTVHigh - m_fTVLow);
			fBeeper = m_fTVLow * 1.4f;
		}
	}

	// paso bajo contra el pitido de las portadoras de 1 bit
	if (m_bBeeperFilter)
	{
		for (unsigned i = 0; i < 2; i++)
		{
			const float *c = LPCoef[i];
			float *st = m_LPState[i];
			float y = c[0] * fBeeper + c[1] * st[0] + c[2] * st[1] - c[3] * st[2] - c[4] * st[3];
			st[1] = st[0]; st[0] = fBeeper;
			st[3] = st[2]; st[2] = y;
			fBeeper = y;
		}
	}
	int nBeeper = (int) fBeeper;

	int nL = m_nAccLeft / m_nAccCount + nBeeper;
	int nR = m_nAccRight / m_nAccCount + nBeeper;
	m_nAccLeft = m_nAccRight = m_nAccCount = 0;
	m_nAccBeeper = 0;

	// y[n] = x[n] - x[n-1] + 0.996 * y[n-1]
	int nOutL = nL - m_nDCInL + (m_nDCOutL * 255 >> 8);
	int nOutR = nR - m_nDCInR + (m_nDCOutR * 255 >> 8);
	m_nDCInL = nL; m_nDCOutL = nOutL;
	m_nDCInR = nR; m_nDCOutR = nOutR;

	if (nOutL > 32767) nOutL = 32767; else if (nOutL < -32768) nOutL = -32768;
	if (nOutR > 32767) nOutR = 32767; else if (nOutR < -32768) nOutR = -32768;

	if (m_nFrames < AUDIO_MAX_FRAME_SAMPLES)
	{
		m_Buffer[m_nFrames * 2]     = (s16) nOutL;
		m_Buffer[m_nFrames * 2 + 1] = (s16) nOutR;
		m_nFrames++;
	}
}

void CAudioMixer::EndFrame (int nTstatesFrame)
{
	AdvanceTo (nTstatesFrame);
	m_nLastTstate -= nTstatesFrame;
}
