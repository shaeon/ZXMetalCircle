#include <stdio.h>
#include <stdlib.h>
#include "ff.h"
#include "diskio.h"
static FILE *img;
DSTATUS disk_initialize (BYTE pdrv) { if (pdrv) return STA_NOINIT; if (!img) img = fopen (getenv ("IMG"), "r+b"); return img ? 0 : STA_NOINIT; }
DSTATUS disk_status (BYTE pdrv) { return (pdrv == 0 && img) ? 0 : STA_NOINIT; }
DRESULT disk_read (BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) { fseek (img, (long) sector * 512, SEEK_SET); return fread (buff, 512, count, img) == count ? RES_OK : RES_ERROR; }
DRESULT disk_write (BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) { fseek (img, (long) sector * 512, SEEK_SET); return fwrite (buff, 512, count, img) == count ? RES_OK : RES_ERROR; }
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff) { if (cmd == GET_SECTOR_SIZE) *(WORD *) buff = 512; return RES_OK; }
DWORD get_fattime (void) { return 0; }
void *ff_memalloc (UINT n) { return malloc (n); }
void ff_memfree (void *p) { free (p); }
