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
#include "palette.h"
#include "ay8912.h"
#include "audiomixer.h"
#include "tape.h"
#include "interface1.h"
#include "plus3disk.h"
#include "sdcard.h"
#include "earinput.h"
#include "spectranet.h"
#include "zxkeyboard.h"
#include "z80.h"
#include "z80operations.h"
#include <circle/types.h>

enum TTapeMode
{
	TapeModeFlash,		// trap de la ROM: carga instantánea de bloques estándar
	TapeModeFast,		// carga por pulsos a máxima velocidad (se ve la carga)
	TapeModeNormal,		// carga por pulsos a velocidad real
	TapeModeCount
};

// Recibe los bloques que la ROM graba con SAVE (trap de SA-BYTES)
class CTapeSaveHandler
{
public:
	virtual ~CTapeSaveHandler (void) {}

	// pBlock = flag + datos + paridad. Devuelve FALSE si no se pudo grabar.
	virtual boolean SaveTapeBlock (const u8 *pBlock, unsigned nLength) = 0;
};

class CSnapshot;

class CSpectrum : public Z80operations
{
	friend class CSnapshot;		// carga y grabación de snapshots

public:
	CSpectrum (CZXKeyboard *pKeyboard, CTape *pTape);
	~CSpectrum (void);

	void SetModel (TModel Model);		// incluye reset
	TModel GetModel (void) const		{ return m_Model; }
	const TMachineTiming *GetTiming (void) const	{ return m_pTiming; }

	void Reset (void);
	// La NMI se lanza a mitad del próximo cuadro, lejos de la interrupción
	// del comienzo del cuadro: con la divMMC, una NMI que llega mientras
	// esxDOS atiende esa interrupción se ignora (como en la máquina real si el
	// botón se pulsa en ese instante)
	void TriggerNMI (void)			{ m_bNMIPending = TRUE; }

	// Ejecuta un cuadro completo (50 Hz) y genera imagen y sonido
	void RunFrame (void);

	// Imagen: SCREEN_WIDTH x SCREEN_HEIGHT píxeles RGB565
	TPixel *GetScreenBuffer (void)		{ return m_Screen; }
	CAudioMixer *GetAudio (void)		{ return &m_Audio; }
	CAY8912 *GetAY (void)			{ return &m_AY; }

	void SetTapeMode (TTapeMode Mode)	{ m_TapeMode = Mode; }
	TTapeMode GetTapeMode (void) const	{ return m_TapeMode; }
	void TapePlay (void);
	void TapeStop (void);
	void TapeInserted (void);	// tras insertar o rebobinar: arma el autoarranque
	// TRUE si se está cargando una cinta en modo acelerado (no sincronizar)
	boolean IsTapeAccelerating (void) const;
	// Aceleración de cargadores como en Fuse (solo en carga rápida e instantánea)
	void SetLoaderAcceleration (boolean bOn)	{ m_bLoaderAccel = bOn; m_AccelMode = AccelNone; }
	boolean GetLoaderAcceleration (void) const	{ return m_bLoaderAccel; }

	// Multiface (MF1 en 48K, MF128 en 128K, MF3 en +2A). Las ROMs (8 KB) se
	// cargan desde la SD; sin ROM no hay Multiface para ese modelo.
	void SetMultifaceROM (TCodeModel CodeModel, const u8 *pROM);
	void SetMultifaceEnabled (boolean bEnabled)	{ m_bMFEnabled = bEnabled; }
	boolean IsMultifaceAvailable (void) const;
	boolean IsMultifacePaged (void) const		{ return m_bMFPaged; }

	// Interface 2: cartucho ROM de 16 KB que sustituye a la ROM del Spectrum
	// (16K/48K, 128K y +2; el +2A no lo admite). Incluye reset.
	boolean InsertCartridge (const u8 *pROM, unsigned nSize);
	void EjectCartridge (void);
	boolean IsCartridgeInserted (void) const	{ return m_bIF2Inserted; }

