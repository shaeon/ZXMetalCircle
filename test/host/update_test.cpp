// Prueba del instalador de actualizaciones (src/updater.cpp) sobre una
// carpeta temporal que hace de SD. Los paquetes se fabrican aquí mismo:
// zips sin comprimir, con el CRC de cada entrada.
#include "updater.h"
#include "zipfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

// --- La "SD": una carpeta, contando lo que se escribe ---------------------

class CDirFS : public CUpdateFS
{
public:
	std::string Root;
	std::vector<std::string> Written;	// en orden
	std::string FailOn;			// escribir este fichero falla

	std::string P (const char *pPath) { return Root + "/" + pPath; }
	boolean GetSize (const char *pPath, unsigned *pSize)
	{
		struct stat st;
		if (stat (P (pPath).c_str (), &st) != 0 || !S_ISREG (st.st_mode)) return FALSE;
		*pSize = st.st_size;
		return TRUE;
	}
	boolean Read (const char *pPath, u8 *pBuffer, unsigned nSize)
	{
		FILE *f = fopen (P (pPath).c_str (), "rb");
		if (!f) return FALSE;
		bool ok = fread (pBuffer, 1, nSize, f) == nSize;
		fclose (f);
		return ok;
	}
	boolean Write (const char *pPath, const u8 *pData, unsigned nSize)
	{
		std::string Name (pPath);
		if (!FailOn.empty () && Name == FailOn + ".new") return FALSE;
		FILE *f = fopen (P (pPath).c_str (), "wb");
		if (!f) return FALSE;
		bool ok = fwrite (pData, 1, nSize, f) == nSize;
		fclose (f);
		if (Name.size () > 4 && Name.substr (Name.size () - 4) == ".new")
			Written.push_back (Name.substr (0, Name.size () - 4));
		return ok;
	}
	boolean Remove (const char *pPath) { return unlink (P (pPath).c_str ()) == 0; }
	boolean Rename (const char *pFrom, const char *pTo)
	{
		unsigned n;
		if (GetSize (pTo, &n)) return FALSE;		// como FatFs
		return rename (P (pFrom).c_str (), P (pTo).c_str ()) == 0;
	}
	boolean MakeDir (const char *pPath)
	{
		mkdir (P (pPath).c_str (), 0755);
		struct stat st;
		return stat (P (pPath).c_str (), &st) == 0 && S_ISDIR (st.st_mode);
	}
};

static std::string ReadText (CDirFS &fs, const char *pPath)
{
	unsigned n;
	if (!fs.GetSize (pPath, &n)) return "(no existe)";
	std::string s (n, '\0');
	fs.Read (pPath, (u8 *) &s[0], n);
	return s;
}

static void WriteText (CDirFS &fs, const char *pPath, const char *pText)
{
	FILE *f = fopen (fs.P (pPath).c_str (), "wb");
	fputs (pText, f);
	fclose (f);
}

// --- Zips sin comprimir -----------------------------------------------------

typedef std::vector<std::pair<std::string, std::string> > TFiles;

static void Put16 (std::string &s, unsigned v) { s += (char) v; s += (char) (v >> 8); }
static void Put32 (std::string &s, unsigned v) { Put16 (s, v & 0xFFFF); Put16 (s, v >> 16); }

static std::string MakeZip (const TFiles &Files, bool bCorrupt = false)
{
	std::string Zip, Dir;
	for (size_t i = 0; i < Files.size (); i++)
	{
		const std::string &Name = Files[i].first, &Data = Files[i].second;
		u32 nCRC = CZipFile::CRC32 ((const u8 *) Data.data (), Data.size ());
		if (bCorrupt && i == Files.size () - 1) nCRC ^= 1;
		unsigned nOffset = Zip.size ();
		Put32 (Zip, 0x04034B50); Put16 (Zip, 10); Put16 (Zip, 0); Put16 (Zip, 0);
		Put32 (Zip, 0); Put32 (Zip, nCRC); Put32 (Zip, Data.size ()); Put32 (Zip, Data.size ());
		Put16 (Zip, Name.size ()); Put16 (Zip, 0);
		Zip += Name; Zip += Data;
		Put32 (Dir, 0x02014B50); Put16 (Dir, 20); Put16 (Dir, 10); Put16 (Dir, 0); Put16 (Dir, 0);
		Put32 (Dir, 0); Put32 (Dir, nCRC); Put32 (Dir, Data.size ()); Put32 (Dir, Data.size ());
		Put16 (Dir, Name.size ()); Put16 (Dir, 0); Put16 (Dir, 0); Put16 (Dir, 0); Put16 (Dir, 0);
		Put32 (Dir, 0); Put32 (Dir, nOffset);
		Dir += Name;
	}
	unsigned nDirOffset = Zip.size ();
	Zip += Dir;
	Put32 (Zip, 0x06054B50); Put16 (Zip, 0); Put16 (Zip, 0);
	Put16 (Zip, Files.size ()); Put16 (Zip, Files.size ());
	Put32 (Zip, Dir.size ()); Put32 (Zip, nDirOffset); Put16 (Zip, 0);
	return Zip;
}

