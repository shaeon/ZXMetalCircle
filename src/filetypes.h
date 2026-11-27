//
// filetypes.h - Qué ficheros sabe cargar la máquina y qué es cada uno
//
// Fase 3b de la capa de máquinas: antes había una lista de extensiones en el
// kernel, otra en el navegador, dos en la web y otra para los ZIP, cada una
// con lo suyo. Ahora cada máquina da su lista (CMachine::GetFileTypes) con el
// tipo de cada extensión, el kernel la pone al arrancar (SetFileTypes) y cada
// sitio pregunta por los tipos que le tocan:
//
//   navegador (F3/Alt+F5)	todo lo que se puede cargar
//   web			todo menos los pokes (que van con el juego cargado)
//   dentro de un ZIP		lo que se carga desde memoria o desde SD:/tmp: ni
//				discos duros, ni otros ZIP, ni pokes
//   pantalla de unidades	solo los de esa unidad
//
// Los ZIP los entiende el kernel con cualquier máquina.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _zx_filetypes_h
#define _zx_filetypes_h

#include <circle/types.h>

enum TFileKind
{
	FileUnknown,
	FileTape,		// cinta
	FileSnapshot,		// estado de la máquina
	FileCartridge,		// cartucho (ROM)
	FileMicrodrive,		// cartucho de Microdrive
	FileDisk,		// disquete
	FileHardDisk,		// disco duro (imagen)
	FileProgramZX81,	// programa del ZX81 (.P)
	FilePokes,		// pokes (.POK)
	FileZip			// ZIP (lo entiende el kernel)
};

struct TFileType
{
	const char *pExt;	// con el punto; 0 cierra la lista
	TFileKind Kind;
};

// La lista de la máquina que se emula (la da CMachine::GetFileTypes)
void SetFileTypes (const TFileType *pTypes);

// El tipo de un fichero por su extensión (FileUnknown si no es de la máquina)
TFileKind GetFileKind (const char *pName);

boolean IsBrowserFile (const char *pName);	// navegador: cualquiera que se cargue
boolean IsWebLoadable (const char *pName);	// web: todo menos los pokes
// Dentro de un ZIP. *pWritable: el programa puede escribirlo (disquetes y
// cartuchos de Microdrive: se sacan a SD:/tmp)
boolean IsZipLoadable (const char *pName, boolean *pWritable = 0);

#endif
