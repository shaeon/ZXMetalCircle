//
// filetypes.cpp - Qué ficheros sabe cargar la máquina y qué es cada uno
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "filetypes.h"
#include <circle/util.h>

static const TFileType *s_pTypes = 0;

void SetFileTypes (const TFileType *pTypes)
{
	s_pTypes = pTypes;
}

static boolean HasExtension (const char *pName, const char *pExt)
{
	size_t nLen = strlen (pName);
	size_t nExtLen = strlen (pExt);
	return nLen > nExtLen && strcasecmp (pName + nLen - nExtLen, pExt) == 0;
}

TFileKind GetFileKind (const char *pName)
{
	if (HasExtension (pName, ".zip"))
	{
		return FileZip;
	}

	for (const TFileType *p = s_pTypes; p != 0 && p->pExt != 0; p++)
	{
		if (HasExtension (pName, p->pExt))
		{
			return p->Kind;
		}
	}

	return FileUnknown;
}

boolean IsBrowserFile (const char *pName)
{
	return GetFileKind (pName) != FileUnknown;
}

boolean IsWebLoadable (const char *pName)
{
	TFileKind Kind = GetFileKind (pName);
	return Kind != FileUnknown && Kind != FilePokes;
}

boolean IsZipLoadable (const char *pName, boolean *pWritable)
{
	TFileKind Kind = GetFileKind (pName);
	if (pWritable != 0)
	{
		*pWritable = Kind == FileDisk || Kind == FileMicrodrive;
	}
	return    Kind != FileUnknown && Kind != FileHardDisk
	       && Kind != FileZip && Kind != FilePokes;
}
