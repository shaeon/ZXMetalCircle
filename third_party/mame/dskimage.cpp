//
// dskimage.cpp
//
// Lectura basada en dsk_dsk.cpp y pistas MFM basadas en build_pc_track_mfm() de
// flopimg.cpp, ambos de MAME (license:BSD-3-Clause, copyright-holders: Olivier
// Galibert y otros).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "dskimage.h"

#define DSK_MAX_LONG_SECTOR	6352

static const char StdHeader[] = "MV - CPC";
static const char ExtHeader[] = "EXTENDED CPC DSK File\r\nDisk-Info\r\n";

CDskImage::CDskImage (void)
:	m_nTracks (0),
	m_nSides (0),
	m_bWriteProtected (false)
{
	memset (m_Track, 0, sizeof m_Track);
}

CDskImage::~CDskImage (void)
{
	Clear ();
}

void CDskImage::Clear (void)
{
	for (int t = 0; t < DSK_MAX_TRACKS; t++)
	{
		for (int s = 0; s < DSK_MAX_SIDES; s++)
		{
			for (int i = 0; i < DSK_MAX_SECTORS; i++)
			{
				delete [] m_Track[t][s].Sector[i].pData;
			}
		}
	}
	memset (m_Track, 0, sizeof m_Track);
	m_nTracks = m_nSides = 0;
}

static unsigned Word (const u8 *p)	{ return p[0] | (p[1] << 8); }

