//
// plus3disk.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#include "plus3disk.h"
#include "emu.h"
#include "upd765.h"
#include "imagedev/floppy.h"
#include "dskimage.h"

CPlus3Disk::CPlus3Disk (void)
:	m_bMotor (FALSE),
	m_nMotorOffAt (0),
	m_nLastAccess (0),
	m_nMotorOnAt (0)
{
	static machine_config Config;

	// uPD765A a 4 MHz (16 MHz / 4), READY conectada, sin líneas de selección
	m_pFDC = new upd765a_device (Config, "upd765", 0, 4000000, true, false);
	m_pFDC->us_wr_callback ().set (&CPlus3Disk::SelectUnit, this);
	m_pFDC->start ();

	m_pDrive[0] = new floppy_image_device (0, 42, false);	// interna: 3" una cara
	m_pDrive[1] = new floppy_image_device (1, 42, false);	// externa
	m_pDisk[0] = m_pDisk[1] = 0;

	m_pFDC->set_floppy (m_pDrive[0]);
}

CPlus3Disk::~CPlus3Disk (void)
{
	Eject (0);
	Eject (1);
	delete m_pDrive[0];
	delete m_pDrive[1];
	delete m_pFDC;
}

void CPlus3Disk::SelectUnit (void *pThis, u8 nData)
{
	CPlus3Disk *p = static_cast<CPlus3Disk *> (pThis);
	p->m_pFDC->set_floppy (p->m_pDrive[nData & 1]);
}

void CPlus3Disk::AdvanceTo (u64 nTstates, unsigned nClock)
{
	// el disco termina de pararse (ver SetMotor)
	if (m_nMotorOffAt != 0 && nTstates >= m_nMotorOffAt)
	{
		u64 nOff = m_nMotorOffAt;
		m_nMotorOffAt = 0;
		AdvanceTo (nOff, nClock);
		m_pDrive[0]->mon_w (1);
		m_pDrive[1]->mon_w (1);
	}

	// ps = t-states * 1e12 / reloj, sin desbordar 64 bits
	u64 nPs = (nTstates / nClock) * 1000000000000ULL
		  + (nTstates % nClock) * 1000000000000ULL / nClock;
	attotime t = attotime::from_ps ((s64) nPs);
	if (t > mame_scheduler::now ())
	{
		mame_scheduler::advance_to (t);
	}
}

void CPlus3Disk::Reset (void)
{
	m_pFDC->reset ();
	m_pDrive[0]->mon_w (1);
	m_pDrive[1]->mon_w (1);
	m_bMotor = FALSE;
	m_nMotorOffAt = 0;
}

u8 CPlus3Disk::ReadStatus (u64 nTstates, unsigned nClock)
{
	AdvanceTo (nTstates, nClock);
	KeepSpinning (nTstates, nClock);
	return m_pFDC->msr_r ();
}

u8 CPlus3Disk::ReadData (u64 nTstates, unsigned nClock)
{
	AdvanceTo (nTstates, nClock);
	KeepSpinning (nTstates, nClock);
	return m_pFDC->fifo_r ();
}

void CPlus3Disk::WriteData (u8 nValue, u64 nTstates, unsigned nClock)
{
	AdvanceTo (nTstates, nClock);
	KeepSpinning (nTstates, nClock);
	m_pFDC->fifo_w (nValue);
}

// Al apagar el motor, el disco no se para en seco: sigue girando y la unidad
// sigue preparada (READY) un momento, como mínimo 1,5 vueltas (300 ms, como
// Fuse en fdd.c), y además mientras se siga usando el controlador: se para a
// los 2 s del último acceso. Hay cargadores que cuentan con ello:
// - El de The Deep (U.S. Gold) apaga el motor al cambiar de página con
//   0x1FFD y enseguida pide a +3DOS un sector.
// - El de The Running Man (Grandslam, también de Emerald Software) apaga el
//   motor del mismo modo (escribe 07 y 04 en 0x1FFD) y carga todo el juego
//   sin volver a encenderlo; con solo 300 ms, la primera lectura se quedaba
//   esperando para siempre con la pantalla en negro.
// +3DOS apaga el motor cuando ya ha terminado, así que con él no cambia nada.
// Si se vuelve a encender antes de pararse, sigue girando sin esperar.
#define MOTOR_SPINDOWN_MS	300
#define MOTOR_IDLE_STOP_MS	2000

void CPlus3Disk::KeepSpinning (u64 nTstates, unsigned nClock)
{
	m_nLastAccess = nTstates;		// para RecentlyUsed (disco acelerado)
	if (m_nMotorOffAt == 0)
	{
		return;				// el motor está encendido (o ya parado)
	}
	u64 nStop = nTstates + (u64) nClock * MOTOR_IDLE_STOP_MS / 1000;
	if (nStop > m_nMotorOffAt)
	{
		m_nMotorOffAt = nStop;
	}
}

