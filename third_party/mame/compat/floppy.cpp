//
// floppy.cpp - Disquetera para el uPD765 de MAME en ZXMetalCircle
//
// Comportamiento tomado de floppy.cpp de MAME (license:BSD-3-Clause, Olivier
// Galibert y otros): el paso del cabezal ocurre en el flanco de bajada de STEP,
// READY (activa en bajo) llega tras dos pulsos de índice con el motor en marcha,
// y el índice dura los primeros 2 ms de cada vuelta.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "imagedev/floppy.h"
#include "../dskimage.h"

#define INDEX_PULSE_PS		2000000000LL		// 2 ms
#define MAX_WRITE_FLUX		(FLOPPY_MAX_CELLS)

floppy_image_device::floppy_image_device (int nUnit, int nTracks, bool bDoubleSided)
:	m_nUnit (nUnit),
	m_nMaxTracks (nTracks),
	m_bDoubleSided (bDoubleSided),
	m_pImage (0),
	m_bMotorOn (false),
	m_nCyl (0),
	m_nDir (0),
	m_nSide (0),
	m_nLastStep (1),
	m_bIndex (false),
	m_bModified (false),
	m_pIndexTimer (0),
	m_bWriting (false),
	m_nWriteFlux (0)
{
	memset (m_pCells, 0, sizeof m_pCells);
	memset (m_bTrackDirty, 0, sizeof m_bTrackDirty);
	memset (m_Weak, 0, sizeof m_Weak);
	m_pWriteFlux = new attotime[MAX_WRITE_FLUX];
	m_nWriteMax = m_pWriteFlux != 0 ? MAX_WRITE_FLUX : 0;
	m_nReadyCounter = 0;
	m_bReady = false;
	m_pIndexTimer = new emu_timer (this, &floppy_image_device::index_thunk);
	m_pCacheCells = 0;
	m_nCacheCount = 0;
	m_nCellFP = 0;
	m_nCacheRevBase = -1;
	m_nCacheCell = 0;
	m_bLastValid = false; m_bLastNever = false;
}

void floppy_image_device::index_thunk (void *pThis, s32)
{
	floppy_image_device *p = static_cast<floppy_image_device *> (pThis);
	p->update (mame_scheduler::now ());
	p->schedule_index ();
}

// Programa el siguiente flanco del pulso de índice
void floppy_image_device::schedule_index ()
{
	if (!m_bMotorOn || m_pImage == 0)
	{
		m_pIndexTimer->enable (false);
		return;
	}

	attotime now = mame_scheduler::now ();
	s64 nPos = rev_position (now);
	s64 nNext = nPos < INDEX_PULSE_PS ? INDEX_PULSE_PS - nPos : FLOPPY_REV_PS - nPos;
	m_pIndexTimer->adjust (attotime::from_ps (nNext));
}

floppy_image_device::~floppy_image_device ()
{
	eject ();
	delete [] m_pWriteFlux;
}

void floppy_image_device::insert (CDskImage *pImage)
{
	eject ();
	m_pImage = pImage;
	m_bReady = false;
	m_nReadyCounter = 2;
	if (m_bMotorOn)
	{
		m_RevStart = mame_scheduler::now ();
		m_nCacheRevBase = -1;
		m_bLastValid = false; m_bLastNever = false;
		m_bIndex = false;
		update (m_RevStart);
		schedule_index ();
	}
}

void floppy_image_device::eject ()
{
	if (m_pImage != 0)
	{
		sync_image ();
	}

	for (int t = 0; t < FLOPPY_TRACKS; t++)
	{
		for (int s = 0; s < 2; s++)
		{
			delete [] m_pCells[t][s];
			m_pCells[t][s] = 0;
			m_bTrackDirty[t][s] = false;
			m_nCells[t][s] = 0;
			m_Weak[t][s].nCount = 0;
		}
	}

	m_pImage = 0;
	m_bReady = false;
	m_bWriting = false;
	if (m_pIndexTimer != 0)
	{
		m_pIndexTimer->enable (false);
	}
}

