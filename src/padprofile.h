//
// padprofile.h
//
// Perfil de un mando: qué hace cada botón. Sin nada de la Pi, para poder
// probarlo en el PC.
//
// Un perfil es un fichero de texto (joysticks/VVVV-PPPP/<tipo>.txt), una
// línea por botón:
//
//	nombre = matamarcianos
//	A = disparo
//	B = disparo 2
//	X = tecla H
//	Y = tecla SS+P
//	LB = pc F5
//	RB = pc Alt+F1
//	Select = joystick
//	Start = menu
//
// Los botones de los mandos conocidos (PS3/PS4, Xbox, Switch Pro, 8BitDo...)
// tienen nombre (A, B, X, Y, LB, RB, LT, RT, Select, Start, L3, R3, Guia,
// Arriba, Abajo, Izquierda, Derecha, Plus, Minus, Touchpad); los de los
// genéricos, número (1 a 32). Las direcciones del stick y del "hat" de los
// genéricos no son botones: siempre mueven el joystick.
//
// Acciones: ninguno, disparo, disparo 2, arriba, abajo, izquierda, derecha,
// joystick (cambiar de joystick), menu (la ayuda, F1), tecla <K> (una tecla
// del Spectrum: A-Z, 0-9, ENTER, ESPACIO, CS, SS, con CS+ o SS+ delante) y
// pc <K> (una tecla del emulador: F1-F12, Enter, Esc, Tab, Retroceso,
// Espacio, Pausa, RePag, AvPag, Inicio, Fin, Supr, cursores, letras y
// números, con Alt+, Ctrl+ o Shift+ delante). También en inglés.
//
#ifndef _zx_padprofile_h
#define _zx_padprofile_h

#include <circle/types.h>

#define PAD_BUTTONS		32
#define PAD_PROFILE_NAME	32
#define PAD_NO_KEY		0xFF	// posición de tecla del Spectrum: ninguna

enum TPadActionType
{
	PadActNone,
	PadActJoy,		// nArg: bits del joystick (JOY_UP...)
	PadActZXKey,		// nArg: tecla (fila << 3 | bit); nArg2: CS o SS, o PAD_NO_KEY
	PadActPCKey,		// nArg: tecla HID; nArg2: modificadores HID
	PadActNextJoy,		// cambiar de joystick (Select)
	PadActMenu		// la ayuda (Start)
};

struct TPadAction
{
	u8 Type;
	u8 nArg;
	u8 nArg2;
};

// Lo que pide un mando con unos botones pulsados
struct TPadOutput
{
	u8 nJoy;			// bits del joystick
	u8 Matrix[8];			// teclas del Spectrum mantenidas (como la matriz)
	unsigned nPCKeys;		// teclas del emulador pulsadas ahora (solo al pulsar)
	u8 PCKey[PAD_BUTTONS];
	u8 PCMods[PAD_BUTTONS];
	boolean bNextJoy;		// pulsado ahora: cambiar de joystick
	boolean bMenu;			// pulsado ahora: la ayuda
};

class CPadProfile
{
public:
	CPadProfile (void);

	// El de siempre: conocidos, A/Y/LB/RB disparo, B/X/LT/RT disparo 2,
	// cruceta, Select joystick y Start menú; genéricos, 1/4/5/6 disparo,
	// 2/3/7/8 disparo 2, 9 joystick y 10 menú
	void SetDefault (boolean bKnown);

	// Lee un perfil (sobre el de siempre: lo que no diga, se queda). FALSE
	// si alguna línea no se entiende (las demás se aplican).
	boolean Parse (const char *pText, unsigned nLength, boolean bKnown);

	// El perfil como texto, para guardarlo
	void Format (char *pBuffer, unsigned nSize, boolean bKnown) const;

	const char *GetName (void) const		{ return m_Name; }
	void SetName (const char *pName);
	const TPadAction &GetAction (unsigned nButton) const	{ return m_Action[nButton % PAD_BUTTONS]; }
	void SetAction (unsigned nButton, const TPadAction &Action);

	// Con los botones de ahora y los de antes (para saber cuáles son nuevos)
	void Evaluate (u32 nButtons, u32 nPrevButtons, TPadOutput *pOut) const;

	// El nombre de un botón (bit) en el fichero; 0 si no existe
	static const char *ButtonName (unsigned nButton, boolean bKnown);
	static int ButtonFromName (const char *pName, boolean bKnown);

	// El fichero del perfil activo de un tipo de joystick (sin la carpeta):
	// "<tipo>.txt", o "<tipo>-<nombre>.txt" si activo.txt (pActive) tiene la
	// línea "<tipo> = <nombre>"
	static void ActiveFile (const char *pActive, unsigned nLength, const char *pType,
				char *pFile, unsigned nSize);

	static boolean ParseAction (const char *pText, TPadAction *pAction);
	static void FormatAction (const TPadAction &Action, char *pBuffer, unsigned nSize);

private:
	TPadAction m_Action[PAD_BUTTONS];
	char m_Name[PAD_PROFILE_NAME];
};

#endif
