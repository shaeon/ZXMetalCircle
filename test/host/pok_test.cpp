// Pokes: el lector de .POK, aplicar y quitar trucos (con bancos del 128K y
// valores que se preguntan) y los POKE escritos a mano (src/pokfile.cpp)
//
//   g++ -O2 -std=c++17 -I. -I../../src -o pok_test pok_test.cpp ../../src/pokfile.cpp
//   ./pok_test [carpeta con .pok]	(con carpeta, además los lee todos)
#include "pokfile.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <dirent.h>

static int fails = 0;
static void chk (const char *pWhat, bool bOK)
{
	printf ("%s  %s\n", bOK ? "ok   " : "FALLO", pWhat);
	if (!bOK) fails++;
}

// Memoria de prueba: 8 páginas de 16 KB y la paginación de un 128K (5, 2, n)
class CTestMemory : public CPokMemory
{
public:
	CTestMemory (void) : m_nPaged (0), m_b48K (false)	{ memset (m_RAM, 0, sizeof m_RAM); }
	u8 Read (unsigned nBank, u16 nAddress) override
	{
		return *Cell (nBank, nAddress);
	}
	void Write (unsigned nBank, u16 nAddress, u8 nValue) override
	{
		if (nBank == POK_BANK_CURRENT && nAddress < 0x4000)
		{
			m_nROMWrites++;
			return;
		}
		*Cell (nBank, nAddress) = nValue;
	}
	u8 *Cell (unsigned nBank, u16 nAddress)
	{
		if (nBank == POK_BANK_CURRENT || m_b48K)
		{
			static const unsigned Slot[4] = {7, 5, 2, 0};	// 7: la ROM (no se escribe)
			unsigned nPage = (nAddress >> 14) == 3 ? m_nPaged : Slot[nAddress >> 14];
			return &m_RAM[nPage][nAddress & 0x3FFF];
		}
		return &m_RAM[nBank & 7][nAddress & 0x3FFF];
	}
	u8 m_RAM[8][0x4000];
	unsigned m_nPaged;
	bool m_b48K;
	unsigned m_nROMWrites = 0;
};

static bool ParseText (CPokFile *pFile, const char *pText)
{
	return pFile->Parse (pText, strlen (pText));
}

static void ReadAll (const char *pDir, unsigned *pFiles, unsigned *pBad, unsigned *pTrainers)
{
	DIR *pDirectory = opendir (pDir);
	if (pDirectory == 0)
	{
		return;
	}
	static CPokFile File;
	struct dirent *pEntry;
	while ((pEntry = readdir (pDirectory)) != 0)
	{
		if (pEntry->d_name[0] == '.')
		{
			continue;
		}
		std::string Path = std::string (pDir) + "/" + pEntry->d_name;
		size_t nLen = Path.size ();
		if (nLen > 4 && strcasecmp (Path.c_str () + nLen - 4, ".pok") == 0)
		{
			FILE *pFile = fopen (Path.c_str (), "rb");
			std::vector<char> Text;
			char Buffer[4096];
			size_t n;
			while (pFile != 0 && (n = fread (Buffer, 1, sizeof Buffer, pFile)) > 0)
			{
				Text.insert (Text.end (), Buffer, Buffer + n);
			}
			if (pFile != 0)
			{
				fclose (pFile);
			}
			(*pFiles)++;
			if (!File.Parse (Text.data (), Text.size ()))
			{
				(*pBad)++;
				printf ("      no se lee: %s\n", Path.c_str ());
			}
			else
			{
				*pTrainers += File.GetCount ();
			}
		}
		else if (pEntry->d_type == DT_DIR)
		{
			ReadAll (Path.c_str (), pFiles, pBad, pTrainers);
		}
	}
	closedir (pDirectory);
}