void floppy_image_device::mon_w (int state)
{
	bool bOn = state == 0;
	if (bOn == m_bMotorOn)
	{
		return;
	}

	m_bMotorOn = bOn;
	if (bOn)
	{
		m_RevStart = mame_scheduler::now ();
		m_nCacheRevBase = -1;
		m_bLastValid = false; m_bLastNever = false;
		m_nReadyCounter = 2;
		m_bIndex = false;
		update (m_RevStart);
		schedule_index ();
	}
	else
	{
		if (m_bWriting)
		{
			write_end (mame_scheduler::now ());
		}
		m_bReady = false;
		m_pIndexTimer->enable (false);
		if (m_bIndex)
		{
			m_bIndex = false;
			m_IndexCB (this, 0);
		}
		sync_image ();
	}
}

void floppy_image_device::stp_w (int state)
{
	if (state == m_nLastStep)
	{
		return;
	}

	m_nLastStep = state;
	m_bLastValid = false; m_bLastNever = false;
	if (state == 0)
	{
		if (m_nDir)
		{
			if (m_nCyl > 0) m_nCyl--;
		}
		else
		{
			if (m_nCyl < m_nMaxTracks - 1) m_nCyl++;
		}
	}
}

int floppy_image_device::ready_r () const
{
	return (m_pImage != 0 && m_bMotorOn && m_bReady) ? 0 : 1;
}

int floppy_image_device::wpt_r () const
{
	return (m_pImage == 0 || m_pImage->IsWriteProtected ()) ? 1 : 0;
}

s64 floppy_image_device::rev_position (const attotime &t) const
{
	s64 nDelta = t.as_ps () - m_RevStart.as_ps ();
	if (nDelta < 0)
	{
		nDelta = 0;
	}
	return nDelta % FLOPPY_REV_PS;
}

attotime floppy_image_device::time_next_index ()
{
	if (!m_bMotorOn || m_pImage == 0)
	{
		return attotime::never;
	}

	attotime now = mame_scheduler::now ();
	s64 nPos = rev_position (now);
	return now + attotime::from_ps (FLOPPY_REV_PS - nPos);
}

void floppy_image_device::update (const attotime &now)
{
	bool bIndex = false;
	if (m_bMotorOn && m_pImage != 0)
	{
		bIndex = rev_position (now) < INDEX_PULSE_PS;
	}

	if (bIndex != m_bIndex)
	{
		m_bIndex = bIndex;
		if (bIndex && !m_bReady && m_nReadyCounter > 0 && --m_nReadyCounter == 0)
		{
			m_bReady = true;
		}
		m_IndexCB (this, bIndex ? 1 : 0);
	}
}

u8 *floppy_image_device::track_cells_writable ()
{
	int nSide = m_bDoubleSided ? m_nSide : 0;
	if (m_nCyl >= FLOPPY_TRACKS)
	{
		return 0;
	}

	u8 *&pCells = m_pCells[m_nCyl][nSide];
	if (pCells == 0 && m_pImage != 0)
	{
		unsigned nCount = m_pImage->GetTrackCells (m_nCyl, nSide);
		if (nCount > FLOPPY_MAX_CELLS)
		{
			nCount = FLOPPY_MAX_CELLS;
		}
		pCells = new u8[(nCount + 7) / 8];
		if (pCells != 0)
		{
			m_nCells[m_nCyl][nSide] = nCount;
			TWeak &W = m_Weak[m_nCyl][nSide];
			m_pImage->BuildTrack (m_nCyl, nSide, pCells, nCount, W.Start, W.End, &W.nCount);
		}
	}
	return pCells;
}

unsigned floppy_image_device::cell_count () const
{
	unsigned n = m_nCells[m_nCyl][m_bDoubleSided ? m_nSide : 0];
	return n != 0 ? n : FLOPPY_CELLS;
}

const u8 *floppy_image_device::track_cells ()
{
	return track_cells_writable ();
}

