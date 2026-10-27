//
// esxfolder.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "esxfolder.h"
#include "i18n.h"
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>

static const char From[] = "esxfolder";

#define COPY_BUFFER_SIZE	(32 * 1024)
#define MIN_IMAGE_MB		16
#define MAX_IMAGE_MB		2048		// hasta 2 GB la tarjeta es SDSC
#define MAX_DEPTH		16
#define TEMP_NAME		"~IMPORT.TMP"	// copia a medias (se renombra al acabar)

////////////////////////////////////////////////////////////////////////////////

CImageDevice::CImageDevice (CHardDiskImage *pImage)
:	m_pImage (pImage),
	m_ullOffset (0)
{
}

CImageDevice::~CImageDevice (void)
{
}

int CImageDevice::Read (void *pBuffer, size_t nCount)
{
	u8 *p = (u8 *) pBuffer;
	for (size_t n = 0; n < nCount; n += 512, m_ullOffset += 512)
	{
		if (!m_pImage->ReadSector ((u32) (m_ullOffset / 512), p + n))
		{
			return -1;
		}
	}
	return (int) nCount;
}

int CImageDevice::Write (const void *pBuffer, size_t nCount)
{
	const u8 *p = (const u8 *) pBuffer;
	for (size_t n = 0; n < nCount; n += 512, m_ullOffset += 512)
	{
		if (!m_pImage->WriteSector ((u32) (m_ullOffset / 512), p + n))
		{
			return -1;
		}
	}
	return (int) nCount;
}

u64 CImageDevice::Seek (u64 ullOffset)
{
	m_ullOffset = ullOffset;
	return ullOffset;
}

u64 CImageDevice::GetSize (void) const
{
	return (u64) m_pImage->GetSectors () * 512;
}

////////////////////////////////////////////////////////////////////////////////

CEsxFolder::CEsxFolder (void)
:	m_pError (0),
	m_pBuffer (0),
	m_pBuffer2 (0),
	m_nImported (0),
	m_nReplaced (0),
	m_nFailed (0),
	m_nDone (0),
	m_nTotal (0),
	m_nBytesDone (0),
	m_nBytesTotal (0),
	m_pProgress (0),
	m_pProgressParam (0)
{
	m_Folder[0] = '\0';
	m_Image[0] = '\0';
}

void CEsxFolder::Setup (const char *pFolder, const char *pImage)
{
	strncpy (m_Folder, pFolder, sizeof m_Folder - 1);
	m_Folder[sizeof m_Folder - 1] = '\0';
	strncpy (m_Image, pImage, sizeof m_Image - 1);
	m_Image[sizeof m_Image - 1] = '\0';
}

boolean CEsxFolder::MountImage (CHardDiskImage *pImage, FATFS *pFS)
{
	CDeviceNameService::Get ()->RemoveDevice (ESX_IMAGE_DEVICE, TRUE);
	CImageDevice *pDevice = new CImageDevice (pImage);
	CDeviceNameService::Get ()->AddDevice (ESX_IMAGE_DEVICE, pDevice, TRUE);
	FRESULT Result = f_mount (pFS, ESX_IMAGE_VOLUME, 1);
	if (Result != FR_OK)
	{
		CLogger::Get ()->Write (From, LogWarning, "f_mount de la imagen: %d", Result);
	}
	return Result == FR_OK;
}

void CEsxFolder::UnmountImage (FATFS *pFS)
{
	f_mount (0, ESX_IMAGE_VOLUME, 0);
	CDevice *pDevice = CDeviceNameService::Get ()->GetDevice (ESX_IMAGE_DEVICE, TRUE);
	CDeviceNameService::Get ()->RemoveDevice (ESX_IMAGE_DEVICE, TRUE);
	delete pDevice;
}

