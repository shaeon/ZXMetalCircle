//
// earinput.h - Entrada EAR externa: la señal de un casete real
//
// El kernel rellena, antes de emular cada cuadro, la lista de cambios de nivel
// de la señal del casete que caen en ese cuadro (en t-states desde su
// comienzo). Viene de un GPIO con un comparador o de una tarjeta de sonido USB
// (ver earsource.h), con un retraso fijo de un par de cuadros. Cuando el Z80
// lee el puerto 0xFE, el bit EAR toma el nivel de ese instante.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_earinput_h
#define _zx_earinput_h

#include <circle/types.h>

#define EAR_MAX_EDGES	4096		// por cuadro (más que cualquier carga turbo)

class CEarInput
{
public:
	CEarInput (void);

	// Nuevo cuadro: el nivel con el que empieza es el último del anterior
	void BeginFrame (void);
	void AddEdge (unsigned nTstate, boolean bLevel);	// en orden creciente

	// Nivel en un instante del cuadro (las lecturas suelen ir en orden: se
	// recuerda dónde se quedó la anterior)
	boolean GetLevel (unsigned nTstate);

	// Hay señal (algún cambio en el último segundo y medio, más o menos)
	boolean IsActive (void) const		{ return m_nQuietFrames < 75; }
	void SetEnabled (boolean bOn)		{ m_bEnabled = bOn; }
	boolean IsEnabled (void) const		{ return m_bEnabled; }

	unsigned GetEdgeCount (void) const	{ return m_nEdges; }
	unsigned GetEdgeTime (unsigned i) const	{ return m_Time[i]; }

private:
	boolean m_bEnabled;
	boolean m_bStartLevel;		// nivel al empezar el cuadro
	boolean m_bLastLevel;		// nivel tras el último cambio añadido
	unsigned m_nEdges;
	u32 m_Time[EAR_MAX_EDGES];
	u8 m_Level[EAR_MAX_EDGES];
	unsigned m_nCursor;
	unsigned m_nQuietFrames;	// cuadros seguidos sin cambios
};

#endif
