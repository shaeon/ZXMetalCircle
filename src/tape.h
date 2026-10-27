//
// tape.h - Cinta virtual (TAP y TZX) con reproducción por pulsos y acceso
//          directo a los bloques estándar para la carga instantánea
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_tape_h
#define _zx_tape_h

#include <circle/types.h>

#define TAPE_MAX_SIZE	(4 * 1024 * 1024)
#define TAPE_MAX_BLOCKS	4096

class CTape
{
public:
	CTape (void);
	~CTape (void);

	// Toman una copia de la imagen. Devuelven FALSE si el formato no es válido.
	boolean InsertTAP (const u8 *pData, unsigned nSize, const char *pName);
	boolean InsertTZX (const u8 *pData, unsigned nSize, const char *pName);
	boolean InsertCSW (const u8 *pData, unsigned nSize, const char *pName);	// v1 y v2
	// PZX (Perfect ZX Tape, de libspectrum/pzxtools): describe la cinta pulso a
	// pulso con su nivel real, así que guarda la polaridad, que el TZX no guarda
	boolean InsertPZX (const u8 *pData, unsigned nSize, const char *pName);
	boolean IsPZX (void) const		{ return m_bPZX; }

	// Invertir la polaridad de la señal (para TZX grabados al revés, como los
	// de Forbidden Planet: su cargador mide los bits por flancos de un sentido)
	void SetInverted (boolean bInverted)	{ m_bInverted = bInverted; }
	boolean IsInverted (void) const		{ return m_bInverted; }
	void Eject (void);

	// Añade un bloque (flag + datos + paridad) al final de una cinta TAP, o crea
	// una cinta nueva si no hay ninguna (lo usa la grabación con SAVE)
	boolean AppendTAPBlock (const u8 *pBlock, unsigned nLength, const char *pName);
	boolean AppendTZXBlock (const u8 *pBlock, unsigned nLength, const char *pName);	// bloque 0x10
	boolean IsInserted (void) const		{ return m_nBlocks > 0; }
	boolean IsTZX (void) const		{ return m_bTZX; }
	const char *GetName (void) const	{ return m_Name; }

	void Rewind (void);

	// Navegador de bloques (F3): qué hay en cada bloque y colocar la cinta
	enum TBlockKind
	{
		BlockProgram, BlockNumArray, BlockCharArray, BlockBytes,	// cabeceras de la ROM
		BlockData,		// bloque estándar sin cabecera (flag en nFlag)
		BlockTurbo, BlockTone, BlockPulses, BlockPureData, BlockDirect, BlockCSW, BlockGDB,
		BlockPause, BlockStop, BlockGroup, BlockGroupEnd, BlockJump, BlockLoop, BlockLoopEnd,
		BlockCall, BlockReturn, BlockSelect, BlockStop48K, BlockLevel, BlockText,
		BlockArchive, BlockHardware, BlockCustom, BlockGlue, BlockUnknown
	};
	struct TBlockInfo
	{
		TBlockKind Kind;
		unsigned nLength;	// bytes de datos (o pulsos, ms de pausa, vueltas...)
		unsigned nStart;	// cabecera "Bytes": dirección
		u8 nFlag;		// bloque de datos: byte de flag
		int nJump;		// salto relativo (bloque 0x23)
		char Text[48];		// nombre de la cabecera, texto, grupo, título...
	};
	boolean GetBlockInfo (unsigned nBlock, TBlockInfo *pInfo) const;
	void SeekBlock (unsigned nBlock);	// para la cinta al principio de ese bloque

	// Bloque 0x33 del TZX (información de hardware): qué dice la cinta de la
	// máquina nId (tipo 0 = ordenadores: 0x00 16K, 0x01 48K, 0x03 128K, 0x04
	// +2, 0x05 +2A/+3). -1 si no dice nada; si no, 0 = funciona, 1 = usa sus
	// funciones especiales, 2 = funciona pero no las usa, 3 = NO funciona.
	int GetHardwareInfo (u8 nType, u8 nId) const;
	boolean IsAtEnd (void) const		{ return m_nBlock >= m_nBlocks; }
	unsigned GetBlock (void) const		{ return m_nBlock; }
	unsigned GetBlockCount (void) const	{ return m_nBlocks; }
	unsigned GetUnsupportedBlocks (void) const	{ return m_nUnsupported; }

