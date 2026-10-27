//
// earsource.h - De dónde viene la señal del casete real para la entrada EAR
//
//   CGPIOEarSource: un GPIO con un comparador (LM393) que convierte el audio
//     del casete en niveles lógicos. Cada flanco se anota con su hora (reloj de
//     1 MHz) en una interrupción.
//   CUSBEarSource: la entrada de micrófono o línea de una tarjeta de sonido USB,
//     a 48 kHz. El nivel lógico se decide por software, con histéresis y
//     siguiendo el nivel medio de la señal.
//
// Las dos entregan, cuadro a cuadro, los cambios de nivel en t-states con un
// retraso fijo (EAR_LATENCY_US), para que el Spectrum los vea al mismo ritmo
// que el casete aunque la emulación vaya a ráfagas.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_earsource_h
#define _zx_earsource_h

#include "earinput.h"
#include <circle/gpiopin.h>
#include <circle/gpiomanager.h>
#include <circle/sound/soundbasedevice.h>
#include <circle/types.h>

#define EAR_LATENCY_US	40000		// dos cuadros

class CEarSource
{
public:
	virtual ~CEarSource (void) {}

	// Rellena los cambios del próximo cuadro (nFrameNs = duración del cuadro en
	// nanosegundos; nTstatesFrame y nClock, los del modelo)
	virtual void FillFrame (CEarInput *pInput, u64 nFrameNs, unsigned nTstatesFrame,
				unsigned nClock) = 0;

	// La emulación se ha parado (menú, pausa, velocidad xN): al volver se
	// resincroniza con el tiempo real
	virtual void Resync (void) = 0;
};

class CGPIOEarSource : public CEarSource
{
public:
	CGPIOEarSource (unsigned nPin, boolean bInvert, CGPIOManager *pManager);
	~CGPIOEarSource (void);

	boolean Initialize (void);
	void FillFrame (CEarInput *pInput, u64 nFrameNs, unsigned nTstatesFrame, unsigned nClock);
	void Resync (void)		{ m_bSynced = FALSE; }

private:
	static void InterruptHandler (void *pParam);

	enum { QueueSize = 16384 };	// flancos (unos 2 s de carga turbo)

	unsigned m_nPin;
	boolean m_bInvert;
	CGPIOPin m_Pin;

	volatile u32 m_Time[QueueSize];		// µs
	volatile u8 m_Level[QueueSize];
	volatile unsigned m_nHead;		// lo escribe la interrupción
	unsigned m_nTail;

	boolean m_bSynced;
	u64 m_nWindowNs;			// comienzo del cuadro en tiempo real (ns)
};

class CUSBEarSource : public CEarSource
{
public:
	CUSBEarSource (CSoundBaseDevice *pDevice);	// ya creado en modo de entrada

	boolean Initialize (void);
	void FillFrame (CEarInput *pInput, u64 nFrameNs, unsigned nTstatesFrame, unsigned nClock);
	void Resync (void)		{ m_bSynced = FALSE; }

private:
	enum { SampleRate = 48000, BufferSize = 8192 };

	CSoundBaseDevice *m_pDevice;
	s16 m_Buffer[BufferSize];
	unsigned m_nBuffered;		// muestras pendientes en m_Buffer
	u64 m_nPhase;			// fracción de muestra (ns * muestras) acumulada
	boolean m_bSynced;

	int m_nAverage;			// nivel medio (x256)
	int m_nPeak;			// amplitud aproximada
	boolean m_bLevel;
};

#endif
