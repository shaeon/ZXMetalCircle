//
// kernel.h - ZXCircle: emulador ZX Spectrum bare-metal para Raspberry Pi
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_kernel_h
#define _zx_kernel_h

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/usb/usbgamepad.h>
#include <circle/input/mouse.h>
#include <circle/sound/soundbasedevice.h>
#include <SDCard/emmc.h>
#include <fatfs/ff.h>
#include <circle/types.h>

#include "video.h"
#include "zxkeyboard.h"
#include "tape.h"
#include "spectrum.h"
#include "menu.h"
#include "gpiokeyboard.h"
#include "recreated.h"
#include "network.h"
#include "harddisk.h"
#include "tnfsbrowser.h"
#include "earinput.h"
#include "earsource.h"
#include "exclusions.h"
#include "snetcircle.h"
#include "settings.h"
#include "esxfolder.h"
#include "if1link.h"
#include "i18n.h"
#include "videocore.h"
#include <circle/gpiomanager.h>
#include <circle/sched/scheduler.h>

#define MAX_KEYBOARDS	4

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel : public CTapeSaveHandler
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

	// CTapeSaveHandler
	boolean SaveTapeBlock (const u8 *pBlock, unsigned nLength) override;

private:
	void ParseOptions (void);
	boolean InitSound (const char *pDevice);
	void UpdateUSB (void);
	void HandleKeyEvent (const TKeyEvent &rEvent);
	void UpdateKeyRepeat (void);		// repetición de los cursores en los menús
	boolean LoadTape (const char *pPath);
	boolean LoadSnapshot (const char *pPath);
	boolean SaveSnapshot (void);
	boolean LoadCartridge (const char *pPath);
	void ToggleRecording (boolean bEarInput = FALSE);
	void StartEarInput (void);
	void LoadAnyFile (const char *pPath);
	void InsertDisk (const char *pPath, int nForceUnit = -1);	// 0: disco nuevo en la primera unidad libre
	void EjectDisks (void);
	void MountHardDisk (const char *pPath);
	void DownloadAndLoad (const char *pNetPath);
	static void BusyHandler (const char *pText, void *pParam);
	static void NetBusyHandler (const char *pText, void *pParam);
	void ShowFrameNow (const TPixel *pScreen);	// se ve ya (avisos con la emulación parada)
	void LoadROMs (void);
	void CheckCmdLine (void);
	void InitSpectranet (void);
	boolean LoadSpectranetFlash (void);
	void ToggleSpectranet (void);
	void ToggleDivMMC (void);
	void ToggleLEC (void);
	void LoadSettings (void);
	static boolean FindCmdLineOption (const char *pCmdLine, const char *pKey, char *pValue, unsigned nSize);
	void SyncSetting (const char *pKey);
	void SyncPeripheralSettings (void);
	void ApplySpeaker (void);
	char			m_Speaker[8];		// speaker=: auto, beeper, tv u off
	void ApplySetting (unsigned nIndex);
	void SaveSettings (void);
	void SetupEsxFolder (void);
	void SetupLanguages (void);
	void OpenUnitsScreen (void);
	void SetupHardDiskList (void);
	void SetupROMLists (void);
	const char *ROMOption (const char *pKey, const char *pDefault);
