//
// imagedev/floppy.h - Disquetera para el uPD765 de MAME en ZXMetalCircle
//
// Ofrece la misma interfaz que floppy_image_device de MAME (la parte que usan
// upd765.cpp y fdc_pll.cpp): motor, cabezal, pista 0, protección, pulsos de
// índice y, sobre todo, las transiciones de flujo de la pista bajo el cabezal
// en función del tiempo, que el PLL del controlador decodifica bit a bit.
//
// La pista se guarda como una cadena de celdas MFM de 2 us (1 = transición):
// 100.000 celdas por vuelta a 300 rpm. Se genera a partir de los sectores de un
// fichero .DSK (ver dskimage.h) y, si el Spectrum escribe, se vuelve a decodificar
// para guardar el .DSK.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef ZX_MAME_FLOPPY_H
#define ZX_MAME_FLOPPY_H

#include "emu.h"

#define FLOPPY_TRACKS		84		// cabezal: 0..83
#define FLOPPY_CELLS		100000		// celdas MFM de 2 us por vuelta (300 rpm)
#define FLOPPY_MAX_CELLS	131072		// pistas largas (protecciones)
#define FLOPPY_REV_PS		200000000000LL	// 200 ms por vuelta

class CDskImage;

class floppy_image_device
{
public:
	// Pulso de índice: llama a una función miembro del controlador
	class index_pulse_cb
	{
	public:
		index_pulse_cb () : m_obj (0), m_thunk (0) {}
		template <class C>
		index_pulse_cb (void (C::*func) (floppy_image_device *, int), C *obj) : m_obj (obj)
		{
			struct Holder { static void (C::*&F ()) (floppy_image_device *, int) { static void (C::*f) (floppy_image_device *, int); return f; } };
			Holder::F () = func;
			m_thunk = [] (void *o, floppy_image_device *d, int s) { (static_cast<C *> (o)->*Holder::F ()) (d, s); };
		}
		void operator() (floppy_image_device *d, int state) const { if (m_thunk) m_thunk (m_obj, d, state); }
	private:
		void *m_obj;
		void (*m_thunk) (void *, floppy_image_device *, int);
	};

	floppy_image_device (int nUnit, int nTracks = 40, bool bDoubleSided = false);
	~floppy_image_device ();

	// Disco (no se toma posesión de la imagen)
	void insert (CDskImage *pImage);
	void eject ();
	bool has_disk () const		{ return m_pImage != 0; }
	CDskImage *image ()		{ return m_pImage; }

	// Líneas de la disquetera (como en MAME; las activas en bajo llevan _w/_r)
	void setup_index_pulse_cb (index_pulse_cb cb)	{ m_IndexCB = cb; }
	void mon_w (int state);			// 0 = motor encendido
	int mon_r () const		{ return m_bMotorOn ? 0 : 1; }
	void dir_w (int state)		{ m_nDir = state; }
	void stp_w (int state);			// flanco de subida: un paso
	void ss_w (int state)		{ m_nSide = m_bDoubleSided ? state : 0; m_bLastValid = false; m_bLastNever = false; }
	void ds_w (int) {}
	int ready_r () const;			// 0 = lista
	int trk00_r () const		{ return m_nCyl == 0 ? 0 : 1; }
	int wpt_r () const;			// 1 = protegido contra escritura
	int twosid_r () const		{ return m_bDoubleSided ? 0 : 1; }

	// Tipo de unidad: 3" de una cara y ~40 pistas (la del +3) o 3,5" de doble
	// cara y 80 pistas (la que se ponía para los discos de 720K). Se elige al
	// meter el disco, según su geometría (CPlus3Disk).
	void set_geometry (int nTracks, bool bDoubleSided)
	{
		m_nMaxTracks = nTracks;
		m_bDoubleSided = bDoubleSided;
		if (!bDoubleSided) m_nSide = 0;
		if (m_nCyl >= nTracks) m_nCyl = nTracks - 1;
		m_bLastValid = false;
		m_bLastNever = false;
	}
	bool is_double_sided (void) const	{ return m_bDoubleSided; }
	int dskchg_r () const		{ return m_pImage != 0 ? 1 : 0; }

	// Flujo magnético
	attotime time_next_index ();
	attotime get_next_transition (const attotime &from_when);
	void write_start (const attotime &tm);
	void write_flux_change (const attotime &tm);
	void write_flush (const attotime &tm);
	void write_end (const attotime &tm);

	// Actualiza el pulso de índice (lo llama su propio temporizador)
	void update (const attotime &now);

	int get_cyl () const		{ return m_nCyl; }
	bool is_modified () const	{ return m_bModified; }
	void clear_modified ()		{ m_bModified = false; }

	// Pasa las pistas modificadas a la imagen .DSK (decodificando el MFM)
	void sync_image ();

private:
	const u8 *track_cells ();		// pista bajo el cabezal (generándola si hace falta)
	u8 *track_cells_writable ();
	unsigned cell_count () const;		// celdas de la pista bajo el cabezal
	s64 cell_centre (s64 nCell, unsigned nCount) const	// ps desde el inicio de la vuelta
		{ return (2 * nCell + 1) * FLOPPY_REV_PS / (2 * (s64) nCount); }
	s64 rev_position (const attotime &t) const;	// ps dentro de la vuelta actual
	void flush_writes (const attotime &upto);
	void schedule_index ();
	static void index_thunk (void *pThis, s32 param);

	int m_nUnit;
	int m_nMaxTracks;
	bool m_bDoubleSided;
	CDskImage *m_pImage;

	bool m_bMotorOn;
	attotime m_RevStart;		// comienzo de la vuelta en curso
	int m_nCyl, m_nDir, m_nSide, m_nLastStep;
	bool m_bIndex;
	bool m_bReady;			// tras dos pulsos de índice con el motor en marcha
	int m_nReadyCounter;

	index_pulse_cb m_IndexCB;

	// Pistas en celdas: 1 bit por celda, generadas bajo demanda
	u8 *m_pCells[FLOPPY_TRACKS][2];
	struct TWeak { unsigned nCount; unsigned Start[29], End[29]; } m_Weak[FLOPPY_TRACKS][2];
	bool is_weak (unsigned nCell, unsigned *pEnd) const;
	bool m_bTrackDirty[FLOPPY_TRACKS][2];
	unsigned m_nCells[FLOPPY_TRACKS][2];

	// caché de get_next_transition ()
	const u8 *m_pCacheCells;
	unsigned m_nCacheCount;
	u64 m_nCellFP;			// periodo de celda en ps << 16
	s64 m_nCacheRevBase;		// ps absolutos del comienzo de la vuelta
	s64 m_nCacheCell;
	bool m_bLastValid;		// última respuesta (válida mientras no cambie nada)
	bool m_bLastNever;		// la pista no tiene flujo (sin formatear): no buscar más
	const u8 *m_pNeverCells;
	s64 m_nLastFrom, m_nLastResult;
	bool m_bModified;
	emu_timer *m_pIndexTimer;

	// Escritura en curso
	bool m_bWriting;
	attotime m_WriteStart;
	attotime *m_pWriteFlux;
	unsigned m_nWriteFlux, m_nWriteMax;
};

#endif
