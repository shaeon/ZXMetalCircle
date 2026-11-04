//
// updater.h
//
// Instalación de una actualización desde el paquete de la SD (el zip de una
// release de GitHub). No depende de la Pi: trabaja con el zip en memoria y
// con los ficheros a través de CUpdateFS (FatFs en la Pi, carpetas en las
// pruebas del PC).
//
// Reglas:
// - Antes de tocar nada se comprueba el CRC de todas las entradas.
// - Solo se copian los ficheros que cambian (tamaño o CRC distintos).
// - Cada fichero se escribe con otro nombre (.new) y luego se renombra.
// - Los kernels se copian los últimos.
// - Los ficheros de configuración (cmdline.txt, config.txt, excluir.txt,
//   tnfs.txt y la configuración de esxDOS) no se sobrescriben: el nuevo se
//   deja al lado como .nuevo; o, si se pide, se pone el nuevo y el del
//   usuario queda como .bak.
// - esxdos_i/ se importa a la tarjeta de esxDOS al arrancar y la carpeta
//   queda vacía, así que no se compara con la SD sino con el paquete
//   anterior: solo se copia lo que cambia de una versión a otra.
// - Mientras dura, existe update/EN_CURSO (con la versión), para saber al
//   arrancar que una instalación no terminó.
//
#ifndef _zx_updater_h
#define _zx_updater_h

#include <circle/types.h>

#define UPDATE_DIR		"update"
#define UPDATE_MARKER		"update/EN_CURSO"
#define UPDATE_MAX_ENTRIES	1024
#define UPDATE_MAX_NOTES	8
#define UPDATE_NOTE_LEN		64

// Los ficheros, con rutas relativas a la raíz de la SD ("roms/48.rom")
class CUpdateFS
{
public:
	virtual ~CUpdateFS (void) {}
	virtual boolean GetSize (const char *pPath, unsigned *pSize) = 0;	// FALSE: no existe
	virtual boolean Read (const char *pPath, u8 *pBuffer, unsigned nSize) = 0;
	virtual boolean Write (const char *pPath, const u8 *pData, unsigned nSize) = 0;
	virtual boolean Remove (const char *pPath) = 0;
	virtual boolean Rename (const char *pFrom, const char *pTo) = 0;	// el destino no existe
	virtual boolean MakeDir (const char *pPath) = 0;			// TRUE también si ya existe
};

enum TUpdateStatus
{
	UpdateOK,
	UpdateBadPackage,		// no es un zip o una entrada tiene mal el CRC
	UpdateNoMemory,
	UpdateWriteError		// a medias: queda update/EN_CURSO
};

struct TUpdateResult
{
	TUpdateStatus Status;
	unsigned nChanged;		// ficheros copiados (si hay alguno, hay que reiniciar)
	unsigned nUnchanged;		// iguales, no se tocan
	unsigned nNotes;		// ficheros de configuración: "cmdline.txt.nuevo"... (un
					// .nuevo no cuenta en nChanged: no afecta a nada en marcha)
	char Notes[UPDATE_MAX_NOTES][UPDATE_NOTE_LEN];
	char Failed[96];		// con UpdateWriteError, el fichero que falló
};

class CUpdater
{
public:
	// Progreso: nDone de nTotal pasos (comprobar y copiar)
	typedef void TProgress (void *pParam, unsigned nDone, unsigned nTotal);

	// pPrev: el paquete instalado antes (para esxdos_i), o 0
	static void Install (CUpdateFS *pFS, const u8 *pZip, unsigned nZipSize,
			     const u8 *pPrev, unsigned nPrevSize,
			     boolean bReplaceConfig, const char *pVersion,
			     TUpdateResult *pResult,
			     TProgress *pProgress = 0, void *pParam = 0);

	static boolean IsConfigFile (const char *pPath);
	static boolean IsKernel (const char *pPath);
	static boolean InEsxImport (const char *pPath);

	// < 0, 0 o > 0 según a sea anterior, igual o posterior a b ("1.06")
	static int CompareVersions (const char *a, const char *b);

	// De la respuesta de api.github.com/repos/.../releases/latest: la versión
	// ("v1.07" -> "1.07") y la dirección del zip de la SD (...-sd.zip)
	static boolean ParseRelease (const char *pJSON, unsigned nLength,
				     char *pVersion, unsigned nVersionSize,
				     char *pURL, unsigned nURLSize);

	// Plan B si la API falla: github.com/.../releases/latest redirige a
	// .../releases/tag/v1.07; de esa dirección, la versión y la del zip
	// (.../releases/download/v1.07/ZXMetalCircle-1.07-sd.zip)
	static boolean ParseTagURL (const char *pFinalURL,
				    char *pVersion, unsigned nVersionSize,
				    char *pURL, unsigned nURLSize);

	// El nombre del paquete de una versión en update/
	static void PackageName (const char *pVersion, char *pName, unsigned nSize);
};

#endif
