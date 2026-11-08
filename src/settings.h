//
// settings.h - Pantallas de configuración (F1 y luego C)
//
// Tabla de las opciones de cmdline.txt que se pueden cambiar desde el
// emulador, agrupadas en páginas. Cada una tiene su lista de valores; las
// marcadas "en vivo" se aplican al momento y las demás al reiniciar. "Guardar"
// escribe los valores en cmdline.txt, conservando el resto de opciones del
// fichero (hdd=, ip=, logdev=...) y dejando una copia en cmdline.bak.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_settings_h
#define _zx_settings_h

#include <circle/types.h>

struct TSettingDef
{
	const char *pKey;		// nombre en cmdline.txt
	const char *pLabel;		// texto en pantalla
	unsigned nPage;
	const char *pValues;		// valores en cmdline.txt, separados por |
	const char *pNames;		// lo que se ve de cada valor (mismo orden)
	const char *pDefault;
	boolean bLive;			// se aplica al momento (si no, al reiniciar)
};

#define SETTINGS_MAX		80
#define SETTING_NAME_LEN	32
#define SETTING_VALUE_LEN	96	// (rutas de hdd=)
#define SETTING_PAD_ID		16	// "045e:028e" o, el segundo igual, "045e:028e#2"
#define SETTING_MAX_PADS	4

class CSettings
{
public:
	CSettings (void);

	static unsigned GetPageCount (void);
	static const char *GetPageTitle (unsigned nPage);

	unsigned GetCount (void) const;
	const TSettingDef &GetDef (unsigned nIndex) const;
	int Find (const char *pKey) const;

	const char *GetValue (unsigned nIndex) const	{ return m_Value[nIndex]; }
	const char *GetValueName (unsigned nIndex) const;	// texto en pantalla del valor
	// FALSE si la opción no se usa con lo elegido ahora (por ejemplo, el Issue 2/3
	// en un +3): el menú la pinta en gris, pero se puede cambiar igual
	boolean IsApplicable (unsigned nIndex) const;
	void SetValue (unsigned nIndex, const char *pValue);
	void SetValue (const char *pKey, const char *pValue);
	void Step (unsigned nIndex, int nDirection);		// siguiente o anterior valor

	// Opciones que se escriben (IP): pValues = "*ip"; pNames = lo que se ve
	// cuando están vacías
	static boolean IsEditable (unsigned nIndex);
	static boolean IsPassword (unsigned nIndex);
	// Filas de acción (pValues = "*accion", clave con "_"): no tienen valores,
	// se ejecutan con ENTER y nunca se escriben en cmdline.txt. Lo que se ve
	// a la derecha lo pone el kernel con SetActionText.
	static boolean IsAction (unsigned nIndex);
	// Filas de mando (pValues = "*pad"): "auto" o el id (vid:pid) de un mando;
	// las flechas pasan por los conectados, que pone el kernel con SetPads
	static boolean IsPad (unsigned nIndex);

	// Lista de valores de una opción (ENTER en las filas de muchos valores):
	// cuántos hay, el nombre en pantalla de cada uno, si se puede elegir
	// (joysticks: no el del otro jugador), cuál está puesto y elegir uno
	static unsigned GetValueCount (unsigned nIndex);
	static boolean GetValueItemName (unsigned nIndex, unsigned nItem, char *pName, unsigned nSize);
	boolean IsValueAllowed (unsigned nIndex, unsigned nItem) const;
	int GetValueItem (unsigned nIndex) const;
	void ChooseValue (unsigned nIndex, unsigned nItem);
	void SetPads (unsigned nCount, const char Ids[][SETTING_PAD_ID], const char Names[][SETTING_NAME_LEN]);
	void SetActionText (const char *pKey, const char *pText);
	static boolean IsValidIP (const char *p);

	// Cambia la lista de valores de una opción (los idiomas, al arrancar)
	static void SetValueList (const char *pKey, const char *pValues, const char *pNames);

	boolean IsDirty (void) const			{ return m_bDirty; }
	void ClearDirty (void)				{ m_bDirty = FALSE; }

	// Línea nueva de cmdline.txt a partir de la que hay (pOld): cambia o
	// añade las opciones de la tabla y conserva todo lo demás.
	void Merge (const char *pOld, char *pOut, unsigned nOutSize) const;

private:
	char m_PadId[SETTING_MAX_PADS][SETTING_PAD_ID];	// mandos conectados
	char m_PadName[SETTING_MAX_PADS][SETTING_NAME_LEN];
	unsigned m_nPads;
	char m_Value[SETTINGS_MAX][SETTING_VALUE_LEN];
	boolean m_bDirty;
};

#endif
