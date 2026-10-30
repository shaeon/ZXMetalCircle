# Cambios de ZXMetalCircle

Todos los cambios de cada versión, de la más reciente a la más antigua. Cada versión nueva añade aquí su entrada.

Leyenda: **Nuevo**, **Cambiado**, **Corregido**, **Quitado**. "Probado" dice dónde se ha comprobado: en el PC (el núcleo del emulador), en QEMU (una Pi Zero emulada en 32 bits o una Pi 3 en 64 bits) o en una Pi real.

Las versiones 0.1 a 0.58 se hicieron entre el 10 y el 13 de octubre de 2026; este historial se reconstruyó después a partir de las notas de desarrollo, así que no lleva fecha por versión.

## Sin publicar

- **Corregido**: Robocop (+3, disco de SPIN Disk Man) dejaba la pantalla con rayas verdes. Su protección lee con N=2 el sector 2 de la pista 0, un sector débil de 512 bytes; la imagen guarda en su cabecera N=0 y 1536 bytes, que son tres copias de 512 (256 bytes de E5 iguales y 256 débiles). Como el uPD765 compara N (igual que en Fuse), no lo encontraba nunca. Ahora, si las copias de un sector con error de datos no empiezan igual con el tamaño de su cabecera pero sí con uno mayor, se toma ese tamaño. Retro Virtual Machine lo carga; Fuse probablemente no.
- **Corregido**: Guillermo Tell (Opera, versión para pistola de luz, disco de CPDRead) se quedaba en la pantalla de carga. Su protección lee el sector 8 de la pista 40 (N=8, 32K, que no cabe en la pista) y espera un error de CRC en los datos; la imagen no guarda datos de ese sector, y sin datos no se escribía la marca de datos, así que el controlador respondía "sin marca de datos". Ahora, si un sector sin datos tiene error de CRC en los datos (ST2 0x20), se escribe su marca de datos y lo que sigue es el resto de la pista, como en el disco real. Fuse lo carga; Retro Virtual Machine se quedaba en la pista 41.
- Probado en el PC: Robocop llega a su menú y Guillermo Tell al juego. Sin cambios (misma pantalla a los 30 s) en Coin-Op Hits (las dos imágenes, y el cambio al disco B en Road Blasters), Tomahawk, Ghouls'n Ghosts, The Deep, Dragon Ninja, Fernandez Must Die, The Running Man y un disco de 720K; pasan las pruebas del controlador y de protecciones.
- **Cambiado**: la pantalla de unidades abierta con Alt+U se cierra con ESC y vuelve al Spectrum; antes iba a la ayuda (F1). Abierta desde la ayuda (U), ESC sigue volviendo a la ayuda.
- **Nuevo**: Alt+F5 abre directamente el navegador de ficheros (como F1 y ENTER). La opción "F5 / Alt+F5" (`f5=`, página "Teclado y mandos" de la configuración: "NMI / ficheros" o "ficheros / NMI") intercambia las dos: con `nmi` (por defecto) F5 hace la NMI y Alt+F5 abre los ficheros; con `files`, al revés. Se aplica al momento y la ayuda (F1) dice en cada caso qué hace cada una.
- **Nuevo**: Alt+V, pulsar varias teclas a la vez. Se escriben las teclas (letras, números y espacio) y, con ENTER, se mantienen pulsadas todas juntas durante 1,5 s. Para trucos como VICTOR en Profanation (vidas infinitas), que muchos teclados USB no registran: el protocolo USB admite como mucho seis teclas a la vez y muchos teclados no detectan ciertas combinaciones. En la ayuda (F1) comparte línea con el diagnóstico del teclado: "Alt+D/V Diagnóstico / varias teclas".
- **Corregido**: cuando el teclado USB manda el aviso de demasiadas teclas a la vez (ErrorRollOver), se mantienen las teclas que ya estaban pulsadas hasta el siguiente informe válido, como dice la norma HID. Antes se soltaban todas y el juego veía de golpe que no había ninguna tecla pulsada. Prueba nueva: `test/host/keyboard_hold_test.cpp` (también en la Action).
- Probado en QEMU: Alt+V con VICTOR pulsa a la vez C, V, R, T, O e I (se ve en el diagnóstico del teclado, Alt+D) y las suelta a los 1,5 s.
- **Corregido**: Astro Marine Corps (Dinamic) en 128K se quedaba en "NO CARGA" al final de la cara 1, en todos los modos de carga. El último bloque de la cinta tiene pausa 0 y la cinta se acababa justo tras su último pulso, sin el flanco que lo cierra: el cargador no terminaba de leer el último bit. En 48K no se notaba porque el juego arranca antes de llegar a los bloques del 128K. Ahora, si el último bloque no tiene pausa, se le da 1 ms con su flanco final, como hacen JSpeccy y Fuse. Encontrado comparando con JSpeccy ejecutado sin pantalla: las dos cargas eran idénticas salvo el último bloque.
- Probado en el PC: Astro Marine Corps carga en 128K y llega al juego; sin cambios (misma pantalla y misma posición de la cinta) en Basil, Batman, Cabal, Forbidden Planet, Hollywood Poker (TAP, TZX y PZX), La Abadía del Crimen, Misifú y la cara 2 de Astro Marine Corps, en 48K y en 128K.
- **Corregido**: juegos durante la carga, como el Master Mind de Astro Marine Corps (Dinamic): al escribir un número salía repetido. En carga instantánea o rápida, o a velocidad x2 o más, mientras la cinta está en marcha el emulador va más deprisa que el Spectrum real, y una pulsación de unas décimas de segundo le duraba al juego varios segundos. Ahora, mientras se escriben teclas para el Spectrum (hasta 3 s después de la última) con la cinta en marcha, se va a la velocidad real; luego vuelve a acelerar. La carga instantánea sigue cargando cada bloque al momento, como JSpeccy. Las teclas de función del emulador (F1-F12) y las de los menús no cuentan, y sin cinta en marcha la velocidad xN no cambia.
- Probado en QEMU con Astro Marine Corps: en carga instantánea, pulsando 5, 3 y 8 escribe "538" (antes "5533"); a x10 en carga normal, F10 no frena la carga.
- **Cambiado**: en las Pi 1 a 4, la imagen se dibuja con triple búfer en lugar de doble. Con dos páginas se cambiaba de página sin esperar al retrazado y, como la pantalla no toma el cambio hasta el siguiente barrido, a veces se dibujaba en la página que todavía se estaba viendo (cortes de imagen). Con tres, la que se dibuja nunca es ni la que se ve ni la que está a punto de verse, sin tener que esperar al retrazado (que pararía la emulación). El trabajo por imagen es el mismo; solo se usa una página más de la memoria de la GPU (unos 230 KB con la imagen normal). Si el firmware no da tres páginas, se usan dos, o una. La Pi 5 sigue con una sola página, porque su firmware no cambia de página. Idea de mejorar los cortes de José Luis Sánchez.
- **Nuevo** (README): cómo forzar la pantalla a 50 Hz en `config.txt` para que el scroll no dé tirones con pantallas de 60 Hz.
- Probado en QEMU (Pi 3, 64 bits): el firmware da las tres páginas ("Framebuffer 400x296, 3 paginas" en el registro) y la imagen y los menús se ven bien. Compila para la Pi 1, la Pi 3 y la Pi 5. Los cortes en sí no se pueden ver en QEMU: falta probarlo en una Pi real.

## 1.04

- **Corregido**: The Running Man (+3, Grandslam, convertido por Emerald Software) se quedaba con la pantalla en negro. Su cargador apaga el motor de la disquetera al cambiar la paginación (escribe 07 y 04 en 0x1FFD) y carga todo el juego sin volver a encenderlo; el disco seguía girando solo 300 ms tras apagar el motor (como Fuse) y la primera lectura se quedaba esperando para siempre. Ahora, con el motor apagado, el disco sigue girando mientras se use el controlador y se para a los 2 s del último acceso (como mínimo, a los 300 ms de siempre). +3DOS apaga el motor cuando ya ha terminado, así que con él no cambia nada.
- Probado en el PC: The Running Man carga ("It's Showtime!" y la introducción); sin cambios en Coin-Op Hits, Tomahawk, Ghouls'n Ghosts, The Deep, Dragon Ninja, Fernandez Must Die y un disco de 720K, ni en las pruebas del controlador y de protecciones.
- **Cambiado**: la SD lleva todos los ficheros DTB del firmware (28, de la Pi 1 a la Pi 5 y los módulos CM), no solo los diez que trae Circle para los modelos que los necesitan. Cada modelo arranca con su descripción de hardware oficial y el firmware deja de avisar de que le falta. Los baja `build.sh` de la misma versión del firmware que usa Circle (1,4 MB en total). Gracias a José Luis Sánchez, que lo propuso y lo ha probado en todas sus Pi, incluida una Pi 1 B+.