int main (int argc, char **argv)
{
	static CPokFile Pok;
	static CTestMemory Mem;

	// 1. Un .POK normal (con \r\n, como los de ZXDB)
	const char *pBarbarian =
		"N[Pt1]Infinite Energy\r\n"
		"M 8 36220 0 0\r\n"
		"M 8 36516 0 0\r\n"
		"Z 8 39371 200 0\r\n"
		"N[Pt2]Infinite Energy\r\n"
		"M 8 36192 0 0\r\n"
		"Z 8 39232 200 0\r\n"
		"N[Pt1][2P]1 Hit to Kill\r\n"
		"Z 8 36798 1 0\r\n"
		"Y\r\n";
	chk ("se lee un .POK", ParseText (&Pok, pBarbarian));
	chk ("tres trucos", Pok.GetCount () == 3 && Pok.GetFileCount () == 3);
	chk ("nombres", strcmp (Pok.GetName (0), "[Pt1]Infinite Energy") == 0
			&& strcmp (Pok.GetName (2), "[Pt1][2P]1 Hit to Kill") == 0);
	chk ("pokes por truco", Pok.GetPokeCount (0) == 3 && Pok.GetPokeCount (1) == 2 && Pok.GetPokeCount (2) == 1);
	chk ("ninguno activo ni con pregunta", !Pok.IsActive (0) && !Pok.NeedsInput (0) && !Pok.IsManual (0));

	// aplicar y quitar: se guarda lo que había
	Mem.m_RAM[2][36220 - 32768] = 0x35;	// 36220 está en la página 2 (8000-BFFF)
	Mem.m_RAM[2][39371 - 32768] = 0x03;
	chk ("aplicar", Pok.Activate (0, &Mem) && Pok.IsActive (0));
	chk ("aplicado en la memoria", Mem.m_RAM[2][36220 - 32768] == 0 && Mem.m_RAM[2][39371 - 32768] == 200);
	Pok.Activate (0, &Mem);			// otra vez: no debe perder lo guardado
	Pok.Deactivate (0, &Mem);
	chk ("quitar: vuelve lo que había", !Pok.IsActive (0) && Mem.m_RAM[2][36220 - 32768] == 0x35
					    && Mem.m_RAM[2][39371 - 32768] == 0x03);

	// 2. Bancos del 128K: la dirección viene entera (49593) y va dentro de la página
	const char *pBank =
		"NInfinite Lives\n"
		"Z 6 49593 0 0\n"
		"NBank 0 high\n"
		"Z 0 44796 2 0\n"
		"NOriginal in file\n"
		"Z 8 50000 99 77\n"
		"Y\n";
	chk ("se lee con bancos", ParseText (&Pok, pBank) && Pok.GetCount () == 3);
	Mem.m_nPaged = 0;
	Mem.m_RAM[6][49593 & 0x3FFF] = 0x3D;
	Pok.Activate (0, &Mem);
	chk ("banco 6: en la página 6 aunque no esté paginada", Mem.m_RAM[6][49593 & 0x3FFF] == 0
								&& Mem.m_RAM[0][49593 & 0x3FFF] == 0);
	Pok.Deactivate (0, &Mem);
	chk ("banco 6: se quita", Mem.m_RAM[6][49593 & 0x3FFF] == 0x3D);
	Pok.Activate (1, &Mem);
	chk ("banco 0 con 44796: dentro de la página 0", Mem.m_RAM[0][44796 & 0x3FFF] == 2);
	Pok.Deactivate (1, &Mem);
	Mem.m_nPaged = 3;
	Mem.m_RAM[3][50000 & 0x3FFF] = 11;
	Pok.Activate (2, &Mem);
	chk ("banco 8: la página que esté puesta (3)", Mem.m_RAM[3][50000 & 0x3FFF] == 99);
	Pok.Deactivate (2, &Mem);
	chk ("original del fichero al quitarlo", Mem.m_RAM[3][50000 & 0x3FFF] == 77);
	Mem.m_nPaged = 0;

	// 48K: el banco no cuenta (la memoria de prueba lo hace como el kernel)
	Mem.m_b48K = true;
	Mem.m_RAM[0][49593 & 0x3FFF] = 0x3D;
	Mem.m_RAM[7][49593 & 0x3FFF] = 0x3D;	// 49593 & 0x3FFF: ahí no debe ir
	Pok.Activate (0, &Mem);
	chk ("48K: banco 6 va a la dirección normal", Mem.m_RAM[0][49593 & 0x3FFF] == 0 && Mem.m_RAM[7][49593 & 0x3FFF] == 0x3D);
	Pok.Deactivate (0, &Mem);
	Mem.m_b48K = false;

	// 3. Valor 256: se pregunta
	const char *pAsk =
		"NLives\n"
		"M 8 40000 256 0\n"
		"Z 8 40001 0 0\n"
		"Y\n";
	chk ("se lee con 256", ParseText (&Pok, pAsk) && Pok.NeedsInput (0));
	Pok.Activate (0, &Mem, 9);
	chk ("256: el valor escrito", Mem.m_RAM[2][40000 - 32768] == 9 && Mem.m_RAM[2][40001 - 32768] == 0);

	// 4. A la ROM no se escribe
	const char *pROM = "NROM\nZ 8 1000 0 0\nY\n";
	unsigned nROMWrites = Mem.m_nROMWrites;
	chk ("se lee uno a la ROM", ParseText (&Pok, pROM));
	Pok.Activate (0, &Mem);
	chk ("a la ROM no se escribe", Mem.m_nROMWrites == nROMWrites + 1);

	// 5. Los que no valen (y los trucos que había se quedan)
	ParseText (&Pok, pBarbarian);
	chk ("sin trucos: no vale", !ParseText (&Pok, "Y\n"));
	chk ("poke sin N: no vale", !ParseText (&Pok, "Z 8 40000 0 0\nY\n"));
	chk ("faltan números: no vale", !ParseText (&Pok, "Nx\nZ 8 40000 0\nY\n"));
	chk ("valor 257: no vale", !ParseText (&Pok, "Nx\nZ 8 40000 257 0\nY\n"));
	chk ("dirección 65536: no vale", !ParseText (&Pok, "Nx\nZ 8 65536 0 0\nY\n"));
	chk ("letras: no vale", !ParseText (&Pok, "Nx\nZ 8 4OOOO 0 0\nY\n"));
	chk ("un fichero cualquiera: no vale", !ParseText (&Pok, "Hola, esto no es un POK\n"));
	chk ("se quedan los de antes", Pok.GetCount () == 3 && strcmp (Pok.GetName (1), "[Pt2]Infinite Energy") == 0);

	// un N sin pokes no se cuenta; sin Y al final, vale
	chk ("N vacío y sin Y", ParseText (&Pok, "NVacio\nNBueno\nZ 8 40000 0 0\nNOtro vacio\n")
				 && Pok.GetCount () == 1 && strcmp (Pok.GetName (0), "Bueno") == 0);
	// nombres: sin espacios al final, Latin-1 como '?', y cortados
	char Long[400] = "N";
	for (unsigned i = 1; i < 250; i++) Long[i] = 'a';
	strcat (Long, "\nZ 8 40000 0 0\nNCaf\xE9  \nZ 8 40000 0 0\nNNi\xC3\xB1o\nZ 8 40000 0 0\nY\n");
	chk ("nombres raros", ParseText (&Pok, Long) && Pok.GetCount () == 3
			   && strlen (Pok.GetName (0)) == POK_NAME_LEN - 1
			   && strcmp (Pok.GetName (1), "Caf?") == 0 && strcmp (Pok.GetName (2), "Ni\xC3\xB1o") == 0);

	// 6. POKE a mano
	u16 Address[4];
	u8 Value[4];
	chk ("a mano: 35136 0", CPokFile::ParseManual ("35136 0", Address, Value, 4) == 1
				&& Address[0] == 35136 && Value[0] == 0);
	chk ("a mano: hexadecimal 8940H C9H", CPokFile::ParseManual (" 8940H  C9H ", Address, Value, 4) == 1
				&& Address[0] == 0x8940 && Value[0] == 0xC9);
	chk ("a mano: dos pares", CPokFile::ParseManual ("35136 0 35137 201", Address, Value, 4) == 2
				&& Address[1] == 35137 && Value[1] == 201);
	chk ("a mano: con coma, como en BASIC", CPokFile::ParseManual ("35136,0", Address, Value, 4) == 1);
	chk ("a mano: en la ROM no", CPokFile::ParseManual ("1000 0", Address, Value, 4) == 0);
	chk ("a mano: valor 256 no", CPokFile::ParseManual ("40000 256", Address, Value, 4) == 0);
	chk ("a mano: sin valor no", CPokFile::ParseManual ("40000", Address, Value, 4) == 0);
	chk ("a mano: A0 sin H no", CPokFile::ParseManual ("40000 A0", Address, Value, 4) == 0);
	chk ("a mano: vacío", CPokFile::ParseManual ("  ", Address, Value, 4) == 0);
	chk ("a mano: demasiados pares", CPokFile::ParseManual ("40000 1 40001 2", Address, Value, 1) == 0);
	u8 nInput;
	chk ("valor: 255 y FFH", CPokFile::ParseValue ("255", &nInput) && nInput == 255
			      && CPokFile::ParseValue ("FFH", &nInput) && nInput == 255);
	chk ("valor: 256 y nada no", !CPokFile::ParseValue ("256", &nInput) && !CPokFile::ParseValue ("", &nInput)
				  && !CPokFile::ParseValue ("1 2", &nInput));

	ParseText (&Pok, pBarbarian);
	int nManual = Pok.AddManual (35136, 7);
	chk ("a mano: va detrás de los del fichero", nManual == 3 && Pok.IsManual (3) && !Pok.IsManual (2)
					 && Pok.GetCount () == 4 && strcmp (Pok.GetName (3), "POKE 35136,7") == 0);
	Mem.m_RAM[2][35136 - 32768] = 0x55;
	Pok.Activate (3, &Mem);
	chk ("a mano: se aplica", Mem.m_RAM[2][35136 - 32768] == 7);
	chk ("a mano: se queda con otro .POK", ParseText (&Pok, pAsk) && Pok.GetCount () == 2
					     && Pok.IsManual (1) && Pok.IsActive (1));
	Pok.Deactivate (1, &Mem);
	chk ("a mano: se quita", Mem.m_RAM[2][35136 - 32768] == 0x55);
	chk ("a mano: en la ROM no", Pok.AddManual (1000, 0) < 0);
	for (unsigned i = 0; i < POK_MAX_MANUAL; i++) Pok.AddManual (40000 + i, 0);
	chk ("a mano: hay un máximo", Pok.AddManual (50000, 0) < 0);

	// 7. Reset y otro juego
	Pok.Activate (0, &Mem, 1);
	Pok.Forget ();
	chk ("reset: ninguno activo", !Pok.IsActive (0) && !Pok.IsActive (1));
	Pok.Clear ();
	chk ("otro juego: vacío", Pok.GetCount () == 0);

	// 8. Opcional: todos los .pok de una carpeta (la colección de ZXDB)
	if (argc > 1)
	{
		unsigned nFiles = 0, nBad = 0, nTrainers = 0;
		ReadAll (argv[1], &nFiles, &nBad, &nTrainers);
		printf ("      %u ficheros, %u trucos\n", nFiles, nTrainers);
		chk ("todos los .pok de la carpeta se leen", nFiles > 0 && nBad == 0);
	}

	printf (fails ? "%d FALLOS\n" : "todo bien\n", fails);
	return fails;
}
