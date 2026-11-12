# ZXMetalCircle

[*Read in English*](README.md)

Este README en español es la referencia. Historial de cambios: [`CHANGELOG.md`](CHANGELOG.md). **Guías de uso** (compilar, programar y grabar en cinta, pines GPIO, mandos USB…): carpeta [`docs`](docs/README.md).

(El proyecto se llamó al principio ZXCircle: los ficheros y el código conservan ese nombre.)

Emulador bare-metal de ZX Spectrum (del 16K al +3e) para Raspberry Pi, escrito en C++ sobre la librería [Circle](https://github.com/rsta2/circle) de Rene Stange.

No necesita Linux: la Pi arranca directamente en el emulador desde la tarjeta SD.

Autor: **Shaeon (Carlos Palmero)**, © 2026.

## Agradecimientos

- A **José Luis Sánchez**, por **JSpeccy** y **ZXBaremulator**, que inspiraron este proyecto, y por **z80cpp**, la CPU Z80 que usa.
- A **Rene Stange**, por la librería **Circle**, sobre la que está hecho todo el emulador.
- Al grupo de **RetroWiki** (https://www.retrowiki.es), por su ayuda y por mantener viva la comunidad.
- A **Jordi** (https://jbinary.itch.io), por sus consejos sobre el Spectrum.
- A **Víctor Iborra (EremusOne)**, por **ESPectrum** (https://github.com/EremusOne/ESPectrum), el emulador de Spectrum para ESP32.
- A **Rodolfo Guerra**, por las ROM v3 del TK90X en 17 idiomas, que incluimos con su permiso, y por su ayuda con los TK90X y TK95 (https://sites.google.com/view/rodolfoguerra).

La licencia y los créditos también se ven en el propio emulador: F1 y luego **A**.

## Créditos y procedencia del código

- **Red ZX Net del Interface 1**: traducción de la de SpectRusty (© 2020-2022 Rafal Michalski, LGPL 3 o posterior). **RS232**: basado en el de Fuse (Gergely Szász, GPL 2 o posterior).

- **Modelo del altavoz del beeper**: de Fuse (Fredrick Meunier: `ula_filter.c`, `speaker_filter.c`, `tv_filter.c`, GPL 2 o posterior).
- **Aceleración de cargadores**: adaptada de Fuse (Philip Kendall y otros: `loader.c`, `loader_detector.c`, GPL 2 o posterior).
- **Carga instantánea y cintas**: la trampa de LD-BYTES y los registros que deja siguen a Fuse (`tape_traps.c`); la polaridad de los bloques TZX, a libspectrum, y el formato TZX, a su especificación (<https://worldofspectrum.net/TZXformat.html>); el formato PZX, a la especificación de pzxtools (Patrik Rak).
- **Discos con protección**: la zona débil del Speedlock del +3 sigue a SAMdisk (Simon Owen, MIT, `IsSpectrumSpeedlockTrack`); los errores de CRC en la cabecera y el tramo débil de los sectores con varias copias, a Fuse (`disk.c`).

- **Spectranet** © Dylan Smith (https://spectrum.alioth.net), firmware con licencia MIT; la emulación de la tarjeta se basa en la de Fuse (Philip Kendall y otros, GPL v2 o posterior).

- **esxDOS 0.8.9** © 2005-2021 Papaya Dezign (https://esxdos.org): ROM y ficheros del sistema, sin modificar, para uso no comercial.

- **Mbed TLS** 3.6 (Apache-2.0), **jsmn** (MIT, Serge Zaitsev), **ZXInfo API** (api.zxinfo.dk) y **Spectrum Computing** (ZXDB). Lista de certificados raíz de **Mozilla** (MPL-2.0).

- **ROMs del Multiface** © Romantic Robot UK Ltd: **no se incluyen**, porque no tenemos permiso para distribuirlas (se ha pedido). La emulación del Multiface funciona con el volcado de la ROM de tu propio Multiface en `roms/` (`mf1.rom`, `mf128.rom`, `mfplus3.rom`).

- **ROMs del +3e** de Garry Lancaster (https://worldofspectrum.org/zxplus3e/), versión 1.43 para ZXMMC; distribución gratuita con permiso de Amstrad.
- **zxp3** de Kyp (https://github.com/Kyp069/zxp3) y **MiST** (`sd_card.v`): referencia para la ZXMMC y la tarjeta SD.

- **MAME** (https://github.com/mamedev/mame): controlador uPD765 y PLL de disquete (BSD-3-Clause, Olivier Galibert); la lectura de .DSK y la generación de pistas MFM siguen su código. Texto de la licencia en `third_party/mame/LICENSE`.

- **CPU Z80**: [z80cpp](https://github.com/jsanchezv/z80cpp) de José Luis Sánchez (GPL 3), sin modificaciones, en `third_party/z80cpp`.
- **ULA, contención, bus flotante, paginación y AY-3-8912**: portados a C++ desde [JSpeccy](https://github.com/jsanchezv/JSpeccy), de José Luis Sánchez.
- **ROMs**: las de JSpeccy (Spectrum e Interface 1). Amstrad permite distribuir las ROMs del Spectrum para su uso en emuladores.
- **Circle**: Rene Stange (GPL 3).

## Asistencia de IA y recursos

Partes de este emulador se han desarrollado con la ayuda de Claude (Anthropic), usado para código, pruebas, depuración y documentación. El acceso a Claude lo ha proporcionado Advanced Computer Trading, S.L. (actsl.com), que también autoriza la publicación de este trabajo bajo la licencia indicada abajo. Todas las decisiones de diseño, la integración, las pruebas en hardware real y la revisión final las ha hecho el autor, Shaeon (Carlos Palmero).

## Licencia

Copyright (C) 2026 Shaeon (Carlos Palmero).

Este programa es software libre: puede redistribuirse y/o modificarse bajo los términos de la **Licencia Pública General de GNU** publicada por la Free Software Foundation, ya sea la **versión 3** de la Licencia o (a su elección) cualquier versión posterior. Ver [LICENSE](LICENSE).

Hace falta la versión 3 porque la librería Circle y z80cpp tienen licencia GPL v3. El código de terceros conserva sus avisos de copyright y sus licencias, todas compatibles con la GPL v3: Fuse (GPL v2 o posterior), SpectRusty (LGPL v3 o posterior), uPD765 y PLL de disquete de MAME (BSD-3-Clause), Mbed TLS (Apache-2.0), jsmn (MIT), firmware de la Spectranet (MIT).

Las ROMs y los ficheros de sistema que acompañan al emulador no están cubiertos por la GPL y son de sus respectivos dueños: las ROMs de Sinclair y Amstrad se distribuyen con permiso de Amstrad para su uso en emuladores; las del +3e, con permiso de Garry Lancaster; esxDOS (Papaya Dezign), para uso no comercial; el CP/M del LEC de Jiří Lamač, gratuito sin ánimo de lucro y citando al autor. El firmware de la Raspberry Pi y el de la WiFi los descarga `build.sh` y conservan sus licencias (`LICENCE.broadcom` y los ficheros de `firmware/`).

## Aviso legal

Este proyecto se ofrece **"tal cual", sin garantía de ningún tipo**, expresa o implícita, incluidas, entre otras, las garantías de comerciabilidad y de idoneidad para un fin determinado. En ningún caso los autores o colaboradores serán responsables de ninguna reclamación, daño u otra responsabilidad derivada del uso de este software, del hardware en que funciona o de cualquier equipo conectado (incluido lo que se conecte a los pines GPIO). Úsalo bajo tu propia responsabilidad.

ZX Spectrum y Sinclair son marcas de sus respectivos propietarios; Raspberry Pi es una marca de Raspberry Pi Ltd. Este proyecto no está afiliado a ellos ni cuenta con su respaldo.

## Estado (versión 1.10)

Hecho:

- Modelos 16K, 48K, 128K, +2 (gris), +2A, +3 y +3e con las temporizaciones de JSpeccy.
- Contención de memoria y de E/S, bus flotante y lectura de 0x7FFD en el 128K.
- Borde completo y pantalla redibujados según el t-state, de modo que funcionan los efectos multicolor y del borde.
- ULAplus: modo de 64 colores, incluidos los cambios de paleta en mitad del cuadro.
- Tamaño de borde seleccionable (sin borde, estándar, completo, enorme) y scanlines.
- Beeper y AY-3-8912 a 48 kHz, por HDMI o por el jack de 3,5 mm (PWM). El AY tiene reparto estéreo seleccionable (ABC, ACB, BAC o mono). El beeper pasa por un modelo del altavoz tomado de Fuse.
- Sonido de la carga: la señal de la cinta se oye por el altavoz, como en el Spectrum real.
- Teclado Issue 2 o Issue 3 y velocidad de emulación x1, x2, x4 o x10.
- Cintas TAP, TZX, PZX y CSW con tres modos de carga:
  - **instantánea**: trampa en LD-BYTES de la ROM, como Fuse: en 0x0562 (sirve igual si se entra por 0x0556 o directamente a media rutina, como el cargador de Cabal de Ocean). Deja los registros como la ROM (los mismos que `tape_traps.c` de Fuse) y vuelve por SA/LD-RET, que restaura el borde y activa las interrupciones;
  - **rápida**: carga real por pulsos a máxima velocidad, viéndose las franjas;
  - **real**: carga por pulsos a velocidad normal.
- **Modelo**: si un TZX dice en su bloque de información de hardware (0x33) que no funciona en el modelo actual, al ponerlo se pregunta si se cambia a uno en el que sí funcione (por ejemplo, del 128K al 48K).
- **Bloques "turbo" con los tiempos de la ROM**: se cargan también al instante, como en JSpeccy.
- **Navegador de bloques (F3)**: lista lo que hay en la cinta, como el de Fuse: las cabeceras de la ROM ("Programa:", "Bytes:" con su nombre, dirección y longitud) y sus datos, los bloques turbo, tonos, pulsos y pausas, y en otro color lo que ayuda a orientarse en una multicarga: los textos y grupos del TZX ("Level 2") y los "PARAR LA CINTA". El ">" marca dónde está la cinta. ENTER la deja parada al principio del bloque elegido: la carga instantánea lee desde ahí, y en rápida o real arranca sola cuando el programa la lee. Inicio y Fin van al primero y al último.
- **TZX como en Fuse (libspectrum)**: el primer pulso de un bloque que va tras una pausa, o al empezar la cinta, es nivel bajo (importa a los cargadores que miden cada bit como nivel bajo más alto, como el de Basil the Great Mouse Detective); una pausa de 0 no cambia el nivel; los bloques de parada (0x20 con pausa 0, y 0x2A en el 16K y el 48K) paran la cinta, pero el cargador puede volver a arrancarla, porque las multicargas los usan entre niveles; al llegar al final, no.
- **Aceleración de cargadores** (`tapeaccel=1`, por defecto), adaptada de Fuse (`loader.c` y `loader_detector.c`, de Philip Kendall, GPL 2 o posterior): en la carga rápida y en los bloques que la instantánea no puede coger, si el bucle que espera cada flanco de la cinta es uno conocido (el de la ROM y sus copias en RAM, Microprose, Alkatraz, Digital Integration, los de búsqueda), en los datos se adelanta el flanco y se dejan los registros como si el bucle lo hubiera encontrado. El tono guía y los sincronismos se emulan normalmente. Vale también para muchos turbos. Con Cabal (Ocean, 128K) en carga rápida, los cuadros emulados bajan de 31.031 a 12.455; con una cinta de prueba con un bloque estándar y uno turbo, de 4.710 a 2.213, sin ningún byte mal (`test/host/accel`).
- Arranque y parada automáticos de la cinta.
- TZX:
  - bloques de señal: 0x10 (estándar), 0x11 (turbo), 0x12 (tono), 0x13 (pulsos), 0x14 (datos puros), 0x15 (grabación directa), 0x18 (CSW, sin comprimir o Z-RLE) y 0x19 (datos generalizados);
  - bloques de control: 0x20 (pausa/parada), 0x23 (salto), 0x24/0x25 (bucles), 0x26/0x27 (llamada a secuencias y vuelta), 0x28 (selección), 0x2A (parar si 48K), 0x2B (nivel) y los informativos;
  - con un bloque **0x28** (cintas con varias partes o versiones), la cinta se para y sale una lista para elegir qué cargar: cursores y ENTER, o el número de la opción; ESC sigue con el bloque siguiente;
  - los bloques no estándar se cargan siempre por pulsos, aunque esté elegida la carga instantánea;
  - un bloque sin pausa seguido de una parada, o al final de la cinta, termina con su flanco final y 1 ms, para que el cargador pueda leer el último bit (*Shadow of the Beast*, *Astro Marine Corps*);
  - los bloques obsoletos 0x34 (información de emulación) y 0x40 (instantánea), de las revisiones del formato hasta la 1.13, se reconocen y se saltan;
  - especificación del formato (revisión 1.20): <https://worldofspectrum.net/TZXformat.html>.
- Ficheros .CSW sueltos (versiones 1 y 2, RLE y Z-RLE).
- Ficheros .PZX (Perfect ZX Tape): pulsos (PULS), datos con su nivel inicial (DATA), pausas (PAUS), textos (BRWS) y paradas (STOP). Guardan la polaridad real de la señal, que el TZX no guarda: hay cargadores que la necesitan (Forbidden Planet). Los bloques de datos con los tiempos de la ROM se cargan también al instante.
- **Polaridad**: en el navegador de bloques (F3), la tecla **P** invierte la señal de la cinta. Sirve para los TZX que la tienen al revés (con el de Forbidden Planet, el cargador desajusta medio bit y no termina nunca). Vuelve a la normal al poner otra cinta.
- Cartuchos del Interface 2 (.ROM).
- Interface 1 con hasta 8 microdrives (cartuchos .MDR y .MDV, lectura y escritura), RS232 y red ZX Net.
- Ampliación de memoria LEC de 528 KB (48K, y en los 128K desde 48 BASIC), con la que funciona el CP/M 2.2 de LEC sobre microdrives (incluido en `cpm/`).
- Grabación con SAVE: los bloques que graba la ROM van a ficheros .TAP en la carpeta `saves` de la SD.
- Snapshots: carga de .Z80 (versiones 1, 2 y 3), .SNA (48K y 128K) y .SZX; grabación en .Z80 v3, .SNA o .SZX con F8.
- Grabación de la señal de cinta (salida MIC) con Alt+W, para programas con rutinas de grabación propias.
- Spectrum +3 con disquetera: discos .DSK (normales y extendidos, con protecciones: sectores débiles, Speedlock, pistas largas, errores de CRC) y discos de 720K (la unidad pasa sola a ser de 3,5"), con el controlador uPD765 de MAME.
- Spectrum +3e (ROMs de Garry Lancaster) con disco duro en una tarjeta ZXMMC: imágenes .VHD, .IMG, .RAW y .HDF.
- divMMC con esxDOS 0.8.9: su tarjeta es una imagen (`hdd/esxdos.img`) y lo que se deja en la carpeta `esxdos_i` se copia a ella; puertos divMMC en el +3e.
- Tarjeta de red Spectranet, con el firmware oficial.
- ROMs propias para cualquier máquina (carpeta `roms` de la SD).
- Red por WiFi o cable: servidor web para gestionar la SD desde el navegador (se puede apagar, dejar en solo lectura o proteger con contraseña), hora por NTP y búsqueda y descarga de programas de ZXDB por HTTPS, desde la web o desde el navegador de ficheros del propio emulador (F3).
- Configuración desde el emulador (F1, C), con guardado en `cmdline.txt` y reinicio de la Pi.
- Menús y página web en español e inglés (se pueden añadir más idiomas sin recompilar).
- En las Pi de 4 núcleos, la ampliación de la imagen va en el núcleo 1.
- Navegador de ficheros de la SD con directorios y salto por letra; al volver a abrirlo, el cursor sigue en el último fichero elegido.
- Teclado USB con conexión en caliente.
- Entrada de cinta desde un casete real, por un GPIO (con un comparador) o por una tarjeta de sonido USB (Pi 4/400), con grabación a .TZX (Alt+G).
- Teclado real de Spectrum y dos joysticks Atari por GPIO, con el montaje de pastbytes para ZXBaremulator: teclados de 48K, Spectrum+, 128 y +2 gris (8 × 5) o del +2A/+2B/+3 (13 × 11), elegido con `gpiokbd=` (ver más abajo).
- Joysticks Kempston, Fuller, Sinclair 1/2, Cursor/AGF/Protek y QAOPSpaceM, manejados con mandos USB o con los cursores del teclado. El Kempston se decodifica solo por A5, como el interfaz original, así que funciona también con los juegos que no leen el puerto 0x1F (Las Tres Luces de Glaurung, Silent Shadow).
- Mandos USB con conexión en caliente (hasta 4):
  - con mapeo conocido: PS3, PS4 DualShock, Xbox 360, Xbox One, Switch Pro y 8BitDo;
  - mandos HID genéricos, incluidos los adaptadores Mayflash en modo DirectInput.
- Ratón Kempston (con rueda, como el Kempston Mouse Turbo) usando un ratón USB.
- Teclado Recreated ZX Spectrum por USB en la capa de juego (Layer A): por cable se reconoce al conectarlo (VID:PID) y, si no, con detección automática.
- Multiface One (48K), Multiface 128 (128K) y Multiface 3 (+2A). Se activa con F5 o con el botón de NMI por GPIO.
- Opciones en `cmdline.txt`.

**RS232** (`FORMAT "t";9600`, `OPEN #4;"t"`, `PRINT #4`, `INPUT #4`, y `OPEN #3;"t"` para que `LPRINT` y `LLIST` salgan por él):
- La Pi hace de terminal en el **puerto TCP 2323**: conéctate desde el PC con `telnet IP-de-la-Pi 2323`, `nc IP 2323` o PuTTY en modo "Raw". Lo que manda el Spectrum sale ahí y lo que escribes le llega.
- Si no hay nadie conectado, la salida se añade a `saves/rs232.txt` en la SD: así `LLIST` con `OPEN #3;"t"` "imprime" el listado en un fichero.
- Se emula como en Fuse: la ROM del IF1 manda y lee los bits uno a uno por el puerto 0xF7, y los bytes se rehacen contando los accesos, sin depender de la velocidad elegida. El IF1 ve siempre un terminal listo (DTR).

**Red ZX Net** (`FORMAT "n";1`, `SAVE *"n";2`, `LOAD *"n";1`, `OPEN #4;"n";2`…):
- Las Pi con ZXMetalCircle de la misma red local forman una red de Spectrum: los paquetes van por **UDP en el puerto 5050**, por difusión.
- La emulación es la de SpectRusty (Rafal Michalski, LGPL 3), traducida a C++: reconoce por los tiempos de los accesos al puerto 0xF7 cada byte de los paquetes (cabecera de 8 bytes y hasta 255 de datos) y los manda enteros; el formato UDP es el suyo, así que también debería entenderse con SpectRusty.
- `rs232=0` y `zxnet=0` los desactivan (también en la configuración, página de periféricos). Hace falta el Interface 1 conectado (`if1=1`).

**Probado en el PC**: un programa con `FORMAT "t";9600`, `OPEN #4;"t"`, `PRINT #4;"HOLA RS232"` e `INPUT #4;a$` manda "HOLA RS232" + CR + LF y recibe "ZX OK"; y dos Spectrum emulados unidos en memoria se pasan un programa con `SAVE *"n";2` en la estación 1 y `LOAD *"n";1` en la 2. Falta probar el TCP y el UDP en la Pi.

## Compilar

Cómo preparar el compilador en Linux o Windows y generar la SD: [docs/01_building_es.md](docs/01_building_es.md).

## 32 y 64 bits

El mismo código se compila en 32 bits (AArch32) y en 64 bits (AArch64). `config.txt` elige el kernel de cada modelo:

- **Pi 1, Zero y Pi 2**: 32 bits.
- **Pi 3, Zero 2 W, 4 y 400**: 64 bits. Los kernels de 32 bits también van en la SD: para usarlos, en la sección `[pi3]`, `[pi02]` o `[pi4]` de `config.txt` se pone `arm_64bit=0` y el kernel de 32 bits (las líneas están explicadas en el propio fichero).
- **Pi 5 y 500**: 64 bits (Circle no admite 32 bits en ellas). Primeras pruebas en una Pi 5 real (0.76): arranca, teclado USB y ayuda. Diferencias con las demás:
  - **Imagen con una sola página**: el firmware de la Pi 5 no cambia de página (`SetVirtualOffset`) y mostraba las dos, una encima de otra y en pequeño. Se dibuja directamente en la página visible; puede verse algún corte de imagen.
  - **La CPU amplía la imagen**: la GPU no escala el framebuffer (lo muestra tal cual, arriba a la izquierda), así que el framebuffer va siempre a la resolución de la pantalla y la CPU hace todo el aumento, con scanlines o sin ellas. Con scanlines y borde completo a 1080p, el núcleo 0 tarda 7,9 ms por cuadro con la imagen incluida.
  - **Sonido por el HDMI0** (el conector junto a la alimentación): el sonido HDMI de Circle no funciona por el HDMI1. Sin jack, el PWM sale por el GPIO12 y el GPIO13, que son líneas del teclado por GPIO, así que solo se usa con `gpiokbd=0` (antes, por el HDMI1, el PWM hacía que el teclado pareciera pulsar teclas solo).
  - Sin varios núcleos todavía: la imagen se amplía en el núcleo 0.

**Imagen sin cortes**: en las Pi 1 a 4 la imagen se dibuja con triple búfer (tres páginas que se turnan), así que nunca se dibuja en la página que se está viendo ni en la que está a punto de verse, y no se ven cortes a velocidad normal. A velocidad x2 o más puede verse alguno. En la Pi 5 hay una sola página (ver arriba).

**Scroll suave (opcional)**: el Spectrum da 50 imágenes por segundo y muchas pantallas van a 60 Hz, así que unas imágenes duran más que otras y el scroll da tirones. Si tu pantalla admite 50 Hz, se puede forzar en `config.txt`, por ejemplo `hdmi_group=1` y `hdmi_mode=31` (1080p a 50 Hz) o `hdmi_mode=19` (720p a 50 Hz). Esas líneas ya vienen en el `config.txt`, comentadas: basta con quitar el `#` de `hdmi_group` y de un `hdmi_mode`.

Para el Spectrum no hay diferencia de funciones entre las dos versiones. En 64 bits el código ocupa algo más y el compilador tiene más registros con los que optimizar el Z80; la diferencia de velocidad real está por medir en una Pi con Alt+D.

**Probar en QEMU**: `QEMU=1 ./build.sh "1 3-64"` deja en `qemu/` kernels compilados con `ZX_QEMU` (sin sonido, porque QEMU no emula el HDMI ni el PWM y Circle se cuelga al intentarlo, y con IP fija para la red de usuario de QEMU). La Pi Zero se prueba en 32 bits (`qemu-system-arm -M raspi0 -bios qemu/kernel.img`, sin opciones) y la Pi 3 en 64 bits (`qemu-system-aarch64 -M raspi3b -kernel qemu/kernel8.img`, con las opciones de `cmdline.txt` en `-append`). Esos kernels no van a la SD.

- **Probado**: el kernel de 64 bits de la Pi 3 en QEMU (`qemu-system-aarch64 -M raspi3b`), con la imagen en el núcleo 1, el teclado USB, la ayuda F1, el 48K y el +3. Falta probarlo en una Pi 3 y 4 reales, y la Pi 5 entera.

## Instalar

1. Formatea una tarjeta SD en FAT32.
2. Copia en ella todo el contenido de `sdcard/` (o del `ZXMetalCircle-x.xx-sd.zip` de la publicación).
3. Copia también tus ficheros `.TAP`, organizados en carpetas como quieras.
4. Conecta un teclado USB y una pantalla HDMI, y enciende la Pi.

**Para actualizar** a una versión nueva, copia el contenido de `sdcard/` encima del de tu SD. Se sustituyen los kernels, `config.txt`, `cmdline.txt` y las ayudas, y no se tocan tus juegos, `saves/`, `snaps/`, `descargas/` ni la tarjeta de esxDOS (`hdd/esxdos.img`, que las versiones nuevas no traen). Si tienes cambios propios en `cmdline.txt` (o en la WiFi), guárdalos antes y vuelve a ponerlos.

## Guías de uso

Todo lo que explica cómo usar el emulador está en la carpeta [`docs`](docs/README.md), en español y en inglés:

| # | Guía | Contenido |
| --- | --- | --- |
| 01 | [Compilar](docs/01_building_es.md) | compilar en Linux y Windows (WSL), probar en QEMU |
| 02 | [Programar y grabar en cinta](docs/02_basic_and_tape_es.md) | escribir programas en BASIC, `SAVE` a la SD, cargarlos |
| 03 | [Pines GPIO](docs/03_gpio_es.md) | pines, teclado real y joysticks por GPIO, casete real |
| 04 | [Joysticks y mandos USB](docs/04_usb_gamepads_es.md) | dos jugadores, qué mando es de cada uno, perfiles de los botones |
| 05 | [Configuración](docs/05_settings_es.md) | opciones de `cmdline.txt`, la configuración (Alt+F1), idiomas, `excluir.txt` |
| 06 | [Teclado y ratón](docs/06_keyboard_and_mouse_es.md) | teclas, varias teclas a la vez, diagnóstico, Recreated ZX, ratón Kempston |
| 07 | [Ficheros y cintas](docs/07_files_and_tapes_es.md) | unidades, ficheros ZIP, snapshots, cartuchos del Interface 2 |
| 08 | [Discos](docs/08_disks_es.md) | +3 y discos .DSK, +3e y disco duro, divMMC con esxDOS |
| 09 | [Periféricos](docs/09_peripherals_es.md) | Spectranet, Interface 1 y Microdrives, LEC y CP/M, Multiface, ULAplus |
| 10 | [Red](docs/10_network_es.md) | WiFi, servidor web y hora, buscar y descargar programas, TNFS |
| 11 | [Imagen y sonido](docs/11_picture_and_sound_es.md) | borde y scanlines, sonido, Issue 2/3 y velocidad, varios núcleos |
| 12 | [Modelos y ROM](docs/12_models_and_roms_es.md) | TK90X y TK95, las ROM |
| 13 | [Actualizar el emulador](docs/13_updating_es.md) | la actualización desde GitHub (página 10 de la configuración) |

## Estructura

```
src/kernel.*      arranque, bucle principal, sincronización con el audio, teclas de función
src/main.cpp      punto de entrada (crea el kernel y lo pone en marcha)
src/spectrum.*    máquina: memoria, ULA, puertos, contención, traps de cinta
src/ay8912.*      chip de sonido AY-3-8912
src/audiomixer.*  generación de muestras (beeper + AY) por t-states
src/tape.*        cinta TAP/TZX/PZX/CSW (pulsos, carga instantánea y bloques)
src/inflate.*     descompresor zlib (bloques CSW Z-RLE y páginas de .SZX)
src/zipfile.*     lector de ficheros ZIP (directorio central, deflate y CRC)
src/snapshot.*    snapshots .Z80/.SNA/.SZX
src/interface1.*  Interface 1, microdrives (.MDR y .MDV) y RS232
src/zxnet.*       red ZX Net del Interface 1 (de SpectRusty)
src/if1link.*     RS232 (TCP 2323) y ZX Net (UDP 5050) por la red de la Pi
src/spectranet.*  Spectranet (flash, W5100) y snetcircle.* (sockets de Circle)
src/esxfolder.*   tarjeta de esxDOS (imagen) e importación desde la carpeta esxdos_i
src/settings.*    pantallas de configuración (F1, C) y guardado en cmdline.txt
src/optcheck.*    comprueba cmdline.txt y avisa de opciones o valores desconocidos
src/exclusions.*  ficheros y carpetas que no se muestran (SD:/excluir.txt)
src/i18n.*        idiomas (locales/*.json)
src/videocore.*   ampliación de la imagen en el núcleo 1 (Pi 2/3/4)
src/network.*     red: WiFi/cable, NTP, horario de verano, mDNS
src/earinput.*    entrada EAR de un casete real; earsource.* (GPIO con comparador o tarjeta de sonido USB)
src/webserver.*   servidor web para gestionar la SD
src/plus3disk.*   disquetera del +3 (envoltorio del código de MAME)
src/sdcard.*      tarjeta SD en modo SPI (ZXMMC del +3e)
src/harddisk.*    imagen de disco duro en un fichero (VHD, IMG, HDF) con fast seek
src/tnfs.*        cliente TNFS (lectura) con transporte intercambiable
src/tnfsbrowser.* servidores TNFS en el selector (UDP de Circle)
src/download/     cliente HTTPS, búsqueda en ZXDB, ZIP; circleglue.* une Mbed TLS con Circle
third_party/mbedtls/ Mbed TLS 3.6 (zxmc_config.h: configuración reducida)
third_party/mame/ uPD765 y PLL de MAME, capa de compatibilidad, imágenes .DSK
src/zxkeyboard.*  teclado USB -> matriz del Spectrum y joysticks
src/recreated.*   teclado Recreated ZX Spectrum (capa de juego, por VID:PID o detección)
src/gpiokeyboard.* teclado real y joysticks por GPIO
src/video.*       framebuffer de la GPU con triple búfer y dibujo de texto
src/palette.h     colores del Spectrum y de ULAplus (RGB565)
src/menu.*        navegador de ficheros, ayuda, configuración y créditos
src/roms.S        ROMs embebidas
tools/i18n/       actualizar.py: mantiene al día las traducciones
```
