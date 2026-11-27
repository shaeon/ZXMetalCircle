//
// spectrum.h - Máquina ZX Spectrum 48K/128K/+2A
//
// Lógica de la ULA, contención, bus flotante y paginación portada a C++
// desde JSpeccy (machine/Spectrum.java, machine/Memory.java)
// (c) José Luis Sánchez, GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_spectrum_h
#define _zx_spectrum_h

#include "machine.h"
#include "machinebase.h"
#include "ula.h"
#include "zxmemory.h"
#include "pagers.h"
#include "divmmc.h"
#include "palette.h"
#include "ay8912.h"
#include "audiomixer.h"
#include "tape.h"
#include "tapedeck.h"
#include "interface1.h"
#include "plus3disk.h"
#include "sdcard.h"
#include "earinput.h"
#include "spectranet.h"
#include "zxkeyboard.h"
#include "z80.h"
#include "z80operations.h"
#include <circle/types.h>

class CSnapshot;

// final: nadie hereda de CSpectrum, así que sus propias llamadas a lo de
// CMachine (RunFrame, WriteMemory...) siguen siendo directas, sin tabla virtual
class CSpectrum final : public Z80operations, public CMachine
{
	friend class CSnapshot;		// carga y grabación de snapshots

public:
	CSpectrum (CZXKeyboard *pKeyboard, CTape *pTape);
	~CSpectrum (void);

	void SetModel (TModel Model);		// incluye reset
	TModel GetModel (void) const		{ return m_Model; }
	const TMachineTiming *GetTiming (void) const	{ return m_pTiming; }

	// CMachine: nombre, tiempos y carga acelerada
	const char *GetName (void) const override		{ return m_pTiming->pName; }
	const char *GetShortName (void) const override		{ return m_pTiming->pShortName; }
	unsigned GetClockFreq (void) const override		{ return m_pTiming->nClockFreq; }
	unsigned GetFrameTstates (void) const override		{ return m_pTiming->nTstatesFrame; }
	boolean IsAccelerating (void) const override
		{ return IsTapeAccelerating () || IsDiskAccelerating (); }
	const TFileType *GetFileTypes (void) const override;
	CMachineInput *GetInput (void) override			{ return m_pKeyboard; }
	// Estado: .Z80, .SNA y .SZX (y se cargan .SP); en snapshot.cpp
	unsigned GetStateFormats (void) const override			{ return 3; }
	const char *GetStateExt (unsigned nFormat) const override;
	const char *LoadState (const u8 *pData, unsigned nSize, const char *pFileName) override;
	const char *CanSaveState (void) const override;
	unsigned SaveState (unsigned nFormat, u8 *pBuffer, unsigned nBufferSize) override;

	void Reset (void) override;
	// La NMI se lanza a mitad del próximo cuadro, lejos de la interrupción
	// del comienzo del cuadro: con la divMMC, una NMI que llega mientras
	// esxDOS atiende esa interrupción se ignora (como en la máquina real si el
	// botón se pulsa en ese instante)
	void TriggerNMI (void)			{ m_bNMIPending = TRUE; }

	// Ejecuta un cuadro completo (50 Hz) y genera imagen y sonido
	void RunFrame (void) override;

	// Imagen: SCREEN_WIDTH x SCREEN_HEIGHT píxeles RGB565
	TPixel *GetScreenBuffer (void) override	{ return m_ULA.GetScreen (); }
	CAudioMixer *GetAudio (void) override	{ return &m_Audio; }
	CAY8912 *GetAY (void)			{ return &m_AY; }

