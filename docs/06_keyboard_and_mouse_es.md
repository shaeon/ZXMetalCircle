# Teclado y ratón

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

## Teclas

Las teclas de función están en una sola tabla (`KeyBindings` en `src/kernel.cpp`). La ayuda de F1 se genera a partir de ella, así que para cambiar una asignación basta con tocar esa tabla.

| Tecla | Función |
| --- | --- |
| F1 | Ayuda y estado; desde ahí, ENTER abre el selector, C la configuración, U las unidades y A los créditos |
| Alt+F1 | La configuración directamente (ESC vuelve al Spectrum) |
| Alt+U | Unidades: ver, montar y vaciar cada unidad |
| Alt+K | Ayuda de teclado |
| F2 | Cinta: reproducir / parar |
| F3 | Cinta: navegador de bloques (lista de lo que hay en la cinta; ENTER la deja en el bloque elegido, Inicio = rebobinar) |
| F4 | Cambiar joystick |
| F5 | NMI: Multiface, navegador de esxDOS o menú de la Spectranet |
| Alt+F5 | Navegador de ficheros (como F1 y ENTER). Con `f5=files` se intercambian: F5 abre los ficheros y Alt+F5 hace la NMI |
| F6 | Velocidad de carga: instantánea / rápida / real |
| F7 | Lista de modelos para elegir (con el actual marcado; los que no tienen ROM lo indican) |
| Alt+F7 | Pasar al siguiente modelo (16K, 48K, 128K, +2, +2A, +3, +3e) |
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
| Alt+V | Pulsar varias teclas a la vez (trucos como VICTOR en Profanation) |
| F12 o Alt+R | Reset |
| Pausa o Alt+P | Pausa |

Correspondencia del teclado del PC con el del Spectrum:

- Mayúsculas = CAPS SHIFT y Ctrl = SYMBOL SHIFT.
- Tab = EXTENDED MODE, Esc = BREAK, Retroceso = DELETE y Bloq Mayús = CAPS LOCK.
- La puntuación del teclado español (`, . - + ' < >` y sus variantes con mayúsculas) se traduce a su combinación con SYMBOL SHIFT.
- **Cursores**: siempre son las teclas de cursor del Spectrum (CAPS SHIFT + 5/6/7/8). Con un joystick elegido, además mueven el joystick, y **las teclas Windows** (o el 0 del teclado numérico) son el disparo; **los dos Ctrl son SYMBOL SHIFT**. Se puede cambiar con `cursors=` y, la tecla de disparo, con `kbdfire=` o en la página 5 de la configuración: teclas Windows, Ctrl derecho (como antes de la 1.08) o ninguna. Sin joystick, o con `cursors=keys`, la tecla de disparo no dispara.

Para cargar una cinta: pulsa F1 y luego ENTER, elige el `.TAP`, `.TZX`, `.PZX` o `.CSW` y escribe `LOAD ""` (en el 128K/+2A, elige "Tape Loader" o "Loader" en el menú). La cinta arranca sola cuando la ROM o un cargador propio empiezan a leerla, y se para sola cuando el programa deja de leerla como un cargador durante 2 segundos. Lo que cuenta es leer como un cargador (el mismo IN una y otra vez, con un contador que avanza, como hace Fuse), no leer mucho: los menús que miran el teclado sin parar no arrancan la cinta ni la mantienen en marcha, y así las multicargas encuentran el siguiente bloque donde lo dejaron. En el modo rápido, el emulador va a toda velocidad mientras dura la carga.

## Varias teclas a la vez (Alt+V)

Algunos trucos piden pulsar muchas teclas juntas, como **VICTOR** en Profanation para tener vidas infinitas. Un teclado USB manda como mucho seis teclas a la vez, y muchos no detectan ciertas combinaciones de cuatro, cinco o seis teclas.

**Alt+V** abre una línea donde se escriben las teclas (letras, números y espacio) y, con ENTER, el emulador las mantiene pulsadas todas a la vez durante 1,5 segundos. Funciona con cualquier teclado. Con un teclado real de Spectrum por GPIO o con un Recreated ZX no hace falta: se pueden pulsar todas a la vez, como en el Spectrum.

