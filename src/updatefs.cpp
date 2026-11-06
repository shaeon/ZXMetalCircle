//
// updatefs.cpp
//
// Los ficheros de la SD (FatFs) para el instalador de actualizaciones
//
#include "updatefs.h"
#include <fatfs/ff.h>
#include <circle/string.h>

static CString SD (const char *pPath)
{
	CString Path;
	Path.Format ("SD:/%s", pPath);
	return Path;
}

boolean CFatUpdateFS::GetSize (const char *pPath, unsigned *pSize)
{
	FILINFO Info;
	if (f_stat (SD (pPath), &Info) != FR_OK || (Info.fattrib & AM_DIR))
	{
		return FALSE;
	}
	*pSize = Info.fsize;
	return TRUE;
}

boolean CFatUpdateFS::Read (const char *pPath, u8 *pBuffer, unsigned nSize)
{
	FIL File;
	if (f_open (&File, SD (pPath), FA_READ | FA_OPEN_EXISTING) != FR_OK)
	{
		return FALSE;
	}
	UINT nRead = 0;
	FRESULT Result = f_read (&File, pBuffer, nSize, &nRead);
	f_close (&File);
	return Result == FR_OK && nRead == nSize;
}

boolean CFatUpdateFS::Write (const char *pPath, const u8 *pData, unsigned nSize)
{
	FIL File;
	if (f_open (&File, SD (pPath), FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
	{
		return FALSE;
	}
	UINT nWritten = 0;
	FRESULT Result = f_write (&File, pData, nSize, &nWritten);
	// f_close escribe lo que queda en la caché: si falla, el fichero no vale
	if (f_close (&File) != FR_OK)
	{
		return FALSE;
	}
	return Result == FR_OK && nWritten == nSize;
}

boolean CFatUpdateFS::Remove (const char *pPath)
{
	return f_unlink (SD (pPath)) == FR_OK;
}

boolean CFatUpdateFS::Rename (const char *pFrom, const char *pTo)
{
	return f_rename (SD (pFrom), SD (pTo)) == FR_OK;
}

boolean CFatUpdateFS::MakeDir (const char *pPath)
{
	FRESULT Result = f_mkdir (SD (pPath));
	return Result == FR_OK || Result == FR_EXIST;
}
