//
// hostinput.h - La entrada del PC, para cualquier máquina
//
// Fase 3d de la capa de máquinas: lo que llega de los teclados USB se
// reparte en dos. CHostInput es de cualquier máquina: junta los teclados que
// haya (cada informe es el estado de SU teclado), aplica la norma HID con
// los informes de error, saca las pulsaciones que son del emulador (teclas
// de función, Pausa, atajos con Alt y, con un menú abierto, todas) a una cola
// y sabe si los menús tienen el teclado (capturado). Lo demás se lo pasa a la
// máquina a través de CMachineInput: en el Spectrum, CZXKeyboard, que lo
// convierte en su matriz de 8x5 y en joysticks.
//
// UpdateFromReport, UpdateFromKeyboard y ForgetKeyboard se llaman desde el
// manejador USB (contexto de interrupción), como antes.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_hostinput_h
#define _zx_hostinput_h

#include <circle/types.h>

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
#define HID_MOD_LGUI		0x08	// tecla Windows izquierda
#define HID_MOD_RGUI		0x80	// tecla Windows derecha

struct TKeyEvent
{
	u8 nKey;		// código HID
	u8 nModifiers;
};

// La parte de la entrada que es de la máquina (en el Spectrum, CZXKeyboard)
class CMachineInput
{
public:
	virtual ~CMachineInput (void) {}

	// Las teclas del PC que tiene pulsadas el jugador (informe HID ya
	// juntado y sin errores), cuando no son del emulador
	virtual void SetPCKeys (u8 nModifiers, const u8 RawKeys[6]) = 0;
	// Con Alt, las teclas son atajos del emulador: las del PC se sueltan
	virtual void ReleasePCKeys (void) = 0;
	// Un menú abre o cierra (capturado: nada llega a la máquina)
	virtual void SetCaptured (boolean bCaptured) = 0;
	// Se suelta todo (al capturar, y cuando lo pide el kernel)
	virtual void ReleaseAll (void) = 0;
};

class CHostInput
{
public:
	CHostInput (void);

	void SetMachineInput (CMachineInput *pMachine);

	// Llamado desde el manejador USB (contexto de interrupción)
	void UpdateFromReport (u8 nModifiers, const u8 RawKeys[6]);

	// Varios teclados USB a la vez (p. ej. el de la Pi 400 y otro): cada
	// informe es el estado de SU teclado, y se juntan con los de los demás.
	// Con UpdateFromReport, el informe de un teclado (aunque sea el que repite
	// cada poco sin teclas) soltaba las que se tenían pulsadas en el otro.
	void UpdateFromKeyboard (unsigned nKeyboard, u8 nModifiers, const u8 RawKeys[6]);
	void ForgetKeyboard (unsigned nKeyboard);	// desenchufado: sus teclas se sueltan

	// Si está activo, las teclas no llegan a la máquina (menús del emulador)
	void SetCaptured (boolean bCaptured);
	boolean IsCaptured (void) const		{ return m_bCaptured; }

	// Cola de pulsaciones (flancos de bajada) para el emulador
	boolean GetKeyEvent (TKeyEvent *pEvent);
	// Eventos generados por otros teclados (GPIO en modo extendido o en menús)
	// y por los mandos
	void InjectKeyEvent (u8 nKey, u8 nModifiers)	{ PushEvent (nKey, nModifiers); }

	// Tecla de navegación (cursores, RePág, AvPág) que sigue pulsada en el
	// teclado USB, o 0 (para la repetición en los menús)
	u8 GetHeldNavKey (void) const;

private:
	void PushEvent (u8 nKey, u8 nModifiers);
	void UpdateMerged (void);

	CMachineInput *m_pMachine;
	volatile boolean m_bCaptured;

	u8 m_PrevKeys[6];

	#define HOSTINPUT_KEYBOARDS	4
	u8 m_KbdModifiers[HOSTINPUT_KEYBOARDS];
	u8 m_KbdKeys[HOSTINPUT_KEYBOARDS][6];

	TKeyEvent m_Events[32];
	volatile unsigned m_nEventIn, m_nEventOut;
};

#endif
