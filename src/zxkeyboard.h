//
// zxkeyboard.h - Traducción de teclado USB (HID) a la matriz del Spectrum y joysticks
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_zxkeyboard_h
#define _zx_zxkeyboard_h

#include <circle/types.h>

// Mismo orden que el ciclo del botón Select de ZXBaremulator
enum TJoystickMode
{
	JoystickNone,		// cursores del PC = CAPS SHIFT + 5/6/7/8
	JoystickKempston,	// puerto 0x1F (disparo 2 = bit 5)
	JoystickFuller,		// puerto 0x7F
	JoystickSinclair1,	// 6-0, disparo 2 = X
	JoystickSinclair2,	// 1-5, disparo 2 = Z
	JoystickCursor,		// 5-8 + 0 (Cursor/AGF/Protek), disparo 2 = 9
	JoystickQAOP,		// Q A O P, disparo SPACE, disparo 2 = M
	JoystickModeCount
};

// Bits del estado de un joystick
#define JOY_RIGHT	0x01
#define JOY_LEFT	0x02
#define JOY_DOWN	0x04
#define JOY_UP		0x08
#define JOY_FIRE	0x10
#define JOY_FIRE2	0x20

#define MAX_GAMEPADS	4

// Qué hacen los cursores del teclado del PC cuando hay un joystick elegido
enum TCursorMode
{
	CursorBoth,		// teclas de cursor del Spectrum y joystick a la vez
	CursorKeysOnly,		// solo CAPS SHIFT + 5/6/7/8
	CursorJoystickOnly	// solo joystick
};

// Teclas HID de interés
#define HID_KEY_ESC		0x29
#define HID_KEY_ENTER		0x28
#define HID_KEY_BACKSPACE	0x2A
#define HID_KEY_TAB		0x2B
#define HID_KEY_SPACE		0x2C
#define HID_KEY_F1		0x3A
#define HID_KEY_F12		0x45
#define HID_KEY_PAUSE		0x48
#define HID_KEY_PAGEUP		0x4B
#define HID_KEY_PAGEDOWN	0x4E
#define HID_KEY_HOME		0x4A
#define HID_KEY_END		0x4D
#define HID_KEY_RIGHT		0x4F
#define HID_KEY_LEFT		0x50
#define HID_KEY_DOWN		0x51
#define HID_KEY_UP		0x52

#define HID_MOD_LCTRL		0x01
#define HID_MOD_LSHIFT		0x02
#define HID_MOD_LALT		0x04
#define HID_MOD_RCTRL		0x10
#define HID_MOD_RSHIFT		0x20
#define HID_MOD_RALT		0x40

struct TKeyEvent
{
	u8 nKey;		// código HID
	u8 nModifiers;
};

class CZXKeyboard
{
public:
	CZXKeyboard (void);

	// Llamado desde el manejador USB (contexto de interrupción)
	void UpdateFromReport (u8 nModifiers, const u8 RawKeys[6]);

	// Lectura del puerto del teclado (byte alto de la dirección)
	u8 ReadPort (u8 nHighByte) const;
	u8 ReadKempston (void) const;
	u8 ReadFuller (void) const;

	// Estado de un mando USB (bits JOY_*); se combina con los cursores del teclado
	void SetGamePadState (unsigned nIndex, u8 nState);

	// Ratón Kempston (activo si hay ratón y el joystick no es Kempston)
	void SetMousePresent (boolean bPresent);
	boolean IsMouseActive (void) const
		{ return m_bMousePresent && m_JoystickMode != JoystickKempston; }
	void MouseUpdate (unsigned nButtons, int nDX, int nDY, int nWheel);
	u8 ReadMouseButtons (void) const;
	u8 ReadMouseX (void) const	{ return m_nMouseX; }
	u8 ReadMouseY (void) const	{ return m_nMouseY; }

	void SetJoystickMode (TJoystickMode Mode);
	void SetCursorMode (TCursorMode Mode)	{ m_CursorMode = Mode; }

	// La tecla de borrar del PC envía cursor izquierda en lugar de DELETE
	// (lo activa el kernel mientras el CP/M de LEC está en marcha)
	void SetBackspaceAsLeft (boolean bOn)	{ m_bBackspaceAsLeft = bOn; }
	TJoystickMode GetJoystickMode (void) const	{ return m_JoystickMode; }

	// Si está activo, las teclas no llegan al Spectrum (menús del emulador)
	void SetCaptured (boolean bCaptured)	{ m_bCaptured = bCaptured; }
	boolean IsCaptured (void) const		{ return m_bCaptured; }

	// Teclado real por GPIO: se combina con el USB en ReadPort()
	void SetGPIOMatrix (const u8 Matrix[8]);

	// Eventos generados por otros teclados (GPIO en modo extendido o en menús)
	void InjectKeyEvent (u8 nKey, u8 nModifiers)	{ PushEvent (nKey, nModifiers); }

	// Tecla HID equivalente a una tecla de la matriz (0 si no tiene: CS, SS)
	static u8 MatrixKeyToHID (unsigned nRow, unsigned nBit);

	// Cola de pulsaciones (flancos de bajada) para el emulador
	boolean GetKeyEvent (TKeyEvent *pEvent);

	// Tecla de navegación (cursores, RePág, AvPág) que sigue pulsada en el
	// teclado USB, o 0 (para la repetición en los menús)
	u8 GetHeldNavKey (void) const;

	void ReleaseAll (void);

private:
	u8 GetJoystickState (void) const;
	void PushEvent (u8 nKey, u8 nModifiers);

private:
	volatile u8 m_Matrix[8];
	volatile u8 m_GPIOMatrix[8];
	volatile u8 m_nKeyJoy;			// joystick desde los cursores del teclado
	volatile u8 m_PadJoy[MAX_GAMEPADS];	// joystick desde los mandos

	volatile boolean m_bMousePresent;
	volatile u8 m_nMouseX, m_nMouseY;
	volatile u8 m_nMouseWheel;		// contador de 4 bits (Kempston Mouse Turbo)
	volatile unsigned m_nMouseButtons;
	TJoystickMode m_JoystickMode;
	TCursorMode m_CursorMode;
	volatile boolean m_bBackspaceAsLeft;
	volatile boolean m_bCaptured;

	u8 m_PrevKeys[6];

	TKeyEvent m_Events[32];
	volatile unsigned m_nEventIn, m_nEventOut;
};

#endif