bool CDskImage::Load (const u8 *pData, unsigned nSize)
{
	Clear ();

	if (nSize < 0x100)
	{
		return false;
	}

	bool bExtended = memcmp (pData, ExtHeader, 16) == 0;
	if (!bExtended && memcmp (pData, StdHeader, 8) != 0)
	{
		return false;
	}

	int nTracks = pData[0x30];
	int nSides = pData[0x31];
	if (nTracks == 0 || nSides < 1 || nSides > 2)
	{
		return false;
	}
	if (nTracks > DSK_MAX_TRACKS)
	{
		nTracks = DSK_MAX_TRACKS;	// como MAME: las pistas de sobra se descartan
	}

	m_nTracks = nTracks;
	m_nSides = nSides;

	unsigned nOffset = 0x100;
	for (int t = 0; t < pData[0x30]; t++)
	{
		for (int s = 0; s < nSides; s++)
		{
			unsigned nTrackSize = bExtended ? pData[0x34 + t * nSides + s] << 8 : Word (pData + 0x32);
			unsigned nTrackOffset = nOffset;
			nOffset += nTrackSize;

			if (t >= nTracks || nTrackSize == 0 || nTrackOffset + 0x100 > nSize)
			{
				continue;
			}

			const u8 *pTrack = pData + nTrackOffset;
			if (memcmp (pTrack, "Track-Info", 10) != 0)
			{
				continue;
			}

			TDskTrack &T = m_Track[t][s];
			T.nSectors = pTrack[0x15] > DSK_MAX_SECTORS ? DSK_MAX_SECTORS : pTrack[0x15];
			T.nGap3 = pTrack[0x16];
			T.nFiller = pTrack[0x17];
			T.bPresent = T.nSectors > 0;
			u8 nTrackN = pTrack[0x14];

			// protecciones que MAME detecta por los tamaños de sector
			int nProtection = 0;
			for (int j = 1; j < T.nSectors; j++)
			{
				const u8 *pInfo = pTrack + 0x18 + j * 8;
				if (pInfo[3] == 2 && pTrack[0x18 + 3] == 6)	nProtection = 1;	// 6144 + 512...
				if (pInfo[3] == j && pTrack[0x18 + 3] == 0)	nProtection = 2;	// 128 + N*128...
			}

			unsigned nDataPos = nTrackOffset + 0x100;
			for (int j = 0; j < T.nSectors; j++)
			{
				const u8 *pInfo = pTrack + 0x18 + j * 8;
				TDskSector &S = T.Sector[j];
				S.C = pInfo[0];
				S.H = pInfo[1];
				S.R = pInfo[2];
				S.N = pInfo[3];
				S.ST1 = pInfo[4];
				S.ST2 = pInfo[5];
				S.nWeakFrom = 256;	// como build_pc_track_mfm de MAME
				S.nWeakLen = 0;

				unsigned nStored = bExtended ? Word (pInfo + 6) : (128u << (nTrackN & 7));
				unsigned nActual = nStored;
				if (bExtended && nProtection == 1)	nActual = 512;
				if (bExtended && nProtection == 2)	nActual = 128;

				// Extensión de EDSK para sectores débiles: varias copias del
				// sector seguidas (lo guardado es un múltiplo del tamaño real).
				// En la pista va una sola copia. Por ejemplo, Coin-Op Hits
				// guarda tres copias de sus sectores de 8K: son iguales en los
				// 6305 bytes útiles y solo cambian tras la vuelta de la pista.
				// Antes se escribían las tres seguidas, como un sector de 24K.
				// Cabecera con N mal guardado: un sector débil con varias
				// copias, cuyas copias no empiezan igual con el tamaño de su
				// cabecera pero sí con uno mayor. Robocop (SPIN Disk Man):
				// sector 2 de la pista 0 con N=0 y 1536 bytes, que son tres
				// copias de 512 (256 bytes de E5 y 256 débiles); el juego lo
				// lee con N=2 y, como el uPD765 compara N, con N=0 no lo
				// encontraba nunca (pantalla de rayas). Se toma ese N mayor.
				if (   bExtended && nProtection == 0 && (S.ST2 & 0x20) && S.N < 6
				    && nDataPos + nStored <= nSize)
				{
					const u8 *pFirst = pData + nDataPos;
					auto SamePrefix = [&] (unsigned nCopy) -> bool
					{
						unsigned nCmp = nCopy / 2 < 16 ? nCopy / 2 : 16;
						for (unsigned c = nCopy; c < nStored; c += nCopy)
						{
							if (memcmp (pFirst + c, pFirst, nCmp) != 0)
							{
								return false;
							}
						}
						return true;
					};

					unsigned nOwn = 128u << S.N;
					if (   nStored > nOwn && nStored % nOwn == 0
					    && !SamePrefix (nOwn))
					{
						for (u8 n = S.N + 1; n < 6; n++)
						{
							unsigned nCopy = 128u << n;
							if (nStored % nCopy == 0 && nStored / nCopy >= 2 && SamePrefix (nCopy))
							{
								S.N = n;
								break;
							}
						}
					}
				}

				unsigned nNative = 128u << (S.N < 6 ? S.N : 6);	// N >= 6: 8K
				if (   bExtended && nProtection == 0
				    && nStored > nNative && nStored % nNative == 0
				    && nDataPos + nStored <= nSize)
				{
					nActual = nNative;

					// Si las copias difieren a partir de un punto, lo que sigue
					// no está grabado en el disco: es lo que se lee al pasar el
					// índice y volver al principio de la pista (un sector de 8K
					// no cabe en una pista). Solo se graba hasta ahí, y el FDC
					// leerá el resto dando la vuelta, como en el disco real.
					// Si no, la pista sería tan larga que sus celdas quedarían
					// demasiado estrechas para el PLL y no se leería nada.
					const u8 *pFirst = pData + nDataPos;
					unsigned nSame = nNative;		// primer byte distinto
					unsigned nLastDiff = 0;			// último, más uno
					for (unsigned c = nNative; c < nStored; c += nNative)
					{
						for (unsigned i = 0; i < nNative; i++)
						{
							if (pFirst[c + i] != pFirst[i])
							{
								if (i < nSame)
								{
									nSame = i;
								}
								if (i + 1 > nLastDiff)
								{
									nLastDiff = i + 1;
								}
							}
						}
					}
					if (nSame < nNative)
					{
						if (S.N >= 6)
						{
							// sector de 8K: lo grabado acaba donde difieren
							if (nSame >= 128)
							{
								nActual = nSame;
							}
						}
						else
						{
							// sector débil normal (Speedlock): una copia entera,
							// débil solo donde las copias difieren, de la
							// primera a la última diferencia (como Fuse,
							// cpc_set_weak_range). Antes, hasta el final.
							S.nWeakFrom = nSame;
							S.nWeakLen = nLastDiff - nSame;
						}
					}
				}

				// Un sector de 8K (N >= 6) no cabe en una pista: en el disco solo
				// hay unos 6150-6330 bytes y el resto se lee dando la vuelta. Si
				// la imagen no deja ver dónde acaba (copias iguales o una sola),
				// se graban como mucho 6352 bytes, lo que cabe en una pista con
				// las celdas un 5 % más estrechas (dentro de lo que sigue el PLL).
				// Coin-Op Hits necesita 6305.
				if (S.N >= 6 && nActual > DSK_MAX_LONG_SECTOR)
				{
					nActual = DSK_MAX_LONG_SECTOR;
				}

				S.nLength = 0;
				S.pData = 0;
				if (!(S.ST1 & 0x04) && nActual > 0 && nDataPos + nActual <= nSize)
				{
					S.pData = new u8[nActual];
					if (S.pData != 0)
					{
						memcpy (S.pData, pData + nDataPos, nActual);
						S.nLength = nActual;
					}
				}

				nDataPos += nStored;
			}

			DetectSpeedlock (T);
		}
	}

	return true;
}