// Siguiente transición de flujo a partir de from_when. Es la función más usada
// (una vez por bit leído): se evitan divisiones de 64 bits recordando la vuelta
// y la celda de la llamada anterior, y se saltan de golpe los bytes sin flujo.
attotime floppy_image_device::get_next_transition (const attotime &from_when)
{
	if (!m_bMotorOn || m_pImage == 0 || from_when.is_never ())
	{
		return attotime::never;
	}

	const u8 *pCells = track_cells ();
	if (pCells == 0)
	{
		return attotime::never;
	}

	// Pista sin formatear (sin ninguna transición): ya se sabe la respuesta.
	// Sin esto, el PLL la recorría entera en cada bit (unas 12500 vueltas de
	// bucle cada 2 µs) y la emulación iba decenas de veces más lenta mientras
	// un cargador leía una pista vacía (Ghouls'n Ghosts, pistas 18 a 20).
	if (m_bLastNever && pCells == m_pNeverCells)
	{
		return attotime::never;
	}

	// El PLL pregunta en cada bit: mientras no se pase la última transición
	// devuelta, la respuesta es la misma
	s64 nWhenPs = from_when.as_ps ();
	if (   m_bLastValid && pCells == m_pCacheCells
	    && nWhenPs >= m_nLastFrom && nWhenPs < m_nLastResult)
	{
		return attotime::from_ps (m_nLastResult);
	}

	int nSide = m_bDoubleSided ? m_nSide : 0;
	unsigned nCount = cell_count ();
	const TWeak &W = m_Weak[m_nCyl][nSide];
	s64 nWhen = from_when.as_ps ();

	// periodo de celda en ps con 16 bits de fracción (pistas de longitud variable)
	if (pCells != m_pCacheCells || nCount != m_nCacheCount)
	{
		m_pCacheCells = pCells;
		m_nCacheCount = nCount;
		m_nCellFP = (u64) (FLOPPY_REV_PS << 16) / nCount;
		m_nCacheRevBase = -1;
		m_bLastValid = false; m_bLastNever = false;
	}

	// comienzo de la vuelta que contiene nWhen (casi siempre la misma de antes)
	if (   m_nCacheRevBase < 0 || nWhen < m_nCacheRevBase
	    || nWhen >= m_nCacheRevBase + FLOPPY_REV_PS)
	{
		m_nCacheRevBase = nWhen - rev_position (from_when);
		m_nCacheCell = 0;
	}
	s64 nPos = nWhen - m_nCacheRevBase;

	// primera celda cuyo centro es posterior a nPos: se parte de la anterior
	// si sirve; si no, se calcula
	s64 nCell = m_nCacheCell;
	if (nCell <= 0 || (s64) (((2 * nCell - 1) * m_nCellFP) >> 17) > nPos)
	{
		nCell = (s64) (((u64) nPos << 16) / m_nCellFP);
	}
	while ((s64) (((2 * nCell + 1) * m_nCellFP) >> 17) <= nPos)
	{
		nCell++;
	}

	u32 nRev = (u32) ((nWhen - m_RevStart.as_ps ()) / FLOPPY_REV_PS);

	for (unsigned n = 0; n < nCount + 1; n++, nCell++)
	{
		unsigned c = (unsigned) (nCell < nCount ? nCell : nCell % nCount);

		// salto rápido de bytes sin flujo (fuera de zonas débiles)
		if (W.nCount == 0 && (c & 7) == 0 && pCells[c >> 3] == 0 && c + 8 <= nCount)
		{
			nCell += 7;
			n += 7;
			continue;
		}

		bool bFlux;
		unsigned nWeakEnd;
		if (W.nCount != 0 && is_weak (c, &nWeakEnd))
		{
			// superficie sin flujo definido: transiciones al azar (distintas en
			// cada vuelta), como las zonas MG_N de MAME
			u32 h = (c * 2654435761u) ^ ((nRev + (u32) (nCell / nCount)) * 40503u) ^ 0x9E3779B9u;
			h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
			bFlux = (h & 3) == 0;
		}
		else
		{
			bFlux = (pCells[c >> 3] & (0x80 >> (c & 7))) != 0;
		}

		if (bFlux)
		{
			m_nCacheCell = nCell + 1;
			s64 nTurn = nCell / nCount;		// 0 casi siempre
			m_nLastFrom = nWhenPs;
			m_nLastResult = m_nCacheRevBase + nTurn * FLOPPY_REV_PS
					+ (s64) (((2 * (u64) c + 1) * m_nCellFP) >> 17);
			m_bLastValid = true;
			return attotime::from_ps (m_nLastResult);
		}
	}

	// pista sin flujo (sin formatear); con zonas débiles hay transiciones al
	// azar, así que solo se recuerda si no las hay
	if (W.nCount == 0)
	{
		m_bLastNever = true;
		m_pNeverCells = pCells;
	}
	return attotime::never;
}

