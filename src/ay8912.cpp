//
// ay8912.cpp - Emulación del AY-3-8912
//
// Portado a C++ desde JSpeccy (machine/AY8912.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "ay8912.h"
#include <circle/util.h>

enum
{
	FineToneA = 0, CoarseToneA, FineToneB, CoarseToneB, FineToneC, CoarseToneC,
	NoisePeriod, Mixer, AmplitudeA, AmplitudeB, AmplitudeC,
	FineEnvelope, CoarseEnvelope, EnvelopeShapeCycle, IOPortA, IOPortB
};

#define TONE_A		0x01
#define TONE_B		0x02
#define TONE_C		0x04
#define NOISE_A		0x08
#define NOISE_B		0x10
#define NOISE_C		0x20
#define ENVELOPE	0x10
#define HOLD		0x01
#define ALTERNATE	0x02
#define ATTACK		0x04
#define CONTINUE	0x08

// Niveles de volumen del AY (en milésimas), de JSpeccy
static const int VolumeRate[16] =
{
	0, 14, 21, 29, 42, 62, 85, 137, 169, 265, 353, 450, 570, 687, 848, 1000
};

#define MAX_AMPLITUDE	7000	// por canal; tres canales no saturan un s16

CAY8912::CAY8912 (void)
:	m_StereoMode (AYStereoABC)
{
	for (unsigned i = 0; i < 16; i++)
	{
		m_VolumeLevel[i] = MAX_AMPLITUDE * VolumeRate[i] / 1000;
	}

	Reset ();
}

void CAY8912::Reset (void)
{
	m_nPeriodA = m_nPeriodB = m_nPeriodC = m_nPeriodN = 1;
	m_nCounterA = m_nCounterB = m_nCounterC = m_nCounterN = 0;
	m_nAmplitudeA = m_nAmplitudeB = m_nAmplitudeC = m_nAmplitudeEnv = 0;
	m_nEnvelopePeriod = 2;
	m_nEnvelopeCounter = 0;
	m_nAddressLatch = 0;
	m_bToneA = m_bToneB = m_bToneC = m_bToneN = false;
	m_bEnvA = m_bEnvB = m_bEnvC = false;
	m_bHold = m_bAlternate = false;
	m_nRng = 1;
	memset (m_Reg, 0, sizeof m_Reg);
	m_Reg[Mixer] = 0xFF;
	m_bDisableToneA = m_bDisableToneB = m_bDisableToneC = true;
	m_bDisableNoiseA = m_bDisableNoiseB = m_bDisableNoiseC = true;
	m_bContinue = m_bAttack = false;
}

u8 CAY8912::ReadRegister (void) const
{
	if (   m_nAddressLatch >= 14
	    && ((m_Reg[Mixer] >> (m_nAddressLatch - 8)) & 1) == 0)
	{
		return 0xFF;
	}

	return m_Reg[m_nAddressLatch];
}

void CAY8912::UpdateEnvelopeAmplitudes (void)
{
	if (m_bEnvA) m_nAmplitudeA = m_VolumeLevel[m_nAmplitudeEnv];
	if (m_bEnvB) m_nAmplitudeB = m_VolumeLevel[m_nAmplitudeEnv];
	if (m_bEnvC) m_nAmplitudeC = m_VolumeLevel[m_nAmplitudeEnv];
}