#define ROM_LISTS 9
	CString			m_ROMValues[ROM_LISTS];	// ROMs de SD:/roms para la configuración
	CString			m_ROMNames[ROM_LISTS];
	CString			m_EsxDirValues;		// carpetas para esxdir=
	CString			m_EsxDirNames;
	CString			m_HDDValues;		// imágenes de SD:/hdd para la configuración
	CString			m_HDDNames;
	void UpdateUnitLines (void);
	void MountUnit (unsigned nUnit, const char *pPath);
	void EjectUnit (unsigned nUnit);
	void ShowFrame (const TPixel *pScreen);
	void SetVideoMode (TBorderSize Border, boolean bScanlines);
	void PollVideo (void);
	void CheckTapeModel (void);
	void OpenTapeSelect (void);
	void UpdateRTC (void);
	void RebootPi (boolean bSaveSettings);
	boolean IsEsxImageMounted (void) const;
	void EsxFolderToImage (boolean bRemount);
	static void EsxProgressHandler (unsigned nDone, unsigned nTotal, unsigned nMBDone, unsigned nMBTotal,
					void *pParam);
	void SaveSpectranetFlash (void);
	const u8 *LoadROMFile (const char *pName, unsigned nSize);
	void SaveDisks (boolean bForce);
	unsigned ReadWholeFile (const char *pPath, u8 *pBuffer, unsigned nMaxSize);
	void YieldDelay (unsigned nMilliseconds);
	void StartNetwork (void);
	boolean InsertMicrodrive (const char *pPath, int nForceDrive = -1);	// 0 = cartucho en blanco
	void EjectMicrodrives (void);
	void SaveMicrodrives (boolean bForce);		// graba los cartuchos modificados
	unsigned CountMicrodrives (void);
	boolean WriteFile (const char *pPath, const u8 *pData, unsigned nSize, boolean bAppend);
	void ShowMessage (const char *pMessage);
	void DrawMessage (TPixel *pScreen);
	void UpdateStatusLines (void);
	void OutputAudio (void);
	void WaitFrame (void);
	void SetModel (TModel Model);
	void HandleGPIOKeyboard (void);
	void LoadMultifaceROMs (void);
	boolean LoadFile (const char *pPath, u8 *pBuffer, unsigned nSize);
	void DrawIndicators (TPixel *pScreen);
	void OpenMenu (void);		// prepara la pantalla para un menú
	void DrawKeyDebug (TPixel *pScreen);	// Alt+D: estado del teclado
	void MenuClosed (void);

	static void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6],
					 void *pArg);
	static void KeyboardRemovedHandler (CDevice *pDevice, void *pContext);
	static void GamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pState);
	static void GamePadRemovedHandler (CDevice *pDevice, void *pContext);
	static void MouseStatusHandler (unsigned nButtons, int nDX, int nDY, int nWheel);
	static void MouseRemovedHandler (CDevice *pDevice, void *pContext);

private:
	// no cambiar el orden de construcción
	CActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	CSerialDevice		m_Serial;
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer			m_Timer;
	CLogger			m_Logger;
	CScheduler		m_Scheduler;	// necesario para la red (tareas cooperativas)
	CGPIOManager		m_GPIOManager;	// interrupciones de GPIO (entrada EAR)
	CUSBHCIDevice		m_USBHCI;
	CEMMCDevice		m_EMMC;
	FATFS			m_FileSystem;
	boolean			m_bSDMounted;

	CVideo			m_Video;
	CZXKeyboard		m_Keyboard;
	CTape			m_Tape;
	CSpectrum		*m_pSpectrum;
	CMenu			m_Menu;
	CNetworkManager		m_Network;
	CSoundBaseDevice	*m_pSound;
	const char		*m_pSoundName;

	CUSBKeyboardDevice * volatile m_pKeyboards[MAX_KEYBOARDS];
	CRecreatedKeyboard	m_Recreated[MAX_KEYBOARDS];	// Recreated ZX (uno por teclado USB)
	TRecreatedMode		m_RecreatedMode;

	// Mandos USB
	CUSBGamePadDevice * volatile m_pGamePads[MAX_GAMEPADS];
	unsigned		m_GamePadProps[MAX_GAMEPADS];
	unsigned		m_PrevPadButtons[MAX_GAMEPADS];	// para detectar pulsaciones nuevas
	u8			m_PrevPadJoy[MAX_GAMEPADS];

	// Ratón USB (ratón Kempston)
	CMouseDevice * volatile	m_pMouse;

	// Teclado real por GPIO
	CGPIOKeyboard		m_GPIOKeyboard;
	boolean			m_bGPIOKeyboard;
	boolean			m_bExtendedMode;	// 1-0 = F1-F10, SYMBOL SHIFT = Alt
	boolean			m_bWaitRelease;
	boolean			m_bPrevButton;
	boolean			m_bPrevCombo;
	unsigned		m_nNMIReleased;		// cuadros con el botón de NMI suelto

	// Audio PWM también por GPIO12 (pin 32)
	CGPIOPin		*m_pAudioGPIO;
	boolean			m_bMonoAudio;
	u8			m_PrevGPIOMatrix[8];

	// Búferes grandes en el heap: la imagen del kernel (código + datos) no puede
	// pasar de 2 MB en Circle, y CKernel tampoco debe vivir en la pila
	TPixel			*m_pFrozen;		// imagen bajo el menú
	TPixel			*m_pOverlay;
	u8			*m_pWork;		// ficheros: snapshots, microdrives
