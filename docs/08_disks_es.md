# Discos

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

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

**Activarla**: añade `divmmc=esxdos` a `cmdline.txt`, o conéctala en cualquier momento con **Alt+X** o en la configuración (F1, C, página 6).

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
