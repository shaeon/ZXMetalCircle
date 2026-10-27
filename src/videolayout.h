//
// videolayout.h - Cálculo del tamaño del framebuffer para las scanlines
//
// La GPU de la Raspberry escala el framebuffer a la pantalla CONSERVANDO SU
// PROPORCIÓN. Por eso el framebuffer debe tener la misma proporción que la
// pantalla (si no, la imagen sale estrecha y con bandas). Para que las scanlines
// sean regulares, además, el escalado total tiene que ser entero:
//
//   pantalla = framebuffer x s   (s entero, lo hace la GPU)
//   framebuffer: cada píxel del Spectrum ocupa r x r (r entero, lo hace la CPU)
//
// con r >= 2 (una o más líneas normales y la última más oscura).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_videolayout_h
#define _zx_videolayout_h

struct TScanlineLayout
{
	unsigned nFBWidth, nFBHeight;	// tamaño del framebuffer
	unsigned nRepeat;		// r: píxeles de framebuffer por píxel del Spectrum
	unsigned nGPUScale;		// s: escala entera que aplica la GPU

	// Aumento fraccionario (nRepeat = 0): framebuffer a resolución nativa y la
	// imagen ocupa nOutWidth x nOutHeight; cada línea del Spectrum ocupa 3 o 4
	// líneas de pantalla (por ejemplo) y la última va oscurecida
	unsigned nOutWidth, nOutHeight;
};

// Devuelve false si la pantalla no es lo bastante grande (hace falta r * s >= 2)
// bFast: la Pi puede escribir un framebuffer a resolución nativa en cada cuadro
// (Pi 2, 3 y 4). Entonces se usa siempre el mayor aumento entero posible; en la
// Pi 1/Zero se prefiere un framebuffer pequeño aunque la imagen quede algo menor.
// bCPUOnly: la GPU no escala el framebuffer (Pi 5: lo muestra tal cual, arriba a
// la izquierda), así que todo el aumento lo hace la CPU (s = 1)
static inline bool ComputeScanlineLayout (unsigned nDisplayWidth, unsigned nDisplayHeight,
					  unsigned nImageWidth, unsigned nImageHeight,
					  TScanlineLayout *pLayout, bool bFast = true,
					  bool bCPUOnly = false)
{
	// mayor aumento total entero que cabe en la pantalla
	unsigned nTotalV = nDisplayHeight / nImageHeight;
	unsigned nTotalH = nDisplayWidth / nImageWidth;
	unsigned nTotal = nTotalV < nTotalH ? nTotalV : nTotalH;
	if (nTotal < 2)
	{
		return false;
	}

	// Se reparte entre la CPU (r) y la GPU (s). Interesa que la CPU haga poco
	// (r = 2) para que el framebuffer sea pequeño; si el total es impar y mayor
	// que 3, compensa perder un escalón para poder usar r = 2.
	unsigned nRepeat, nScale;
	if (bCPUOnly)
	{
		nRepeat = nTotal;
		nScale = 1;
	}
	else if (nTotal % 2 == 0)
	{
		nRepeat = 2;
		nScale = nTotal / 2;
	}
	else if (nTotal == 3 || bFast)
	{
		// impar: framebuffer a resolución nativa, la CPU hace todo el aumento
		// (por ejemplo 1080p con borde completo: x3; 1200p con borde estándar: x5)
		nRepeat = nTotal;
		nScale = 1;
	}
	else
	{
		nRepeat = 2;		// Pi 1/Zero: se pierde un escalón, pero gasta mucho menos
		nScale = (nTotal - 1) / 2;
	}

	pLayout->nRepeat = nRepeat;
	pLayout->nGPUScale = nScale;
	pLayout->nFBWidth = (nDisplayWidth / nScale) & ~1U;	// misma proporción que la pantalla
	pLayout->nFBHeight = nDisplayHeight / nScale;
	pLayout->nOutWidth = nImageWidth * nTotal;
	pLayout->nOutHeight = nImageHeight * nTotal;

	// Si el aumento entero deja la imagen mucho más pequeña que sin scanlines
	// (menos del 90 %; por ejemplo el borde enorme a 1200 líneas: x3 en lugar
	// de x3,95), en las Pi rápidas se usa un aumento fraccionario a resolución
	// nativa: las líneas del Spectrum ocupan 3 o 4 líneas de pantalla, casi
	// iguales a esa resolución, y la imagen llena la pantalla igual que sin ellas
	unsigned nFill1000V = nDisplayHeight * 1000 / nImageHeight;
	unsigned nFill1000H = nDisplayWidth * 1000 / nImageWidth;
	unsigned nFill1000 = nFill1000V < nFill1000H ? nFill1000V : nFill1000H;
	if (bFast && nTotal * 1000 * 100 < nFill1000 * 90)
	{
		pLayout->nRepeat = 0;
		pLayout->nGPUScale = 1;
		pLayout->nFBWidth = nDisplayWidth & ~1U;
		pLayout->nFBHeight = nDisplayHeight;
		pLayout->nOutWidth = (nImageWidth * nFill1000 / 1000) & ~1U;
		pLayout->nOutHeight = nImageHeight * nFill1000 / 1000;
	}

	return true;
}

// Sin scanlines, cuando la GPU no escala (Pi 5): framebuffer a resolución
// nativa y la CPU amplía la imagen (aumento fraccionario, vecino más próximo)
// hasta llenar la pantalla conservando la proporción, como haría la GPU
static inline bool ComputeFillLayout (unsigned nDisplayWidth, unsigned nDisplayHeight,
				      unsigned nImageWidth, unsigned nImageHeight,
				      TScanlineLayout *pLayout)
{
	unsigned nFill1000V = nDisplayHeight * 1000 / nImageHeight;
	unsigned nFill1000H = nDisplayWidth * 1000 / nImageWidth;
	unsigned nFill1000 = nFill1000V < nFill1000H ? nFill1000V : nFill1000H;
	if (nFill1000 < 1000)
	{
		return false;		// la pantalla es más pequeña que la imagen
	}

	pLayout->nRepeat = 0;
	pLayout->nGPUScale = 1;
	pLayout->nFBWidth = nDisplayWidth & ~1U;
	pLayout->nFBHeight = nDisplayHeight;
	pLayout->nOutWidth = (nImageWidth * nFill1000 / 1000) & ~1U;
	pLayout->nOutHeight = nImageHeight * nFill1000 / 1000;

	return true;
}

#endif