#define WORK_BUFFER_SIZE	(2 * 1024 * 1024)
	boolean			m_bPaused;
	char			m_Message[44];
	unsigned		m_nMessageFrames;
	char			m_TapePath[MENU_PATH_LEN];	// fichero de la cinta actual
	boolean			m_bSaveTape;			// la cinta actual es de grabaciones
	char			m_CartridgeName[48];
	boolean			m_bSaveTZX;		// SAVE en .TZX (si no, .TAP)
	unsigned		m_nSnapFormat;		// 0 = .Z80, 1 = .SNA, 2 = .SZX
	unsigned		m_nIF1Mode;		// 0 = nunca, 1 = siempre, 2 = al insertar un cartucho
	char			m_MDRPath[MICRODRIVES][MENU_PATH_LEN];
	char			m_DSKPath[2][MENU_PATH_LEN];	// discos del +3 (A: y B:)
	CHardDiskImage		m_HardDisk;			// tarjeta de la ZXMMC (+3e)
	char			m_HDDPath[MENU_PATH_LEN];
	u64			m_nHDActivity;
	CString			m_MissingROMs;		// para el aviso al arrancar
	CTNFSBrowser		m_TNFS;			// servidores TNFS en el selector
	CEarInput		m_EarInput;		// casete real (GPIO o USB)
	CExclusions		m_Exclusions;		// excluir.txt (selector y web)
	CSpectranetCircleNet	m_SnetNet;		// red de la Pi para la Spectranet
	CSettings		m_Settings;		// pantallas de configuración
	CEsxFolder		m_EsxFolder;		// esxDOS sobre la carpeta SD:/esxdos
	unsigned		m_nEsxProgressTime;	// último aviso de progreso de la importación
	CIF1Link		m_IF1Link;		// RS232 (TCP 2323) y ZX Net (UDP 5050)
	CI18n			m_I18n;			// idiomas (SD:/locales)
#ifdef ARM_ALLOW_MULTI_CORE
	CVideoCore		*m_pVideoCore;		// ampliación de la imagen en el núcleo 1
#endif
	unsigned		m_nCore0Time;		// tiempo de cada cuadro en el núcleo 0 (us, media)
	u64			m_nFrameStart;
	CString			m_LangValues;		// "es|en..." para la configuración
	CString			m_LangNames;
	unsigned		m_nSnetSaveFrames;	// cuadros para guardar la flash (0 = nada)
	unsigned		m_nRTCLast;		// segundo de la última hora dada a esxDOS
	boolean			m_bModelChoice;		// pregunta: la cinta no va en este modelo
	boolean			m_bDiskChoice;		// pregunta: formato del disco nuevo
	boolean			m_bNewDisk720;		// el disco nuevo es de 720K
	TModel			m_SuggestedModel;
	boolean			m_bRebootChoice;	// la lista abierta es la del reinicio
	char			m_OptionWarning[44];	// aviso de cmdline.txt al arrancar
	unsigned		m_nWarningFrames;	// cuadros que le quedan al aviso
	CEarSource		*m_pEarSource;
	CSoundBaseDevice	*m_pUSBInput;		// tarjeta USB solo para la entrada
	boolean			m_bUSBSound;		// sounddev=sndusb
	boolean			m_bEarUSB;		// ear=usb
	u8			m_nRepeatKey;		// tecla que se repite en el menú (0 = ninguna)
	u64			m_nRepeatNext;		// próxima repetición (us)
	u64			m_nDiskMotorStop;
	u64			m_nLastDSKSave;
	unsigned		m_nFrames;
	u64			m_nNextFrameTime;	// en nanosegundos
	unsigned		m_nSpeed;		// cuadros emulados por cuadro real (1 = normal)
	u64			m_nFPSStart;		// medida de la velocidad real
	unsigned		m_nFPSFrames;
	unsigned		m_nFPSx100;		// cuadros por segundo x 100
	u64			m_nMotorStop;		// último momento con un microdrive girando
	boolean			m_bKeyDebug;		// Alt+D
	volatile u8		m_LastReport[8];	// último informe USB (modificadores + 6 teclas)
	unsigned		m_nSkippedFrames;	// cuadros no mostrados por ir con retraso
	u64			m_nLastMDRSave;
	TBorderSize		m_BorderSize;		// tamaño de borde elegido
	boolean			m_bScanlines;

	static CKernel *s_pThis;
};

#endif
