//
// kernel.cpp - ZXCircle: emulador ZX Spectrum bare-metal para Raspberry Pi
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "kernel.h"
#include "snapshot.h"
#include <circle/sound/pwmsoundbasedevice.h>
#include <circle/sound/hdmisoundbasedevice.h>
#include <circle/string.h>
#include <circle/gpiopin.h>
#include <circle/synchronize.h>
#include <circle/util.h>

#include "version.h"
#include "harddisk.h"
#include "tnfsbrowser.h"
#include "optcheck.h"
#include "i18n.h"
#include <stdlib.h>
#include <circle/time.h>
#include <circle/startup.h>
#if RASPPI >= 4
#include <circle/sound/usbsoundbasedevice.h>	// sonido USB: solo Pi 4 y 400 (xHCI)
#endif
#include <circle/machineinfo.h>

static const char FromKernel[] = "zxcircle";

#define RECREATED_VID	0x0F39		// Recreated ZX Spectrum por cable USB (TG3 Electronics)
#define RECREATED_PID	0x0404

#define SOUND_CHUNK_SIZE	(384 * 4)	// múltiplo de 384 (requisito HDMI), ~16 ms
#define SOUND_QUEUE_MS		120
#define SOUND_TARGET_FRAMES	(AUDIO_SAMPLE_RATE * 50 / 1000)	// ~50 ms en cola
#define SOUND_MAX_QUEUED_FRAMES	(AUDIO_SAMPLE_RATE * 90 / 1000)	// más de ~90 ms: se descarta

#define MESSAGE_FRAMES		150	// 3 s
#define ACCEL_PRESENT_EVERY	8	// en carga acelerada se muestra 1 de cada 8 cuadros

static const char *JoystickNames[JoystickModeCount] =
{
	N_("ninguno"), "Kempston", "Fuller", "Sinclair 1", "Sinclair 2", "Cursor/AGF/Protek", "QAOPSpaceM"
};

// Pseudotecla generada por el botón Select/Share/View/Capture de los mandos
#define KEY_PAD_SELECT	0xF0

static const char *AYStereoNames[AYStereoModeCount] =
{
	"ABC", "ACB", "BAC", "mono"
};

static const char *TapeModeNames[TapeModeCount] =
{
	N_("instantánea"), N_("rápida"), N_("real")
};

// Teclas de función. Para cambiar una asignación basta con tocar esta tabla:
// la ayuda (F1) se genera a partir de ella.
#define MULTIKEY_FRAMES		75		// Alt+V: las teclas se mantienen 1,5 s

enum TAction
{
	ActionHelp,
	ActionKeyHelp,
	ActionTapePlayStop,
	ActionTapeRewind,
	ActionJoystick,
	ActionNMI,
	ActionTapeMode,
	ActionModel,
	ActionReset,
	ActionPause,
	ActionSaveSnapshot,
	ActionAYStereo,
	ActionSpeed,
	ActionIssue,
	ActionEjectCartridge,
	ActionBorder,
	ActionScanlines,
	ActionRecord,
	ActionRecordEar,
	ActionKeyDebug,
	ActionMultiKey,			// Alt+V: pulsar varias teclas a la vez
	ActionBlankDisk,
	ActionEjectDisks,
	ActionBlankMicrodrive,
	ActionEjectMicrodrives,
	ActionLEC,
	ActionSpectranet,
	ActionDivMMC,
	ActionUnits
};

static const unsigned Speeds[] = {1, 2, 4, 10};

struct TKeyBinding
{
	u8 nKey;		// código HID
	boolean bAlt;		// requiere Alt
	TAction Action;
	const char *pKeyName;
	const char *pDescription;	// NULL: no se muestra en la ayuda (atajo repetido)
};

static const TKeyBinding KeyBindings[] =
{
	{HID_KEY_F1,     FALSE, ActionHelp,         "F1",     N_("Ayuda, cintas y snapshots")},
	{0x0E,           TRUE,  ActionKeyHelp,      "Alt+K",  N_("Ayuda de teclado")},
	{HID_KEY_F1 + 1, FALSE, ActionTapePlayStop, "F2/F3",  N_("Cinta: play-stop / bloques")},
	{HID_KEY_F1 + 2, FALSE, ActionTapeRewind,   "F3",     0},
	{HID_KEY_F1 + 3, FALSE, ActionJoystick,     "F4",     N_("Cambiar joystick")},
	{HID_KEY_F1 + 4, FALSE, ActionNMI,          "F5",     N_("NMI / Multiface")},
	{HID_KEY_F1 + 5, FALSE, ActionTapeMode,     "F6",     N_("Velocidad de carga")},
	{HID_KEY_F1 + 6, FALSE, ActionModel,        "F7",     N_("Modelo (16K, 48K ... +3, +3e)")},
	{HID_KEY_F1 + 7, FALSE, ActionSaveSnapshot, "F8",     N_("Grabar snapshot")},
	{HID_KEY_F1 + 8, FALSE, ActionAYStereo,     "F9",     N_("Sonido AY: ABC/ACB/BAC/mono")},
	{HID_KEY_F1 + 9, FALSE, ActionSpeed,        "F10",    N_("Velocidad x1/x2/x4/x10")},
	// Alt+I (Issue 2/3) se quitó: solo en la configuración (F1, C, página 1), que
	// la pone en gris fuera del 16K y el 48K. Así cabe la línea de la red en la ayuda.
	{HID_KEY_F1 + 10, FALSE, ActionEjectCartridge, "F11", N_("Quitar cartucho (IF2)")},
	{0x18,           TRUE,  ActionUnits,        "Alt+U",  N_("Unidades: montar y vaciar")},
	{0x05,           TRUE,  ActionBorder,       "Alt+B/S", N_("Borde / scanlines")},
	{0x16,           TRUE,  ActionScanlines,    "Alt+S",  0},
	{0x1A,           TRUE,  ActionRecord,       "Alt+W/G", N_("Grabar salida MIC / casete")},
	{0x0A,           TRUE,  ActionRecordEar,    "Alt+G",  0},
	{0x11,           TRUE,  ActionBlankMicrodrive, "Alt+N/M", N_("Microdrive: nuevo / sacar todos")},
	{0x10,           TRUE,  ActionEjectMicrodrives, "Alt+M", 0},
	{0x0F,           TRUE,  ActionLEC,          "Alt+L/T/X", N_("LEC/Spectranet/divMMC sí/no")},
	{0x17,           TRUE,  ActionSpectranet,   "Alt+T",  0},
	{0x1B,           TRUE,  ActionDivMMC,       "Alt+X",  0},
	{0x07,           TRUE,  ActionKeyDebug,     "Alt+D/V", N_("Teclado: diagnóstico / varias a la vez")},
	{0x19,           TRUE,  ActionMultiKey,     "Alt+V",  0},
	{0x09,           TRUE,  ActionBlankDisk,    "Alt+F/E", N_("Disco +3: nuevo / expulsar")},
	{0x08,           TRUE,  ActionEjectDisks,   "Alt+E",  0},
	{HID_KEY_F12,    FALSE, ActionReset,        "F12",    N_("Reset (también Alt+R)")},
	{0x15,           TRUE,  ActionReset,        "Alt+R",  0},
	{HID_KEY_PAUSE,  FALSE, ActionPause,        "Pausa",  N_("Pausa (también Alt+P)")},
	{0x13,           TRUE,  ActionPause,        "Alt+P",  0},
	{KEY_PAD_SELECT, FALSE, ActionJoystick,     "Select", 0},	// botón del mando
};

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_GPIOManager (&m_Interrupt),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),	// TRUE: conexión en caliente
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_bSDMounted (FALSE),
	m_pSpectrum (0),
	m_Menu (&m_Video),
	m_pSound (0),
	m_pSoundName (N_("ninguno")),
	m_bPaused (FALSE),
	m_nMessageFrames (0),
	m_nFrames (0),
	m_nNextFrameTime (0)
{
	m_nSpeed = 1;
	m_nLastKeyTime = 0;
	m_nFPSStart = 0;
	m_nFPSFrames = 0;
	m_nFPSx100 = 0;
	m_nMotorStop = 0;
	m_nLastMDRSave = 0;
	m_nDiskMotorStop = 0;
	m_nLastDSKSave = 0;
	m_nHDActivity = 0;
	m_HDDPath[0] = '\0';
	m_nEsxProgressTime = 0;
	m_pEarSource = 0;
	m_pUSBInput = 0;
	m_bUSBSound = FALSE;
	m_bEarUSB = FALSE;
	m_nRepeatKey = 0;
	m_nRepeatNext = 0;
	m_nWarningFrames = 0;
	m_OptionWarning[0] = '\0';
	m_nSnetSaveFrames = 0;
	m_nRTCLast = 0;
	m_bRebootChoice = FALSE;
	m_bModelChoice = FALSE;
	m_SuggestedModel = Model48K;
	m_bDiskChoice = FALSE;
	m_bNewDisk720 = FALSE;
#ifdef ARM_ALLOW_MULTI_CORE
	m_pVideoCore = 0;
#endif
	m_nCore0Time = 0;
	m_nFrameStart = 0;
	m_DSKPath[0][0] = m_DSKPath[1][0] = '\0';
	m_bKeyDebug = FALSE;
	m_nSkippedFrames = 0;
	for (unsigned i = 0; i < 8; i++)
	{
		m_LastReport[i] = 0;
	}
	m_BorderSize = BorderFull;
	m_RecreatedMode = RecreatedOff;
	m_bScanlines = FALSE;
	m_RecreatedMode = RecreatedOff;
	m_bGPIOKeyboard = FALSE;
	m_bExtendedMode = FALSE;
	m_bWaitRelease = FALSE;
	m_bPrevButton = FALSE;
	m_bPrevCombo = FALSE;
	m_nNMIReleased = 0;
	m_pAudioGPIO = 0;
	m_bMonoAudio = FALSE;
	memset (m_PrevGPIOMatrix, 0, sizeof m_PrevGPIOMatrix);
	s_pThis = this;

	for (unsigned i = 0; i < MAX_KEYBOARDS; i++)
	{
		m_pKeyboards[i] = 0;
	}

	m_pMouse = 0;

	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		m_pGamePads[i] = 0;
		m_GamePadProps[i] = 0;
		m_PrevPadButtons[i] = 0;
		m_PrevPadJoy[i] = 0;
	}

	m_pFrozen = new TPixel[SCREEN_WIDTH * SCREEN_HEIGHT];
	m_pOverlay = new TPixel[SCREEN_WIDTH * SCREEN_HEIGHT];
	m_pWork = new u8[WORK_BUFFER_SIZE];

	m_Message[0] = '\0';
	m_TapePath[0] = '\0';
	m_CartridgeName[0] = '\0';
	m_bSaveTZX = FALSE;
	m_nSnapFormat = 0;
	m_nIF1Mode = 2;
	for (unsigned i = 0; i < MICRODRIVES; i++)
	{
		m_MDRPath[i][0] = '\0';
	}
	m_bSaveTape = FALSE;
	m_ActLED.Blink (2);
}

CKernel::~CKernel (void)
{
	s_pThis = 0;
}

boolean CKernel::Initialize (void)
{
	boolean bOK = m_Serial.Initialize (115200);

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Serial;
		}
		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		static const char *BorderOptions[BorderSizeCount] = {"none", "standard", "full", "huge"};
		const char *pBorder = m_Options.GetAppOptionString ("border", "full");
		for (unsigned i = 0; i < BorderSizeCount; i++)
		{
			if (strcasecmp (pBorder, BorderOptions[i]) == 0)
			{
				m_BorderSize = (TBorderSize) i;
			}
		}
		m_bScanlines = m_Options.GetAppOptionDecimal ("scanlines", 0) != 0;

		bOK = m_Video.Initialize (m_BorderSize, m_bScanlines);
		if (!bOK)
		{
			m_Logger.Write (FromKernel, LogError, "Cannot initialize frame buffer");
		}
#ifdef ARM_ALLOW_MULTI_CORE
		// La ampliación de la imagen va en el núcleo 1 (multicore=0 lo evita)
		else if (m_Options.GetAppOptionDecimal ("multicore", 1) != 0)
		{
			m_pVideoCore = new CVideoCore (&m_Video);
			if (!m_pVideoCore->Start ())
			{
				m_Logger.Write (FromKernel, LogWarning, "No arrancan los otros nucleos");
				delete m_pVideoCore;
				m_pVideoCore = 0;
			}
			else
			{
				m_Logger.Write (FromKernel, LogNotice, "Imagen ampliada en el nucleo 1");
			}
		}
#endif
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	if (bOK)
	{
		bOK = m_GPIOManager.Initialize ();
	}

	// La tarjeta SD no es imprescindible: sin ella no hay cintas, pero el emulador arranca
	if (bOK)
	{
		if (   m_EMMC.Initialize ()
		    && f_mount (&m_FileSystem, "SD:", 1) == FR_OK)
		{
			m_bSDMounted = TRUE;
		}
		else
		{
			m_Logger.Write (FromKernel, LogWarning, "Cannot mount SD card");
		}
	}

	if (bOK)
	{
		m_pSpectrum = new CSpectrum (&m_Keyboard, &m_Tape);
		bOK = m_pSpectrum != 0;
	}

	if (bOK)
	{
		m_Video.SetFont (m_pSpectrum->GetROMFont ());
		ParseOptions ();
	}

	return bOK;
}

void CKernel::ParseOptions (void)
{
	const char *pModel = m_Options.GetAppOptionString ("model", "128k");
	TModel Model = Model128K;
	if (strcasecmp (pModel, "16k") == 0)
	{
		Model = Model16K;
	}
	else if (strcasecmp (pModel, "48k") == 0)
	{
		Model = Model48K;
	}
	else if (strcasecmp (pModel, "128k") == 0)
	{
		Model = Model128K;
	}
	else if (strcasecmp (pModel, "plus2") == 0 || strcasecmp (pModel, "+2") == 0)
	{
		Model = ModelPlus2;
	}
	else if (strcasecmp (pModel, "plus2a") == 0 || strcasecmp (pModel, "+2a") == 0)
	{
		Model = ModelPlus2A;
	}
	else if (strcasecmp (pModel, "plus3") == 0 || strcasecmp (pModel, "+3") == 0)
	{
		Model = ModelPlus3;
	}
	else if (strcasecmp (pModel, "plus3e") == 0 || strcasecmp (pModel, "+3e") == 0)
	{
		Model = ModelPlus3E;
	}

	// Idioma (lang=, SD:/locales): antes de los avisos de las opciones
	SetupLanguages ();
	SetupHardDiskList ();
	SetupROMLists ();

	// Ficheros y carpetas que no se muestran en el selector ni en la web
	m_Exclusions.Load ();

	// Opciones de cmdline.txt mal escritas (se avisa al arrancar)
	CheckCmdLine ();

	// ROMs de la carpeta roms de la SD (todas salvo las del 48K y el 128K)
	LoadROMs ();

	// divMMC con esxDOS: divmmc=esxdos (ROM en roms/esxmmc.bin o romesx=)
#ifdef ZX_TEST_DIVMMC
	if (TRUE)		// solo pruebas en QEMU (no lee cmdline.txt)
#else
	if (strcasecmp (m_Options.GetAppOptionString ("divmmc", "0"), "esxdos") == 0)
#endif
	{
		const u8 *pESX = LoadROMFile (ROMOption ("romesx", "esxmmc.bin"), 0x2000);
		if (pESX != 0)
		{
			m_pSpectrum->SetDivMMCROM (pESX);
			m_pSpectrum->SetDivMMCEnabled (TRUE);
			m_Logger.Write (FromKernel, LogNotice, "divMMC con esxDOS");
		}
		else
		{
			m_MissingROMs.Append (m_MissingROMs.GetLength () > 0 ? " esxDOS" : "esxDOS");
		}
	}

	// Spectranet: spectranet=1
	InitSpectranet ();

	if (!m_pSpectrum->IsModelAvailable (Model))
	{
		m_Logger.Write (FromKernel, LogWarning, "Falta la ROM de %s: se arranca en 48K",
				MachineTiming[Model].pName);
		Model = Model48K;
	}
	m_pSpectrum->SetModel (Model);

	// Tarjeta SD: hdd=hdd/imagen.vhd. Con divmmc=esxdos y sin hdd=, se usa
	// hdd/esxdos.img (la tarjeta de esxDOS que viene en la SD)
	const char *pHDD = m_Options.GetAppOptionString ("hdd", "");
	if (strcmp (pHDD, "none") == 0 || strcmp (pHDD, "0") == 0)
	{
		pHDD = "";			// "ninguno" en la configuración
	}
	SetupEsxFolder ();
	if (*pHDD == '\0' && m_pSpectrum->IsDivMMCActive ())
	{
		if (m_EsxFolder.IsEnabled ())
		{
			EsxFolderToImage (FALSE);	// crea o pone al día la imagen
		}
		FILINFO Info;
		if (f_stat ("SD:/hdd/esxdos.img", &Info) == FR_OK)
		{
			pHDD = "hdd/esxdos.img";
		}
	}
	if (*pHDD != '\0' && m_bSDMounted)
	{
		CString Path;
		Path.Format (strncmp (pHDD, "SD:", 3) == 0 ? "%s" : "SD:/%s", pHDD);
		MountHardDisk (Path);
	}

	// Aceleración de cargadores (como en Fuse), en carga rápida e instantánea
	m_pSpectrum->SetLoaderAcceleration (m_Options.GetAppOptionDecimal ("tapeaccel", 1) != 0);

	const char *pTape = m_Options.GetAppOptionString ("tape", "flash");
	if (strcasecmp (pTape, "flash") == 0)
	{
		m_pSpectrum->SetTapeMode (TapeModeFlash);
	}
	else if (strcasecmp (pTape, "normal") == 0)
	{
		m_pSpectrum->SetTapeMode (TapeModeNormal);
	}
	else
	{
		m_pSpectrum->SetTapeMode (TapeModeFast);
	}

	static const char *JoyOptions[JoystickModeCount] =
	{
		"none", "kempston", "fuller", "sinclair1", "sinclair2", "cursor", "qaop"
	};
	// Sonido: reparto estéreo del AY y sonido de la carga
	static const char *StereoOptions[AYStereoModeCount] = {"abc", "acb", "bac", "mono"};
	const char *pStereo = m_Options.GetAppOptionString ("aystereo", "abc");
	for (unsigned i = 0; i < AYStereoModeCount; i++)
	{
		if (strcasecmp (pStereo, StereoOptions[i]) == 0)
		{
			m_pSpectrum->GetAY ()->SetStereoMode ((TAYStereoMode) i);
		}
	}
	m_pSpectrum->GetAudio ()->SetTapeSound (m_Options.GetAppOptionDecimal ("tapesound", 1) != 0);
	m_pSpectrum->GetAudio ()->SetBeeperFilter (m_Options.GetAppOptionDecimal ("beeperfilter", 1) != 0);
	strncpy (m_Speaker, m_Options.GetAppOptionString ("speaker", "auto"), sizeof m_Speaker - 1);
	m_Speaker[sizeof m_Speaker - 1] = '\0';
	ApplySpeaker ();

	// Cursores del PC: both (por defecto), keys o joystick
	const char *pCursors = m_Options.GetAppOptionString ("cursors", "both");
	m_Keyboard.SetCursorMode (strcasecmp (pCursors, "keys") == 0 ? CursorKeysOnly
				  : (strcasecmp (pCursors, "joystick") == 0 ? CursorJoystickOnly : CursorBoth));

	// Teclado Issue 2/3 y velocidad inicial
	m_pSpectrum->SetIssue2 (m_Options.GetAppOptionDecimal ("issue", 3) == 2);
	unsigned nSpeed = m_Options.GetAppOptionDecimal ("speed", 1);
	for (unsigned i = 0; i < sizeof Speeds / sizeof Speeds[0]; i++)
	{
		if (Speeds[i] == nSpeed)
		{
			m_nSpeed = nSpeed;
		}
	}

	// ULAplus (64 colores); ulaplus=0 lo desactiva
	m_pSpectrum->SetULAplusEnabled (m_Options.GetAppOptionDecimal ("ulaplus", 1) != 0);
	m_pSpectrum->SetPureBlack (m_Options.GetAppOptionDecimal ("pureblack", 0) != 0);

	// Recreated ZX: recreated=0 (por defecto: teclado normal sin ningún retardo),
	// auto (detección automática) o 1 (todos los teclados USB son Recreated)
	const char *pRecreated = m_Options.GetAppOptionString ("recreated", "auto");
	if (strcasecmp (pRecreated, "auto") == 0)
	{
		m_RecreatedMode = RecreatedAuto;
	}
	else if (strcmp (pRecreated, "1") == 0)
	{
		m_RecreatedMode = RecreatedForced;
	}
	else
	{
		m_RecreatedMode = RecreatedOff;
	}
	for (unsigned i = 0; i < MAX_KEYBOARDS; i++)
	{
		m_Recreated[i].Reset (m_RecreatedMode);
	}

	// Interface 1: if1=auto (se conecta al insertar un cartucho), 1 (siempre), 0 (nunca)
	const char *pIF1 = m_Options.GetAppOptionString ("if1", "auto");
	m_nIF1Mode = strcmp (pIF1, "0") == 0 ? 0 : (strcmp (pIF1, "1") == 0 ? 1 : 2);
	m_pSpectrum->SetIF1Connected (m_nIF1Mode == 1);

	// RS232 y ZX Net del Interface 1 (rs232=0 / zxnet=0 los desactivan)
	m_IF1Link.Setup (m_Options.GetAppOptionDecimal ("rs232", 1) != 0,
			 m_Options.GetAppOptionDecimal ("zxnet", 1) != 0);
	m_pSpectrum->GetInterface1 ()->GetZXNet ()->GetSocket ()->SetTransport (&m_IF1Link);

	// Ampliación de memoria LEC (48K): lec=1 la conecta
	if (m_Options.GetAppOptionDecimal ("lec", 0) != 0)
	{
		m_pSpectrum->SetLECConnected (TRUE);
	}

	// Formatos de grabación
	m_bSaveTZX = strcasecmp (m_Options.GetAppOptionString ("savefmt", "tap"), "tzx") == 0;
	const char *pSnapFormat = m_Options.GetAppOptionString ("snapformat", "z80");
	m_nSnapFormat = strcasecmp (pSnapFormat, "sna") == 0 ? 1 : (strcasecmp (pSnapFormat, "szx") == 0 ? 2 : 0);

	// Grabación con SAVE en la carpeta "saves" de la SD; savetrap=0 la desactiva
	if (m_bSDMounted && m_Options.GetAppOptionDecimal ("savetrap", 1) != 0)
	{
		m_pSpectrum->SetSaveHandler (this);
	}

	// Multiface: ROMs desde la SD; multiface=0 lo desactiva
	m_pSpectrum->SetMultifaceEnabled (m_Options.GetAppOptionDecimal ("multiface", 1) != 0);
	LoadMultifaceROMs ();

	// Teclado real por GPIO (montaje de pastbytes): gpiokbd=48 (o 1, 128) para
	// los de 8 x 5 (48K, Spectrum+, 128, +2 gris), plus3 (o plus2a) para el del
	// +2A/+3 (13 x 11) y 0 para no usarlo
	const char *pGPIOKbd = m_Options.GetAppOptionString ("gpiokbd", "48");
	if (strcmp (pGPIOKbd, "0") != 0 && strcasecmp (pGPIOKbd, "off") != 0)
	{
		boolean bPlus3 =    strcasecmp (pGPIOKbd, "plus3") == 0 || strcasecmp (pGPIOKbd, "plus2a") == 0
				 || strcasecmp (pGPIOKbd, "+3") == 0 || strcasecmp (pGPIOKbd, "+2a") == 0;
		m_bGPIOKeyboard = m_GPIOKeyboard.Initialize (bPlus3 ? GPIOKeyboardPlus3 : GPIOKeyboard48);
	}

	const char *pJoy = m_Options.GetAppOptionString ("joystick", "kempston");
	for (unsigned i = 0; i < JoystickModeCount; i++)
	{
		if (strcasecmp (pJoy, JoyOptions[i]) == 0)
		{
			m_Keyboard.SetJoystickMode ((TJoystickMode) i);
		}
	}

	LoadSettings ();
}

