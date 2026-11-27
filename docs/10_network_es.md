# Red

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

## Red: WiFi, servidor web y hora

**Modelos**: WiFi en la Raspberry Pi 3, 3B+, 3A+, 4, 400, Zero W y Zero 2 W, y **en pruebas en la Pi 5 y la 500** (Circle avisa de que en la Pi 5 la WiFi puede ser inestable con una pantalla HDMI a la vez; su firmware, `brcmfmac43455-sdio.raspberrypi,5-model-b.*`, va también en `firmware`). Las Pi con conector de red (1B, 2B, 3B, 4, 5) también pueden usar cable, igual que un adaptador USB de red compatible con Circle. Con `net=auto` se usa la WiFi si hay alguna red guardada en `wpa_supplicant.conf`; para forzar el cable, `net=eth`.

**Configurar la WiFi desde la Pi**: Alt+F1 (o F1 y luego C), página 7 «Red y hora», y ENTER en **«Red WiFi»**:
1. La Pi busca las redes de alrededor (unos segundos) y salen **de más a menos señal**: el nombre, la señal (**buena** en verde, **regular** en amarillo, **mala** en rojo) y si tiene **clave**, es **abierta** o ya está **guardada**; la conectada lleva `*`. Debajo, las guardadas que no se ven («sin señal»), **«Otra red (oculta)...»** para escribir el nombre de una red oculta (tal cual: las mayúsculas cuentan) y **«Buscar otra vez»**. Busca también si la Pi arrancó por cable o sin red: entonces arranca solo el chip WiFi para buscar. Solo salen las redes de 2,4 GHz.
2. Elige una red nueva y escribe su **contraseña** (si es abierta, se guarda directamente) (de 8 a 63 caracteres; vacía si la red es abierta). Se ve mientras se escribe, para comprobar que está bien; **TAB** la oculta o la vuelve a mostrar. Mayúsculas con Mayúsculas o CAPS SHIFT; símbolos con Ctrl o SYMBOL SHIFT, como en el Spectrum: además de los de los nombres de fichero, `* ? / : < >` (SS + B, C, V, Z, R, T) y, en modo extendido, `| \` (S, D).
3. **«Guardada. Para conectar, reiniciar»**: **Reiniciar ahora** o **Más tarde**. La red recién guardada es la primera, la de más prioridad. Si «Red» estaba en «cable» o «no», pasa a «automática» (y se guarda al reiniciar).

Con una red guardada, ENTER da **Conectar** (pasa a ser la primera y pregunta si reiniciar), **Cambiar la contraseña** (empieza con la que tiene, para corregirla) y **Olvidar esta red**; arriba se ve su contraseña. Caben 8 redes. **«País WiFi»** (debajo) dice en qué país se usa, porque cambia los canales permitidos: al principio, el del idioma (España para el español). Nada de esto se hace solo: hay que abrirlo desde el menú. El firmware del chip WiFi ya va en la carpeta `firmware` de la SD.

**A mano**: también se puede escribir `wpa_supplicant.conf` en la SD (hay un ejemplo, `wpa_supplicant.conf.ejemplo`). Solo valen redes de 2,4 GHz con WPA/WPA2. La Pi conserva lo que haya escrito a mano que no entienda (comentarios, `proto=`, `bssid=`...) y, al guardar, deja el anterior como `wpa_supplicant.conf.bak`. Si el fichero tiene un error (una red sin cerrar, por ejemplo), no lo toca y lo dice.

Al arrancar, la conexión se hace en segundo plano y la ayuda (F1, línea "Red:") dice en qué paso está: **buscando** la red, **conectando** a ella, **conectada, esperando IP** o que la red **rechaza la contraseña** (esto, además, sale abajo en cuanto pasa). Cuando está lista, aparece un mensaje con la dirección, por ejemplo `WiFi http://192.168.1.50`. Si a los dos minutos no ha conectado, sale una vez el aviso «WiFi sin conectar: mira F1 o reinicia».

