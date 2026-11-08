//
// kernelupdate.cpp
//
// La actualización desde las releases de GitHub (página 10 de la
// configuración): buscar, descargar, instalar con CUpdater y reiniciar.
//
// - Buscar: la API de GitHub; si falla (límite de 60 consultas por hora y
//   por IP), la redirección de .../releases/latest a .../releases/tag/vX.YY.
// - El paquete se guarda en update/ y se puede volver a instalar (o uno
//   anterior) desde "Paquete guardado".
// - Antes de instalar se graba lo pendiente (discos, Microdrives...) y, si
//   hay cambios sin guardar en la configuración, se pregunta.
// - Al terminar, se reinicia sin dar opción: lo que hay en la SD ya no es
//   lo que está en marcha. Si no ha cambiado nada, no hace falta.
// - Si falla a medias, no se reinicia: se ofrece reintentar, y queda
//   update/EN_CURSO para avisar al arrancar.
//
#include "kernel.h"
#include "updater.h"
#include "updatefs.h"
#include "version.h"
#include "i18n.h"
#include "download/circleglue.h"
#include "download/httpsclient.h"
#include <circle/timer.h>
#include <circle/sched/task.h>
#include <circle/util.h>
#include <circle/string.h>
#include <fatfs/ff.h>
#include <string.h>

#define UPDATE_API_URL		"https://api.github.com/repos/shaeon/ZXMetalCircle/releases/latest"
#define UPDATE_WEB_URL		"https://github.com/shaeon/ZXMetalCircle/releases/latest"
#define UPDATE_DOWNLOAD_MAX	(32 * 1024 * 1024)	// el paquete de la 1.06: 11,8 MB
#define UPDATE_PAGE_MAX		(1024 * 1024)		// respuesta de la API o página web

// El transporte de red con aviso: mientras se conecta y negocia con GitHub
// (lo lento en una Pi), el mensaje dice qué pasa y cuántos segundos lleva,
// para que no parezca colgado. En la descarga calla en cuanto llegan datos
// (ahí ya se ven los KB).
class CBusyTransport : public ITransport
{
public:
	typedef void TShow (void *pParam, const char *pText);

	CBusyTransport (TShow *pShow, void *pParam, const char *pConnecting, const char *pWaiting,
			const unsigned *pnQuietWhenNonZero = 0)
	:	m_pShow (pShow), m_pParam (pParam), m_pConnecting (pConnecting), m_pWaiting (pWaiting),
		m_pnQuiet (pnQuietWhenNonZero), m_nStart (CTimer::Get ()->GetTicks ()), m_nLast (0)
	{
	}

	bool Connect (const char *pHost, unsigned nPort)
	{
		if (!Quiet ())
		{
			CString Text;
			Text.Format (m_pConnecting, pHost);
			(*m_pShow) (m_pParam, Text);
		}
		return m_Inner.Connect (pHost, nPort);
	}
	int Send (const unsigned char *pData, size_t nLength)	{ Tick (); return m_Inner.Send (pData, nLength); }
	int Receive (unsigned char *pBuffer, size_t nSize)	{ Tick (); return m_Inner.Receive (pBuffer, nSize); }
	void Close (void)					{ m_Inner.Close (); }

private:
	boolean Quiet (void) const	{ return m_pnQuiet != 0 && *m_pnQuiet != 0; }

	void Tick (void)
	{
		unsigned nNow = CTimer::Get ()->GetTicks ();
		if (Quiet () || nNow - m_nLast < HZ / 2)
		{
			return;
		}
		m_nLast = nNow;
		CString Text;
		Text.Format (m_pWaiting, (nNow - m_nStart) / HZ);
		(*m_pShow) (m_pParam, Text);
	}

	CCircleTransport m_Inner;
	TShow *m_pShow;
	void *m_pParam;
	const char *m_pConnecting;
	const char *m_pWaiting;
	const unsigned *m_pnQuiet;
	unsigned m_nStart;
	unsigned m_nLast;
};


static const char *Setting (CSettings &Settings, const char *pKey)
{
	int n = Settings.Find (pKey);
	return n >= 0 ? Settings.GetValue (n) : "";
}

void CKernel::UpdateBusy (const char *pText)
{
	BusyHandler (pText, this);
}