bool floppy_image_device::is_weak (unsigned nCell, unsigned *pEnd) const
{
	int nSide = m_bDoubleSided ? m_nSide : 0;
	const TWeak &W = m_Weak[m_nCyl][nSide];
	for (unsigned i = 0; i < W.nCount; i++)
	{
		if (nCell >= W.Start[i] && nCell < W.End[i])
		{
			*pEnd = W.End[i];
			return true;
		}
	}
	return false;
}

void floppy_image_device::write_start (const attotime &tm)
{
	m_bWriting = true;
	m_WriteStart = tm;
	m_nWriteFlux = 0;
}

void floppy_image_device::write_flux_change (const attotime &tm)
{
	if (m_bWriting && m_nWriteFlux < m_nWriteMax)
	{
		m_pWriteFlux[m_nWriteFlux++] = tm;
	}
}

// Pasa a la pista las transiciones escritas desde la última vez hasta "upto":
// las celdas de ese intervalo se borran y se marcan las transiciones nuevas
void floppy_image_device::flush_writes (const attotime &upto)
{
	if (!m_bWriting || m_pImage == 0 || m_pImage->IsWriteProtected () || !m_bMotorOn)
	{
		m_nWriteFlux = 0;
		m_WriteStart = upto;
		return;
	}

	u8 *pCells = track_cells_writable ();
	if (pCells == 0 || upto <= m_WriteStart)
	{
		return;
	}

	s64 nStart = m_WriteStart.as_ps ();
	s64 nEnd = upto.as_ps ();
	s64 nRevBase = nStart - rev_position (m_WriteStart);

	// borra las celdas cuyo centro cae en el intervalo (las transiciones van en
	// el centro de la celda: así un tramo no borra la del tramo anterior)
	unsigned nCount = cell_count ();
	s64 nFirst = (nStart - nRevBase) * nCount / FLOPPY_REV_PS;
	if (cell_centre (nFirst, nCount) < nStart - nRevBase)
	{
		nFirst++;
	}
	s64 nLast = (nEnd - nRevBase) * nCount / FLOPPY_REV_PS;
	if (cell_centre (nLast, nCount) < nEnd - nRevBase)
	{
		nLast++;
	}
	for (s64 n = nFirst; n < nLast; n++)
	{
		unsigned c = (unsigned) (n % nCount);
		pCells[c >> 3] &= ~(0x80 >> (c & 7));
	}

	// las transiciones posteriores a "upto" se guardan para el siguiente tramo
	// (el PLL puede anotar una justo en el límite)
	unsigned nKept = 0;
	for (unsigned i = 0; i < m_nWriteFlux; i++)
	{
		s64 t = m_pWriteFlux[i].as_ps ();
		if (t >= nEnd)
		{
			m_pWriteFlux[nKept++] = m_pWriteFlux[i];
			continue;
		}
		if (t < nStart)
		{
			continue;
		}
		unsigned c = (unsigned) (((t - nRevBase) * nCount / FLOPPY_REV_PS) % nCount);
		pCells[c >> 3] |= 0x80 >> (c & 7);
	}

	m_nWriteFlux = nKept;
	m_WriteStart = upto;
	m_bLastValid = false; m_bLastNever = false;
	m_Weak[m_nCyl][m_bDoubleSided ? m_nSide : 0].nCount = 0;	// lo escrito ya no es débil
	m_bTrackDirty[m_nCyl][m_bDoubleSided ? m_nSide : 0] = true;
	m_bModified = true;
}

void floppy_image_device::write_flush (const attotime &tm)
{
	flush_writes (tm);
}

void floppy_image_device::write_end (const attotime &tm)
{
	flush_writes (tm);
	m_nWriteFlux = 0;
	m_bWriting = false;
}

void floppy_image_device::sync_image ()
{
	if (m_pImage == 0)
	{
		return;
	}

	for (int t = 0; t < FLOPPY_TRACKS; t++)
	{
		for (int s = 0; s < 2; s++)
		{
			if (m_bTrackDirty[t][s] && m_pCells[t][s] != 0)
			{
				m_pImage->DecodeTrack (t, s, m_pCells[t][s], m_nCells[t][s]);
				m_bTrackDirty[t][s] = false;
			}
		}
	}
}
