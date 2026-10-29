//
// plus3disk.h - Disquetera del Spectrum +3: uPD765A de MAME, dos unidades y .DSK
//
// Esta clase aísla el código de MAME (tipos y cabeceras propios) del resto del
// emulador. Conexión como en el driver specpls3 de MAME: uPD765A a 4 MHz con
// READY conectada y sin líneas de selección; la unidad la elige la salida US0
// del controlador; el motor de las dos unidades, el bit 3 del puerto 0x1FFD.
// Puertos: 0x2FFD (estado, lectura) y 0x3FFD (datos, lectura y escritura).
//
// El tiempo del controlador avanza con los t-states del Z80: en cada acceso a
// sus puertos y al final de cada cuadro.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_plus3disk_h
#define _zx_plus3disk_h

#include <circle/types.h>

class upd765a_device;
class floppy_image_device;
class CDskImage;

class CPlus3Disk
{
public:
	CPlus3Disk (void);
	~CPlus3Disk (void);

	void Reset (void);

	// nTstates: t-states absolutos; nClock: reloj del Z80 en Hz
	u8 ReadStatus (u64 nTstates, unsigned nClock);
	u8 ReadData (u64 nTstates, unsigned nClock);
	void WriteData (u8 nValue, u64 nTstates, unsigned nClock);
	void SetMotor (boolean bOn, u64 nTstates, unsigned nClock);
	void Sync (u64 nTstates, unsigned nClock);	// al final de cada cuadro

	// Discos: unidad 0 = A:, 1 = B:
	boolean InsertDSK (unsigned nUnit, const u8 *pData, unsigned nSize);
	boolean InsertBlank (unsigned nUnit, boolean b720K = FALSE);	// FALSE: 3" 180K; TRUE: 3,5" 720K
	boolean Is35 (unsigned nUnit) const;	// la unidad es de 3,5" (doble cara, 80 pistas)
	void Eject (unsigned nUnit);
	boolean HasDisk (unsigned nUnit) const;
	boolean IsModified (unsigned nUnit);
	void ClearModified (unsigned nUnit);
	unsigned GetSaveSize (unsigned nUnit);
	unsigned Save (unsigned nUnit, u8 *pBuffer, unsigned nSize);	// .DSK extendido

	boolean IsMotorOn (void) const	{ return m_bMotor; }

private:
	void KeepSpinning (u64 nTstates, unsigned nClock);	// sin motor, sigue girando si se usa
	void SetDriveType (unsigned nUnit, const CDskImage *pDisk);
	void AdvanceTo (u64 nTstates, unsigned nClock);
	static void SelectUnit (void *pThis, u8 nData);

	upd765a_device *m_pFDC;
	floppy_image_device *m_pDrive[2];
	CDskImage *m_pDisk[2];
	boolean m_bMotor;
	u64 m_nMotorOffAt;		// t-states en que se para el disco (0 = no hay parada pendiente)
};

#endif
