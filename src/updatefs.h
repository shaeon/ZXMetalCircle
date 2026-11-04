//
// updatefs.h
//
// Los ficheros de la SD (FatFs) para el instalador de actualizaciones
//
#ifndef _zx_updatefs_h
#define _zx_updatefs_h

#include "updater.h"

class CFatUpdateFS : public CUpdateFS
{
public:
	boolean GetSize (const char *pPath, unsigned *pSize);
	boolean Read (const char *pPath, u8 *pBuffer, unsigned nSize);
	boolean Write (const char *pPath, const u8 *pData, unsigned nSize);
	boolean Remove (const char *pPath);
	boolean Rename (const char *pFrom, const char *pTo);
	boolean MakeDir (const char *pPath);
};

#endif