boolean CKernel::InitSound (const char *pDevice)
{
	m_bUSBSound = strcmp (pDevice, "sndusb") == 0;
	if (m_bUSBSound)
	{
#if RASPPI >= 4
		// tarjeta de sonido USB; con ear=usb, la misma tarjeta da la entrada
		m_pSound = new CUSBSoundBaseDevice (AUDIO_SAMPLE_RATE,
						    m_bEarUSB ? CUSBSoundBaseDevice::DeviceModeTXRX
							      : CUSBSoundBaseDevice::DeviceModeTXOnly);
		m_pSoundName = "USB";
#else
		// Circle solo tiene sonido USB en la Pi 4/400 (controlador xHCI)
		m_Logger.Write (FromKernel, LogWarning, "sndusb: solo en la Pi 4/400");
		m_bUSBSound = FALSE;
		return FALSE;
#endif
	}
	else if (strcmp (pDevice, "sndpwm") == 0)
	{
#if RASPPI >= 5
		// La Pi 5 no tiene jack: Circle saca el PWM por el GPIO12 y el GPIO13
		// del conector, que son líneas del teclado por GPIO (el 13 es una
		// columna: con el PWM sonando parecían teclas pulsándose sin parar)
		if (m_bGPIOKeyboard)
		{
			m_Logger.Write (FromKernel, LogWarning,
					"sndpwm: GPIO12/13 are used by the GPIO keyboard (gpiokbd=0 to use PWM)");
			return FALSE;
		}
		m_pSound = new CPWMSoundBaseDevice (&m_Interrupt, AUDIO_SAMPLE_RATE, SOUND_CHUNK_SIZE);
		m_pSoundName = N_("PWM (GPIO12 y GPIO13)");
#else
		m_pSound = new CPWMSoundBaseDevice (&m_Interrupt, AUDIO_SAMPLE_RATE, SOUND_CHUNK_SIZE);
		m_pSoundName = N_("PWM (jack 3,5 mm)");
#endif
	}
	else
	{
		m_pSound = new CHDMISoundBaseDevice (&m_Interrupt, AUDIO_SAMPLE_RATE, SOUND_CHUNK_SIZE);
		m_pSoundName = "HDMI";
	}

	if (   m_pSound == 0
	    || !m_pSound->AllocateQueue (SOUND_QUEUE_MS))
	{
		delete m_pSound;
		m_pSound = 0;
		return FALSE;
	}

	m_pSound->SetWriteFormat (SoundFormatSigned16, 2);

	// Un poco de silencio inicial para no quedarse sin datos al arrancar
	static s16 Silence[SOUND_TARGET_FRAMES * 2];
	memset (Silence, 0, sizeof Silence);
	m_pSound->Write (Silence, sizeof Silence);

	if (!m_pSound->Start ())
	{
		delete m_pSound;
		m_pSound = 0;
		return FALSE;
	}

	// Con PWM, la señal se copia también al GPIO12 (pin 32) para sacar el audio
	// por el conector (Pi Zero sin jack, carcasas...). El GPIO12 es el canal 1 del
	// PWM0; como en unos modelos es el izquierdo y en otros el derecho, el sonido
	// pasa a ser mono. En la Pi 4 el audio usa el PWM1, que no llega al conector.
	if (   strcmp (pDevice, "sndpwm") == 0
	    && m_Options.GetAppOptionDecimal ("gpioaudio", 1) != 0
	    && !(m_bGPIOKeyboard && m_GPIOKeyboard.GetType () == GPIOKeyboardPlus3))	// usa el GPIO12
	{
#if RASPPI <= 3
		m_pAudioGPIO = new CGPIOPin (12, GPIOModeAlternateFunction0);
		m_bMonoAudio = TRUE;
		m_pSoundName = N_("PWM (jack y GPIO12, mono)");
#else
		m_Logger.Write (FromKernel, LogWarning, "GPIO12 audio not available on this model");
#endif
	}

	return TRUE;
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6],
				   void *pArg)
{
	if (s_pThis == 0)
	{
		return;
	}

	s_pThis->m_LastReport[0] = ucModifiers;
	for (unsigned i = 0; i < 6; i++)
	{
		s_pThis->m_LastReport[1 + i] = RawKeys[i];
	}
	// Teclas para el Spectrum (letras, números, ENTER, espacio, símbolos y
	// cursores; no las de función del emulador ni con un menú abierto): ver
	// IsTyping
	if (!s_pThis->m_Keyboard.IsCaptured ())
	{
		for (unsigned i = 0; i < 6; i++)
		{
			u8 k = RawKeys[i];
			if ((k >= 0x04 && k <= 0x38) || (k >= 0x4F && k <= 0x52))
			{
				s_pThis->m_nLastKeyTime = CTimer::GetClockTicks64 ();
				break;
			}
		}
	}

	unsigned nIndex = (unsigned) (uintptr) pArg;
	if (nIndex >= MAX_KEYBOARDS)
	{
		s_pThis->m_Keyboard.UpdateFromReport (ucModifiers, RawKeys);
		return;
	}

	// El detector del Recreated ZX decide qué informes llegan como teclado normal
	CRecreatedKeyboard::TOutput Out;
	s_pThis->m_Recreated[nIndex].ProcessReport (ucModifiers, RawKeys, CTimer::GetClockTicks (), &Out);
	for (unsigned i = 0; i < Out.nCount; i++)
	{
		s_pThis->m_Keyboard.UpdateFromReport (Out.Modifiers[i], Out.Keys[i]);
	}
}

void CKernel::KeyboardRemovedHandler (CDevice *pDevice, void *pContext)
{
	if (s_pThis == 0)
	{
		return;
	}

	unsigned nIndex = (unsigned) (uintptr) pContext;
	if (nIndex < MAX_KEYBOARDS)
	{
		s_pThis->m_pKeyboards[nIndex] = 0;
		s_pThis->m_Recreated[nIndex].Reset (s_pThis->m_RecreatedMode);
	}

	s_pThis->m_Keyboard.ReleaseAll ();
}

void CKernel::UpdateUSB (void)
{
	if (!m_USBHCI.UpdatePlugAndPlay ())
	{
		return;
	}

	for (unsigned i = 0; i < MAX_KEYBOARDS; i++)
	{
		if (m_pKeyboards[i] != 0)
		{
			continue;
		}

		CString Name;
		Name.Format ("ukbd%u", i + 1);

		CUSBKeyboardDevice *pKeyboard =
			(CUSBKeyboardDevice *) m_DeviceNameService.GetDevice (Name, FALSE);
		if (pKeyboard != 0)
		{
			pKeyboard->RegisterRemovedHandler (KeyboardRemovedHandler, (void *) (uintptr) i);
			m_Recreated[i].Reset (m_RecreatedMode);

			// El Recreated ZX Spectrum por cable USB se reconoce por su
			// VID:PID (0F39:0404, TG3 Electronics; dato de José Luis Sánchez)
			const TUSBDeviceDescriptor *pDesc = pKeyboard->GetDevice ()->GetDeviceDescriptor ();
			if (pDesc != 0 && pDesc->idVendor == RECREATED_VID && pDesc->idProduct == RECREATED_PID)
			{
				m_Recreated[i].ActivateByID ();
			}
			pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw, FALSE, (void *) (uintptr) i);
			m_pKeyboards[i] = pKeyboard;

			m_Logger.Write (FromKernel, LogNotice, "Keyboard %s attached", (const char *) Name);
		}
	}

	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		if (m_pGamePads[i] != 0)
		{
			continue;
		}

		CUSBGamePadDevice *pGamePad =
			(CUSBGamePadDevice *) m_DeviceNameService.GetDevice ("upad", i + 1, FALSE);
		if (pGamePad == 0)
		{
			continue;
		}

		m_GamePadProps[i] = pGamePad->GetProperties ();
		m_PrevPadButtons[i] = 0;
		m_PrevPadJoy[i] = 0;
		m_pGamePads[i] = pGamePad;

		pGamePad->RegisterRemovedHandler (GamePadRemovedHandler, (void *) (uintptr) i);
		pGamePad->RegisterStatusHandler (GamePadStatusHandler);

		if (m_GamePadProps[i] & GamePadPropertyHasLED)
		{
			pGamePad->SetLEDMode ((TGamePadLEDMode) (GamePadLEDModeOn1 + i));
		}

		const TGamePadState *pState = pGamePad->GetInitialState ();
		m_Logger.Write (FromKernel, LogNotice, "Gamepad %u attached (%s mapping, %d buttons, %d axes, %d hats)",
				i + 1, (m_GamePadProps[i] & GamePadPropertyIsKnown) ? "known" : "generic",
				pState->nbuttons, pState->naxes, pState->nhats);

		CString Message;
		Message.Format (T ("Mando %u conectado"), i + 1);
		ShowMessage (Message);
	}

	if (m_pMouse == 0)
	{
		CMouseDevice *pMouse = (CMouseDevice *) m_DeviceNameService.GetDevice ("mouse", 1, FALSE);
		if (pMouse != 0)
		{
			m_pMouse = pMouse;
			pMouse->RegisterRemovedHandler (MouseRemovedHandler, 0);
			pMouse->RegisterStatusHandler (MouseStatusHandler);
			m_Keyboard.SetMousePresent (TRUE);

			m_Logger.Write (FromKernel, LogNotice, "Mouse attached");
			ShowMessage (m_Keyboard.GetJoystickMode () == JoystickKempston
				     ? T ("Ratón conectado (inactivo con joystick Kempston)")
				     : T ("Ratón Kempston conectado"));
		}
	}
}

void CKernel::MouseStatusHandler (unsigned nButtons, int nDX, int nDY, int nWheel)
{
	if (s_pThis != 0)
	{
		s_pThis->m_Keyboard.MouseUpdate (nButtons, nDX, nDY, nWheel);
	}
}

void CKernel::MouseRemovedHandler (CDevice *pDevice, void *pContext)
{
	if (s_pThis != 0)
	{
		s_pThis->m_pMouse = 0;
		s_pThis->m_Keyboard.SetMousePresent (FALSE);
	}
}

// Fuera del centro más de un 40 % del recorrido
static int AxisDirection (const TGamePadState *pState, int nAxis)
{
	if (nAxis >= pState->naxes)
	{
		return 0;
	}

	int nMin = pState->axes[nAxis].minimum;
	int nMax = pState->axes[nAxis].maximum;
	if (nMax <= nMin)
	{
		return 0;
	}

	int nCenter = (nMin + nMax) / 2;
	int nThreshold = (nMax - nMin) * 2 / 10;	// 40 % de medio recorrido
	int nValue = pState->axes[nAxis].value;

	if (nValue < nCenter - nThreshold)	return -1;
	if (nValue > nCenter + nThreshold)	return 1;
	return 0;
}

void CKernel::GamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pState)
{
	if (s_pThis == 0 || nDeviceIndex >= MAX_GAMEPADS || pState == 0)
	{
		return;
	}

	CKernel *pThis = s_pThis;
	unsigned nButtons = pState->buttons;
	u8 nJoy = 0;
	boolean bSelect, bStart;

	if (pThis->m_GamePadProps[nDeviceIndex] & GamePadPropertyIsKnown)
	{
		// PS3/PS4, Xbox 360/One, Switch Pro, 8BitDo...: mapeo conocido.
		// Como en ZXBaremulator: A/Cruz, Y/Triángulo, L1 y R1 = disparo;
		// B/Círculo, X/Cuadrado, L2 y R2 = disparo 2
		if (nButtons & GamePadButtonUp)		nJoy |= JOY_UP;
		if (nButtons & GamePadButtonDown)	nJoy |= JOY_DOWN;
		if (nButtons & GamePadButtonLeft)	nJoy |= JOY_LEFT;
		if (nButtons & GamePadButtonRight)	nJoy |= JOY_RIGHT;

		if (nButtons & (GamePadButtonA | GamePadButtonY | GamePadButtonLB | GamePadButtonRB))
		{
			nJoy |= JOY_FIRE;
		}
		if (nButtons & (GamePadButtonB | GamePadButtonX | GamePadButtonLT | GamePadButtonRT))
		{
			nJoy |= JOY_FIRE2;
		}

		int nX = AxisDirection (pState, GamePadAxisLeftX);
		int nY = AxisDirection (pState, GamePadAxisLeftY);
		if (nX < 0) nJoy |= JOY_LEFT;
		if (nX > 0) nJoy |= JOY_RIGHT;
		if (nY < 0) nJoy |= JOY_UP;
		if (nY > 0) nJoy |= JOY_DOWN;

		bSelect = (nButtons & (GamePadButtonSelect | GamePadButtonMinus)) != 0;
		bStart = (nButtons & (GamePadButtonStart | GamePadButtonPlus)) != 0;
	}
	else
	{
		// Mando HID genérico (o adaptadores Mayflash en modo DirectInput):
		// cruceta por el primer "hat" y/o los dos primeros ejes
		if (pState->nhats > 0)
		{
			int nHat = pState->hats[0];	// 0 = arriba, en sentido horario; fuera de rango = centro
			if (nHat == 7 || nHat == 0 || nHat == 1)	nJoy |= JOY_UP;
			if (nHat >= 1 && nHat <= 3)			nJoy |= JOY_RIGHT;
			if (nHat >= 3 && nHat <= 5)			nJoy |= JOY_DOWN;
			if (nHat >= 5 && nHat <= 7)			nJoy |= JOY_LEFT;
		}

		int nX = AxisDirection (pState, 0);
		int nY = AxisDirection (pState, 1);
		if (nX < 0) nJoy |= JOY_LEFT;
		if (nX > 0) nJoy |= JOY_RIGHT;
		if (nY < 0) nJoy |= JOY_UP;
		if (nY > 0) nJoy |= JOY_DOWN;

		if (nButtons & (BIT (0) | BIT (3) | BIT (4) | BIT (5)))	nJoy |= JOY_FIRE;
		if (nButtons & (BIT (1) | BIT (2) | BIT (6) | BIT (7)))	nJoy |= JOY_FIRE2;

		bSelect = (nButtons & BIT (8)) != 0;
		bStart = (nButtons & BIT (9)) != 0;
	}

	pThis->m_Keyboard.SetGamePadState (nDeviceIndex, nJoy);

	// Pulsaciones nuevas
	unsigned nOldButtons = pThis->m_PrevPadButtons[nDeviceIndex];
	u8 nOldJoy = pThis->m_PrevPadJoy[nDeviceIndex];
	boolean bOldSelect = (nOldButtons & 0x80000000) != 0;
	boolean bOldStart = (nOldButtons & 0x40000000) != 0;

	if (bSelect && !bOldSelect)
	{
		pThis->m_Keyboard.InjectKeyEvent (KEY_PAD_SELECT, 0);
	}

	if (pThis->m_Keyboard.IsCaptured ())
	{
		// En los menús: cruceta = cursores, disparo = ENTER, disparo 2 = ESC
		u8 nNew = nJoy & ~nOldJoy;
		if (nNew & JOY_UP)	pThis->m_Keyboard.InjectKeyEvent (HID_KEY_UP, 0);
		if (nNew & JOY_DOWN)	pThis->m_Keyboard.InjectKeyEvent (HID_KEY_DOWN, 0);
		if (nNew & JOY_LEFT)	pThis->m_Keyboard.InjectKeyEvent (HID_KEY_LEFT, 0);
		if (nNew & JOY_RIGHT)	pThis->m_Keyboard.InjectKeyEvent (HID_KEY_RIGHT, 0);
		if (nNew & JOY_FIRE)	pThis->m_Keyboard.InjectKeyEvent (HID_KEY_ENTER, 0);
		if (nNew & JOY_FIRE2)	pThis->m_Keyboard.InjectKeyEvent (HID_KEY_ESC, 0);
	}
	else if (bStart && !bOldStart)
	{
		pThis->m_Keyboard.InjectKeyEvent (HID_KEY_F1, 0);	// Start: ayuda y selector de cintas
	}

	pThis->m_PrevPadButtons[nDeviceIndex] = (bSelect ? 0x80000000 : 0) | (bStart ? 0x40000000 : 0);
	pThis->m_PrevPadJoy[nDeviceIndex] = nJoy;
}

void CKernel::GamePadRemovedHandler (CDevice *pDevice, void *pContext)
{
	if (s_pThis == 0)
	{
		return;
	}

	unsigned nIndex = (unsigned) (uintptr) pContext;
	if (nIndex < MAX_GAMEPADS)
	{
		s_pThis->m_pGamePads[nIndex] = 0;
		s_pThis->m_Keyboard.SetGamePadState (nIndex, 0);
	}
}

void CKernel::ShowMessage (const char *pMessage)
{
	strncpy (m_Message, pMessage, sizeof m_Message - 1);
	m_Message[sizeof m_Message - 1] = '\0';
	m_nMessageFrames = MESSAGE_FRAMES;
}

void CKernel::DrawMessage (TPixel *pScreen)
{
	DrawIndicators (pScreen);

	if (m_nMessageFrames == 0 && !m_bPaused)
	{
		return;
	}

	const char *pText = m_bPaused ? T ("PAUSA - Pausa o Alt+P para seguir") : m_Message;

	const TVisibleRect &rVisible = m_Video.GetVisibleRect ();
	int nLen = strlen (pText);
	int nMaxLen = rVisible.nWidth / 8 - 1;
	if (nLen > nMaxLen)
	{
		nLen = nMaxLen;
	}
	int x = rVisible.x + (rVisible.nWidth - nLen * 8) / 2;
	int y = rVisible.y + rVisible.nHeight - 20;

	m_Video.FillRect (pScreen, x - 4, y - 3, nLen * 8 + 8, 14, COLOR_UI_PANEL);
	m_Video.DrawText (pScreen, x, y, pText, COLOR_UI_TITLE, COLOR_UI_PANEL, nLen);
}

void CKernel::UpdateStatusLines (void)
{

	CString Line;

	unsigned nHelpLine = 0;
	for (unsigned i = 0; i < sizeof KeyBindings / sizeof KeyBindings[0]; i++)
	{
		if (KeyBindings[i].pDescription == 0)
		{
			continue;
		}
		Line.Format ("%-7s %s", KeyBindings[i].pKeyName, T (KeyBindings[i].pDescription));
		m_Menu.SetHelpLine (nHelpLine++, Line);
	}

	Line.Format (T ("Modelo: %s%s  Vel. x%u"), m_pSpectrum->GetTiming ()->pName,
		     (m_pSpectrum->IsIssue2 () && m_pSpectrum->GetTiming ()->CodeModel == CodeModel48K)
		     ? (m_pSpectrum->IsLECConnected () ? " +LEC" : " (Issue 2)")
		     : (m_pSpectrum->IsLECConnected () && m_pSpectrum->GetModel () == Model48K ? " +LEC" : ""),
		     m_nSpeed);
	m_Menu.SetStatusLine (0, Line);

	Line.Format (T ("Joystick: %s%s  %u.%02u c/s"), T (JoystickNames[m_Keyboard.GetJoystickMode ()]),
		     m_Keyboard.IsMouseActive () ? T (" +ratón") : "", m_nFPSx100 / 100, m_nFPSx100 % 100);
	m_Menu.SetStatusLine (1, Line);

	if (m_pSpectrum->IsIF1Connected ())
	{
		Line.Format (T ("Carga: %s  Microdrives: %u"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]),
			     CountMicrodrives ());
	}
	else if (m_pSpectrum->HasDiskDrive ())
	{
		CPlus3Disk *pDisk = m_pSpectrum->GetDisk ();
		Line.Format (T ("Carga: %s  Discos A:%s B:%s%s"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]),
			     pDisk->HasDisk (0) ? T ("sí") : "-", pDisk->HasDisk (1) ? T ("sí") : "-",
			     m_pSpectrum->HasZXMMC () && m_HardDisk.IsOpen () ? T (" HD:sí") : "");
	}
	else if (m_pEarSource != 0)
	{
		Line.Format (T ("Carga: %s  Casete: %s"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]),
			     m_pSpectrum->IsEarInputActive () ? T ("con señal") : T ("sin señal"));
	}
	else
	{
		Line.Format (T ("Carga: %s"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]));
	}
	m_Menu.SetStatusLine (2, Line);

	if (m_Tape.IsInserted ())
	{
		Line.Format (T ("Cinta: %s"), m_Tape.GetName ());
		m_Menu.SetStatusLine (3, Line);
		Line.Format (T ("Bloque %u de %u%s%s"), m_Tape.GetBlock () + 1 > m_Tape.GetBlockCount ()
						? m_Tape.GetBlockCount () : m_Tape.GetBlock () + 1,
			     m_Tape.GetBlockCount (), m_Tape.IsPlaying () ? T (" (en marcha)") : "",
			     m_Tape.GetUnsupportedBlocks () > 0 ? T (" *no soportados") : "");
		m_Menu.SetStatusLine (4, Line);
	}
	else
	{
		m_Menu.SetStatusLine (3, T ("Cinta: (ninguna)"));
		m_Menu.SetStatusLine (4, "");
	}

	Line.Format (T ("Sonido: %s, AY %s"), T (m_pSoundName),
		     AYStereoNames[m_pSpectrum->GetAY ()->GetStereoMode ()]);
	m_Menu.SetStatusLine (5, Line);

	if (m_pSpectrum->IsCartridgeInserted ())
	{
		Line.Format (T ("Cartucho: %s"), m_CartridgeName[0] != '\0' ? m_CartridgeName : T ("(snapshot)"));
	}
	else
	{
		if (m_pSpectrum->IsSpectranetEnabled ())
		{
			Line.Format (T ("Spectranet: sí  ULAplus: %s"), m_pSpectrum->IsULAplusActive () ? T ("activo") : T ("no"));
		}
		else if (m_pSpectrum->IsDivMMCActive ())
		{
			Line.Format (T ("divMMC: esxDOS  ULAplus: %s"), m_pSpectrum->IsULAplusActive () ? T ("activo") : T ("no"));
		}
		else
		{
			Line.Format (T ("Multiface: %s  ULAplus: %s"),
				     m_pSpectrum->IsMultifaceAvailable () ? T ("sí") : T ("no"),
				     m_pSpectrum->IsULAplusActive () ? T ("activo") : T ("no"));
		}
	}
	m_Menu.SetStatusLine (6, Line);

	Line.Format (T ("Borde: %s, scanlines: %s"), T (CVideo::GetBorderName (m_BorderSize)),
		     m_bScanlines ? T ("sí") : T ("no"));
	m_Menu.SetStatusLine (7, Line);

	Line.Format (T ("Red: %s"), m_Network.GetStateText ());
	m_Menu.SetStatusLine (8, Line);
}

boolean CKernel::LoadFile (const char *pPath, u8 *pBuffer, unsigned nSize)
{
	FIL File;
	if (f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		return FALSE;
	}

	unsigned nRead = 0;
	boolean bOK =    f_size (&File) == nSize
		      && f_read (&File, pBuffer, nSize, &nRead) == FR_OK
		      && nRead == nSize;
	f_close (&File);

	return bOK;
}

// Las ROMs del Multiface (de Romantic Robot, tomadas de JSpeccy) se cargan de la
// carpeta "roms" de la tarjeta SD (8192 bytes cada una).
void CKernel::LoadMultifaceROMs (void)
{
	if (!m_bSDMounted)
	{
		return;
	}

	static const char *ROMFiles[3][2] =
	{
		{"SD:/roms/mf1.rom",     0},				// Multiface One (48K)
		{"SD:/roms/mf128.rom",   0},				// Multiface 128 (128K)
		{"SD:/roms/mfplus3.rom", "SD:/roms/mf3.rom"},		// Multiface 3 (+2A/+3)
	};

	static u8 Buffer[0x2000];
	for (unsigned i = 0; i < 3; i++)
	{
		boolean bLoaded = FALSE;
		for (unsigned j = 0; j < 2 && !bLoaded; j++)
		{
			if (ROMFiles[i][j] != 0 && LoadFile (ROMFiles[i][j], Buffer, sizeof Buffer))
			{
				m_pSpectrum->SetMultifaceROM ((TCodeModel) i, Buffer);
				m_Logger.Write (FromKernel, LogNotice, "Multiface ROM %s loaded", ROMFiles[i][j]);
				bLoaded = TRUE;
			}
		}
	}
}

