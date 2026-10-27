//
// esxfolder.h - esxDOS: la tarjeta es una imagen y una carpeta para importar
//
// esxDOS necesita una tarjeta FAT y la lee por sectores, así que la tarjeta de
// la divMMC es una imagen (hdd/esxdos.img). Para meter ficheros sin herramientas,
// lo que se deja en la carpeta de importación (SD:/esxdos_i) se copia a la
// imagen al conectar la divMMC y en cada reset, se comprueba la copia y se
// borra de la carpeta. Lo que no se puede importar se queda en la carpeta.
//
// (Hasta la versión 0.94 la imagen y la carpeta SD:/esxdos se sincronizaban en
// los dos sentidos; era frágil y lento, y ocupaba el doble.)
//
// La imagen se monta con FatFs como el volumen "NVME:" a través de un
// dispositivo de Circle con el nombre "nvme1" (en la Pi 1-4 no hay NVMe).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_esxfolder_h
#define _zx_esxfolder_h

#include "harddisk.h"
#include <circle/device.h>
#include <fatfs/ff.h>

#define ESX_IMAGE_VOLUME	"NVME:"
#define ESX_IMAGE_DEVICE	"nvme1"
#define ESX_IMPORT_DIR		"esxdos_i"	// carpeta para importar (8 letras)

// La imagen abierta (CHardDiskImage) vista como un disco de Circle
class CImageDevice : public CDevice
{
public:
	CImageDevice (CHardDiskImage *pImage);
	~CImageDevice (void);

	int Read (void *pBuffer, size_t nCount);
	int Write (const void *pBuffer, size_t nCount);
	u64 Seek (u64 ullOffset);
	u64 GetSize (void) const;

private:
	CHardDiskImage *m_pImage;
	u64 m_ullOffset;
};

class CEsxFolder
{
public:
	CEsxFolder (void);

	// pFolder: "SD:/esxdos_i"; pImage: "hdd/esxdos.img" (relativa a SD:/)
	void Setup (const char *pFolder, const char *pImage);
	boolean IsEnabled (void) const		{ return m_Folder[0] != '\0'; }
	const char *GetFolder (void) const	{ return m_Folder; }
	const char *GetImagePath (void) const	{ return m_Image; }

	// ¿hay algo que importar? (rápido: solo mira si la carpeta está vacía)
	boolean HasPending (void);

	// Importa la carpeta a la imagen (la crea o la agranda si hace falta). La
	// imagen no debe estar montada en la tarjeta del Spectrum. Progreso:
	// nDone de nTotal ficheros y nMBDone de nMBTotal megas (nTotal = 0:
	// preparando una imagen mayor).
	typedef void TProgressHandler (unsigned nDone, unsigned nTotal, unsigned nMBDone, unsigned nMBTotal,
				       void *pParam);
	boolean Import (TProgressHandler *pProgress, void *pParam);

	unsigned GetImported (void) const	{ return m_nImported; }	// ficheros
	unsigned GetReplaced (void) const	{ return m_nReplaced; }	// de ellos, ya estaban
	unsigned GetFailed (void) const		{ return m_nFailed; }	// se quedan en la carpeta
	const char *GetError (void) const	{ return m_pError; }

	// Monta la imagen como "NVME:" (para la web y el selector)
	static boolean MountImage (CHardDiskImage *pImage, FATFS *pFS);
	static void UnmountImage (FATFS *pFS);

	// Nombre que se usa dentro de la imagen (sin bytes que esxDOS pinte raros)
	static void SafeName (const char *pIn, char *pOut, unsigned nSize);

private:
	boolean CreateImage (u32 nMB);
	static u64 FolderSize (const char *pPath, unsigned nDepth, boolean bRound = TRUE);
	static unsigned CountFiles (const char *pPath, unsigned nDepth);
	boolean CopyVerified (const char *pSrc, const char *pDstDir, const char *pDst,
			      const FILINFO &rInfo);
	void ImportTree (const char *pSrc, const char *pDst, unsigned nDepth);
	void SkipTree (const char *pPath, unsigned nDepth);
	boolean ExportTree (const char *pSrc, const char *pDst, unsigned nDepth);
	void Progress (void);

	char m_Folder[64];
	char m_Image[64];
	const char *m_pError;
	u8 *m_pBuffer;
	u8 *m_pBuffer2;
	unsigned m_nImported;
	unsigned m_nReplaced;
	unsigned m_nFailed;
	unsigned m_nDone;
	unsigned m_nTotal;
	u64 m_nBytesDone;
	u64 m_nBytesTotal;
	TProgressHandler *m_pProgress;
	void *m_pProgressParam;
};

#endif