	// Disquetera del +3 (solo con el modelo +3)
	CPlus3Disk *GetDisk (void)		{ return m_pDisk; }
	boolean HasDiskDrive (void) const	{ return m_Model == ModelPlus3 || m_Model == ModelPlus3E; }

	// divMMC con esxDOS: puertos 0xE3 (control), 0xE7 (selección de tarjeta) y
	// 0xEB (datos SPI), 8 KB de ROM (esxmmc.bin) y 128 KB de RAM que aparecen en
	// 0x0000-0x3FFF al ejecutar los puntos de entrada (0x0000, 0x0008, 0x0038,
	// 0x0066, 0x04C6, 0x0562; 0x3D00-0x3DFF al instante) y se retiran al
	// ejecutar 0x1FF8-0x1FFF. La tarjeta es la misma que la de la ZXMMC.
	// En el +3e solo se ponen los puertos (sin ROM ni paginación), como en el
	// ZX-Uno, para los programas que hablan directamente con la tarjeta.
	void SetDivMMCROM (const u8 *pROM);		// 8 KB (0 = sin divMMC completa)
	void SetDivMMCEnabled (boolean bOn);		// divmmc=esxdos
	boolean IsDivMMCActive (void) const	{ return m_bDivMMC && m_bDivROM && m_Model != ModelPlus3E; }
	boolean HasDivMMCPorts (void) const	{ return IsDivMMCActive () || m_Model == ModelPlus3E; }

	// Reloj para esxDOS (RTC.SYS de ZXMetalCircle): fecha y hora en formato
	// MS-DOS (0 = sin hora). Puerto 0x7C3B: un OUT la fija y cuatro IN la
	// leen (fecha byte bajo, alto, hora byte bajo, alto). Solo con la divMMC.
	void SetRTC (u16 nDate, u16 nTime)	{ m_nRTCDate = nDate; m_nRTCTime = nTime; }

	// Spectranet (tarjeta de red): ver spectranet.h
	CSpectranet *GetSpectranet (void)	{ return &m_Snet; }
	void SetSpectranetEnabled (boolean bOn);
	boolean IsSpectranetEnabled (void) const	{ return m_bSnet; }

	// ZXMMC (tarjeta SD del +3e): puertos 0x1F (CS) y 0x3F (datos SPI)
	CSDCard *GetSDCard (void)		{ return &m_SDCard; }
	boolean HasZXMMC (void) const		{ return m_Model == ModelPlus3E; }

	// ROMs cargadas de la SD (16 KB por banco: 1 en 16K/48K, 2 en 128K/+2,
	// 4 en +2A/+3/+3e). Solo el 48K y el 128K tienen ROM incluida; un modelo
	// sin ROM no está disponible y SetModel () usa el 128K en su lugar.
	void SetModelROM (TModel Model, const u8 *pROM);
	boolean IsModelAvailable (TModel Model) const;
	TModel TakeFallback (void)	{ TModel M = m_FallbackFrom; m_FallbackFrom = ModelCount; return M; }

	// ROM del Interface 1 (8 KB): sin ella no se puede conectar
	void SetIF1ROM (const u8 *pROM);
	CInterface1 *GetIF1 (void)		{ return &m_IF1; }
	boolean HasIF1ROM (void) const	{ return m_bIF1ROM; }

	// Interface 1 con microdrives (no en el +2A)
	void SetIF1Connected (boolean bConnected);
	boolean IsIF1Connected (void) const	{ return m_bIF1Connected; }
	CInterface1 *GetInterface1 (void)	{ return &m_IF1; }
	// Lectura de la memoria tal como la ve ahora la CPU (sin contención; pruebas)
	u8 ReadMemory (u16 nAddress) const	{ return m_pReadPage[nAddress >> 14][nAddress & 0x3FFF]; }