## 1.03

- **Cambiado**: la paleta depende del modelo, como en JSpeccy. El 16K y el 48K usan la paleta calculada a partir de los voltajes YUV de la ULA del 48K (la de JSpeccy, como hasta ahora); el 128K, +2, +2A, +3 y +3e, la de siempre (negro puro, 0xC0 y 0xFF; la de Fuse y MAME es casi igual). Antes se usaba la del 48K para todos, y su negro (#060800, que en pantalla queda #000800) no era negro del todo: un usuario lo midió con una capturadora, #000A00 en la Pi frente a #000000 en el Spectrum real.
- **Nuevo**: opción "Negro puro (paleta 48K)" (`pureblack=1`, página "Imagen y sonido" de la configuración): negro #000000 también en la paleta del 16K y el 48K. Se aplica al momento y sale en gris con los demás modelos, que ya lo tienen. Prueba nueva: `test/host/palette_test.cpp`.

## 1.02

- **Cambiado**: el joystick Kempston se decodifica solo por A5 = 0 (con A0 = 1; con A0 = 0 gana la ULA), como el interfaz original y el core zxp3 de Kyp. Gracias a José Luis Sánchez, que lo propuso. Hay juegos españoles que no leen el puerto 0x1F: Las Tres Luces de Glaurung lee el 0xDF (que ya aceptábamos, como JSpeccy y ESPectrum) y Silent Shadow lee el 0xD3 (`IN A,(0D3h)`), con A6 y A7 a 1. Con A5 = A6 = A7 = 0, como Fuse y MAME, a Silent Shadow no le funcionaba el joystick. Prueba nueva: `test/host/kempston_ports_test.cpp`.
- **Nuevo**: compilación en GitHub (`.github/workflows/build.yml`). En cada push a `main`, en los pull requests y a mano: pruebas en el PC, los siete kernels y la SD como zip descargable. Con una etiqueta `vX.YY`, además, un borrador de release con el zip. Probado en un Ubuntu 24.04 con los mismos pasos: las cinco pruebas pasan y `build.sh` compila los siete kernels con el Arm GNU Toolchain 14.2 descargado.
- **Corregido**: la prueba del controlador del +3 (`fdc_test`) grababa su resultado en una carpeta de `/tmp` que solo existía en el entorno de desarrollo; en GitHub fallaba. Ahora graba en la carpeta de trabajo.
- **Nuevo**: `GPIO.md`, con todos los pines GPIO que usa el emulador (teclado 48K/128 y +2A/+3, pulsadores, sonido PWM, casete real, puerto serie), una tabla por pines, y qué se quita o cambia cuando dos opciones comparten un pin. Enlazado desde los README.
- **Cambiado**: en la configuración, "Sonido por GPIO12" sale en gris también con el teclado del +2A/+3 (que usa el GPIO12 como fila) y en las Pi 4, 400 y 5 (donde no se usa). Antes solo se ponía en gris si la salida de sonido no era el jack.

## 1.01

- **Nuevo**: discos de 720K en el +3 (reportado por Ximo). Las dos unidades eran siempre la de 3" del +3 (una cara, ~40 pistas), y +3DOS respondía "disk unsuitable" con los DSK de 720K (80 pistas, dos caras, como los que crea Cronomantic). Ahora cada unidad se adapta al disco que se mete: con dos caras o más de 44 pistas pasa a ser una de 3,5" de 80 pistas y doble cara (+3DOS lo ve en el bit "dos caras" de ST3), y si no, la de 3" de siempre. Alt+F (disco nuevo) pregunta el formato: 180K (3") o 720K (3,5", ya formateado para +3DOS: 702K libres). La pantalla de unidades y el aviso al meter el disco dicen "(unidad de 3,5")".
- Probado en el PC: los dos DSK de 720K de Ximo (CAT, y el Loader arranca los dos; el de 170 pantallas va mostrando imágenes de todo el disco), y un disco de 720K nuevo: SAVE, CAT con 700K libres, guardarlo a fichero, volver a cargarlo y el fichero sigue ahí. En QEMU: Alt+F con la pregunta del formato, y CAT de los dos discos nuevos (702K y 173K libres). Sin cambios en Coin-Op Hits, Tomahawk, Ghouls'n Ghosts, The Deep, Dragon Ninja, Fernandez Must Die ni en las pruebas del controlador y de protecciones.

## 1.00

- **Nuevo**: el Recreated ZX Spectrum por cable USB se reconoce al conectarlo por su VID:PID (`0F39:0404`, TG3 Electronics; dato de José Luis Sánchez) y el modo Recreated se activa sin pulsar nada. `recreated=auto` pasa a ser el valor por defecto (antes `0`: había que activarlo a mano); la detección por tiempos sigue para cuando no se reconoce por el identificador (un adaptador). `recreated=0` lo desactiva (Layer B, QWERTY).
- **Nuevo**: protección de la página web, en una página nueva de la configuración (8/8, "Servidor web"):
  - `web=0` la apaga (no se arranca el servidor; F1 dice "(web apagada)"); `web=ro`, solo lectura (ver, bajar, buscar y cargar; sin subir, borrar, crear carpetas ni descargar de ZXDB, que ni aparecen); `web=1`, completa (por defecto).
  - `webpass=`: contraseña. Como el servidor web de Circle no da acceso a las cabeceras (ni cookies ni Authorization), la página la pide en un formulario y crea una clave de sesión al azar (6 a la vez) que el servidor añade a cada enlace y formulario (a los POST en la acción y a todos como campo oculto). Las claves se pierden al apagar; 5 intentos fallidos bloquean la entrada 30 s. Sin cifrar (HTTP). En la configuración se escribe con letras minúsculas y números y se ve como `********`.
- Probado en el PC: la inserción de la clave en una página de ejemplo (enlaces internos y externos, formulario POST de subida, formulario GET, un `>` dentro de un onclick) y la prueba del Recreated. En QEMU (Pi 3, 64 bits): la página 8 de la configuración y escribir y aceptar la contraseña. Sin probar con red (QEMU no la tiene): la entrada con contraseña, el modo solo lectura y la web apagada; ni el reconocimiento del Recreated por su VID:PID (no tengo uno).
- Agradecimientos (en los README y en F1, A): Víctor Iborra (EremusOne), por ESPectrum (https://github.com/EremusOne/ESPectrum).
- **Corregido**: Hollywood Poker (TZX, con el tono guía "con clics") cargaba en carga real pero no en rápida ni en instantánea: tras el BASIC, su cargador volvía con "0 OK". La aceleración de cargadores, al encontrar el flanco, hacía siempre el RET de la rutina de espera como si fuera la de la ROM; pero el bucle de este cargador no termina en RET (espera los dos flancos de cada bit seguidos, con EXX y DJNZ) y el RET lo devolvía a mitad de bit. Ahora el atajo del RET solo se usa si lo que sigue al bucle es el final exacto de la rutina de la ROM; en cualquier otro bucle se deja que el propio IN vea el nivel tras el flanco y el cargador sigue por su camino. Este TZX solo funciona en 48K (su cargador se copia a 0x5B00, que el 128K usa), también en el Spectrum real.
- Revisión antes de publicar: README (lista de funciones con las novedades, ficheros del código que faltaban, créditos de la trampa de Fuse, de SAMdisk y del formato PZX, cómo actualizar sin perder nada), `third_party/mame/LICENSE` (texto de la licencia BSD-3 del código de MAME), `.gitignore` (las ROMs del Multiface no se suben), y las dos entradas de la 0.86 unidas en una.
- Probado en el PC: Hollywood Poker (TZX) en 48K, carga rápida e instantánea, llega al menú del juego; sin cambios en la cinta turbo de prueba (2.213 y 542 cuadros, como antes), Cabal, Basil, Misifu, La Abadía del Crimen, The Coin-Op, Forbidden Planet (PZX) y Batman (mismos bloques en los mismos cuadros que la 1.00 publicada).

## 0.99

- **Nuevo**: al poner un TZX, si su bloque de información de hardware (0x33) dice que no funciona en el modelo actual, se pregunta si se cambia a uno en el que sí funcione ("Esta cinta no va en el ZX Spectrum 128K: 1 Cambiar al ZX Spectrum 48K / 2 Seguir con el ZX Spectrum 128K"). Se elige con ENTER o con 1/2; ESC sigue igual. El modelo propuesto es el primero que la cinta dice que funciona (48K, 128K, +2, +3, 16K) o, si no dice ninguno, el 48K. Viene bien ahora que el 128K es el modelo por defecto: hay juegos de 48K que en un 128K se reinician o se cuelgan.
- **Nuevo**: carga instantánea también de los bloques 0x11 ("turbo") que tienen los tiempos de la ROM (±15 %, bytes enteros), como hace JSpeccy: hay TZX que guardan así bloques normales (JSpeccy cita Midnight Resistance 128K). Antes iban por pulsos.
- Comparado con Fuse y JSpeccy: los tres reconocemos los mismos bloques TZX (todos los de la norma 1.20) y las mismas formas de cargar (trampa de la ROM, carga rápida, aceleración de cargadores, arranque y parada automáticos, trampa de SAVE).
- Probado en el PC: un TZX con la cabecera en 0x10 y los datos en 0x11 con tiempos estándar carga al instante; sin cambios en la cinta turbo de prueba (sus bloques turbo de verdad siguen por pulsos), Basil y Cabal. En QEMU (Pi 3, 64 bits): con el 128K, ese TZX (con 0x33 "no funciona en 128K") abre la pregunta, y al elegir el 48K cambia de modelo con la cinta puesta.

## 0.98

- **Cambiado**: el Spectrum por defecto es el 128K (`model=128k` en el `cmdline.txt` de la SD, y también si no se pone `model=`). `model=48k` sigue arrancando en el 48K.
- **Corregido**: la SD generada llevaba `tape=fast` en su `cmdline.txt`; ahora lleva `tape=flash` (carga instantánea), que es el valor por defecto.
- **Cambiado**: en el navegador de bloques de la cinta (F3), la línea "P polaridad" va siempre en amarillo, como las demás teclas que se pueden pulsar (antes, en gris cuando la polaridad era normal).

## 0.97

- **Nuevo**: errores de CRC en la cabecera de un sector, como en Fuse y en el uPD765. En el DSK, ST1 = 0x20 sin ST2 = 0x20 quiere decir que el error está en la cabecera (ID) y no en los datos; ahora la cabecera se graba con el CRC mal y el controlador no da por bueno ese sector (ST1 con DE, ST2 sin DD, sin datos). Antes se tomaba siempre como error en los datos (como MAME): el sector se encontraba y se leía, y una protección que esperara no encontrarlo fallaba.
- **Cambiado**: en los sectores débiles con varias copias en la imagen, solo es débil el tramo entre la primera y la última diferencia entre las copias (como `cpc_set_weak_range` de Fuse). Antes era desde la primera diferencia hasta el final del sector, y lo que viene después salía al azar aunque en el disco es fijo (el mismo problema que tenía Dragon Ninja con una sola copia).
- **Nuevo** (prueba): `test/host/plus3/prot_test.cpp` con un DSK hecho a mano (`prot.dsk`): un sector normal, uno con error en la cabecera y uno débil con tres copias que solo difieren en los bytes 100-119.
- Probado en el PC: la prueba nueva, la del controlador, y Coin-Op Hits, Tomahawk, Ghouls'n Ghosts, The Deep y Dragon Ninja como antes.

## 0.96

- **Corregido**: Dragon Ninja (+3, Speedlock) se quedaba con la pantalla en negro. Su protección lee dos veces el sector 2 de la pista 0 (el sector débil) y comprueba que los primeros 128 bytes sean 0xE5 y que los bytes 304-319 salgan iguales. Hacíamos débil todo el sector desde el byte 256 (como MAME), así que esos bytes salían al azar y el cargador se colgaba a propósito. Ahora se reconoce el Speedlock del +3 como lo hace SAMdisk (`IsSpectrumSpeedlockTrack`): pista de 9 sectores de 512 bytes, el segundo con error de datos y "SPEEDLOCK" en el primero (byte 304 o 176). Si la primera mitad del sector débil es constante, solo cambian 32 bytes desde el 336; si no, el sector entero.
- Probado en el PC: Dragon Ninja carga y llega a su menú. Sin cambios en Coin-Op Hits, Tomahawk, Ghouls'n Ghosts, The Deep ni en la prueba del controlador.

## 0.95

- **Cambiado**: la tarjeta de esxDOS es ahora solo la imagen `hdd/esxdos.img`, y la carpeta pasa a ser una carpeta de importación, `esxdos_i`. Lo que se deja en ella se copia a la imagen al conectar la divMMC y en cada reset (F12, F7), respetando las subcarpetas; cada copia se lee otra vez y se compara con el original, y solo entonces se borra de la carpeta. Lo que no se puede importar se queda en la carpeta, con un aviso que dice cuántos y por qué. Un fichero con el mismo nombre que uno de la tarjeta lo sustituye. Si no cabe, se crea una tarjeta mayor (hasta 2 GB) y se pasa a ella lo que había. Se acaba la sincronización en los dos sentidos (la lista de borrados, la copia periódica de la imagen a la carpeta, los conflictos): era la parte más frágil, ocupaba el doble en la SD y paraba la emulación.
- **Nuevo**: aviso de progreso mientras se importa, con ficheros y megas ("esxDOS: importando 12 de 40 (9 de 20 MB)..."), que se pinta al momento aunque la emulación esté parada.
- **Paso desde la 0.94**: si existe la carpeta `esxdos` y no `esxdos_i`, se renombra y su contenido se importa; un `esxdir=esxdos` en `cmdline.txt` se entiende como `esxdos_i`. Se borra la lista vieja `hdd/esxdos.img.sync`.
- La SD de la versión ya no trae `hdd/esxdos.img` (al actualizar copiando encima machacaría la del usuario, que ahora es el único sitio con sus ficheros): trae `esxdos_i` con el sistema de esxDOS, que entra en la tarjeta la primera vez.
- Probado en QEMU (Pi 3, 64 bits): la carpeta `esxdos` vieja se renombra, se importa (sistema y juegos) y queda vacía, y esxDOS lista los juegos con `.ls`; un juego nuevo en una subcarpeta y otra versión de uno que ya estaba (sustituido); y 21 MB con una tarjeta de 16 MB: pasa a una de 32 MB con todo lo que había, los ficheros idénticos al original y el progreso en pantalla. Si se corta a medias, todo sigue en la carpeta (solo se borra lo ya comprobado).
- Pendiente (siguientes pasos): ver, subir, bajar y borrar ficheros de la tarjeta desde la web, y cargar desde ella en el selector (F1).

## 0.94

- **Corregido**: cambiar de modelo (F7) o hacer un reset (F12) parecía congelar el emulador. Cada vez se sincronizaba entera la carpeta `esxdos` con su imagen (montar la imagen, recorrer los dos lados, copiar, reescribir la lista de sincronización), aunque la divMMC no estuviera conectada: bastaba con que existiera la carpeta, que viene en la SD. Y como F7 va pasando de un modelo a otro, se repetía en cada pulsación. En QEMU, con la carpeta casi vacía, cada F7 tardaba 1,6 s; ahora 0,1 s.
  - Sin la divMMC (ni la imagen montada) ya no se sincroniza nada.
  - De la imagen a la carpeta solo se copia si el Spectrum ha escrito en la tarjeta desde la última vez (`CHardDiskImage` cuenta las escrituras).
  - De la carpeta a la imagen solo si la carpeta ha cambiado: una firma rápida (nombres, tamaños y fechas, sin abrir ficheros ni montar la imagen) se compara con la de la última sincronización. La firma solo se da por buena si la carpeta estaba al día, para que lo que llegue a la carpeta mientras tanto (por ejemplo, por la web) se copie en el siguiente reset.
- Probado en QEMU (Pi 3, 64 bits): F7 responde en 0,1 s con y sin divMMC (antes, 1,6 s sin ella).

## 0.93

- **Nuevo (en pruebas)**: WiFi en la Raspberry Pi 5 (y la 500 y la CM5). Circle la soporta con el mismo chip 43455 y un firmware propio (`brcmfmac43455-sdio.raspberrypi,5-model-b.bin/.txt/.clm_blob`); `build.sh` lo descarga ahora (`make firmware5`) y va en la carpeta `firmware` de la SD. Antes el emulador no daba WiFi a la Pi 5 aunque Circle la soporta. Circle avisa (doc/issues.txt) de que en la Pi 5 la conexión puede ser "delicada" con una pantalla HDMI a la vez.
- **Ojo**: con `net=auto` (por defecto), si hay `wpa_supplicant.conf` en la SD, la Pi 5 usa ahora la WiFi en lugar del cable. Para seguir con el cable: `net=eth`.
- Sin probar en una Pi 5 real todavía (QEMU no emula la Pi 5).

## 0.92

- **Cambiado**: los .MDV (formato "MDVT" de JSpeccy, el que usa el CP/M de LEC) se graban con el mismo tamaño de sector que JSpeccy: 792 bytes en los cartuchos en bruto y 587 (con el flag "convertido de MDR" y la longitud de hueco 10) en los que vienen de un .MDR. Antes la cinta se troceaba en "sectores" de hasta 64 KB: JSpeccy los leía bien, pero al volver a grabarlos escribe siempre 792 o 587 por el número de sectores que leyó, y la cabecera dejaba de cuadrar con los datos.
- **Cambiado**: un cartucho que se metió como .MDV se graba siempre como .MDV. Antes, si todos sus sectores eran de Sinclair, se grababa como .MDR y cambiaba la extensión del fichero. Los cartuchos nuevos y los .MDR siguen igual (.MDR si todo son sectores de Sinclair; si no, .MDV).
- Comprobado con el código de JSpeccy (`Microdrive.java`): la cabecera MDVT 1.0, los flags, la compresión zlib y la codificación de los huecos (tramos de 16 bits alternos, empezando por el estado del primer byte) coinciden. Los .MDV ya se leían y grababan desde que se añadió la Interface 1.
- Probado en el PC: un .MDR pasado a .MDV, leído y vuelto a grabar como lo hace JSpeccy (comprimido, sectores de 587) y leído otra vez da exactamente el mismo cartucho; la prueba de microdrives de `test/host` (`mdv_test`) pasa entera. También se ha puesto al día `recreated_test` (ahora hacen falta 3 pares pulsar/soltar para detectar el teclado).

## 0.91

- **Cambiado**: en la configuración (F1, C), las opciones en gris (las que no se usan con lo elegido, como el teclado Issue 2/3 fuera del 16K y el 48K) ya no se pueden elegir ni cambiar: el cursor se las salta al subir y bajar, al cambiar de página y al abrir la configuración, y si se queda encima de una (al cambiar el modelo) no la deja cambiar. Antes solo cambiaban de color y se podían modificar, lo que llevaba a confusión.
- Probado en QEMU (Pi 3, 64 bits): con el +3 elegido, bajar desde "Modelo" va a "Velocidad" (se salta "Teclado") y subir vuelve a "Modelo".

## 0.90

- **Nuevo**: ficheros de cinta .PZX (Perfect ZX Tape, de pzxtools). Se reproducen directamente, pulso a pulso y con el nivel que dice el fichero: PULS (pulsos, también los de duración 0 que solo cambian el nivel), DATA (bits con sus pulsos, nivel inicial y pulso final), PAUS (pausa con su nivel), BRWS (texto), STOP (parar, también "solo en el 48K"). Los bloques DATA con los tiempos de la ROM (con su tono guía delante) se cargan al instante; los demás, por pulsos, con aceleración. Aparecen en el selector, en la pantalla de unidades, en la web, por TNFS y en las descargas de ZXDB, y en el navegador de bloques (F3) con su título, pulsos, cabeceras, pausas y paradas.
- **Nuevo**: invertir la polaridad de la cinta con la tecla P en el navegador de bloques (F3); el estado se ve en el propio navegador. Vale para cualquier formato y vuelve a la normal al poner otra cinta.
- **Por qué**: Forbidden Planet (Design Design) no cargaba. Su cargador es una copia de LD-BYTES que mide los bits por pares de flancos y depende de la polaridad; el TZX no la guarda (la norma dice que empieza en nivel bajo) y en el original el tono guía del bloque principal tiene 8064 pulsos, no 8063, así que los datos empiezan al revés. El cargador desajustaba medio bit, fallaba la suma a los 261 bytes y volvía a empezar sin fin. Spectrum Computing tiene un .PZX "con la polaridad correcta".
- Comprobado también: Hollywood Poker sí cargaba. El cargador ("UNILOADER BY AIS'93") restaura un snapshot que se quedó esperando una tecla en un PAUSE de la ROM: pantalla negra hasta pulsar una, y entonces sale el menú del juego.
- Probado en el PC: Forbidden Planet desde el .PZX (48K, rápida e instantánea) y desde el .TZX con la polaridad invertida llega a su "Hall of Fame". Sin cambios en la cinta de prueba turbo, Cabal, Basil y Misifu. En QEMU (Pi 3, 64 bits): el .PZX se carga desde F1 y el navegador muestra sus bloques; la tecla P cambia la polaridad.

## 0.89

- **Corregido** (de la 0.86): con la divMMC activada (esxDOS) la carga instantánea solo cargaba el primer bloque y el programa se perdía (Misifu Remeow se quedaba en el bloque 2 de 12). Al mover la trampa a 0x0562, como Fuse, coincidió con uno de los puntos en los que la divMMC pagina su memoria sola al leer la instrucción (esxDOS lo usa para sus cintas). La trampa cargaba el bloque y devolvía un RET, pero la divMMC se quedaba paginada y la vuelta por SA/LD-RET ejecutaba código de esxDOS en lugar de la ROM. Ahora, si la trampa carga el bloque, se anula ese paginado automático (la rutina de la ROM ya no se ejecuta). Sin la divMMC no cambia nada.
- Probado en QEMU (Pi 3, 64 bits) con esxDOS: Misifu Remeow carga entero al instante en 48K y 128K, y en 128K arranca el juego. En el PC, sin cambios en Cabal, Basil y Misifu (instantánea, sin divMMC).

## 0.88

- **Corregido**: al descargar un fichero desde F1 con "Internet (TNFS)" el emulador parecía congelado: el aviso "Descargando..." se mandaba a la pantalla con ShowFrame, que con el núcleo 1 ocupado descarta el cuadro, y no se llegaba a ver. Ahora se muestra al momento (como el de la web, esperando a que esté en pantalla), igual que "Conectando con..." y "Leyendo la lista...".
- **Nuevo**: durante la descarga por TNFS se ve cuánto lleva ("Descargando... 24 KB", cada 8 KB). `CTNFSClient::SetProgressHandler`.
- Probado en el PC con un servidor TNFS mínimo: una descarga de 64 KB avisa a los 8, 16... 56 KB y llega idéntica. El aviso en pantalla usa el mismo camino que el de la web (probado en QEMU en la 0.85); con TNFS real, pendiente.

## 0.87

- **Nuevo**: navegador de bloques de la cinta (F3), como el de Fuse (Media, Tape, Browse). Lista lo que hay en la cinta: cabeceras de la ROM ("Programa:", "Bytes:" con nombre, dirección y longitud) y sus datos, bloques turbo, tonos, pulsos, datos puros, grabaciones directas y CSW, pausas, bucles, saltos, selecciones... y destaca en otro color los textos y grupos del TZX ("Level 2") y los "PARAR LA CINTA", que es lo que sirve para orientarse en una multicarga. El ">" marca dónde está la cinta. ENTER la deja parada al principio del bloque elegido (la carga instantánea lee desde ahí; en rápida o real arranca sola cuando el programa la lee). Cursores, RePág/AvPág, Inicio (= rebobinar) y Fin. Sustituye al antiguo "rebobinar" de F3.
  - `CTape::GetBlockInfo` describe cada bloque (sin textos, para que las pruebas del PC sigan compilando sin la parte de idiomas) y `CTape::SeekBlock` coloca la cinta; el menú solo pide las líneas visibles (hay TZX con miles de bloques).
- Probado en el PC: la descripción de los 27 bloques de Basil y los 42 de The Coin-Op. En QEMU (Pi 3, 64 bits): F3 con Basil, bajar a "Level 2" y ENTER; al volver a abrirlo la cinta está en el bloque 9 de 27.

## 0.86

Revisado cómo carga Fuse los TZX (libspectrum, `tape.c`) y comparado con nuestra cinta. Dos diferencias, las dos con Basil the Great Mouse Detective (Gremlin, 48/128K):

- **Corregido**: los bloques de parada de la cinta (0x20 con pausa 0, y 0x2A en el 16K y el 48K) contaban como una parada manual y la cinta ya no arrancaba sola. Las multicargas los ponen entre niveles: en 128K el cargador seguía esperando y la cinta se quedaba en "Bloque 9 de 27". Ahora, como en Fuse ("los bloques de parada siguen permitiendo el arranque automático, porque las multicargas los usan entre niveles"), el cargador puede volver a arrancarla. Al llegar al final de la cinta, no.
- **Corregido**: la polaridad de la señal. En libspectrum, el primer pulso de un bloque que va tras una pausa (o al empezar la cinta) es nivel bajo, y los demás cambian de nivel. En la nuestra ese primer pulso cambiaba desde el nivel bajo de la pausa y salía alto, así que todo el bloque iba invertido. A los cargadores que miran flancos (la ROM y casi todos) les da igual; el de Basil mide cada bit como nivel bajo más nivel alto, juntaba medio bit con medio del siguiente y no cargaba nada (franjas verdes y negras y luego el borde verde). Igual que libspectrum: el bloque CSW sigue el nivel que había y los datos generalizados respetan su polaridad.
- Comprobado sin cambios: una pausa de 0 no cambia el nivel; la pausa de cola es un flanco y luego nivel bajo; 0x2A según el modelo (16K y 48K); al final de la cinta no se arranca sola.
- **Corregido**: Basil the Great Mouse Detective (Gremlin, TZX) no cargaba en carga instantánea: tras la primera parte, el cargador seguía leyendo los bloques de los niveles sin terminar. Su cargador mide cada bit como un ciclo completo (espera a que la señal suba y luego a que baje), así que depende de la polaridad. Tras cargar al instante los bloques de la ROM, la cinta se queda parada y el cargador la arranca solo en el bloque turbo; `CTape::Play` marcaba "primer pulso en nivel bajo" después de generar ese primer pulso, que salía alto, y todo el bloque iba con la polaridad invertida (cada medida juntaba medio bit 0 con medio bit 1). Ahora se marca antes. En carga rápida y real ya funcionaba, porque la cinta llega a ese bloque sonando seguida.
- **Cambiado**: la carga instantánea funciona ahora como en Fuse (`tape_traps.c`): una sola trampa en 0x0562, donde la ROM ya ha hecho DI, ha puesto el borde y ha apilado la vuelta por SA/LD-RET (0x053F); se sale con un RET, así que la vuelta pasa por SA/LD-RET (borde y EI) como en una carga real, o vuelve directamente a quien llamó si se entró a media rutina (Cabal). Los registros quedan como los deja Fuse: A = paridad, F de CP 1 (acarreo = bien), B = 0xB0 o 0x00, C = 0x01, H = paridad, L = último byte, DE e IX avanzados, AF' = 0x0145. Antes la trampa estaba en 0x0556, volvía sin pasar por SA/LD-RET y no tocaba B, C, H, L ni AF'.
- Probado en el PC (primera parte, carga rápida): Basil carga en 48K y 128K hasta "STOP TAPE - ENTER TO START". Sin cambios en Cabal (rápida e instantánea), La Abadía del Crimen, la cinta de prueba turbo (`test/host/accel`, cuatro modos), las Turtles y la multicarga de The Coin-Op.
- Probado en el PC (segunda parte, carga instantánea): Basil en 48K y 128K, carga instantánea y rápida, llega a "STOP TAPE - ENTER TO START" y la cinta se para en el bloque 8. Sin cambios en la cinta de prueba turbo (los cuatro modos), Cabal (instantánea y rápida), La Abadía del Crimen, The Coin-Op (instantánea: elegir jugadores, cargar el nivel A y jugar) ni las otras Turtles.

## 0.85

- **Nuevo**: aviso en pantalla cuando la red para la emulación. La red y la emulación comparten el núcleo 0, y algunas operaciones se hacen de una vez: leer o escribir un fichero entero en la SD desde la web, o buscar y descargar de ZXDB (HTTPS, con el saludo TLS). Antes la imagen se quedaba congelada sin decir nada. Ahora la propia tarea de red pide el aviso y el kernel lo pinta y lo muestra al momento (con el núcleo 1, esperando a que se vea): "Red: enviando juego.tap (512 KB)", "Red: guardando juego.tap (512 KB)" (desde 32 KB), "Red: buscando en ZXDB...", "Red: descargando de ZXDB...". Se queda unos segundos cuando la emulación sigue.
- **Quitado**: el atajo Alt+I (teclado Issue 2/3). Se cambia en la configuración (F1, C, página 1), que lo pone en gris fuera del 16K y el 48K. Con esa línea menos en la ayuda vuelve a caber la línea "Red:" (con la IP de la WiFi) aunque haya una cinta puesta: antes la tapaba la línea "Bloque N de M".
- Probado en QEMU (Pi 3, 64 bits): la ayuda con una cinta puesta muestra la línea de la red; con una operación de red simulada de 3 segundos (en un kernel de prueba) el aviso aparece al empezar y se queda después. Con la WiFi real, pendiente.

## 0.84

- **Corregido**: The Deep (U.S. Gold, +3) volvía al menú del +3 al arrancar. Su código de arranque apaga el motor de la disquetera al cambiar la paginación con 0x1FFD y enseguida pide un sector a +3DOS. En el emulador la unidad pasaba al instante a "no preparada" y la lectura fallaba (ST0 0x48). En una disquetera real el disco sigue girando un momento; Fuse mantiene la unidad preparada 1,5 vueltas (300 ms) tras apagar el motor. Ahora se hace igual (`plus3disk.cpp`): el disco sigue girando 300 ms y, si se vuelve a encender antes, sigue sin esperar a que arranque.
- Probado en el PC: The Deep arranca y llega a su menú y a la tabla de récords. Sin cambios en Coin-Op Hits (Out Run carga), Tomahawk (pide el código), Ghouls'n Ghosts (carga la pantalla), la prueba del controlador y SAVE en un disco en blanco.

## 0.83

- **Corregido**: las multicargas podían quedarse esperando un nivel para siempre (Teenage Mutant Hero Turtles - The Coin-Op, 128K). Tras cargar el bloque principal, el juego se queda en sus menús (jugador, controles, empezar), que leen el teclado miles de veces por cuadro. El emulador lo tomaba por un cargador: en carga rápida la cinta no se paraba y seguía por los bloques de los niveles mientras se elegía en el menú; en instantánea, la arrancaba sola. Al empezar la partida y pedir el nivel A, la cinta ya estaba en otro sitio.
  - Ahora el arranque y la parada automáticos cuentan solo las lecturas del puerto 0xFE "de cargador": el mismo IN que la anterior (mismo PC y puerto) con un contador que ha avanzado una unidad entre las dos (B en la ROM y la mayoría; E en La Abadía del Crimen). Es el bucle que espera un flanco y mide lo que tarda; Fuse los detecta igual. Un menú que espera una tecla repite el IN sin contar nada, y el teclado se lee fila a fila (cambia el puerto). Siguen contando los flancos acelerados.
  - Más de 500 lecturas así por cuadro arrancan la cinta (antes, 1000 lecturas cualesquiera); menos de 20 durante 2 s la paran (antes, menos de 200 cualesquiera y sin seguir los flancos).
- Probado en el PC: The Coin-Op (TAP de 42 bloques) en 128K, carga rápida e instantánea: carga, se elige jugador y controles, la cinta se queda parada en el bloque 10 y, al empezar, carga el nivel A y se juega. Sin cambios en Cabal (rápida e instantánea), La Abadía del Crimen, la cinta de prueba turbo (`test/host/accel`, los cuatro modos) ni las otras Turtles (que no son multicarga; `TTURTLES.TAP` ya no sigue reproduciendo sus dos últimos bloques, que el juego no usa).

## 0.82

- **Nuevo**: en la configuración (F1, C), las opciones que no se usan con lo elegido salen en gris, y al ponerse encima se lee "No se usa con lo elegido ahora". Se pueden cambiar igual. Reglas: teclado Issue 2/3 solo en el 16K y el 48K (sugerencia de José Luis Sánchez; la emulación ya lo ignoraba en los demás); idioma y disco duro del +3e solo en el +3e; aceleración de cargadores sin carga real; formato de SAVE con SAVE a la SD; sonido por GPIO12 con la salida PWM; imagen en el núcleo 1 solo en las Pi 2, 3 y 4; máscara, puerta de enlace y DNS con IP fija; IP, zona horaria, horario de verano y HTTPS con red; inversión y GPIO del casete con casete real; carpeta de esxDOS con la divMMC.
- Comprobado: Tomahawk (+3, Digital Integration) también se quedaba colgado por lo mismo que se corrigió en la 0.81 (una pista sin formatear dejaba la emulación casi parada: en el PC, 9 segundos de parón al poco de arrancar). Con la 0.81 carga y llega a pedir el código de la tarjeta de seguridad.
- Probado en QEMU (Pi 3, 64 bits): con el 48K, el idioma y el disco duro del +3e en gris; al elegir el +3, también el teclado, con el aviso.

## 0.81

- **Corregido**: la emulación se volvía cientos de veces más lenta (parecía colgada) cuando un programa intentaba leer una pista sin formatear del disco del +3. En una pista sin transiciones, el PLL del controlador la recorría entera (unas 12500 vueltas de bucle) en cada bit. Ahora se recuerda que la pista está vacía hasta que cambia algo (cabeza, cara, motor o una escritura). Afectaba a cualquier cargador que mire una pista vacía, por ejemplo para una protección.
- **Corregido** (de la 0.80): al contar el hueco 3 que dice la imagen, las pistas llenas se alargaban sin necesidad y los bytes llegaban hasta un 3 % antes de tiempo, con errores de desbordamiento en algunos cargadores. Ahora se hace como en un disco real: si la pista no cabe en 6250 bytes, primero se acortan los huecos 3, luego el preámbulo (sin IAM), y solo si aun así no cabe se alarga la pista (Coin-Op Hits).
- Probado en el PC: Coin-Op Hits sigue arrancando y Out Run carga; la prueba del controlador (formatear, leer, escribir, guardar y recargar) bien. Ghouls'n Ghosts (U.S. Gold) arranca y carga la pantalla, pero la imagen que hay no trae las pistas 18 a 20 y su cargador se queda reintentando la 18 (ahora sin ralentizar la emulación): hace falta otro volcado.

## 0.80

- **Corregido**: Coin-Op Hits (+3) no cargaba. Sus pistas tienen un sector de 8K con 6305 bytes útiles, grabado con las celdas de bit algo más estrechas, y la imagen EDSK trae tres copias de cada uno (https://simonowen.com/samdisk/sys_plus3/). Tres fallos en `dskimage.cpp`:
  - las copias de un sector se escribían todas seguidas, como un sector de 24K: ahora va una sola;
  - un sector de 8K no cabe en una pista: ahora solo se graba lo que hay realmente en el disco (hasta donde difieren las copias o, si son iguales, como mucho 6352 bytes, con las celdas un 5 % más estrechas) y el resto se lee dando la vuelta, como en el original. Antes la pista salía tan larga que sus celdas quedaban demasiado estrechas para el PLL y no se leía nada;
  - la longitud de la pista no contaba el hueco 3: la pista 0 (10 sectores de 512 bytes con hueco 47) no cabía y se cortaba el último sector. Los discos normales del +3 caben igual que antes.
- **Cambiado**: en los sectores débiles con varias copias en la imagen (Speedlock), va una sola copia en la pista, débil desde el byte en que las copias empiezan a diferir (antes, desde el byte 256 y con todas las copias seguidas).
- **Nuevo**: al volver a abrir el selector (F1, ENTER), el cursor sigue en el último fichero elegido y a la misma altura, en vez de volver al principio.
- **Corregido** (pruebas): las pruebas del +3 de `test/host/plus3` cargan ahora la ROM del +3 (`roms/plus3-0..3.rom`); sin ella el modelo arrancaba como un 128K.
- Probado en el PC: Coin-Op Hits arranca, sale su menú y Out Run carga hasta su menú. La prueba del controlador (formatear, leer, escribir, guardar y recargar un DSK) y SAVE en un disco en blanco del +3 siguen bien. En QEMU (Pi 3, 64 bits): el cursor del selector se queda en el fichero elegido. Sin probar con un disco Speedlock (no hay ninguno aquí).

## 0.79

- **Corregido**: La Abadía del Crimen (Opera Soft, 128K, TZX) no cargaba: se quedaba en un bucle a mitad del segundo bloque turbo. No era su cargador: era la parada automática de la cinta. Ese cargador pasa casi todo el tiempo en bucles de retardo y lee el puerto 0xFE unas 150 veces por cuadro; con menos de 200 durante 2 segundos, el emulador lo tomaba por inactivo y paraba la cinta, y al volver a leer la cinta arrancaba otra vez desde el principio del bloque. Ahora también cuenta si el programa sigue la cinta: si al menos la mitad de los flancos de un cuadro los ve alguna lectura (un cargador lee en cada pulso), no se para. Un programa que solo mira el teclado ve muy pocos, así que la cinta se sigue parando sola cuando acaba la carga.
- Probado en el PC: La Abadía del Crimen carga entera en carga real, rápida e instantánea y arranca la introducción; la cinta se para sola al acabar. Sin cambios en la cinta de prueba (`test/host/accel`, los tres modos con y sin aceleración) ni en Cabal (rápida e instantánea). Su cargador (contador en E, `AND 40h; CP D`) no es de los que acelera Fuse: en carga rápida tarda lo que tarda la cinta, a la velocidad máxima del emulador.

## 0.78

- **Corregido** (Pi 5): sin scanlines la imagen salía pequeña, arriba a la izquierda. El firmware de la Pi 5 no escala el framebuffer a la pantalla, como hacen la Pi 1 a 4. En la Pi 5 el framebuffer va siempre a la resolución de la pantalla y la CPU amplía la imagen: sin scanlines, con un aumento fraccionario (vecino más próximo) que llena la pantalla igual que antes la GPU; con scanlines, sin escala de la GPU (r = aumento total). En los demás modelos no cambia nada.
- Probado en una Pi 5 real (por Carlos, 0.77): una sola imagen; con scanlines, imagen a pantalla completa, 50,02 c/s y 7,9 ms del núcleo 0 con la imagen. La pantalla verde de la 0.76 al activar las scanlines ya no aparece.
- **Cambiado** (publicación en GitHub): `README.md` pasa a ser el README en inglés (portada del repositorio) y el de español pasa a `README_es.md`, que sigue siendo la referencia. Los dos tienen ahora las secciones de asistencia de IA, licencia (con las de terceros y las ROMs) y aviso legal.
- **Quitado**: las ROMs del Multiface (© Romantic Robot) ya no se incluyen ni en el código ni en la SD, porque no hay permiso para distribuirlas (se ha pedido a Romantic Robot). La emulación sigue: basta con poner en `roms/` el volcado de un Multiface propio; sin ellas, F5 hace una NMI normal. `build.sh` las copia a la SD solo si están.
- **Nuevo**: `build.sh` deja en `firmware/LICENCE.txt` de la SD las licencias del firmware WiFi de Broadcom y Cypress, sacadas del mismo commit de firmware-nonfree.
- Probado en el PC: los tamaños calculados para varias pantallas (720p, 1080p, 1200p, 4K, 1024 × 768) y la prueba existente de las scanlines sin cambios.

## 0.77

Primeras pruebas en una Raspberry Pi 5 real (por Carlos, con la 0.76) y sus arreglos:

- **Corregido** (Pi 5): la imagen salía doble, una encima de otra y en pequeño. El firmware de la Pi 5 no hace el cambio de página (`SetVirtualOffset`) y mostraba el framebuffer virtual entero con las dos páginas. En la Pi 5 se usa ahora una sola página; puede verse algún corte de imagen.
- **Corregido** (Pi 5): con el monitor en el HDMI1 (el conector más alejado de la alimentación), el teclado parecía pulsar teclas sin parar. El sonido HDMI de Circle solo funciona por el HDMI0, así que se pasaba al PWM, que en la Pi 5 sale por el GPIO12 y el GPIO13, y el GPIO13 es una columna del teclado por GPIO. En la Pi 5 ya no se usa el PWM si está el teclado por GPIO (`gpiokbd=0` para usarlo). Sonido: por el HDMI0 o una tarjeta USB.
- **Cambiado**: el framebuffer se comprueba al crearlo (tamaño del búfer y paso de línea devueltos por el firmware); si no cuadra no se dibuja en él, y si el de las scanlines no se puede crear se sigue sin scanlines.
- **Corregido** (Pi 5): con las scanlines la pantalla se quedaba verde con líneas y el emulador se congelaba. Con una sola página el framebuffer es la mitad de grande y ahora se comprueba antes de dibujar (confirmado en la Pi 5 con la 0.77).
- Ayuda de `sounddev=` al día en los dos idiomas (HDMI0, PWM en la Pi 5, sonido USB también en la Pi 5).
- Probado: compilación de los siete kernels y QEMU (Pi 3, 64 bits; el cambio de una página solo afecta a la Pi 5). En la Pi 5, pendiente.

## 0.76

- **Nuevo**: aceleración de cargadores, adaptada de Fuse (`tapeaccel=1`, por defecto; también en la configuración, página de cintas, "Acelerar cargadores"). Si el IN que espera cada flanco de la cinta está en un bucle conocido (el de la ROM y sus copias en RAM, Microprose, Alkatraz, Digital Integration, los de búsqueda; como Fuse, no el de Technician Ted), en los datos se adelanta el flanco y se dejan B, C y el acarreo como si el bucle lo hubiera encontrado. El tono guía, los sincronismos y las pausas se emulan normalmente. Solo en carga rápida e instantánea. La cinta sabe ahora si cada pulso es de un bit 0 o de un bit 1 (bloques 0x10 y 0x11).
- **Nuevo**: la carga instantánea también coge la rutina de la ROM cuando se entra a media rutina, en 0x0562, con el flag y el modo LOAD en AF'. Cabal (Ocean, 128K) lo hace con un `CALL 0x0562` desde su cargador; antes cargaba en modo rápido y ahora al momento.
- **Corregido**: con la aceleración, la parada automática de la cinta (pocas lecturas del puerto durante 2 s) paraba la cinta a mitad de un bloque, porque un cargador acelerado lee mucho menos. Los flancos acelerados cuentan ahora como actividad.
- **Nuevo**: prueba en el PC `test/host/accel` (cinta generada con un bloque estándar y uno turbo cargado por una copia reubicada de la rutina de la ROM con otros tiempos; compara byte a byte lo cargado).
- Probado en el PC: Cabal en carga rápida pasa de 31.031 a 12.455 cuadros emulados con el juego funcionando, y en instantánea carga al momento. La cinta de prueba carga sin errores en los tres modos, con y sin aceleración (rápida: de 4.710 a 2.213 cuadros). Pruebas de grabación con SAVE sin cambios. En una Pi real, pendiente.

## 0.75

- **Corregido**: el tiempo del núcleo 0 en Alt+D se disparaba al salir de un menú (más de 200 ms) y tardaba en bajar: el tiempo con el menú abierto contaba como si fuera un cuadro. Era solo la cifra. Ahora la medida empieza de nuevo al volver, igual que la de c/s.
- Probado en QEMU (Pi 3, 64 bits): Alt+D, F1 abierto 10 s y cerrar: antes 20,9 ms un segundo después, ahora 8,6 ms.

## 0.74

- **Corregido**: la medida de c/s (ayuda F1 y Alt+D) salía muy baja después de tener abierto un menú, por ejemplo 0,8 c/s al volver a abrir F1 tras cerrarlo. Con el menú abierto no se emula, pero ese tiempo contaba dentro de la medida como cuadros perdidos. Era solo la cifra: la emulación iba a su velocidad. Ahora la medida empieza de nuevo al volver del menú o de la pausa. Venía de versiones anteriores.
- Probado en QEMU (Pi 3, 64 bits): F1 abierto 12 s, cerrar y volver a abrir: antes 0,84 c/s, ahora 50,16 c/s.

## 0.73

- **Corregido**: la pantalla de unidades abierta desde la ayuda (F1 y U) no mostraba lo montado desde el selector (por ejemplo un disco duro con F1 y ENTER): enseñaba las unidades como estaban la última vez que se abrió con Alt+U. Ahora las dos formas de abrirla las ponen al día. Venía de versiones anteriores, no es de los 64 bits.
- Probado en una Pi real (por Carlos, 0.72): las scanlines ya funcionan en 64 bits; la imagen tarda 12,2 ms con scanlines (antes 44,9) y 0,3 ms sin ellas.
- Probado en QEMU (Pi 3, 64 bits): disco duro montado al arrancar, F1 y U lo muestran.

## 0.72

- **Corregido**: en la 0.71, al activar las scanlines la pantalla se quedaba en negro y la Pi se colgaba. GCC convertía los bucles de copia al framebuffer en escrituras de 128 bits (`str q` en 64 bits, `vst1` en 32) sobre direcciones que no estaban alineadas a 16 bytes, y en la memoria de dispositivo eso es un fallo de alineación (QEMU no lo comprueba, por eso allí funcionaba). Ahora cada escritura al framebuffer es de 64, 32 o 16 bits según la alineación, y se ha comprobado en el código generado por los dos compiladores que no queda ninguna de 128 bits ni de bytes sueltos.
- Probado en QEMU (Pi 3, 64 bits): scanlines con borde completo, estándar y sin borde. En una Pi real, pendiente.

## 0.71

- **Corregido**: en 64 bits la ampliación de la imagen tardaba unas 4 veces más que en 32 (45 ms frente a 12 ms con scanlines y borde completo a 1080p, medido con Alt+D en una Pi real). El framebuffer es memoria de dispositivo, sin caché, y el `memcpy` de Circle en 64 bits copia byte a byte si origen o destino no están alineados a 8 bytes, como pasaba con la posición de la imagen. Ahora la imagen empieza siempre en una posición alineada a 8 bytes y las líneas se copian con una rutina propia que nunca baja de 16 bits.
- **Cambiado**: con scanlines, cada línea se prepara en RAM normal (con caché) y al framebuffer solo se escribe; antes se leían líneas del propio framebuffer para repetirlas y oscurecerlas, que es aún más lento. Mejora también en 32 bits.
- Probado en QEMU (Pi 3, 64 bits, pantalla de 1920 × 1080 simulada): scanlines con borde completo, estándar y sin borde (aumento fraccionario y entero). La medida de tiempos, pendiente en una Pi real.

## 0.70

- **Nuevo**: versión de 64 bits (AArch64) con el mismo código que la de 32 bits. Kernels nuevos: `kernel8.img` (Pi 3 y Zero 2 W), `kernel8-rpi4.img` (Pi 4 y 400) y `kernel_2712.img` (Pi 5 y 500, sin probar).
- **Cambiado**: `config.txt` arranca en 64 bits la Pi 3, Zero 2 W, 4 y 400; la Pi 1, Zero y Pi 2 siguen en 32 bits. Los kernels de 32 bits de la Pi 3 y 4 siguen en la SD como alternativa.
- **Cambiado**: `build.sh` compila por objetivos (`1`, `2`, `3-32`, `4-32`, `3-64`, `4-64`, `5`) con los dos toolchains, y copia a la SD el `armstub8-rpi4.bin` y los ficheros de la Pi 5 (`bcm2712*.dtb`, `overlays/bcm2712d0.dtbo`).
- **Nuevo**: `QEMU=1 ./build.sh` compila kernels de prueba para QEMU (con `ZX_QEMU`) en la carpeta `qemu/`.
- Probado en QEMU (Pi 3, 64 bits): arranque con la imagen en el núcleo 1, teclado USB, ayuda F1, configuración, 48K, +3, esxDOS sobre carpeta (crea y monta la imagen) y el `cmdline.txt` completo. En QEMU (Pi Zero, 32 bits): arranque y ayuda F1 con el toolchain y el Circle nuevos.

## 0.61

- **Nuevo**: en la configuración, las opciones que antes solo se podían escribir en `cmdline.txt`:
  - página 6: IP fija, máscara, puerta de enlace y DNS (se escriben con los números y el punto, y se comprueba que son IPs válidas), comprobar HTTPS y GPIO del casete;
  - página 7, nueva, "ROMs y carpetas": la ROM de cada máquina, del Interface 1, de esxDOS y la flash de la Spectranet (de la carpeta `roms`, solo las del tamaño correcto), y la carpeta de esxDOS.
- Probado en QEMU: escribir una IP y guardarla en `cmdline.txt`, el aviso de IP no válida y la página nueva.

## 0.60

- **Nuevo**: pantalla de unidades (F1 y luego U, o Alt+U): cinta, discos A: y B:, disco duro, cartucho del Interface 2 y microdrives 1 a 8, con el fichero metido en cada una. ENTER monta (el selector solo muestra los ficheros de esa unidad) y SUPR vacía. Por fin se puede meter un disco en la unidad B: desde el selector.
- **Nuevo**: "Disco duro al inicio" en la configuración (página 1), con las imágenes de la carpeta `hdd`; se guarda como `hdd=`.
- Probado en QEMU: montar un disco en B:, vaciarlo, el selector filtrado y la nueva opción.

## 0.59

- **Nuevo**: en la tarjeta de esxDOS sobre carpeta, los borrados se reflejan en los dos lados. Lo que borra el Spectrum desaparece de la carpeta; lo que se borra en la carpeta, de la imagen en el siguiente reset. Se apoya en la lista `hdd/esxdos.img.sync`; sin ella, o con un lado vacío, no se borra nada.
- Probado en QEMU: un fichero borrado en la carpeta desaparece de la imagen al arrancar, y uno borrado con `.rm` desaparece de la carpeta.

## 0.58

- **Quitado**: el menú de periféricos de la ayuda (tecla P). Los periféricos se conectan con Alt+T/X/L o en la página 5 de la configuración.
- **Cambiado**: el pie de la ayuda dice "ENTER cintas  C configuración  A acerca".
- **Documentación**: la guía de uso y pruebas, al día; `README.en.md`, `cmdline-en.hlp` y los `README.txt` en inglés; el README en español, revisado.

## 0.57

- **Nuevo**: modelo del altavoz del beeper tomado de Fuse (ULA, altavoz interno del 48K y televisor). Opción `speaker=auto|beeper|tv|off` ("Altavoz" en la configuración); en automático, el 16K/48K usan el altavoz interno y los demás el televisor.
- **Cambiado**: "Filtro del beeper" pasa a llamarse "Quitar pitido 1 bit".

## 0.56

- **Nuevo**: agradecimiento a Jordi (jbinary.itch.io) en los créditos.

## 0.55

- **Nuevo**: paso bajo de 4.º orden en 6 kHz para el beeper (`beeperfilter=`). Quita el pitido fijo de las rutinas de sonido de 1 bit (la portadora de unos 16 kHz de *We Are Vocoders* baja unos 50 dB).

## 0.54

- **Corregido**: al guardar la configuración (G) se escribían los valores del momento de nueve opciones (modelo, carga de cintas, borde…) en lugar de los del fichero. Ahora solo pasa a la configuración lo que se cambia con las teclas (F4, F6, F7, F9, F10, Alt+B/S/I/T/X/L), y la configuración vuelve a leer `cmdline.txt` al abrirse si no hay cambios pendientes.

## 0.53

- **Corregido**: la salida de sonido elegida (por ejemplo el jack) aparecía siempre como HDMI y se perdía al volver a guardar: Circle se queda la opción `sounddev`. La configuración lee ahora sus valores del propio `cmdline.txt`.

## 0.52

- **Nuevo**: en la Pi 2, 3, 4 y Zero 2 W, la ampliación de la imagen va en el núcleo 1 (`multicore=`, "Imagen en el núcleo 1"). Circle se compila con multinúcleo para esos modelos.
- **Nuevo**: Alt+D muestra el tiempo de cada cuadro en el núcleo 0 y en el núcleo 1.
- **Cambiado**: la carpeta `locales` ya no está en `excluir.txt` (se ve en la web).
- Probado: el reparto entre núcleos en el PC con hilos; falta en una Pi real.

## 0.51

- **Nuevo**: idiomas. Español (dentro del programa) e inglés (`locales/en.json`); `lang=` e "Idioma" en la configuración, con cambio al momento; también la web. Se pueden añadir idiomas copiando el JSON.
- **Nuevo**: fuente con las letras de Europa (acentos, ñ, ç, ¡¿, «», €, ß, ø y las más usadas de Europa central); el español lleva ya tildes, y los nombres de fichero de la SD se ven con sus acentos.
- **Nuevo**: `tools/i18n/actualizar.py`, que mantiene al día las traducciones en cada compilación.

## 0.50

- **Nuevo**: el autor, Shaeon (Carlos Palmero), en los créditos y el README.

## 0.49

- **Nuevo**: pantalla de licencia y créditos (F1 y luego A).

## 0.48

- **Corregido**: la Pi se congelaba al conectar la Spectranet a 127.0.0.1. Las conexiones TCP se hacen ahora sin parar la emulación, con un límite de unos 10 s, y las direcciones locales fallan al momento.
- **Nuevo**: reiniciar la Pi desde la configuración (R), con confirmación y aviso de cambios sin guardar.
- **Nuevo**: `RTC.SYS` propio para esxDOS, que le da la hora de la Pi (NTP).
- **Corregido**: nombres raros en el navegador de esxDOS (letras acentuadas que esxDOS pintaba como palabras del BASIC).

## 0.47

- **Nuevo**: RS232 del Interface 1, por el puerto TCP 2323 de la Pi (o a `saves/rs232.txt`).
- **Nuevo**: red ZX Net del Interface 1, por UDP 5050 entre Pi de la red local (emulación de SpectRusty).
- Probado: los dos en el PC; falta en la Pi.

## 0.46

- **Nuevo**: memoria LEC en los modelos de 128K, desde 48 BASIC.

## 0.45

- **Nuevo**: bloques TZX 0x26/0x27 (llamada a secuencias) y 0x28 (selección, con una lista para elegir qué cargar).

## 0.44

- **Nuevo**: la tarjeta de esxDOS es una carpeta de la SD (`esxdos/`, `esxdir=`); el emulador mantiene la imagen al día en los dos sentidos.
- **Corregido**: el sector de arranque de las imágenes FAT16 que crea el emulador, que colgaba a esxDOS.

## 0.43

- **Nuevo**: configuración desde el emulador (F1 y luego C), en seis páginas, con guardado en `cmdline.txt` (G).

## 0.42

- **Nuevo**: Alt+T y Alt+X conectan y desconectan la Spectranet y la divMMC; menú de periféricos en la ayuda (P, quitado en la 0.58).

## 0.41

- **Nuevo**: tarjeta de red Spectranet, con el firmware oficial ya instalado y su DHCP contestado por el emulador.

## 0.40

- **Nuevo**: `excluir.txt`, para ocultar ficheros y carpetas en el selector y la web.
- **Nuevo**: aviso al arrancar de las opciones mal escritas en `cmdline.txt`.

## 0.39

- **Corregido**: el borde enorme con scanlines se veía mucho más pequeño; ahora usa un aumento fraccionario a resolución nativa en la Pi 2, 3 y 4.

## 0.38

- **Corregido**: el CP/M 3.0 de la imagen "Mar Océana" no arrancaba: la tarjeta SD emulada pasa a ser SDSC o SDHC según el tamaño, con CRC real.
- **Nuevo**: menú de Mar Océana sin las opciones del ZX-Uno y controlador del CP/M para SDHC (experimental).

## 0.37

- **Nuevo**: divMMC con esxDOS 0.8.9, y los puertos de la tarjeta de la divMMC también en el +3e.

## 0.36

- **Corregido**: letras borrosas sin scanlines (`scaling_kernel=8` en `config.txt`) y tamaño de la imagen.

## 0.35

- **Nuevo**: los cursores y RePág/AvPág se repiten al mantenerlos pulsados en los menús.

## 0.34

- **Corregido**: "no arranca el chip WiFi": la WiFi se compilaba con otra versión de una estructura de FatFs.

## 0.33

- **Corregido**: cuelgue al abrir el selector con la red caída.

## 0.32

- **Cambiado**: con el teclado del +3, la entrada de cinta va por el GPIO15.

## 0.31

- **Nuevo**: entrada de cinta desde un casete real, por un GPIO (con un LM393) o por una tarjeta de sonido USB (Pi 4/400), con grabación a TZX (Alt+G).

## 0.30

- **Nuevo**: teclado real del +2A/+2B/+3 por GPIO (13 × 11, `gpiokbd=plus3`).

## 0.29

- **Nuevo**: servidores TNFS en el selector, para recorrer y descargar programas.

## 0.28

- **Cambiado**: solo las ROMs del 48K y del 128K van dentro del emulador; las demás se cargan de la carpeta `roms` de la SD.

## 0.27

- **Nuevo**: Spectrum +3e con disco duro en una tarjeta ZXMMC, y ROMs propias para todas las máquinas.

## 0.26

- **Nuevo**: Spectrum +3 con disquetera y discos `.DSK` (controlador uPD765 de MAME), protecciones incluidas.

## 0.25

- **Nuevo**: red por WiFi o cable, servidor web para gestionar la SD, hora por NTP y descarga de programas de ZXDB por HTTPS.

## 0.24

- **Nuevo**: diagnóstico del teclado (Alt+D).
- **Corregido**: lentitud en algunos casos.

## 0.23

- **Corregido**: los cartuchos de microdrive se grababan cada vez que se paraba el motor y podían atascar la emulación; ahora, tras 2 s parado y como mucho cada 3 s.

## 0.22

- **Corregido**: en el CP/M de LEC, la tecla de borrar del PC borra de forma limpia (en lugar del "rubout" del CP/M).

## 0.21

- **Corregido**: el ritmo del emulador dependía del audio (teclado demasiado sensible).
- **Nuevo**: test de velocidad (`utils/speedtest.tap`).

## 0.20

- **Cambiado**: el proyecto pasa a llamarse ZXMetalCircle.
- **Corregido**: las scanlines (la GPU no estira la imagen como se suponía).

## 0.19

- **Nuevo**: memoria LEC de 528 KB y el CP/M 2.2 de LEC sobre microdrives.

## 0.18

- **Nuevo**: Interface 1 con hasta 8 microdrives (.MDR y .MDV).

## 0.17

- **Nuevo**: grabar snapshots en .SNA y .SZX (`snapformat=`).

## 0.16

- **Nuevo**: tamaño del borde seleccionable (sin borde, estándar, completo, enorme).

## 0.15

- **Nuevo**: cartuchos del Interface 2 (.ROM).

## 0.14

- **Nuevo**: teclado Issue 2/3 y velocidad de emulación x1, x2, x4 y x10.

## 0.13

- **Nuevo**: reparto estéreo del AY (ABC, ACB, BAC, mono) y sonido de la carga.

## 0.12

- **Nuevo**: ULAplus (64 colores).

## 0.11

- **Nuevo**: modelos 16K y +2 gris.

## 0.10

- **Nuevo**: snapshots .Z80, .SNA y .SZX (carga) y .Z80 (grabación, F8).

## 0.9

- **Nuevo**: bloques TZX 0x18 (CSW) y 0x19 (datos generalizados), y grabación con SAVE en la SD.

## 0.8

- **Nuevo**: teclado Recreated ZX Spectrum por USB.

## 0.7

- **Nuevo**: Multiface One, 128 y 3.

## 0.6

- **Nuevo**: ratón Kempston con un ratón USB.

## 0.5

- **Nuevo**: mandos USB (hasta 4), con el esquema de ZXBaremulator.

## 0.4

- **Nuevo**: audio por GPIO12 y botón de NMI por GPIO.

## 0.3

- **Nuevo**: teclado real y dos joysticks Atari por GPIO (montaje de pastbytes).

## 0.2

- **Nuevo**: cintas TZX y teclas de función reorganizadas al estilo de ZXBaremulator.

## 0.1

- Primera versión: ZX Spectrum 48K, 128K y +2A sobre Circle, con la CPU z80cpp y la ULA, la contención y el AY de JSpeccy; cintas TAP, teclado USB, sonido por HDMI y jack, y selector de ficheros (F1). Probada en una Pi Zero emulada con QEMU.