**Si no conecta**: pon **«Registro WiFi»** (página 7) en «sí», o `wifilog=1` en `cmdline.txt`, y reinicia. Desde ese arranque se va apuntando en `SD:/wifi-diag.txt` (se añade a lo que haya) cada intento de conexión, los cortes con su motivo, si la red rechaza la contraseña, lo que dice el chip WiFi (por qué falla un intento y los avisos de su firmware) y el estado que se ve en F1, cada línea con los segundos desde el arranque. Con «detallado» (`wifilog=2`) se apunta también todo lo que hace wpa_supplicant por dentro. El fichero no pasa de 4 MB; cuando acabes, vuelve a ponerlo en «no».

**Operaciones largas**: la red y la emulación comparten el núcleo 0, así que mientras se lee o se escribe un fichero grande (desde 32 KB) por la web, o se busca o descarga de ZXDB (HTTPS), la emulación se para un momento. Durante ese rato se ve un aviso abajo, por ejemplo "Red: enviando juego.tap (512 KB)" o "Red: descargando de ZXDB...".

**Servidor web**: abre esa dirección en el navegador del PC o del móvil. Desde ahí puedes:
- recorrer las carpetas de la SD;
- **cargar** un fichero en el Spectrum **pulsando su nombre**: hace lo mismo que elegirlo en el navegador de ficheros de la Pi (cinta, snapshot, cartucho, microdrive, disco del +3 o disco duro). Los ficheros que el emulador no sabe cargar salen sin enlace. Con un disco duro (`.hdf`, `.img`, `.vhd`, `.raw`) se pide confirmación, porque montarlo reinicia el Spectrum;
- **ver el contenido de un ZIP** pulsando su nombre, y **cargar** el fichero de dentro que quieras pulsando el suyo (sin tener que elegirlo en la pantalla de la Pi). Los discos y microdrives de un ZIP se sacan a `tmp`, como desde la Pi; si ya estaba ahí, se pregunta en la pantalla de la Pi si usar el que hay o sobrescribirlo;
- **descargar** cualquier fichero de hasta 4 MB, por ejemplo tus partidas, snapshots o cartuchos de microdrive. Los más grandes muestran «más de 4 MB»: cópialos sacando la tarjeta;
- **subir** varios ficheros a la vez a la carpeta en la que estés (hasta 32 MB en total, como los ZIP que abre el emulador);
- **renombrar** ficheros y carpetas;
- **mover** ficheros y carpetas: «mover» lo marca, vas a la carpeta de destino y pulsas «Pegar aquí» (o «Cancelar»). Es inmediato, porque no se copia: solo cambia de carpeta;
- **sustituir** un fichero por otro que eliges en tu ordenador (pide confirmación);
- **borrar** ficheros y carpetas vacías (pide confirmación) y **crear carpetas**.

En el móvil, las acciones de cada fila salen al pulsar **⋮** (un toque fuera cierra el menú); en el ordenador, directamente en la fila.

**Lo que no se puede tocar desde la web** (no se puede renombrar, mover, sustituir ni borrar):
- **Lo que está usando el emulador**: un disco del +3 o un microdrive puesto, el disco duro montado (por defecto, `hdd/esxdos.img`) y la cinta puesta si los SAVE van a la SD. Sale como «en uso»: sácalo primero.
- **Lo del sistema**, que sale como «protegido»: los ficheros del arranque de la raíz (kernels, firmware, `config.txt`, `cmdline.txt`…), las carpetas que usa el emulador (`roms`, `saves`, `snaps`, `descargas`, `joysticks`, `hdd`…, aunque su contenido sí se puede tocar, por ejemplo para añadir tus ROMs o borrar partidas) y todo lo de `overlays`, `firmware`, `certs`, `locales` y `update`. Si necesitas cambiar algo de eso desde la web (por ejemplo, subir un kernel), pon **«Ficheros del sistema» en «sin proteger»** en la página 9 de la configuración (o `webprotect=0`): entonces se puede, pero cada operación sobre algo del sistema pide confirmación. Esa opción solo se cambia en la propia Pi, no desde la web. Lo que está en uso sigue protegido siempre.

