# Red

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

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
- **subir** varios ficheros a la vez a la carpeta en la que estés (hasta 32 MB en total, como los ZIP que abre el emulador);
- **descargar** cualquier fichero (hasta 4 MB), por ejemplo tus partidas, snapshots o cartuchos de microdrive;
- **cargar** una cinta, snapshot, cartucho o microdrive directamente en el Spectrum;
- **borrar** ficheros (pide confirmación) y **crear carpetas**.

Además, se anuncia en la red local (mDNS) como servicio web "ZXMetalCircle".

**Proteger la página** (F1, C, página 9, o en `cmdline.txt`):
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
