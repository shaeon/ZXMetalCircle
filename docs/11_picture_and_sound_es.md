# Imagen y sonido

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

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
