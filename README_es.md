# ZXMetalCircle

[*Read in English*](README.md)

Este README en español es la referencia. Historial de cambios: [`CHANGELOG.md`](CHANGELOG.md).

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

La licencia y los créditos también se ven en el propio emulador: F1 y luego **A**.

## Créditos y procedencia del código

- **Red ZX Net del Interface 1**: traducción de la de SpectRusty (© 2020-2022 Rafal Michalski, LGPL 3 o posterior). **RS232**: basado en el de Fuse (Gergely Szász, GPL 2 o posterior).

- **Modelo del altavoz del beeper**: de Fuse (Fredrick Meunier: `ula_filter.c`, `speaker_filter.c`, `tv_filter.c`, GPL 2 o posterior).
- **Aceleración de cargadores**: adaptada de Fuse (Philip Kendall y otros: `loader.c`, `loader_detector.c`, GPL 2 o posterior).
- **Carga instantánea y cintas**: la trampa de LD-BYTES y los registros que deja siguen a Fuse (`tape_traps.c`); la polaridad de los bloques TZX, a libspectrum; el formato PZX, a la especificación de pzxtools (Patrik Rak).
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

## Estado (versión 1.01)

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
  - los bloques no estándar se cargan siempre por pulsos, aunque esté elegida la carga instantánea.
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
- Red por WiFi o cable: servidor web para gestionar la SD desde el navegador (se puede apagar, dejar en solo lectura o proteger con contraseña), hora por NTP y búsqueda y descarga de programas de ZXDB por HTTPS.
- Configuración desde el emulador (F1, C), con guardado en `cmdline.txt` y reinicio de la Pi.
- Menús y página web en español e inglés (se pueden añadir más idiomas sin recompilar).
- En las Pi de 4 núcleos, la ampliación de la imagen va en el núcleo 1.
- Navegador de ficheros de la SD con directorios y salto por letra; al volver a abrirlo, el cursor sigue en el último fichero elegido.
- Teclado USB con conexión en caliente.
- Entrada de cinta desde un casete real, por un GPIO (con un comparador) o por una tarjeta de sonido USB (Pi 4/400), con grabación a .TZX (Alt+G).
- Teclado real de Spectrum y dos joysticks Atari por GPIO, con el montaje de pastbytes para ZXBaremulator: teclados de 48K, Spectrum+, 128 y +2 gris (8 × 5) o del +2A/+2B/+3 (13 × 11), elegido con `gpiokbd=` (ver más abajo).
- Joysticks Kempston, Fuller, Sinclair 1/2, Cursor/AGF/Protek y QAOPSpaceM, manejados con mandos USB o con los cursores del teclado.
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

Hacen falta los dos toolchains de la Arm GNU Toolchain (probado con la 14.2): `arm-none-eabi` para los kernels de 32 bits y `aarch64-none-elf` para los de 64 bits, además de `git`, `wget`, `make`, `python3`, `mtools` y `dosfstools`:

```
./build.sh              # compila todos los kernels y deja todo en sdcard/
./build.sh 3-64         # solo uno
./build.sh "1 3-64 5"   # varios
```

El script descarga Circle y el firmware de la Raspberry, compila las librerías de Circle para cada objetivo y genera estos kernels:

| Objetivo | Modelo | Bits | Kernel |
| --- | --- | --- | --- |
| `1` | Pi 1 / Zero / Zero W | 32 | `kernel.img` |
| `2` | Pi 2 | 32 | `kernel7.img` |
| `3-32` | Pi 3 / Zero 2 W | 32 | `kernel8-32.img` |
| `4-32` | Pi 4 / 400 | 32 | `kernel7l.img` |
| `3-64` | Pi 3 / Zero 2 W | 64 | `kernel8.img` |
| `4-64` | Pi 4 / 400 | 64 | `kernel8-rpi4.img` |
| `5` | Pi 5 / 500 | 64 | `kernel_2712.img` |

Las rutas de los compiladores se pueden cambiar con `PREFIX32=` y `PREFIX64=`.

## 32 y 64 bits

El mismo código se compila en 32 bits (AArch32) y en 64 bits (AArch64). `config.txt` elige el kernel de cada modelo:

- **Pi 1, Zero y Pi 2**: 32 bits.
- **Pi 3, Zero 2 W, 4 y 400**: 64 bits. Los kernels de 32 bits también van en la SD: para usarlos, en la sección `[pi3]`, `[pi02]` o `[pi4]` de `config.txt` se pone `arm_64bit=0` y el kernel de 32 bits (las líneas están explicadas en el propio fichero).
- **Pi 5 y 500**: 64 bits (Circle no admite 32 bits en ellas). Primeras pruebas en una Pi 5 real (0.76): arranca, teclado USB y ayuda. Diferencias con las demás:
  - **Imagen con una sola página**: el firmware de la Pi 5 no cambia de página (`SetVirtualOffset`) y mostraba las dos, una encima de otra y en pequeño. Se dibuja directamente en la página visible; puede verse algún corte de imagen.
  - **La CPU amplía la imagen**: la GPU no escala el framebuffer (lo muestra tal cual, arriba a la izquierda), así que el framebuffer va siempre a la resolución de la pantalla y la CPU hace todo el aumento, con scanlines o sin ellas. Con scanlines y borde completo a 1080p, el núcleo 0 tarda 7,9 ms por cuadro con la imagen incluida.
  - **Sonido por el HDMI0** (el conector junto a la alimentación): el sonido HDMI de Circle no funciona por el HDMI1. Sin jack, el PWM sale por el GPIO12 y el GPIO13, que son líneas del teclado por GPIO, así que solo se usa con `gpiokbd=0` (antes, por el HDMI1, el PWM hacía que el teclado pareciera pulsar teclas solo).
  - Sin varios núcleos todavía: la imagen se amplía en el núcleo 0.

Para el Spectrum no hay diferencia de funciones entre las dos versiones. En 64 bits el código ocupa algo más y el compilador tiene más registros con los que optimizar el Z80; la diferencia de velocidad real está por medir en una Pi con Alt+D.

**Probar en QEMU**: `QEMU=1 ./build.sh "1 3-64"` deja en `qemu/` kernels compilados con `ZX_QEMU` (sin sonido, porque QEMU no emula el HDMI ni el PWM y Circle se cuelga al intentarlo, y con IP fija para la red de usuario de QEMU). La Pi Zero se prueba en 32 bits (`qemu-system-arm -M raspi0 -bios qemu/kernel.img`, sin opciones) y la Pi 3 en 64 bits (`qemu-system-aarch64 -M raspi3b -kernel qemu/kernel8.img`, con las opciones de `cmdline.txt` en `-append`). Esos kernels no van a la SD.

- **Probado**: el kernel de 64 bits de la Pi 3 en QEMU (`qemu-system-aarch64 -M raspi3b`), con la imagen en el núcleo 1, el teclado USB, la ayuda F1, el 48K y el +3. Falta probarlo en una Pi 3 y 4 reales, y la Pi 5 entera.

## Instalar

1. Formatea una tarjeta SD en FAT32.
2. Copia en ella todo el contenido de `sdcard/` (o del `ZXMetalCircle-x.xx-sd.zip` de la publicación).
3. Copia también tus ficheros `.TAP`, organizados en carpetas como quieras.
4. Conecta un teclado USB y una pantalla HDMI, y enciende la Pi.

**Para actualizar** a una versión nueva, copia el contenido de `sdcard/` encima del de tu SD. Se sustituyen los kernels, `config.txt`, `cmdline.txt` y las ayudas, y no se tocan tus juegos, `saves/`, `snaps/`, `descargas/` ni la tarjeta de esxDOS (`hdd/esxdos.img`, que las versiones nuevas no traen). Si tienes cambios propios en `cmdline.txt` (o en la WiFi), guárdalos antes y vuelve a ponerlos.

## Opciones de `cmdline.txt`

En la SD, el fichero `cmdline.hlp` (`cmdline-en.hlp` en inglés) explica todas las opciones con sus valores posibles. Casi todas se pueden cambiar también desde la configuración del emulador (F1, C).

Todas las opciones van en una sola línea:

| Opción | Valores | Por defecto |
| --- | --- | --- |
| `model=` | `16k`, `48k`, `128k`, `plus2`, `plus2a`, `plus3`, `plus3e` | `128k` |
| `tape=` | `flash` (instantánea), `fast` (rápida), `normal` (real) | `flash` |
| `tapeaccel=` | `1` (acelerar cargadores conocidos), `0` | `1` |
| `joystick=` | `none`, `kempston`, `fuller`, `sinclair1`, `sinclair2`, `cursor`, `qaop` | `kempston` |
| `sounddev=` | `sndhdmi`, `sndpwm`, `sndusb` (Pi 4/400), `none` | `sndhdmi` |
| `speaker=` | `auto`, `beeper`, `tv`, `off` (modelo del altavoz del beeper) | `auto` |
| `beeperfilter=` | `1` (quita el pitido de 1 bit), `0` | `1` |
| `recreated=` | `0` (nunca), `auto` (por cable USB se reconoce al conectarlo; si no, detección automática), `1` (todos los teclados USB son Recreated) | `auto` |
| `cursors=` | `both` (cursores = CAPS SHIFT+5-8 y joystick), `keys` (solo teclas), `joystick` (solo joystick) | `both` |
| `aystereo=` | `abc`, `acb`, `bac`, `mono` | `abc` |
| `tapesound=` | `1` (se oye la carga), `0` (silencio) | `1` |
| `border=` | `none`, `standard`, `full`, `huge` | `full` |
| `scanlines=` | `1` (activadas), `0` | `0` |
| `multicore=` | `1` (ampliación de la imagen en el núcleo 1, Pi 2/3/4), `0` | `1` |
| `issue=` | `3` (Issue 3), `2` (Issue 2, solo afecta al 16K/48K) | `3` |
| `speed=` | `1`, `2`, `4`, `10` | `1` |
| `ulaplus=` | `1` (ULAplus disponible), `0` (desactivado) | `1` |
| `lang=` | `es`, `en` o cualquier idioma de `locales/` | `es` |
| `net=` | `auto` (WiFi si hay `wpa_supplicant.conf`, si no cable), `wifi`, `eth`, `0` | `auto` |
| `tz=` | diferencia horaria en minutos (60 = España peninsular, 0 = Canarias) | `60` |
| `dst=` | `1` (horario de verano europeo automático), `0` | `1` |
| `ip=`, `mask=`, `gw=`, `dns=` | IP fija (si no se pone `ip=`, se usa DHCP) | — |
| `lec=` | `1` (memoria LEC conectada; en los 128K se usa desde 48 BASIC), `0` | `0` |
| `if1=` | `auto` (se conecta al insertar un microdrive), `1` (siempre), `0` (nunca) | `auto` |
| `rs232=`, `zxnet=` | `1`, `0` (RS232 y ZX Net del Interface 1) | `1` |
| `divmmc=` | `0`, `esxdos` | `0` |
| `esxdir=` | carpeta desde la que se importa a la tarjeta de esxDOS, `0` para ninguna | `esxdos_i` |
| `spectranet=` | `1`, `0` | `0` |
| `savefmt=` | `tap`, `tzx` (formato de las grabaciones con SAVE) | `tap` |
| `snapformat=` | `z80`, `sna`, `szx` (formato de F8) | `z80` |
| `savetrap=` | `1` (SAVE graba en `saves/` de la SD), `0` (desactivado) | `1` |
| `multiface=` | `1` (activo si está su ROM en la SD), `0` (desactivado) | `1` |
| `gpioaudio=` | `1` (con `sounddev=sndpwm`, el audio sale también por GPIO12, en mono), `0` | `1` |
| `gpiokbd=` | `48`, `plus3` (tipo de teclado por GPIO), `0` (ninguno) | `48` |
| `logdev=` | `ttyS1` (mensajes por el puerto serie) | |

