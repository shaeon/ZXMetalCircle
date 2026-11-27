// Las listas de ficheros (filetypes.h, la del Spectrum en spectrum.cpp) dan
// lo mismo que las cinco listas que había antes, copiadas aquí tal cual:
// el navegador, la web, los ZIP (con lo que se puede escribir), la pantalla
// de unidades y lo que hacía LoadAnyFile con cada extensión.
//
//   (con las mismas FLAGS y CORE que las demás pruebas de la Action)
//   g++ $FLAGS -o filetypes_test filetypes_test.cpp ../../src/filetypes.cpp $CORE roms_host.S
#include "testutil.h"
#include "filetypes.h"
#include <string>

static boolean HasExtension (const char *pName, const char *pExt)
{
	size_t nLen = strlen (pName);
	size_t nExtLen = strlen (pExt);
	return nLen > nExtLen && strcasecmp (pName + nLen - nExtLen, pExt) == 0;
}

static bool AnyOf (const char *pName, std::initializer_list<const char *> Ext)
{
	for (const char *p : Ext) if (HasExtension (pName, p)) return true;
	return false;
}

// menu.cpp, IsLoadableName
static bool OldBrowser (const char *n)
{
	return AnyOf (n, {".tap", ".tzx", ".pzx", ".z80", ".sna", ".szx", ".csw", ".rom", ".mdr",
			  ".mdv", ".dsk", ".vhd", ".img", ".raw", ".hdf", ".zip", ".p", ".sp", ".pok"});
}

// webserver.cpp, IsLoadable
static bool OldWeb (const char *n)
{
	return AnyOf (n, {".tap", ".tzx", ".pzx", ".csw", ".z80", ".sna", ".szx", ".rom", ".mdr", ".mdv",
			  ".dsk", ".vhd", ".img", ".raw", ".hdf", ".zip", ".p", ".sp"});
}

// kernel.cpp, ZipLoadable (y webserver.cpp, IsZipEntryLoadable: la misma lista)
static bool OldZip (const char *pName, bool *pWritable)
{
	static const char *Ext[] = {".tap", ".tzx", ".pzx", ".csw", ".z80", ".sna", ".szx", ".rom",
				    ".dsk", ".mdr", ".mdv"};
	unsigned nLen = strlen (pName);
	*pWritable = false;
	if (   (nLen > 2 && strcasecmp (pName + nLen - 2, ".p") == 0)
	    || (nLen > 3 && strcasecmp (pName + nLen - 3, ".sp") == 0))
	{
		return true;
	}
	for (unsigned i = 0; i < sizeof Ext / sizeof Ext[0]; i++)
	{
		if (nLen > 4 && strcasecmp (pName + nLen - 4, Ext[i]) == 0)
		{
			*pWritable = i >= 8;
			return true;
		}
	}
	return false;
}

// menu.cpp, IsUnitName: cinta, disquete, disco duro, cartucho, microdrive
static int OldUnit (const char *n)
{
	if (AnyOf (n, {".tap", ".tzx", ".pzx", ".csw"}))	return 1;
	if (AnyOf (n, {".dsk"}))				return 2;
	if (AnyOf (n, {".vhd", ".img", ".raw", ".hdf"}))	return 3;
	if (AnyOf (n, {".rom"}))				return 4;
	if (AnyOf (n, {".mdr", ".mdv"}))			return 5;
	return 0;
}
static int NewUnit (const char *n)
{
	switch (GetFileKind (n))
	{
	case FileTape:		return 1;
	case FileDisk:		return 2;
	case FileHardDisk:	return 3;
	case FileCartridge:	return 4;
	case FileMicrodrive:	return 5;
	default:		return 0;
	}
}