void CKernel::UpdateShowBusy (void *pParam, const char *pText)
{
	BusyHandler (pText, pParam);
}

// Los resultados se dejan más tiempo en pantalla (unos 6 s)
void CKernel::UpdateMessage (const char *pText)
{
	ShowMessage (pText);
	m_nMessageFrames = 300;
}

// Los paquetes de update/ (sus versiones), de la más nueva a la más antigua
unsigned CKernel::UpdateScanPackages (void)
{
	m_nUpdatePackages = 0;
	DIR Dir;
	FILINFO Info;
	if (f_opendir (&Dir, "SD:/" UPDATE_DIR) != FR_OK)
	{
		return 0;
	}
	while (f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
	{
		unsigned nLen = strlen (Info.fname);
		if (   (Info.fattrib & AM_DIR) || nLen <= 21 || nLen - 21 >= sizeof m_UpdatePackages[0]
		    || strncasecmp (Info.fname, "ZXMetalCircle-", 14) != 0
		    || strcasecmp (Info.fname + nLen - 7, "-sd.zip") != 0)
		{
			continue;
		}
		char Version[sizeof m_UpdatePackages[0]];
		memcpy (Version, Info.fname + 14, nLen - 21);
		Version[nLen - 21] = '\0';
		if (m_nUpdatePackages >= MENU_CHOICE_ROWS)
		{
			break;
		}
		// en orden: de la más nueva a la más antigua
		unsigned i = m_nUpdatePackages++;
		while (i > 0 && CUpdater::CompareVersions (m_UpdatePackages[i - 1], Version) < 0)
		{
			strcpy (m_UpdatePackages[i], m_UpdatePackages[i - 1]);
			i--;
		}
		strcpy (m_UpdatePackages[i], Version);
	}
	f_closedir (&Dir);
	return m_nUpdatePackages;
}

// Lo que se ve a la derecha de las filas de acción
void CKernel::UpdateRows (void)
{
	m_Settings.SetActionText ("_updversion", ZXCIRCLE_VERSION);
	CString Text;
	if (m_UpdateVersion[0] != '\0')
	{
		Text.Format (T ("instalar la %s"), m_UpdateVersion);
	}
	m_Settings.SetActionText ("_updcheck", Text);
	m_Settings.SetActionText ("_updlocal", UpdateScanPackages () == 0 ? T ("ninguno") : "");
}

// Al arrancar: si una instalación no terminó, se avisa (TRUE)
boolean CKernel::UpdateInit (void)
{
	UpdateRows ();
	CFatUpdateFS FS;
	unsigned nSize;
	if (!FS.GetSize (UPDATE_MARKER, &nSize) || nSize >= sizeof m_UpdateTarget)
	{
		return FALSE;
	}
	char Version[sizeof m_UpdateTarget] = "";
	FS.Read (UPDATE_MARKER, (u8 *) Version, nSize);
	Version[nSize] = '\0';
	// como el aviso de cmdline.txt: si otro mensaje lo tapa, vuelve a salir
	strncpy (m_OptionWarning, T ("Actualización a medias: Alt+F1, pág. 10"), sizeof m_OptionWarning - 1);
	m_OptionWarning[sizeof m_OptionWarning - 1] = '\0';
	ShowMessage (m_OptionWarning);
	m_nWarningFrames = 500;			// unos 10 s
	m_nMessageFrames = m_nWarningFrames;
	return TRUE;
}

// La última versión publicada en GitHub: primero la redirección de
// github.com/.../releases/latest a .../tag/vX.YY (una sola petición pequeña y
// sin límite de consultas) y, si falla, la API. Sin nada de pantalla: la usan
// la búsqueda a mano y la de al arrancar (en segundo plano).
static boolean QueryLatest (ITransport *pTransport, char *pVersion, size_t nVersionSize,
			    char *pURL, size_t nURLSize)
{
	CHTTPSClient Client (pTransport);
	boolean bFound = FALSE;
	char *pLocation = new char[1024];
	if (pLocation != 0)
	{
		int nStatus = Client.GetRedirect (UPDATE_WEB_URL, pLocation, 1024);
		if (nStatus >= 300 && nStatus < 400)
		{
			bFound = CUpdater::ParseTagURL (pLocation, pVersion, nVersionSize, pURL, nURLSize);
		}
		delete [] pLocation;
	}
	if (!bFound)
	{
		u8 *pPage = new u8[UPDATE_PAGE_MAX];
		size_t nLength = 0;
		if (pPage != 0 && Client.Get (UPDATE_API_URL, pPage, UPDATE_PAGE_MAX, &nLength) == 200)
		{
			bFound = CUpdater::ParseRelease ((const char *) pPage, nLength,
							 pVersion, nVersionSize, pURL, nURLSize);
		}
		delete [] pPage;
	}
	return bFound;
}

// La búsqueda de al arrancar, en una tarea aparte: el Spectrum sigue mientras
// se espera a GitHub (la tarea avanza cuando el bucle principal espera al
// cuadro siguiente). Deja el resultado en el kernel (la tarea se borra sola al
// terminar, así que no se lee nada de ella) y avisa con bDone.
class CUpdateCheckTask : public CTask
{
public:
	CUpdateCheckTask (TUpdateBootResult *pResult)
	:	CTask (0x20000),		// el TLS necesita bastante pila
		m_pResult (pResult)
	{
	}

	void Run (void)
	{
		CCircleTransport Transport;
		m_pResult->bFound = QueryLatest (&Transport, m_pResult->Version, sizeof m_pResult->Version,
						 m_pResult->URL, sizeof m_pResult->URL);
		m_pResult->bDone = TRUE;
	}

private:
	TUpdateBootResult *m_pResult;
};

// Lo que se ha encontrado, en pantalla y en las filas de la página 10
void CKernel::UpdateShowResult (boolean bFound, const char *pVersion, const char *pURL)
{
	CString Message;
	if (!bFound)
	{
		UpdateMessage (T ("No se pudo consultar GitHub"));
	}
	else if (CUpdater::CompareVersions (pVersion, ZXCIRCLE_VERSION) > 0)
	{
		strncpy (m_UpdateVersion, pVersion, sizeof m_UpdateVersion - 1);
		m_UpdateVersion[sizeof m_UpdateVersion - 1] = '\0';
		strncpy (m_UpdateURL, pURL, sizeof m_UpdateURL - 1);
		m_UpdateURL[sizeof m_UpdateURL - 1] = '\0';
		Message.Format (T ("Hay una versión nueva: %s"), pVersion);
		UpdateMessage (Message);
	}
	else
	{
		m_UpdateVersion[0] = '\0';
		Message.Format (T ("Ya tienes la última (%s)"), ZXCIRCLE_VERSION);
		UpdateMessage (Message);
	}
	UpdateRows ();
}

// A mano ("Buscar actualización"): con el menú abierto, esperando, y con los
// avisos de lo que pasa
void CKernel::UpdateCheck (void)
{
	if (!CNetworkManager::IsNetRunning ())
	{
		UpdateMessage (T ("Sin red: no se puede buscar"));
		return;
	}
	UpdateBusy (T ("Buscando actualización..."));
	CBusyTransport Transport (UpdateShowBusy, this, T ("Buscando: conectando con %s..."),
				  T ("Buscando: esperando a GitHub (%u s)..."));
	char Version[sizeof m_UpdateVersion], URL[sizeof m_UpdateURL];
	boolean bFound = QueryLatest (&Transport, Version, sizeof Version, URL, sizeof URL);
	UpdateShowResult (bFound, Version, URL);
}

// "Buscar al arrancar": la primera vez que hay red, en segundo plano (el
// Spectrum sigue arrancando); el resultado se dice cuando llega
void CKernel::UpdateBootCheck (void)
{
	if (   !m_bUpdateBootChecked && CNetworkManager::IsNetRunning ()
	    && strcmp (Setting (m_Settings, "updboot"), "1") == 0)
	{
		m_bUpdateBootChecked = TRUE;
		m_UpdateBootResult.bDone = FALSE;
		m_UpdateBootResult.bFound = FALSE;
		m_bUpdateBootRunning = TRUE;
		ShowMessage (T ("Buscando actualización..."));
		m_nMessageFrames = 1500;			// hasta el resultado (como mucho, 30 s)
		new CUpdateCheckTask (&m_UpdateBootResult);	// se borra sola al terminar
	}
	if (m_bUpdateBootRunning && m_UpdateBootResult.bDone)
	{
		m_bUpdateBootRunning = FALSE;
		UpdateShowResult (m_UpdateBootResult.bFound, m_UpdateBootResult.Version, m_UpdateBootResult.URL);
	}
}

// Confirmación: la instalación termina reiniciando
void CKernel::UpdateConfirm (void)
{
	CString Title;
	Title.Format (T ("Instalar la %s (luego se reinicia)"), m_UpdateTarget);
	m_bUpdateChoice = TRUE;
	m_Menu.OpenChoice (Title);
	m_Menu.SetChoiceItem (0, T ("No"));
	if (m_Settings.IsDirty ())
	{
		m_Menu.SetChoiceItem (1, T ("Guardar la configuración e instalar"));
		m_Menu.SetChoiceItem (2, T ("Instalar sin guardar la configuración"));
	}
	else
	{
		m_Menu.SetChoiceItem (1, T ("Sí, instalar"));
	}
}

// ENTER en una fila de acción de la página 10
void CKernel::UpdateAction (const char *pKey)
{
	if (strcmp (pKey, "_updcheck") == 0)
	{
		if (m_UpdateVersion[0] == '\0')
		{
			UpdateCheck ();		// si hay una nueva, la fila lo dice
		}
		else
		{
			strcpy (m_UpdateTarget, m_UpdateVersion);
			m_UpdateLocal[0] = '\0';	// se descarga
			UpdateConfirm ();
		}
	}
	else if (strcmp (pKey, "_updlocal") == 0)
	{
		if (UpdateScanPackages () == 0)
		{
			ShowMessage (T ("No hay paquetes en la carpeta update"));
			return;
		}
		m_bUpdateListChoice = TRUE;
		m_Menu.OpenChoice (T ("Elige el paquete que instalar"));
		for (unsigned i = 0; i < m_nUpdatePackages; i++)
		{
			CString Item;
			if (strcmp (m_UpdatePackages[i], ZXCIRCLE_VERSION) == 0)
			{
				Item.Format (T ("%s (la que tienes)"), m_UpdatePackages[i]);
			}
			else
			{
				Item = m_UpdatePackages[i];
			}
			m_Menu.SetChoiceItem (i, Item);
		}
	}
}

// Las respuestas de la página 10 y de sus listas; TRUE si era suya
boolean CKernel::UpdateMenuResult (TMenuResult Result)
{
	if (Result == MenuResultSettingAction)
	{
		UpdateAction (m_Settings.GetDef (m_Menu.GetChangedSetting ()).pKey);
		return TRUE;
	}
	if (Result != MenuResultChoice && Result != MenuResultChoiceCancel)
	{
		return FALSE;
	}
	unsigned nChoice = Result == MenuResultChoice ? m_Menu.GetChoice () : 0;

	if (m_bUpdateChoice)
	{
		m_bUpdateChoice = FALSE;
		if (nChoice == 0)
		{
			m_Menu.OpenSettings ();
			return TRUE;
		}
		if (m_Settings.IsDirty () && nChoice == 1)
		{
			SaveSettings ();
		}
		UpdateRun ();
		return TRUE;
	}
	if (m_bUpdateListChoice)
	{
		m_bUpdateListChoice = FALSE;
		if (Result == MenuResultChoiceCancel || nChoice >= m_nUpdatePackages)
		{
			m_Menu.OpenSettings ();
			return TRUE;
		}
		strcpy (m_UpdateTarget, m_UpdatePackages[nChoice]);
		CUpdater::PackageName (m_UpdateTarget, m_UpdateLocal, sizeof m_UpdateLocal);
		UpdateConfirm ();
		return TRUE;
	}
	if (m_bUpdateErrorChoice)
	{
		m_bUpdateErrorChoice = FALSE;
		if (Result == MenuResultChoice && nChoice == 0)
		{
			UpdateRun ();			// reintentar (con el paquete guardado)
		}
		else
		{
			m_Menu.OpenSettings ();
			ShowMessage (T ("La actualización ha quedado a medias"));
		}
		return TRUE;
	}
	return FALSE;
}

void CKernel::UpdateDownloadProgress (void *pParam, size_t nReceived, size_t nTotal)
{
	CKernel *pThis = static_cast<CKernel *> (pParam);
	if (nReceived - pThis->m_nUpdateShown < 256 * 1024 && nReceived != nTotal)
	{
		return;
	}
	pThis->m_nUpdateShown = nReceived;
	CString Text;
	if (nTotal > 0)
	{
		Text.Format (T ("Descargando la %s: %u de %u KB"), pThis->m_UpdateTarget,
			     (unsigned) (nReceived / 1024), (unsigned) (nTotal / 1024));
	}
	else
	{
		Text.Format (T ("Descargando la %s: %u KB"), pThis->m_UpdateTarget, (unsigned) (nReceived / 1024));
	}
	pThis->UpdateBusy (Text);
}

void CKernel::UpdateInstallProgress (void *pParam, unsigned nDone, unsigned nTotal)
{
	CKernel *pThis = static_cast<CKernel *> (pParam);
	unsigned nPercent = nTotal > 0 ? nDone * 100 / nTotal : 0;
	if (nPercent == pThis->m_nUpdateShown)
	{
		return;
	}
	pThis->m_nUpdateShown = nPercent;
	// la primera mitad es comprobar el paquete; la segunda, copiar
	CString Text;
	if (nDone * 2 < nTotal)
	{
		Text.Format (T ("Comprobando el paquete: %u %%"), nPercent * 2);
	}
	else
	{
		Text.Format (T ("Instalando la %s: %u %%"), pThis->m_UpdateTarget, (nPercent - 50) * 2);
	}
	pThis->UpdateBusy (Text);
}

// Lee un paquete de la SD a memoria (0 si no está)
static u8 *ReadPackage (CUpdateFS *pFS, const char *pPath, unsigned *pSize)
{
	if (!pFS->GetSize (pPath, pSize) || *pSize == 0)
	{
		return 0;
	}
	u8 *pData = new u8[*pSize];
	if (pData != 0 && !pFS->Read (pPath, pData, *pSize))
	{
		delete [] pData;
		pData = 0;
	}
	return pData;
}

// Deja en update/ el paquete instalado y, con "actual y anterior", el de la
// versión que había (para poder volver a ella)
void CKernel::UpdatePrune (void)
{
	if (strcmp (Setting (m_Settings, "updkeep"), "all") == 0)
	{
		return;
	}
	CFatUpdateFS FS;
	UpdateScanPackages ();
	for (unsigned i = 0; i < m_nUpdatePackages; i++)
	{
		if (   strcmp (m_UpdatePackages[i], m_UpdateTarget) != 0
		    && strcmp (m_UpdatePackages[i], ZXCIRCLE_VERSION) != 0)
		{
			char Name[sizeof m_UpdateLocal];
			CUpdater::PackageName (m_UpdatePackages[i], Name, sizeof Name);
			FS.Remove (Name);
		}
	}
}

void CKernel::UpdateRun (void)
{
	CFatUpdateFS FS;
	CString Message;

	// lo pendiente, a la SD (como antes de reiniciar)
	UpdateBusy (T ("Grabando lo pendiente..."));
	FlushToSD ();

	// el paquete: descargado o de update/
	char Package[sizeof m_UpdateLocal];
	CUpdater::PackageName (m_UpdateTarget, Package, sizeof Package);
	u8 *pZip = 0;
	unsigned nZipSize = 0;
	boolean bDownloaded = FALSE;
	if (m_UpdateLocal[0] != '\0')
	{
		UpdateBusy (T ("Leyendo el paquete..."));
		pZip = ReadPackage (&FS, m_UpdateLocal, &nZipSize);
		if (pZip == 0)
		{
			m_Menu.OpenSettings ();
			ShowMessage (T ("No se pudo leer el paquete"));
			return;
		}
	}
	else
	{
		if (!CNetworkManager::IsNetRunning ())
		{
			m_Menu.OpenSettings ();
			ShowMessage (T ("Sin red: no se puede descargar"));
			return;
		}
		pZip = new u8[UPDATE_DOWNLOAD_MAX];
		if (pZip == 0)
		{
			m_Menu.OpenSettings ();
			ShowMessage (T ("No hay memoria para la actualización"));
			return;
		}
		m_nUpdateShown = 0;
		CString Connecting, Waiting;
		Connecting.Format (T ("Descargando la %s: conectando..."), m_UpdateTarget);
		Waiting.Format (T ("Descargando la %s: esperando (%%u s)..."), m_UpdateTarget);
		CBusyTransport Transport (UpdateShowBusy, this, Connecting, Waiting, &m_nUpdateShown);
		CHTTPSClient Client (&Transport);
		Client.SetProgress (UpdateDownloadProgress, this);
		size_t nLength = 0;
		int nStatus = Client.Get (m_UpdateURL, pZip, UPDATE_DOWNLOAD_MAX, &nLength);
		if (nStatus != 200)
		{
			delete [] pZip;
			m_Menu.OpenSettings ();
			if (nStatus < 0)
			{
				Message.Format (T ("Error al descargar: %s"), Client.GetError ());
			}
			else
			{
				Message.Format (T ("Error al descargar: código %d"), nStatus);
			}
			ShowMessage (Message);
			return;
		}
		nZipSize = nLength;
		bDownloaded = TRUE;

		// se guarda: para reintentar, volver a ella o descomprimirlo a mano
		UpdateBusy (T ("Guardando el paquete en update..."));
		FS.MakeDir (UPDATE_DIR);
		if (FS.Write (Package, pZip, nZipSize))
		{
			strcpy (m_UpdateLocal, Package);
		}
	}

	// el de la versión que hay, para esxdos_i
	u8 *pPrev = 0;
	unsigned nPrevSize = 0;
	if (strcmp (m_UpdateTarget, ZXCIRCLE_VERSION) != 0)
	{
		char Prev[sizeof m_UpdateLocal];
		CUpdater::PackageName (ZXCIRCLE_VERSION, Prev, sizeof Prev);
		pPrev = ReadPackage (&FS, Prev, &nPrevSize);
	}

	TUpdateResult Result;
	m_nUpdateShown = (unsigned) -1;
	CUpdater::Install (&FS, pZip, nZipSize, pPrev, nPrevSize,
			   strcmp (Setting (m_Settings, "updconfig"), "new") == 0,
			   m_UpdateTarget, &Result, UpdateInstallProgress, this);
	delete [] pZip;
	delete [] pPrev;

	switch (Result.Status)
	{
	case UpdateOK:
		UpdatePrune ();
		if (Result.nChanged == 0)
		{
			m_Menu.OpenSettings ();
			Message.Format (T ("Nada que cambiar: la SD ya tenía la %s"), m_UpdateTarget);
			UpdateMessage (Message);
			UpdateRows ();
			return;
		}
		// terminada: se reinicia sin dar opción
		Message.Format (T ("Instalada la %s"), m_UpdateTarget);
		m_Menu.OpenChoice (Message);
		m_Menu.SetChoiceInfo (TRUE);
		Message.Format (T ("Ficheros cambiados: %u"), Result.nChanged);
		m_Menu.SetChoiceItem (0, Message);
		for (unsigned i = 0; i < Result.nNotes; i++)
		{
			Message.Format (T ("Configuración: %s"), Result.Notes[i]);
			m_Menu.SetChoiceItem (i + 1, Message);
		}
		for (unsigned n = 5; n > 0; n--)
		{
			Message.Format (T ("Reiniciando en %u s..."), n);
			UpdateBusy (Message);
			CTimer::Get ()->MsDelay (1000);
		}
		RebootPi (FALSE);
		return;

	case UpdateBadPackage:
		if (bDownloaded)
		{
			FS.Remove (Package);		// descargado mal: no se guarda
			m_UpdateLocal[0] = '\0';
		}
		m_Menu.OpenSettings ();
		Message.Format (T ("El paquete está dañado (%s)"), Result.Failed);
		ShowMessage (Message);
		UpdateRows ();
		return;

	case UpdateNoMemory:
		m_Menu.OpenSettings ();
		ShowMessage (T ("No hay memoria para la actualización"));
		return;

	case UpdateWriteError:
		// a medias: queda update/EN_CURSO; no se reinicia
		m_bUpdateErrorChoice = TRUE;
		Message.Format (T ("Error al copiar %s"), Result.Failed);
		m_Menu.OpenChoice (Message);
		m_Menu.SetChoiceItem (0, T ("Reintentar"));
		m_Menu.SetChoiceItem (1, T ("Seguir (queda a medias)"));
		return;
	}
}
