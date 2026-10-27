//
// gpiokeyboard.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "gpiokeyboard.h"
#include <circle/timer.h>
#include <circle/util.h>

// Filas A8..A15 y columnas D0..D4 (montaje de pastbytes); en el teclado del
// +3, además, 5 filas y 6 columnas propias de las teclas de más
static const unsigned RowPins[GPIO_KBD_MAX_ROWS] = {18, 23, 24, 25, 22, 27, 17, 4,  20, 21, 16, 12, 7};
static const unsigned ColPins[GPIO_KBD_MAX_COLS] = {26, 19, 13, 6, 5,  8, 9, 10, 11, 2, 3};
#define BUTTON_PIN	21
#define NMI_PIN		16

#define SETTLE_US	4	// tras activar una fila, antes de leer
#define RECOVER_US	10	// tras soltarla, para que las columnas vuelvan a nivel alto

// Teclas de más del +2A/+3 (fila, columna de la matriz de 13 x 11) y su
// equivalente en el Spectrum, como en traduceextra2a () de zxunops2.
// Filas del Spectrum: 0 = A8 (CS..V) ... 7 = A15 (SPACE..B); bit = columna.
struct TPlus3Key
{
	u8 nRow, nCol;		// en la matriz de 13 x 11
	u8 nShiftRow, nShiftBit;	// CAPS SHIFT (0, 0x01) o SYMBOL SHIFT (7, 0x02)
	u8 nKeyRow, nKeyBit;
};

#define CS	0, 0x01
#define SS	7, 0x02

static const TPlus3Key Plus3Keys[] =
{
	{ 8,  9, SS, 5, 0x01},	// "      SYMBOL SHIFT + P
	{ 8, 10, SS, 5, 0x02},	// ;      SYMBOL SHIFT + O
	{ 9,  5, SS, 7, 0x04},	// .      SYMBOL SHIFT + M
	{ 9,  6, SS, 7, 0x08},	// ,      SYMBOL SHIFT + N
	{10,  5, CS, 3, 0x04},	// TRUE VIDEO   CAPS SHIFT + 3
	{10,  6, CS, 3, 0x08},	// INV. VIDEO   CAPS SHIFT + 4
	{10,  7, CS, 3, 0x01},	// EDIT         CAPS SHIFT + 1
	{10,  8, CS, 3, 0x02},	// CAPS LOCK    CAPS SHIFT + 2
	{10, 10, CS, 3, 0x10},	// cursor izq.  CAPS SHIFT + 5
	{11,  5, CS, 4, 0x04},	// cursor der.  CAPS SHIFT + 8
	{11,  6, CS, 4, 0x08},	// cursor arr.  CAPS SHIFT + 7
	{11,  7, CS, 4, 0x01},	// DELETE       CAPS SHIFT + 0
	{11,  8, CS, 4, 0x02},	// GRAPH        CAPS SHIFT + 9
	{11, 10, CS, 4, 0x10},	// cursor abajo CAPS SHIFT + 6
	{12,  7, CS, 7, 0x01},	// BREAK        CAPS SHIFT + SPACE
	{12,  8, CS, 7, 0x02},	// EXTEND MODE  CAPS SHIFT + SYMBOL SHIFT
};

CGPIOKeyboard::CGPIOKeyboard (void)
:	m_Type (GPIOKeyboard48),
	m_nRows (GPIO_KBD_ROWS),
	m_nCols (GPIO_KBD_COLS),
	m_pRowPins (RowPins),
	m_pColPins (ColPins),
	m_pButton (0),
	m_pNMI (0),
	m_nRowMask (0),
	m_bButton (FALSE),
	m_bNMI (FALSE)
{
	for (unsigned i = 0; i < GPIO_KBD_MAX_ROWS; i++)
	{
		m_pRow[i] = 0;
		m_Raw[i] = 0;
	}

	for (unsigned i = 0; i < GPIO_KBD_MAX_COLS; i++)
	{
		m_pCol[i] = 0;
	}

	memset (m_Matrix, 0, sizeof m_Matrix);
}

CGPIOKeyboard::~CGPIOKeyboard (void)
{
	for (unsigned i = 0; i < GPIO_KBD_MAX_ROWS; i++)
	{
		delete m_pRow[i];
	}

	for (unsigned i = 0; i < GPIO_KBD_MAX_COLS; i++)
	{
		delete m_pCol[i];
	}

	delete m_pButton;
	delete m_pNMI;
}

