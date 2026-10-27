//
// i18n.h - Idiomas de ZXMetalCircle
//
// El español va dentro del programa: cada texto que se ve en pantalla se
// escribe en español y pasa por T () (o se marca con N_ en las tablas y se
// traduce con T () al usarlo). Las traducciones están en SD:/locales/xx.json:
// un objeto con el texto en español como clave y su traducción como valor
// ("" = sin traducir). Si falta una traducción, se ve el español.
//
// Las claves "_idioma" (nombre del idioma en ese idioma) y "_version" son de
// cabecera. tools/i18n/actualizar.py busca los textos marcados con T y N_ del
// código, rehace locales/es.json (de referencia) y añade a los demás los
// textos nuevos.
//
// Los textos van en UTF-8; la fuente de los menús tiene las letras de Europa
// occidental (Latin-1 y alguna más, ver video.cpp).
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_i18n_h
#define _zx_i18n_h

#include <circle/types.h>

#define I18N_MAX_LANGUAGES	16
#define I18N_FOLDER		"SD:/locales"

// Marca un texto para traducir sin traducirlo aún (tablas estáticas)
#define N_(text)		(text)

// Traduce un texto en español al idioma actual
const char *T (const char *pSpanish);

class CI18n
{
public:
	CI18n (void);
	~CI18n (void);

	static CI18n *Get (void)	{ return s_pThis; }

	// Busca los idiomas que hay en SD:/locales (el español siempre está)
	void ScanLanguages (void);
	unsigned GetLanguageCount (void) const		{ return m_nLanguages; }
	const char *GetLanguageCode (unsigned i) const	{ return m_Code[i]; }
	const char *GetLanguageName (unsigned i) const	{ return m_Name[i]; }

	// "es", "en"... (un código que no existe deja el español)
	boolean SetLanguage (const char *pCode);
	const char *GetLanguage (void) const		{ return m_Current; }

	const char *Translate (const char *pSpanish) const;

	// Textos traducidos y sin traducir del idioma actual (para el registro)
	unsigned GetTranslatedCount (void) const	{ return m_nEntries; }
	unsigned GetEmptyCount (void) const		{ return m_nEmpty; }

private:
	void Clear (void);
	boolean Load (const char *pPath);
	static u32 Hash (const char *p);

	struct TEntry
	{
		u32 nHash;
		const char *pKey;
		const char *pValue;
	};

	char *m_pText;			// claves y valores, ya sin escapes JSON
	TEntry *m_pEntries;		// ordenadas por hash
	unsigned m_nEntries;
	unsigned m_nEmpty;

	char m_Current[8];
	unsigned m_nLanguages;
	char m_Code[I18N_MAX_LANGUAGES][8];
	char m_Name[I18N_MAX_LANGUAGES][32];

	static CI18n *s_pThis;
};

// Nombre de fichero de la SD (página de códigos 850 de FatFs) en UTF-8, para
// verlo con sus acentos en los menús
void CP850ToUTF8 (const char *pIn, char *pOut, unsigned nSize);

// Número de caracteres (no de bytes) de un texto UTF-8
unsigned UTF8Length (const char *p);

#endif