Las subidas se escriben primero en un fichero temporal, y solo al terminar sustituyen al que había: si algo falla a mitad, no se pierde el original. Y todo lo que cambia algo lleva un código de seguridad que solo conoce la propia página, para que una página de fuera no pueda hacerlo con un enlace escondido: si alguna vez sale «Petición caducada», vuelve a cargar la página (pasa, por ejemplo, tras reiniciar la Pi).

Además, se anuncia en la red local (mDNS) como servicio web "ZXMetalCircle".

**Proteger la página** (F1, C, página 9, o en `cmdline.txt`):
- `web=0` la apaga del todo (ni siquiera se arranca el servidor); en F1 la línea de la red dice "(web apagada)".
- `web=ro` la deja en **solo lectura**: se puede ver la SD, bajar ficheros, buscar en ZXDB y mandar cargar un juego, pero no subir, borrar, renombrar, mover, sustituir, crear carpetas ni descargar de ZXDB a la SD (esas opciones ni aparecen).
- `webpass=` pone una **contraseña** (en la configuración se escribe con letras minúsculas y números, y se ve como `********`). La página la pide una vez y entonces crea una clave de sesión al azar que va en los enlaces y formularios (el servidor web de Circle no da acceso a cookies ni a la contraseña del navegador). Las claves se pierden al apagar la Pi; tras 5 intentos fallidos hay que esperar 30 s. Va sin cifrar (HTTP): protege de curiosos de tu red, no de alguien que la espíe.

**Hora**: se toma de `pool.ntp.org`. Los ficheros que grabes (SAVE, snapshots, microdrives) llevan fecha y hora reales. La zona se ajusta con `tz=` y el horario de verano europeo se aplica solo (`dst=1`, «automático»); con `dst=on` («sí») siempre se suma una hora y con `dst=0` («no»), nunca.

**Zero W**: tiene un solo núcleo, así que la WiFi le quita tiempo a la emulación. Al conectar (el cifrado WPA es costoso) y mientras subes o bajas ficheros puede haber cortes breves de imagen o sonido; el Spectrum no se retrasa, porque el emulador salta cuadros si hace falta. En reposo no se nota. Al arrancar con WiFi en una Zero W aparece un aviso.

## Buscar y descargar programas (HTTPS)

En la página web del emulador hay un buscador: escribes un título y busca en **ZXDB**, la base de datos de Spectrum Computing, a través de la API de ZXInfo.
- Solo salen programas de Spectrum: ni ZX81, ni SAM Coupé, etc.
- Para cada uno se elige el mejor fichero disponible, por este orden: TZX, TAP, Z80, SNA, SZX, DSK, CSW…
- Con "**descargar y cargar**", el emulador lo baja de spectrumcomputing.co.uk, lo descomprime si viene en ZIP, lo guarda en la carpeta **`descargas`** de la SD y lo carga en el Spectrum. Si hace falta, cambia de modelo, por ejemplo al +3 para un `.DSK`.

**También desde el propio emulador**, sin la página web: en el navegador de ficheros (F1 y ENTER), con red, la línea **«Buscar en internet (ZXDB)»** aparece al principio de la raíz de la SD, y **F3** lleva directo a ella desde cualquier carpeta.
1. Escribe el título, o una parte, con letras, números y espacios: no hacen falta guiones ni puntos (para *R-Type* basta con `R TYPE`).
2. Sale una lista con los resultados: título, año, editora y el formato del fichero. Los que no tienen fichero en ZXDB lo indican.
3. Al elegir uno, se descarga y se carga, igual que con «descargar y cargar» en la web, y queda en la carpeta `descargas` para la próxima vez. ESC en la lista hace otra búsqueda; ESC al escribir vuelve al navegador.

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