// Speedlock del +3 (como IsSpectrumSpeedlockTrack de SAMdisk): pista de 9
// sectores de 512 bytes, el segundo con error de datos (el sector débil) y
// "SPEEDLOCK" en el primero, en el byte 304 o en el 176. Si la primera mitad
// del sector débil es constante, solo cambian 32 bytes desde el 336; si no,
// el sector entero. Sin esto la zona débil iba del byte 256 al final (como en
// MAME) y el cargador de Dragon Ninja, que comprueba que los bytes 304-319
// del sector salgan iguales, se quedaba colgado con la pantalla en negro.
void CDskImage::DetectSpeedlock (TDskTrack &T)
{
	if (T.nSectors != 9)
	{
		return;
	}
	TDskSector &S0 = T.Sector[0];
	TDskSector &S1 = T.Sector[1];
	if (   S0.N != 2 || S1.N != 2 || S0.pData == 0 || S1.pData == 0
	    || S0.nLength < 512 || S1.nLength < 512
	    || !(S1.ST2 & 0x20) || (S1.ST2 & 0x40))
	{
		return;
	}
	if (   memcmp (S0.pData + 304, "SPEEDLOCK", 9) != 0
	    && memcmp (S0.pData + 176, "SPEEDLOCK", 9) != 0)
	{
		return;
	}

	if (memcmp (S1.pData, S1.pData + 1, 255) == 0)
	{
		S1.nWeakFrom = 336;
		S1.nWeakLen = 32;
	}
	else
	{
		S1.nWeakFrom = 0;
		S1.nWeakLen = 0;		// hasta el final
	}
}

// Disco del +3 recién formateado: 40 pistas, una cara, 9 sectores de 512 bytes
// numerados del 1 al 9 y relleno E5
bool CDskImage::CreateBlank3 (void)
{
	Clear ();
	m_nTracks = 40;
	m_nSides = 1;

	for (int t = 0; t < 40; t++)
	{
		TDskTrack &T = m_Track[t][0];
		T.bPresent = true;
		T.nGap3 = 0x52;
		T.nFiller = 0xE5;
		T.nSectors = 9;
		for (int j = 0; j < 9; j++)
		{
			TDskSector &S = T.Sector[j];
			S.C = t;
			S.H = 0;
			S.R = j + 1;
			S.N = 2;
			S.ST1 = S.ST2 = 0;
			S.nLength = 512;
			S.pData = new u8[512];
			if (S.pData == 0)
			{
				return false;
			}
			memset (S.pData, 0xE5, 512);
		}
	}

	// Todo E5, como deja el disco el FORMAT del +3 (el +3DOS lo trata como
	// formato estándar del +3)
	return true;
}

