//
// audiomixer.h - Generación de muestras de audio (beeper + AY) sincronizada con los t-states
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_audiomixer_h
#define _zx_audiomixer_h

#include "ay8912.h"
#include "tape.h"
#include "earinput.h"
#include <circle/types.h>

#define AUDIO_SAMPLE_RATE	48000
#define AUDIO_MAX_FRAME_SAMPLES	1024	// 48000/50 = 960 + margen

class CAudioMixer
{
public:
	CAudioMixer (void);

	void Configure (unsigned nClockFreq, CAY8912 *pAY);	// pAY == 0 -> sin AY
	void Reset (void);

	// Avanza la generación de audio hasta el t-state indicado del cuadro actual
	void AdvanceTo (int nTstates);

	// Cambia el nivel del beeper; llamar antes a AdvanceTo()
	void SetBeeper (int nLevel)	{ m_nBeeper = nLevel; }

	// Sonido de la carga: la señal de la cinta (entrada EAR) también llega al
	// altavoz, como en el Spectrum real. nFrameStart = t-states absolutos del
	// comienzo del cuadro actual.
	void SetTape (CTape *pTape)		{ m_pTape = pTape; }
	void SetEarInput (CEarInput *pEar)	{ m_pEarInput = pEar; }
	void SetTapeSound (boolean bOn)		{ m_bTapeSound = bOn; }

	// Filtro paso bajo del beeper (y del sonido de la carga): en el Spectrum
	// real el altavoz y el circuito no dejan pasar los agudos. Sin él se oye la
	// portadora de las rutinas de sonido de 1 bit por PWM (p. ej. un pitido
	// fijo de unos 16 kHz en muchas demos), que en la máquina real no suena.
	void SetBeeperFilter (boolean bOn)	{ m_bBeeperFilter = bOn; }
	boolean GetBeeperFilter (void) const	{ return m_bBeeperFilter; }

	// Modelo del altavoz, como en Fuse (sound/ula_filter.c, speaker_filter.c,
	// tv_filter.c, de Fredrick Meunier): la respuesta eléctrica de la ULA (un
	// paso bajo asimétrico, ajustado con grabaciones de máquinas reales) y luego
	//   - Beeper: el altavoz interno del 48K, que no da graves (paso alto de
	//     750 Hz);
	//   - TV: el televisor, por el que suenan el 128K, el +2 y el +3 (100 Hz -
	//     10 kHz);
	//   - Off: sin modelo (solo se quita la componente continua).
	// Solo afecta al beeper y al sonido de la carga, no al AY.
	enum TSpeaker { SpeakerOff, SpeakerBeeper, SpeakerTV };
	void SetSpeaker (TSpeaker Speaker);
	TSpeaker GetSpeaker (void) const	{ return m_Speaker; }
	boolean GetTapeSound (void) const	{ return m_bTapeSound; }
	void SetFrameStart (u64 nFrameStart)	{ m_nFrameStart = nFrameStart; }

	// Cierra el cuadro: genera hasta nTstatesFrame y reajusta el contador
	void EndFrame (int nTstatesFrame);

	// Muestras estéreo s16 intercaladas (L,R) generadas en el último cuadro
	const s16 *GetBuffer (void) const	{ return m_Buffer; }
	unsigned GetFrameCount (void) const	{ return m_nFrames; }	// pares L/R
	void ClearBuffer (void)			{ m_nFrames = 0; }

private:
	void Emit (void);

private:
	unsigned m_nClockFreq;
	CAY8912 *m_pAY;
	CTape *m_pTape;
	CEarInput *m_pEarInput;
	boolean m_bTapeSound;
	u64 m_nFrameStart;

	int m_nLastTstate;
	unsigned m_nPhase;
	int m_nBeeper;

	int m_nAccLeft, m_nAccRight, m_nAccCount;
	int m_nAccBeeper;

	// paso bajo de 4.º orden (Butterworth, 6 kHz a 48 kHz): dos biquads
	boolean m_bBeeperFilter;
	float m_LPState[2][4];		// x1, x2, y1, y2 de cada biquad

	TSpeaker m_Speaker;
	float m_fULA;			// estado del filtro de la ULA
	boolean m_bULAInit;
	float m_HPState[4];		// paso alto de 750 Hz (altavoz del 48K)
	float m_fTVIn, m_fTVHigh, m_fTVLow;	// televisor

	// filtro paso alto para eliminar la componente continua del beeper
	int m_nDCInL, m_nDCOutL, m_nDCInR, m_nDCOutR;

	s16 m_Buffer[AUDIO_MAX_FRAME_SAMPLES * 2];
	unsigned m_nFrames;
};

#endif
