//
// pokfile.h
//
// Pokes de los ficheros .POK (los de ZXDB y The Tipshop) y los escritos a
// mano. Sin nada de la Pi, para poder probarlo en el PC.
//
// Un .POK es texto, una línea por cosa:
//
//	N<nombre del truco>
//	M <banco> <dirección> <valor> <original>	(un poke; siguen más)
//	Z <banco> <dirección> <valor> <original>	(el último del truco)
//	Y						(fin del fichero)
//
// Banco: 8 (o cualquiera con el bit 3) = la memoria tal como está paginada;
// 0-7 = esa página de RAM del 128K (la dirección se toma dentro de la página,
// & 0x3FFF, aunque venga entera: 49152...). En los modelos de 48K el banco no
// cuenta. Valor 256: se pregunta al usuario (uno para todo el truco).
// Original: el valor de antes, para quitar el truco (0 = no se sabe: se lee de
// la memoria al aplicarlo).
//
#ifndef _zx_pokfile_h
#define _zx_pokfile_h

#include <circle/types.h>

#define POK_MAX_TRAINERS	160	// el .POK más grande de ZXDB tiene 114
#define POK_MAX_POKES		4096	// y 487 pokes
#define POK_NAME_LEN		100
#define POK_MAX_MANUAL		32	// POKE escritos a mano
#define POK_BANK_CURRENT	8	// la memoria tal como está paginada
#define POK_ASK			256	// valor que se pregunta

// La memoria del Spectrum, para aplicar y quitar los pokes. nBank: 0-7 (página
// de RAM del 128K: nAddress & 0x3FFF dentro de ella; en los 48K, que no tienen
// páginas, nAddress tal cual) o POK_BANK_CURRENT (nAddress tal cual,
// 16384-65535). nAddress siempre viene entera, como en el .POK.
class CPokMemory
{
public:
	virtual ~CPokMemory (void) {}
	virtual u8 Read (unsigned nBank, u16 nAddress) = 0;
	virtual void Write (unsigned nBank, u16 nAddress, u8 nValue) = 0;
};

class CPokFile
{
public:
	CPokFile (void);

	void Clear (void);			// sin trucos (otro juego)

	// Lee un .POK y sustituye los trucos del fichero (los escritos a mano se
	// quedan). FALSE si no es un .POK válido (y entonces no cambia nada).
	boolean Parse (const char *pText, unsigned nLength);

	// Los del fichero y después los escritos a mano
	unsigned GetCount (void) const		{ return m_nTrainers + m_nManual; }
	unsigned GetFileCount (void) const	{ return m_nTrainers; }
	const char *GetName (unsigned nTrainer) const;
	boolean IsActive (unsigned nTrainer) const;
	boolean IsManual (unsigned nTrainer) const;	// POKE escrito a mano
	boolean NeedsInput (unsigned nTrainer) const;	// algún valor 256
	unsigned GetPokeCount (unsigned nTrainer) const;

	// Aplica un truco (nInput: el valor de los 256) y guarda lo que había
	boolean Activate (unsigned nTrainer, CPokMemory *pMemory, u8 nInput = 0);
	// Lo quita: el original del .POK si lo trae, si no lo que había al aplicarlo
	void Deactivate (unsigned nTrainer, CPokMemory *pMemory);
	// Todos sin aplicar, sin tocar la memoria (después de un reset)
	void Forget (void);

	// Un POKE escrito a mano (16384-65535), como un truco más: "POKE 35136,0".
	// Devuelve su número o -1 si no cabe. Se quedan aunque se lea otro .POK;
	// Clear los quita.
	int AddManual (u16 nAddress, u8 nValue);

	// "35136 0", "8940H 0", "35136 0 35137 201": pares dirección valor, en
	// decimal o en hexadecimal con H al final. Devuelve cuántos pares (0 si hay
	// algo mal: una dirección fuera de 16384-65535, un valor de más de 255...)
	static unsigned ParseManual (const char *pText, u16 *pAddress, u8 *pValue, unsigned nMax);
	// Un valor 0-255 (decimal o con H), para los 256; FALSE si no vale
	static boolean ParseValue (const char *pText, u8 *pValue);

private:
	struct TPoke
	{
		u16 nAddress;
		u16 nValue;		// 0-255 o POK_ASK
		u8 nBank;
		u8 nOriginal;		// el del .POK (0 = no se sabe)
		u8 nSaved;		// lo que había al aplicarlo
	};

	struct TTrainer
	{
		char Name[POK_NAME_LEN];
		TPoke *pPokes;
		unsigned nCount;
		boolean bActive;
	};

	TTrainer *Trainer (unsigned nTrainer);
	const TTrainer *Trainer (unsigned nTrainer) const;
	// pStore == 0: solo comprobar y contar; si no, guardar en él
	static boolean ParseText (const char *pText, unsigned nLength, CPokFile *pStore,
				  unsigned *pTrainers, unsigned *pPokes);

	TTrainer m_Trainers[POK_MAX_TRAINERS];
	unsigned m_nTrainers;
	TPoke m_Pokes[POK_MAX_POKES];
	unsigned m_nPokes;

	TTrainer m_Manual[POK_MAX_MANUAL];	// un poke cada uno
	TPoke m_ManualPokes[POK_MAX_MANUAL];
	unsigned m_nManual;
};

#endif