// Disco de 3,5" de 720K formateado para +3DOS: 80 pistas, dos caras, 9
// sectores de 512 numerados del 1 al 9 y relleno E5, con la especificación
// del disco en el primer sector (tipo 3: doble cara, 80 pistas, 9 x 512,
// bloques de 2K; la misma que dejan las herramientas de PC, como Cronomantic)
bool CDskImage::CreateBlank720 (void)
{
	Clear ();
	m_nTracks = 80;
	m_nSides = 2;

	for (int t = 0; t < 80; t++)
	{
		for (int h = 0; h < 2; h++)
		{
			TDskTrack &T = m_Track[t][h];
			T.bPresent = true;
			T.nGap3 = 0x52;
			T.nFiller = 0xE5;
			T.nSectors = 9;
			for (int j = 0; j < 9; j++)
			{
				TDskSector &S = T.Sector[j];
				S.C = t;
				S.H = h;
				S.R = j + 1;
				S.N = 2;
				S.ST1 = S.ST2 = 0;
				S.nLength = 512;
				S.pData = new u8[512];
				if (S.pData == 0)
				{
					return false;
				}
				memset (S.pData, 0xE5, 512);
			}
		}
	}

	static const u8 Spec[] = {0x03, 0x81, 0x50, 0x09, 0x02, 0x02, 0x04, 0x04, 0x2A, 0x52};
	memcpy (m_Track[0][0].Sector[0].pData, Spec, sizeof Spec);
	return true;
}

//------------------------------------------------------------------------------
// MFM

namespace
{

class CMFMWriter
{
public:
	CMFMWriter (u8 *pCells, unsigned nCells) : m_pCells (pCells), m_nCells (nCells), m_nPos (0), m_nLast (0), m_nCRC (0xFFFF)
	{
		memset (pCells, 0, (nCells + 7) / 8);
	}

	void Raw16 (u16 nRaw)
	{
		for (int i = 15; i >= 0; i--)
		{
			Cell ((nRaw >> i) & 1);
		}
		m_nLast = nRaw & 1;
	}

	void Byte (u8 nByte, bool bCRC = true)
	{
		for (int i = 7; i >= 0; i--)
		{
			int nBit = (nByte >> i) & 1;
			Cell (!m_nLast && !nBit);	// reloj
			Cell (nBit);			// dato
			m_nLast = nBit;
		}
		if (bCRC)
		{
			UpdateCRC (nByte);
		}
	}

	void Bytes (u8 nByte, unsigned nCount)
	{
		while (nCount--)
		{
			Byte (nByte);
		}
	}

	void SyncA1 (void)	{ Raw16 (0x4489); UpdateCRC (0xA1); }	// A1 sin un pulso de reloj
	void SyncC2 (void)	{ Raw16 (0x5224); }			// C2 sin un pulso de reloj
	void StartCRC (void)	{ m_nCRC = 0xFFFF; }
	void WriteCRC (bool bBad)
	{
		u16 nCRC = bBad ? m_nCRC ^ 0xFFFF : m_nCRC;
		Byte (nCRC >> 8, false);
		Byte (nCRC & 0xFF, false);
	}

	unsigned Position (void) const	{ return m_nPos; }

private:
	void Cell (int nBit)
	{
		if (m_nPos < m_nCells)
		{
			if (nBit)
			{
				m_pCells[m_nPos >> 3] |= 0x80 >> (m_nPos & 7);
			}
			m_nPos++;
		}
	}

	void UpdateCRC (u8 nByte)
	{
		m_nCRC ^= nByte << 8;
		for (int i = 0; i < 8; i++)
		{
			m_nCRC = (m_nCRC & 0x8000) ? (m_nCRC << 1) ^ 0x1021 : m_nCRC << 1;
		}
	}

	u8 *m_pCells;
	unsigned m_nCells, m_nPos;
	int m_nLast;
	u16 m_nCRC;
};

}	// namespace

