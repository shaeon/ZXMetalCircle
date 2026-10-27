//
// videocore.h - Ampliación de la imagen en el núcleo 1
//
// En la Pi 2, 3 y 4 (con ARM_ALLOW_MULTI_CORE), el núcleo 0 sigue con todo
// (emulación, sonido, USB, red, SD) y el núcleo 1 se encarga solo de ampliar
// cada cuadro a la pantalla (CVideo::Render), que es lo que más memoria mueve.
//
//   - Submit copia el cuadro terminado a un búfer propio y avisa al núcleo 1.
//     Si el núcleo 1 aún está con el anterior, ese cuadro no se muestra (como
//     cuando la emulación va con retraso).
//   - El núcleo 1 lo amplía en la página oculta y avisa; el cambio de página
//     (una llamada a la GPU) lo hace el núcleo 0 en el siguiente Poll o Submit.
//   - Para cambiar el modo de vídeo, Pause detiene al núcleo 1 y Resume lo
//     deja seguir.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_videocore_h
#define _zx_videocore_h

#include <circle/sysconfig.h>
#include <circle/types.h>
#include "video.h"

#ifdef ARM_ALLOW_MULTI_CORE

#include <circle/multicore.h>

class CVideoCore : public CMultiCoreSupport
{
public:
	CVideoCore (CVideo *pVideo);
	~CVideoCore (void);

	boolean Start (void);

	// Núcleo 0
	boolean Submit (const TPixel *pScreen);	// FALSE: núcleo 1 ocupado, no se muestra
	// Como Submit, pero espera a que el cuadro esté en pantalla (como mucho
	// nTimeoutMs). Para avisos mientras la emulación está parada.
	void SubmitAndWait (const TPixel *pScreen, unsigned nTimeoutMs);
	void Poll (void);			// cambia de página si hay una lista
	void Pause (void);			// antes de CVideo::SetMode
	void Resume (void);

	// Tiempo del último Render en el núcleo 1 (microsegundos)
	unsigned GetRenderTime (void) const	{ return m_nRenderTime; }

	void Run (unsigned nCore);

private:
	static void Wake (void);

	CVideo *m_pVideo;
	TPixel *m_pBuffer;

	volatile boolean m_bPending;		// hay un cuadro para ampliar
	volatile boolean m_bFlipPending;	// ampliado: falta cambiar de página
	volatile boolean m_bPauseRequest;
	volatile boolean m_bPaused;
	volatile unsigned m_nRenderTime;
};

#endif

#endif
