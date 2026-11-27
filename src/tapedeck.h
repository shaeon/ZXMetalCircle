//
// tapedeck.h - El casete del Spectrum
//
// Fase 2d de la capa de máquinas: lo que hace el Spectrum con la cinta sale
// de CSpectrum. CTape es la cinta (bloques, pulsos, nivel en cada instante);
// CTapeDeck, lo que pasa alrededor:
//
//  - los mandos (Play, Stop) y el arranque y la parada automáticos según
//    lea o no el programa la cinta;
//  - las trampas de la ROM de BASIC: carga instantánea (0x0562), SAVE
//    instantáneo (0x04D0) y arrancar la cinta al cargar (0x0556);
//  - la aceleración de cargadores (se salta sus bucles de espera);
//  - la grabación por pulsos de la salida MIC o de la entrada EAR externa
//    (casete real por GPIO o tarjeta de sonido).
//
// Mira los registros del Z80 y la memoria (para reconocer cargadores y para
// la carga instantánea), y escribe a través de CSpectrum (pWrite), como lo
// haría el Z80. Lo que se usa en cada lectura del puerto 0xFE (ReadEar,
// CountLoaderRead) no es virtual.
//
// La carga instantánea y la aceleración de cargadores están adaptadas de
// Fuse (© 2006-2018 Philip Kendall y otros, GPL 2 o posterior).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_tapedeck_h
#define _zx_tapedeck_h

#include "tape.h"
#include "zxmemory.h"
#include "divmmc.h"
#include "earinput.h"
#include "z80.h"
#include <circle/types.h>

enum TTapeMode
{
	TapeModeFlash,		// trap de la ROM: carga instantánea de bloques estándar
	TapeModeFast,		// carga por pulsos a máxima velocidad (se ve la carga)
	TapeModeNormal,		// carga por pulsos a velocidad real
	TapeModeCount
};

// Recibe los bloques que la ROM graba con SAVE (trap de SA-BYTES)
class CTapeSaveHandler
{
public:
	virtual ~CTapeSaveHandler (void) {}

	// pBlock = flag + datos + paridad. Devuelve FALSE si no se pudo grabar.
	virtual boolean SaveTapeBlock (const u8 *pBlock, unsigned nLength) = 0;
};

class CTapeDeck
{
public:
	// Escribir un byte como el Z80 (carga instantánea)
	typedef void TWriteByte (void *pParam, u16 nAddress, u8 nValue);

	CTapeDeck (CTape *pTape, Z80 &rCPU, const CMemory &rMem, const CDivMMC &rDiv,
		   TWriteByte *pWrite, void *pWriteParam);
	~CTapeDeck (void);

	// Ajustes
	void SetMode (TTapeMode Mode)		{ m_Mode = Mode; }
	TTapeMode GetMode (void) const		{ return m_Mode; }
	void SetLoaderAcceleration (boolean bOn) { m_bLoaderAccel = bOn; m_AccelMode = AccelNone; }
	boolean GetLoaderAcceleration (void) const { return m_bLoaderAccel; }
	void SetSaveHandler (CTapeSaveHandler *pHandler) { m_pSaveHandler = pHandler; }	// 0 = SAVE "al aire"
	void SetEarInput (CEarInput *pEarInput)	{ m_pEarInput = pEarInput; }
	boolean IsEarInputActive (void) const;

	// Mandos (nNow: t-states absolutos)
	void Play (u64 nNow);
	void Stop (void);			// parada manual: no arranca sola
	void Inserted (void);			// tras insertar o rebobinar: arma el autoarranque
	boolean IsAccelerating (void) const;	// cargando en un modo rápido

	// Puerto 0xFE
	void CountLoaderRead (u16 nPort);	// en cada lectura (¿pinta de cargador?)
	boolean ReadEar (u64 nNow);		// nivel EAR con la cinta en marcha
	boolean ReadEarInput (int nTstates)	{ return m_pEarInput->GetLevel (nTstates); }
	void MICChanged (u64 nNow)		// cambio del bit MIC
	{
		if (m_bRecording && !m_bRecordEar)
		{
			RecordEdge (nNow);
		}
	}

