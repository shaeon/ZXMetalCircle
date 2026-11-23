//
// filecopy.h
//
// Copiar un fichero de la SD a trozos (para no parar el emulador ni la web
// con uno grande, como una imagen de disco duro): se copia con un nombre
// temporal en la carpeta de destino y solo al terminar se le pone el suyo, así
// que si se cancela o falla no queda nada a medias.
//
#ifndef _zx_filecopy_h
#define _zx_filecopy_h

#include <circle/types.h>
#include <fatfs/ff.h>

class CFileCopy
{
public:
	CFileCopy (void);
	~CFileCopy (void);

	// pTemp: el nombre temporal (sin carpeta: va en la del destino).
	// bReplace: si pTo ya existe, se sustituye al terminar. Devuelve 0 o el
	// error (texto en español, para T ()).
	const char *Start (const char *pFrom, const char *pTo, const char *pTemp, boolean bReplace);

	// Copia hasta nMaxBytes: 1 terminado, 0 sigue, -1 error (GetError)
	int Step (unsigned nMaxBytes);

	void Cancel (void);		// cierra y borra el temporal

	boolean IsActive (void) const	{ return m_bActive; }
	u64 GetDone (void) const	{ return m_nDone; }
	u64 GetTotal (void) const	{ return m_nTotal; }
	const char *GetError (void) const	{ return m_pError; }

	// "nombre (2).ext", "(3)"... el primero que no existe en pDir; FALSE si no hay
	static boolean FreeName (const char *pDir, const char *pName, char *pOut, unsigned nOutSize);

private:
	void Close (void);

	boolean m_bActive;
	boolean m_bReplace;
	FIL m_From;
	FIL m_To;
	char m_TempPath[512];
	char m_FinalPath[512];
	FILINFO m_Info;			// la fecha del original, para la copia
	u64 m_nDone;
	u64 m_nTotal;
	const char *m_pError;
};

#endif