static TUpdateResult Install (CDirFS &fs, const std::string &Zip, const std::string *pPrev,
			      bool bReplace, const char *pVersion)
{
	TUpdateResult R;
	fs.Written.clear ();
	CUpdater::Install (&fs, (const u8 *) Zip.data (), Zip.size (),
			   pPrev ? (const u8 *) pPrev->data () : 0, pPrev ? pPrev->size () : 0,
			   bReplace, pVersion, &R);
	return R;
}

static bool WasWritten (CDirFS &fs, const char *pPath)
{
	for (size_t i = 0; i < fs.Written.size (); i++)
		if (fs.Written[i] == pPath) return true;
	return false;
}

int main (void)
{
	char Tmp[] = "/tmp/updtestXXXXXX";
	CDirFS fs;
	fs.Root = mkdtemp (Tmp);

	TFiles V1 = {
		{"kernel.img", "kernel 1"}, {"kernel8.img", "kernel8 1"},
		{"cmdline.txt", "model=128k"}, {"config.txt", "arm_64bit=0"},
		{"excluir.txt", "*.bak"}, {"tnfs.txt", "vexed4.alioth.net"},
		{"roms/48.rom", "rom 48"}, {"roms/tk95-es.rom", "rom tk95"},
		{"locales/pt.json", "{\"a\": 1}"}, {"README.md", "leeme 1"},
		{"esxdos_i/SYS/ESXDOS.SYS", "esx 1"}, {"esxdos_i/SYS/CONFIG/ESXDOS.CFG", "cfg 1"},
	};
	std::string Zip1 = MakeZip (V1);

	// 1. SD vacía: se instala todo
	TUpdateResult R = Install (fs, Zip1, 0, false, "1.06");
	chk ("SD vacía: se instala bien", R.Status == UpdateOK);
	chk ("SD vacía: se escriben los ficheros normales y la configuración",
	     ReadText (fs, "roms/48.rom") == "rom 48" && ReadText (fs, "cmdline.txt") == "model=128k");
	chk ("SD vacía: sin paquete anterior, esxdos_i no se toca",
	     ReadText (fs, "esxdos_i/SYS/ESXDOS.SYS") == "(no existe)");
	chk ("SD vacía: no queda la marca update/EN_CURSO", ReadText (fs, UPDATE_MARKER) == "(no existe)");
	chk ("SD vacía: los kernels, los últimos",
	     fs.Written.size () >= 2 && CUpdater::IsKernel (fs.Written[fs.Written.size () - 1].c_str ())
	     && CUpdater::IsKernel (fs.Written[fs.Written.size () - 2].c_str ())
	     && !CUpdater::IsKernel (fs.Written[fs.Written.size () - 3].c_str ()));
	chk ("SD vacía: no quedan ficheros .new", ReadText (fs, "roms/48.rom.new") == "(no existe)");

	// 2. El mismo paquete otra vez: nada que hacer
	R = Install (fs, Zip1, 0, false, "1.06");
	chk ("mismo paquete: ningún fichero cambiado", R.Status == UpdateOK && R.nChanged == 0 && fs.Written.empty ());

	// 3. El usuario cambia su configuración; la 1.07 cambia varias cosas
	WriteText (fs, "cmdline.txt", "model=48k joystick=kempston");
	WriteText (fs, "excluir.txt", "*.tmp");
	TFiles V2 = V1;
	V2[0].second = "kernel 2";			// kernel.img
	V2[2].second = "model=128k ulaplus=1";		// cmdline.txt nuevo
	V2[4].second = "*.bak\n*.old";			// excluir.txt nuevo
	V2[6].second = "rom 48 bis";			// roms/48.rom
	V2[11].second = "cfg 2";			// ESXDOS.CFG
	V2.push_back ({"locales/ca.json", "{}"});	// fichero nuevo
	V2.push_back ({"esxdos_i/BIN/NUEVO", "nuevo"});	// esxDOS nuevo
	std::string Zip2 = MakeZip (V2);

	R = Install (fs, Zip2, &Zip1, false, "1.07");
	chk ("1.07 conservando la configuración: bien", R.Status == UpdateOK);
	chk ("cmdline.txt del usuario intacto", ReadText (fs, "cmdline.txt") == "model=48k joystick=kempston");
	chk ("el nuevo, en cmdline.txt.nuevo", ReadText (fs, "cmdline.txt.nuevo") == "model=128k ulaplus=1");
	chk ("excluir.txt del usuario intacto y el nuevo en .nuevo",
	     ReadText (fs, "excluir.txt") == "*.tmp" && ReadText (fs, "excluir.txt.nuevo") == "*.bak\n*.old");
	chk ("config.txt, igual que el nuevo: ni se toca ni .nuevo",
	     !WasWritten (fs, "config.txt") && ReadText (fs, "config.txt.nuevo") == "(no existe)");
	chk ("se avisa de los dos .nuevo", R.nNotes == 2);
	chk ("ROM y kernel cambiados se copian",
	     ReadText (fs, "roms/48.rom") == "rom 48 bis" && ReadText (fs, "kernel.img") == "kernel 2");
	chk ("lo que no cambia no se escribe",
	     !WasWritten (fs, "roms/tk95-es.rom") && !WasWritten (fs, "README.md") && !WasWritten (fs, "kernel8.img"));
	chk ("fichero nuevo del paquete, copiado", ReadText (fs, "locales/ca.json") == "{}");
	chk ("esxdos_i: solo lo nuevo respecto al paquete anterior",
	     ReadText (fs, "esxdos_i/BIN/NUEVO") == "nuevo" && !WasWritten (fs, "esxdos_i/SYS/ESXDOS.SYS"));
	chk ("esxdos_i: su configuración no se toca al conservar",
	     !WasWritten (fs, "esxdos_i/SYS/CONFIG/ESXDOS.CFG"));
	chk ("cambiados: 4 (kernel, ROM, ca.json y NUEVO); los .nuevo no cuentan", R.nChanged == 4);

	// 3b. Ya en la 1.07, reinstalarla: sin paquete anterior (es la misma
	// versión, como hace el kernel) y con los .nuevo ya puestos, nada que hacer
	R = Install (fs, Zip2, 0, false, "1.07");
	chk ("reinstalar la 1.07: nada cambiado, nada escrito, sin avisos",
	     R.Status == UpdateOK && R.nChanged == 0 && fs.Written.empty () && R.nNotes == 0);

	// 4. Volver a instalar la 1.07 poniendo la configuración nueva
	R = Install (fs, Zip2, &Zip1, true, "1.07");
	chk ("poniendo la configuración nueva: bien", R.Status == UpdateOK);
	chk ("cmdline.txt pasa a ser el nuevo", ReadText (fs, "cmdline.txt") == "model=128k ulaplus=1");
	chk ("el del usuario, en cmdline.txt.bak", ReadText (fs, "cmdline.txt.bak") == "model=48k joystick=kempston");
	chk ("esxdos_i: con poner la nueva, su configuración sí se copia",
	     ReadText (fs, "esxdos_i/SYS/CONFIG/ESXDOS.CFG") == "cfg 2");

	// 5. Volver a la 1.06 (paquete guardado): la ROM vuelve a ser la vieja
	R = Install (fs, Zip1, &Zip2, false, "1.06");
	chk ("vuelta a la 1.06: bien y la ROM vuelve", R.Status == UpdateOK && ReadText (fs, "roms/48.rom") == "rom 48");
	chk ("vuelta a la 1.06: kernel.img vuelve", ReadText (fs, "kernel.img") == "kernel 1");

	// 6. Paquete dañado: no se toca nada
	std::string Bad = MakeZip (V2, true);
	WriteText (fs, "roms/48.rom", "rom 48");
	R = Install (fs, Bad, &Zip1, false, "1.07");
	chk ("paquete dañado: se rechaza", R.Status == UpdateBadPackage);
	chk ("paquete dañado: no se escribe nada ni queda la marca",
	     fs.Written.empty () && ReadText (fs, UPDATE_MARKER) == "(no existe)");

	// 7. Falla una escritura a medias: se para y queda la marca
	fs.FailOn = "roms/48.rom";
	R = Install (fs, Zip2, &Zip1, false, "1.07");
	chk ("error al escribir: se informa del fichero", R.Status == UpdateWriteError && strcmp (R.Failed, "roms/48.rom") == 0);
	chk ("error al escribir: queda update/EN_CURSO con la versión", ReadText (fs, UPDATE_MARKER) == "1.07");
	chk ("error al escribir: los kernels no se han tocado", ReadText (fs, "kernel.img") == "kernel 1");
	fs.FailOn = "";
	R = Install (fs, Zip2, &Zip1, false, "1.07");
	chk ("reintento: termina bien y quita la marca",
	     R.Status == UpdateOK && ReadText (fs, UPDATE_MARKER) == "(no existe)" && ReadText (fs, "kernel.img") == "kernel 2");

	// 8. Versiones
	chk ("versiones: 1.06 < 1.07, 1.10 > 1.9, 1.06 = 1.06",
	     CUpdater::CompareVersions ("1.06", "1.07") < 0 && CUpdater::CompareVersions ("1.10", "1.9") > 0
	     && CUpdater::CompareVersions ("1.06", "1.06") == 0);

	// 9. La respuesta de GitHub (con el formato de su API) y el plan B
	{
		const char *JSON =
			"{\"url\": \"https://api.github.com/repos/shaeon/ZXMetalCircle/releases/1\",\n"
			" \"tag_name\": \"v1.07\", \"name\": \"ZXMetalCircle 1.07\", \"draft\": false,\n"
			" \"assets\": [ {\"name\": \"otro.txt\", \"browser_download_url\": "
			"\"https://github.com/shaeon/ZXMetalCircle/releases/download/v1.07/otro.txt\"},\n"
			"  {\"name\": \"ZXMetalCircle-1.07-sd.zip\", \"size\": 11825238,\n"
			"   \"browser_download_url\": \"https://github.com/shaeon/ZXMetalCircle/releases/download/v1.07/ZXMetalCircle-1.07-sd.zip\"} ]}";
		char V[16], U[256];
		bool bOK = CUpdater::ParseRelease (JSON, strlen (JSON), V, sizeof V, U, sizeof U);
		chk ("API: versión 1.07 y el zip de la SD (no otro fichero)",
		     bOK && strcmp (V, "1.07") == 0
		     && strcmp (U, "https://github.com/shaeon/ZXMetalCircle/releases/download/v1.07/ZXMetalCircle-1.07-sd.zip") == 0);
		const char *Limit = "{\"message\":\"API rate limit exceeded for 1.2.3.4.\"}";
		chk ("API: límite de consultas -> no hay versión", !CUpdater::ParseRelease (Limit, strlen (Limit), V, sizeof V, U, sizeof U));
		bOK = CUpdater::ParseTagURL ("https://github.com/shaeon/ZXMetalCircle/releases/tag/v1.07", V, sizeof V, U, sizeof U);
		chk ("plan B: de .../releases/tag/v1.07, la versión y el zip",
		     bOK && strcmp (V, "1.07") == 0
		     && strcmp (U, "https://github.com/shaeon/ZXMetalCircle/releases/download/v1.07/ZXMetalCircle-1.07-sd.zip") == 0);
		chk ("plan B: sin /releases/tag/ no hay versión",
		     !CUpdater::ParseTagURL ("https://github.com/shaeon/ZXMetalCircle/releases", V, sizeof V, U, sizeof U));
		char N[64];
		CUpdater::PackageName ("1.07", N, sizeof N);
		chk ("nombre del paquete: update/ZXMetalCircle-1.07-sd.zip", strcmp (N, "update/ZXMetalCircle-1.07-sd.zip") == 0);
	}

	// 9. Opcional: un paquete real (UPDATE_ZIP=ruta), en otra carpeta
	if (getenv ("UPDATE_ZIP"))
	{
		FILE *f = fopen (getenv ("UPDATE_ZIP"), "rb");
		std::string Real;
		char Buf[65536]; size_t n;
		while ((n = fread (Buf, 1, sizeof Buf, f)) > 0) Real.append (Buf, n);
		fclose (f);
		char Tmp2[] = "/tmp/updrealXXXXXX";
		CDirFS sd; sd.Root = mkdtemp (Tmp2);
		TUpdateResult R1 = Install (sd, Real, 0, false, "1.06");
		printf ("      paquete real en SD vacía: %u copiados, %u sin tocar\n", R1.nChanged, R1.nUnchanged);
		chk ("paquete real: se instala", R1.Status == UpdateOK && R1.nChanged > 100);
		TUpdateResult R2 = Install (sd, Real, &Real, false, "1.06");
		chk ("paquete real otra vez: nada que copiar", R2.Status == UpdateOK && R2.nChanged == 0);
		char Cmd2[96]; snprintf (Cmd2, sizeof Cmd2, "rm -rf %s", sd.Root.c_str ());
		if (system (Cmd2) != 0) printf ("no se pudo borrar %s\n", sd.Root.c_str ());
	}

	char Cmd[96];
	snprintf (Cmd, sizeof Cmd, "rm -rf %s", fs.Root.c_str ());
	if (system (Cmd) != 0) printf ("no se pudo borrar %s\n", fs.Root.c_str ());
	printf (fails ? "%d fallos\n" : "todo OK\n", fails);
	return fails ? 1 : 0;
}