	// Casete (tapedeck.h)
	void SetTapeMode (TTapeMode Mode)	{ m_Deck.SetMode (Mode); }
	TTapeMode GetTapeMode (void) const	{ return m_Deck.GetMode (); }
	void TapePlay (void)			{ m_Deck.Play (GetAbsoluteTstates ()); }
	void TapeStop (void)			{ m_Deck.Stop (); }
	void TapeInserted (void)		{ m_Deck.Inserted (); }	// tras insertar o rebobinar: arma el autoarranque
	// TRUE si se está cargando una cinta en modo acelerado (no sincronizar)
	boolean IsTapeAccelerating (void) const	{ return m_Deck.IsAccelerating (); }
	// Aceleración de cargadores como en Fuse (solo en carga rápida e instantánea)
	void SetLoaderAcceleration (boolean bOn)	{ m_Deck.SetLoaderAcceleration (bOn); }
	boolean GetLoaderAcceleration (void) const	{ return m_Deck.GetLoaderAcceleration (); }
	// diskaccel=1: emulación acelerada mientras se usa la disquetera
	void SetDiskAcceleration (boolean bOn)	{ m_bDiskAccel = bOn; }
	boolean GetDiskAcceleration (void) const	{ return m_bDiskAccel; }
	boolean IsDiskAccelerating (void) const;
	// TK90X con el diodo D1 de los vendidos en Brasil: bit 7 del puerto 254 a 0
	void SetTKDiode (boolean bOn)		{ m_bTKDiode = bOn; }

	// TK90X/TK95: ULA Ferranti (tiempos del 48K) o de Microdigital a 50/60 Hz;
	// se aplica al elegir el modelo (SetModel)
	void SetTKULA (TTKULA ULA)		{ m_nTKULA = ULA; }
	TTKULA GetTKULA (void) const		{ return m_nTKULA; }
	boolean IsTKMicrodigital (void) const	{ return m_bTKMicro; }

	// Multiface (MF1 en 48K, MF128 en 128K, MF3 en +2A). Las ROMs (8 KB) se
	// cargan desde la SD; sin ROM no hay Multiface para ese modelo.
	void SetMultifaceROM (TCodeModel CodeModel, const u8 *pROM)	{ m_MF.SetROM (CodeModel, pROM); }
	void SetMultifaceEnabled (boolean bEnabled)	{ m_MF.SetEnabled (bEnabled); }
	boolean IsMultifaceAvailable (void) const;
	boolean IsMultifacePaged (void) const		{ return m_MF.IsPaged (); }

	// Interface 2: cartucho ROM de 16 KB que sustituye a la ROM del Spectrum
	// (16K/48K, 128K y +2; el +2A no lo admite). Incluye reset.
	boolean InsertCartridge (const u8 *pROM, unsigned nSize);
	void EjectCartridge (void);
	boolean IsCartridgeInserted (void) const	{ return m_IF2.IsInserted (); }

	// Disquetera del +3 (solo con el modelo +3)
	CPlus3Disk *GetDisk (void)		{ return m_pDisk; }
	boolean HasDiskDrive (void) const	{ return ModelHas (m_Model, MODEL_DISK); }

	// divMMC con esxDOS (ver divmmc.h). La tarjeta es la misma que la de la
	// ZXMMC. En el +3e solo se ponen los puertos (sin ROM ni paginación), como
	// en el ZX-Uno, para los programas que hablan directamente con la tarjeta.
	void SetDivMMCROM (const u8 *pROM)	{ m_Div.SetROM (pROM); UpdateDivActive (); }
	void SetDivMMCEnabled (boolean bOn)	{ m_Div.SetEnabled (bOn); UpdateDivActive (); }
	boolean IsDivMMCActive (void) const	{ return m_Div.IsEnabled () && m_Div.HasROM () && !HasZXMMC (); }
	boolean IsDivMMCEnabled (void) const	{ return m_Div.IsEnabled (); }	// (en el +3e no se usa)
	boolean HasDivMMCPorts (void) const	{ return IsDivMMCActive () || HasZXMMC (); }

	// Reloj para esxDOS (RTC.SYS de ZXMetalCircle): ver divmmc.h
	void SetRTC (u16 nDate, u16 nTime)	{ m_Div.SetRTC (nDate, nTime); }