	// Bloque 0x28 (selección): la cinta se para y espera a que se elija una
	// de las opciones. GetSelectItem devuelve el texto de cada una.
	boolean IsSelectPending (void) const	{ return m_bSelectPending; }
	unsigned GetSelectCount (void) const;
	boolean GetSelectItem (unsigned nItem, char *pText, unsigned nSize) const;
	void Select (unsigned nItem);		// salta al bloque elegido (luego Play)
	void CancelSelect (void);		// sigue con el bloque siguiente

	// Reloj del modelo emulado (las duraciones de la cinta son a 3,5 MHz)
	void SetMachine (unsigned nClockFreq, boolean b48K);

	// Reproducción por pulsos. nNow = t-states absolutos.
	void Play (u64 nNow);			// (re)empieza el bloque actual
	void Stop (void);
	boolean IsPlaying (void) const		{ return m_bPlaying; }
	boolean GetEarBit (u64 nNow);		// avanza la cinta hasta nNow

	// Aceleración de cargadores (como en Fuse): qué es el pulso que está
	// sonando ahora (el que empezó en el último flanco) y adelantar su final.
	enum TPulseClass { PulseUnknown, PulseShort, PulseLong };	// bit 0 / bit 1 de datos
	TPulseClass GetPulseClass (void) const	{ return m_PulseClass; }
	// TRUE si desde la última aceleración ha habido un flanco "natural" (a su
	// hora): el cargador aún no lo ha visto y no se puede adelantar el siguiente
	boolean HadNaturalEdge (void) const	{ return m_bNaturalEdge; }
	void ClearNaturalEdge (void)		{ m_bNaturalEdge = FALSE; }
	void ForceEdge (u64 nNow);		// el pulso actual termina ya (nNow)
	// TRUE (una vez) si la cinta se ha parado sola en un bloque de parada (0x20
	// con pausa 0, o 0x2A en un 48K): no es una parada manual, así que el
	// cargador puede volver a arrancarla (como en Fuse)
	boolean TakeStoppedByBlock (void)
		{ boolean b = m_bStoppedByBlock; m_bStoppedByBlock = FALSE; return b; }
	u32 GetEdgeCount (void) const		{ return m_nEdges; }	// flancos desde el principio

	// Carga instantánea: siguiente bloque de datos estándar (TAP o TZX 0x10).
	// Salta los bloques informativos. Devuelve 0 si el siguiente bloque con
	// señal no es estándar (turbo, tonos...) o si se acabó la cinta.
	const u8 *GetStandardBlock (unsigned *pLength);
	void NextBlock (void);

private:
	boolean IndexTAP (void);
	boolean IndexTZX (void);
	unsigned TZXBlockSize (unsigned nOffset) const;
	boolean PrepareBlock (void);		// configura el estado para el bloque actual
	boolean PrepareCSW (const u8 *p);
	void JumpRelative (unsigned nFrom, int nRel);
	unsigned GetSelectCountAt (const u8 *p) const;
	boolean PrepareGDB (const u8 *p);
	void StartSymbol (const u8 *pSymDef, unsigned nMaxPulses);
	unsigned SymbolPulse (void);		// 0 = fin del símbolo
	void FreeCSWBuffer (void);
	unsigned Step (void);			// procesa un evento; devuelve la duración del siguiente

	u16 Word (const u8 *p) const		{ return p[0] | (p[1] << 8); }
	u32 Word24 (const u8 *p) const		{ return p[0] | (p[1] << 8) | (p[2] << 16); }
	u32 DWord (const u8 *p) const		{ return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32) p[3] << 24); }

