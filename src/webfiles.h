//
// webfiles.h
//
// Reglas de las operaciones con ficheros de la página web: nombres válidos,
// qué es del sistema (no se toca desde la web) y rutas dentro de otras.
// Sin dependencias de Circle (se prueba en el PC: test/host/webfiles_test.cpp).
//
// ZXMetalCircle, Shaeon (Carlos Palmero). GPLv3.
//
#ifndef _webfiles_h
#define _webfiles_h

// Las rutas son relativas a la raíz de la SD, con '/' y sin '/' al principio
// ("juegos/manic.tap"); "SD:/..." y "/..." también valen (se quitan).

// Un nombre de fichero o carpeta (sin ruta) que se puede usar
bool WebValidName (const char *pName);

// Del sistema: no se puede renombrar, mover, borrar ni sustituir. Los ficheros
// del arranque de la raíz, las carpetas que usa el emulador y, en algunas de
// ellas (overlays, firmware, certs, locales, update), todo lo que tienen dentro
bool WebIsSystem (const char *pPath);

// Una carpeta en la que no se puede poner nada (subir, mover o renombrar dentro)
bool WebIsSystemFolder (const char *pDir);

// pPath es pDir o está dentro de ella (sin distinguir mayúsculas)
bool WebPathIsUnder (const char *pPath, const char *pDir);

// Quita "SD:", "/" del principio y "/" del final; pOut debe tener nSize bytes
void WebNormalizePath (const char *pIn, char *pOut, unsigned nSize);

#endif