	// Spectranet (tarjeta de red): ver spectranet.h
	CSpectranet *GetSpectranet (void)	{ return &m_Snet; }
	void SetSpectranetEnabled (boolean bOn);
	boolean IsSpectranetEnabled (void) const	{ return m_Snet.IsEnabled (); }

	// ZXMMC (tarjeta SD del +3e): puertos 0x1F (CS) y 0x3F (datos SPI)
	CSDCard *GetSDCard (void)		{ return &m_SDCard; }
	boolean HasZXMMC (void) const		{ return ModelHas (m_Model, MODEL_ZXMMC); }

	// ROMs cargadas de la SD (16 KB por banco: 1 en 16K/48K, 2 en 128K/+2,
	// 4 en +2A/+3/+3e). Solo el 48K y el 128K tienen ROM incluida; un modelo
	// sin ROM no está disponible y SetModel () usa el 128K en su lugar.
	void SetModelROM (TModel Model, const u8 *pROM);
	boolean IsModelAvailable (TModel Model) const;
	TModel TakeFallback (void)	{ TModel M = m_FallbackFrom; m_FallbackFrom = ModelCount; return M; }

	// ROM del Interface 1 (8 KB): sin ella no se puede conectar
	void SetIF1ROM (const u8 *pROM)		{ m_IF1ROM.SetROM (pROM); }
	CInterface1 *GetIF1 (void)		{ return &m_IF1; }
	boolean HasIF1ROM (void) const	{ return m_IF1ROM.HasROM (); }

	// Interface 1 con microdrives (no en el +2A)
	void SetIF1Connected (boolean bConnected);
	boolean IsIF1Connected (void) const	{ return m_IF1ROM.IsConnected (); }
	CInterface1 *GetInterface1 (void)	{ return &m_IF1; }
	// Lectura de la memoria tal como la ve ahora la CPU (sin contención; pruebas)
	u8 ReadMemory (u16 nAddress) const override	{ return m_Mem.ReadPage (nAddress >> 14)[nAddress & 0x3FFF]; }
	// Escritura en la memoria tal como la ve ahora la CPU (la ROM no se escribe)
	void WriteMemory (u16 nAddress, u8 nValue) override	{ m_Mem.WritePage (nAddress >> 14)[nAddress & 0x3FFF] = nValue; }
	// Una página de RAM (0-7) directamente, esté paginada o no (pokes del 128K)
	u8 ReadRAM (unsigned nBank, u16 nOffset) const	{ return m_Mem.Bank (nBank & 7)[nOffset & 0x3FFF]; }
	void WriteRAM (unsigned nBank, u16 nOffset, u8 nValue)	{ m_Mem.Bank (nBank & 7)[nOffset & 0x3FFF] = nValue; }
	// Los reset que lleva: con uno, los pokes aplicados se pierden
	unsigned GetResetCount (void) const override	{ return m_nResetCount; }

	// Ampliación de memoria LEC: 16 páginas de 32 KB que sustituyen a los 32 KB
	// bajos (0x0000-0x7FFF). Necesaria para el CP/M de LEC. En los 128K (128,
	// +2, +2A, +3, +3e) funciona en modo 48K: con la paginación bloqueada y la
	// ROM del BASIC de 48K (opción "48 BASIC" o USR 0).
	boolean SetLECConnected (boolean bConnected);
	boolean IsLECConnected (void) const	{ return m_LEC.IsConnected (); }
	boolean IsLECPaged (void) const		{ return m_LEC.IsPaged (); }
	// La LEC se ha paginado desde el último reset (el CP/M está en marcha; en
	// el prompt la LEC puede estar despaginada mientras el BIOS lee el teclado)
	boolean IsLECInUse (void) const		{ return m_LEC.IsUsed (); }

	// Teclado Issue 2 (16K/48K antiguos): el bit EAR leído depende también del bit MIC
	void SetIssue2 (boolean bIssue2)	{ m_bIssue2 = bIssue2; }
	boolean IsIssue2 (void) const		{ return m_bIssue2; }