Si falla el dispositivo de sonido elegido, el emulador prueba automáticamente el otro.

## Spectranet (tarjeta de red)

La **Spectranet** de Dylan Smith (https://spectrum.alioth.net) da red de verdad al Spectrum: monta servidores **TNFS** desde el BASIC, abre conexiones TCP y UDP, y la usan programas como clientes de IRC, navegadores o demos que dicen "Spectranet not found" si no la encuentran.

**Activarla**: añade `spectranet=1` a `cmdline.txt`, o conéctala en cualquier momento con **Alt+T** o en la configuración (F1, C, página 5). Hace falta que la Pi tenga red (WiFi o cable).
- Al arrancar sale "Alioth Spectranet", "Link.>OK", y en la pantalla de inicio la IP. La Spectranet usa la red de la Pi: su DHCP lo contesta el propio emulador con la IP de la Pi, así que no hay que configurar nada. Si la WiFi tarda en conectar, la Spectranet espera.
- **F5** (NMI) abre su menú: configuración de red, módulos de ROM, cargar datos y gestor de snapshots.
- Desde el BASIC, por ejemplo:
  - `%mount 0,"tnfs.markround.com"` y `%cat` para ver un servidor TNFS;
  - `%tapein "juego.tap"` y `LOAD ""` para cargar una cinta del servidor;
  - `%connect #4,"servidor",puerto`, `PRINT #4` e `INPUT #4` para una conexión TCP.
- **Conexiones**: las conexiones TCP se hacen en segundo plano, así que el Spectrum (y F1) siguen funcionando mientras tanto; si en unos 10 s no conecta, el programa recibe un error. Las direcciones 127.x.x.x, 0.x.x.x y la propia IP de la Pi fallan al momento ("Socket error"): Circle no tiene interfaz local, y antes de la 0.48 un `%connect` a 127.0.0.1 dejaba la Pi congelada. Para un servidor del PC, usa la IP del PC en la red local.
- En los modelos de 128K arranca en el BASIC de 48K. No es un fallo: lo hace el propio firmware (el módulo de extensiones del BASIC fuerza el "modo USR 0"), porque los comandos `%` solo funcionan con la ROM de 48K. En la tarjeta real pasa lo mismo. Con la Spectranet conectada, cada reset (también al cambiar de máquina con F7) pasa por su arranque y acaba en el BASIC de 48K; para usar el menú del 128K, desconéctala con Alt+T.

**El firmware**: es el oficial, de código abierto (licencia MIT), compilado desde https://github.com/spectrumero/spectranet e instalado con su instalador dentro del propio emulador. `roms/spectranet.rom` es esa flash de 128 KB, ya configurada con DHCP, la MAC 02:5A:58:4D:45:54 y el nombre `zxmetal`. La primera vez se copia a `spectranet.flash` en la raíz de la SD, que es la que se usa y se guarda sola cuando cambia (configuración, módulos nuevos…); para volver a empezar, borra ese fichero. En `utils/spectranet/` está el instalador oficial (`installer.tap`).

**No convive** con la divMMC (las dos se paginan en 0x0008), el Interface 1 ni el Multiface: con `spectranet=1` se desactivan y el emulador lo avisa.

**Cómo se emula** (basado en Fuse):
- Memoria: 128 KB de flash AM29F010 (con su programación y borrado), 128 KB de RAM, y las ventanas de 4 KB en 0x0000–0x3FFF (página 0 fija, A y B por los puertos 0x003B y 0x013B, página 3 de RAM fija).
- Paginación al ejecutar 0x0008 o 0x3FF8–0x3FFF, con la NMI y por el puerto 0x033B; se retira al ejecutar 0x007C. La trampa programable (puerto 0x023B) lanza una NMI, y la tarjeta se pagina al aceptarse.
- El chip de red W5100 se emula a nivel de socket: cada socket TCP o UDP del chip es un socket de Circle.

**Probado** en el PC (con sockets reales): instalación del firmware, arranque con DHCP, menú NMI, configuración guardada en la flash, `%mount` y `%cat` contra un servidor TNFS, `%tapein` + `LOAD ""` cargando un programa del servidor, y una conexión TCP en los dos sentidos con `%connect`, `INPUT #4` y `PRINT #4`. En QEMU, el arranque y el menú con F5 (QEMU no tiene red).

## Sonido del beeper

El beeper (y el sonido de la carga) pasa por un modelo del altavoz tomado de **Fuse** (de Fredrick Meunier: `sound/ula_filter.c`, `speaker_filter.c` y `tv_filter.c`), seguido de un filtro contra el pitido de las rutinas de 1 bit. El AY no pasa por ellos.

1. **La ULA**: un paso bajo asimétrico (sube con 36,5 µs y baja con 68,9 µs), ajustado en Fuse escuchando grabaciones de máquinas reales.
2. **El altavoz**, según la opción `speaker=` ("Altavoz" en la configuración, página 2, al momento):
   - `auto` (por defecto, como Fuse): el 16K y el 48K suenan por su **altavoz interno**, y el 128K, +2 y +3 por el **televisor**. Cambia solo al cambiar de máquina;
   - `beeper`: el altavoz interno del 48K, que no da graves (paso alto de 750 Hz); suena más fino, como el real;
   - `tv`: un televisor (100 Hz – 10 kHz);
   - `off`: sin modelo (solo se quita la componente continua): el sonido de la 0.55.
3. **Quitar el pitido de 1 bit** (`beeperfilter=1`, "Quitar pitido 1 bit"): un paso bajo de 4.º orden en 6 kHz. Muchas rutinas de voz o de varios canales usan una portadora de 10–20 kHz que en la máquina real no se oye; el modelo de Fuse solo la baja unos 13 dB, así que se mantiene este filtro: con la demo *We Are Vocoders*, la portadora (unos 16 kHz) baja unos 50 dB y lo que se oye por debajo de 4 kHz queda igual.

Cada modelo lleva su compensación de volumen (el del 48K quita los graves y sonaría más bajo).

## Varios núcleos (Pi 2, 3, 4 y Zero 2 W)

En los modelos de 4 núcleos, la **ampliación de la imagen a la pantalla** (el aumento, las scanlines, el borde enorme) se hace en el **núcleo 1**, mientras el núcleo 0 sigue con todo lo demás: la emulación, el sonido, USB, la red y la SD. Es lo que más memoria mueve en cada cuadro (unos 4 MB a 1080p), así que el núcleo 0 queda con más margen para la red, las descargas, esxDOS o la Spectranet.

- El núcleo 0 entrega cada cuadro terminado y el núcleo 1 lo amplía en la página oculta de la pantalla; el cambio de página lo hace el núcleo 0 en cuanto el 1 termina. Si el núcleo 1 aún está con el anterior, ese cuadro no se muestra (como cuando la emulación va con retraso). La imagen puede llegar hasta un cuadro (20 ms) más tarde.
- Los cambios de borde o scanlines paran un momento al núcleo 1.
- **Alt+D** muestra cuánto tarda cada cuadro en el núcleo 0 y la ampliación en el núcleo 1 (de los 20 ms que dura un cuadro). En la Pi 1 y la Zero, de un solo núcleo, muestra el tiempo del núcleo 0 con la imagen incluida.
- `multicore=0` (o "Imagen en el núcleo 1" en la configuración, página 2) vuelve a hacerlo todo en el núcleo 0. Se aplica al reiniciar.
- Por dentro: `src/videocore.h/.cpp` y Circle compilado con `ARM_ALLOW_MULTI_CORE` (solo para la Pi 2, 3 y 4; la Pi 5 de momento usa un solo núcleo).
- **Probado**: el reparto entre los dos núcleos, en el PC con hilos (3.000 cuadros con tiempos al azar y 31 cambios de modo, sin errores), y en QEMU con el kernel de 64 bits de la Pi 3 (QEMU no arranca los de 32 bits de la Pi 2 y 3). Falta probarlo en una Pi real.

## Idiomas

El emulador está en **español** (el idioma principal, que va dentro del programa) e **inglés**. Se elige con `lang=en` en `cmdline.txt` o en la página 1 de la configuración (F1, C), donde el cambio se ve al momento y se guarda con G. También cambia la página web.

- Las traducciones están en la carpeta `locales/` de la SD (se ve en la web para poder bajarlas y subirlas): un fichero por idioma, `en.json`, con el texto en español como clave y su traducción como valor. Si falta una traducción (o está vacía, `""`), se ve el español.
- `es.json` es solo de referencia: la lista completa de textos, para quien quiera traducir a otro idioma. Para añadir uno, copia `en.json` a, por ejemplo, `fr.json`, cambia `"_idioma"` por `"Français"` y traduce los valores: el emulador lo encuentra solo y aparece en la configuración.
- Los menús tienen las letras de Europa: acentos, diéresis, ñ, ç, ¡¿, «», €, æ, ø, ß y las más usadas de Europa central (č, š, ž, ł, ő…).
- La documentación está en español e inglés: `README_es.md` / `README.md`, `cmdline.hlp` / `cmdline-en.hlp` y los `LEEME.txt` / `README.txt`. El registro (`logdev=`) sigue en español.

**Mantener las traducciones al día** (para quien programa): todo texto que se vea en pantalla se escribe en español dentro de `T ("…")`, o de `N_ ("…")` en las tablas (y se traduce con `T ()` al usarlo). `build.sh` ejecuta `tools/i18n/actualizar.py`, que:
- rehace `locales/es.json`;
- añade a los demás idiomas los textos nuevos, sin traducir;
- guarda como `"_huerfano|texto"` los que ya no se usan (por si el español solo ha cambiado un poco);
- avisa de lo que falta y de las traducciones cuyos `%u`, `%s`… no coinciden con el español. Solo avisa: la compilación sigue.

## Configuración desde el emulador

Desde la ayuda (F1), la tecla **C** abre la configuración: siete páginas con las opciones de `cmdline.txt` que se pueden cambiar sin tocar el fichero. Las opciones que no se usan con lo elegido salen en gris, y el cursor se las salta: no se pueden cambiar hasta que vuelvan a tener sentido (por ejemplo, al elegir otro modelo): por ejemplo, el teclado Issue 2/3 fuera del 16K y el 48K, el idioma y el disco duro del +3e en los demás modelos, el formato de SAVE sin SAVE a la SD, o la máscara, la puerta de enlace y el DNS con la IP automática.

| Página | Opciones |
| --- | --- |
| 1 Máquina | idioma, modelo, teclado Issue 2/3, velocidad, ULAplus, idioma del +3e*, disco duro al inicio* |
| 2 Imagen y sonido | borde, scanlines, sonido AY, oír la carga, altavoz, quitar pitido 1 bit, salida de sonido*, sonido por GPIO12*, imagen en el núcleo 1* |
| 3 Cintas y grabación | modo de carga, SAVE a la SD, formato de SAVE, formato de F8 |
| 4 Teclado y mandos | joystick, cursores del PC, Recreated ZX, teclado por GPIO* |
| 5 Periféricos | Spectranet, divMMC con esxDOS, memoria LEC, Interface 1, Multiface, RS232*, ZX Net* |
| 6 Red, hora y casete | red*, zona horaria*, horario de verano*, IP fija*, máscara*, puerta de enlace*, DNS*, comprobar HTTPS*, casete real*, invertir casete*, GPIO del casete* |
| 7 ROMs y carpetas | ROM de cada máquina*, del Interface 1*, de esxDOS* y flash de la Spectranet* (las de la carpeta `roms` con el tamaño correcto), carpeta de esxDOS* |
| 8 Servidor web | página web* (sí, solo lectura o no) y su contraseña* |

- Cursores arriba y abajo para elegir; **izquierda y derecha** (o ENTER) para cambiar el valor; **RePág y AvPág** (o Tab) para cambiar de página.
- **Las direcciones IP se escriben**: ENTER en "IP fija", "Máscara de red", "Puerta de enlace" o "Servidor DNS" y se teclean los números y los puntos (también con el teclado numérico); Retroceso borra, SUPR la deja vacía (automática, con DHCP), ENTER acepta si es una IP válida y ESC la deja como estaba.
- En las listas de ROMs y de discos duros solo salen los ficheros sin espacios en el nombre (`cmdline.txt` separa las opciones con espacios).
- Los cambios se aplican **al momento**, salvo los marcados con `*`, que necesitan reiniciar la Pi.
- **G** guarda los valores en `cmdline.txt`. Conserva el resto de opciones del fichero (`hdd=`, `ip=`, `logdev=`…) y su orden, cambia las que ya estaban y añade las que no tienen su valor por defecto. Antes deja una copia en `cmdline.bak`.
- **R** reinicia la Raspberry Pi. Pide confirmación ("No" es la opción marcada) y, si hay cambios sin guardar, ofrece guardarlos antes. Antes de reiniciar se guarda todo lo pendiente: discos del +3, cartuchos de microdrive, la flash de la Spectranet, lo grabado por esxDOS y la salida del RS232.
- La configuración muestra lo que hay en `cmdline.txt` (se vuelve a leer al abrirla si no hay cambios pendientes, por si se ha cambiado desde la web), más lo que se cambie con las teclas: F4, F6, F7, F9, F10, Alt+B, Alt+S y los periféricos (Alt+T/X/L). Así G también sirve para guardar esos cambios. Lo que cambia solo, como el modelo al cargar un snapshot de 128K, no pasa a la configuración, para no guardarlo sin querer. Si hay algo sin guardar, se avisa en la pantalla.

## Unidades (F1 y luego U, o Alt+U)

Una pantalla con lo que hay metido en cada unidad: **cinta**, **disco A:** y **disco B:** del +3, **disco duro** (+3e o divMMC), **cartucho del Interface 2** y **microdrives 1 a 8**.

- **ENTER** abre el selector solo con los ficheros de esa unidad (cintas, `.dsk`, imágenes de disco duro, `.rom`, `.mdr`/`.mdv`) y lo que eliges va a esa unidad. Así se puede meter un disco en la unidad **B:** o un cartucho en un microdrive concreto. Al elegir, se vuelve a la pantalla de unidades; ESC en el selector también vuelve.
- **SUPR** (o Retroceso) vacía la unidad, grabando antes lo pendiente (discos, cartuchos, lo que haya escrito esxDOS).
- Lo que se monta aquí vale **solo para esta sesión**. El disco duro con el que arranca la Pi se elige en la configuración ("Disco duro al inicio", página 1), con las imágenes de la carpeta `hdd` (las que no tienen espacios en el nombre: `cmdline.txt` separa las opciones con espacios).

## Periféricos que se paginan sobre la ROM

La Spectranet (Alt+T), la divMMC con esxDOS (Alt+X), la memoria LEC (Alt+L) y el Interface 1 se conectan con su tecla o en la página 5 de la configuración (F1, C). No pueden estar a la vez: al conectar uno se desconectan los demás. Cada cambio reinicia el Spectrum. La divMMC no se usa en el +3e (allí la tarjeta va por la ZXMMC), y la memoria LEC en los 128K se usa desde 48 BASIC.

## Ocultar ficheros: excluir.txt

`excluir.txt`, en la raíz de la SD, dice qué ficheros y carpetas **no se muestran** en el selector (F1) ni en la web. Viene preparado para ocultar lo que necesita la Raspberry para arrancar: `bootcode.bin`, `start*.elf`, `fixup*.dat`, los `kernel*.img` (que si no aparecerían en el selector como imágenes de disco duro), `armstub*.bin`, los `*.dtb`, y las carpetas `firmware` y `certs`.

Formato, una entrada por línea (lo que va tras `#` se ignora):
- Si termina en **`/` o `\`**, es una **carpeta**; si no, un fichero.
- Si **empieza** por `/` o `\`, es una ruta desde la raíz de la SD (puede tener varias carpetas: `/juegos/viejos/`); si no, vale para ese nombre en cualquier carpeta.
- Se admiten los comodines `*` y `?`, y no distingue mayúsculas.

El fichero se vuelve a leer cada vez que se abre el selector, así que los cambios (también los hechos desde la web) se aplican sin reiniciar.

## Opciones mal escritas en cmdline.txt

Un valor mal escrito en `cmdline.txt` (por ejemplo `sounddev=sndpwn`) no da error: el emulador usa el valor por defecto. Al arrancar, ZXMetalCircle comprueba el fichero y avisa en pantalla durante unos 10 segundos ("sounddev=sndpwn no valido"; con "(+N)" si hay más problemas). También avisa de las opciones que no existen y de si `cmdline.txt` tiene más de una línea (la Raspberry solo lee la primera). Todos los problemas se escriben en el registro (`logdev=ttyS1`).

## Teclas

Las teclas de función están en una sola tabla (`KeyBindings` en `src/kernel.cpp`). La ayuda de F1 se genera a partir de ella, así que para cambiar una asignación basta con tocar esa tabla.

| Tecla | Función |
| --- | --- |
| F1 | Ayuda y estado; desde ahí, ENTER abre el selector, C la configuración, U las unidades y A los créditos |
| Alt+U | Unidades: ver, montar y vaciar cada unidad |
| Alt+K | Ayuda de teclado |
| F2 | Cinta: reproducir / parar |
| F3 | Cinta: navegador de bloques (lista de lo que hay en la cinta; ENTER la deja en el bloque elegido, Inicio = rebobinar) |
| F4 | Cambiar joystick |
| F5 | NMI: Multiface, navegador de esxDOS o menú de la Spectranet |
| F6 | Velocidad de carga: instantánea / rápida / real |
| F7 | Cambiar modelo (16K → 48K → 128K → +2 → +2A → +3 → +3e), con reset |
| F8 | Grabar snapshot en `snaps/` (.Z80, .SNA o .SZX según `snapformat=`) |
| F9 | Reparto estéreo del AY: ABC → ACB → BAC → mono |
| F10 | Velocidad: x1 → x2 → x4 → x10 |
| F11 | Quitar el cartucho del Interface 2 (con reset) |
| Alt+B | Tamaño del borde: sin borde → estándar → completo → enorme |
| Alt+S | Scanlines sí/no |
| Alt+W | Grabar la salida MIC (señal de cinta del Spectrum) |
| Alt+G | Grabar la entrada de cinta (casete real, con `ear=`) |
| Alt+N | Microdrive: insertar un cartucho nuevo (sin formatear) |
| Alt+M | Microdrives: grabar los cambios y sacar todos los cartuchos |
| Alt+L / Alt+T / Alt+X | Memoria LEC / Spectranet / divMMC sí/no (con reset) |
| Alt+F / Alt+E | Disco +3: nuevo / expulsar |
| Alt+D | Diagnóstico del teclado: lo que llega por USB, las teclas que ve el Spectrum, los cuadros por segundo y el tiempo de cada cuadro |
| F12 o Alt+R | Reset |
| Pausa o Alt+P | Pausa |

Correspondencia del teclado del PC con el del Spectrum:

- Mayúsculas = CAPS SHIFT y Ctrl = SYMBOL SHIFT.
- Tab = EXTENDED MODE, Esc = BREAK, Retroceso = DELETE y Bloq Mayús = CAPS LOCK.
- La puntuación del teclado español (`, . - + ' < >` y sus variantes con mayúsculas) se traduce a su combinación con SYMBOL SHIFT.
- **Cursores**: siempre son las teclas de cursor del Spectrum (CAPS SHIFT + 5/6/7/8). Con un joystick elegido, además mueven el joystick, y el Ctrl derecho (o el 0 del teclado numérico) es el disparo. Se puede cambiar con `cursors=`.

Para cargar una cinta: pulsa F1 y luego ENTER, elige el `.TAP`, `.TZX`, `.PZX` o `.CSW` y escribe `LOAD ""` (en el 128K/+2A, elige "Tape Loader" o "Loader" en el menú). La cinta arranca sola cuando la ROM o un cargador propio empiezan a leerla, y se para sola cuando el programa deja de leerla como un cargador durante 2 segundos. Lo que cuenta es leer como un cargador (el mismo IN una y otra vez, con un contador que avanza, como hace Fuse), no leer mucho: los menús que miran el teclado sin parar no arrancan la cinta ni la mantienen en marcha, y así las multicargas encuentran el siguiente bloque donde lo dejaron. En el modo rápido, el emulador va a toda velocidad mientras dura la carga.

## Diagnóstico del teclado (Alt+D)

Muestra arriba a la izquierda, sin parar la emulación:
- el último informe del teclado USB (modificadores y hasta 6 teclas, en hexadecimal);
- la matriz del Spectrum con las teclas pulsadas resaltadas, sumando todas las fuentes (USB, GPIO, Recreated, cursores);
- la velocidad real en cuadros por segundo, y cuánto tarda cada cuadro en el núcleo 0 (y la ampliación de la imagen en el núcleo 1, en los modelos de 4 núcleos).

Sirve para ver exactamente qué recibe el Spectrum cuando una tecla hace algo raro. Alt+D otra vez lo quita.

Si la Raspberry no da abasto para dibujar todos los cuadros (por ejemplo, con scanlines a resolución nativa en una Pi lenta), el emulador deja de mostrar algún cuadro, hasta 4 seguidos, en lugar de ir más lento. Así el Spectrum y el teclado mantienen su velocidad.

## Spectrum +3 y discos .DSK

**Cargar un disco**: elige un `.DSK` en el selector (F1) o pulsa "cargar" en la web. El disco va a la unidad **A:**. Si el modelo no es el +3, el emulador cambia al +3 con el disco ya dentro; en el menú de arranque, **Loader** lo arranca. También se puede arrancar con `model=plus3`.

**Teclas**:
- **Alt+F**: disco nuevo, ya formateado para +3DOS, en la primera unidad libre (A:, luego B:). Pregunta el formato: **180K** (3", una cara, 40 pistas, el del +3) o **720K** (3,5", dos caras, 80 pistas).
- **Discos de 720K**: cada unidad se adapta al disco que se mete. Con un DSK de dos caras o de más de 44 pistas, esa unidad pasa a ser una disquetera de 3,5" de 80 pistas y doble cara, como las que se ponían en el +3 para esos discos; con los demás sigue siendo la de 3" del +3. En la pantalla de unidades y en el aviso al meter el disco se ve "(unidad de 3,5\")".
- **Alt+E**: expulsa los dos discos (grabando antes los cambios).

**Grabación**: lo que el Spectrum escribe en un disco se graba solo en su fichero cuando el motor lleva 2 segundos parado. Los discos nuevos se guardan en `saves/disk_NNN.dsk`. Siempre se graban en formato extendido. Mientras gira el disco aparece una **D** roja arriba a la derecha.

**Cómo se emula**:
- El controlador es el **uPD765A de MAME** (`third_party/mame/upd765.cpp` y `fdc_pll.cpp`, licencia BSD-3, de Olivier Galibert), recortado al modelo del +3 y sin más cambios. Lee la superficie del disco bit a bit con su PLL, igual que el chip real, así que funcionan protecciones que se apoyan en detalles del formato.
- Una capa de compatibilidad (`third_party/mame/compat`) le da lo que espera de MAME: el tiempo (avanza con los t-states del Z80), los temporizadores y la disquetera, que entrega las transiciones magnéticas de la pista.
- `dskimage.cpp` lee los `.DSK` siguiendo `dsk_dsk.cpp` de MAME y genera las pistas MFM como su `build_pc_track_mfm()`. Incluye las pistas más largas de lo normal (sectores de 6 KB y de 8 KB), los sectores débiles (que cambian en cada lectura) y los errores de CRC. Cuando la imagen guarda varias copias de un sector (extensión de EDSK para sectores débiles), en la pista va una sola, débil desde donde las copias empiezan a diferir. Un sector de 8K no cabe en una pista: solo se graba lo que hay realmente en el disco (hasta donde difieren las copias, o como mucho 6352 bytes) y el resto se lee dando la vuelta a la pista, como en el original. Así carga **Coin-Op Hits**, que guarda 6305 bytes en sectores de 8K grabados con las celdas de bit más estrechas (ver https://simonowen.com/samdisk/sys_plus3/). Los errores de CRC en la cabecera de un sector (ST1 sin ST2) se graban en la cabecera, como en Fuse; en los sectores débiles con varias copias solo cambia el tramo donde difieren las copias. Los sectores débiles del Speedlock del +3 se reconocen como en SAMdisk ("SPEEDLOCK" en el primer sector): solo cambian los 32 bytes desde el 336 (Dragon Ninja). Al apagar el motor, el disco sigue girando y la unidad preparada 300 ms (1,5 vueltas, como en Fuse), por los cargadores que apagan el motor y enseguida leen (The Deep). Lo escrito se vuelve a decodificar para guardarlo.
- La conexión es la del driver `specpls3` de MAME: uPD765A a 4 MHz, puertos 0x2FFD (estado) y 0x3FFD (datos), motor en el bit 3 de 0x1FFD, ROMs 4.0 del +3.

**Probado** con discos comerciales protegidos (RoboCop, RoboCop 2, Batman The Movie, Turrican) y con SAVE, CAT y LOAD en +3 BASIC. Leer el disco cuesta CPU (unos 230 µs por cuadro en un PC moderno mientras gira el motor, frente a 380 µs del resto del +3): en la Pi 3 y la Pi 4 sobra, en la Zero W puede ir justo.

## Spectrum +3e y disco duro

El **+3e** es el +3 con las ROMs de Garry Lancaster, que añaden el sistema IDEDOS: discos duros y tarjetas con particiones, unidades C:, D:…, CP/M en partición, imágenes de disco montables, etc. ZXMetalCircle emula la interfaz **ZXMMC**, igual que el core zxp3 de Kyp para MiST/SiDi, así que las imágenes que se usan allí funcionan tal cual, sin convertirlas.

**Usar una imagen**:
1. Copia la imagen a la SD de la Pi, por ejemplo en una carpeta `hdd`.
2. Elígela en el selector (F1), o con "cargar" en la web, o ponla en `cmdline.txt` con `hdd=hdd/imagen.vhd`.
3. El emulador pasa al +3e y lo reinicia, porque el +3e lee las particiones al arrancar. En la pantalla de inicio debe aparecer "1 MMC".

**Formatos**:
- `.img`/`.raw`: en bruto.
- `.vhd`: en bruto, con o sin el pie de los VHD fijos. En MiST, un "VHD" suele ser una `.img` renombrada.
- `.hdf`: con la cabecera RS-IDE; los HDF de 256 bytes por sector, propios de la interfaz IDE de 8 bits, no se admiten.

El fichero puede tener hasta 4 GB, que es el límite de FAT32.

**Escrituras**: lo que grabe el +3e va directamente a la imagen y se confirma en la SD tras un segundo sin actividad. Mientras se usa el disco aparece una **H** azul arriba a la derecha.

**Cómo se emula**:
- La ZXMMC usa el puerto **0x1F** (bit 0 = selección de la tarjeta, activa en bajo) y el **0x3F** (datos SPI). Como en el `spi.v` de zxp3, cada acceso a 0x3F devuelve el byte de la transferencia anterior.
- `src/sdcard.cpp` emula una tarjeta SD en modo SPI:
  - imágenes de **hasta 2 GB**: tarjeta **SDSC** (las órdenes llevan la dirección en bytes);
  - imágenes mayores: **SDHC** (la dirección es el número de sector), como una tarjeta real de ese tamaño;
  - los bloques llevan su **CRC16** real;
  - sin la tarjeta seleccionada, la línea de datos se lee a 0, como en el ZX-Uno;
  - la ZXMMC y la divMMC tienen cada una su registro de datos, a 0 al encender.

  Todo esto lo necesitan programas que hablan directamente con la tarjeta, como el CP/M 3 de Otivax, que solo conoce las tarjetas SDSC y detecta la interfaz por esos detalles.
- `src/harddisk.cpp` lee y escribe sectores del fichero usando el *fast seek* de FatFs (activado al compilar Circle), para no recorrer la cadena de bloques del fichero en cada acceso.

**ROMs**: incluye las del +3e 1.43 para ZXMMC, en castellano (por defecto) e inglés (`p3elang=en`). Garry Lancaster las distribuye gratuitamente, con permiso de Amstrad para modificar y distribuir las ROMs del +3. Si quieres usar la misma ROM que en la FPGA, cópiala como `roms/plus3e.rom` o usa `romplus3e=zxp3.rom`.

**Probado**: con una imagen vacía, el +3e ejecuta `FORMAT TO 0,100`, `NEW DATA "prueba",4`, `MOVE "c:" IN "prueba"`, `SAVE` y `CAT` sin problemas. La imagen resultante la lee correctamente la utilidad `3e` de PC (tabla de particiones y fichero). En QEMU, montar la imagen desde el selector, `CAT TAB` y `LOAD` funcionan.

## divMMC con esxDOS

La **divMMC** es la interfaz de tarjeta SD más usada hoy en el Spectrum, y **esxDOS** su sistema: carga de cintas, snapshots y discos TR-DOS desde la tarjeta, comandos `.ls`, `.cd`, `.tapein`…, y un navegador de ficheros con la NMI. ZXMetalCircle incluye **esxDOS 0.8.9** (© Papaya Dezign, https://esxdos.org, uso no comercial) y una tarjeta lista para usar.

**Activarla**: añade `divmmc=esxdos` a `cmdline.txt`, o conéctala en cualquier momento con **Alt+X** o en la configuración (F1, C, página 5).

**La tarjeta es una imagen**: `hdd/esxdos.img`, una tarjeta FAT16 que crea el emulador la primera vez (de 16 MB, y mayor si hace falta, hasta 2 GB). esxDOS la lee por sectores, como una tarjeta de verdad.

**Para meter ficheros en ella, déjalos en la carpeta `esxdos_i`** (de "importar"), en la raíz de la SD, desde el PC o subiéndolos por la web. Al conectar la divMMC y en cada reset (F12, F7) se copian a la tarjeta, respetando las subcarpetas, y **se borran de la carpeta**:
- Cada copia se lee otra vez y se compara con el original antes de borrarlo. Lo que no se puede importar (no cabe, el nombre no vale…) se queda en la carpeta y un aviso dice cuántos y por qué, por ejemplo "esxDOS: 40 importados, 2 no (raíz de la tarjeta llena: usa subcarpetas)".
- Si en la tarjeta ya hay un fichero con el mismo nombre, el nuevo lo sustituye ("esxDOS: 3 importados (1 sustituidos)").
- Si no cabe todo, se crea una tarjeta mayor y se pasa a ella lo que ya había.
- Mientras se importa se ve el progreso ("esxDOS: importando 12 de 40 (9 de 20 MB)..."); la emulación está parada.
- Con la carpeta vacía no se hace nada, y sin la divMMC conectada, tampoco.
- La carpeta viene con el sistema de esxDOS (SYS, BIN, TMP) y una carpeta JUEGOS vacía: entran en la tarjeta la primera vez.
- Lo que graba el Spectrum (`.cp`, snapshots, `.tapeout`…) se queda en la tarjeta. La imagen no va en las SD de las versiones nuevas, así que al actualizar copiando la SD nueva encima no se pierde.
- **Desde la 0.94 y antes** la carpeta era `esxdos` y se sincronizaba con la imagen en los dos sentidos. Al arrancar, si existe `esxdos` y no `esxdos_i`, se renombra, y lo que había se importa como cualquier otra cosa (lo que ya estaba en la tarjeta se sustituye por lo mismo y deja de estar repetido en la SD).
- `esxdir=otra` usa otra carpeta para importar; `esxdir=0` desactiva la importación (la imagen de `hdd=`, o `hdd/esxdos.img`, se usa tal cual).
- Al conectar se ve la pantalla de esxDOS ("sda: ZX Metal", "hd0: …FAT16") y luego el BASIC.
- **Nombres**: esxDOS 0.8.9 muestra los nombres cortos (8.3) y pinta los bytes de 0x80 en adelante como gráficos o palabras del BASIC. Por eso, dentro de la imagen, las letras acentuadas, la ñ y la ç se guardan sin acento y lo demás como `_`; en la carpeta siguen con su nombre, y lo que graba el Spectrum vuelve a su fichero original.
- **Reloj**: la carpeta trae un `SYS/RTC.SYS` propio que da a esxDOS la hora de la Pi (puesta en hora por NTP con `pool.ntp.org`, con la zona de `tz=` y `dst=`). Así los ficheros que graba el Spectrum tienen fecha y `.date` la muestra. Sin hora todavía, esxDOS sigue como sin reloj. Por dentro, el RTC.SYS lee la fecha y la hora del puerto 0x7C3B (fuente en `tools/esxrtc/rtc.asm`). Si tu tarjeta es de una versión anterior, copia `SYS/RTC.SYS` de la carpeta `esxdos_i` de la SD nueva a la tuya, dentro de `esxdos_i/SYS/`, y se importará.

**Lo que no puede convivir con ella** (en la máquina real pasa lo mismo):
- El **Interface 1**: usa los mismos puertos (0xE7, 0xEB) y también se pagina en 0x0008.
- La **memoria LEC** (Alt+L).
- El **Multiface**: la NMI la atiende esxDOS.

- La **Spectranet**.

Al conectar una se desconectan las demás.

**En el +3e**: la divMMC no se activa (las ROMs del +3e usan la ZXMMC), pero sus **puertos de la tarjeta (0xE7 y 0xEB)** están siempre presentes y dan acceso a la misma imagen, como en el ZX-Uno. Así, los programas que hablan directamente con la tarjeta (por ejemplo, el CP/M 3.0 del Workbench del ZX-Uno) pueden encontrarla.

**Cómo se emula**:
- Puertos 0xE3 (control: CONMEM, MAPRAM y banco), 0xE7 (selección de tarjeta) y 0xEB (datos SPI).
- 8 KB de ROM (`roms/esxmmc.bin`) y 128 KB de RAM en bancos de 8 KB.
- Paginación automática al ejecutar 0x0000, 0x0008, 0x0038, 0x0066, 0x04C6 y 0x0562 (después de leer la instrucción) o 0x3D00–0x3DFF (al instante), y se retira al ejecutar 0x1FF8–0x1FFF, como describe la documentación de la divIDE/divMMC y emula ZEsarUX.
- La NMI de F5 se lanza a mitad de cuadro: si llegara justo con la interrupción del cuadro, mientras esxDOS la atiende, se ignoraría.

**Probado** en el PC y en QEMU:
- la importación: crear la imagen desde `esxdos_i/`, `.ls` y `.cd`, sustituir un fichero, y pasar a una tarjeta mayor sin perder lo que había;
- arranque de esxDOS 0.8.9 en 48K y 128K;
- `.ls` y `.cd`;
- `.tapein` seguido de `LOAD ""`, que carga un programa de la tarjeta;
- el navegador NMI con F5;
- `.128`.

### La imagen "Mar Océana" (desUBIkado)

Funciona entera en el +3e, incluido el **CP/M 3.0** del menú, que monta sus particiones como C: y D:. Su menú de arranque está pensado para el ZX-Uno. En `utils/mar_oceana/` hay una versión para ZXMetalCircle (`menu_zxmetal.tap`) que quita las opciones que aquí no hacen nada (FilePlus, uGophy y NetMan por WiFi del ZX-Uno, y ZXUC Core Setup); el `LEEME.txt` (`README.txt` en inglés) de esa carpeta explica cómo instalarla desde el propio emulador.

En `utils/mar_oceana/cpm_sdhc/` hay además un controlador del CP/M (`DIVMMC.BIN`) modificado para tarjetas SDSC y SDHC. Es experimental: en ZXMetalCircle funciona, pero en el core zxp3 dio error 23 al cargar el CP/M. Con imágenes de hasta 2 GB hay que usar el original, que es lo que viene en la imagen.

## ROMs

Solo las ROMs del **16K/48K y del 128K** van dentro del kernel, para que el emulador arranque siempre. Las demás se cargan al encender desde la carpeta **`roms`** de la SD, donde vienen incluidas:

| Fichero | Tamaño | Máquina o periférico |
| --- | --- | --- |
| `plus2.rom` | 32 KB | +2 gris |
| `plus2a.rom` | 64 KB | +2A |
| `plus3.rom` | 64 KB | +3 (y discos .DSK) |
| `plus3e-es.rom` / `plus3e-en.rom` | 64 KB | +3e (y disco duro); se elige con `p3elang=` |
| `if1.rom` | 8 KB | Interface 1 (microdrives, CP/M de LEC) |
| `mf1.rom`, `mf128.rom`, `mfplus3.rom` | 8 KB | Multiface One, 128 y 3 (F5); **no se incluyen**, pon las tuyas; sin ellas, F5 hace una NMI normal |
| `esxmmc.bin` | 8 KB | esxDOS 0.8.9 para la divMMC (`divmmc=esxdos`) |
| `spectranet.rom` | 128 KB | Flash de la Spectranet con el firmware instalado (`spectranet=1`) |

**Si falta alguna** (o tiene otro tamaño), lo que la necesita deja de estar disponible y el resto funciona:
- F7 se salta esa máquina;
- `model=` arranca en 48K;
- el selector y la web avisan al cargar un `.DSK`, una imagen de disco duro o un microdrive;
- los snapshots de esa máquina se cargan en el 128K, con aviso.

Al arrancar aparece "Faltan ROMs: …". Todo está explicado en `roms/LEEME.txt` (`roms/README.txt` en inglés), por si alguien borra una ROM sin querer.

**ROMs propias**: `48.rom` y `128.rom` sustituyen a las incluidas en el kernel, y `plus3e.rom` tiene preferencia sobre las del +3e por idioma. Con `rom48=`, `rom128=`, `romplus2=`, `romplus2a=`, `romplus3=`, `romplus3e=` y `romif1=` se puede indicar otro fichero de la carpeta (por ejemplo `romplus3e=zxp3.rom`). Las trampas de cinta solo actúan si la ROM tiene las rutinas estándar en su sitio.

## Red: WiFi, servidor web y hora

**Modelos**: WiFi en la Raspberry Pi 3, 3B+, 3A+, 4, 400, Zero W y Zero 2 W, y **en pruebas en la Pi 5 y la 500** (Circle avisa de que en la Pi 5 la WiFi puede ser inestable con una pantalla HDMI a la vez; su firmware, `brcmfmac43455-sdio.raspberrypi,5-model-b.*`, va también en `firmware`). Las Pi con conector de red (1B, 2B, 3B, 4, 5) también pueden usar cable, igual que un adaptador USB de red compatible con Circle. Con `net=auto` se usa la WiFi si hay `wpa_supplicant.conf` en la SD; para forzar el cable, `net=eth`.

**Configurar la WiFi**:
1. En la SD, cambia el nombre de `wpa_supplicant.conf.ejemplo` a `wpa_supplicant.conf`.
2. Escribe en él el nombre de tu red y su contraseña. Solo valen redes de 2,4 GHz con WPA/WPA2.
3. El firmware del chip WiFi ya va en la carpeta `firmware` de la SD.

Al arrancar, la conexión se hace en segundo plano. Cuando está lista, aparece un mensaje con la dirección, por ejemplo `WiFi http://192.168.1.50`, y la ayuda (F1) la muestra en la línea "Red:".

**Operaciones largas**: la red y la emulación comparten el núcleo 0, así que mientras se lee o se escribe un fichero grande (desde 32 KB) por la web, o se busca o descarga de ZXDB (HTTPS), la emulación se para un momento. Durante ese rato se ve un aviso abajo, por ejemplo "Red: enviando juego.tap (512 KB)" o "Red: descargando de ZXDB...".

**Servidor web**: abre esa dirección en el navegador del PC o del móvil. Desde ahí puedes:
- recorrer las carpetas de la SD;
- **subir** varios ficheros a la vez a la carpeta en la que estés (hasta 8 MB en total);
- **descargar** cualquier fichero (hasta 4 MB), por ejemplo tus partidas, snapshots o cartuchos de microdrive;
- **cargar** una cinta, snapshot, cartucho o microdrive directamente en el Spectrum;
- **borrar** ficheros (pide confirmación) y **crear carpetas**.

Además, se anuncia en la red local (mDNS) como servicio web "ZXMetalCircle".

**Proteger la página** (F1, C, página 8, o en `cmdline.txt`):
- `web=0` la apaga del todo (ni siquiera se arranca el servidor); en F1 la línea de la red dice "(web apagada)".
- `web=ro` la deja en **solo lectura**: se puede ver la SD, bajar ficheros, buscar en ZXDB y mandar cargar un juego, pero no subir, borrar, crear carpetas ni descargar de ZXDB a la SD (esas opciones ni aparecen).
- `webpass=` pone una **contraseña** (en la configuración se escribe con letras minúsculas y números, y se ve como `********`). La página la pide una vez y entonces crea una clave de sesión al azar que va en los enlaces y formularios (el servidor web de Circle no da acceso a cookies ni a la contraseña del navegador). Las claves se pierden al apagar la Pi; tras 5 intentos fallidos hay que esperar 30 s. Va sin cifrar (HTTP): protege de curiosos de tu red, no de alguien que la espíe.

**Hora**: se toma de `pool.ntp.org`. Los ficheros que grabes (SAVE, snapshots, microdrives) llevan fecha y hora reales. La zona se ajusta con `tz=` y el horario de verano europeo se aplica solo (`dst=1`).

**Zero W**: tiene un solo núcleo, así que la WiFi le quita tiempo a la emulación. Al conectar (el cifrado WPA es costoso) y mientras subes o bajas ficheros puede haber cortes breves de imagen o sonido; el Spectrum no se retrasa, porque el emulador salta cuadros si hace falta. En reposo no se nota. Al arrancar con WiFi en una Zero W aparece un aviso.

## Buscar y descargar programas (HTTPS)

En la página web del emulador hay un buscador: escribes un título y busca en **ZXDB**, la base de datos de Spectrum Computing, a través de la API de ZXInfo.
- Solo salen programas de Spectrum: ni ZX81, ni SAM Coupé, etc.
- Para cada uno se elige el mejor fichero disponible, por este orden: TZX, TAP, Z80, SNA, SZX, DSK, CSW…
- Con "**descargar y cargar**", el emulador lo baja de spectrumcomputing.co.uk, lo descomprime si viene en ZIP, lo guarda en la carpeta **`descargas`** de la SD y lo carga en el Spectrum. Si hace falta, cambia de modelo, por ejemplo al +3 para un `.DSK`.

**Cómo funciona**:
- `src/download/httpsclient.cpp` es un cliente HTTP/1.1 con redirecciones y respuestas "chunked".
- **Mbed TLS 3.6** (`third_party/mbedtls`, licencia Apache-2.0) cifra la conexión con TLS 1.2. Su configuración está en `zxmc_config.h`: ECDHE/RSA, AES-GCM, ChaCha20.
- `src/download/zxinfo.cpp` interpreta el JSON con **jsmn** (MIT) y descomprime los ZIP con el descompresor propio del emulador.
- Las conexiones se hacen en una tarea de Circle con 128 KB de pila, así que el Spectrum sigue funcionando mientras tanto.

**Certificados**: el certificado del servidor se comprueba con la lista de autoridades raíz de Mozilla, en `certs/cacert.pem` de la SD. No se comprueba la fecha de caducidad, porque Mbed TLS se ha compilado sin reloj. Con `tlsverify=0` no se comprueba nada; úsalo solo si falla por un certificado y te fías de la red.

**Probado**: en el PC, con los mismos ficheros y sockets normales, la búsqueda y la descarga funcionan contra los servidores reales (Manic Miner, Head over Heels…). También en la Pi.

## Servidores TNFS (Spectranet) en el selector

Con la red en marcha, la raíz del selector (F1) tiene una primera entrada **`[Internet (TNFS)]`**:
- Dentro aparecen los servidores TNFS de Spectrum de `SD:/tnfs.txt`: por defecto tnfs.markround.com (más de 16.000 programas), vexed4.alioth.net, tnfs.millhill.org y tnfs.bytedelight.com.
- Se recorren como carpetas de la SD.
- Al elegir un fichero, se descarga a **`SD:/descargas/<servidor>/`** y se carga como cualquier otro: cinta, snapshot, `.DSK`…

**Cómo funciona**:
- TNFS es el sistema de ficheros en red de Spectranet: UDP, puerto 16384.
- `src/tnfs.cpp` implementa la parte de lectura: MOUNT, OPENDIR/READDIR, STAT, OPEN/READ/CLOSE y UMOUNT, con reintentos y respetando el tiempo mínimo que pide el servidor.
- Para que las listas largas no tarden, usa **OPENDIRX/READDIRX**, que devuelven muchas entradas por datagrama y si son carpetas. Si el servidor no los conoce, usa el método clásico y pregunta con STAT solo por lo que no tiene extensión de fichero.
- Mientras se habla con el servidor, el menú muestra "Conectando…", "Leyendo la lista…" o "Descargando…".

Con la **Spectranet** emulada (ver su sección), el Spectrum también usa estos servidores directamente.

**Probado**: en el PC, el cliente contra el servidor de referencia `tnfsd`:
- listado de la raíz y de subcarpetas, con READDIRX y con el método clásico;
- STAT de ficheros y carpetas;
- descarga byte a byte idéntica de un `.DSK` de 128 KB;
- nombres con espacios y errores.

También usado en la Pi.

## Test de velocidad

En `utils/speedtest.tap` de la SD hay un programa para comprobar que el emulador va a la velocidad correcta (el fuente está en `tools/speedtest`):

- **Mide los t-states de un cuadro**: 69888 en 48K, 70908 en 128K/+2/+2A, con un margen de ±2.
- **Muestra el tiempo transcurrido según el propio Spectrum** (cuadros × duración exacta del cuadro). Cada segundo suena un clic y cambia el borde. Compáralo con un cronómetro: tras 10 minutos no debe haber más de 1 segundo de diferencia.
- SPACE vuelve a medir.
- Se carga en 48K, 128K, +2 y +2A con `LOAD ""` o el Tape Loader. No funciona en el 16K, porque el código va en 32768.

La ayuda (F1) muestra también la velocidad real que mide el emulador, en cuadros por segundo: debe marcar 50,08 en el 48K y 50,02 en el 128K.

## Imagen: borde y scanlines

**Tamaños de borde** (los mismos que JSpeccy):

| Borde | Imagen |
| --- | --- |
| Sin borde | 256 × 192 |
| Estándar | 320 × 240 |
| Completo | 352 × 296 (por defecto) |
| Enorme | 384 × 304 |

- El emulador genera siempre la imagen con el borde enorme y muestra la parte elegida. Cuanto menos borde, más grande se ve la pantalla del Spectrum.
- Con un borde pequeño, los menús amplían la imagen al borde completo mientras están abiertos.

**Scanlines**:
- La GPU de la Raspberry escala la imagen conservando su proporción. Por eso el framebuffer tiene siempre la misma proporción que la pantalla, y cada píxel del Spectrum ocupa r × r píxeles (r = 2 o 3).
- De cada grupo de r líneas, la última va a la mitad de brillo. La GPU aplica después una escala entera, así que todas las scanlines salen iguales.
- El emulador elige r y la escala para que la imagen sea lo más grande posible. Por ejemplo, con el borde completo:

  | Pantalla | Imagen visible |
  | --- | --- |
  | 1920×1080 | 1056×888 |
  | 1920×1200 o 1600×1200 | 1408×1184 |
  | 4K | 2112×1776 |

  El cálculo está en `src/videolayout.h` y se comprueba con `test/host/scanlines_layout_test.cpp`.
- **Tamaño con scanlines**: se usa el mayor aumento entero que cabe (por ejemplo, x5 con el borde estándar a 1200 líneas, que llena la pantalla). En la Pi 2, 3 y 4, si ese aumento deja la imagen por debajo del 90 % del tamaño sin scanlines, se usa un **aumento fraccionario a resolución nativa**: cada línea del Spectrum ocupa 3 o 4 líneas de pantalla (o 4 o 5, según el caso) y la última se oscurece. A esas resoluciones la diferencia entre líneas apenas se nota, y la imagen ocupa lo mismo que sin scanlines.
  - Ejemplos: el borde enorme a 1200 líneas pasa de x3 (76 %) a x3,95 (99 %); a 1080 líneas, todos los bordes llenan la pantalla.
  - En la Pi 1 y la Zero, con menos potencia, se mantienen los aumentos enteros con un framebuffer pequeño.
- **Nitidez**: `config.txt` lleva `scaling_kernel=8`, que hace que la GPU amplíe con el método del vecino más próximo. Sin esa línea, la GPU suaviza la imagen al ampliarla y, sin scanlines, las letras se ven borrosas.
- Si la pantalla es demasiado pequeña, el emulador avisa y no las activa. Escribir más píxeles por cuadro cuesta algo de CPU; en una Pi Zero puede notarse.

## Sonido

- **AY en estéreo**: los tres canales se reparten entre los dos altavoces y el canal central se oye por ambos lados a mitad de volumen. Los repartos posibles son:
  - **ABC** (por defecto, el más habitual);
  - **ACB**;
  - **BAC**;
  - **mono**.

  Se elige con `aystereo=` o se cambia en marcha con F9. Con la salida por GPIO12 el sonido es siempre mono.
- **Sonido de la carga**: como en el Spectrum real, la señal de la cinta llega al altavoz y se oyen el tono guía y los datos. Solo se oye en la carga a velocidad real: en la rápida el emulador va a toda velocidad y se silencia, y en la instantánea no hay señal. Se desactiva con `tapesound=0`.

## Issue 2/3 y velocidad

- **Issue 2/3**:
  - en los 16K/48K de placa Issue 2, el bit EAR que se lee del puerto 0xFE se activa tanto con el bit EAR (4) como con el MIC (3) del último OUT. En las Issue 3 y posteriores, solo con el EAR;
  - algunos programas antiguos, escritos para placas Issue 2, solo leen bien el teclado en ese modo;
  - por defecto es Issue 3. Se cambia con `issue=2` o en la configuración (F1, C, página 1), y solo afecta a los modelos 16K y 48K;
  - los snapshots .Z80 guardan y recuperan este ajuste.
- **Velocidad**:
  - F10 alterna entre x1, x2, x4 y x10;
  - a más de x1 se emulan varios cuadros por cada cuadro de la pantalla y el sonido se silencia;
  - x10 equivale a un Z80 a 35 MHz. Que se alcance depende de la Raspberry: en las Pi 3/4 debería sobrar potencia, y en una Zero quizá no llegue.

## ULAplus

ULAplus es una ampliación moderna de la ULA que añade una paleta de 64 colores, elegidos entre 256. Muchos juegos y demos recientes la aprovechan, y los programas antiguos no se ven afectados.

- **Puertos**: 0xBF3B (registro) y 0xFF3B (datos), decodificados como en JSpeccy.
- **Paleta**: los registros 0–63 son las entradas de la paleta, en formato GGGRRRBB. El registro 64 (grupo de modo) activa o desactiva la paleta.
- **Con el modo activo**, los bits FLASH y BRIGHT de cada atributo eligen una de las 4 subpaletas de 16 colores (8 de tinta y 8 de papel), así que deja de haber parpadeo. El borde usa los colores de papel de la primera subpaleta.
- Los cambios de paleta en mitad del cuadro se respetan: la imagen se genera directamente en color (RGB565), y lo dibujado antes del cambio conserva sus colores.
- Al hacer reset vuelve el modo normal.
- Los .SZX con bloque de paleta (PLT) restauran también ULAplus.
- Se puede desactivar con `ulaplus=0`.

## Cartuchos del Interface 2

El Interface 2 de Sinclair tenía una ranura para cartuchos ROM de 16 KB que, al conectarse, sustituían a la ROM del Spectrum; el juego arrancaba al encender. Hubo una decena de juegos en este formato (Jetpac, Pssst, Cookie, Tranz Am, Chess…).

- Se cargan desde el selector (F1, ENTER), eligiendo un fichero `.ROM` de hasta 16 KB. Si ocupa menos, el resto se rellena con 0xFF.
- Al insertarlo se hace un reset y arranca el cartucho. **F11** lo quita y vuelve a arrancar con la ROM normal.
- Funciona en 16K, 48K, 128K y +2. El +2A no lo admite, igual que en JSpeccy.
- Los joysticks del Interface 2 son los modos Sinclair 1 y Sinclair 2.
- Cargar un snapshot retira el cartucho, salvo que sea un .SZX que lo incluya (bloque IF2R).

## Interface 1 y microdrives

El Interface 1 de Sinclair añadía al Spectrum hasta 8 microdrives (pequeñas cintas sin fin de unos 90–126 KB), una red local y un puerto RS232. Aquí se emulan el Interface 1 y los microdrives, con la lógica de JSpeccy.

- **Insertar un cartucho**: elige un fichero `.MDR` o `.MDV` en el selector (F1, ENTER). Va a la primera unidad libre: la 1, luego la 2, etc.
- **Cartucho nuevo**: Alt+N inserta uno sin formatear de 180 sectores (unos 90 KB, como uno real; es lo mismo que usa JSpeccy) en la primera unidad libre. Hay que formatearlo desde BASIC: `FORMAT "m";2;"nombre"`.
- **Comandos**: los del Interface 1 en el BASIC del 48K, por ejemplo `CAT 1`, `SAVE *"m";1;"prog"`, `LOAD *"m";1;"prog"`, `ERASE "m";1;"prog"`, `MOVE`… En el teclado del Spectrum, FORMAT, CAT, ERASE y MOVE están en modo extendido con SYMBOL SHIFT (encima de los números).
- **Grabación**: cuando se modifica un cartucho, se graba en cuanto se para el motor, y Alt+M graba lo pendiente y saca todos los cartuchos. Los nuevos se guardan en `saves/mdr_001.…`. El formato se elige solo:
  - **.MDR** si todo el contenido son sectores de Sinclair. Para saberlo, el emulador lo convierte, lo vuelve a cargar y comprueba que no se pierde nada.
  - **.MDV** (la cinta en bruto, formato "MDVT" de JSpeccy) si no lo son, como pasa con el CP/M de LEC. También se cargan los .MDV comprimidos de JSpeccy, y los que se graban usan su mismo tamaño de sector (792, o 587 si vienen de un .MDR), así que JSpeccy los puede abrir y volver a grabar. Un cartucho que se metió como .MDV se graba siempre como .MDV.
- Mientras gira un microdrive aparece "M1".."M8" arriba a la derecha.
- Se usa la ROM 2 del Interface 1, como en JSpeccy, y el formato `.MDR` de Fuse y Spectaculator: 254 sectores y un byte final de protección contra escritura, que se respeta.
- Con `if1=auto` (por defecto), el Interface 1 se conecta al insertar el primer cartucho, de modo que no altera nada si no se usan microdrives. El +2A no lo admite.
- El RS232 (TCP 2323) y la red ZX Net (UDP 5050) salen a la red de la Pi: ver la sección del Interface 1 al principio.

## Memoria LEC y CP/M

La ampliación LEC (Jiří Lamač, 1987) sustituía las memorias del 48K por chips más grandes. Con un puerto (0xFD; en realidad, cualquier puerto con A1 = 0) se puede poner una página de 32 KB de RAM en 0x0000–0x7FFF en lugar de la ROM y la pantalla. Hay 16 páginas: la 15 es la RAM alta del propio Spectrum, que siempre está en 0x8000–0xFFFF. Su gran uso fue el **CP/M 2.2 de LEC**, que funciona con microdrives.

- Se conecta con `lec=1`, Alt+L o en la configuración (F1, C, página 5).
- **En los 128K** (128, +2, +2A, +3, +3e) funciona en modo 48K: tras conectarla, elige **48 BASIC** en el menú (o entra con USR 0) y carga el CP/M como en el 48K. La LEC original era para el 48K, y en un 128K real su puerto chocaría con el de la paginación (0x7FFD) y con el del chip AY (0xBFFD/0xFFFD), que también acaban en FD. El emulador solo toma para la LEC las escrituras con la forma que usa el CP/M (`OUT (0xFD),A`, en las que el byte alto del puerto es el propio valor), así que el AY y lo demás siguen funcionando. Probado: el CP/M arranca y lista el disco en el 48K, el 128K, el +2A y el +3. En el +2A y el +3 no hay Interface 1, así que no hay unidad B: de microdrive. Mientras la LEC está paginada no hay ROM, así que no actúan las trampas de cinta ni el Interface 1, el Multiface o el AY, igual que en JSpeccy y en el hardware real.
- La pantalla sigue siendo la RAM original del 48K, que la ULA lee siempre.
- Con la LEC paginada no se pueden grabar snapshots, porque los formatos no incluyen esa memoria.

**Probar el CP/M** (la cinta va en `cpm/` de la SD, con permiso de distribución del autor):
1. Pon el 48K con LEC (`lec=1` o Alt+L) y pulsa Alt+N para meter un cartucho nuevo, que será la unidad B:.
2. F1, ENTER, entra en `cpm` y elige `cpm22_lec.tzx`. Teclea `LOAD ""`.
3. Tras cargar, el CP/M cuenta la memoria (528 KB), monta un disco en RAM (A:) con PW, FORMAT, MLOAD y MSAVE, y muestra `A>`.
4. Formatea el microdrive: `format`, `1`, `8`, ENTER. Al terminar dice "Operation completed, 96K bytes available"; pulsa ENTER para volver.
5. Ya se puede usar B:, por ejemplo `save 4 b:prueba.com` y `dir b:`.

**Teclas en el CP/M**:
- **Borrar** (tecla de borrar del PC o flecha izquierda): mientras el CP/M está en marcha, la tecla de borrar del PC envía cursor izquierda, que es el retroceso limpio. El DELETE del Spectrum es el "rubout" del CP/M 2.2, que borra pero vuelve a escribir lo borrado (la palabra sale al revés). En el BASIC, la tecla de borrar sigue siendo DELETE.
- **Borrar la línea**: Mayúsculas + Ctrl izquierdo + X.
- **Ctrl-C** (reiniciar el CP/M, al principio de la línea): Mayúsculas + Ctrl izquierdo + C.
6. El cartucho se graba solo en `saves/mdr_001.mdv`. En otra sesión, inserta ese .MDV antes de arrancar el CP/M y tendrás tus ficheros en B:.

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

## Grabar con SAVE

Cuando la ROM graba un bloque (SAVE, SAVE ... CODE, SAVE ... SCREEN$, el "Tape" de los menús del 128K/+2A…), el emulador intercepta la rutina SA-BYTES y escribe el bloque directamente en la SD, sin esperar el tiempo real de grabación.

- Los ficheros van a la carpeta `saves` de la SD, que se crea sola. El nombre sale de la cabecera: `SAVE "prueba"` crea `saves/prueba.tap`, y si ya existe, `prueba_2.tap`, `prueba_3.tap`…
- **Nunca se modifican las cintas de los juegos.** Si la cinta actual no es de grabaciones, se crea un fichero nuevo, que pasa a ser la cinta actual. Las siguientes grabaciones se añaden a ese fichero, como en una cinta real.
- Si cargas un .TAP de la carpeta `saves` desde el selector, lo que grabes después se añade a él.
- Después de grabar, la cinta queda al final. Para volver a cargar, rebobina con F3 (Inicio y ENTER) o elígela otra vez en el selector.
- Con `savefmt=tzx` las grabaciones van a ficheros .TZX con bloques estándar (0x10) en lugar de .TAP.
- Las rutinas de grabación propias (turbo, cargadores especiales) no pasan por la ROM: para esas está la grabación de la señal (abajo).

### Grabar la señal de cinta (Alt+W)

Funciona como poner una cinta virgen a grabar: se registra cada cambio de la salida MIC del Spectrum con una precisión de 16 t-states. Sirve con cualquier rutina de grabación, incluidas las turbo y las de los juegos que graban partidas.

1. Pulsa **Alt+W**. Aparece un punto rojo arriba a la izquierda.
2. Haz que el programa grabe (SAVE, opción de grabar del juego…).
3. Pulsa **Alt+W** otra vez para parar.

Se crea `saves/rec_001.tzx` (con un bloque CSW 0x18), que pasa a ser la cinta actual, rebobinada y lista para cargar. Mientras se graba, el trap de SAVE está desactivado, así que también las grabaciones de la ROM se registran como señal.

## Recreated ZX Spectrum

El Recreated ZX Spectrum (Elite Systems, 2013) es una réplica del 48K de gomas que funciona como teclado USB o Bluetooth. Aquí solo se soporta por USB (Circle no tiene Bluetooth HID).

- Ponlo en **Layer A**, la capa de juego, con el interruptor trasero. En esa capa, cada tecla envía un carácter al pulsarse y otro al soltarse (por ejemplo, el 1 envía `a` al pulsarse y `b` al soltarse). Así se conocen las 40 teclas en cualquier combinación, como en la membrana original.
- **Por cable USB se reconoce al conectarlo**, por su identificador USB (VID:PID `0F39:0404`, TG3 Electronics; dato de José Luis Sánchez), sin pulsar nada. `recreated=0` lo desactiva (por ejemplo, para usarlo en Layer B, QWERTY).
- **Detección automática** (con `recreated=auto`, el valor por defecto) para cuando no se reconoce por el identificador, por ejemplo a través de un adaptador:
  - el emulador lo reconoce por su forma de transmitir: tres parejas completas de pulsar y soltar (por ejemplo, 'a' y luego 'b' para la tecla 1) que duran menos de 30 ms. Basta con pulsar 3 veces cualquier tecla y aparece "Teclado Recreated ZX detectado";
  - mientras tanto, esas pulsaciones no llegan al Spectrum;
  - un teclado normal se reconoce como tal tras 8 pulsaciones, y hasta entonces cada tecla puede llegar con un retraso de hasta unos 50 ms.
- Puede usarse solo o junto a un teclado USB normal y al teclado real por GPIO. Sus teclas se combinan con las del teclado GPIO.
- **Modo extendido**: CAPS SHIFT + SYMBOL SHIFT + ENTER, igual que con el teclado GPIO y que en ZXBaremulator. En ese modo, 1–0 hacen de F1–F10 y SYMBOL SHIFT de Alt.
- En Layer B (QWERTY), el Recreated funciona como un teclado normal.

## Multiface

Se emula el que corresponde a cada modelo, siguiendo la lógica de JSpeccy:

| Modelo | Multiface | Fichero en `roms/` de la SD |
| --- | --- | --- |
| 16K y 48K | Multiface One | `mf1.rom` |
| 128K y +2 | Multiface 128 | `mf128.rom` |
| +2A | Multiface 3 | `mfplus3.rom` (o `mf3.rom`) |

- **Las ROMs** (© Romantic Robot) **no se incluyen**: no tenemos permiso para distribuirlas. Copia el volcado de la ROM de tu propio Multiface en la carpeta `roms` de la SD como `mf1.rom`, `mf128.rom` o `mfplus3.rom` (8192 bytes cada una). Si falta alguna, F5 hace una NMI normal en esa máquina.
- Sin la ROM de un modelo, F5 y el botón de NMI generan una NMI normal en ese modelo.
- Se entra con F5, con el botón de NMI por GPIO o, en el modo extendido del teclado real, con la tecla 5.
- Funcionan la paginación por puertos (0x9F/0x1F, 0xBF/0x3F), el bloqueo del MF128/MF3 (incluido el modo "invisible"), la lectura de 0x7FFD/0x1FFD desde el MF y los 8 KB de RAM propia.

## Mandos USB

Se conectan y desconectan en caliente, hasta 4 a la vez. Todos manejan el joystick emulado que esté seleccionado.

| Botón | Kempston | Fuller | Sinclair 1 | Sinclair 2 | Cursor | QAOPSpaceM |
| --- | --- | --- | --- | --- | --- | --- |
| A / Cruz, Y / Triángulo, L1, R1 | Disparo | Disparo | Disparo | Disparo | Disparo | SPACE |
| B / Círculo, X / Cuadrado, L2, R2 | Disparo 2 | Disparo | X | Z | 9 | M |

- La cruceta y el stick izquierdo mueven el joystick.
- **Select / Share / View / Capture** cambia el joystick, en el mismo ciclo que ZXBaremulator: ninguno → Kempston → Fuller → Sinclair 1 → Sinclair 2 → Cursor → QAOPSpaceM.
- **Start / Options** abre la ayuda y el selector de cintas.
- En los menús, la cruceta navega, el disparo equivale a ENTER y el disparo 2 a ESC.
- Mandos genéricos: el primer "hat" o los dos primeros ejes son las direcciones. Los botones 1, 4, 5 y 6 son disparo; los botones 2, 3, 7 y 8 son disparo 2; el botón 9 es Select y el 10 es Start.
- Alimentación: varios mandos consumen bastante al arrancar. Usa una fuente de al menos 2,5 A.

## Ratón Kempston

Un ratón USB conectado (en caliente) funciona como ratón Kempston. Igual que en ZXBaremulator, solo está activo cuando el joystick seleccionado **no** es Kempston, porque comparten puertos. Si no hay ratón conectado, los puertos no responden, así que los programas que lo detectan no lo encuentran.

| Puerto | Contenido |
| --- | --- |
| 0xFADF | Botones: bit 0 derecho, bit 1 izquierdo, bit 2 central (activos a 0); bits 4-7, rueda |
| 0xFBDF | X (crece hacia la derecha) |
| 0xFFDF | Y (crece hacia arriba) |

La decodificación es parcial (A0, A5, A8 y A10), como en Fuse.

## Teclado real y joysticks por GPIO

Es compatible con el montaje "teclado y 2 joysticks compatibles Atari" de Jorge Castillo (pastbytes) para ZXBaremulator: https://pastbytes.com/zxb/. Admite dos tipos de teclado, que se eligen con `gpiokbd=`:

| `gpiokbd=` | Teclados | Matriz |
| --- | --- | --- |
| `48` (por defecto; también `1` o `128`) | 48K de goma, Spectrum+, **128** y **+2 gris** | 8 × 5 |
| `plus3` (también `plus2a`) | **+2A, +2B y +3** | 13 × 11 |
| `0` | ninguno (deja libres los pines) | — |

### Teclados de 48K, Spectrum+, 128 y +2 gris

Estos teclados tienen la misma matriz de 8 × 5 que el 48K. Las teclas de más del Spectrum+, el 128 y el +2 (cursores, EDIT, DELETE, BREAK, EXTEND MODE, `;`, `"`, `.`, `,`…) cierran dos contactos a la vez dentro de la membrana, por ejemplo CAPS SHIFT + 5. El Spectrum, y este lector, las ven igual que en la máquina real, así que no hay que configurar nada más.

Se conectan con sus dos cintas: **KB1** (5 vías, datos D0–D4) y **KB2** (8 vías, direcciones A8–A15). El orden de las líneas en las cintas es el que usa el proyecto zxunops2 de spark2k06 (https://github.com/spark2k06/zxunops2):

| Cinta | Pin | Señal | Teclas | GPIO (BCM) | Pin de la Pi |
| --- | --- | --- | --- | --- | --- |
| KB2 | 1 | A11 | 1 2 3 4 5 | 25 | 22 |
| KB2 | 2 | A10 | Q W E R T | 24 | 18 |
| KB2 | 3 | A9 | A S D F G | 23 | 16 |
| KB2 | 4 | A12 | 0 9 8 7 6 | 22 | 15 |
| KB2 | 5 | A13 | P O I U Y | 27 | 13 |
| KB2 | 6 | A8 | CAPS SHIFT Z X C V | 18 | 12 |
| KB2 | 7 | A14 | ENTER L K J H | 17 | 11 |
| KB2 | 8 | A15 | SPACE SYMBOL SHIFT M N B | 4 | 7 |
| KB1 | 1 | D0 | | 26 | 37 |
| KB1 | 2 | D1 | | 19 | 35 |
| KB1 | 3 | D2 | | 13 | 33 |
| KB1 | 4 | D3 | | 6 | 31 |
| KB1 | 5 | D4 | | 5 | 29 |

Además, opcionalmente:

| Señal | GPIO (BCM) | Pin |
| --- | --- | --- |
| Pulsador de modo extendido (a GND, pin 39) | 21 | 40 |
| Pulsador de NMI (a GND, pin 34) | 16 | 36 |
| Salida de audio PWM (ver abajo) | 12 | 32 |

### Teclado del +2A, +2B y +3

El teclado de Amstrad de estos modelos no cierra dos contactos para las teclas de más: las lleva en **5 filas y 6 columnas propias**, y la placa del +2A/+3 las convierte. ZXMetalCircle lee la matriz completa de 13 × 11, como la versión ATmega644 de zxunops2, y hace esa misma conversión.

- Las 8 primeras filas y las 5 primeras columnas son las de la tabla anterior (A8–A15 y D0–D4).
- Además, hacen falta estas líneas:

| Línea de zxunops2 | GPIO (BCM) | Pin de la Pi |
| --- | --- | --- |
| Fila 9 (R8) | 20 | 38 |
| Fila 10 (R9) | 21 | 40 |
| Fila 11 (R10) | 16 | 36 |
| Fila 12 (R11) | 12 | 32 |
| Fila 13 (R12) | 7 | 26 |
| Columna 6 (C5) | 8 | 24 |
| Columna 7 (C6) | 9 | 21 |
| Columna 8 (C7) | 10 | 19 |
| Columna 9 (C8) | 11 | 23 |
| Columna 10 (C9) | 2 | 3 |
| Columna 11 (C10) | 3 | 5 |

- **Conversión de las teclas de más** (la misma que `traduceextra2a()` de zxunops2):

| Tecla | Fila, columna | En el Spectrum |
| --- | --- | --- |
| `"` | R8, C9 | SYMBOL SHIFT + P |
| `;` | R8, C10 | SYMBOL SHIFT + O |
| `.` | R9, C5 | SYMBOL SHIFT + M |
| `,` | R9, C6 | SYMBOL SHIFT + N |
| TRUE VIDEO | R10, C5 | CAPS SHIFT + 3 |
| INV. VIDEO | R10, C6 | CAPS SHIFT + 4 |
| EDIT | R10, C7 | CAPS SHIFT + 1 |
| CAPS LOCK | R10, C8 | CAPS SHIFT + 2 |
| Cursor izquierda | R10, C10 | CAPS SHIFT + 5 |
| Cursor derecha | R11, C5 | CAPS SHIFT + 8 |
| Cursor arriba | R11, C6 | CAPS SHIFT + 7 |
| DELETE | R11, C7 | CAPS SHIFT + 0 |
| GRAPH | R11, C8 | CAPS SHIFT + 9 |
| Cursor abajo | R11, C10 | CAPS SHIFT + 6 |
| BREAK | R12, C7 | CAPS SHIFT + SPACE |
| EXTEND MODE | R12, C8 | CAPS SHIFT + SYMBOL SHIFT |

- **Pines compartidos**: este teclado usa los GPIO21, GPIO16 y GPIO12, así que con `gpiokbd=plus3` **no hay pulsador de modo extendido** (sirve la combinación CAPS SHIFT + SYMBOL SHIFT + ENTER), **ni pulsador de NMI**, **ni audio por el GPIO12**: el sonido sale por HDMI o por el jack. También usa los GPIO2 y GPIO3, que en la Pi tienen resistencias de pull-up fijas; para leer columnas no estorban.
- Todo junto ocupa 24 de los 26 GPIO del conector. Quedan el 14 y el 15, los del puerto serie: el GPIO15 (pin 10) puede servir de **entrada de cinta** (`ear=gpio`, ver más abajo), porque el registro por el puerto serie solo usa el GPIO14.

### Comprobar el cableado

El diagnóstico de teclado (**Alt+D**) muestra en pantalla la matriz del Spectrum con las teclas pulsadas resaltadas. Con `gpiokbd=plus3`, añade una línea "+3 R8-12" con el estado de las filas de más, un número por fila (bit 0 = C5 … bit 5 = C10).

Si al pulsar una tecla se ilumina otra, lo más probable es que una cinta esté al revés o con un hilo cambiado. Por ejemplo, si la "1" ilumina la "B", KB2 está del revés.

### Detalles comunes

- Las filas llevan un diodo cada una, con el cátodo hacia la Raspberry. Las columnas usan las resistencias de pull-up internas, así que no hacen falta componentes extra.
- El emulador nunca pone las filas a nivel alto: las deja como entradas o las lleva a nivel bajo para seleccionarlas.
- Los joysticks van cableados a la matriz como en el Interface 2: JOY1 son las teclas 6-0 (Sinclair 1) y JOY2 las teclas 1-5 (Sinclair 2).
- **Modo extendido**: se activa con el pulsador o con CAPS SHIFT + SYMBOL SHIFT + ENTER, y aparece un icono de teclado azul abajo a la derecha. En ese modo, 1–0 hacen de F1–F10 y SYMBOL SHIFT hace de Alt (SS+K muestra la ayuda de teclado, SS+R hace reset, SS+P pausa). Con la misma combinación se vuelve al modo normal.
- **En los menús**: los nombres demasiado largos se desplazan solos cuando se seleccionan (los demás terminan en `>`). Los cursores y RePág/AvPág se repiten al mantenerlos pulsados (a los 400 ms y luego cada 60 ms), también con CAPS SHIFT + 5/6/7/8 en el teclado GPIO. Se navega con CAPS SHIFT + 5/6/7/8 (cursores), ENTER, BREAK (Esc) y CAPS SHIFT + 0 (subir de carpeta). Las letras saltan en la lista.
- El teclado USB y el de GPIO pueden usarse a la vez.
- **NMI** (solo con `gpiokbd=48`): un pulsador entre el GPIO16 (pin 36) y GND (pin 34) genera una NMI, que activa el Multiface. Tiene antirrebote.

## Entrada de cinta: casete real

El Spectrum emulado puede cargar de un casete de verdad, igual que el original. La señal llega por un **GPIO** (con un comparador) o por la entrada de una **tarjeta de sonido USB**. Se oye la carga por el altavoz (con `tapesound=1`) y se puede **grabar a un .TZX** para conservar las cintas.

| Opción | Valores | Por defecto |
| --- | --- | --- |
| `ear=` | `0` (sin entrada), `gpio`, `usb` | `0` |
| `earpin=` | GPIO (BCM) de la entrada con `ear=gpio` | `20` (pin 38); con `gpiokbd=plus3`, `15` (pin 10) |
| `earinvert=` | `1` si la señal llega invertida | `0` |

### Por GPIO (cualquier Raspberry)

La salida de auriculares de un casete es una señal alterna de unas décimas de voltio a 1–2 V, centrada en 0 V. El GPIO solo admite 0–3,3 V, así que **no se puede conectar directamente**: hace falta un comparador que la convierta en niveles lógicos, igual que hace la ULA del Spectrum. Con un **LM393** (comparador doble que funciona a 3,3 V) alimentado desde la Pi:

```
                3,3V (pin 1)                       3,3V
                  │                                 │
                [R1 100k]          ┌──[R5 1M]──┐  [R6 10k]
casete  ──[C1 1µF]──┬──────────────┤           │    │
(punta del jack)    │      (3) IN+ ┴─┐          │    │
                  [R2 100k]          │ LM393 (1)├────┴──── GPIO20 (pin 38)
                    │      (2) IN− ┬─┘ OUT
                   GND             │
                3,3V──[R3 100k]──┬─┘
                                 ├──[C2 100nF]── GND
                          GND──[R4 100k]─┘
casete (masa del jack) ─────────────────────────────── GND (pin 39)

LM393: pin 8 = 3,3V, pin 4 = GND
```

- **C1** quita la continua y **R1/R2** centran la señal en 1,65 V.
- **R3/R4** (con C2 para estabilizar) fijan la referencia en 1,65 V.
- **R5** da unos 160 mV de histéresis, para que el ruido no produzca pulsos falsos.
- La salida del LM393 es de colector abierto, con **R6** a 3,3 V: nunca supera lo que admite la Pi.
- Volumen del casete: medio-alto; el tono, plano.
- Con el teclado de 48 (`gpiokbd=48`), la entrada va por defecto en el **GPIO20 (pin 38)**, que está libre.
- Con el **teclado del +3** (`gpiokbd=plus3`) todos los demás GPIO están ocupados, así que la entrada va por defecto en el **GPIO15 (pin 10)**. Es la entrada RX del puerto serie; el registro de diagnóstico (`logdev=ttyS1`) solo usa la salida TX (GPIO14, pin 8), así que sigue funcionando.
- Si `earpin=` indica un GPIO que ya está en uso (por el teclado, sus pulsadores, el audio por GPIO12 o el GPIO14), el emulador avisa y no activa la entrada.

**Cómo se lee con precisión**: cada flanco del pin produce una interrupción que lo anota con la hora del reloj de 1 MHz. Antes de emular cada cuadro, el emulador pasa al Spectrum los flancos de ese cuadro, convertidos a t-states, con un retraso fijo de 40 ms (dos cuadros). Así, aunque la emulación vaya a ráfagas, el Z80 ve la señal en el instante correcto.

### Por una tarjeta de sonido USB (Pi 4 y 400)

Una tarjeta de sonido USB barata, con entrada de micrófono o línea, sirve sin soldar nada: la señal se toma a 48 kHz y el emulador decide los niveles por software.
- Quita la continua.
- Sigue la amplitud de la señal: sube deprisa y baja despacio.
- Aplica una histéresis con un mínimo, para no inventar flancos en los silencios.

Con `sounddev=sndusb`, la misma tarjeta saca también el **sonido** del emulador.

**Solo funciona en la Pi 4 y la 400**: Circle tiene audio USB únicamente con el controlador xHCI de esos modelos. En las demás, `ear=usb` y `sndusb` avisan y no hacen nada; usa `ear=gpio`. El sonido USB consume algo más de CPU que el HDMI o el jack, pero en la Pi 4 sobra.

### Uso

- La entrada solo actúa cuando hay señal (algún cambio en el último segundo y medio). Si está sonando una cinta virtual, manda esa.
- Funciona a velocidad x1, como un casete real: las cargas instantánea y rápida no se aplican a la entrada externa.
- La ayuda (F1) indica "Casete: con señal / sin señal".
- **Alt+G** empieza y termina la grabación de la entrada a `saves/rec_NNN.tzx`, con un bloque de grabación directa (CSW, 16 t-states por muestra). Alt+W sigue grabando la salida MIC del propio Spectrum.

**Probado** en el PC con el núcleo del emulador:
- La señal de una cinta, entregada como flancos con su hora, carga con `LOAD ""` en un 48K. Sigue cargando con un error de temporización de hasta ±57 µs, mucho más de lo que añade una interrupción.
- La misma cinta convertida en muestras a 48 kHz (filtradas como en un casete, con continua y ruido), pasada por el detector de la entrada USB: se detectan 37.107 de 37.447 flancos y el programa carga.
- Con hardware real (el LM393 y una tarjeta USB) aún no se ha probado.

### Audio por GPIO12

Con `sounddev=sndpwm`, la señal PWM sale también por el GPIO12 (pin 32), en mono. Funciona en Pi 1, Zero, Zero 2, 2 y 3, pero no en la Pi 4/400: allí el audio PWM usa un bloque que no llega al conector, así que hay que usar el jack. Es una señal PWM, no audio analógico, y necesita un filtro:

```
GPIO12 (pin 32) ──[ 1 kΩ ]──┬──[ 10 µF + ]── salida (a un amplificador, p. ej. PAM8403)
                            │
                          [10 nF]
                            │
GND (pin 30 o 34) ──────────┴────────────── GND
```

## Estructura

```
src/kernel.*      arranque, bucle principal, sincronización con el audio, teclas de función
src/main.cpp      punto de entrada (crea el kernel y lo pone en marcha)
src/spectrum.*    máquina: memoria, ULA, puertos, contención, traps de cinta
src/ay8912.*      chip de sonido AY-3-8912
src/audiomixer.*  generación de muestras (beeper + AY) por t-states
src/tape.*        cinta TAP/TZX/PZX/CSW (pulsos, carga instantánea y bloques)
src/inflate.*     descompresor zlib (bloques CSW Z-RLE y páginas de .SZX)
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
src/video.*       framebuffer de la GPU con doble búfer y dibujo de texto
src/palette.h     colores del Spectrum y de ULAplus (RGB565)
src/menu.*        navegador de ficheros, ayuda, configuración y créditos
src/roms.S        ROMs embebidas
tools/i18n/       actualizar.py: mantiene al día las traducciones
```
