//
// ay8912.h - Emulación del AY-3-8912
//
// Portado a C++ desde JSpeccy (machine/AY8912.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_ay8912_h
#define _zx_ay8912_h

#include <circle/types.h>

// Reparto de los tres canales entre los altavoces. El canal central se oye por
// los dos lados a mitad de volumen.
enum TAYStereoMode
{
	AYStereoABC,		// A izquierda, B centro, C derecha (el más habitual)
	AYStereoACB,		// A izquierda, C centro, B derecha
	AYStereoBAC,		// B izquierda, A centro, C derecha
	AYStereoMono,
	AYStereoModeCount
};

class CAY8912
{
public:
	CAY8912 (void);

	void Reset (void);

	u8 GetAddressLatch (void) const		{ return m_nAddressLatch; }
	void SetAddressLatch (u8 nValue)	{ m_nAddressLatch = nValue & 0x0F; }

	u8 ReadRegister (void) const;
	u8 GetRegister (unsigned nReg) const	{ return m_Reg[nReg & 0x0F]; }
	void WriteRegister (u8 nValue);

	void SetStereoMode (TAYStereoMode Mode)	{ m_StereoMode = Mode; }
	TAYStereoMode GetStereoMode (void) const	{ return m_StereoMode; }

	// Avanza el chip un paso (16 t-states de CPU = 8 ciclos de reloj AY)
	// y devuelve la salida izquierda y derecha según el modo estéreo.
	void Tick (int *pLeft, int *pRight);

private:
	void UpdateEnvelopeAmplitudes (void);

private:
	u8 m_Reg[16];
	u8 m_nAddressLatch;

	int m_nPeriodA, m_nPeriodB, m_nPeriodC, m_nPeriodN;
	int m_nCounterA, m_nCounterB, m_nCounterC, m_nCounterN;
	int m_nAmplitudeA, m_nAmplitudeB, m_nAmplitudeC;
	unsigned m_nRng;
	int m_nEnvelopePeriod, m_nEnvelopeCounter;
	int m_nAmplitudeEnv;

	bool m_bContinue, m_bAttack, m_bHold, m_bAlternate;
	bool m_bToneA, m_bToneB, m_bToneC, m_bToneN;
	bool m_bDisableToneA, m_bDisableToneB, m_bDisableToneC;
	bool m_bDisableNoiseA, m_bDisableNoiseB, m_bDisableNoiseC;
	bool m_bEnvA, m_bEnvB, m_bEnvC;

	int m_VolumeLevel[16];
	TAYStereoMode m_StereoMode;
};

#endif