// Cómo se reparte la pista. Por defecto, como build_pc_track_mfm de MAME:
// hueco 4a (80), IAM y hueco 1 (50), y el hueco 3 que dice la imagen. Pero
// el hueco 3 de una EDSK suele ser el del formateo, no el real, y en un disco
// lleno (10 sectores de 512, o un sector de 6K u 8K) no cabe todo en las
// 6250 bytes de una pista a 250 kbps y 300 rpm. Como en el disco real, se
// recortan primero los huecos, luego el preámbulo, y solo si aun así no cabe
// se alarga la pista (celdas de bit más estrechas, como Coin-Op Hits). Una
// pista alargada sin necesidad hace que los bytes lleguen antes de tiempo y
// algunos cargadores no llegan a recogerlos (Ghouls'n Ghosts: desbordamiento).
#define TRACK_BYTES		6250		// 100000 celdas de bit
#define GAP3_MIN		4

void CDskImage::GetTrackLayout (const TDskTrack &T, TTrackLayout *pLayout)
{
	unsigned nData = 0;
	for (int j = 0; j < T.nSectors; j++)
	{
		nData += T.Sector[j].nLength;
	}
	unsigned nFixed = T.nSectors * 62 + nData;	// identificación, sincronismos, CRC y datos

	pLayout->nGap4a = 80;
	pLayout->bIAM = true;
	pLayout->nGap1 = 50;
	pLayout->nGap3 = T.nGap3;

	unsigned nPre = 80 + 16 + 50;
	if (nPre + nFixed + T.nSectors * pLayout->nGap3 > TRACK_BYTES && T.nSectors > 0)
	{
		// huecos 3 más cortos
		unsigned nRoom = TRACK_BYTES > nPre + nFixed ? TRACK_BYTES - nPre - nFixed : 0;
		unsigned nGap3 = nRoom / T.nSectors;
		pLayout->nGap3 = nGap3 < GAP3_MIN ? GAP3_MIN : (nGap3 < T.nGap3 ? nGap3 : T.nGap3);

		// preámbulo mínimo, sin IAM (+3DOS no la necesita)
		if (nPre + nFixed + T.nSectors * pLayout->nGap3 > TRACK_BYTES)
		{
			pLayout->nGap4a = 16;
			pLayout->bIAM = false;
			pLayout->nGap1 = 0;
			nPre = 16;
		}
	}

	unsigned nTotal = nPre + nFixed + T.nSectors * pLayout->nGap3;
	pLayout->nCells = nTotal > TRACK_BYTES ? (nTotal + 16) * 16 : TRACK_BYTES * 16;
}

unsigned CDskImage::GetTrackCells (int nCyl, int nSide) const
{
	if (nCyl < m_nTracks && nSide < m_nSides && m_Track[nCyl][nSide].bPresent)
	{
		TTrackLayout Layout;
		GetTrackLayout (m_Track[nCyl][nSide], &Layout);
		return Layout.nCells;
	}
	return TRACK_BYTES * 16;
}