	// Ampliación de memoria LEC: 16 páginas de 32 KB que sustituyen a los 32 KB
	// bajos (0x0000-0x7FFF). Necesaria para el CP/M de LEC. En los 128K (128,
	// +2, +2A, +3, +3e) funciona en modo 48K: con la paginación bloqueada y la
	// ROM del BASIC de 48K (opción "48 BASIC" o USR 0).
	boolean SetLECConnected (boolean bConnected);
	boolean IsLECConnected (void) const	{ return m_pLECRAM != 0; }
	boolean IsLECPaged (void) const		{ return m_bLECPaged; }
	// La LEC se ha paginado desde el último reset (el CP/M está en marcha; en
	// el prompt la LEC puede estar despaginada mientras el BIOS lee el teclado)
	boolean IsLECInUse (void) const		{ return m_bLECUsed; }

	// Teclado Issue 2 (16K/48K antiguos): el bit EAR leído depende también del bit MIC
	void SetIssue2 (boolean bIssue2)	{ m_bIssue2 = bIssue2; }
	boolean IsIssue2 (void) const		{ return m_bIssue2; }

	// ULAplus (64 colores, puertos 0xBF3B/0xFF3B)
	void SetULAplusEnabled (boolean bEnabled)	{ m_bULAplusEnabled = bEnabled; }
	// negro puro (#000000) también en la paleta del 16K/48K (pureblack=1)
	void SetPureBlack (boolean bPureBlack);
	boolean IsPureBlack (void) const	{ return m_bPureBlack; }
	TPixel GetStdColor (unsigned nIndex) const	{ return m_StdColor[nIndex & 15]; }
	boolean IsULAplusActive (void) const		{ return m_bULAplusActive; }

	// Grabación por pulsos de la salida MIC (cualquier rutina de grabación,
	// incluidas las turbo). Durante la grabación no se usa el trap de SAVE.
	// Formato de los datos: RLE de CSW, 1 muestra = 16 t-states.
	// Entrada EAR externa (casete real por GPIO o tarjeta de sonido USB)
	void SetEarInput (CEarInput *pEarInput)	{ m_pEarInput = pEarInput; m_Audio.SetEarInput (pEarInput); }
	boolean IsEarInputActive (void) const;

	// bEarInput: graba la entrada EAR externa en lugar de la salida MIC
	boolean StartRecording (boolean bEarInput = FALSE);
	boolean IsRecordingEarInput (void) const	{ return m_bRecording && m_bRecordEar; }
	void StopRecording (void);
	boolean IsRecording (void) const	{ return m_bRecording; }
	const u8 *GetRecording (unsigned *pLength, unsigned *pPulses, unsigned *pSampleRate) const;
	void FreeRecording (void);

	// Grabación con SAVE (0 = desactivada: la ROM graba "al aire")
	void SetSaveHandler (CTapeSaveHandler *pHandler)	{ m_pSaveHandler = pHandler; }

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
	void BuildTables (void);
	int TstatesToBorderPix (int nTstates);

	void UpdateScreen (int nTstates);
	void UpdateBorder (int nTstates);

	void PreIO (uint16_t nPort);
	void PostIO (uint16_t nPort);

	inline void Contend (uint16_t nAddress, int nTstates);
	inline void WriteByte (uint16_t nAddress, uint8_t nValue);
	u8 Plus3FloatingBus (void);
	void NoteBus (u8 nValue)	{ m_nBusTstates = GetAbsoluteTstates (); m_nBusValue = nValue; m_bBusValid = TRUE; }
	inline boolean IsScreenWrite (uint16_t nAddress) const;

	u8 FloatingBus (void);
	void PageMultiface (void);
	void UnpageMultiface (void);
	boolean MultifaceIn (uint16_t nPort, u8 *pValue);	// TRUE si el MF da el valor
	void MultifaceOut (uint16_t nPort);
	boolean FlashLoad (void);		// carga instantánea en 0x0562 (como Fuse)
	boolean FlashSave (void);
	u64 GetAbsoluteTstates (void) const	{ return m_nFrameStart + (u64) m_nTstates; }
	void TapeFrameHousekeeping (void);

private:
	CZXKeyboard *m_pKeyboard;
	CTape *m_pTape;
	Z80 m_CPU;
	CAY8912 m_AY;
	CAudioMixer m_Audio;

	TModel m_Model;
	const TMachineTiming *m_pTiming;