	// ULAplus (64 colores, puertos 0xBF3B/0xFF3B)
	void SetULAplusEnabled (boolean bEnabled)	{ m_ULA.SetULAplusEnabled (bEnabled); }
	// Efecto nieve del 16K, 48K, 128K y +2 (snow=1): si I apunta a memoria
	// lenta, el refresco de la memoria choca con la lectura de la pantalla de
	// la ULA
	void SetSnowEnabled (boolean bEnabled)	{ m_bSnowEnabled = bEnabled; UpdateSnow (); }
	// negro puro (#000000) también en la paleta del 16K/48K (pureblack=1)
	void SetPureBlack (boolean bPureBlack)	{ m_ULA.SetPureBlack (bPureBlack); }
	boolean IsPureBlack (void) const	{ return m_ULA.IsPureBlack (); }
	TPixel GetStdColor (unsigned nIndex) const	{ return m_ULA.GetStdColor (nIndex); }
	boolean IsULAplusActive (void) const		{ return m_ULA.IsULAplusActive (); }
	// el programa usa la ULAplus desde el último reset: modo de 64 colores
	// activo o una paleta cargada (aunque ahora tenga el modo apagado)
	boolean IsULAplusInUse (void) const	{ return m_ULA.IsULAplusInUse (); }

	// Grabación por pulsos de la salida MIC (cualquier rutina de grabación,
	// incluidas las turbo). Durante la grabación no se usa el trap de SAVE.
	// Formato de los datos: RLE de CSW, 1 muestra = 16 t-states.
	// Entrada EAR externa (casete real por GPIO o tarjeta de sonido USB)
	void SetEarInput (CEarInput *pEarInput)	{ m_Deck.SetEarInput (pEarInput); m_Audio.SetEarInput (pEarInput); }
	boolean IsEarInputActive (void) const	{ return m_Deck.IsEarInputActive (); }

	// bEarInput: graba la entrada EAR externa en lugar de la salida MIC
	boolean StartRecording (boolean bEarInput = FALSE)
		{ return m_Deck.StartRecording (bEarInput, GetAbsoluteTstates ()); }
	boolean IsRecordingEarInput (void) const	{ return m_Deck.IsRecordingEarInput (); }
	void StopRecording (void)		{ m_Deck.StopRecording (GetAbsoluteTstates ()); }
	boolean IsRecording (void) const	{ return m_Deck.IsRecording (); }
	// muestras de 16 t-states del reloj del modelo (tiempo real, como una cinta)
	const u8 *GetRecording (unsigned *pLength, unsigned *pPulses, unsigned *pSampleRate) const
		{ return m_Deck.GetRecording (pLength, pPulses, pSampleRate, m_pTiming->nClockFreq); }
	void FreeRecording (void)		{ m_Deck.FreeRecording (); }

	// Grabación con SAVE (0 = desactivada: la ROM graba "al aire")
	void SetSaveHandler (CTapeSaveHandler *pHandler)	{ m_Deck.SetSaveHandler (pHandler); }

	// Juego de caracteres de la ROM del 48K (96 caracteres x 8 bytes)
	const u8 *GetROMFont (void) const;

public:
	// Z80operations
	uint8_t fetchOpcode (uint16_t address) override;
	uint8_t peek8 (uint16_t address) override;
	void poke8 (uint16_t address, uint8_t value) override;
	uint16_t peek16 (uint16_t address) override;
	void poke16 (uint16_t address, RegisterPair word) override;
	uint8_t inPort (uint16_t port) override;
	void outPort (uint16_t port, uint8_t value) override;
	void addressOnBus (uint16_t address, int32_t wstates) override;
	void interruptHandlingTime (int32_t wstates) override;
	bool isActiveINT (void) override;
	uint8_t breakpoint (uint16_t address, uint8_t opcode) override;

private:
	void UpdatePaging (void);
	void Snow (const u8 *pPage)	// en cada M1, si I apunta a memoria lenta
		{ m_ULA.Snow (pPage, m_nTstates, m_CPU.getRegR ()); }
	void UpdateSnow (void)
	{
		m_bSnow = m_bSnowEnabled && (m_Model == Model48K || m_Model == Model16K);
		m_bSnow128 = m_bSnowEnabled && (m_Model == Model128K || m_Model == ModelPlus2);
	}