private:
	u8 *m_pData;
	unsigned m_nSize;
	boolean m_bTZX;
	boolean m_bPZX;
	boolean m_bInverted;
	unsigned m_BlockOffset[TAPE_MAX_BLOCKS];
	unsigned m_nBlocks;
	unsigned m_nBlock;
	unsigned m_nUnsupported;
	char m_Name[64];

	unsigned m_nClockFreq;
	boolean m_b48K;

	enum TState
	{
		StateNextBlock,
		StatePilot,
		StateSync1,
		StateSync2,
		StateDataStart,
		StateData,
		StatePulseSeq,
		StateDirect,
		StateCSW,		// bloque 0x18
		StateGDBPilot,		// bloque 0x19, tono guía/sincronismo
		StateGDBData,		// bloque 0x19, datos
		StatePause,
		StatePauseLow,
		StatePZXPulses,		// bloque PULS
		StatePZXData,		// bloque DATA
		StatePZXTail,		// pulso final de un bloque DATA
		StateStop,
		StateEnd
	};

	TState m_State;
	// PZX
	boolean IndexPZX (void);
	boolean PreparePZX (const u8 *p);
	const u8 *m_pPZX;		// PULS: siguiente palabra; DATA: bits
	const u8 *m_pPZXEnd;
	boolean m_bPZXLevel;		// nivel del pulso siguiente
	u32 m_nPZXCount;		// PULS: pulsos que quedan con la misma duración
	u32 m_nPZXDur;
	u32 m_nPZXBits, m_nPZXBit;	// DATA
	unsigned m_nPZXSym;		// pulso dentro del bit
	u16 m_nPZXTail;
	const u8 *m_pPZXSym[2];	// duraciones de los pulsos del bit 0 y del bit 1
	u8 m_nPZXSymLen[2];
	boolean m_bPlaying;
	boolean m_bEar;
	u64 m_nNextEdge;
	TPulseClass m_PulseClass;		// pulso en curso (aceleración)
	u32 m_nEdges;
	boolean m_bNaturalEdge;
	boolean m_bStoppedByBlock;
	boolean m_bForceLow;			// el próximo pulso empieza en nivel bajo
	void Edge (void);

	// parámetros del bloque en curso
	unsigned m_nPilotLen, m_nPilotPulses;
	unsigned m_nSync1, m_nSync2;
	unsigned m_nZeroLen, m_nOneLen;
	unsigned m_nUsedBits;		// bits usados del último byte
	unsigned m_nPauseMs;
	const u8 *m_pBlockData;
	unsigned m_nDataLen;
	unsigned m_nSampleLen;		// bloque 0x15

	// posición
	unsigned m_nCount;
	unsigned m_nByte;
	u8 m_nBit;
	unsigned m_nHalf;
	const u8 *m_pPulses;

	// bloque 0x18 (CSW)
	const u8 *m_pCSW;
	unsigned m_nCSWLen;
	unsigned m_nCSWPos;
	unsigned m_nCSWRate;
	u64 m_nCSWFrac;
	u8 *m_pCSWBuffer;		// datos descomprimidos (Z-RLE)

	// bloque 0x19 (datos generalizados)
	const u8 *m_pSymDefPilot, *m_pPRLE, *m_pSymDefData, *m_pGDBData;
	unsigned m_nTOTP, m_nNPP, m_nTOTD, m_nNPD, m_nASD, m_nNB;
	unsigned m_nPRLEIndex, m_nRepeat, m_nPRLESymbol, m_nSymIndex;
	const u8 *m_pSym;		// símbolo en curso (0 = ninguno)
	unsigned m_nSymPulse, m_nSymMax;

	// bucles TZX (0x24/0x25)
	unsigned m_nLoopStart;
	unsigned m_nLoopCount;

	// 0x26/0x27: llamada a secuencias de bloques
	unsigned m_nCallBlock;		// bloque 0x26 en curso
	unsigned m_nCallIndex;		// secuencia que se está reproduciendo
	boolean m_bInCall;

	// 0x28: selección
	boolean m_bSelectPending;
	unsigned m_nSelectBlock;
};

#endif