// kernel.cpp, LoadAnyFile: qué se hacía con cada uno
enum TAction { Pokes, ZX81, Snapshot, Cartridge, Microdrive, Disk, Zip, HardDisk, Tape };
static TAction OldAction (const char *pPath)
{
	size_t nLen = strlen (pPath);
	const char *pExt = nLen > 4 ? pPath + nLen - 4 : "";
	if (strcasecmp (pExt, ".pok") == 0) return Pokes;
	if (nLen > 2 && strcasecmp (pPath + nLen - 2, ".p") == 0) return ZX81;
	if (   strcasecmp (pExt, ".z80") == 0 || strcasecmp (pExt, ".sna") == 0
	    || strcasecmp (pExt, ".szx") == 0 || (nLen > 3 && strcasecmp (pPath + nLen - 3, ".sp") == 0))
		return Snapshot;
	if (strcasecmp (pExt, ".rom") == 0) return Cartridge;
	if (strcasecmp (pExt, ".mdr") == 0 || strcasecmp (pExt, ".mdv") == 0) return Microdrive;
	if (strcasecmp (pExt, ".dsk") == 0) return Disk;
	if (strcasecmp (pExt, ".zip") == 0) return Zip;
	if (   strcasecmp (pExt, ".vhd") == 0 || strcasecmp (pExt, ".img") == 0
	    || strcasecmp (pExt, ".raw") == 0 || strcasecmp (pExt, ".hdf") == 0) return HardDisk;
	return Tape;
}
static TAction NewAction (const char *pPath)
{
	switch (GetFileKind (pPath))
	{
	case FilePokes:		return Pokes;
	case FileProgramZX81:	return ZX81;
	case FileSnapshot:	return Snapshot;
	case FileCartridge:	return Cartridge;
	case FileMicrodrive:	return Microdrive;
	case FileDisk:		return Disk;
	case FileZip:		return Zip;
	case FileHardDisk:	return HardDisk;
	default:		return Tape;
	}
}

int main (void)
{
	S = new CSpectrum (&kb, &tape);
	SetFileTypes (S->GetFileTypes ());

	static const char *Ext[] = {".tap", ".tzx", ".pzx", ".csw", ".z80", ".sna", ".szx", ".sp", ".rom",
				    ".mdr", ".mdv", ".dsk", ".vhd", ".img", ".raw", ".hdf", ".p", ".pok", ".zip",
				    ".scr", ".txt", ".trd", ".scl", ".bin", ".tap.bak", ".zip.txt", ".z8", ""};
	static const char *Base[] = {"juego", "JUEGO", "a", "x.sp", "x.p", ""};
	unsigned nNames = 0, nBad = 0;
	for (const char *b : Base)
	{
		for (const char *e : Ext)
		{
			for (int nUpper = 0; nUpper < 2; nUpper++)
			{
				std::string Name = std::string (b) + e;
				if (nUpper) for (char &c : Name) c = toupper (c);
				const char *n = Name.c_str ();
				bool bOldW, bNewWr;
				boolean bNewW;
				bool bOldZ = OldZip (n, &bOldW);
				bool bNewZ = IsZipLoadable (n, &bNewW);
				bNewWr = bNewZ && bNewW;
				bool bOK =    OldBrowser (n) == (bool) IsBrowserFile (n)
					   && OldWeb (n) == (bool) IsWebLoadable (n)
					   && bOldZ == bNewZ && (bOldZ && bOldW) == bNewWr
					   && OldUnit (n) == NewUnit (n)
					   && OldAction (n) == NewAction (n);
				if (!bOK)
				{
					printf ("distinto: \"%s\"\n", n);
					nBad++;
				}
				nNames++;
			}
		}
	}
	char Text[80];
	snprintf (Text, sizeof Text, "las mismas respuestas que las listas antiguas (%u nombres)", nNames);
	chk (Text, nBad == 0);

	chk ("ZIP: un .dsk se puede escribir", IsZipLoadable ("a.dsk") && ({boolean b; IsZipLoadable ("a.dsk", &b); b;}));
	chk ("ZIP: un .tap no", ({boolean b; IsZipLoadable ("a.tap", &b); !b;}));
	chk ("web: un .pok no, en el navegador sí", !IsWebLoadable ("a.pok") && IsBrowserFile ("a.pok"));
	SetFileTypes (0);
	chk ("sin lista de la máquina, solo los ZIP", GetFileKind ("a.zip") == FileZip && !IsBrowserFile ("a.tap"));

	printf (fails == 0 ? "todo OK\n" : "%d FALLOS\n", fails);
	return fails != 0;
}