bool CDskImage::BuildTrack (int nCyl, int nSide, u8 *pCells, unsigned nCells,
			    unsigned *pWeakStart, unsigned *pWeakEnd, unsigned *pnWeak) const
{
	memset (pCells, 0, (nCells + 7) / 8);
	if (pnWeak != 0)
	{
		*pnWeak = 0;
	}

	if (nCyl >= m_nTracks || nSide >= m_nSides || !m_Track[nCyl][nSide].bPresent)
	{
		return false;		// pista sin formato: sin transiciones
	}

	const TDskTrack &T = m_Track[nCyl][nSide];
	CMFMWriter W (pCells, nCells);

	TTrackLayout Layout;
	GetTrackLayout (T, &Layout);

	W.Bytes (0x4E, Layout.nGap4a);
	if (Layout.bIAM)
	{
		W.Bytes (0x00, 12);
		W.SyncC2 (); W.SyncC2 (); W.SyncC2 ();
		W.Byte (0xFC);
		W.Bytes (0x4E, Layout.nGap1);
	}

	for (int j = 0; j < T.nSectors; j++)
	{
		const TDskSector &S = T.Sector[j];

		// campo de identificación
		W.Bytes (0x00, 12);
		W.StartCRC ();
		W.SyncA1 (); W.SyncA1 (); W.SyncA1 ();
		// Error de CRC en la cabecera: ST1 con "error de datos" (0x20) y ST2
		// sin él. Como en el uPD765 (y en Fuse): ST1 0x20 solo dice que hubo
		// un error de CRC; ST2 0x20 dice que fue en los datos. Antes se
		// tomaba siempre como error en los datos (como MAME), y una
		// protección que esperara no encontrar el sector lo encontraba.
		bool bBadID = (S.ST1 & 0x20) && !(S.ST2 & 0x20);
		W.Byte (0xFE);
		W.Byte (S.C); W.Byte (S.H); W.Byte (S.R); W.Byte (S.N);
		W.WriteCRC (bBadID);
		W.Bytes (0x4E, 22);		// hueco 2

		// Sector sin datos en la imagen pero con error de CRC en los datos
		// (ST2 0x20): la marca de datos sí estaba; lo que sigue es el resto
		// de la pista, y el CRC no cuadra. Guillermo Tell (Opera) lee así el
		// sector 8 de la pista 40 (N=8, 32K, no cabe en la pista) y espera
		// ese error; sin la marca, el FDC respondía "sin marca de datos" y el
		// juego se quedaba en la pantalla de carga.
		if (S.pData == 0 && (S.ST2 & 0x20) && !(S.ST1 & 0x04) && !(S.ST2 & 0x01))
		{
			W.Bytes (0x00, 12);
			W.StartCRC ();
			W.SyncA1 (); W.SyncA1 (); W.SyncA1 ();
			W.Byte ((S.ST2 & 0x40) ? 0xF8 : 0xFB);
		}

		// campo de datos (si el sector tiene datos)
		if (S.pData != 0)
		{
			W.Bytes (0x00, 12);
			W.StartCRC ();
			W.SyncA1 (); W.SyncA1 (); W.SyncA1 ();
			W.Byte ((S.ST2 & 0x40) ? 0xF8 : 0xFB);	// marca de datos borrados o normal
			unsigned nDataStart = W.Position ();
			for (unsigned i = 0; i < S.nLength; i++)
			{
				W.Byte (S.pData[i]);
			}
			bool bBadCRC = (S.ST2 & 0x20) != 0;	// error en los datos
			W.WriteCRC (bBadCRC);

			// sector débil (como build_pc_track_mfm de MAME): desde el byte
			// 256, o desde donde difieren las copias de la imagen
			// Pero no en los sectores largos (N >= 6, 8K o más): no caben en
			// la pista y lo que sigue a sus datos es el resto de la pista,
			// no datos débiles. La protección de Opera (Corsarios, MOT) lee
			// el sector 8 de la pista 40 (N=8) y busca en él, a 1298 bytes de
			// su comienzo, los datos del sector 7, que está solapado dentro;
			// al salir al azar desde el byte 256, volvía a la pantalla de
			// selección. Fuse tampoco los trata como débiles.
			bool bWeak = bBadCRC && !(S.ST2 & 0x40) && S.N < 6;
			if (bWeak && pnWeak != 0 && *pnWeak < DSK_MAX_SECTORS
			    && nDataStart + S.nWeakFrom * 16 < W.Position ())
			{
				pWeakStart[*pnWeak] = nDataStart + S.nWeakFrom * 16;
				pWeakEnd[*pnWeak] = W.Position ();
				if (S.nWeakLen != 0 && nDataStart + (S.nWeakFrom + S.nWeakLen) * 16 < W.Position ())
				{
					pWeakEnd[*pnWeak] = nDataStart + (S.nWeakFrom + S.nWeakLen) * 16;
				}
				(*pnWeak)++;
			}
		}

		W.Bytes (0x4E, Layout.nGap3);	// hueco 3
	}

	while (W.Position () < nCells)
	{
		W.Byte (0x4E);
	}

	return true;
}

//------------------------------------------------------------------------------
// Decodificación de una pista escrita

namespace
{

class CMFMReader
{
public:
	CMFMReader (const u8 *pCells, unsigned nCells) : m_pCells (pCells), m_nCells (nCells), m_nPos (0), m_nShift (0) {}

	bool Done (void) const		{ return m_nPos >= m_nCells; }
	unsigned Position (void) const	{ return m_nPos; }