// Bytes que ocupará la carpeta (con el redondeo de los clústeres)
u64 CEsxFolder::FolderSize (const char *pPath, unsigned nDepth, boolean bRound)
{
	u64 nTotal = 0;
	DIR Dir;
	FILINFO Info;
	if (nDepth > MAX_DEPTH || f_opendir (&Dir, pPath) != FR_OK)
	{
		return 0;
	}
	while (f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
	{
		if (Info.fattrib & AM_DIR)
		{
			CString Sub;
			Sub.Format ("%s/%s", pPath, Info.fname);
			nTotal += FolderSize (Sub, nDepth + 1, bRound) + (bRound ? 4096 : 0);
		}
		else
		{
			nTotal += Info.fsize + (bRound ? 4096 : 0);
		}
	}
	f_closedir (&Dir);
	return nTotal;
}

unsigned CEsxFolder::CountFiles (const char *pPath, unsigned nDepth)
{
	unsigned nCount = 0;
	DIR Dir;
	FILINFO Info;
	if (nDepth > MAX_DEPTH || f_opendir (&Dir, pPath) != FR_OK)
	{
		return 0;
	}
	while (f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
	{
		if (Info.fattrib & AM_DIR)
		{
			CString Sub;
			Sub.Format ("%s/%s", pPath, Info.fname);
			nCount += CountFiles (Sub, nDepth + 1) + 1;	// la carpeta también cuenta
		}
		else
		{
			nCount++;
		}
	}
	f_closedir (&Dir);
	return nCount;
}

boolean CEsxFolder::HasPending (void)
{
	if (!IsEnabled ())
	{
		return FALSE;
	}
	DIR Dir;
	FILINFO Info;
	if (f_opendir (&Dir, m_Folder) != FR_OK)
	{
		return FALSE;
	}
	boolean bAny = f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0';
	f_closedir (&Dir);
	return bAny;
}

// Crea una imagen FAT16 vacía de nMB megas (con tabla de particiones)
boolean CEsxFolder::CreateImage (u32 nMB)
{
	CString Path;
	Path.Format ("SD:/%s", m_Image);

	FIL File;
	if (f_open (&File, Path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
	{
		m_pError = N_("no se puede crear la imagen");
		return FALSE;
	}
	FRESULT Result = f_expand (&File, (FSIZE_t) nMB * 1024 * 1024, 1);
	if (Result != FR_OK)
	{
		// sin espacio contiguo: se crea igual escribiendo hasta el final
		Result = f_lseek (&File, (FSIZE_t) nMB * 1024 * 1024);
	}
	f_close (&File);
	if (Result != FR_OK)
	{
		m_pError = N_("no hay sitio en la SD para la imagen");
		return FALSE;
	}

	CHardDiskImage Image;
	if (Image.Open (m_Image) != 0)
	{
		m_pError = N_("no se puede abrir la imagen nueva");
		return FALSE;
	}

	FATFS *pFS = new FATFS;
	CDeviceNameService::Get ()->RemoveDevice (ESX_IMAGE_DEVICE, TRUE);
	CImageDevice *pDevice = new CImageDevice (&Image);
	CDeviceNameService::Get ()->AddDevice (ESX_IMAGE_DEVICE, pDevice, TRUE);

	static const MKFS_PARM Param = {FM_FAT, 2, 0, 0, 0};
	u8 *pWork = new u8[FF_MAX_SS * 32];
	Result = f_mkfs (ESX_IMAGE_VOLUME, &Param, pWork, FF_MAX_SS * 32);
	delete [] pWork;

	CDeviceNameService::Get ()->RemoveDevice (ESX_IMAGE_DEVICE, TRUE);
	delete pDevice;
	delete pFS;

	// esxDOS solo mira el total de sectores de 32 bits y el tipo "FAT16";
	// en volúmenes de menos de 32 MB FatFs pone el total en el campo de 16
	// bits (y FatFs exige que siga ahí) y el tipo "FAT". Se copia el total
	// también al campo de 32 bits y se pone "FAT16" (sin esto, esxDOS se
	// cuelga al montar la tarjeta).
	if (Result == FR_OK)
	{
		u8 Sector[512];
		if (Image.ReadSector (0, Sector))
		{
			u32 nStart = Sector[446 + 8] | (Sector[446 + 9] << 8)
				   | (Sector[446 + 10] << 16) | (Sector[446 + 11] << 24);
			if (Image.ReadSector (nStart, Sector))
			{
				u32 nTotal16 = Sector[19] | (Sector[20] << 8);
				if (nTotal16 != 0)
				{
					Sector[32] = nTotal16 & 0xFF;
					Sector[33] = nTotal16 >> 8;
					Sector[34] = Sector[35] = 0;
				}
				memcpy (Sector + 54, "FAT16   ", 8);
				Image.WriteSector (nStart, Sector);
			}
		}
	}

	Image.Flush ();
	Image.Close ();

	if (Result != FR_OK)
	{
		m_pError = N_("no se puede formatear la imagen");
		return FALSE;
	}
	CLogger::Get ()->Write (From, LogNotice, "imagen %s creada (%u MB)", m_Image, nMB);
	return TRUE;
}

// Nombre que se usa dentro de la imagen: esxDOS escribe los nombres cortos
// (8.3) tal cual, y los bytes de 0x80 en adelante el Spectrum los pinta como
// gráficos o palabras clave del BASIC ("nombres raros y largos"). Las letras
// acentuadas, la ñ, la ç... (página de códigos 850 de FatFs) pasan a su letra
// sin acento y lo demás a '_'.
void CEsxFolder::SafeName (const char *pIn, char *pOut, unsigned nSize)
{
	static const struct { u8 nCode; char c; } Map[] =
	{
		{0xA0, 'a'}, {0x85, 'a'}, {0x83, 'a'}, {0x84, 'a'}, {0xC6, 'a'}, {0x86, 'a'},
		{0xB5, 'A'}, {0xB7, 'A'}, {0xB6, 'A'}, {0x8E, 'A'}, {0xC7, 'A'}, {0x8F, 'A'},
		{0x82, 'e'}, {0x8A, 'e'}, {0x88, 'e'}, {0x89, 'e'},
		{0x90, 'E'}, {0xD4, 'E'}, {0xD2, 'E'}, {0xD3, 'E'},
		{0xA1, 'i'}, {0x8D, 'i'}, {0x8C, 'i'}, {0x8B, 'i'},
		{0xD6, 'I'}, {0xDE, 'I'}, {0xD7, 'I'}, {0xD8, 'I'},
		{0xA2, 'o'}, {0x95, 'o'}, {0x93, 'o'}, {0x94, 'o'}, {0xE4, 'o'},
		{0xE0, 'O'}, {0xE3, 'O'}, {0xE2, 'O'}, {0x99, 'O'}, {0xE5, 'O'},
		{0xA3, 'u'}, {0x97, 'u'}, {0x96, 'u'}, {0x81, 'u'},
		{0xE9, 'U'}, {0xEB, 'U'}, {0xEA, 'U'}, {0x9A, 'U'},
		{0xA4, 'n'}, {0xA5, 'N'}, {0x87, 'c'}, {0x80, 'C'},
	};

	unsigned n = 0;
	for (; *pIn != '\0' && n < nSize - 1; pIn++)
	{
		u8 c = (u8) *pIn;
		char cOut = '_';
		if (c >= 0x20 && c < 0x7F && c != '?')
		{
			cOut = (char) c;
		}
		else
		{
			for (unsigned i = 0; i < sizeof Map / sizeof Map[0]; i++)
			{
				if (Map[i].nCode == c)
				{
					cOut = Map[i].c;
					break;
				}
			}
		}
		pOut[n++] = cOut;
	}
	pOut[n] = '\0';
}

// Copia pSrc en pDst por medio de un fichero temporal, comprueba la copia
// leyéndola otra vez y solo entonces la pone en su sitio (sustituyendo al
// que hubiera). Así, si algo falla, no se pierde ni el original ni lo que ya
// estaba en la imagen.
boolean CEsxFolder::CopyVerified (const char *pSrc, const char *pDstDir, const char *pDst,
				  const FILINFO &rInfo)
{
	if (m_pBuffer == 0)
	{
		m_pBuffer = new u8[COPY_BUFFER_SIZE];
		m_pBuffer2 = new u8[COPY_BUFFER_SIZE];
	}

	CString Temp;
	Temp.Format ("%s/%s", pDstDir, TEMP_NAME);

	FIL In, Out;
	if (f_open (&In, pSrc, FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		m_pError = N_("no se puede leer un fichero de la carpeta");
		return FALSE;
	}
	FRESULT Result = f_open (&Out, Temp, FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		f_close (&In);
		// en la raíz de una FAT16 caben 512 entradas
		m_pError = Result == FR_DENIED ? N_("raíz de la tarjeta llena: usa subcarpetas")
					       : N_("no se puede escribir en la tarjeta");
		return FALSE;
	}

	boolean bOK = TRUE;
	for (;;)
	{
		UINT nRead = 0, nWritten = 0;
		if (f_read (&In, m_pBuffer, COPY_BUFFER_SIZE, &nRead) != FR_OK)
		{
			m_pError = N_("no se puede leer un fichero de la carpeta");
			bOK = FALSE;
			break;
		}
		if (nRead == 0)
		{
			break;
		}
		if (f_write (&Out, m_pBuffer, nRead, &nWritten) != FR_OK || nWritten != nRead)
		{
			m_pError = N_("la tarjeta está llena");
			bOK = FALSE;
			break;
		}
		if (m_nTotal > 0)
		{
			m_nBytesDone += nRead;
			Progress ();		// el aviso se pinta como mucho cada 200 ms
		}
	}
	if (f_close (&Out) != FR_OK)
	{
		bOK = FALSE;
	}

	// comprobación: la copia se lee otra vez y se compara con el original
	if (bOK)
	{
		FIL Check;
		bOK = f_open (&Check, Temp, FA_READ | FA_OPEN_EXISTING) == FR_OK;
		if (bOK)
		{
			bOK = f_size (&Check) == rInfo.fsize && f_lseek (&In, 0) == FR_OK;
			while (bOK)
			{
				UINT nRead1 = 0, nRead2 = 0;
				if (   f_read (&In, m_pBuffer, COPY_BUFFER_SIZE, &nRead1) != FR_OK
				    || f_read (&Check, m_pBuffer2, COPY_BUFFER_SIZE, &nRead2) != FR_OK
				    || nRead1 != nRead2 || memcmp (m_pBuffer, m_pBuffer2, nRead1) != 0)
				{
					bOK = FALSE;
					break;
				}
				if (nRead1 == 0)
				{
					break;
				}
			}
			f_close (&Check);
		}
		if (!bOK)
		{
			m_pError = N_("la copia no coincide con el original");
		}
	}
	f_close (&In);

	if (!bOK)
	{
		f_unlink (Temp);
		return FALSE;
	}

	// misma fecha que el original y al sitio definitivo
	FILINFO Time;
	Time.fdate = rInfo.fdate;
	Time.ftime = rInfo.ftime;
	f_utime (Temp, &Time);
	f_unlink (pDst);			// el que hubiera (si no hay, da igual)
	if (f_rename (Temp, pDst) != FR_OK)
	{
		f_unlink (Temp);
		m_pError = N_("no se puede escribir en la tarjeta");
		return FALSE;
	}
	return TRUE;
}

void CEsxFolder::Progress (void)
{
	if (m_pProgress != 0)
	{
		m_pProgress (m_nDone, m_nTotal, (unsigned) (m_nBytesDone >> 20), (unsigned) (m_nBytesTotal >> 20),
			     m_pProgressParam);
	}
}

// Una carpeta que no se puede importar: sus ficheros se quedan en la carpeta
void CEsxFolder::SkipTree (const char *pPath, unsigned nDepth)
{
	DIR Dir;
	FILINFO Info;
	if (nDepth <= MAX_DEPTH && f_opendir (&Dir, pPath) == FR_OK)
	{
		while (f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
		{
			if (Info.fattrib & AM_DIR)
			{
				CString Sub;
				Sub.Format ("%s/%s", pPath, Info.fname);
				SkipTree (Sub, nDepth + 1);
			}
			else
			{
				m_nFailed++;
			}
			m_nDone++;
		}
		f_closedir (&Dir);
	}
	m_nDone++;
	Progress ();
}

// Carpeta -> imagen. Lo que se copia bien se borra de la carpeta; lo que
// no, se queda (y cuenta en m_nFailed). Las subcarpetas vacías se borran.
void CEsxFolder::ImportTree (const char *pSrc, const char *pDst, unsigned nDepth)
{
	if (nDepth > MAX_DEPTH)
	{
		return;
	}

	DIR Dir;
	FILINFO Info;
	if (f_opendir (&Dir, pSrc) != FR_OK)
	{
		return;
	}

	// los nombres se guardan antes de borrar nada: borrar mientras se recorre
	// el directorio puede saltarse entradas
	struct TEntry { char Name[FF_MAX_LFN + 1]; FILINFO Info; };
	unsigned nMax = 64, nCount = 0;
	TEntry *pEntries = new TEntry[nMax];
	while (f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
	{
		if (nCount == nMax)
		{
			TEntry *pNew = new TEntry[nMax * 2];
			memcpy (pNew, pEntries, nMax * sizeof (TEntry));
			delete [] pEntries;
			pEntries = pNew;
			nMax *= 2;
		}
		strcpy (pEntries[nCount].Name, Info.fname);
		pEntries[nCount].Info = Info;
		nCount++;
	}
	f_closedir (&Dir);

	for (unsigned i = 0; i < nCount; i++)
	{
		const FILINFO &rInfo = pEntries[i].Info;
		char Safe[FF_MAX_LFN + 1];
		SafeName (pEntries[i].Name, Safe, sizeof Safe);

		CString Src, Dst;
		Src.Format ("%s/%s", pSrc, pEntries[i].Name);
		Dst.Format ("%s/%s", pDst, Safe);

		if (rInfo.fattrib & AM_DIR)
		{
			FILINFO Existing;
			if (f_stat (Dst, &Existing) != FR_OK)
			{
				FRESULT Result = f_mkdir (Dst);
				if (Result != FR_OK)
				{
					m_pError = Result == FR_DENIED ? N_("raíz de la tarjeta llena: usa subcarpetas")
								       : N_("no se puede escribir en la tarjeta");
					SkipTree (Src, nDepth + 1);
					continue;
				}
			}
			else if (!(Existing.fattrib & AM_DIR))
			{
				m_pError = N_("en la tarjeta ya hay un fichero con ese nombre");
				SkipTree (Src, nDepth + 1);
				continue;
			}
			ImportTree (Src, Dst, nDepth + 1);
			f_unlink (Src);			// solo se borra si se ha quedado vacía
			m_nDone++;
			Progress ();
			continue;
		}

		FILINFO Existing;
		boolean bExists = f_stat (Dst, &Existing) == FR_OK;
		m_nDone++;
		if (bExists && (Existing.fattrib & AM_DIR))
		{
			m_pError = N_("en la tarjeta ya hay una carpeta con ese nombre");
			m_nFailed++;
		}
		else if (CopyVerified (Src, pDst, Dst, rInfo) && f_unlink (Src) == FR_OK)
		{
			m_nImported++;
			if (bExists)
			{
				m_nReplaced++;
			}
		}
		else
		{
			m_nFailed++;
		}
		Progress ();
	}

	delete [] pEntries;
}

// Imagen -> carpeta, sin sustituir nada de la carpeta (lo de la carpeta es
// más nuevo): para pasar a una imagen mayor. FALSE si algo no se pudo copiar.
boolean CEsxFolder::ExportTree (const char *pSrc, const char *pDst, unsigned nDepth)
{
	if (nDepth > MAX_DEPTH)
	{
		return FALSE;
	}
	f_mkdir (pDst);

	DIR Dir;
	FILINFO Info;
	if (f_opendir (&Dir, pSrc) != FR_OK)
	{
		return FALSE;
	}
	boolean bOK = TRUE;
	while (bOK && f_readdir (&Dir, &Info) == FR_OK && Info.fname[0] != '\0')
	{
		CString Src, Dst;
		Src.Format ("%s/%s", pSrc, Info.fname);
		Dst.Format ("%s/%s", pDst, Info.fname);
		if (Info.fattrib & AM_DIR)
		{
			bOK = ExportTree (Src, Dst, nDepth + 1);
			continue;
		}
		FILINFO Existing;
		if (f_stat (Dst, &Existing) == FR_OK)
		{
			continue;		// la de la carpeta es más nueva
		}
		bOK = CopyVerified (Src, pDst, Dst, Info);
	}
	f_closedir (&Dir);
	return bOK;
}

boolean CEsxFolder::Import (TProgressHandler *pProgress, void *pParam)
{
	m_pError = 0;
	m_nImported = m_nReplaced = m_nFailed = m_nDone = 0;
	m_pProgress = pProgress;
	m_pProgressParam = pParam;

	u64 nNeeded = FolderSize (m_Folder, 0) * 11 / 10;

	// ¿hay imagen y cabe lo que hay que importar?
	boolean bCreate = TRUE;
	CHardDiskImage Image;
	if (Image.Open (m_Image) == 0)
	{
		FATFS *pFS = new FATFS;
		if (MountImage (&Image, pFS))
		{
			bCreate = FALSE;
			DWORD nFree = 0;
			FATFS *pVol;
			if (f_getfree (ESX_IMAGE_VOLUME, &nFree, &pVol) == FR_OK)
			{
				u64 nFreeBytes = (u64) nFree * pVol->csize * 512;
				if (nFreeBytes < nNeeded)
				{
					// no cabe: el contenido pasa a la carpeta (sin pisar lo
					// nuevo) y se hace una imagen mayor con todo
					if (pProgress != 0)
					{
						pProgress (0, 0, 0, 0, pParam);	// "preparando"
					}
					if (!ExportTree (ESX_IMAGE_VOLUME, m_Folder, 0))
					{
						UnmountImage (pFS);
						delete pFS;
						Image.Close ();
						m_pError = N_("no hay sitio en la SD para agrandar la tarjeta");
						return FALSE;
					}
					bCreate = TRUE;
				}
			}
			UnmountImage (pFS);
		}
		delete pFS;
		Image.Close ();
	}

	if (bCreate)
	{
		u64 nAll = FolderSize (m_Folder, 0) * 5 / 4 + 4 * 1024 * 1024;
		u32 nMB = MIN_IMAGE_MB;
		while ((u64) nMB * 1024 * 1024 < nAll && nMB < MAX_IMAGE_MB)
		{
			nMB *= 2;
		}
		if (!CreateImage (nMB))
		{
			return FALSE;
		}
	}

	if (Image.Open (m_Image) != 0)
	{
		m_pError = N_("no se puede abrir la imagen");
		return FALSE;
	}

	m_nTotal = CountFiles (m_Folder, 0);
	m_nBytesTotal = FolderSize (m_Folder, 0, FALSE);
	m_nBytesDone = 0;
	Progress ();

	FATFS *pFS = new FATFS;
	if (MountImage (&Image, pFS))
	{
		f_unlink (ESX_IMAGE_VOLUME "/" TEMP_NAME);	// restos de una importación cortada
		ImportTree (m_Folder, ESX_IMAGE_VOLUME, 0);
	}
	else
	{
		m_pError = N_("la imagen no tiene un sistema FAT");
	}
	UnmountImage (pFS);
	delete pFS;
	Image.Flush ();
	Image.Close ();

	m_pProgress = 0;
	m_nTotal = 0;
	CLogger::Get ()->Write (From, LogNotice, "importados %u (%u sustituidos), %u sin importar",
				m_nImported, m_nReplaced, m_nFailed);
	return m_nFailed == 0;
}