	int m_nTstates;			// t-states del cuadro actual
	u64 m_nFrameStart;		// t-states absolutos al comienzo del cuadro
	boolean m_bPlus3Bus;		// +2A/+3: se apuntan los accesos a 0x4000-0x5FFF
	boolean m_bBusValid;
	u64 m_nBusTstates;		// último acceso de la CPU a 0x4000-0x5FFF
	u8 m_nBusValue;
	unsigned m_nFrames;

	// memoria
	u8 m_RAM[8][0x4000];
	u8 m_ROMSink[0x4000];		// destino de las escrituras en ROM
	u8 m_NoRAM[0x4000];		// memoria inexistente (16K): se lee 0xFF
	const u8 *m_ROM[4];
	const u8 *m_pReadPage[4];
	u8 *m_pWritePage[4];
	u8 *m_pScreenPage;
	boolean m_bContendedRAM[4];
	boolean m_bContendedIO[4];
	const u8 *m_pBasicROM;		// ROM donde vive LD-BYTES (para el trap de carga)

	u8 m_nPortFE;
	u8 m_nPort7FFD;
	u8 m_nPort1FFD;
	boolean m_bEarOut;
	boolean m_bIssue2;

	// pantalla
	TPixel m_Screen[SCREEN_WIDTH * SCREEN_HEIGHT];
	TPixel m_StdColor[16];		// colores normales del Spectrum en RGB565
	boolean m_bPureBlack;
	void UpdatePalette (void);
	u8 m_Delay[MAX_TSTATES_FRAME + 128];		// contención
	int m_States2Border[MAX_TSTATES_FRAME + 128];
	int m_StepStates[6144];
	u16 m_StepAddr[6144];
	unsigned m_nSteps, m_nStep;
	int m_nFirstBorderUpdate, m_nLastBorderUpdate, m_nLastChgBorder;
	u16 m_ScrAddr[192];		// offset (0-0x17FF) del primer byte de cada línea
	u16 m_Scr2Attr[6144];
	u32 m_BufAddr[6144];
	u8 m_Ink[256], m_Paper[256];
	boolean m_bFlash;

	// ULAplus
	void ULAplusWrite (uint16_t nPort, u8 nValue);
	void SetULAplusColor (unsigned nIndex, u8 nColor);
	boolean m_bULAplusEnabled;
	boolean m_bULAplusActive;
	u8 m_nULAplusRegister;		// 0-63 = entrada de la paleta, 0x40 = modo
	u8 m_ULAplusPalette[64];	// GGGRRRBB
	TPixel m_ULAplusColor[64];

	// Disquetera del +3
	CPlus3Disk *m_pDisk;

	// divMMC
	boolean DivPaged (void) const
		{ return m_bDivActive && (m_bDivAutomap || (m_nDivControl & 0x80)); }
	u8 DivRead (u16 nAddress) const;
	void DivWrite (u16 nAddress, u8 nValue);
	u8 DivFetch (u16 nAddress);
	u8 SnetFetch (u16 nAddress);
	void UpdateDivActive (void)	{ m_bDivActive = IsDivMMCActive (); }

	boolean m_bNMIPending;

	// Reloj para esxDOS
	u16 m_nRTCDate, m_nRTCTime;
	u8 m_RTCLatch[4];
	unsigned m_nRTCIndex;

	// Spectranet
	CSpectranet m_Snet;
	boolean m_bSnet;
	boolean m_bPrevED;		// el último código leído en un M1 fue el prefijo ED
	boolean m_bSnetNMIPending;	// paginar la Spectranet al aceptarse la NMI
	boolean m_bDivMMC;		// divmmc=esxdos
	boolean m_bDivROM;
	boolean m_bDivActive;		// caché de IsDivMMCActive () (se mira en cada acceso)
	boolean m_bDivAutomapLoad;		// la paginación automática ha sido en 0x0562
	boolean m_bDivAutomap;		// paginada por un punto de entrada
	u8 m_nDivControl;		// puerto 0xE3: CONMEM, MAPRAM, banco
	u8 m_DivROM[0x2000];
	u8 *m_pDivRAM;			// 128 KB: 16 bancos de 8 KB

