//
// gpiokeyboard.h - Teclado real de ZX Spectrum (y joysticks Atari en la matriz)
//                  conectado a los GPIO, compatible con el montaje de pastbytes
//                  para ZXBaremulator: https://pastbytes.com/zxb/
//
// Conexiones (número de GPIO BCM, entre paréntesis el pin del conector):
//
//   Filas (con diodo, cátodo hacia la Raspberry):
//     A8  GPIO18 (12)   A9  GPIO23 (16)   A10 GPIO24 (18)   A11 GPIO25 (22)
//     A12 GPIO22 (15)   A13 GPIO27 (13)   A14 GPIO17 (11)   A15 GPIO4  (7)
//   Columnas:
//     D0  GPIO26 (37)   D1  GPIO19 (35)   D2  GPIO13 (33)   D3  GPIO6  (31)
//     D4  GPIO5  (29)
//   Pulsador de modo extendido (opcional): GPIO21 (40) a GND (39)
//   Pulsador de NMI (opcional, añadido en ZXCircle): GPIO16 (36) a GND (34)
//
// Los joysticks del montaje van cableados a la matriz como en el Interface 2
// de Sinclair (JOY1 = teclas 6-0, JOY2 = teclas 1-5), así que no necesitan nada más.
//
// Tipos de teclado (opción gpiokbd=):
//   48 (o 1, 128): matriz de 8 x 5, la de siempre. Vale para el 48K de goma,
//       el Spectrum+, el 128 y el +2 gris: sus teclas de más (cursores, EDIT,
//       DELETE, ";", ".", ","...) cierran dos contactos a la vez en la membrana.
//   plus3 (o plus2a): teclado del +2A/+3, matriz de 13 x 11 como en el
//       proyecto zxunops2 de spark2k06 (versión ATmega644). Las 8 primeras
//       filas y 5 primeras columnas son las de siempre; las teclas de más van en
//       5 filas y 6 columnas propias y aquí se traducen a la combinación del
//       Spectrum (cursor = CAPS SHIFT + 5..8, ";" = SYMBOL SHIFT + O, etc.).
//       Filas de más: R8 GPIO20 (38), R9 GPIO21 (40), R10 GPIO16 (36),
//                     R11 GPIO12 (32), R12 GPIO7 (26)
//       Columnas de más: C5 GPIO8 (24), C6 GPIO9 (21), C7 GPIO10 (19),
//                        C8 GPIO11 (23), C9 GPIO2 (3), C10 GPIO3 (5)
//       Como usa los GPIO21, GPIO16 y GPIO12, con este teclado no hay pulsador
//       de modo extendido (sirve CAPS SHIFT + SYMBOL SHIFT + ENTER), ni pulsador
//       de NMI, ni audio por el GPIO12.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_gpiokeyboard_h
#define _zx_gpiokeyboard_h

#include <circle/gpiopin.h>
#include <circle/types.h>

#define GPIO_KBD_ROWS		8	// matriz del Spectrum
#define GPIO_KBD_COLS		5
#define GPIO_KBD_MAX_ROWS	13	// teclado del +2A/+3
#define GPIO_KBD_MAX_COLS	11

enum TGPIOKeyboardType
{
	GPIOKeyboard48,		// 8 x 5: 48K, Spectrum+, 128 y +2 gris
	GPIOKeyboardPlus3	// 13 x 11: +2A y +3
};

class CGPIOKeyboard
{
public:
	CGPIOKeyboard (void);
	~CGPIOKeyboard (void);

	boolean Initialize (TGPIOKeyboardType Type = GPIOKeyboard48);
	TGPIOKeyboardType GetType (void) const	{ return m_Type; }

	// Lee la matriz completa (unos 100 us; unos 200 us con el teclado del +3)
	void Scan (void);

	// Matriz del Spectrum: filas A8..A15; bit n = columna Dn pulsada. Con el
	// teclado del +3 ya incluye la traducción de las teclas de más.
	const u8 *GetMatrix (void) const	{ return m_Matrix; }

	// Matriz leída tal cual (13 filas; bit n = columna n), para el diagnóstico
	const u16 *GetRawMatrix (void) const	{ return m_Raw; }

	boolean IsButtonPressed (void) const	{ return m_bButton; }
	boolean IsNMIPressed (void) const	{ return m_bNMI; }

private:
	void TranslatePlus3Keys (void);

	TGPIOKeyboardType m_Type;
	unsigned m_nRows, m_nCols;
	const unsigned *m_pRowPins;
	const unsigned *m_pColPins;

	CGPIOPin *m_pRow[GPIO_KBD_MAX_ROWS];
	CGPIOPin *m_pCol[GPIO_KBD_MAX_COLS];
	CGPIOPin *m_pButton;
	CGPIOPin *m_pNMI;
	u32 m_nRowMask;

	u16 m_Raw[GPIO_KBD_MAX_ROWS];		// bit n = columna n pulsada
	u8 m_Matrix[GPIO_KBD_ROWS];
	boolean m_bButton;
	boolean m_bNMI;
};

#endif