boolean CGPIOKeyboard::Initialize (TGPIOKeyboardType Type)
{
	m_Type = Type;
	m_nRows = Type == GPIOKeyboardPlus3 ? GPIO_KBD_MAX_ROWS : GPIO_KBD_ROWS;
	m_nCols = Type == GPIOKeyboardPlus3 ? GPIO_KBD_MAX_COLS : GPIO_KBD_COLS;

	// Las filas nunca se ponen a nivel alto: o son entradas (inactivas) o salidas
	// a nivel bajo (fila seleccionada), como un colector abierto. Así no se fuerza
	// nada si en esos pines hay conectado otro dispositivo.
	for (unsigned i = 0; i < m_nRows; i++)
	{
		m_pRow[i] = new CGPIOPin (RowPins[i], GPIOModeInputPullUp);
		m_nRowMask |= 1 << RowPins[i];
	}

	for (unsigned i = 0; i < m_nCols; i++)
	{
		m_pCol[i] = new CGPIOPin (ColPins[i], GPIOModeInputPullUp);
	}

	// el teclado del +3 usa los pines de los pulsadores
	if (Type == GPIOKeyboard48)
	{
		m_pButton = new CGPIOPin (BUTTON_PIN, GPIOModeInputPullUp);
		m_pNMI = new CGPIOPin (NMI_PIN, GPIOModeInputPullUp);
	}

	return TRUE;
}

void CGPIOKeyboard::Scan (void)
{
	if (m_pRow[0] == 0)
	{
		return;
	}

	// Latch de salida a 0 para todas las filas (no afecta mientras sean entradas)
	CGPIOPin::WriteAll (0, m_nRowMask);

	for (unsigned nRow = 0; nRow < m_nRows; nRow++)
	{
		u32 nRowBit = 1 << RowPins[nRow];

		CGPIOPin::SetModeAll (m_nRowMask & ~nRowBit, nRowBit);
		CTimer::SimpleusDelay (SETTLE_US);

		u32 nLevels = CGPIOPin::ReadAll ();

		CGPIOPin::SetModeAll (m_nRowMask, 0);

		u16 nKeys = 0;
		for (unsigned nCol = 0; nCol < m_nCols; nCol++)
		{
			if ((nLevels & (1 << ColPins[nCol])) == 0)
			{
				nKeys |= 1 << nCol;
			}
		}
		m_Raw[nRow] = nKeys;

		CTimer::SimpleusDelay (RECOVER_US);
	}

	u32 nLevels = CGPIOPin::ReadAll ();
	m_bButton = m_pButton != 0 && (nLevels & (1 << BUTTON_PIN)) == 0;
	m_bNMI = m_pNMI != 0 && (nLevels & (1 << NMI_PIN)) == 0;

	// Todas las teclas pulsadas a la vez es imposible: no hay teclado (o las
	// resistencias de pull-up no funcionan, como en QEMU). Se ignora la lectura.
	boolean bAllPressed = TRUE;
	for (unsigned nRow = 0; nRow < GPIO_KBD_ROWS; nRow++)
	{
		if ((m_Raw[nRow] & 0x1F) != 0x1F)
		{
			bAllPressed = FALSE;
		}
	}

	if (bAllPressed)
	{
		memset (m_Raw, 0, sizeof m_Raw);
		m_bButton = FALSE;
		m_bNMI = FALSE;
	}

	// Matriz del Spectrum: las 8 filas y 5 columnas de siempre...
	for (unsigned nRow = 0; nRow < GPIO_KBD_ROWS; nRow++)
	{
		m_Matrix[nRow] = m_Raw[nRow] & 0x1F;
	}

	// ... y, con el teclado del +3, sus teclas de más
	if (m_Type == GPIOKeyboardPlus3)
	{
		TranslatePlus3Keys ();
	}
}

void CGPIOKeyboard::TranslatePlus3Keys (void)
{
	for (unsigned i = 0; i < sizeof Plus3Keys / sizeof Plus3Keys[0]; i++)
	{
		const TPlus3Key &K = Plus3Keys[i];
		if (m_Raw[K.nRow] & (1 << K.nCol))
		{
			m_Matrix[K.nShiftRow] |= K.nShiftBit;
			m_Matrix[K.nKeyRow] |= K.nKeyBit;
		}
	}
}
