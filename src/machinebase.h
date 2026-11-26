//
// machinebase.h - Lo que el kernel necesita de cualquier máquina emulada
//
// Primer paso de la capa de máquinas: el kernel habla con la máquina a
// través de CMachine (emular un cuadro, la imagen, el sonido, el reset y la
// memoria) en vez de con CSpectrum directamente. De momento la única máquina
// es CSpectrum (todos los modelos de Spectrum y los TK); lo que solo tiene el
// Spectrum (cinta, disco del +3, periféricos...) se irá separando en pasos
// siguientes.
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
#include <circle/types.h>

class CAudioMixer;

class CMachine
{
public:
	virtual ~CMachine (void) {}

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
