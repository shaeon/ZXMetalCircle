//
// machinebase.h - Lo que el kernel necesita de cualquier máquina emulada
//
// El kernel habla con la máquina a través de CMachine (su nombre y sus
// tiempos, emular un cuadro, la imagen, el sonido, el reset y la memoria) en
// vez de con CSpectrum directamente. De momento la única máquina es CSpectrum
// (todos los modelos de Spectrum y los TK); lo que solo tiene el Spectrum
// (cinta, disco del +3, periféricos...) se va separando en la fase 3 (ver
// kernelspectrum.cpp).
//
// Coste: solo se llama a través de CMachine una vez por cuadro (RunFrame) o
// desde los menús; lo que se hace millones de veces por segundo (la memoria
// y los puertos del Z80) sigue dentro de CSpectrum, sin pasar por aquí.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_machinebase_h
#define _zx_machinebase_h

#include "palette.h"
#include "filetypes.h"
#include <circle/types.h>

class CAudioMixer;

// Lo más que ocupa el estado de una máquina (el búfer del kernel para F8 y
// para cargar snapshots). Spectrum: un .Z80 de 128K sin comprimir, ~131 KB
#define SNAPSHOT_MAX_SIZE	(256 * 1024)

class CMachine
{
public:
	virtual ~CMachine (void) {}

	// El modelo que se emula ahora: "ZX Spectrum 48K" y "48k"
	virtual const char *GetName (void) const = 0;
	virtual const char *GetShortName (void) const = 0;

	// Reloj del procesador (Hz) y ciclos de un cuadro: de ahí sale el ritmo
	// de la emulación (50 Hz, 60 Hz...)
	virtual unsigned GetClockFreq (void) const = 0;
	virtual unsigned GetFrameTstates (void) const = 0;
	u64 GetFrameNs (void) const
		{ return (u64) GetFrameTstates () * 1000000000ULL / GetClockFreq (); }
	unsigned GetFrameUs (void) const
		{ return (unsigned) ((u64) GetFrameTstates () * 1000000 / GetClockFreq ()); }

	// Está cargando de cinta o de disco en un modo rápido: el kernel emula
	// sin esperar al reloj (y sin sonido)
	virtual boolean IsAccelerating (void) const = 0;

	// Los ficheros que sabe cargar y qué es cada uno (filetypes.h)
	virtual const TFileType *GetFileTypes (void) const = 0;

	// Estado de la máquina (snapshots). Formatos 0..GetStateFormats () - 1;
	// GetStateExt da su extensión, sin el punto ("z80"). Los textos de error
	// van sin traducir (N_), para T () del kernel.
	virtual unsigned GetStateFormats (void) const = 0;
	virtual const char *GetStateExt (unsigned nFormat) const = 0;
	// El formato va por la extensión de pFileName. Devuelve el error o 0.
	virtual const char *LoadState (const u8 *pData, unsigned nSize, const char *pFileName) = 0;
	// 0 si se puede grabar ahora; si no, por qué
	virtual const char *CanSaveState (void) const = 0;
	// Los bytes del fichero (0 = no se pudo)
	virtual unsigned SaveState (unsigned nFormat, u8 *pBuffer, unsigned nBufferSize) = 0;

	// Emula un cuadro entero (imagen y sonido)
	virtual void RunFrame (void) = 0;

	// Reset de la máquina (como el botón); GetResetCount cuenta los que ha
	// habido, para que quien guarde algo de la máquina sepa que ya no vale
	virtual void Reset (void) = 0;
	virtual unsigned GetResetCount (void) const = 0;

	// La imagen del último cuadro (SCREEN_WIDTH x SCREEN_HEIGHT, con el borde)
	// y el sonido
	virtual TPixel *GetScreenBuffer (void) = 0;
	virtual CAudioMixer *GetAudio (void) = 0;

	// La memoria tal como la ve el procesador ahora (pokes, trucos)
	virtual u8 ReadMemory (u16 nAddress) const = 0;
	virtual void WriteMemory (u16 nAddress, u8 nValue) = 0;
};

#endif