	void PreIO (uint16_t nPort);
	void PostIO (uint16_t nPort);

	inline void Contend (uint16_t nAddress, int nTstates);
	inline void WriteByte (uint16_t nAddress, uint8_t nValue);
	u8 Plus3FloatingBus (void);
	unsigned TKIOContention (u16 nPort) const;
	void NoteBus (u8 nValue)	{ m_nBusTstates = GetAbsoluteTstates (); m_nBusValue = nValue; m_bBusValid = TRUE; }
	inline boolean IsScreenWrite (uint16_t nAddress) const;

	void PageMultiface (void);
	void UnpageMultiface (void);
	u64 GetAbsoluteTstates (void) const	{ return m_nFrameStart + (u64) m_nTstates; }
	static void TapeWriteByte (void *pParam, u16 nAddress, u8 nValue);

private:
	CZXKeyboard *m_pKeyboard;
	CTape *m_pTape;
	Z80 m_CPU;
	CAY8912 m_AY;
	CAudioMixer m_Audio;
	CULA m_ULA;			// imagen, borde, colores y contención
	CMemory m_Mem;			// RAM, ROMs y paginación
	CInterface2 m_IF2;		// periféricos que tapan la memoria (después de m_Mem)
	CLEC m_LEC;
	CMultiface m_MF;

	TModel m_Model;
	const TMachineTiming *m_pTiming;

	int m_nTstates;			// t-states del cuadro actual
	u64 m_nFrameStart;		// t-states absolutos al comienzo del cuadro
	boolean m_bPlus3Bus;		// +2A/+3: se apuntan los accesos a 0x4000-0x5FFF
	boolean m_bBusValid;
	u64 m_nBusTstates;		// último acceso de la CPU a 0x4000-0x5FFF
	u8 m_nBusValue;

	u8 m_nPortFE;
	unsigned m_nResetCount;
	boolean m_bEarOut;
	boolean m_bIssue2;

	// efecto nieve (lo pinta la ULA; aquí, si el modelo lo tiene)
	boolean m_bSnowEnabled;		// snow=1
	boolean m_bSnow;		// y el modelo la tiene: 16K y 48K...
	boolean m_bSnow128;		// ... o 128K y +2 (no los +2A/+3 ni los clónicos)

	// Disquetera del +3
	CPlus3Disk *m_pDisk;

	void UpdateDivActive (void)	{ m_Div.SetActive (IsDivMMCActive ()); }

	boolean m_bNMIPending;

	// Spectranet
	CSpectranet m_Snet;

	// tarjeta SD, con sus dos interfaces
	CSDCard m_SDCard;
	CDivMMC m_Div;
	CZXMMC m_ZXMMC;

	// casete (después de m_CPU, m_Mem y m_Div)
	CTapeDeck m_Deck;

	const u8 *m_pModelROM[ModelCount];
	TModel m_FallbackFrom;		// modelo pedido sin ROM (ModelCount = ninguno)

	// Interface 1
	boolean IF1Active (void) const
		{ return m_IF1ROM.IsConnected () && m_pTiming->CodeModel != CodeModelPlus3; }
	CInterface1 m_IF1;		// microdrives, red y RS232
	CInterface1ROM m_IF1ROM;	// su ROM y cuándo se ve (pagers.h)

	// disco acelerado y TK
	boolean m_bDiskAccel;
	boolean m_bTKDiode;
	TTKULA m_nTKULA;
	boolean m_bTKMicro;		// TK con ULA de Microdigital (tiempos propios)
};

#endif