	int Cell (void)
	{
		int nBit = (m_pCells[m_nPos >> 3] >> (7 - (m_nPos & 7))) & 1;
		m_nPos++;
		m_nShift = (m_nShift << 1) | nBit;
		return nBit;
	}

	// avanza hasta tres A1 (0x4489) seguidos; false si se acaba la pista
	bool FindSync (unsigned nLimit)
	{
		unsigned nEnd = m_nPos + nLimit;
		int nCount = 0;
		while (!Done () && m_nPos < nEnd)
		{
			Cell ();
			if ((m_nShift & 0xFFFF) == 0x4489)
			{
				nCount++;
				if (nCount == 3)
				{
					return true;
				}
				// el siguiente A1 empieza justo a continuación
				for (int i = 0; i < 16 && !Done (); i++)
				{
					Cell ();
				}
				if ((m_nShift & 0xFFFF) == 0x4489)
				{
					nCount++;
					if (nCount == 3)
					{
						return true;
					}
					for (int i = 0; i < 16 && !Done (); i++)
					{
						Cell ();
					}
					if ((m_nShift & 0xFFFF) == 0x4489)
					{
						return true;
					}
				}
				nCount = 0;
			}
		}
		return false;
	}

	u8 Byte (void)
	{
		u8 nByte = 0;
		for (int i = 0; i < 8 && !Done (); i++)
		{
			Cell ();		// reloj
			nByte = (nByte << 1) | Cell ();
		}
		return nByte;
	}

private:
	const u8 *m_pCells;
	unsigned m_nCells, m_nPos;
	u32 m_nShift;
};

u16 CRC (const u8 *p, unsigned n, u16 nCRC)
{
	while (n--)
	{
		nCRC ^= *p++ << 8;
		for (int i = 0; i < 8; i++)
		{
			nCRC = (nCRC & 0x8000) ? (nCRC << 1) ^ 0x1021 : nCRC << 1;
		}
	}
	return nCRC;
}

}	// namespace

void CDskImage::DecodeTrack (int nCyl, int nSide, const u8 *pCells, unsigned nCells)
{
	if (nCyl >= DSK_MAX_TRACKS || nSide >= DSK_MAX_SIDES)
	{
		return;
	}

	TDskTrack &T = m_Track[nCyl][nSide];
	u8 nGap3 = T.bPresent ? T.nGap3 : 0x52;
	u8 nFiller = T.bPresent ? T.nFiller : 0xE5;

	for (int i = 0; i < DSK_MAX_SECTORS; i++)
	{
		delete [] T.Sector[i].pData;
	}
	memset (&T, 0, sizeof T);
	for (int i = 0; i < DSK_MAX_SECTORS; i++)
	{
		T.Sector[i].nWeakFrom = 256;
		T.Sector[i].nWeakLen = 0;
	}
	T.nGap3 = nGap3;
	T.nFiller = nFiller;

	CMFMReader R (pCells, nCells);
	static const u8 A1[3] = {0xA1, 0xA1, 0xA1};

	while (T.nSectors < DSK_MAX_SECTORS && R.FindSync (nCells))
	{
		u8 nMark = R.Byte ();
		if (nMark != 0xFE)
		{
			continue;
		}

		u8 Id[5] = {0xFE, R.Byte (), R.Byte (), R.Byte (), R.Byte ()};
		u16 nIdCRC = (R.Byte () << 8);
		nIdCRC |= R.Byte ();
		u16 nCalc = CRC (Id, 5, CRC (A1, 3, 0xFFFF));

		TDskSector &S = T.Sector[T.nSectors];
		S.C = Id[1]; S.H = Id[2]; S.R = Id[3]; S.N = Id[4];
		S.ST1 = nCalc != nIdCRC ? 0x20 : 0;
		S.ST2 = 0;
		S.pData = 0;
		S.nLength = 0;

		// el campo de datos va poco después (hueco 2 + sincronismo)
		unsigned nSave = R.Position ();
		if (R.FindSync (60 * 16))
		{
			u8 nDataMark = R.Byte ();
			if (nDataMark == 0xFB || nDataMark == 0xF8)
			{
				unsigned nLength = 128u << (S.N & 7);
				if (nLength > 8192)
				{
					nLength = 8192;
				}
				S.pData = new u8[nLength];
				if (S.pData != 0)
				{
					for (unsigned i = 0; i < nLength; i++)
					{
						S.pData[i] = R.Byte ();
					}
					S.nLength = nLength;

					u16 nDataCRC = (R.Byte () << 8);
					nDataCRC |= R.Byte ();
					u8 nM = nDataMark;
					u16 nCalcData = CRC (S.pData, nLength, CRC (&nM, 1, CRC (A1, 3, 0xFFFF)));
					if (nCalcData != nDataCRC)
					{
						S.ST1 |= 0x20;
						S.ST2 |= 0x20;
					}
				}
				if (nDataMark == 0xF8)
				{
					S.ST2 |= 0x40;
				}
			}
		}
		else
		{
			(void) nSave;
			S.ST1 |= 0x04;		// sin datos
		}

		T.nSectors++;
	}

	T.bPresent = T.nSectors > 0;
	if (T.bPresent && nCyl >= m_nTracks)
	{
		m_nTracks = nCyl + 1;
	}
	if (T.bPresent && nSide >= m_nSides)
	{
		m_nSides = nSide + 1;
	}
}