	// ZXMMC
	CSDCard m_SDCard;
	u8 m_nMMCData;			// ZXMMC: byte recibido en la transferencia anterior
	u8 m_nDivData;			// divMMC: ídem (cada interfaz tiene su registro)

	const u8 *m_pModelROM[ModelCount];
	TModel m_FallbackFrom;		// modelo pedido sin ROM (ModelCount = ninguno)
	boolean m_bIF1ROM;

	// Interface 1
	boolean IF1Active (void) const
		{ return m_bIF1Connected && m_pTiming->CodeModel != CodeModelPlus3; }
	CInterface1 m_IF1;
	boolean m_bIF1Connected;
	boolean m_bIF1Paged;
	u8 m_IF1Page[0x4000];		// ROM de 8 KB repetida

	// LEC
	boolean LECActive (void) const
		{ return m_pLECRAM != 0 && (m_Model == Model48K || LEC128Mode ()); }
	boolean LEC128Mode (void) const;	// 128K en modo 48K (ver arriba)
	boolean IsLECPort (u16 nPort, u8 nValue) const;
	void LECWrite (u8 nValue);
	u8 *m_pLECRAM;			// páginas 0-14 (la 15 es la RAM alta del propio 48K)
	boolean m_bLECPaged;
	boolean m_bLECUsed;
	unsigned m_nLECPage;

	// Interface 2
	boolean m_bIF2Inserted;
	u8 m_IF2ROM[0x4000];

	// Multiface
	boolean m_bMFEnabled;
	boolean m_bMFPaged;
	boolean m_bMFLocked;
	boolean m_bMFHasROM[3];
	u8 m_MFROM[3][0x2000];
	u8 m_MFPage[0x4000];		// 0x0000-0x1FFF ROM, 0x2000-0x3FFF RAM

	// grabación por pulsos
	void RecordEdge (u64 nTime);
	boolean m_bRecording;
	boolean m_bRecordEar;
	CEarInput *m_pEarInput;
	u8 *m_pRecBuffer;
	unsigned m_nRecLength, m_nRecCapacity, m_nRecPulses;
	u64 m_nRecLastEdge;
	unsigned m_nRecRemainder;

	// cinta
	CTapeSaveHandler *m_pSaveHandler;
	TTapeMode m_TapeMode;
	unsigned m_nFEReads;
	unsigned m_nIdleFrames;
	unsigned m_nLoadingFrames;
	boolean m_bAutoStopped;

	// aceleración de cargadores (ver CheckLoaderAcceleration)
	enum TAccelMode { AccelNone, AccelIncreasing, AccelDecreasing };
	boolean m_bLoaderAccel;
	TAccelMode m_AccelMode;
	u16 m_nAccelPC;
	unsigned m_nAccelEdges;		// flancos acelerados en este cuadro
	// parada automática: ¿el programa sigue los flancos de la cinta?
	u32 m_nSeenEdge;			// último flanco que ha visto una lectura de 0xFE
	u32 m_nFrameEdges;			// flancos al empezar el cuadro
	unsigned m_nObservedEdges;		// lecturas de este cuadro que han visto un flanco nuevo
	// ¿lee como un cargador? (ver CountLoaderRead)
	unsigned m_nLoaderReads;		// lecturas de 0xFE de este cuadro con pinta de cargador
	u16 m_nLastFEPC;
	u16 m_nLastFEPort;
	u8 m_LastFERegs[6];			// B, C, D, E, H, L en la lectura anterior
	void CountLoaderRead (u16 nPort);
	u8 PeekLoader (u16 nAddress) const;
	boolean LoaderPatternMatches (u16 nAddress, const struct TLoaderPatternByte *pPattern,
				      unsigned nLength) const;
	boolean LoaderWordMatches (u16 nAddress, u16 nValue) const;
	TAccelMode DetectLoader (u16 nPC) const;
	TAccelMode DetectLoaderAtIn (u16 nPC) const;
	boolean ModifiedROMLoader (u16 nPC) const;
	void CheckLoaderAcceleration (void);
};

#endif