void CAY8912::WriteRegister (u8 nValue)
{
	switch (m_nAddressLatch)
	{
	case FineToneA:
	case CoarseToneA:
		m_Reg[m_nAddressLatch] = nValue & (m_nAddressLatch == FineToneA ? 0xFF : 0x0F);
		m_nPeriodA = (m_Reg[CoarseToneA] << 8) | m_Reg[FineToneA];
		break;

	case FineToneB:
	case CoarseToneB:
		m_Reg[m_nAddressLatch] = nValue & (m_nAddressLatch == FineToneB ? 0xFF : 0x0F);
		m_nPeriodB = (m_Reg[CoarseToneB] << 8) | m_Reg[FineToneB];
		break;

	case FineToneC:
	case CoarseToneC:
		m_Reg[m_nAddressLatch] = nValue & (m_nAddressLatch == FineToneC ? 0xFF : 0x0F);
		m_nPeriodC = (m_Reg[CoarseToneC] << 8) | m_Reg[FineToneC];
		break;

	case NoisePeriod:
		m_Reg[NoisePeriod] = nValue & 0x1F;
		break;

	case Mixer:
		m_Reg[Mixer] = nValue;
		m_bDisableToneA  = (nValue & TONE_A) != 0;
		m_bDisableToneB  = (nValue & TONE_B) != 0;
		m_bDisableToneC  = (nValue & TONE_C) != 0;
		m_bDisableNoiseA = (nValue & NOISE_A) != 0;
		m_bDisableNoiseB = (nValue & NOISE_B) != 0;
		m_bDisableNoiseC = (nValue & NOISE_C) != 0;
		break;

	case AmplitudeA:
		m_Reg[AmplitudeA] = nValue & 0x1F;
		m_bEnvA = (nValue & ENVELOPE) != 0;
		m_nAmplitudeA = m_VolumeLevel[m_bEnvA ? m_nAmplitudeEnv : (nValue & 0x0F)];
		break;

	case AmplitudeB:
		m_Reg[AmplitudeB] = nValue & 0x1F;
		m_bEnvB = (nValue & ENVELOPE) != 0;
		m_nAmplitudeB = m_VolumeLevel[m_bEnvB ? m_nAmplitudeEnv : (nValue & 0x0F)];
		break;

	case AmplitudeC:
		m_Reg[AmplitudeC] = nValue & 0x1F;
		m_bEnvC = (nValue & ENVELOPE) != 0;
		m_nAmplitudeC = m_VolumeLevel[m_bEnvC ? m_nAmplitudeEnv : (nValue & 0x0F)];
		break;

	case FineEnvelope:
	case CoarseEnvelope:
		m_Reg[m_nAddressLatch] = nValue;
		m_nEnvelopePeriod = (m_Reg[CoarseEnvelope] << 8) | m_Reg[FineEnvelope];
		m_nEnvelopePeriod = m_nEnvelopePeriod == 0 ? 2 : m_nEnvelopePeriod << 1;
		break;

	case EnvelopeShapeCycle:
		m_Reg[EnvelopeShapeCycle] = nValue & 0x0F;
		m_bHold      = (nValue & HOLD) != 0;
		m_bAlternate = (nValue & ALTERNATE) != 0;
		m_bAttack    = (nValue & ATTACK) != 0;
		m_nAmplitudeEnv = m_bAttack ? 0 : 15;
		m_bContinue = true;
		m_nEnvelopeCounter = 0;
		UpdateEnvelopeAmplitudes ();
		break;

	case IOPortA:
	case IOPortB:
		m_Reg[m_nAddressLatch] = nValue;
		break;
	}
}

void CAY8912::Tick (int *pLeft, int *pRight)
{
	if (++m_nCounterA >= m_nPeriodA) { m_bToneA = !m_bToneA; m_nCounterA = 0; }
	if (++m_nCounterB >= m_nPeriodB) { m_bToneB = !m_bToneB; m_nCounterB = 0; }
	if (++m_nCounterC >= m_nPeriodC) { m_bToneC = !m_bToneC; m_nCounterC = 0; }

	if (++m_nCounterN >= m_nPeriodN)
	{
		m_nCounterN = 0;
		m_nPeriodN = m_Reg[NoisePeriod];
		if (m_nPeriodN == 0)
		{
			m_nPeriodN = 1;
		}
		m_nPeriodN <<= 1;

		// ¿Va a cambiar la salida del ruido? (bit0 ^ bit1)
		if (((m_nRng + 1) & 0x02) != 0)
		{
			m_bToneN = !m_bToneN;
		}

		// Registro de desplazamiento de 17 bits (configuración Galois)
		if (m_nRng & 0x01)
		{
			m_nRng ^= 0x24000;
		}
		m_nRng >>= 1;
	}

	if (m_bContinue && ++m_nEnvelopeCounter >= m_nEnvelopePeriod)
	{
		m_nEnvelopeCounter = 0;
		m_nAmplitudeEnv += m_bAttack ? 1 : -1;

		if (m_nAmplitudeEnv & 0x10)
		{
			if ((m_Reg[EnvelopeShapeCycle] & CONTINUE) == 0)
			{
				m_nAmplitudeEnv = 0;
				m_bContinue = false;
			}
			else
			{
				if (m_bAlternate)
				{
					m_bAttack = !m_bAttack;
				}

				if (m_bHold)
				{
					m_nAmplitudeEnv = m_bAttack ? 15 : 0;
					m_bContinue = false;
				}
				else
				{
					m_nAmplitudeEnv = m_bAttack ? 0 : 15;
				}
			}
		}

		UpdateEnvelopeAmplitudes ();
	}

	int nA = ((m_bToneA || m_bDisableToneA) && (m_bToneN || m_bDisableNoiseA)) ? m_nAmplitudeA : 0;
	int nB = ((m_bToneB || m_bDisableToneB) && (m_bToneN || m_bDisableNoiseB)) ? m_nAmplitudeB : 0;
	int nC = ((m_bToneC || m_bDisableToneC) && (m_bToneN || m_bDisableNoiseC)) ? m_nAmplitudeC : 0;

	switch (m_StereoMode)
	{
	case AYStereoABC:
	default:
		*pLeft  = nA + (nB >> 1);
		*pRight = nC + (nB >> 1);
		break;

	case AYStereoACB:
		*pLeft  = nA + (nC >> 1);
		*pRight = nB + (nC >> 1);
		break;

	case AYStereoBAC:
		*pLeft  = nB + (nA >> 1);
		*pRight = nC + (nA >> 1);
		break;

	case AYStereoMono:
		*pLeft = *pRight = (nA + nB + nC) >> 1;	// mismo volumen máximo que en estéreo
		break;
	}
}
