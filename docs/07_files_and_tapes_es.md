# Ficheros y cintas

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

## Ficheros ZIP

Los `.zip` se cargan directamente desde el selector (F1) o la web, sin descomprimirlos antes:

- **Si el ZIP tiene un solo fichero que se pueda cargar**, se carga al elegir el ZIP. Si tiene varios (por ejemplo, las dos caras de un juego), sale una lista para elegir; los demás ficheros (textos, instrucciones) no aparecen.
- **Cintas, snapshots y cartuchos** (`.tap`, `.tzx`, `.pzx`, `.csw`, `.z80`, `.sna`, `.szx`, `.rom`) se cargan al vuelo, desde memoria, sin escribir nada en la tarjeta.
- **Discos y cartuchos de Microdrive** (`.dsk`, `.mdr`, `.mdv`), que el programa puede escribir, se sacan a la carpeta `tmp` de la tarjeta (se crea si no existe) y se cargan desde ahí, así que lo que se grabe queda en ese fichero. Si ya está en `tmp`, se pregunta si usar el que hay (con sus cambios) o sobrescribirlo con el del ZIP.
- Se admiten los ZIP normales (sin comprimir o con deflate, que son casi todos), de hasta 32 MB y 512 ficheros. No los cifrados.
- Si hay muchos ficheros para elegir, la lista se desplaza: cursores, RePág/AvPág, Inicio y Fin; arriba a la derecha se ve en cuál estás («23/100»).

## Unidades (F1 y luego U, o Alt+U)

Una pantalla con lo que hay metido en cada unidad: **cinta**, **disco A:** y **disco B:** del +3, **disco duro** (+3e o divMMC), **cartucho del Interface 2** y **microdrives 1 a 8**.

- **ENTER** abre el selector solo con los ficheros de esa unidad (cintas, `.dsk`, imágenes de disco duro, `.rom`, `.mdr`/`.mdv`) y lo que eliges va a esa unidad. Así se puede meter un disco en la unidad **B:** o un cartucho en un microdrive concreto. Al elegir, se vuelve a la pantalla de unidades; ESC en el selector también vuelve.
- **SUPR** (o Retroceso) vacía la unidad, grabando antes lo pendiente (discos, cartuchos, lo que haya escrito esxDOS).
- Lo que se monta aquí vale **solo para esta sesión**. El disco duro con el que arranca la Pi se elige en la configuración ("Disco duro al inicio", página 1), con las imágenes de la carpeta `hdd` (las que no tienen espacios en el nombre: `cmdline.txt` separa las opciones con espacios).

## Cartuchos del Interface 2

El Interface 2 de Sinclair tenía una ranura para cartuchos ROM de 16 KB que, al conectarse, sustituían a la ROM del Spectrum; el juego arrancaba al encender. Hubo una decena de juegos en este formato (Jetpac, Pssst, Cookie, Tranz Am, Chess…).

- Se cargan desde el selector (F1, ENTER), eligiendo un fichero `.ROM` de hasta 16 KB. Si ocupa menos, el resto se rellena con 0xFF.
- Al insertarlo se hace un reset y arranca el cartucho. **F11** lo quita y vuelve a arrancar con la ROM normal.
- Funciona en 16K, 48K, 128K y +2. El +2A no lo admite, igual que en JSpeccy.
- Los joysticks del Interface 2 son los modos Sinclair 1 y Sinclair 2.
- Cargar un snapshot retira el cartucho, salvo que sea un .SZX que lo incluya (bloque IF2R).

## Snapshots

Un snapshot es una "foto" del Spectrum entero: registros del Z80, memoria, paginación, borde y chip de sonido. Al cargarlo, el programa sigue exactamente donde estaba. Sirve para guardar la partida en cualquier momento o para cargar juegos al instante.

- **Cargar**: desde el selector (F1 y luego ENTER), igual que las cintas. Formatos:
  - .Z80, versiones 1, 2 y 3, comprimidos o no;
  - .SNA de 48K y de 128K;
  - .SZX, el formato de Spectaculator y Fuse, con páginas comprimidas o no.
- El emulador cambia solo al modelo del snapshot (16K, 48K, 128K, +2 o +2A). Un .SNA de 128K se carga en el +2 o el +2A si ese es el modelo actual, porque el formato no los distingue.
- **Grabar**: F8 guarda el estado en `snaps/<cinta>_001.z80`, `_002`, etc., con el nombre de la cinta actual o `snap` si no hay ninguna. El formato se elige con `snapformat=`:
  - **.Z80** v3 comprimido, el más compatible y el que se usa por defecto;
  - **.SNA**: en 48K, el PC va a la pila, como manda el formato; el +2A se guarda como 128K;
  - **.SZX**: el más completo (incluye la paleta ULAplus); las páginas van sin comprimir.

  Tanto .Z80 como .SZX guardan la posición exacta dentro del cuadro.
- Los snapshots de Pentagon 128 se cargan como 128K, lo más parecido que hay.
- No se guarda el estado del Multiface ni de la cinta. Graba el snapshot con el Multiface cerrado.