void CPlus3Disk::SetMotor (boolean bOn, u64 nTstates, unsigned nClock)
{
	if (bOn && !m_bMotor)
	{
		m_nMotorOnAt = nTstates;	// para RecentlyUsed (disco acelerado)
	}
	AdvanceTo (nTstates, nClock);
	if (bOn == m_bMotor)
	{
		return;
	}
	m_bMotor = bOn;

	if (bOn)
	{
		if (m_nMotorOffAt != 0)
		{
			m_nMotorOffAt = 0;		// aún giraba: sigue igual
			return;
		}
		m_pDrive[0]->mon_w (0);
		m_pDrive[1]->mon_w (0);
	}
	else
	{
		m_nMotorOffAt = nTstates + (u64) nClock * MOTOR_SPINDOWN_MS / 1000;
		if (m_nMotorOffAt == 0)
		{
			m_nMotorOffAt = 1;
		}
	}
}

void CPlus3Disk::Sync (u64 nTstates, unsigned nClock)
{
	AdvanceTo (nTstates, nClock);
}

boolean CPlus3Disk::InsertDSK (unsigned nUnit, const u8 *pData, unsigned nSize)
{
	if (nUnit > 1)
	{
		return FALSE;
	}

	CDskImage *pDisk = new CDskImage;
	if (pDisk == 0 || !pDisk->Load (pData, nSize))
	{
		delete pDisk;
		return FALSE;
	}

	Eject (nUnit);
	m_pDisk[nUnit] = pDisk;
	SetDriveType (nUnit, pDisk);
	m_pDrive[nUnit]->insert (pDisk);
	return TRUE;
}

boolean CPlus3Disk::InsertBlank (unsigned nUnit, boolean b720K)
{
	if (nUnit > 1)
	{
		return FALSE;
	}

	CDskImage *pDisk = new CDskImage;
	if (pDisk == 0 || !(b720K ? pDisk->CreateBlank720 () : pDisk->CreateBlank3 ()))
	{
		delete pDisk;
		return FALSE;
	}

	Eject (nUnit);
	m_pDisk[nUnit] = pDisk;
	SetDriveType (nUnit, pDisk);
	m_pDrive[nUnit]->insert (pDisk);
	return TRUE;
}

// La unidad se adapta al disco: con más de 44 pistas o dos caras (720K) es una
// de 3,5" de 80 pistas y doble cara, como las que se ponían en el +3 para esos
// discos; si no, la de 3" de una cara del +3. +3DOS lo mira en ST3 (bit "dos
// caras") al leer la especificación del disco; antes los discos de 720K daban
// "disk unsuitable" porque las dos unidades eran siempre de 3".
void CPlus3Disk::SetDriveType (unsigned nUnit, const CDskImage *pDisk)
{
	boolean b35 = pDisk->GetSides () > 1 || pDisk->GetTracks () > 44;
	m_pDrive[nUnit]->set_geometry (b35 ? 84 : 42, b35);
}

boolean CPlus3Disk::Is35 (unsigned nUnit) const
{
	return nUnit <= 1 && m_pDrive[nUnit]->is_double_sided ();
}

void CPlus3Disk::Eject (unsigned nUnit)
{
	if (nUnit > 1 || m_pDisk[nUnit] == 0)
	{
		return;
	}

	m_pDrive[nUnit]->eject ();
	delete m_pDisk[nUnit];
	m_pDisk[nUnit] = 0;
}

boolean CPlus3Disk::HasDisk (unsigned nUnit) const
{
	return nUnit <= 1 && m_pDisk[nUnit] != 0;
}

boolean CPlus3Disk::IsModified (unsigned nUnit)
{
	return nUnit <= 1 && m_pDisk[nUnit] != 0 && m_pDrive[nUnit]->is_modified ();
}

void CPlus3Disk::ClearModified (unsigned nUnit)
{
	if (nUnit <= 1)
	{
		m_pDrive[nUnit]->clear_modified ();
	}
}

unsigned CPlus3Disk::GetSaveSize (unsigned nUnit)
{
	if (nUnit > 1 || m_pDisk[nUnit] == 0)
	{
		return 0;
	}
	m_pDrive[nUnit]->sync_image ();
	return m_pDisk[nUnit]->GetSaveSize ();
}

unsigned CPlus3Disk::Save (unsigned nUnit, u8 *pBuffer, unsigned nSize)
{
	if (nUnit > 1 || m_pDisk[nUnit] == 0)
	{
		return 0;
	}
	m_pDrive[nUnit]->sync_image ();
	return m_pDisk[nUnit]->Save (pBuffer, nSize);
}

// ¿Se está usando la disquetera? Para acelerar la emulación mientras tanto
// (diskaccel=1): accesos al controlador en los últimos 300 ms, o el motor
// encendido hace menos de 1,5 s (+3DOS espera con un bucle a que coja
// velocidad, sin tocar el controlador). Tiempo del Spectrum, no real.
boolean CPlus3Disk::RecentlyUsed (u64 nTstates, unsigned nClock) const
{
	if (m_nLastAccess != 0 && nTstates - m_nLastAccess < (u64) nClock * 3 / 10)
	{
		return TRUE;
	}
	return m_bMotor && nTstates - m_nMotorOnAt < (u64) nClock * 3 / 2;
}