//------------------------------------------------------------------------------
// Grabación (formato extendido)

unsigned CDskImage::GetSaveSize (void) const
{
	unsigned nSize = 0x100;
	for (int t = 0; t < m_nTracks; t++)
	{
		for (int s = 0; s < m_nSides; s++)
		{
			const TDskTrack &T = m_Track[t][s];
			if (!T.bPresent)
			{
				continue;
			}
			unsigned nTrack = 0x100;
			for (int j = 0; j < T.nSectors; j++)
			{
				nTrack += T.Sector[j].nLength;
			}
			nSize += (nTrack + 0xFF) & ~0xFF;
		}
	}
	return nSize;
}

unsigned CDskImage::Save (u8 *pBuffer, unsigned nBufferSize) const
{
	unsigned nTotal = GetSaveSize ();
	if (nTotal > nBufferSize)
	{
		return 0;
	}

	memset (pBuffer, 0, nTotal);
	memcpy (pBuffer, ExtHeader, 34);
	memcpy (pBuffer + 0x22, "ZXMetalCircle", 13);
	pBuffer[0x30] = m_nTracks;
	pBuffer[0x31] = m_nSides;

	unsigned nPos = 0x100;
	for (int t = 0; t < m_nTracks; t++)
	{
		for (int s = 0; s < m_nSides; s++)
		{
			const TDskTrack &T = m_Track[t][s];
			if (!T.bPresent)
			{
				continue;		// tamaño 0 en la tabla: pista sin formato
			}

			u8 *pTrack = pBuffer + nPos;
			memcpy (pTrack, "Track-Info\r\n", 12);
			pTrack[0x10] = t;
			pTrack[0x11] = s;
			pTrack[0x14] = T.nSectors > 0 ? T.Sector[0].N : 2;
			pTrack[0x15] = T.nSectors;
			pTrack[0x16] = T.nGap3;
			pTrack[0x17] = T.nFiller;

			unsigned nData = 0x100;
			for (int j = 0; j < T.nSectors; j++)
			{
				const TDskSector &S = T.Sector[j];
				u8 *pInfo = pTrack + 0x18 + j * 8;
				pInfo[0] = S.C; pInfo[1] = S.H; pInfo[2] = S.R; pInfo[3] = S.N;
				pInfo[4] = S.ST1; pInfo[5] = S.ST2;
				pInfo[6] = S.nLength & 0xFF;
				pInfo[7] = S.nLength >> 8;
				if (S.pData != 0)
				{
					memcpy (pTrack + nData, S.pData, S.nLength);
				}
				nData += S.nLength;
			}

			nData = (nData + 0xFF) & ~0xFF;
			pBuffer[0x34 + t * m_nSides + s] = nData >> 8;
			nPos += nData;
		}
	}

	return nPos;
}