Si el teclado USB no puede con una combinación y manda un aviso de demasiadas teclas a la vez, el emulador mantiene las teclas que ya tenía pulsadas hasta el siguiente informe válido, como dice la norma USB, en lugar de soltarlas todas.

## Diagnóstico del teclado (Alt+D)

Muestra arriba a la izquierda, sin parar la emulación:
- el último informe del teclado USB (modificadores y hasta 6 teclas, en hexadecimal);
- la matriz del Spectrum con las teclas pulsadas resaltadas, sumando todas las fuentes (USB, GPIO, Recreated, cursores);
- la velocidad real en cuadros por segundo, y cuánto tarda cada cuadro en el núcleo 0 (y la ampliación de la imagen en el núcleo 1, en los modelos de 4 núcleos).

Sirve para ver exactamente qué recibe el Spectrum cuando una tecla hace algo raro. Alt+D otra vez lo quita.

Si la Raspberry no da abasto para dibujar todos los cuadros (por ejemplo, con scanlines a resolución nativa en una Pi lenta), el emulador deja de mostrar algún cuadro, hasta 4 seguidos, en lugar de ir más lento. Así el Spectrum y el teclado mantienen su velocidad.

## Recreated ZX Spectrum

El Recreated ZX Spectrum (Elite Systems, 2013) es una réplica del 48K de gomas que funciona como teclado USB o Bluetooth. Aquí solo se soporta por USB (Circle no tiene Bluetooth HID).

- Ponlo en **Layer A**, la capa de juego, con el interruptor trasero. En esa capa, cada tecla envía un carácter al pulsarse y otro al soltarse (por ejemplo, el 1 envía `a` al pulsarse y `b` al soltarse). Así se conocen las 40 teclas en cualquier combinación, como en la membrana original.
- **Por cable USB se reconoce al conectarlo**, por su identificador USB (VID:PID `0F39:0404`, TG3 Electronics; dato de José Luis Sánchez), sin pulsar nada. `recreated=0` lo desactiva (por ejemplo, para usarlo en Layer B, QWERTY).
- **Detección automática** (con `recreated=auto`, el valor por defecto) para cuando no se reconoce por el identificador, por ejemplo a través de un adaptador:
  - el emulador lo reconoce por su forma de transmitir: tres parejas completas de pulsar y soltar (por ejemplo, 'a' y luego 'b' para la tecla 1) que duran menos de 30 ms. Basta con pulsar 3 veces cualquier tecla y aparece "Teclado Recreated ZX detectado";
  - mientras tanto, esas pulsaciones no llegan al Spectrum;
  - un teclado normal se reconoce como tal tras 8 pulsaciones, y hasta entonces cada tecla puede llegar con un retraso de hasta unos 50 ms.
- Puede usarse solo o junto a un teclado USB normal y al teclado real por GPIO. Sus teclas se combinan con las del teclado GPIO.
- **Modo extendido**: CAPS SHIFT + SYMBOL SHIFT + ENTER, igual que con el teclado GPIO y que en ZXBaremulator. Las teclas que se consiguen en ese modo y con CAPS SHIFT son las mismas que con el teclado GPIO: ver la tabla en [03_gpio_es.md](03_gpio_es.md).
- En Layer B (QWERTY), el Recreated funciona como un teclado normal.

## Ratón Kempston

Un ratón USB conectado (en caliente) funciona como ratón Kempston. Igual que en ZXBaremulator, solo está activo cuando el joystick seleccionado **no** es Kempston, porque comparten puertos. Si no hay ratón conectado, los puertos no responden, así que los programas que lo detectan no lo encuentran.

| Puerto | Contenido |
| --- | --- |
| 0xFADF | Botones: bit 0 derecho, bit 1 izquierdo, bit 2 central (activos a 0); bits 4-7, rueda |
| 0xFBDF | X (crece hacia la derecha) |
| 0xFFDF | Y (crece hacia arriba) |

La decodificación es parcial (A0, A5, A8 y A10), como en Fuse.