	// Trampas de la ROM de BASIC (con la ROM de BASIC paginada, al leer la
	// instrucción). TRUE: hay que ejecutar un RET
	boolean SaveTrap (void);		// 0x04D0 (SA-BYTES)
	boolean LoadTrap (void);		// 0x0562 (LD-BYTES, el IN)
	void LoadBytes (u64 nNow);		// 0x0556 (LD-BYTES): arranca la cinta

	// Cada cuadro
	void RecordEarInput (u64 nFrameStart);	// antes de cerrar el sonido del cuadro
	void EndFrame (u64 nNow);		// ya en el cuadro siguiente

	// Grabación por pulsos. Formato: RLE de CSW, 1 muestra = 16 t-states.
	// bEarInput: graba la entrada EAR externa en lugar de la salida MIC.
	// Durante la grabación no se usa el trap de SAVE.
	boolean StartRecording (boolean bEarInput, u64 nNow);
	void StopRecording (u64 nNow);
	boolean IsRecording (void) const	{ return m_bRecording; }
	boolean IsRecordingEarInput (void) const { return m_bRecording && m_bRecordEar; }
	const u8 *GetRecording (unsigned *pLength, unsigned *pPulses, unsigned *pSampleRate,
				unsigned nClockFreq) const;
	void FreeRecording (void);

private:
	u8 ReadByte (u16 nAddress) const	{ return m_rMem.ReadPage (nAddress >> 14)[nAddress & 0x3FFF]; }
	boolean FlashLoad (void);		// carga instantánea en 0x0562 (como Fuse)
	boolean FlashSave (void);
	void RecordEdge (u64 nTime);

	// aceleración de cargadores (ver CheckLoaderAcceleration)
	enum TAccelMode { AccelNone, AccelIncreasing, AccelDecreasing };
	u8 PeekLoader (u16 nAddress) const;
	boolean LoaderPatternMatches (u16 nAddress, const struct TLoaderPatternByte *pPattern,
				      unsigned nLength) const;
	boolean LoaderWordMatches (u16 nAddress, u16 nValue) const;
	TAccelMode DetectLoader (u16 nPC) const;
	TAccelMode DetectLoaderAtIn (u16 nPC) const;
	boolean ModifiedROMLoader (u16 nPC) const;
	void CheckLoaderAcceleration (u64 nNow);

private:
	CTape *m_pTape;
	Z80 &m_rCPU;
	const CMemory &m_rMem;
	const CDivMMC &m_rDiv;
	TWriteByte *m_pWrite;
	void *m_pWriteParam;
	CEarInput *m_pEarInput;
	CTapeSaveHandler *m_pSaveHandler;
	TTapeMode m_Mode;
	boolean m_bLoaderAccel;

	// grabación por pulsos
	boolean m_bRecording;
	boolean m_bRecordEar;
	u8 *m_pRecBuffer;
	unsigned m_nRecLength, m_nRecCapacity, m_nRecPulses;
	u64 m_nRecLastEdge;
	unsigned m_nRecRemainder;

	// arranque y parada automáticos
	unsigned m_nIdleFrames;
	unsigned m_nLoadingFrames;
	boolean m_bAutoStopped;

	TAccelMode m_AccelMode;
	u16 m_nAccelPC;
	unsigned m_nAccelEdges;		// flancos acelerados en este cuadro
	// parada automática: ¿el programa sigue los flancos de la cinta?
	u32 m_nSeenEdge;		// último flanco que ha visto una lectura de 0xFE
	u32 m_nFrameEdges;		// flancos al empezar el cuadro
	unsigned m_nObservedEdges;	// lecturas de este cuadro que han visto un flanco nuevo
	// ¿lee como un cargador? (ver CountLoaderRead)
	unsigned m_nLoaderReads;	// lecturas de 0xFE de este cuadro con pinta de cargador
	u16 m_nLastFEPC;
	u16 m_nLastFEPort;
	u8 m_LastFERegs[6];		// B, C, D, E, H, L en la lectura anterior
};

#endif
