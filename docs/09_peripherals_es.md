# Periféricos

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

## Spectranet (tarjeta de red)

La **Spectranet** de Dylan Smith (https://spectrum.alioth.net) da red de verdad al Spectrum: monta servidores **TNFS** desde el BASIC, abre conexiones TCP y UDP, y la usan programas como clientes de IRC, navegadores o demos que dicen "Spectranet not found" si no la encuentran.

**Activarla**: añade `spectranet=1` a `cmdline.txt`, o conéctala en cualquier momento con **Alt+T** o en la configuración (F1, C, página 6). Hace falta que la Pi tenga red (WiFi o cable).
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

## Periféricos que se paginan sobre la ROM

La Spectranet (Alt+T), la divMMC con esxDOS (Alt+X), la memoria LEC (Alt+L) y el Interface 1 se conectan con su tecla o en la página 6 de la configuración (F1, C). No pueden estar a la vez: al conectar uno se desconectan los demás. Cada cambio reinicia el Spectrum. La divMMC no se usa en el +3e (allí la tarjeta va por la ZXMMC), y la memoria LEC en los 128K se usa desde 48 BASIC.

## ULAplus

ULAplus es una ampliación moderna de la ULA que añade una paleta de 64 colores, elegidos entre 256. Muchos juegos y demos recientes la aprovechan, y los programas antiguos no se ven afectados.

- **Puertos**: 0xBF3B (registro) y 0xFF3B (datos). Se miran A15, A14 y A7-A0, no A13-A8, como en el núcleo FPGA ToK'ra de Rodolfo Guerra: los programas cargan la paleta con OTIR, que va bajando la parte alta del puerto. Los puertos del reloj de esxDOS (0x7C3B, 0x7D3B) no son de ULAplus.
- **Contención**: en los modelos de Sinclair, sus puertos tienen la de los de la ULA, aunque sean impares.
- **En los TK90X y TK95**, ULAplus es una extensión: funciona en el núcleo FPGA ToK'ra de Rodolfo Guerra, pero no existe en un TK de la época, y su contención en un TK real aún no está comprobada, así que sus puertos se tratan como cualquier otro puerto impar.
- **Paleta**: los registros 0–63 son las entradas de la paleta, en formato GGGRRRBB. El registro 64 (grupo de modo) activa o desactiva la paleta.
- **Con el modo activo**, los bits FLASH y BRIGHT de cada atributo eligen una de las 4 subpaletas de 16 colores (8 de tinta y 8 de papel), así que deja de haber parpadeo. El borde usa los colores de papel de la primera subpaleta.
- Los cambios de paleta en mitad del cuadro se respetan: la imagen se genera directamente en color (RGB565), y lo dibujado antes del cambio conserva sus colores.
- Al hacer reset vuelve el modo normal.
- Los .SZX con bloque de paleta (PLT) restauran también ULAplus.
- Se puede desactivar con `ulaplus=0`.

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

- Se conecta con `lec=1`, Alt+L o en la configuración (F1, C, página 6).
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