// SAVE: los bloques se añaden a la cinta de grabaciones actual (un .TAP en la
// carpeta "saves"). Si la cinta actual no es de grabaciones, se crea un fichero
// nuevo con el nombre de la cabecera y pasa a ser la cinta actual. Así nunca se
// modifican las cintas de los juegos.
boolean CKernel::SaveTapeBlock (const u8 *pBlock, unsigned nLength)
{
	if (!m_bSDMounted || nLength > 0xFFFF)
	{
		return FALSE;
	}

	if (!m_bSaveTape || !m_Tape.IsInserted ())
	{
		// Nombre a partir de la cabecera (bloque de 19 bytes con flag 0)
		char Name[16];
		unsigned n = 0;
		if (pBlock[0] == 0x00 && nLength == 19)
		{
			for (unsigned i = 2; i < 12; i++)
			{
				char ch = (char) pBlock[i];
				if (   (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
				    || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_')
				{
					Name[n++] = ch;
				}
				else if (ch == ' ' || ch == '.')
				{
					Name[n++] = '_';
				}
			}
			while (n > 0 && Name[n - 1] == '_')
			{
				n--;
			}
		}
		if (n == 0)
		{
			strcpy (Name, "save");
			n = 4;
		}
		Name[n] = '\0';

		f_mkdir ("SD:/saves");

		CString Path;
		FILINFO Info;
		const char *pExt = m_bSaveTZX ? "tzx" : "tap";
		Path.Format ("SD:/saves/%s.%s", Name, pExt);
		for (unsigned nTry = 2; f_stat (Path, &Info) == FR_OK; nTry++)
		{
			if (nTry > 999)
			{
				return FALSE;
			}
			Path.Format ("SD:/saves/%s_%u.%s", Name, nTry, pExt);
		}

		strncpy (m_TapePath, Path, sizeof m_TapePath - 1);
		m_TapePath[sizeof m_TapePath - 1] = '\0';
		m_bSaveTape = TRUE;

		m_Tape.Eject ();

		// un .TZX nuevo empieza con su cabecera
		if (m_bSaveTZX && !WriteFile (m_TapePath, (const u8 *) "ZXTape!\x1A\x01\x14", 10, FALSE))
		{
			ShowMessage (T ("Error al grabar en la SD"));
			return FALSE;
		}
	}

	boolean bTZXFile = strcasecmp (m_TapePath + strlen (m_TapePath) - 4, ".tzx") == 0;

	// .TAP: longitud + bloque; .TZX: bloque 0x10 (pausa de 1 s) + longitud + bloque
	u8 Header[5];
	unsigned nHeader;
	if (bTZXFile)
	{
		Header[0] = 0x10;
		Header[1] = 1000 & 0xFF;
		Header[2] = 1000 >> 8;
		Header[3] = nLength & 0xFF;
		Header[4] = nLength >> 8;
		nHeader = 5;
	}
	else
	{
		Header[0] = nLength & 0xFF;
		Header[1] = nLength >> 8;
		nHeader = 2;
	}

	if (   !WriteFile (m_TapePath, Header, nHeader, TRUE)
	    || !WriteFile (m_TapePath, pBlock, nLength, TRUE))
	{
		ShowMessage (T ("Error al grabar en la SD"));
		return FALSE;
	}

	const char *pName = m_TapePath;
	for (const char *p = m_TapePath; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pName = p + 1;
		}
	}

	if (bTZXFile)
	{
		m_Tape.AppendTZXBlock (pBlock, nLength, pName);
	}
	else
	{
		m_Tape.AppendTAPBlock (pBlock, nLength, pName);
	}

	CString Message;
	Message.Format (T ("Grabado en saves/%s"), pName);
	ShowMessage (Message);

	return TRUE;
}

// "a.b.c.d" -> 4 bytes; FALSE si no es una dirección válida
static boolean ParseIP (const char *pText, u8 *pIP)
{
	unsigned nPart = 0, nValue = 0, nDigits = 0;
	for (const char *p = pText; ; p++)
	{
		if (*p >= '0' && *p <= '9')
		{
			nValue = nValue * 10 + (*p - '0');
			if (++nDigits > 3 || nValue > 255)
			{
				return FALSE;
			}
		}
		else if ((*p == '.' || *p == '\0') && nDigits > 0 && nPart < 4)
		{
			pIP[nPart++] = nValue;
			nValue = nDigits = 0;
			if (*p == '\0')
			{
				return nPart == 4;
			}
		}
		else
		{
			return FALSE;
		}
	}
}

// Entrada de cinta (casete real): ear=gpio (comparador en earpin=, 20 por
// defecto; earinvert=1 si la señal llega invertida) o ear=usb (entrada de una
// tarjeta de sonido USB); ear=0, sin entrada (por defecto)
void CKernel::StartEarInput (void)
{
	const char *pEar = m_Options.GetAppOptionString ("ear", "0");

	if (strcasecmp (pEar, "gpio") == 0)
	{
		// Con el teclado del +3 todos los GPIO están ocupados salvo el 14 y el 15
		// (el puerto serie): se usa el GPIO15 (pin 10), que es la entrada RX del
		// puerto serie. El registro por el puerto serie (logdev=ttyS1) solo usa
		// la salida TX (GPIO14), así que sigue funcionando.
		boolean bPlus3Kbd = m_bGPIOKeyboard && m_GPIOKeyboard.GetType () == GPIOKeyboardPlus3;
		unsigned nPin = m_Options.GetAppOptionDecimal ("earpin", bPlus3Kbd ? 15 : 20);
		boolean bInvert = m_Options.GetAppOptionDecimal ("earinvert", 0) != 0;

		// Pines que ya están en uso
		static const unsigned Keyboard48[] = {18, 23, 24, 25, 22, 27, 17, 4, 26, 19, 13, 6, 5, 21, 16};
		static const unsigned Plus3Extra[] = {20, 21, 16, 12, 7, 8, 9, 10, 11, 2, 3};
		boolean bBusy = nPin < 2 || nPin > 27 || nPin == 14 || (nPin == 12 && m_pAudioGPIO != 0);
		if (m_bGPIOKeyboard)
		{
			for (unsigned i = 0; i < sizeof Keyboard48 / sizeof Keyboard48[0]; i++)
			{
				bBusy = bBusy || nPin == Keyboard48[i];
			}
			for (unsigned i = 0; bPlus3Kbd && i < sizeof Plus3Extra / sizeof Plus3Extra[0]; i++)
			{
				bBusy = bBusy || nPin == Plus3Extra[i];
			}
		}

		if (bBusy)
		{
			m_Logger.Write (FromKernel, LogWarning, "ear=gpio: el GPIO%u esta en uso", nPin);
			CString Message;
			Message.Format (T ("earpin=%u: ese GPIO ya se usa"), nPin);
			ShowMessage (Message);
			return;
		}

		CGPIOEarSource *pSource = new CGPIOEarSource (nPin, bInvert, &m_GPIOManager);
		if (pSource != 0 && pSource->Initialize ())
		{
			m_pEarSource = pSource;
		}
		else
		{
			delete pSource;
		}
	}
	else if (strcasecmp (pEar, "usb") == 0)
	{
#if RASPPI < 4
		// Circle solo tiene audio USB en la Pi 4/400
		ShowMessage (T ("ear=usb: solo en la Pi 4/400 (usa ear=gpio)"));
		m_Logger.Write (FromKernel, LogWarning, "ear=usb: solo en la Pi 4/400");
#else
		CSoundBaseDevice *pDevice = m_bUSBSound ? m_pSound : 0;
		if (pDevice == 0)
		{
			// la tarjeta USB solo para la entrada (el sonido sale por HDMI o jack)
			m_pUSBInput = new CUSBSoundBaseDevice (48000, CUSBSoundBaseDevice::DeviceModeRXOnly);
			pDevice = m_pUSBInput;
		}

		CUSBEarSource *pSource = new CUSBEarSource (pDevice);
		if (   pSource != 0 && pSource->Initialize ()
		    && (pDevice != m_pUSBInput || m_pUSBInput->Start ()))
		{
			m_pEarSource = pSource;
		}
		else
		{
			delete pSource;
			m_Logger.Write (FromKernel, LogWarning, "ear=usb: no hay tarjeta de sonido USB");
			ShowMessage (T ("ear=usb: no se encuentra la tarjeta de sonido USB"));
		}
#endif
	}

	if (m_pEarSource != 0)
	{
		m_EarInput.SetEnabled (TRUE);
		m_pSpectrum->SetEarInput (&m_EarInput);
		m_Logger.Write (FromKernel, LogNotice, "Entrada de cinta: %s", pEar);
	}
}

// Red: net=auto (WiFi si hay wpa_supplicant.conf, si no cable), wifi, eth o 0;
// tz= diferencia horaria en minutos (60 = España peninsular); dst=1 horario de verano europeo
void CKernel::StartNetwork (void)
{
	if (!m_bSDMounted)
	{
		return;
	}

	const char *pMode = m_Options.GetAppOptionString ("net", "auto");
	TNetMode Mode = NetModeAuto;
	if (strcmp (pMode, "0") == 0 || strcasecmp (pMode, "off") == 0)	Mode = NetModeOff;
	else if (strcasecmp (pMode, "wifi") == 0)				Mode = NetModeWiFi;
	else if (strcasecmp (pMode, "eth") == 0)				Mode = NetModeEthernet;

	// tz admite signo (por ejemplo tz=-300 para Nueva York)
	const char *pTZ = m_Options.GetAppOptionString ("tz", "60");
	int nSign = 1;
	if (*pTZ == '-' || *pTZ == '+')
	{
		nSign = *pTZ == '-' ? -1 : 1;
		pTZ++;
	}
	int nTimeZone = 0;
	while (*pTZ >= '0' && *pTZ <= '9')
	{
		nTimeZone = nTimeZone * 10 + (*pTZ++ - '0');
	}
	nTimeZone *= nSign;
	boolean bDST = m_Options.GetAppOptionDecimal ("dst", 1) != 0;

	// IP fija opcional: ip=192.168.1.50 mask=255.255.255.0 gw=192.168.1.1 dns=192.168.1.1
	u8 IP[4], Mask[4] = {255, 255, 255, 0}, Gateway[4], DNS[4];
#ifdef ZX_QEMU
	boolean bStatic = ParseIP ("10.0.2.15", IP);	// QEMU: red de usuario (su DHCP no responde)
#else
	boolean bStatic = ParseIP (m_Options.GetAppOptionString ("ip", ""), IP);
#endif
	if (bStatic)
	{
		ParseIP (m_Options.GetAppOptionString ("mask", "255.255.255.0"), Mask);
		memcpy (Gateway, IP, 4);
		Gateway[3] = 1;
		ParseIP (m_Options.GetAppOptionString ("gw", ""), Gateway);
#ifdef ZX_QEMU
		Gateway[3] = 2;
#endif
		memcpy (DNS, Gateway, 4);
		ParseIP (m_Options.GetAppOptionString ("dns", ""), DNS);
	}

	m_Network.SetTLSVerify (m_Options.GetAppOptionDecimal ("tlsverify", 1) != 0);
	// web=1 (completa), ro (solo lectura) o 0 (apagada); webpass= contraseña
	const char *pWeb = m_Options.GetAppOptionString ("web", "1");
	m_Network.SetWebOptions (strcmp (pWeb, "0") == 0 ? 0 : (strcasecmp (pWeb, "ro") == 0 ? 2 : 1),
				 m_Options.GetAppOptionString ("webpass", ""));
	m_Network.Start (Mode, "zxmetalcircle", nTimeZone, bDST,
			 bStatic ? IP : 0, Mask, Gateway, DNS);

	if (   m_Network.IsWiFi ()
	    && CMachineInfo::Get ()->GetMachineModel () == MachineModelZeroW)
	{
		// La Zero W tiene un solo núcleo: la WiFi le quita tiempo a la emulación
		ShowMessage (T ("Zero W: con WiFi puede haber cortes breves"));
	}
}

// Carga cualquier fichero según su extensión (selector y web)
void CKernel::LoadAnyFile (const char *pPath)
{
	if (strncmp (pPath, "NET:", 4) == 0)
	{
		DownloadAndLoad (pPath);
		return;
	}

	size_t nLen = strlen (pPath);
	const char *pExt = nLen > 4 ? pPath + nLen - 4 : "";
	if (   strcasecmp (pExt, ".z80") == 0 || strcasecmp (pExt, ".sna") == 0
	    || strcasecmp (pExt, ".szx") == 0)
	{
		LoadSnapshot (pPath);
	}
	else if (strcasecmp (pExt, ".rom") == 0)
	{
		LoadCartridge (pPath);
	}
	else if (strcasecmp (pExt, ".mdr") == 0 || strcasecmp (pExt, ".mdv") == 0)
	{
		InsertMicrodrive (pPath);
	}
	else if (strcasecmp (pExt, ".dsk") == 0)
	{
		InsertDisk (pPath);
	}
	else if (   strcasecmp (pExt, ".vhd") == 0 || strcasecmp (pExt, ".img") == 0
		 || strcasecmp (pExt, ".raw") == 0 || strcasecmp (pExt, ".hdf") == 0)
	{
		MountHardDisk (pPath);
	}
	else
	{
		LoadTape (pPath);
	}
}

// Spectranet: la flash (firmware y ajustes) está en SD:/spectranet.flash; la
// primera vez se copia de roms/spectranet.rom, que trae el firmware ya
// instalado y configurado con DHCP. No convive con la divMMC (las dos se
// paginan en 0x0008), ni con el Interface 1 ni el Multiface.
#define SNET_FLASH_FILE		"SD:/spectranet.flash"

void CKernel::InitSpectranet (void)
{
#ifndef ZX_TEST_SNET
	if (m_Options.GetAppOptionDecimal ("spectranet", 0) == 0)
	{
		return;
	}
#endif

	if (!LoadSpectranetFlash ())
	{
		m_MissingROMs.Append (m_MissingROMs.GetLength () > 0 ? " Spectranet" : "Spectranet");
		return;
	}

	if (m_pSpectrum->IsDivMMCActive ())
	{
		m_pSpectrum->SetDivMMCEnabled (FALSE);
		m_Logger.Write (FromKernel, LogWarning, "Spectranet: divMMC desactivada");
	}

	m_pSpectrum->SetSpectranetEnabled (TRUE);
	m_Logger.Write (FromKernel, LogNotice, "Spectranet activa");
}

// Carga la flash (la primera vez, o tras haberla desconectado)
boolean CKernel::LoadSpectranetFlash (void)
{
	static boolean bLoaded = FALSE;
	if (bLoaded)
	{
		return TRUE;
	}

	static u8 Flash[SNET_FLASH_SIZE];
	boolean bFromFile = FALSE;
	FIL File;
	UINT nRead = 0;
	if (f_open (&File, SNET_FLASH_FILE, FA_READ | FA_OPEN_EXISTING) == FR_OK)
	{
		bFromFile = f_read (&File, Flash, sizeof Flash, &nRead) == FR_OK && nRead == sizeof Flash;
		f_close (&File);
	}
	if (!bFromFile)
	{
		const u8 *pROM = LoadROMFile (ROMOption ("romsnet", "spectranet.rom"),
					      SNET_FLASH_SIZE);
		if (pROM == 0)
		{
			return FALSE;
		}
		memcpy (Flash, pROM, sizeof Flash);
	}

	CSpectranet *pSnet = m_pSpectrum->GetSpectranet ();
	pSnet->SetNet (&m_SnetNet);
	pSnet->LoadFlash (Flash, sizeof Flash);
	bLoaded = TRUE;
	m_Logger.Write (FromKernel, LogNotice, "Spectranet: flash de %s",
			bFromFile ? SNET_FLASH_FILE : "roms/spectranet.rom");
	return TRUE;
}

// Periféricos que se paginan sobre la ROM (Spectranet, divMMC, memoria LEC) e
// Interface 1: no conviven, así que al conectar uno se desconectan los demás.
// Cada cambio reinicia el Spectrum. Alt+T, Alt+X y Alt+L, o el menú de
// periféricos (P en la ayuda).
void CKernel::ToggleSpectranet (void)
{
	if (m_pSpectrum->IsSpectranetEnabled ())
	{
		if (m_nSnetSaveFrames > 0)
		{
			m_nSnetSaveFrames = 0;
			SaveSpectranetFlash ();
		}
		m_pSpectrum->SetSpectranetEnabled (FALSE);
		m_pSpectrum->Reset ();
		ShowMessage (T ("Spectranet desconectada"));
		SyncPeripheralSettings ();
		return;
	}

	if (!LoadSpectranetFlash ())
	{
		ShowMessage (T ("Falta roms/spectranet.rom (ver LEEME)"));
		SyncPeripheralSettings ();
		return;
	}

	m_pSpectrum->SetDivMMCEnabled (FALSE);
	m_pSpectrum->SetLECConnected (FALSE);
	m_pSpectrum->SetIF1Connected (FALSE);
	m_pSpectrum->SetSpectranetEnabled (TRUE);
	m_pSpectrum->Reset ();
	ShowMessage (CNetworkManager::IsNetRunning () ? T ("Spectranet conectada")
						      : T ("Spectranet conectada (sin red)"));
	SyncPeripheralSettings ();
}

void CKernel::ToggleDivMMC (void)
{
	if (m_pSpectrum->IsDivMMCActive ())
	{
		m_pSpectrum->SetDivMMCEnabled (FALSE);
		m_pSpectrum->Reset ();
		ShowMessage (T ("divMMC desconectada"));
		SyncPeripheralSettings ();
		return;
	}

	if (m_pSpectrum->GetModel () == ModelPlus3E)
	{
		ShowMessage (T ("La divMMC no se usa en el +3e"));
		SyncPeripheralSettings ();
		return;
	}

	const u8 *pESX = LoadROMFile (ROMOption ("romesx", "esxmmc.bin"), 0x2000);
	if (pESX == 0)
	{
		ShowMessage (T ("Falta roms/esxmmc.bin (ver LEEME)"));
		SyncPeripheralSettings ();
		return;
	}

	if (m_pSpectrum->IsSpectranetEnabled ())
	{
		ToggleSpectranet ();		// la desconecta (y guarda su flash)
	}
	m_pSpectrum->SetLECConnected (FALSE);
	m_pSpectrum->SetIF1Connected (FALSE);
	m_pSpectrum->SetDivMMCROM (pESX);
	m_pSpectrum->SetDivMMCEnabled (TRUE);

	// sin tarjeta montada, la de esxDOS (con lo que haya en la carpeta esxdos_i)
	if (m_HDDPath[0] == '\0')
	{
		if (m_EsxFolder.IsEnabled ())
		{
			EsxFolderToImage (FALSE);
		}
		FILINFO Info;
		if (f_stat ("SD:/hdd/esxdos.img", &Info) == FR_OK)
		{
			MountHardDisk ("hdd/esxdos.img");	// reinicia la máquina
			ShowMessage (T ("divMMC conectada (hdd/esxdos.img)"));
			SyncPeripheralSettings ();
		return;
		}
	}

	m_pSpectrum->Reset ();
	ShowMessage (m_HDDPath[0] != '\0' ? T ("divMMC conectada") : T ("divMMC conectada (sin tarjeta)"));
	SyncPeripheralSettings ();
}

void CKernel::ToggleLEC (void)
{
	if (m_pSpectrum->IsLECConnected ())
	{
		m_pSpectrum->SetLECConnected (FALSE);
		m_pSpectrum->Reset ();
		ShowMessage (T ("Memoria LEC desconectada"));
		SyncPeripheralSettings ();
		return;
	}

	if (m_pSpectrum->GetModel () == Model16K)
	{
		ShowMessage (T ("La memoria LEC necesita 48K o más"));
		SyncPeripheralSettings ();
		return;
	}

	if (m_pSpectrum->IsSpectranetEnabled ())
	{
		ToggleSpectranet ();
	}
	m_pSpectrum->SetDivMMCEnabled (FALSE);

	if (m_pSpectrum->SetLECConnected (TRUE))
	{
		m_pSpectrum->Reset ();
		ShowMessage (m_pSpectrum->GetModel () == Model48K ? T ("Memoria LEC conectada (528 KB)")
				 : T ("Memoria LEC conectada: úsala desde 48 BASIC"));
	}
	SyncPeripheralSettings ();
}

void CKernel::SaveSpectranetFlash (void)
{
	FIL File;
	UINT nWritten = 0;
	if (f_open (&File, SNET_FLASH_FILE, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
	{
		f_write (&File, m_pSpectrum->GetSpectranet ()->GetFlash (), SNET_FLASH_SIZE, &nWritten);
		f_close (&File);
	}
	m_Logger.Write (FromKernel, LogNotice, "Spectranet: flash guardada (%u bytes)", nWritten);
}

////////////////////////////////////////////////////////////////////////////////
// Pantallas de configuración (F1 y luego C)

// Valores de arranque: los de cmdline.txt
void CKernel::LoadSettings (void)
{
	// Se leen del propio cmdline.txt: es donde se guardan, y Circle se queda
	// algunas opciones para sí (sounddev) y no las devuelve como opciones de
	// la aplicación, así que con m_Options saldría siempre el valor por defecto
	// (y al guardar se perdería, por ejemplo, la salida por el jack)
	static char CmdLine[1024];
	CmdLine[0] = '\0';
	FIL File;
	UINT nRead = 0;
	if (m_bSDMounted && f_open (&File, "SD:/cmdline.txt", FA_READ | FA_OPEN_EXISTING) == FR_OK)
	{
		f_read (&File, CmdLine, sizeof CmdLine - 1, &nRead);
		f_close (&File);
	}
	CmdLine[nRead] = '\0';

	for (unsigned i = 0; i < m_Settings.GetCount (); i++)
	{
		const TSettingDef &Def = m_Settings.GetDef (i);
		char Value[SETTING_VALUE_LEN];
		if (FindCmdLineOption (CmdLine, Def.pKey, Value, sizeof Value))
		{
			m_Settings.SetValue (i, Value);
		}
		else if (strcmp (Def.pKey, "sounddev") == 0 && *m_Options.GetSoundDevice () != '\0')
		{
			m_Settings.SetValue (i, m_Options.GetSoundDevice ());
		}
		else
		{
			m_Settings.SetValue (i, m_Options.GetAppOptionString (Def.pKey, Def.pDefault));
		}
	}
	m_Settings.ClearDirty ();
	m_Menu.SetSettings (&m_Settings);
}

// Valor de una opción en la primera línea de cmdline.txt (la última si se repite)
boolean CKernel::FindCmdLineOption (const char *pCmdLine, const char *pKey, char *pValue, unsigned nSize)
{
	boolean bFound = FALSE;
	unsigned nKey = strlen (pKey);
	const char *p = pCmdLine;
	while (*p != '\0' && *p != '\r' && *p != '\n')
	{
		while (*p == ' ' || *p == '\t')
		{
			p++;
		}
		const char *pToken = p;
		while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
		{
			p++;
		}
		if (   (unsigned) (p - pToken) > nKey && strncmp (pToken, pKey, nKey) == 0
		    && pToken[nKey] == '=')
		{
			unsigned n = p - pToken - nKey - 1;
			if (n > nSize - 1)
			{
				n = nSize - 1;
			}
			memcpy (pValue, pToken + nKey + 1, n);
			pValue[n] = '\0';
			bFound = TRUE;
		}
	}
	return bFound;
}

// Lo que se cambia con las teclas (F4, F6, F7, F9, F10, Alt+B, Alt+S, Alt+I y
// los periféricos) pasa a la configuración, para poder guardarlo con G. Solo
// lo que cambia el usuario: lo que cambia solo (por ejemplo, el modelo al
// cargar un snapshot de 128K) no se toca, para no guardarlo sin querer.
void CKernel::SyncSetting (const char *pKey)
{
	const char *pValue = 0;
	CString Value;

	if (strcmp (pKey, "model") == 0)
	{
		static const char *Models[] = {"16k", "48k", "128k", "plus2", "plus2a", "plus3", "plus3e"};
		static const TModel ModelIDs[] = {Model16K, Model48K, Model128K, ModelPlus2, ModelPlus2A, ModelPlus3, ModelPlus3E};
		for (unsigned i = 0; i < sizeof Models / sizeof Models[0]; i++)
		{
			if (m_pSpectrum->GetModel () == ModelIDs[i])
			{
				pValue = Models[i];
			}
		}
	}
	else if (strcmp (pKey, "issue") == 0)		pValue = m_pSpectrum->IsIssue2 () ? "2" : "3";
	else if (strcmp (pKey, "speed") == 0)
	{
		Value.Format ("%u", m_nSpeed);
		pValue = Value;
	}
	else if (strcmp (pKey, "border") == 0)
	{
		static const char *Borders[] = {"none", "standard", "full", "huge"};
		pValue = Borders[m_BorderSize % 4];
	}
	else if (strcmp (pKey, "scanlines") == 0)	pValue = m_bScanlines ? "1" : "0";
	else if (strcmp (pKey, "pureblack") == 0)	pValue = m_pSpectrum->IsPureBlack () ? "1" : "0";
	else if (strcmp (pKey, "aystereo") == 0)
	{
		static const char *Stereo[] = {"abc", "acb", "bac", "mono"};
		pValue = Stereo[m_pSpectrum->GetAY ()->GetStereoMode () % 4];
	}
	else if (strcmp (pKey, "tape") == 0)
	{
		TTapeMode TapeMode = m_pSpectrum->GetTapeMode ();
		pValue = TapeMode == TapeModeFlash ? "flash" : (TapeMode == TapeModeNormal ? "normal" : "fast");
	}
	else if (strcmp (pKey, "tapeaccel") == 0)	pValue = m_pSpectrum->GetLoaderAcceleration () ? "1" : "0";
	else if (strcmp (pKey, "joystick") == 0)
	{
		static const char *Joy[] = {"none", "kempston", "fuller", "sinclair1", "sinclair2", "cursor", "qaop"};
		pValue = Joy[m_Keyboard.GetJoystickMode () % JoystickModeCount];
	}
	else if (strcmp (pKey, "spectranet") == 0)	pValue = m_pSpectrum->IsSpectranetEnabled () ? "1" : "0";
	else if (strcmp (pKey, "divmmc") == 0)		pValue = m_pSpectrum->IsDivMMCActive () ? "esxdos" : "0";
	else if (strcmp (pKey, "lec") == 0)		pValue = m_pSpectrum->IsLECConnected () ? "1" : "0";

	int n = m_Settings.Find (pKey);
	if (pValue != 0 && n >= 0 && strcmp (m_Settings.GetValue (n), pValue) != 0)
	{
		m_Settings.SetValue (pKey, pValue);	// (marca "sin guardar")
	}
}

// Los periféricos que se paginan sobre la ROM: al conectar uno se desconectan
// los demás
void CKernel::SyncPeripheralSettings (void)
{
	SyncSetting ("spectranet");
	SyncSetting ("divmmc");
	SyncSetting ("lec");
}

// Aplica en vivo una opción recién cambiada (las que no son "en vivo" se
// quedan para el próximo arranque)
void CKernel::ApplySetting (unsigned nIndex)
{
	const TSettingDef &Def = m_Settings.GetDef (nIndex);
	const char *pKey = Def.pKey;
	const char *pValue = m_Settings.GetValue (nIndex);
	CString Message;

	if (!Def.bLive)
	{
		Message.Format (T ("%s: %s (al reiniciar)"), T (Def.pLabel), m_Settings.GetValueName (nIndex));
		ShowMessage (Message);
		return;
	}

	if (strcmp (pKey, "lang") == 0)
	{
		m_I18n.SetLanguage (pValue);
		SetupHardDiskList ();		// ("ninguno" en el idioma nuevo)
		SetupROMLists ();
	}
	else if (strcmp (pKey, "model") == 0)
	{
		static const char *Models[] = {"16k", "48k", "128k", "plus2", "plus2a", "plus3", "plus3e"};
		static const TModel ModelIDs[] = {Model16K, Model48K, Model128K, ModelPlus2, ModelPlus2A, ModelPlus3, ModelPlus3E};
		for (unsigned i = 0; i < sizeof Models / sizeof Models[0]; i++)
		{
			if (strcmp (pValue, Models[i]) == 0)
			{
				if (m_pSpectrum->IsModelAvailable (ModelIDs[i]))
				{
					SetModel (ModelIDs[i]);
				}
				else
				{
					ShowMessage (T ("Falta la ROM de ese modelo (se guarda igual)"));
				}
			}
		}
	}
	else if (strcmp (pKey, "issue") == 0)		m_pSpectrum->SetIssue2 (strcmp (pValue, "2") == 0);
	else if (strcmp (pKey, "speed") == 0)		m_nSpeed = atoi (pValue);
	else if (strcmp (pKey, "ulaplus") == 0)		m_pSpectrum->SetULAplusEnabled (strcmp (pValue, "1") == 0);
	else if (strcmp (pKey, "pureblack") == 0)	m_pSpectrum->SetPureBlack (strcmp (pValue, "1") == 0);
	else if (strcmp (pKey, "border") == 0 || strcmp (pKey, "scanlines") == 0)
	{
		static const char *Borders[] = {"none", "standard", "full", "huge"};
		const char *pBorder = m_Settings.GetValue (m_Settings.Find ("border"));
		for (unsigned i = 0; i < 4; i++)
		{
			if (strcmp (pBorder, Borders[i]) == 0)
			{
				m_BorderSize = (TBorderSize) i;
			}
		}
		m_bScanlines = strcmp (m_Settings.GetValue (m_Settings.Find ("scanlines")), "1") == 0;
		SetVideoMode (m_BorderSize, m_bScanlines);
		if (m_bScanlines && !m_Video.GetScanlines ())
		{
			m_bScanlines = FALSE;
			ShowMessage (T ("Scanlines: pantalla demasiado pequeña"));
		}
	}
	else if (strcmp (pKey, "aystereo") == 0)
	{
		static const char *Stereo[] = {"abc", "acb", "bac", "mono"};
		for (unsigned i = 0; i < 4; i++)
		{
			if (strcmp (pValue, Stereo[i]) == 0)
			{
				m_pSpectrum->GetAY ()->SetStereoMode ((TAYStereoMode) i);
			}
		}
	}
	else if (strcmp (pKey, "tapesound") == 0)	m_pSpectrum->GetAudio ()->SetTapeSound (strcmp (pValue, "1") == 0);
	else if (strcmp (pKey, "tapeaccel") == 0)	m_pSpectrum->SetLoaderAcceleration (strcmp (pValue, "1") == 0);
	else if (strcmp (pKey, "beeperfilter") == 0)	m_pSpectrum->GetAudio ()->SetBeeperFilter (strcmp (pValue, "1") == 0);
	else if (strcmp (pKey, "speaker") == 0)
	{
		strncpy (m_Speaker, pValue, sizeof m_Speaker - 1);
		m_Speaker[sizeof m_Speaker - 1] = '\0';
		ApplySpeaker ();
	}
	else if (strcmp (pKey, "tape") == 0)
	{
		m_pSpectrum->SetTapeMode (strcmp (pValue, "flash") == 0 ? TapeModeFlash
					  : (strcmp (pValue, "normal") == 0 ? TapeModeNormal : TapeModeFast));
	}
	else if (strcmp (pKey, "savetrap") == 0)
	{
		m_pSpectrum->SetSaveHandler (m_bSDMounted && strcmp (pValue, "1") == 0 ? this : 0);
	}
	else if (strcmp (pKey, "savefmt") == 0)		m_bSaveTZX = strcmp (pValue, "tzx") == 0;
	else if (strcmp (pKey, "snapformat") == 0)
	{
		m_nSnapFormat = strcmp (pValue, "sna") == 0 ? 1 : (strcmp (pValue, "szx") == 0 ? 2 : 0);
	}
	else if (strcmp (pKey, "joystick") == 0)
	{
		static const char *Joy[] = {"none", "kempston", "fuller", "sinclair1", "sinclair2", "cursor", "qaop"};
		for (unsigned i = 0; i < JoystickModeCount; i++)
		{
			if (strcmp (pValue, Joy[i]) == 0)
			{
				m_Keyboard.SetJoystickMode ((TJoystickMode) i);
			}
		}
	}
	else if (strcmp (pKey, "cursors") == 0)
	{
		m_Keyboard.SetCursorMode (strcmp (pValue, "keys") == 0 ? CursorKeysOnly
					  : (strcmp (pValue, "joystick") == 0 ? CursorJoystickOnly : CursorBoth));
	}
	else if (strcmp (pKey, "recreated") == 0)
	{
		m_RecreatedMode = strcmp (pValue, "auto") == 0 ? RecreatedAuto
				: (strcmp (pValue, "1") == 0 ? RecreatedForced : RecreatedOff);
		for (unsigned i = 0; i < MAX_KEYBOARDS; i++)
		{
			m_Recreated[i].Reset (m_RecreatedMode);
		}
	}
	else if (strcmp (pKey, "spectranet") == 0)
	{
		if ((strcmp (pValue, "1") == 0) != m_pSpectrum->IsSpectranetEnabled ())
		{
			ToggleSpectranet ();
		}
	}
	else if (strcmp (pKey, "divmmc") == 0)
	{
		if ((strcmp (pValue, "esxdos") == 0) != m_pSpectrum->IsDivMMCActive ())
		{
			ToggleDivMMC ();
		}
	}
	else if (strcmp (pKey, "lec") == 0)
	{
		if ((strcmp (pValue, "1") == 0) != m_pSpectrum->IsLECConnected ())
		{
			ToggleLEC ();
		}
	}
	else if (strcmp (pKey, "if1") == 0)
	{
		m_nIF1Mode = strcmp (pValue, "0") == 0 ? 0 : (strcmp (pValue, "1") == 0 ? 1 : 2);
		m_pSpectrum->SetIF1Connected (m_nIF1Mode == 1);
	}
	else if (strcmp (pKey, "multiface") == 0)	m_pSpectrum->SetMultifaceEnabled (strcmp (pValue, "1") == 0);

	// los periféricos pueden haber desconectado otros: la pantalla lo refleja
	if (   strcmp (pKey, "spectranet") == 0 || strcmp (pKey, "divmmc") == 0
	    || strcmp (pKey, "lec") == 0)
	{
		SyncPeripheralSettings ();
	}
	UpdateStatusLines ();
}

// Escribe los valores en cmdline.txt (con copia en cmdline.bak)
void CKernel::SaveSettings (void)
{
	static char Old[1024];
	static char New[1100];
	FIL File;
	UINT nBytes = 0;

	Old[0] = '\0';
	if (f_open (&File, "SD:/cmdline.txt", FA_READ | FA_OPEN_EXISTING) == FR_OK)
	{
		f_read (&File, Old, sizeof Old - 1, &nBytes);
		f_close (&File);
		Old[nBytes] = '\0';

		if (f_open (&File, "SD:/cmdline.bak", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
		{
			f_write (&File, Old, nBytes, &nBytes);
			f_close (&File);
		}
	}

	m_Settings.Merge (Old, New, sizeof New);

	if (   f_open (&File, "SD:/cmdline.txt", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK
	    || f_write (&File, New, strlen (New), &nBytes) != FR_OK)
	{
		ShowMessage (T ("No se ha podido guardar cmdline.txt"));
		return;
	}
	f_close (&File);

	m_Settings.ClearDirty ();
	ShowMessage (T ("Guardado en cmdline.txt (copia: cmdline.bak)"));
	m_Logger.Write (FromKernel, LogNotice, "cmdline.txt: %s", New);
}

// Reinicia la Raspberry Pi desde la configuración (tras confirmarlo): antes
// se guarda todo lo que el emulador deja para más tarde (discos, cartuchos,
// flash de la Spectranet, carpeta de esxDOS, salida del RS232)
void CKernel::RebootPi (boolean bSaveSettings)
{
	if (bSaveSettings)
	{
		SaveSettings ();
	}

	ShowMessage (T ("Reiniciando..."));
	m_Logger.Write (FromKernel, LogNotice, "Reinicio pedido desde la configuracion");

	SaveDisks (TRUE);
	SaveMicrodrives (TRUE);
	if (m_pSpectrum->IsSpectranetEnabled () && m_nSnetSaveFrames > 0)
	{
		SaveSpectranetFlash ();
	}
	if (m_HardDisk.IsDirty ())
	{
		m_HardDisk.Flush ();
	}
	m_IF1Link.Flush ();

	m_HardDisk.Close ();
	f_mount (0, "SD:", 1);			// cierra la SD con todo escrito

	CTimer::Get ()->MsDelay (200);
	reboot ();
}

// Fecha y hora en formato MS-DOS para el RTC.SYS de esxDOS (una vez por segundo)
void CKernel::UpdateRTC (void)
{
	unsigned nNow = CTimer::Get ()->GetLocalTime ();
	if (nNow == m_nRTCLast)
	{
		return;
	}
	m_nRTCLast = nNow;

	CTime Time;
	Time.Set (nNow);
	if (Time.GetYear () < 2024)		// la Pi aún no tiene hora (sin NTP)
	{
		m_pSpectrum->SetRTC (0, 0);
		return;
	}
	u16 nDate = ((Time.GetYear () - 1980) << 9) | (Time.GetMonth () << 5) | Time.GetMonthDay ();
	u16 nTime = (Time.GetHours () << 11) | (Time.GetMinutes () << 5) | (Time.GetSeconds () / 2);
	m_pSpectrum->SetRTC (nDate, nTime);
}

// Bloque 0x28 de una cinta TZX: lista de opciones (por ejemplo, la parte
// del juego que se quiere cargar)
void CKernel::OpenTapeSelect (void)
{
	memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
	OpenMenu ();
	m_Menu.OpenChoice (T ("La cinta pide elegir qué cargar:"));
	for (unsigned i = 0; i < m_Tape.GetSelectCount () && i < MENU_CHOICE_ITEMS; i++)
	{
		char Text[40];
		if (!m_Tape.GetSelectItem (i, Text, sizeof Text))
		{
			strcpy (Text, "?");
		}
		m_Menu.SetChoiceItem (i, Text);
	}
	m_Keyboard.SetCaptured (TRUE);
}

// Nombre de la ROM de una opción (rom...=): "default" (o nada) es la de siempre
const char *CKernel::ROMOption (const char *pKey, const char *pDefault)
{
	const char *pName = m_Options.GetAppOptionString (pKey, pDefault);
	return (pName == 0 || *pName == '\0' || strcasecmp (pName, "default") == 0) ? pDefault : pName;
}

// Página "ROMs y carpetas" de la configuración: para cada ROM, las de la
// carpeta roms con su tamaño; para esxDOS, las carpetas de la raíz de la SD
void CKernel::SetupROMLists (void)
{
	static const struct { const char *pKey; unsigned nSize; const char *pDefaultName; } ROMs[] =
	{
		{"rom48",     0x4000,  N_("la incluida")},
		{"rom128",    0x8000,  N_("la incluida")},
		{"romplus2",  0x8000,  "plus2.rom"},
		{"romplus2a", 0x10000, "plus2a.rom"},
		{"romplus3",  0x10000, "plus3.rom"},
		{"romplus3e", 0x10000, N_("según el idioma")},
		{"romif1",    0x2000,  "if1.rom"},
		{"romesx",    0x2000,  "esxmmc.bin"},
		{"romsnet",   0x20000, "spectranet.rom"},
	};
	for (unsigned i = 0; i < sizeof ROMs / sizeof ROMs[0] && i < ROM_LISTS; i++)
	{
		m_ROMValues[i] = "default";
		m_ROMNames[i] = T (ROMs[i].pDefaultName);
		DIR Dir;
		FILINFO Info;
		if (m_bSDMounted && f_opendir (&Dir, "SD:/roms") == FR_OK)
		{
			unsigned nCount = 0;
			while (nCount < 24 && f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
			{
				if (   (Info.fattrib & AM_DIR) || Info.fsize != ROMs[i].nSize
				    || strchr (Info.fname, ' ') != 0 || strlen (Info.fname) > 60)
				{
					continue;
				}
				char UTF8[128];
				CP850ToUTF8 (Info.fname, UTF8, sizeof UTF8);
				m_ROMValues[i].Append ("|");
				m_ROMValues[i].Append (Info.fname);
				m_ROMNames[i].Append ("|");
				m_ROMNames[i].Append (UTF8);
				nCount++;
			}
			f_closedir (&Dir);
		}
		CSettings::SetValueList (ROMs[i].pKey, m_ROMValues[i], m_ROMNames[i]);
	}

	// carpeta de importación de esxDOS: esxdos_i, las demás carpetas de la raíz y "no"
	static const char *Skip[] = {ESX_IMPORT_DIR, "esxdos", "roms", "hdd", "firmware", "certs", "locales",
				     "saves", "snaps", "descargas", "utils", "cpm", 0};
	m_EsxDirValues = ESX_IMPORT_DIR;
	m_EsxDirNames = ESX_IMPORT_DIR;
	DIR Dir;
	FILINFO Info;
	if (m_bSDMounted && f_opendir (&Dir, "SD:/") == FR_OK)
	{
		unsigned nCount = 0;
		while (nCount < 16 && f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
		{
			if (!(Info.fattrib & AM_DIR) || strchr (Info.fname, ' ') != 0 || strlen (Info.fname) > 40)
			{
				continue;
			}
			boolean bSkip = FALSE;
			for (unsigned k = 0; Skip[k] != 0; k++)
			{
				bSkip |= strcasecmp (Info.fname, Skip[k]) == 0;
			}
			if (bSkip)
			{
				continue;
			}
			char UTF8[96];
			CP850ToUTF8 (Info.fname, UTF8, sizeof UTF8);
			m_EsxDirValues.Append ("|");
			m_EsxDirValues.Append (Info.fname);
			m_EsxDirNames.Append ("|");
			m_EsxDirNames.Append (UTF8);
			nCount++;
		}
		f_closedir (&Dir);
	}
	m_EsxDirValues.Append ("|0");
	m_EsxDirNames.Append ("|");
	m_EsxDirNames.Append (T ("no"));
	CSettings::SetValueList ("esxdir", m_EsxDirValues, m_EsxDirNames);
}

// "Disco duro al arrancar" (hdd=) en la configuración: las imágenes de la
// carpeta hdd de la SD. Las que tienen espacios en el nombre no se pueden
// poner en cmdline.txt (separa las opciones con espacios), y esxdos.img la
// lleva la propia divMMC.
void CKernel::SetupHardDiskList (void)
{
	m_HDDValues = "none";
	m_HDDNames = T ("ninguno");

	DIR Dir;
	FILINFO Info;
	if (!m_bSDMounted || f_opendir (&Dir, "SD:/hdd") != FR_OK)
	{
		CSettings::SetValueList ("hdd", m_HDDValues, m_HDDNames);
		return;
	}
	unsigned nCount = 0;
	while (nCount < 32 && f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
	{
		const char *pName = Info.fname;
		size_t nLen = strlen (pName);
		const char *pExt = nLen > 4 ? pName + nLen - 4 : "";
		if (   (Info.fattrib & AM_DIR) || strchr (pName, ' ') != 0 || nLen > 60
		    || strcasecmp (pName, "esxdos.img") == 0
		    || !(   strcasecmp (pExt, ".vhd") == 0 || strcasecmp (pExt, ".img") == 0
			 || strcasecmp (pExt, ".raw") == 0 || strcasecmp (pExt, ".hdf") == 0))
		{
			continue;
		}
		char UTF8[128];
		CP850ToUTF8 (pName, UTF8, sizeof UTF8);
		m_HDDValues.Append ("|hdd/");
		m_HDDValues.Append (pName);
		m_HDDNames.Append ("|");
		m_HDDNames.Append (UTF8);
		nCount++;
	}
	f_closedir (&Dir);
	CSettings::SetValueList ("hdd", m_HDDValues, m_HDDNames);
}

////////////////////////////////////////////////////////////////////////////////
// Pantalla de unidades (U desde la ayuda o Alt+U)

void CKernel::OpenUnitsScreen (void)
{
	if (!m_Menu.IsOpen ())
	{
		memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
		OpenMenu ();
		m_Keyboard.SetCaptured (TRUE);
	}
	UpdateUnitLines ();
	m_Menu.OpenUnits ();
}

void CKernel::UpdateUnitLines (void)
{
	m_Menu.SetUnitLine (UNIT_TAPE, m_Tape.IsInserted () ? m_Tape.GetName () : "");

	CPlus3Disk *pDisk = m_pSpectrum->GetDisk ();
	for (unsigned i = 0; i < 2; i++)
	{
		const char *pName = "";
		CString Line;
		if (pDisk->HasDisk (i))
		{
			pName = m_DSKPath[i][0] != '\0' ? m_DSKPath[i] : T ("disco nuevo");
			if (pDisk->Is35 (i))
			{
				Line.Format ("%s%s", pName, T (" (unidad de 3,5\")"));
				pName = Line;
			}
		}
		m_Menu.SetUnitLine (UNIT_DISK_A + i, pName);
	}

	m_Menu.SetUnitLine (UNIT_HARDDISK, m_HDDPath);
	m_Menu.SetUnitLine (UNIT_CARTRIDGE, m_pSpectrum->IsCartridgeInserted () ? m_CartridgeName : "");

	CInterface1 *pIF1 = m_pSpectrum->GetInterface1 ();
	for (unsigned i = 0; i < MICRODRIVES; i++)
	{
		const char *pName = "";
		if (pIF1->GetDrive (i)->IsCartridge ())
		{
			pName = m_MDRPath[i][0] != '\0' ? m_MDRPath[i] : T ("cartucho nuevo");
		}
		m_Menu.SetUnitLine (UNIT_MICRODRIVE + i, pName);
	}
}

// Monta un fichero en una unidad concreta (el selector ya solo muestra los de
// su tipo)
void CKernel::MountUnit (unsigned nUnit, const char *pPath)
{
	if (nUnit == UNIT_TAPE)
	{
		LoadTape (pPath);
	}
	else if (nUnit == UNIT_DISK_A || nUnit == UNIT_DISK_B)
	{
		InsertDisk (pPath, (int) (nUnit - UNIT_DISK_A));
	}
	else if (nUnit == UNIT_HARDDISK)
	{
		MountHardDisk (pPath);
	}
	else if (nUnit == UNIT_CARTRIDGE)
	{
		LoadCartridge (pPath);
	}
	else if (nUnit < UNIT_COUNT)
	{
		InsertMicrodrive (pPath, (int) (nUnit - UNIT_MICRODRIVE));
	}
}

// Deja una unidad vacía (grabando antes lo pendiente)
void CKernel::EjectUnit (unsigned nUnit)
{
	if (nUnit == UNIT_TAPE)
	{
		if (m_Tape.IsInserted ())
		{
			m_pSpectrum->TapeStop ();
			m_Tape.Eject ();
			ShowMessage (T ("Cinta sacada"));
		}
	}
	else if (nUnit == UNIT_DISK_A || nUnit == UNIT_DISK_B)
	{
		unsigned i = nUnit - UNIT_DISK_A;
		CPlus3Disk *pDisk = m_pSpectrum->GetDisk ();
		if (pDisk->HasDisk (i))
		{
			SaveDisks (TRUE);
			pDisk->Eject (i);
			m_DSKPath[i][0] = '\0';
			ShowMessage (i == 0 ? T ("Disco A: expulsado") : T ("Disco B: expulsado"));
		}
	}
	else if (nUnit == UNIT_HARDDISK)
	{
		if (m_HDDPath[0] != '\0')
		{
			m_pSpectrum->GetSDCard ()->Insert (0);
			m_HardDisk.Flush ();
			m_HardDisk.Close ();
			m_HDDPath[0] = '\0';
			ShowMessage (T ("Disco duro desmontado"));
		}
	}
	else if (nUnit == UNIT_CARTRIDGE)
	{
		if (m_pSpectrum->IsCartridgeInserted ())
		{
			m_pSpectrum->EjectCartridge ();
			m_CartridgeName[0] = '\0';
			ShowMessage (T ("Cartucho retirado"));
		}
	}
	else if (nUnit < UNIT_COUNT)
	{
		unsigned i = nUnit - UNIT_MICRODRIVE;
		CInterface1 *pIF1 = m_pSpectrum->GetInterface1 ();
		if (pIF1->GetDrive (i)->IsCartridge ())
		{
			SaveMicrodrives (TRUE);
			pIF1->GetDrive (i)->Eject ();
			m_MDRPath[i][0] = '\0';

			boolean bAny = FALSE;
			for (unsigned j = 0; j < MICRODRIVES; j++)
			{
				bAny |= pIF1->GetDrive (j)->IsCartridge ();
			}
			if (!bAny && m_nIF1Mode == 2)
			{
				m_pSpectrum->SetIF1Connected (FALSE);
			}

			CString Message;
			Message.Format (T ("Microdrive %u vacío"), i + 1);
			ShowMessage (Message);
		}
	}
}

// Idiomas: los de SD:/locales, en la configuración (página 1) y con lang=
void CKernel::SetupLanguages (void)
{
	if (m_bSDMounted)
	{
		m_I18n.ScanLanguages ();
	}
	m_LangValues = "";
	m_LangNames = "";
	for (unsigned i = 0; i < m_I18n.GetLanguageCount (); i++)
	{
		if (i > 0)
		{
			m_LangValues.Append ("|");
			m_LangNames.Append ("|");
		}
		m_LangValues.Append (m_I18n.GetLanguageCode (i));
		m_LangNames.Append (m_I18n.GetLanguageName (i));
	}
	CSettings::SetValueList ("lang", m_LangValues, m_LangNames);
	m_I18n.SetLanguage (m_Options.GetAppOptionString ("lang", "es"));
}

////////////////////////////////////////////////////////////////////////////////
// esxDOS: imagen hdd/esxdos.img y carpeta de importación SD:/esxdos_i (ver esxfolder.h)

void CKernel::SetupEsxFolder (void)
{
	const char *pDir = m_Options.GetAppOptionString ("esxdir", ESX_IMPORT_DIR);
	if (!m_bSDMounted || strcmp (pDir, "0") == 0 || *pDir == '\0')
	{
		return;
	}

	// Hasta la 0.94 la carpeta era "esxdos" y se sincronizaba con la imagen en
	// los dos sentidos. Ahora es una carpeta para importar: la vieja se
	// renombra y su contenido se importa como cualquier otro (lo que ya
	// estaba en la imagen se sustituye por lo mismo y deja de estar repetido).
	if (strcasecmp (pDir, "esxdos") == 0)
	{
		pDir = ESX_IMPORT_DIR;
	}
	if (strcasecmp (pDir, ESX_IMPORT_DIR) == 0)
	{
		FILINFO Old, New;
		if (   f_stat ("SD:/esxdos", &Old) == FR_OK && (Old.fattrib & AM_DIR)
		    && f_stat ("SD:/" ESX_IMPORT_DIR, &New) != FR_OK)
		{
			if (f_rename ("SD:/esxdos", "SD:/" ESX_IMPORT_DIR) == FR_OK)
			{
				m_Logger.Write (FromKernel, LogNotice, "esxDOS: carpeta esxdos renombrada a " ESX_IMPORT_DIR);
			}
		}
		f_unlink ("SD:/hdd/esxdos.img.sync");	// la lista de la sincronización vieja
		f_mkdir ("SD:/" ESX_IMPORT_DIR);
	}

	CString Folder;
	Folder.Format ("SD:/%s", pDir);
	FILINFO Info;
	if (f_stat (Folder, &Info) == FR_OK && (Info.fattrib & AM_DIR))
	{
		m_EsxFolder.Setup (Folder, "hdd/esxdos.img");
		m_Logger.Write (FromKernel, LogNotice, "esxDOS: carpeta %s", (const char *) Folder);
	}
}

boolean CKernel::IsEsxImageMounted (void) const
{
	const char *pPath = m_HDDPath;
	if (strncmp (pPath, "SD:/", 4) == 0)
	{
		pPath += 4;
	}
	return m_EsxFolder.IsEnabled () && strcasecmp (pPath, m_EsxFolder.GetImagePath ()) == 0;
}

// Carpeta -> imagen. Si la imagen está montada en la tarjeta del Spectrum, se
// saca mientras tanto (y se vuelve a poner si bRemount).
void CKernel::EsxFolderToImage (boolean bRemount)
{
	if (!m_EsxFolder.IsEnabled ())
	{
		return;
	}

	// Sin la divMMC (ni la imagen montada) no se importa: no la usa nadie
	boolean bMounted = IsEsxImageMounted ();
	if (!bMounted && !m_pSpectrum->IsDivMMCActive ())
	{
		return;
	}

	// carpeta vacía: nada que hacer (sin montar la imagen)
	if (!m_EsxFolder.HasPending ())
	{
		return;
	}

	if (bMounted)
	{
		if (!bRemount)
		{
			return;			// en uso y sin poder desmontarla ahora
		}
		m_pSpectrum->GetSDCard ()->Insert (0);
		m_HardDisk.Flush ();
		m_HardDisk.Close ();
	}

	m_nEsxProgressTime = 0;
	boolean bOK = m_EsxFolder.Import (EsxProgressHandler, this);

	if (bMounted)
	{
		if (m_HardDisk.Open (m_HDDPath) == 0)
		{
			m_pSpectrum->GetSDCard ()->Insert (&m_HardDisk);
		}
	}

	CString Message;
	unsigned nImported = m_EsxFolder.GetImported ();
	unsigned nFailed = m_EsxFolder.GetFailed ();
	if (!bOK && nImported == 0 && nFailed == 0 && m_EsxFolder.GetError () != 0)
	{
		Message.Format (T ("esxDOS: %s"), T (m_EsxFolder.GetError ()));
	}
	else if (nFailed > 0)
	{
		Message.Format (T ("esxDOS: %u importados, %u no (%s)"), nImported, nFailed,
				m_EsxFolder.GetError () != 0 ? T (m_EsxFolder.GetError ()) : "?");
	}
	else if (m_EsxFolder.GetReplaced () > 0)
	{
		Message.Format (T ("esxDOS: %u importados (%u sustituidos)"), nImported, m_EsxFolder.GetReplaced ());
	}
	else if (nImported > 0)
	{
		Message.Format (T ("esxDOS: %u importados"), nImported);
	}
	if (Message.GetLength () > 0)
	{
		ShowMessage (Message);
	}
}

// Progreso de la importación: la emulación está parada mientras tanto, así
// que se pinta y se muestra al momento (como mucho cada 200 ms)
void CKernel::EsxProgressHandler (unsigned nDone, unsigned nTotal, unsigned nMBDone, unsigned nMBTotal,
				  void *pParam)
{
	CKernel *pThis = static_cast<CKernel *> (pParam);
	unsigned nNow = CTimer::GetClockTicks ();
	if (pThis->m_nEsxProgressTime != 0 && nNow - pThis->m_nEsxProgressTime < 200000 && nDone < nTotal)
	{
		return;
	}
	pThis->m_nEsxProgressTime = nNow == 0 ? 1 : nNow;

	CString Text;
	if (nTotal == 0)
	{
		Text = T ("esxDOS: preparando una tarjeta mayor...");
	}
	else
	{
		Text.Format (T ("esxDOS: importando %u de %u (%u de %u MB)..."), nDone, nTotal, nMBDone, nMBTotal);
	}
	NetBusyHandler (Text, pThis);
}

// Busca en cmdline.txt opciones desconocidas o con valores no admitidos: un
// valor mal escrito (sounddev=sndpwn) no da error, el emulador usa el valor por
// defecto sin avisar. El primero se muestra al arrancar; todos van al registro.
static void LogOptionProblem (const char *pText)
{
	CLogger::Get ()->Write ("zxcircle", LogWarning, "cmdline.txt: %s", pText);
}

void CKernel::CheckCmdLine (void)
{
	m_OptionWarning[0] = '\0';
	if (!m_bSDMounted)
	{
		return;
	}

	static char Buffer[1024];
	FIL File;
	UINT nRead = 0;
	if (f_open (&File, "SD:/cmdline.txt", FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		return;
	}
	f_read (&File, Buffer, sizeof Buffer - 1, &nRead);
	f_close (&File);
	Buffer[nRead] = '\0';

	char Problem[sizeof m_OptionWarning];
	unsigned nProblems = CheckOptions (Buffer, Problem, sizeof Problem, LogOptionProblem);
	if (nProblems > 1)
	{
		CString Message;
		Message.Format ("%s (+%u)", Problem, nProblems - 1);
		strncpy (m_OptionWarning, Message, sizeof m_OptionWarning - 1);
		m_OptionWarning[sizeof m_OptionWarning - 1] = '\0';
	}
	else
	{
		strcpy (m_OptionWarning, Problem);
	}
}

// ROMs de la carpeta roms de la SD. Solo las del 48K y el 128K van dentro del
// kernel; las demás se cargan aquí y, si falta alguna, la máquina o el
// periférico que la necesita no está disponible (ver roms/LEEME.txt):
//   48.rom      16 KB  opcional, sustituye a la del 16K/48K incluida
//   128.rom     32 KB  opcional, sustituye a la del 128K incluida
//   plus2.rom   32 KB  +2 gris
//   plus2a.rom  64 KB  +2A
//   plus3.rom   64 KB  +3 (y discos .DSK)
//   plus3e-es.rom / plus3e-en.rom  64 KB  +3e (y disco duro); p3elang=es|en.
//               Si existe plus3e.rom, tiene preferencia.
//   if1.rom      8 KB  Interface 1 (microdrives y CP/M de LEC)
// Con rom48=, rom128=, romplus2=, romplus2a=, romplus3=, romplus3e= y romif1=
// se puede indicar otro fichero de la carpeta roms.
const u8 *CKernel::LoadROMFile (const char *pName, unsigned nSize)
{
	CString Path;
	Path.Format ("SD:/roms/%s", pName);

	FILINFO Info;
	if (f_stat (Path, &Info) != FR_OK)
	{
		return 0;
	}

	if (Info.fsize != nSize)
	{
		m_Logger.Write (FromKernel, LogWarning, "%s: tamano incorrecto (debe ser %u bytes)",
				(const char *) Path, nSize);
		return 0;
	}

	u8 *pROM = new u8[nSize];
	if (pROM != 0 && !LoadFile (Path, pROM, nSize))
	{
		delete [] pROM;
		pROM = 0;
	}
	return pROM;
}

void CKernel::LoadROMs (void)
{
	m_MissingROMs = "";
	if (!m_bSDMounted)
	{
		m_MissingROMs = "+2 +2A +3 +3e IF1";
		return;
	}

	static const struct
	{
		const char *pOption;
		const char *pDefault;
		unsigned nSize;
		TModel Model;
		const char *pShort;	// nombre para el aviso (0 = ROM opcional)
	}
	ROMs[] =
	{
		{"rom48",     "48.rom",     0x4000,  Model48K,    0},
		{"rom128",    "128.rom",    0x8000,  Model128K,   0},
		{"romplus2",  "plus2.rom",  0x8000,  ModelPlus2,  "+2"},
		{"romplus2a", "plus2a.rom", 0x10000, ModelPlus2A, "+2A"},
		{"romplus3",  "plus3.rom",  0x10000, ModelPlus3,  "+3"},
		{"romplus3e", 0,            0x10000, ModelPlus3E, "+3e"},
	};

	for (unsigned i = 0; i < sizeof ROMs / sizeof ROMs[0]; i++)
	{
		const char *pName = m_Options.GetAppOptionString (ROMs[i].pOption, ROMs[i].pDefault);
		if (pName != 0 && strcasecmp (pName, "default") == 0)
		{
			pName = ROMs[i].pDefault;	// "la incluida" en la configuración
		}
		const u8 *pROM = 0;

		if (pName != 0)
		{
			pROM = LoadROMFile (pName, ROMs[i].nSize);
		}
		else
		{
			// +3e: plus3e.rom si existe; si no, la del idioma elegido
			pROM = LoadROMFile ("plus3e.rom", ROMs[i].nSize);
			if (pROM == 0)
			{
				boolean bEnglish = strcasecmp (m_Options.GetAppOptionString ("p3elang", "es"), "en") == 0;
				pROM = LoadROMFile (bEnglish ? "plus3e-en.rom" : "plus3e-es.rom", ROMs[i].nSize);
			}
		}

		if (pROM != 0)
		{
			m_pSpectrum->SetModelROM (ROMs[i].Model, pROM);
		}
		else if (ROMs[i].pShort != 0)
		{
			m_MissingROMs.Append (m_MissingROMs.GetLength () > 0 ? " " : "");
			m_MissingROMs.Append (ROMs[i].pShort);
		}
	}

	const u8 *pIF1 = LoadROMFile (ROMOption ("romif1", "if1.rom"), 0x2000);
	if (pIF1 != 0)
	{
		m_pSpectrum->SetIF1ROM (pIF1);
	}
	else
	{
		m_MissingROMs.Append (m_MissingROMs.GetLength () > 0 ? " IF1" : "IF1");
	}

	if (m_MissingROMs.GetLength () > 0)
	{
		m_Logger.Write (FromKernel, LogWarning, "Faltan ROMs en SD:/roms: %s",
				(const char *) m_MissingROMs);
	}
}

// Disco duro del +3e (tarjeta de la ZXMMC): abre la imagen y arranca el +3e
// La imagen es la tarjeta de la ZXMMC (+3e) y de la divMMC (esxDOS). Si
// lleva tabla del +3e (PLUSIDEDOS) o no hay divMMC, se arranca el +3e; si no,
// con la divMMC activa, se reinicia la máquina actual para que esxDOS la lea.
void CKernel::MountHardDisk (const char *pPath)
{
	m_pSpectrum->GetSDCard ()->Insert (0);

	const char *pError = m_HardDisk.Open (pPath);
	if (pError != 0)
	{
		pError = T (pError);
		m_HDDPath[0] = '\0';
		ShowMessage (pError);
		return;
	}

	u8 Sector[512];
	boolean bPlus3e =    m_HardDisk.ReadSector (0, Sector)
			  && memcmp (Sector, "PLUSIDEDOS", 10) == 0;
	boolean bEsxDOS = !bPlus3e && m_pSpectrum->IsDivMMCActive ();

	if (!bEsxDOS && !m_pSpectrum->IsModelAvailable (ModelPlus3E))
	{
		m_HardDisk.Close ();
		m_HDDPath[0] = '\0';
		ShowMessage (T ("Falta roms/plus3e-es.rom (ver LEEME)"));
		return;
	}

	strncpy (m_HDDPath, pPath, MENU_PATH_LEN - 1);
	m_HDDPath[MENU_PATH_LEN - 1] = '\0';
	m_pSpectrum->GetSDCard ()->Insert (&m_HardDisk);

	// el +3e y esxDOS leen la tarjeta al arrancar
	SetModel (bEsxDOS ? m_pSpectrum->GetModel () : ModelPlus3E);

	const char *pName = pPath;
	for (const char *p = pPath; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pName = p + 1;
		}
	}
	CString Message;
	Message.Format ("%s  SD: %s (%u MB)", bEsxDOS ? "esxDOS" : "+3e", pName,
			m_HardDisk.GetSectors () / 2048);
	ShowMessage (Message);
}

// Mientras se habla con un servidor TNFS el bucle principal está parado: se
// dibuja el menú con un aviso para que se vea qué está pasando
void CKernel::BusyHandler (const char *pText, void *pParam)
{
	CKernel *pThis = static_cast<CKernel *> (pParam);
	TPixel *Overlay = pThis->m_pOverlay;
	memcpy (Overlay, pThis->m_pFrozen, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
	if (pThis->m_Menu.IsOpen ())
	{
		pThis->m_Menu.Draw (Overlay);
	}
	strncpy (pThis->m_Message, pText, sizeof pThis->m_Message - 1);
	pThis->m_Message[sizeof pThis->m_Message - 1] = '\0';
	pThis->m_nMessageFrames = 1;
	pThis->DrawMessage (Overlay);
	// al momento: con ShowFrame, si el núcleo 1 estaba ocupado el cuadro se
	// descartaba y durante una descarga por TNFS no se veía ningún aviso
	pThis->ShowFrameNow (Overlay);
}

// Aviso de la red: lo llama una tarea de red (la web, las descargas de ZXDB)
// antes de una operación larga, mientras el bucle principal está cedido y la
// emulación parada. Se pinta y se muestra al momento y además se deja como
// mensaje, para que siga viéndose un poco cuando la emulación vuelva.
void CKernel::NetBusyHandler (const char *pText, void *pParam)
{
	CKernel *pThis = static_cast<CKernel *> (pParam);
	TPixel *Overlay = pThis->m_pOverlay;
	if (pThis->m_Menu.IsOpen ())
	{
		memcpy (Overlay, pThis->m_pFrozen, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
		pThis->m_Menu.Draw (Overlay);
	}
	else
	{
		memcpy (Overlay, pThis->m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
	}
	pThis->ShowMessage (pText);
	pThis->DrawMessage (Overlay);
	pThis->ShowFrameNow (Overlay);
}

void CKernel::ShowFrameNow (const TPixel *pScreen)
{
#ifdef ARM_ALLOW_MULTI_CORE
	if (m_pVideoCore != 0)
	{
		m_pVideoCore->SubmitAndWait (pScreen, 100);
		return;
	}
#endif
	m_Video.Present (pScreen);
}

// "NET:servidor/ruta/fichero": se descarga a SD:/descargas/<servidor>/ y se carga
void CKernel::DownloadAndLoad (const char *pNetPath)
{
	unsigned nSize = m_TNFS.Download (pNetPath, m_pWork, WORK_BUFFER_SIZE);
	if (nSize == 0)
	{
		CString Message;
		Message.Format (T ("Error: %s"), T (m_TNFS.GetError ()));
		ShowMessage (Message);
		return;
	}

	// SD:/descargas/servidor/fichero
	const char *pHost = pNetPath + 4;
	const char *pSlash = strchr (pHost, '/');
	const char *pName = pSlash;
	for (const char *p = pSlash; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pName = p + 1;
		}
	}

	CString Dir, Path;
	Dir.Format ("SD:/descargas");
	f_mkdir (Dir);
	char Host[64];
	unsigned nHostLen = pSlash - pHost < (int) sizeof Host ? pSlash - pHost : sizeof Host - 1;
	memcpy (Host, pHost, nHostLen);
	Host[nHostLen] = '\0';
	Dir.Format ("SD:/descargas/%s", Host);
	f_mkdir (Dir);
	Path.Format ("%s/%s", (const char *) Dir, pName);

	if (!WriteFile (Path, m_pWork, nSize, FALSE))
	{
		ShowMessage (T ("No se pudo guardar la descarga en la SD"));
		return;
	}

	LoadAnyFile (Path);
}

unsigned CKernel::ReadWholeFile (const char *pPath, u8 *pBuffer, unsigned nMaxSize)
{
	FIL File;
	if (f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		return 0;
	}

	unsigned nSize = f_size (&File), nRead = 0;
	boolean bOK =    nSize > 0 && nSize <= nMaxSize
		      && f_read (&File, pBuffer, nSize, &nRead) == FR_OK && nRead == nSize;
	f_close (&File);

	return bOK ? nSize : 0;
}

// Disco del +3: el .DSK va a la unidad A: (si el modelo no es el +3, se cambia
// al +3 antes, con el disco ya dentro, para que "Loader" arranque de él).
// Sin ruta: disco nuevo formateado en la primera unidad libre (A:, luego B:).
void CKernel::InsertDisk (const char *pPath, int nForceUnit)
{
	if (!m_pSpectrum->HasDiskDrive () && !m_pSpectrum->IsModelAvailable (ModelPlus3))
	{
		ShowMessage (T ("Falta roms/plus3.rom (ver LEEME)"));
		return;
	}

	SaveDisks (TRUE);

	CPlus3Disk *pDisk = m_pSpectrum->GetDisk ();
	unsigned nUnit = 0;
	unsigned nSize = 0;

	if (pPath != 0)
	{
		nSize = ReadWholeFile (pPath, m_pWork, WORK_BUFFER_SIZE);
		if (nSize == 0)
		{
			ShowMessage (T ("No se pudo leer el disco"));
			return;
		}
	}
	else if (pDisk->HasDisk (0))
	{
		nUnit = pDisk->HasDisk (1) ? 0 : 1;
	}
	if (nForceUnit >= 0)
	{
		nUnit = (unsigned) nForceUnit;	// desde la pantalla de unidades
	}

	boolean bChangeModel = !m_pSpectrum->HasDiskDrive ();
	if (bChangeModel)
	{
		SetModel (ModelPlus3);
	}

	boolean bOK = pPath != 0 ? pDisk->InsertDSK (nUnit, m_pWork, nSize) : pDisk->InsertBlank (nUnit, m_bNewDisk720);
	if (!bOK)
	{
		ShowMessage (pPath != 0 ? T ("El fichero no es un disco .DSK válido") : T ("No se pudo crear el disco"));
		return;
	}

	if (pPath != 0)
	{
		strncpy (m_DSKPath[nUnit], pPath, MENU_PATH_LEN - 1);
		m_DSKPath[nUnit][MENU_PATH_LEN - 1] = '\0';
	}
	else
	{
		m_DSKPath[nUnit][0] = '\0';	// se le dará nombre al grabarlo
	}

	CString Message;
	const char *pName = pPath != 0 ? pPath : T ("disco nuevo");
	for (const char *p = pName; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pName = p + 1;
		}
	}
	Message.Format ("%s%s: %s%s", bChangeModel ? "Spectrum +3  " : "", nUnit == 0 ? "A" : "B", pName,
			pDisk->Is35 (nUnit) ? T (" (unidad de 3,5\")") : "");
	ShowMessage (Message);
}

void CKernel::EjectDisks (void)
{
	SaveDisks (TRUE);

	CPlus3Disk *pDisk = m_pSpectrum->GetDisk ();
	pDisk->Eject (0);
	pDisk->Eject (1);
	m_DSKPath[0][0] = m_DSKPath[1][0] = '\0';
	ShowMessage (T ("Discos expulsados"));
}

// Graba los discos modificados en su fichero (los nuevos, en saves/disk_NNN.dsk)
void CKernel::SaveDisks (boolean bForce)
{
	CPlus3Disk *pDisk = m_pSpectrum->GetDisk ();
	if (!bForce && pDisk->IsMotorOn ())
	{
		return;
	}

	for (unsigned i = 0; i < 2; i++)
	{
		if (!pDisk->IsModified (i))
		{
			continue;
		}

		unsigned nSize = pDisk->Save (i, m_pWork, WORK_BUFFER_SIZE);
		if (nSize == 0)
		{
			continue;
		}

		if (m_DSKPath[i][0] == '\0')
		{
			f_mkdir ("SD:/saves");
			CString Path;
			FILINFO Info;
			unsigned nNumber = 1;
			do
			{
				Path.Format ("SD:/saves/disk_%03u.dsk", nNumber++);
			}
			while (f_stat (Path, &Info) == FR_OK && nNumber < 1000);
			strncpy (m_DSKPath[i], Path, MENU_PATH_LEN - 1);
		}

		CString Message;
		if (WriteFile (m_DSKPath[i], m_pWork, nSize, FALSE))
		{
			pDisk->ClearModified (i);
			Message.Format (T ("Disco %c: grabado en %s"), 'A' + i, m_DSKPath[i] + 4);
		}
		else
		{
			Message.Format (T ("Error al grabar el disco %c:"), 'A' + i);
		}
		ShowMessage (Message);
	}
}

// Espera cediendo el control a las tareas de red
void CKernel::YieldDelay (unsigned nMilliseconds)
{
	u64 nEnd = CTimer::GetClockTicks64 () + nMilliseconds * 1000;
	while (CTimer::GetClockTicks64 () < nEnd)
	{
		m_Scheduler.Yield ();
		PollVideo ();
	}
}

// Muestra un cuadro: en la Pi 2-4 lo amplía el núcleo 1; si no, aquí mismo
void CKernel::ShowFrame (const TPixel *pScreen)
{
#ifdef ARM_ALLOW_MULTI_CORE
	if (m_pVideoCore != 0)
	{
		m_pVideoCore->Submit (pScreen);
		return;
	}
#endif
	m_Video.Present (pScreen);
}

void CKernel::PollVideo (void)
{
#ifdef ARM_ALLOW_MULTI_CORE
	if (m_pVideoCore != 0)
	{
		m_pVideoCore->Poll ();
	}
#endif
}

// Cambio de borde o scanlines: con el núcleo 1 parado mientras tanto
void CKernel::SetVideoMode (TBorderSize Border, boolean bScanlines)
{
#ifdef ARM_ALLOW_MULTI_CORE
	if (m_pVideoCore != 0)
	{
		m_pVideoCore->Pause ();
		m_Video.SetMode (Border, bScanlines);
		m_pVideoCore->Resume ();
		return;
	}
#endif
	m_Video.SetMode (Border, bScanlines);
}

boolean CKernel::WriteFile (const char *pPath, const u8 *pData, unsigned nSize, boolean bAppend)
{
	FIL File;
	if (f_open (&File, pPath, FA_WRITE | (bAppend ? FA_OPEN_APPEND : FA_CREATE_ALWAYS)) != FR_OK)
	{
		return FALSE;
	}

	unsigned nWritten = 0;
	FRESULT Result = f_write (&File, pData, nSize, &nWritten);
	f_close (&File);

	return Result == FR_OK && nWritten == nSize;
}

// Alt+W: graba la señal de la salida MIC (cualquier rutina de grabación, también
// las turbo) y la guarda como .TZX con un bloque CSW (0x18) en "saves"
// Alt+W graba la salida MIC del Spectrum; Alt+G, la entrada EAR (un casete real)
void CKernel::ToggleRecording (boolean bEarInput)
{
	if (!m_pSpectrum->IsRecording ())
	{
		if (!m_bSDMounted)
		{
			ShowMessage (T ("No hay tarjeta SD montada"));
			return;
		}

		if (bEarInput && m_pEarSource == 0)
		{
			ShowMessage (T ("Sin entrada de cinta (ear=gpio o ear=usb)"));
			return;
		}

		if (m_pSpectrum->StartRecording (bEarInput))
		{
			ShowMessage (bEarInput ? T ("Grabando el casete (Alt+G para parar)")
					       : T ("Grabando la salida MIC (Alt+W para parar)"));
		}
		return;
	}

	m_pSpectrum->StopRecording ();

	unsigned nLength, nPulses, nRate;
	const u8 *pData = m_pSpectrum->GetRecording (&nLength, &nPulses, &nRate);
	if (pData == 0 || nPulses < 100)
	{
		m_pSpectrum->FreeRecording ();
		ShowMessage (T ("No se ha grabado nada"));
		return;
	}

	f_mkdir ("SD:/saves");

	CString Path;
	FILINFO Info;
	unsigned nNumber = 1;
	do
	{
		Path.Format ("SD:/saves/rec_%03u.tzx", nNumber++);
	}
	while (f_stat (Path, &Info) == FR_OK && nNumber < 1000);

	// Cabecera TZX + bloque 0x18 (CSW, RLE)
	u8 Header[10 + 15];
	memcpy (Header, "ZXTape!\x1A\x01\x14", 10);
	u8 *p = Header + 10;
	u32 nBlockLen = 10 + nLength;
	p[0] = 0x18;
	p[1] = nBlockLen; p[2] = nBlockLen >> 8; p[3] = nBlockLen >> 16; p[4] = nBlockLen >> 24;
	p[5] = 0; p[6] = 0;
	p[7] = nRate; p[8] = nRate >> 8; p[9] = nRate >> 16;
	p[10] = 1;
	p[11] = nPulses; p[12] = nPulses >> 8; p[13] = nPulses >> 16; p[14] = nPulses >> 24;

	boolean bOK =    WriteFile (Path, Header, sizeof Header, FALSE)
		      && WriteFile (Path, pData, nLength, TRUE);

	// la grabación pasa a ser la cinta actual, rebobinada
	if (bOK)
	{
		u8 *pTZX = new u8[sizeof Header + nLength];
		if (pTZX != 0)
		{
			memcpy (pTZX, Header, sizeof Header);
			memcpy (pTZX + sizeof Header, pData, nLength);
			if (m_Tape.InsertTZX (pTZX, sizeof Header + nLength, (const char *) Path + 10))
			{
				m_pSpectrum->TapeInserted ();
				strncpy (m_TapePath, Path, sizeof m_TapePath - 1);
				m_TapePath[sizeof m_TapePath - 1] = '\0';
				m_bSaveTape = FALSE;
			}
			delete [] pTZX;
		}
	}

	m_pSpectrum->FreeRecording ();

	CString Message;
	if (bOK)
	{
		Message.Format (T ("Grabado: saves/%s (%u pulsos)"), (const char *) Path + 10, nPulses);
	}
	else
	{
		Message = T ("Error al grabar en la SD");
	}
	ShowMessage (Message);
}

unsigned CKernel::CountMicrodrives (void)
{
	unsigned nCount = 0;
	for (unsigned i = 0; i < MICRODRIVES; i++)
	{
		if (m_pSpectrum->GetInterface1 ()->GetDrive (i)->IsCartridge ())
		{
			nCount++;
		}
	}
	return nCount;
}

// Inserta un .MDR (o un cartucho en blanco si pPath es 0) en la primera unidad libre
boolean CKernel::InsertMicrodrive (const char *pPath, int nForceDrive)
{
	if (!m_pSpectrum->HasIF1ROM ())
	{
		ShowMessage (T ("Falta roms/if1.rom (ver LEEME)"));
		return FALSE;
	}

	if (m_pSpectrum->IsDivMMCActive ())
	{
		// los dos usan los puertos 0xE7/0xEB y se paginan en 0x0008
		ShowMessage (T ("El Interface 1 no convive con la divMMC"));
		return FALSE;
	}

	if (m_pSpectrum->IsSpectranetEnabled ())
	{
		ShowMessage (T ("El Interface 1 no convive con la Spectranet"));
		return FALSE;
	}

	if (m_nIF1Mode == 0 || m_pSpectrum->GetTiming ()->CodeModel == CodeModelPlus3)
	{
		ShowMessage (m_nIF1Mode == 0 ? T ("Interface 1 desactivado (if1=0)")
					     : T ("El +2A no admite el Interface 1"));
		return FALSE;
	}

	CInterface1 *pIF1 = m_pSpectrum->GetInterface1 ();
	unsigned nDrive = 0;
	if (nForceDrive >= 0 && nForceDrive < MICRODRIVES)
	{
		// desde la pantalla de unidades: si ya tenía cartucho, se graba y se saca
		nDrive = (unsigned) nForceDrive;
		if (pIF1->GetDrive (nDrive)->IsCartridge ())
		{
			SaveMicrodrives (TRUE);
			pIF1->GetDrive (nDrive)->Eject ();
			m_MDRPath[nDrive][0] = '\0';
		}
	}
	while (nForceDrive < 0 && nDrive < MICRODRIVES && pIF1->GetDrive (nDrive)->IsCartridge ())
	{
		nDrive++;
	}
	if (nDrive >= MICRODRIVES)
	{
		ShowMessage (T ("Las 8 unidades están ocupadas (Alt+M las vacía)"));
		return FALSE;
	}

	CMicrodrive *pDrive = pIF1->GetDrive (nDrive);
	CString Message;

	if (pPath == 0)
	{
		if (!pDrive->InsertBlank ())
		{
			ShowMessage (T ("Sin memoria"));
			return FALSE;
		}
		m_MDRPath[nDrive][0] = '\0';	// se le dará nombre al grabarlo
		Message.Format (T ("Microdrive %u: cartucho nuevo (FORMAT \"m\";%u;\"nombre\")"),
				nDrive + 1, nDrive + 1);
	}
	else
	{
		u8 *Buffer = m_pWork;
		const unsigned nBufferSize = WORK_BUFFER_SIZE;
		FIL File;
		unsigned nRead = 0;
		boolean bOK = f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) == FR_OK;
		if (bOK)
		{
			unsigned nSize = f_size (&File);
			bOK =    nSize <= nBufferSize
			      && f_read (&File, Buffer, nSize, &nRead) == FR_OK && nRead == nSize;
			f_close (&File);
		}

		size_t nPathLen = strlen (pPath);
		boolean bMDV = nPathLen > 4 && strcasecmp (pPath + nPathLen - 4, ".mdv") == 0;
		if (!bOK || !(bMDV ? pDrive->InsertMDV (Buffer, nRead) : pDrive->InsertMDR (Buffer, nRead)))
		{
			ShowMessage (bMDV ? T ("Fichero .MDV no válido") : T ("Fichero .MDR no válido"));
			return FALSE;
		}

		strncpy (m_MDRPath[nDrive], pPath, MENU_PATH_LEN - 1);
		m_MDRPath[nDrive][MENU_PATH_LEN - 1] = '\0';

		const char *pName = pPath;
		for (const char *p = pPath; *p != '\0'; p++)
		{
			if (*p == '/')
			{
				pName = p + 1;
			}
		}
		Message.Format (T ("Microdrive %u: %s%s"), nDrive + 1, pName,
				pDrive->IsWriteProtected () ? T (" (protegido)") : "");
	}

	m_pSpectrum->SetIF1Connected (TRUE);
	ShowMessage (Message);

	return TRUE;
}

// Graba en la SD los cartuchos modificados, cuando su motor está parado
void CKernel::SaveMicrodrives (boolean bForce)
{
	CInterface1 *pIF1 = m_pSpectrum->GetInterface1 ();
	int nRunning = pIF1->GetRunningDrive ();

	for (unsigned i = 0; i < MICRODRIVES; i++)
	{
		CMicrodrive *pDrive = pIF1->GetDrive (i);
		if (   !pDrive->IsCartridge () || !pDrive->IsModified ()
		    || (!bForce && (int) i == nRunning))
		{
			continue;
		}

		// Un cartucho que vino de un .MDV (JSpeccy, CP/M de LEC) se graba
		// siempre como .MDV. Los nuevos y los .MDR: .MDR si todo son sectores
		// de Sinclair; si no (CP/M de LEC...), .MDV en bruto.
		u8 *Buffer = m_pWork;
		size_t nPathLen = strlen (m_MDRPath[i]);
		boolean bWasMDV = nPathLen > 4 && strcasecmp (m_MDRPath[i] + nPathLen - 4, ".mdv") == 0;
		boolean bSinclair = !bWasMDV && pDrive->IsSinclairFormat ();
		unsigned nSize = bSinclair ? pDrive->ExportMDR (Buffer, WORK_BUFFER_SIZE)
					   : pDrive->ExportMDV (Buffer, WORK_BUFFER_SIZE);
		if (nSize == 0)
		{
			continue;
		}
		const char *pExt = bSinclair ? "mdr" : "mdv";

		if (m_MDRPath[i][0] == '\0')
		{
			f_mkdir ("SD:/saves");
			CString Path;
			FILINFO Info;
			unsigned nNumber = 1;
			do
			{
				Path.Format ("SD:/saves/mdr_%03u.%s", nNumber++, pExt);
			}
			while (f_stat (Path, &Info) == FR_OK && nNumber < 1000);
			strncpy (m_MDRPath[i], Path, MENU_PATH_LEN - 1);
		}
		else
		{
			// mismo nombre con la extensión del formato que toca
			size_t nLen = strlen (m_MDRPath[i]);
			if (nLen > 4 && m_MDRPath[i][nLen - 4] == '.')
			{
				strcpy (m_MDRPath[i] + nLen - 3, pExt);
			}
		}

		CString Message;
		if (WriteFile (m_MDRPath[i], Buffer, nSize, FALSE))
		{
			pDrive->ClearModified ();
			Message.Format (T ("Microdrive %u grabado en %s"), i + 1, m_MDRPath[i] + 4);
		}
		else
		{
			Message.Format (T ("Error al grabar el microdrive %u"), i + 1);
		}
		ShowMessage (Message);
	}
}

void CKernel::EjectMicrodrives (void)
{
	SaveMicrodrives (TRUE);

	for (unsigned i = 0; i < MICRODRIVES; i++)
	{
		m_pSpectrum->GetInterface1 ()->GetDrive (i)->Eject ();
		m_MDRPath[i][0] = '\0';
	}

	if (m_nIF1Mode == 2)
	{
		m_pSpectrum->SetIF1Connected (FALSE);
	}

	ShowMessage (T ("Microdrives vacíos"));
}

// Cartucho del Interface 2 (.ROM de hasta 16 KB)
boolean CKernel::LoadCartridge (const char *pPath)
{
	static u8 Buffer[0x4000];

	if (m_pSpectrum->GetTiming ()->CodeModel == CodeModelPlus3)
	{
		ShowMessage (T ("El +2A no admite cartuchos"));
		return FALSE;
	}

	FIL File;
	if (f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		ShowMessage (T ("No se puede abrir el fichero"));
		return FALSE;
	}

	unsigned nSize = f_size (&File);
	unsigned nRead = 0;
	boolean bOK =    nSize > 0 && nSize <= SNAPSHOT_MAX_SIZE
		      && f_read (&File, Buffer, nSize, &nRead) == FR_OK && nRead == nSize;
	f_close (&File);

	if (!bOK || !m_pSpectrum->InsertCartridge (Buffer, nSize))
	{
		ShowMessage (T ("Cartucho no válido (máximo 16 KB)"));
		return FALSE;
	}

	const char *pName = pPath;
	for (const char *p = pPath; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pName = p + 1;
		}
	}
	strncpy (m_CartridgeName, pName, sizeof m_CartridgeName - 1);
	m_CartridgeName[sizeof m_CartridgeName - 1] = '\0';

	m_nNextFrameTime = 0;

	CString Message;
	Message.Format (T ("Cartucho: %s"), pName);
	ShowMessage (Message);

	return TRUE;
}

boolean CKernel::LoadSnapshot (const char *pPath)
{
	u8 *Buffer = m_pWork;	// SNAPSHOT_MAX_SIZE < WORK_BUFFER_SIZE

	FIL File;
	if (f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		ShowMessage (T ("No se puede abrir el fichero"));
		return FALSE;
	}

	unsigned nSize = f_size (&File);
	unsigned nRead = 0;
	boolean bOK =    nSize > 0 && nSize <= SNAPSHOT_MAX_SIZE
		      && f_read (&File, Buffer, nSize, &nRead) == FR_OK && nRead == nSize;
	f_close (&File);

	if (!bOK)
	{
		ShowMessage (T ("Snapshot no válido"));
		return FALSE;
	}

	const char *pError = CSnapshot::Load (m_pSpectrum, Buffer, nSize, pPath);
	if (pError != 0)
	{
		pError = T (pError);
		ShowMessage (pError);
		return FALSE;
	}

	ApplySpeaker ();		// el snapshot puede haber cambiado de máquina
	m_nNextFrameTime = 0;

	const char *pName = pPath;
	for (const char *p = pPath; *p != '\0'; p++)
	{
		if (*p == '/')
		{
			pName = p + 1;
		}
	}

	CString Message;
	TModel Missing = m_pSpectrum->TakeFallback ();
	if (Missing != ModelCount)
	{
		// el snapshot pedía una máquina sin ROM: se ha cargado en el 128K
		Message.Format (T ("Falta la ROM de %s: cargado en 128K"), MachineTiming[Missing].pShortName);
	}
	else
	{
		Message.Format ("%s (%s)", pName, m_pSpectrum->GetTiming ()->pShortName);
	}
	ShowMessage (Message);

	return TRUE;
}

// F8: graba el estado en SD:/snaps/<nombre>_NNN.z80, con el nombre de la cinta
// actual (o "snap" si no hay ninguna)
boolean CKernel::SaveSnapshot (void)
{
	if (!m_bSDMounted)
	{
		ShowMessage (T ("No hay tarjeta SD montada"));
		return FALSE;
	}

	u8 *Buffer = m_pWork;	// SNAPSHOT_MAX_SIZE < WORK_BUFFER_SIZE
	if (m_pSpectrum->IsLECPaged ())
	{
		ShowMessage (T ("Con la memoria LEC en uso no se graban snapshots"));
		return FALSE;
	}

	static const char *SnapExt[3] = {"z80", "sna", "szx"};
	unsigned nSize;
	switch (m_nSnapFormat)
	{
	case 1:  nSize = CSnapshot::SaveSNA (m_pSpectrum, Buffer, SNAPSHOT_MAX_SIZE);	break;
	case 2:  nSize = CSnapshot::SaveSZX (m_pSpectrum, Buffer, SNAPSHOT_MAX_SIZE);	break;
	default: nSize = CSnapshot::SaveZ80 (m_pSpectrum, Buffer, SNAPSHOT_MAX_SIZE);	break;
	}
	if (nSize == 0)
	{
		ShowMessage (T ("Error al crear el snapshot"));
		return FALSE;
	}

	char Base[32];
	unsigned n = 0;
	const char *pTapeName = m_Tape.IsInserted () ? m_Tape.GetName () : "";
	for (const char *p = pTapeName; *p != '\0' && *p != '.' && n < sizeof Base - 1; p++)
	{
		char ch = *p;
		if (   (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
		    || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_')
		{
			Base[n++] = ch;
		}
		else if (ch == ' ')
		{
			Base[n++] = '_';
		}
	}
	if (n == 0)
	{
		strcpy (Base, "snap");
		n = 4;
	}
	Base[n] = '\0';

	f_mkdir ("SD:/snaps");

	CString Path;
	FILINFO Info;
	unsigned nNumber = 1;
	do
	{
		Path.Format ("SD:/snaps/%s_%03u.%s", Base, nNumber++, SnapExt[m_nSnapFormat]);
	}
	while (f_stat (Path, &Info) == FR_OK && nNumber < 1000);

	FIL File;
	unsigned nWritten = 0;
	boolean bOK =    f_open (&File, Path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK
		      && f_write (&File, Buffer, nSize, &nWritten) == FR_OK
		      && nWritten == nSize;
	f_close (&File);

	if (!bOK)
	{
		ShowMessage (T ("Error al grabar el snapshot"));
		return FALSE;
	}

	CString Message;
	Message.Format (T ("Snapshot: %s"), (const char *) Path + 4);	// sin "SD:/"
	ShowMessage (Message);

	return TRUE;
}

boolean CKernel::LoadTape (const char *pPath)
{
	FIL File;
	if (f_open (&File, pPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		ShowMessage (T ("No se puede abrir el fichero"));
		return FALSE;
	}

	unsigned nSize = f_size (&File);
	if (nSize == 0 || nSize > TAPE_MAX_SIZE)
	{
		f_close (&File);
		ShowMessage (T ("Tamaño de cinta no válido"));
		return FALSE;
	}

	u8 *pBuffer = new u8[nSize];
	if (pBuffer == 0)
	{
		f_close (&File);
		ShowMessage (T ("Sin memoria"));
		return FALSE;
	}

	unsigned nRead = 0;
	FRESULT Result = f_read (&File, pBuffer, nSize, &nRead);
	f_close (&File);

	// nombre sin la ruta
	const char *pName = pPath;
	for (const char *p = pPath; *p != '\0'; p++)
	{
		if (*p == '/' || *p == ':')
		{
			pName = p + 1;
		}
	}

	size_t nNameLen = strlen (pName);
	boolean bTZX = nNameLen > 4 && strcasecmp (pName + nNameLen - 4, ".tzx") == 0;
	boolean bCSW = nNameLen > 4 && strcasecmp (pName + nNameLen - 4, ".csw") == 0;
	boolean bPZX = nNameLen > 4 && strcasecmp (pName + nNameLen - 4, ".pzx") == 0;

	boolean bOK = Result == FR_OK && nRead == nSize;
	if (bOK)
	{
		if (bPZX)
		{
			bOK = m_Tape.InsertPZX (pBuffer, nSize, pName);
		}
		else if (bTZX)
		{
			bOK = m_Tape.InsertTZX (pBuffer, nSize, pName);
		}
		else if (bCSW)
		{
			bOK = m_Tape.InsertCSW (pBuffer, nSize, pName);
			bTZX = TRUE;	// no es un .TAP: SAVE no debe añadir a ella
		}
		else
		{
			bOK = m_Tape.InsertTAP (pBuffer, nSize, pName);
		}
	}
	delete [] pBuffer;

	if (!bOK)
	{
		ShowMessage (bPZX ? T ("Fichero PZX no válido")
				  : bCSW ? T ("Fichero CSW no válido") : (bTZX ? T ("Fichero TZX no válido") : T ("Fichero TAP no válido")));
		return FALSE;
	}

	if (m_Tape.GetUnsupportedBlocks () > 0)
	{
		m_Logger.Write (FromKernel, LogWarning, "%s: %u unsupported TZX blocks",
				pName, m_Tape.GetUnsupportedBlocks ());
	}

	m_pSpectrum->TapeInserted ();

	strncpy (m_TapePath, pPath, sizeof m_TapePath - 1);
	m_TapePath[sizeof m_TapePath - 1] = '\0';
	// las cintas de "saves" admiten más grabaciones (si son del formato elegido)
	m_bSaveTape =    !bCSW && !bPZX && strncasecmp (pPath, "SD:/saves/", 10) == 0
		      && bTZX == m_bSaveTZX;

	CString Message;
	Message.Format (T ("Cinta: %s"), pName);
	ShowMessage (Message);

	CheckTapeModel ();

	return TRUE;
}

// Modelos y su número en el bloque 0x33 del TZX (información de hardware,
// tipo 0 = ordenadores)
static const struct { TModel Model; u8 nId; } TZXMachines[] =
{
	{Model48K,	0x01},		// en este orden se busca uno en el que funcione
	{Model128K,	0x03},
	{ModelPlus2,	0x04},
	{ModelPlus3,	0x05},		// +2A/+3
	{ModelPlus2A,	0x05},
	{ModelPlus3E,	0x05},
	{Model16K,	0x00},
};

// Si el TZX dice que no funciona en el modelo actual (bloque 0x33), se
// pregunta si se cambia a uno en el que sí funcione. No se cambia solo.
void CKernel::CheckTapeModel (void)
{
	TModel Current = m_pSpectrum->GetModel ();
	int nCurrent = -1;
	for (const auto &M : TZXMachines)
	{
		if (M.Model == Current)
		{
			nCurrent = m_Tape.GetHardwareInfo (0, M.nId);
			break;
		}
	}
	if (nCurrent != 3)
	{
		return;				// no dice nada, o funciona
	}

	// uno que la cinta diga que funciona; si no dice ninguno, el 48K
	TModel Suggested = Current;
	for (const auto &M : TZXMachines)
	{
		int nInfo = m_Tape.GetHardwareInfo (0, M.nId);
		if (M.Model != Current && nInfo >= 0 && nInfo <= 2)
		{
			Suggested = M.Model;
			break;
		}
	}
	if (Suggested == Current && Current != Model48K && m_Tape.GetHardwareInfo (0, 0x01) != 3)
	{
		Suggested = Model48K;
	}
	if (Suggested == Current)
	{
		ShowMessage (T ("Esta cinta no funciona en este modelo"));
		return;
	}

	m_SuggestedModel = Suggested;
	m_bModelChoice = TRUE;
	if (!m_Menu.IsOpen ())
	{
		memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
		OpenMenu ();
		m_Keyboard.SetCaptured (TRUE);
	}
	CString Title, Change, Keep;
	Title.Format (T ("Esta cinta no va en el %s"), m_pSpectrum->GetTiming ()->pName);
	Change.Format (T ("Cambiar al %s"), MachineTiming[Suggested].pName);
	Keep.Format (T ("Seguir con el %s"), m_pSpectrum->GetTiming ()->pName);
	m_Menu.OpenChoice (Title);
	m_Menu.SetChoiceItem (0, Change);
	m_Menu.SetChoiceItem (1, Keep);
}

// Modelo del altavoz (ver CAudioMixer::TSpeaker). Automático, como en Fuse:
// el 16K y el 48K suenan por su altavoz interno; el 128K, el +2 y el +3, por
// el televisor
void CKernel::ApplySpeaker (void)
{
	CAudioMixer::TSpeaker Speaker;
	if (strcmp (m_Speaker, "beeper") == 0)		Speaker = CAudioMixer::SpeakerBeeper;
	else if (strcmp (m_Speaker, "tv") == 0)		Speaker = CAudioMixer::SpeakerTV;
	else if (strcmp (m_Speaker, "off") == 0)	Speaker = CAudioMixer::SpeakerOff;
	else
	{
		TModel Model = m_pSpectrum->GetModel ();
		Speaker = Model == Model16K || Model == Model48K ? CAudioMixer::SpeakerBeeper
								  : CAudioMixer::SpeakerTV;
	}
	m_pSpectrum->GetAudio ()->SetSpeaker (Speaker);
}

void CKernel::SetModel (TModel Model)
{
	EsxFolderToImage (TRUE);
	m_pSpectrum->SetModel (Model);
	ApplySpeaker ();
	m_nNextFrameTime = 0;

	CString Message;
	Message.Format (T ("Modelo: %s"), m_pSpectrum->GetTiming ()->pName);
	ShowMessage (Message);
}

// En los menús, los cursores y RePág/AvPág se repiten si se mantienen pulsados:
// la primera vez a los 400 ms y luego cada 60 ms. Vale para el teclado USB y
// para el teclado GPIO (CAPS SHIFT + 5/6/7/8).
#define REPEAT_DELAY_US		400000
#define REPEAT_RATE_US		60000

void CKernel::UpdateKeyRepeat (void)
{
	if (!m_Menu.IsOpen ())
	{
		m_nRepeatKey = 0;
		return;
	}

	u8 nHeld = m_Keyboard.GetHeldNavKey ();

	// teclado GPIO (en el menú las teclas van capturadas): CAPS SHIFT + 5..8
	if (nHeld == 0 && m_bGPIOKeyboard && (m_PrevGPIOMatrix[0] & 0x01))
	{
		if (m_PrevGPIOMatrix[4] & 0x10)		nHeld = HID_KEY_DOWN;	// 6
		else if (m_PrevGPIOMatrix[4] & 0x08)	nHeld = HID_KEY_UP;	// 7
		else if (m_PrevGPIOMatrix[4] & 0x04)	nHeld = HID_KEY_RIGHT;	// 8
		else if (m_PrevGPIOMatrix[3] & 0x10)	nHeld = HID_KEY_LEFT;	// 5
	}

	u64 nNow = CTimer::GetClockTicks64 ();
	if (nHeld == 0)
	{
		m_nRepeatKey = 0;
		return;
	}

	if (nHeld != m_nRepeatKey)
	{
		// recién pulsada: la primera pulsación ya llegó como evento normal
		m_nRepeatKey = nHeld;
		m_nRepeatNext = nNow + REPEAT_DELAY_US;
		return;
	}

	// si el bucle va lento (por ejemplo, al dibujar), se recuperan las
	// repeticiones atrasadas, hasta 8 de golpe, para que el ritmo no dependa de él
	for (unsigned n = 0; n < 8 && nNow >= m_nRepeatNext; n++)
	{
		TKeyEvent Event;
		Event.nKey = nHeld;
		Event.nModifiers = 0;
		HandleKeyEvent (Event);
		m_nRepeatNext += REPEAT_RATE_US;
	}
	if (nNow >= m_nRepeatNext)
	{
		m_nRepeatNext = nNow + REPEAT_RATE_US;
	}
}

void CKernel::HandleKeyEvent (const TKeyEvent &rEvent)
{
	if (m_Menu.IsOpen ())
	{
		TMenuResult Result = m_Menu.HandleKey (rEvent);
		if (Result == MenuResultFileSelected)
		{
			int nUnit = m_Menu.TakeUnitTarget ();
			if (nUnit >= 0)
			{
				MountUnit ((unsigned) nUnit, m_Menu.GetSelectedPath ());
				UpdateUnitLines ();
				m_Menu.OpenUnits ();		// vuelta a la pantalla de unidades
			}
			else
			{
				LoadAnyFile (m_Menu.GetSelectedPath ());
			}
		}
		else if (Result == MenuResultTapeInvert)
		{
			m_Tape.SetInverted (!m_Tape.IsInverted ());
			ShowMessage (m_Tape.IsInverted () ? T ("Cinta: polaridad invertida") : T ("Cinta: polaridad normal"));
		}
		else if (Result == MenuResultTapeSeek)
		{
			// la cinta se queda parada al principio del bloque elegido; la
			// carga instantánea lee desde ahí y en rápida o real arranca sola
			// cuando el programa la lee
			unsigned nBlock = m_Menu.GetSelectedTapeBlock ();
			m_Tape.SeekBlock (nBlock);
			m_pSpectrum->TapeInserted ();
			CString Message;
			Message.Format (T ("Cinta en el bloque %u"), nBlock + 1);
			ShowMessage (Message);
		}
		else if (Result == MenuResultUnitEject)
		{
			EjectUnit (m_Menu.GetSelectedUnit ());
			UpdateUnitLines ();
		}
		else if (Result == MenuResultInput)
		{
			// Alt+V: las teclas escritas se pulsan a la vez al volver al Spectrum
			const char *pKeys = m_Menu.GetInput ();
			if (m_Keyboard.HoldKeys (pKeys, MULTIKEY_FRAMES) != 0)
			{
				CString Message;
				Message.Format (T ("Pulsando a la vez: %s"), pKeys);
				ShowMessage (Message);
			}
			else
			{
				ShowMessage (T ("No hay teclas que pulsar"));
			}
		}
		else if ((Result == MenuResultChoice || Result == MenuResultChoiceCancel) && m_bDiskChoice)
		{
			m_bDiskChoice = FALSE;
			if (Result == MenuResultChoice)
			{
				m_bNewDisk720 = m_Menu.GetChoice () == 1;
				m_Menu.Close ();
				InsertDisk (0);
				m_bNewDisk720 = FALSE;
			}
		}
		else if ((Result == MenuResultChoice || Result == MenuResultChoiceCancel) && m_bModelChoice)
		{
			// la cinta no funciona en este modelo (bloque 0x33 del TZX)
			m_bModelChoice = FALSE;
			if (Result == MenuResultChoice && m_Menu.GetChoice () == 0)
			{
				m_Menu.Close ();
				SetModel (m_SuggestedModel);
			}
		}
		else if (Result == MenuResultChoice && m_bRebootChoice)
		{
			m_bRebootChoice = FALSE;
			unsigned nChoice = m_Menu.GetChoice ();
			if (nChoice == 0)			// no: vuelta a la configuración
			{
				m_Menu.OpenSettings ();
			}
			else
			{
				RebootPi (m_Settings.IsDirty () && nChoice == 1);
			}
		}
		else if (Result == MenuResultChoiceCancel && m_bRebootChoice)
		{
			m_bRebootChoice = FALSE;
			m_Menu.OpenSettings ();
		}
		else if (Result == MenuResultChoice)
		{
			m_Tape.Select (m_Menu.GetChoice ());	// bloque 0x28: sigue la carga
			m_pSpectrum->TapePlay ();
		}
		else if (Result == MenuResultChoiceCancel)
		{
			m_Tape.CancelSelect ();
			m_pSpectrum->TapePlay ();
		}
		else if (Result == MenuResultReboot)
		{
			// confirmación (y aviso si hay cambios sin guardar)
			m_bRebootChoice = TRUE;
			if (m_Settings.IsDirty ())
			{
				m_Menu.OpenChoice (T ("¿Reiniciar? Hay cambios sin guardar"));
				m_Menu.SetChoiceItem (0, T ("No, volver a la configuración"));
				m_Menu.SetChoiceItem (1, T ("Guardar los cambios y reiniciar"));
				m_Menu.SetChoiceItem (2, T ("Reiniciar sin guardar"));
			}
			else
			{
				m_Menu.OpenChoice (T ("¿Reiniciar la Raspberry Pi?"));
				m_Menu.SetChoiceItem (0, T ("No, volver a la configuración"));
				m_Menu.SetChoiceItem (1, T ("Sí, reiniciar"));
			}
		}
		else if (Result == MenuResultOpenUnits)
		{
			// desde la ayuda (U): sin esto se veían las unidades como estaban
			// la última vez que se abrió la pantalla con Alt+U
			OpenUnitsScreen ();
		}
		else if (Result == MenuResultOpenSettings)
		{
			// sin cambios pendientes, se vuelve a leer cmdline.txt (puede
			// haberse cambiado desde la web)
			if (!m_Settings.IsDirty ())
			{
				LoadSettings ();
			}
			m_Menu.OpenSettings ();
		}
		else if (Result == MenuResultSetting)
		{
			ApplySetting (m_Menu.GetChangedSetting ());
		}
		else if (Result == MenuResultSaveSettings)
		{
			SaveSettings ();
		}

		if (!m_Menu.IsOpen ())
		{
			MenuClosed ();
			m_Keyboard.SetCaptured (FALSE);
			m_Keyboard.ReleaseAll ();
			m_nNextFrameTime = 0;
		}
		return;
	}

	boolean bAlt = (rEvent.nModifiers & (HID_MOD_LALT | HID_MOD_RALT)) != 0;

	const TKeyBinding *pBinding = 0;
	for (unsigned i = 0; i < sizeof KeyBindings / sizeof KeyBindings[0]; i++)
	{
		if (KeyBindings[i].nKey == rEvent.nKey && KeyBindings[i].bAlt == bAlt)
		{
			pBinding = &KeyBindings[i];
			break;
		}
	}

	if (pBinding == 0)
	{
		return;
	}

	CString Message;

	switch (pBinding->Action)
	{
	case ActionHelp:
		memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
		UpdateStatusLines ();
		OpenMenu ();
		m_Menu.OpenHelp ();
		m_Keyboard.SetCaptured (TRUE);
		break;

	case ActionKeyHelp:
		memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
		OpenMenu ();
		m_Menu.OpenKeyHelp ();
		m_Keyboard.SetCaptured (TRUE);
		break;

	case ActionTapePlayStop:
		if (!m_Tape.IsInserted ())
		{
			ShowMessage (T ("No hay cinta"));
		}
		else if (m_Tape.IsPlaying ())
		{
			m_pSpectrum->TapeStop ();
			ShowMessage (T ("Cinta parada"));
		}
		else
		{
			m_pSpectrum->TapePlay ();
			ShowMessage (m_Tape.IsPlaying () ? T ("Cinta en marcha") : T ("Fin de la cinta"));
		}
		break;

	case ActionTapeRewind:		// F3: navegador de bloques (Inicio = rebobinar)
		if (!m_Tape.IsInserted ())
		{
			ShowMessage (T ("No hay ninguna cinta puesta"));
			break;
		}
		if (!m_Menu.IsOpen ())
		{
			memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
			OpenMenu ();
			m_Keyboard.SetCaptured (TRUE);
		}
		m_Menu.OpenTape (&m_Tape);
		break;

	case ActionJoystick:
		m_Keyboard.SetJoystickMode ((TJoystickMode) ((m_Keyboard.GetJoystickMode () + 1) % JoystickModeCount));
		Message.Format (T ("Joystick: %s"), T (JoystickNames[m_Keyboard.GetJoystickMode ()]));
		ShowMessage (Message);
		SyncSetting ("joystick");
		break;

	case ActionNMI:
		m_pSpectrum->TriggerNMI ();
		ShowMessage (m_pSpectrum->IsMultifaceAvailable () ? "Multiface" : "NMI");
		break;

	case ActionTapeMode:
		m_pSpectrum->SetTapeMode ((TTapeMode) ((m_pSpectrum->GetTapeMode () + 1) % TapeModeCount));
		Message.Format (T ("Carga: %s"), T (TapeModeNames[m_pSpectrum->GetTapeMode ()]));
		ShowMessage (Message);
		SyncSetting ("tape");
		break;

	case ActionModel:
	{
		unsigned nIndex = 0;
		while (nIndex < ModelCount && ModelCycle[nIndex] != m_pSpectrum->GetModel ())
		{
			nIndex++;
		}
		// siguiente modelo con ROM (los que no tienen se saltan)
		do
		{
			nIndex = (nIndex + 1) % ModelCount;
		}
		while (!m_pSpectrum->IsModelAvailable (ModelCycle[nIndex]));
		SetModel (ModelCycle[nIndex]);
	}
		SyncSetting ("model");
		break;

	case ActionReset:
		EsxFolderToImage (TRUE);	// esxDOS: lo nuevo de la carpeta, antes de arrancar
		m_pSpectrum->Reset ();
		ShowMessage (T ("Reset"));
		break;

	case ActionPause:
		m_bPaused = !m_bPaused;
		m_nNextFrameTime = 0;
		break;

	case ActionSaveSnapshot:
		SaveSnapshot ();
		break;

	case ActionSpeed:
	{
		unsigned nIndex = 0;
		while (nIndex < sizeof Speeds / sizeof Speeds[0] && Speeds[nIndex] != m_nSpeed)
		{
			nIndex++;
		}
		m_nSpeed = Speeds[(nIndex + 1) % (sizeof Speeds / sizeof Speeds[0])];
		m_nNextFrameTime = 0;
		Message.Format (T ("Velocidad: x%u%s"), m_nSpeed, m_nSpeed > 1 ? T (" (sin sonido)") : "");
		ShowMessage (Message);
		SyncSetting ("speed");
		break;
	}

	case ActionSpectranet:
		ToggleSpectranet ();
		break;

	case ActionDivMMC:
		ToggleDivMMC ();
		break;

	case ActionLEC:
		ToggleLEC ();
		break;

	case ActionBlankMicrodrive:
		InsertMicrodrive (0);
		break;

	case ActionEjectMicrodrives:
		EjectMicrodrives ();
		break;

	case ActionRecord:
		ToggleRecording (FALSE);
		break;

	case ActionRecordEar:
		ToggleRecording (TRUE);
		break;

	case ActionKeyDebug:
		m_bKeyDebug = !m_bKeyDebug;
		break;

	case ActionMultiKey:
		// Alt+V: se escriben las teclas y se pulsan todas a la vez (trucos como
		// VICTOR en Profanation, con teclados que no registran tantas juntas)
		if (!m_Menu.IsOpen ())
		{
			memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
			OpenMenu ();
			m_Keyboard.SetCaptured (TRUE);
		}
		m_Menu.OpenInput (T ("Pulsar varias teclas a la vez"),
				  T ("Teclas que se pulsan a la vez (1,5 s):"),
				  T ("letras, números y espacio. Ej.: VICTOR"));
		break;

	case ActionBlankDisk:
		// disco nuevo: se pregunta el formato (3" de siempre o 3,5" de 720K)
		if (!m_Menu.IsOpen ())
		{
			memcpy (m_pFrozen, m_pSpectrum->GetScreenBuffer (), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel));
			OpenMenu ();
			m_Keyboard.SetCaptured (TRUE);
		}
		m_bDiskChoice = TRUE;
		m_Menu.OpenChoice (T ("Disco nuevo del +3"));
		m_Menu.SetChoiceItem (0, T ("180K (3\", una cara, 40 pistas)"));
		m_Menu.SetChoiceItem (1, T ("720K (3,5\", dos caras, 80 pistas)"));
		break;

	case ActionEjectDisks:
		EjectDisks ();
		break;

	case ActionBorder:
		m_BorderSize = (TBorderSize) ((m_BorderSize + 1) % BorderSizeCount);
		SetVideoMode (m_BorderSize, m_bScanlines);
		Message.Format (T ("Borde: %s"), T (CVideo::GetBorderName (m_BorderSize)));
		ShowMessage (Message);
		SyncSetting ("border");
		break;

	case ActionScanlines:
		m_bScanlines = !m_bScanlines;
		SetVideoMode (m_BorderSize, m_bScanlines);
		if (m_bScanlines && !m_Video.GetScanlines ())
		{
			m_bScanlines = FALSE;
			ShowMessage (T ("Scanlines: pantalla demasiado pequeña"));
		}
		else
		{
			ShowMessage (m_bScanlines ? T ("Scanlines: sí") : T ("Scanlines: no"));
		}
		SyncSetting ("scanlines");
		break;

	case ActionUnits:
		OpenUnitsScreen ();
		break;

	case ActionEjectCartridge:
		if (m_pSpectrum->IsCartridgeInserted ())
		{
			m_pSpectrum->EjectCartridge ();
			m_CartridgeName[0] = '\0';
			ShowMessage (T ("Cartucho retirado"));
		}
		else
		{
			ShowMessage (T ("No hay cartucho"));
		}
		break;

	case ActionIssue:
		m_pSpectrum->SetIssue2 (!m_pSpectrum->IsIssue2 ());
		ShowMessage (m_pSpectrum->IsIssue2 () ? T ("Teclado Issue 2") : T ("Teclado Issue 3"));
		SyncSetting ("issue");
		break;

	case ActionAYStereo:
	{
		CAY8912 *pAY = m_pSpectrum->GetAY ();
		pAY->SetStereoMode ((TAYStereoMode) ((pAY->GetStereoMode () + 1) % AYStereoModeCount));
		Message.Format (T ("Sonido AY: %s%s"), AYStereoNames[pAY->GetStereoMode ()],
				m_bMonoAudio ? T (" (salida mono)") : "");
		ShowMessage (Message);
		SyncSetting ("aystereo");
		break;
	}
	}
}

// Teclado real por GPIO:
//  - normal: la matriz va directamente al Spectrum
//  - modo extendido (pulsador en GPIO21 o CAPS SHIFT + SYMBOL SHIFT + ENTER):
//    1-0 hacen de F1-F10 y SYMBOL SHIFT de Alt, como en ZXBaremulator
//  - con un menú abierto, las teclas sirven para navegar
void CKernel::HandleGPIOKeyboard (void)
{
	// Teclados "de matriz": el real por GPIO y los Recreated ZX. Se combinan y
	// comparten el modo extendido (CAPS SHIFT + SYMBOL SHIFT + ENTER).
	u8 Combined[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	boolean bAny = FALSE;

	if (m_bGPIOKeyboard)
	{
		m_GPIOKeyboard.Scan ();
		const u8 *pGPIO = m_GPIOKeyboard.GetMatrix ();
		for (unsigned i = 0; i < 8; i++)
		{
			Combined[i] |= pGPIO[i];
		}
		bAny = TRUE;
	}

	for (unsigned k = 0; k < MAX_KEYBOARDS; k++)
	{
		// pulsaciones retenidas por el detector que ya han caducado
		CRecreatedKeyboard::TOutput Out;
		EnterCritical ();
		m_Recreated[k].CheckTimeout (CTimer::GetClockTicks (), &Out);
		for (unsigned i = 0; i < Out.nCount; i++)
		{
			m_Keyboard.UpdateFromReport (Out.Modifiers[i], Out.Keys[i]);
		}
		LeaveCritical ();

		if (m_Recreated[k].CheckJustDetected ())
		{
			ShowMessage (T ("Teclado Recreated ZX detectado"));
			m_Logger.Write (FromKernel, LogNotice, "Recreated ZX keyboard on ukbd%u", k + 1);
		}

		if (m_pKeyboards[k] != 0 && m_Recreated[k].IsActive ())
		{
			const u8 *pRec = m_Recreated[k].GetMatrix ();
			for (unsigned i = 0; i < 8; i++)
			{
				Combined[i] |= pRec[i];
			}
			bAny = TRUE;
		}
	}

	if (!bAny)
	{
		return;
	}

	const u8 *pMatrix = Combined;

	boolean bCS = (pMatrix[0] & 0x01) != 0;
	boolean bSS = (pMatrix[7] & 0x02) != 0;
	boolean bEnter = (pMatrix[6] & 0x01) != 0;
	boolean bCombo = bCS && bSS && bEnter;
	boolean bButton = m_bGPIOKeyboard && m_GPIOKeyboard.IsButtonPressed ();

	if ((bButton && !m_bPrevButton) || (bCombo && !m_bPrevCombo))
	{
		m_bExtendedMode = !m_bExtendedMode;
		m_bWaitRelease = TRUE;		// no pasar al Spectrum las teclas de la combinación
		ShowMessage (m_bExtendedMode ? T ("Teclado: modo extendido (1-0 = F1-F10)")
					     : T ("Teclado: modo normal"));
	}
	m_bPrevButton = bButton;
	m_bPrevCombo = bCombo;

	// Pulsador de NMI, con antirrebote: hay que soltarlo 3 cuadros antes de repetir
	if (m_bGPIOKeyboard && m_GPIOKeyboard.IsNMIPressed ())
	{
		if (m_nNMIReleased >= 3 && !m_Menu.IsOpen () && !m_bPaused)
		{
			m_pSpectrum->TriggerNMI ();
			ShowMessage (m_pSpectrum->IsMultifaceAvailable () ? "Multiface" : "NMI");
		}
		m_nNMIReleased = 0;
	}
	else if (m_nNMIReleased < 3)
	{
		m_nNMIReleased++;
	}

	boolean bAnyKey = FALSE;
	for (unsigned i = 0; i < 8; i++)
	{
		if (pMatrix[i] != 0)
		{
			bAnyKey = TRUE;
		}
	}

	if (m_bWaitRelease)
	{
		static const u8 Empty[8] = {0};
		m_Keyboard.SetGPIOMatrix (Empty);
		memcpy (m_PrevGPIOMatrix, pMatrix, sizeof m_PrevGPIOMatrix);
		if (!bAnyKey)
		{
			m_bWaitRelease = FALSE;
		}
		return;
	}

	if (bAnyKey && !m_Keyboard.IsCaptured ())
	{
		m_nLastKeyTime = CTimer::GetClockTicks64 ();		// ver IsTyping
	}

	boolean bCaptured = m_Keyboard.IsCaptured ();
	if (!m_bExtendedMode && !bCaptured)
	{
		m_Keyboard.SetGPIOMatrix (pMatrix);
		memcpy (m_PrevGPIOMatrix, pMatrix, sizeof m_PrevGPIOMatrix);
		return;
	}

	static const u8 Empty[8] = {0};
	m_Keyboard.SetGPIOMatrix (Empty);

	// Traduce las pulsaciones nuevas a eventos de teclado
	for (unsigned nRow = 0; nRow < 8; nRow++)
	{
		u8 nNew = pMatrix[nRow] & ~m_PrevGPIOMatrix[nRow];

		for (unsigned nBit = 0; nBit < 5; nBit++)
		{
			if ((nNew & (1 << nBit)) == 0)
			{
				continue;
			}

			u8 nHID = CZXKeyboard::MatrixKeyToHID (nRow, nBit);
			if (nHID == 0)
			{
				continue;	// CAPS SHIFT / SYMBOL SHIFT solos
			}

			u8 nModifiers = 0;

			if (m_bExtendedMode && nHID >= 0x1E && nHID <= 0x27)
			{
				nHID = HID_KEY_F1 + (nHID - 0x1E);	// 1-9 -> F1-F9, 0 -> F10
			}
			else if (bCS)
			{
				switch (nHID)
				{
				case 0x22: nHID = HID_KEY_LEFT;		break;	// CS+5
				case 0x23: nHID = HID_KEY_DOWN;		break;	// CS+6
				case 0x24: nHID = HID_KEY_UP;		break;	// CS+7
				case 0x25: nHID = HID_KEY_RIGHT;	break;	// CS+8
				case 0x27: nHID = HID_KEY_BACKSPACE;	break;	// CS+0
				case HID_KEY_SPACE: nHID = HID_KEY_ESC;	break;	// BREAK
				default:				break;
				}
			}

			if (m_bExtendedMode && bSS)
			{
				nModifiers = HID_MOD_LALT;		// SYMBOL SHIFT = Alt
			}

			m_Keyboard.InjectKeyEvent (nHID, nModifiers);
		}
	}

	memcpy (m_PrevGPIOMatrix, pMatrix, sizeof m_PrevGPIOMatrix);
}

// Alt+D: muestra lo que llega del teclado USB y las teclas que ve el Spectrum
void CKernel::DrawKeyDebug (TPixel *pScreen)
{
	static const char *Rows[8][5] =
	{
		{"CS", "Z", "X", "C", "V"},   {"A", "S", "D", "F", "G"},
		{"Q", "W", "E", "R", "T"},    {"1", "2", "3", "4", "5"},
		{"0", "9", "8", "7", "6"},    {"P", "O", "I", "U", "Y"},
		{"EN", "L", "K", "J", "H"},   {"SP", "SS", "M", "N", "B"}
	};

	const TVisibleRect &rVisible = m_Video.GetVisibleRect ();
	int x0 = rVisible.x + 4, y0 = rVisible.y + 4;

	m_Video.FillRect (pScreen, x0 - 2, y0 - 2, 34 * 8 + 4, 12 * 8 + 4, COLOR_UI_PANEL);

	CString Line;
	Line.Format ("USB mod %02X teclas %02X %02X %02X %02X %02X %02X",
		     m_LastReport[0], m_LastReport[1], m_LastReport[2], m_LastReport[3],
		     m_LastReport[4], m_LastReport[5], m_LastReport[6]);
	m_Video.DrawText (pScreen, x0, y0, Line, COLOR_UI_TEXT, COLOR_UI_PANEL);

	unsigned nRender = 0;
#ifdef ARM_ALLOW_MULTI_CORE
	if (m_pVideoCore != 0)
	{
		nRender = m_pVideoCore->GetRenderTime ();
	}
#endif
	Line.Format (T ("%u.%02u c/s  Alt+D cierra"), m_nFPSx100 / 100, m_nFPSx100 % 100);
	m_Video.DrawText (pScreen, x0, y0 + 8, Line, COLOR_UI_DIM, COLOR_UI_PANEL);

	// Tiempo por cuadro (de 20 ms): emulación y demás en el núcleo 0, y la
	// ampliación de la imagen en el núcleo 1 si va aparte
	if (nRender != 0)
	{
		Line.Format (T ("Núcleo 0: %u.%u ms  Imagen: %u.%u ms"),
			     m_nCore0Time / 1000, (m_nCore0Time / 100) % 10, nRender / 1000, (nRender / 100) % 10);
	}
	else
	{
		Line.Format (T ("Núcleo 0: %u.%u ms (con la imagen)"), m_nCore0Time / 1000, (m_nCore0Time / 100) % 10);
	}
	m_Video.DrawText (pScreen, x0, y0 + 10 * 8, Line, COLOR_UI_DIM, COLOR_UI_PANEL, 34);

	// Teclado del +3 por GPIO: estado de sus filas de más (para comprobar el cableado)
	if (m_bGPIOKeyboard && m_GPIOKeyboard.GetType () == GPIOKeyboardPlus3)
	{
		const u16 *pRaw = m_GPIOKeyboard.GetRawMatrix ();
		Line.Format ("+3 R8-12: %03X %03X %03X %03X %03X",
			     pRaw[8] >> 5, pRaw[9] >> 5, pRaw[10] >> 5, pRaw[11] >> 5, pRaw[12] >> 5);
		m_Video.DrawText (pScreen, x0, y0 + 11 * 8, Line, COLOR_UI_DIM, COLOR_UI_PANEL);
	}

	// Matriz del Spectrum (todas las fuentes juntas): pulsadas en vídeo inverso
	for (unsigned nRow = 0; nRow < 8; nRow++)
	{
		u8 nBits = m_Keyboard.ReadPort ((u8) ~(1 << nRow));
		for (unsigned nBit = 0; nBit < 5; nBit++)
		{
			boolean bPressed = !(nBits & (1 << nBit));
			int x = x0 + ((nRow & 1) ? 17 : 0) * 8 + nBit * 3 * 8;
			int y = y0 + (3 + nRow / 2 * 2) * 8;
			m_Video.DrawText (pScreen, x, y, Rows[nRow][nBit],
					  bPressed ? COLOR_UI_HILITE_TEXT : COLOR_UI_TEXT,
					  bPressed ? COLOR_UI_HILITE : COLOR_UI_PANEL);
		}
	}
}

// Los menús necesitan al menos el borde "completo": con bordes más pequeños se
// amplía la imagen mientras están abiertos
void CKernel::OpenMenu (void)
{
	if (m_BorderSize == BorderNone || m_BorderSize == BorderStandard)
	{
		SetVideoMode (BorderFull, m_bScanlines);
	}
}

void CKernel::MenuClosed (void)
{
	if (m_Video.GetBorderSize () != m_BorderSize)
	{
		SetVideoMode (m_BorderSize, m_bScanlines);
	}
}

void CKernel::DrawIndicators (TPixel *pScreen)
{
	int nRunning = m_pSpectrum->IsIF1Connected () ? m_pSpectrum->GetInterface1 ()->GetRunningDrive () : -1;
	if (nRunning >= 0)
	{
		// "M1".."M8" arriba a la derecha mientras gira un microdrive
		const TVisibleRect &rVisible = m_Video.GetVisibleRect ();
		char Text[3] = {'M', (char) ('1' + nRunning), '\0'};
		m_Video.DrawText (pScreen, rVisible.x + rVisible.nWidth - 24, rVisible.y + 6, Text,
				  COLOR_UI_HILITE_TEXT, 4 /* verde */);
	}

	if (m_HardDisk.IsOpen () && CTimer::GetClockTicks64 () - m_nHDActivity < 200000)
	{
		// "H" arriba a la derecha mientras se usa el disco duro
		const TVisibleRect &rVisible = m_Video.GetVisibleRect ();
		m_Video.DrawText (pScreen, rVisible.x + rVisible.nWidth - 56, rVisible.y + 6, "H",
				  COLOR_UI_HILITE_TEXT, 1 /* azul */);
	}

	if (m_pSpectrum->HasDiskDrive () && m_pSpectrum->GetDisk ()->IsMotorOn ())
	{
		// "D" arriba a la derecha mientras gira el disco del +3
		const TVisibleRect &rVisible = m_Video.GetVisibleRect ();
		m_Video.DrawText (pScreen, rVisible.x + rVisible.nWidth - 40, rVisible.y + 6, "D",
				  COLOR_UI_HILITE_TEXT, 2 /* rojo */);
	}

	if (m_pSpectrum->IsRecording ())
	{
		// punto rojo arriba a la izquierda mientras se graba
		const TVisibleRect &rVisible = m_Video.GetVisibleRect ();
		m_Video.FillRect (pScreen, rVisible.x + 6, rVisible.y + 6, 10, 10, 15);
		m_Video.FillRect (pScreen, rVisible.x + 7, rVisible.y + 7, 8, 8, 10);
	}

	if (!m_bExtendedMode)
	{
		return;
	}

	// Icono de teclado azul abajo a la derecha (modo extendido)
	const TVisibleRect &rVisible = m_Video.GetVisibleRect ();
	int x = rVisible.x + rVisible.nWidth - 28, y = rVisible.y + rVisible.nHeight - 16;
	m_Video.FillRect (pScreen, x, y, 20, 10, 9);		// azul brillante
	for (int nRow = 0; nRow < 3; nRow++)
	{
		for (int nKey = 0; nKey < 6; nKey++)
		{
			m_Video.FillRect (pScreen, x + 2 + nKey * 3 + (nRow & 1), y + 2 + nRow * 2, 2, 1, 15);
		}
	}
}

void CKernel::OutputAudio (void)
{
	CAudioMixer *pAudio = m_pSpectrum->GetAudio ();

	// Si la cola de audio va llena (la tarjeta consume algo más despacio que el
	// emulador), se descarta este cuadro de sonido para no acumular retraso
	boolean bQueueFull =    m_pSound != 0
			     && m_pSound->GetQueueFramesAvail () > SOUND_MAX_QUEUED_FRAMES;

	if (m_pSound != 0 && !IsAccelerating () && !bQueueFull)
	{
		if (m_bMonoAudio)
		{
			static s16 Mono[AUDIO_MAX_FRAME_SAMPLES * 2];
			const s16 *pIn = pAudio->GetBuffer ();
			unsigned nFrames = pAudio->GetFrameCount ();
			for (unsigned i = 0; i < nFrames; i++)
			{
				s16 nSample = (s16) (((int) pIn[2 * i] + pIn[2 * i + 1]) / 2);
				Mono[2 * i] = Mono[2 * i + 1] = nSample;
			}
			m_pSound->Write (Mono, nFrames * 2 * sizeof (s16));
		}
		else
		{
			m_pSound->Write (pAudio->GetBuffer (), pAudio->GetFrameCount () * 2 * sizeof (s16));
		}
	}

	pAudio->ClearBuffer ();
}

// El ritmo lo marca siempre el temporizador de 1 MHz de la Raspberry, con la
// duración exacta del cuadro del modelo (19968 us en el 48K, 19991,5 us en el
// 128K). Antes lo marcaba la cola de audio, pero si la tarjeta o el monitor no
// consumen el sonido al ritmo esperado (por ejemplo, un monitor DVI sin audio),
// el emulador podía ir demasiado deprisa o demasiado despacio.
// Juegos durante la carga (el Master Mind de Astro Marine Corps, de Dinamic):
// en carga instantánea o rápida, o a velocidad x2 o más, mientras la cinta
// está en marcha se emula más deprisa que el Spectrum real, y una pulsación de
// unas décimas de segundo le duraba al juego varios segundos: escribía cada
// número dos o más veces. Mientras se escribe (hasta 3 s después de la última
// tecla) y la cinta está en marcha, se va a la velocidad del Spectrum real. La
// carga instantánea sigue cargando cada bloque al momento, como JSpeccy, y sin
// cinta en marcha la velocidad xN no cambia.
#define TYPING_SLOWDOWN_US	3000000

boolean CKernel::IsTyping (void) const
{
	return    m_nLastKeyTime != 0
	       && CTimer::GetClockTicks64 () - m_nLastKeyTime < TYPING_SLOWDOWN_US
	       && m_Tape.IsPlaying ();
}

boolean CKernel::IsAccelerating (void) const
{
	return m_pSpectrum->IsTapeAccelerating () && !IsTyping ();
}

unsigned CKernel::GetEffectiveSpeed (void) const
{
	return IsTyping () ? 1 : m_nSpeed;
}

void CKernel::WaitFrame (void)
{
	const TMachineTiming *T = m_pSpectrum->GetTiming ();
	u64 nFrameNs = (u64) T->nTstatesFrame * 1000000000ULL / T->nClockFreq;

	u64 nNowNs = CTimer::GetClockTicks64 () * 1000;
	if (m_nNextFrameTime == 0 || nNowNs > m_nNextFrameTime + 100000000ULL)
	{
		m_nNextFrameTime = nNowNs;	// arranque o retraso grande: se resincroniza
	}

	// tiempo que ha necesitado el núcleo 0 para este cuadro (media)
	u64 nBusyUs = CTimer::GetClockTicks64 ();
	if (m_nFrameStart != 0 && nBusyUs > m_nFrameStart)
	{
		unsigned nBusy = (unsigned) (nBusyUs - m_nFrameStart);
		m_nCore0Time = m_nCore0Time == 0 ? nBusy : (m_nCore0Time * 15 + nBusy) / 16;
	}

	m_nNextFrameTime += nFrameNs;
	while (CTimer::GetClockTicks64 () * 1000 < m_nNextFrameTime)
	{
		// mientras se espera, avanzan las tareas de red (si no hay, vuelve
		// enseguida) y se muestra el cuadro si el núcleo 1 ya lo ha ampliado
		m_Scheduler.Yield ();
		PollVideo ();
	}
	m_nFrameStart = CTimer::GetClockTicks64 ();

	// Medida de la velocidad real (cuadros emulados por segundo)
	u64 nNowUs = CTimer::GetClockTicks64 ();
	if (m_nFPSStart == 0)
	{
		m_nFPSStart = nNowUs;
		m_nFPSFrames = 0;
	}
	m_nFPSFrames += GetEffectiveSpeed ();
	if (nNowUs - m_nFPSStart >= 2000000)
	{
		m_nFPSx100 = (unsigned) ((u64) m_nFPSFrames * 100000000ULL / (nNowUs - m_nFPSStart));
		m_nFPSStart = nNowUs;
		m_nFPSFrames = 0;
	}
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "ZXMetalCircle " ZXCIRCLE_VERSION " - compiled " __DATE__ " " __TIME__);

	// ear=usb con sounddev=sndusb: una sola tarjeta para entrada y salida
	m_bEarUSB = strcasecmp (m_Options.GetAppOptionString ("ear", "0"), "usb") == 0;

	const char *pSoundDevice = m_Options.GetSoundDevice ();
#ifdef ZX_QEMU
	pSoundDevice = "none";		// QEMU no emula el sonido HDMI/PWM
#endif
	if (strcmp (pSoundDevice, "none") == 0)
	{
		m_pSoundName = N_("desactivado");
	}
	else if (!InitSound (pSoundDevice))
	{
		// si falla el HDMI se prueba el jack (y viceversa)
		if (!InitSound (strcmp (pSoundDevice, "sndhdmi") == 0 ? "sndpwm" : "sndhdmi"))
		{
			m_pSoundName = N_("ninguno");
			m_Logger.Write (FromKernel, LogWarning, "No sound device available");
		}
	}

	StartEarInput ();

	if (m_OptionWarning[0] != '\0')
	{
		ShowMessage (m_OptionWarning);		// opción mal escrita en cmdline.txt
		m_nWarningFrames = 500;			// unos 10 s
		m_nMessageFrames = m_nWarningFrames;
	}
	else if (m_MissingROMs.GetLength () > 0)
	{
		CString Message;
		Message.Format (T ("Faltan ROMs: %s (roms/LEEME.txt)"), (const char *) m_MissingROMs);
		ShowMessage (Message);
	}
	else
	{
		{
		CString Welcome;
		Welcome.Format ("ZXMetalCircle %s  %s", ZXCIRCLE_VERSION, T ("F1 ayuda"));
		ShowMessage (Welcome);
	}
	}

	StartNetwork ();

	// Servidores TNFS en el selector
	m_TNFS.LoadServers ();
	m_TNFS.SetBusyHandler (BusyHandler, this);
	m_Network.SetBusyHandler (NetBusyHandler, this);
	m_Menu.SetRemoteSource (&m_TNFS);

	while (TRUE)
	{
		UpdateUSB ();
		HandleGPIOKeyboard ();

		TKeyEvent Event;
		while (m_Keyboard.GetKeyEvent (&Event))
		{
			HandleKeyEvent (Event);
		}
		UpdateKeyRepeat ();

		// Menú abierto o pausa: la emulación se detiene
		if (m_Menu.IsOpen () || m_bPaused)
		{
			TPixel *Overlay = m_pOverlay;
			const unsigned nOverlayBytes = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof (TPixel);

			if (m_Menu.IsOpen ())
			{
				UpdateStatusLines ();
				memcpy (Overlay, m_pFrozen, nOverlayBytes);
				m_Menu.Draw (Overlay);
				DrawIndicators (Overlay);
			}
			else
			{
				memcpy (Overlay, m_pSpectrum->GetScreenBuffer (), nOverlayBytes);
				DrawMessage (Overlay);
			}

			ShowFrame (Overlay);
			YieldDelay (20);

			// las medidas empiezan de nuevo al volver: si no, el tiempo con el
			// menú abierto (sin emular) contaba como cuadros perdidos (c/s muy
			// bajos tras un rato en F1) o como tiempo de un cuadro del núcleo 0
			// (más de 200 ms en Alt+D, que tardaba en bajar)
			m_nFPSStart = 0;
			m_nFrameStart = 0;

			if (m_pEarSource != 0)
			{
				m_pEarSource->Resync ();	// al volver, la señal sigue en tiempo real
			}
			continue;
		}

		// A velocidad xN se emulan N cuadros por cada cuadro real, sin sonido
		unsigned nSpeed = GetEffectiveSpeed ();
		for (unsigned nFrame = 0; nFrame < nSpeed; nFrame++)
		{
			// Casete real: los cambios de su señal que caen en este cuadro (solo a
			// velocidad normal; si no, se resincroniza al volver)
			m_EarInput.BeginFrame ();
			if (m_pEarSource != 0)
			{
				if (nSpeed == 1 && !IsAccelerating ())
				{
					const TMachineTiming *T = m_pSpectrum->GetTiming ();
					m_pEarSource->FillFrame (&m_EarInput,
								 (u64) T->nTstatesFrame * 1000000000ULL / T->nClockFreq,
								 T->nTstatesFrame, T->nClockFreq);
				}
				else
				{
					m_pEarSource->Resync ();
				}
			}

			m_pSpectrum->RunFrame ();
			m_Keyboard.TickHeld ();			// Alt+V: teclas mantenidas

			// Reloj para esxDOS (RTC.SYS): la hora de la Pi, si ya la tiene (NTP)
			if (m_pSpectrum->IsDivMMCActive ())
			{
				UpdateRTC ();
			}

			// Interface 1: RS232 y red ZX Net hacia la red de la Pi
			if (m_pSpectrum->IsIF1Connected ())
			{
				m_IF1Link.Poll (m_pSpectrum->GetInterface1 ());
			}

			// Cinta con bloque de selección (0x28): se pregunta qué cargar
			if (m_Tape.IsSelectPending () && !m_Menu.IsOpen ())
			{
				OpenTapeSelect ();
			}

			// Spectranet: atender sus sockets y guardar la flash cuando
			// lleva 2 s sin cambios (la configuración, módulos nuevos...)
			if (m_pSpectrum->IsSpectranetEnabled ())
			{
				CSpectranet *pSnet = m_pSpectrum->GetSpectranet ();
				pSnet->Poll ();
				if (pSnet->TakeFlashModified ())
				{
					m_nSnetSaveFrames = 100;
				}
				else if (m_nSnetSaveFrames > 0 && --m_nSnetSaveFrames == 0)
				{
					SaveSpectranetFlash ();
				}
			}
			m_nFrames++;

			if (nSpeed == 1)
			{
				OutputAudio ();
			}
			else
			{
				m_pSpectrum->GetAudio ()->ClearBuffer ();
			}
		}

		boolean bAccelerating = IsAccelerating ();

		// Los cartuchos modificados se graban cuando el motor lleva al menos
		// 2 s parado, y como mucho una vez cada 3 s: grabar cuesta (convertir
		// y escribir ~200 KB) y el CP/M arranca y para el motor muy a menudo
		if (m_pSpectrum->IsIF1Connected ())
		{
			u64 nNow = CTimer::GetClockTicks64 ();
			if (m_pSpectrum->GetInterface1 ()->GetRunningDrive () >= 0)
			{
				m_nMotorStop = nNow;
			}
			else if (   nNow - m_nMotorStop >= 2000000
				 && nNow - m_nLastMDRSave >= 3000000)
			{
				m_nLastMDRSave = nNow;
				SaveMicrodrives (FALSE);
			}
		}

		// Al menos una vez por cuadro se cede el control a las tareas de red,
		// aunque la emulación vaya justa y WaitFrame no llegue a esperar
		m_Scheduler.Yield ();

		// Red: servicios al conectar, peticiones de la web y estado para la web
		if (m_Network.Process ())
		{
			CString Message;
			Message.Format ("%s", m_Network.GetStateText ());
			ShowMessage (Message);
		}
		{
			CString LoadPath;
			if (m_Network.TakeLoadRequest (&LoadPath))
			{
				LoadAnyFile (LoadPath);
			}
		}
		if (m_nFrames % 50 == 0)
		{
			CString Status;
			Status.Format (T ("%s | Cinta: %s | %u.%02u c/s"),
				       m_pSpectrum->GetTiming ()->pName,
				       m_Tape.IsInserted () ? m_Tape.GetName () : T ("(ninguna)"),
				       m_nFPSx100 / 100, m_nFPSx100 % 100);
			m_Network.SetEmulatorStatus (Status);
		}

		// Disco duro: confirma las escrituras tras 1 s sin actividad
		if (m_HardDisk.IsOpen ())
		{
			u64 nNow = CTimer::GetClockTicks64 ();
			if (m_pSpectrum->GetSDCard ()->IsActive ())
			{
				m_nHDActivity = nNow;
			}
			else if (m_HardDisk.IsDirty () && nNow - m_nHDActivity >= 1000000)
			{
				m_HardDisk.Flush ();
			}
		}

		// Discos del +3 modificados: con el motor parado 2 s, como mucho cada 3 s
		if (m_pSpectrum->HasDiskDrive ())
		{
			u64 nNow = CTimer::GetClockTicks64 ();
			if (m_pSpectrum->GetDisk ()->IsMotorOn ())
			{
				m_nDiskMotorStop = nNow;
			}
			else if (   nNow - m_nDiskMotorStop >= 2000000
				 && nNow - m_nLastDSKSave >= 3000000)
			{
				m_nLastDSKSave = nNow;
				SaveDisks (FALSE);
			}
		}

		// CP/M de LEC en marcha: la tecla de borrar hace de retroceso
		m_Keyboard.SetBackspaceAsLeft (m_pSpectrum->IsLECInUse ());

		// El aviso de cmdline.txt se mantiene unos 10 s aunque salgan otros
		// mensajes al arrancar (teclado o ratón conectado, red...): cuando
		// acaba otro mensaje, vuelve a mostrarse durante lo que le quede
		if (m_nWarningFrames > 0)
		{
			m_nWarningFrames--;
			if (m_nMessageFrames <= 1 && m_nWarningFrames > 0)
			{
				ShowMessage (m_OptionWarning);
				m_nMessageFrames = m_nWarningFrames;
			}
		}

		if (m_nMessageFrames > 0)
		{
			m_nMessageFrames--;
		}

		// Si la emulación va con más de un cuadro de retraso (por ejemplo, porque
		// la Pi no da abasto con las scanlines), no se muestra este cuadro: así la
		// velocidad del Spectrum, y la del teclado, se mantienen
		u64 nNowNs = CTimer::GetClockTicks64 () * 1000;
		const TMachineTiming *T = m_pSpectrum->GetTiming ();
		u64 nFrameNs = (u64) T->nTstatesFrame * 1000000000ULL / T->nClockFreq;
		boolean bLate =    !bAccelerating && m_nNextFrameTime != 0
				&& nNowNs > m_nNextFrameTime + nFrameNs;

		if (bLate && m_nSkippedFrames < 4)
		{
			m_nSkippedFrames++;
		}
		else if (!bAccelerating || (m_nFrames % ACCEL_PRESENT_EVERY) == 0)
		{
			m_nSkippedFrames = 0;
			TPixel *pScreen = m_pSpectrum->GetScreenBuffer ();
			DrawMessage (pScreen);	// la emulación redibuja todo en el cuadro siguiente
			if (m_bKeyDebug)
			{
				DrawKeyDebug (pScreen);
			}
			ShowFrame (pScreen);
		}

		if (!bAccelerating)
		{
			WaitFrame ();
		}
		else
		{
			m_nNextFrameTime = 0;
		}
	}

	return ShutdownHalt;
}
