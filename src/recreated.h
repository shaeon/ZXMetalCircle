//
// recreated.h - Teclado Recreated ZX Spectrum (Elite Systems) en "Layer A" (capa de juego)
//
// En la capa de juego cada tecla del Spectrum se transmite como un carácter al
// pulsarla y otro distinto al soltarla (por ejemplo, el 1 envía 'a' al pulsarse y
// 'b' al soltarse). Así el teclado puede informar de cualquier combinación de las
// 40 teclas, igual que la membrana original.
//
// Referencia: "Recreated ZX Keyboard Technical Document" (Elite Systems, 13/05/2015)
//
// Como no se conoce un identificador USB fiable del teclado, se detecta por su
// forma de transmitir: pulsaciones de una sola tecla de la tabla que se sueltan
// casi inmediatamente, varias veces seguidas. Un teclado normal mantiene la tecla
// pulsada mucho más tiempo.
//
// Mientras no se sabe de qué tipo es el teclado, cada pulsación candidata se
// retiene hasta 30 ms: si se suelta enseguida (patrón Recreated) no llega al
// Spectrum, y así no aparecen caracteres sueltos antes de la detección. Tras 8
// pulsaciones normales el teclado se da por normal y deja de retenerse nada.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_recreated_h
#define _zx_recreated_h

#include <circle/types.h>

enum TRecreatedMode
{
	RecreatedOff,		// nunca
	RecreatedAuto,		// detección automática
	RecreatedForced		// todos los teclados USB son Recreated
};

class CRecreatedKeyboard
{
public:
	CRecreatedKeyboard (void);

	void Reset (TRecreatedMode Mode);

	// Informes que hay que pasar al teclado normal
	struct TOutput
	{
		unsigned nCount;
		u8 Modifiers[2];
		u8 Keys[2][6];
	};

	// Procesa un informe USB (contexto de interrupción). Lo que deba tratarse
	// como teclado normal se devuelve en *pOut (0, 1 o 2 informes).
	// nNowUs: tiempo en microsegundos.
	void ProcessReport (u8 nModifiers, const u8 RawKeys[6], unsigned nNowUs, TOutput *pOut);

	// Llamar periódicamente (con interrupciones desactivadas): entrega una
	// pulsación retenida que ya ha superado el tiempo de espera.
	void CheckTimeout (unsigned nNowUs, TOutput *pOut);

	boolean IsActive (void) const		{ return m_bActive; }
	// Reconocido por su VID:PID (0F39:0404, TG3 Electronics): se activa ya,
	// sin esperar a la detección (salvo con el modo apagado)
	void ActivateByID (void)		{ if (m_Mode != RecreatedOff && !m_bActive) { m_bActive = TRUE; m_bJustDetected = TRUE; } }
	const u8 *GetMatrix (void) const	{ return (const u8 *) m_Matrix; }

	// TRUE una sola vez tras la detección (para avisar en pantalla)
	boolean CheckJustDetected (void);

	// Carácter US de una tecla HID (0 si no tiene)
	static char HIDToChar (u8 nKey, boolean bShift);

private:
	void Decode (u8 nModifiers, const u8 RawKeys[6]);
	void Detect (u8 nModifiers, const u8 RawKeys[6], unsigned nNowUs, TOutput *pOut);
	void Emit (TOutput *pOut, u8 nModifiers, const u8 RawKeys[6]);
	void FlushPending (TOutput *pOut, boolean bNormalHold);

private:
	TRecreatedMode m_Mode;
	volatile boolean m_bActive;
	volatile boolean m_bJustDetected;
	volatile u8 m_Matrix[8];
	u8 m_PrevKeys[6];

	// detección
	boolean m_bNormal;		// ya se sabe que es un teclado normal
	boolean m_bPending;		// pulsación retenida
	u8 m_nPendingModifiers;
	u8 m_PendingKeys[6];
	unsigned m_nDownTime;
	unsigned m_nHits;
	boolean m_TapDown[40];		// teclas con pulsación vista y aún sin soltar
	unsigned m_nNormalHolds;
	boolean m_bPrevEmpty;
};

#endif
