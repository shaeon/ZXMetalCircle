# Pines GPIO de ZXMetalCircle

Qué pines del conector de 40 de la Raspberry Pi usa el emulador, para qué, y
qué opciones se excluyen entre sí porque comparten pines.

Los números son siempre **GPIO (BCM)**; entre paréntesis, el **pin físico**
del conector. Todos los pines que lee el emulador son entradas con resistencia
interna a 3,3 V (pull-up): **nunca les metas 5 V**.

## Qué usa los GPIO

| Función | Opción en `cmdline.txt` | En la configuración (F1, C) |
| --- | --- | --- |
| Teclado real del 48K/128 (matriz 8 × 5), con los joysticks cableados a la matriz | `gpiokbd=48` (por defecto) | 4 Teclado y mandos: "Teclado por GPIO" = 48K/128 |
| Teclado real del +2A/+3 (matriz 13 × 11) | `gpiokbd=plus3` | "Teclado por GPIO" = +2A/+3 |
| Sin teclado por GPIO | `gpiokbd=0` | "Teclado por GPIO" = ninguno |
| Pulsador de modo extendido (solo con el teclado 48K/128) | va con `gpiokbd=48` | — |
| Pulsador de NMI (solo con el teclado 48K/128) | va con `gpiokbd=48` | — |
| Copia del sonido PWM por el GPIO12 (Pi 1, 2, 3 y Zero) | `sounddev=sndpwm gpioaudio=1` | 2 Imagen y sonido: "Sonido por GPIO12" |
| Sonido PWM en la Pi 5 (no tiene jack) | `sounddev=sndpwm` | "Salida de sonido" = jack (PWM) |
| Entrada de un casete real (con un comparador, p. ej. LM393) | `ear=gpio earpin=...` | 6 Red, hora y casete: "Casete real" = GPIO, "GPIO del casete" |
| Registro por el puerto serie | `logdev=ttyS1` | — |

El montaje del teclado es el de pastbytes para ZXBaremulator
(https://pastbytes.com/zxb/), con un diodo en cada fila (cátodo hacia la Raspberry).
El del +2A/+3 sigue el proyecto zxunops2 de spark2k06.

## Tabla por pines

"48K" es `gpiokbd=48` y "+3" es `gpiokbd=plus3`. Una casilla vacía quiere decir
que esa opción no usa el pin.

| GPIO | Pin | Teclado 48K | Teclado +3 | Sonido | Casete (`ear=gpio`) | Otros |
| ---: | ---: | --- | --- | --- | --- | --- |
| 2 | 3 | | columna C9 | | | (I2C; la Pi le pone pull-up fija) |
| 3 | 5 | | columna C10 | | | (I2C; la Pi le pone pull-up fija) |
| 4 | 7 | fila A15 | fila A15 | | | |
| 5 | 29 | columna D4 | columna D4 | | | |
| 6 | 31 | columna D3 | columna D3 | | | |
| 7 | 26 | | fila R12 | | | |
| 8 | 24 | | columna C5 | | | |
| 9 | 21 | | columna C6 | | | |
| 10 | 19 | | columna C7 | | | |
| 11 | 23 | | columna C8 | | | |
| 12 | 32 | | fila R11 | copia del PWM (Pi 1-3, `gpioaudio=1`); PWM en la Pi 5 | | |
| 13 | 33 | columna D2 | columna D2 | PWM en la Pi 5 | | |
| 14 | 8 | | | | | TX del puerto serie (`logdev=ttyS1`) |
| 15 | 10 | | | | **por defecto con el teclado +3** | RX del puerto serie (el registro no lo usa) |
| 16 | 36 | pulsador de NMI | fila R10 | | | |
| 17 | 11 | fila A14 | fila A14 | | | |
| 18 | 12 | fila A8 | fila A8 | | | |
| 19 | 35 | columna D1 | columna D1 | | | |
| 20 | 38 | | fila R8 | | **por defecto** (sin teclado o con el 48K) | |
| 21 | 40 | pulsador de modo extendido | fila R9 | | | |
| 22 | 15 | fila A12 | fila A12 | | | |
| 23 | 16 | fila A9 | fila A9 | | | |
| 24 | 18 | fila A10 | fila A10 | | | |
| 25 | 22 | fila A11 | fila A11 | | | |
| 26 | 37 | columna D0 | columna D0 | | | |
| 27 | 13 | fila A13 | fila A13 | | | |
| 0, 1 | 27, 28 | | | | | (EEPROM de las placas HAT: no se usan) |

Los pulsadores van de su GPIO a GND: el de modo extendido del GPIO21 (pin 40) al
pin 39, y el de NMI del GPIO16 (pin 36) al pin 34.

Pines libres según el teclado:

- **Sin teclado por GPIO**: todos menos el 14 (si usas `logdev=ttyS1`).
- **Con el teclado 48K/128**: quedan libres el 2, 3, 7, 8, 9, 10, 11, 12, 15 y 20 (y el 14 sin registro serie).
- **Con el teclado +2A/+3**: solo quedan el 14 y el 15 (el puerto serie).

## Pines compartidos: qué se quita según la opción

| Si eliges... | ...pasa esto | Por qué |
| --- | --- | --- |
| `gpiokbd=plus3` | No hay pulsador de modo extendido (sirve CAPS SHIFT + SYMBOL SHIFT + ENTER) ni pulsador de NMI | El GPIO21 y el GPIO16 son filas del teclado del +3 |
| `gpiokbd=plus3` | No hay copia del sonido por el GPIO12, aunque `gpioaudio=1` | El GPIO12 es la fila R11 |
| `gpiokbd=plus3` y `ear=gpio` | El casete va al GPIO15 (pin 10) en vez del 20 | El GPIO20 es la fila R8; el 15 es la entrada RX del puerto serie, que el registro (`logdev=ttyS1`) no usa |
| `gpiokbd=48` o `plus3` en la **Pi 5** con `sounddev=sndpwm` | El PWM no arranca y el sonido sale por HDMI | La Pi 5 saca el PWM por el GPIO12 y el GPIO13, y el 13 es una columna del teclado: con el PWM sonando parecían teclas pulsadas sin parar |
| `gpioaudio=1` en la **Pi 4 / 400** | No hace nada | En la Pi 4 el audio del jack usa el PWM1, que no llega al conector |
| `gpioaudio=1` | El sonido pasa a ser mono | El GPIO12 es un solo canal del PWM0 (en unos modelos el izquierdo y en otros el derecho) |
| `ear=gpio earpin=N` con un pin ocupado | No se activa el casete y sale el aviso "earpin=N: ese GPIO ya se usa" | El pin es del teclado, del GPIO12 con sonido, o es el 0, 1 o 14 |
| `ear=gpio` y `ear=usb` | Solo uno a la vez | Es la misma opción |

## En la configuración (F1, C)

Las opciones que no se usan con lo elegido salen **en gris** y no se pueden
cambiar (versión 0.91 y posteriores):

| Opción | Sale en gris cuando... |
| --- | --- |
| Sonido por GPIO12 | la salida de sonido no es el jack (PWM); el teclado por GPIO es el del +2A/+3; o la Pi es una 4, 400 o 5 |
| GPIO del casete | "Casete real" no es GPIO |
| Invertir casete | "Casete real" es no |

Dos casos que el menú **no** puede marcar, porque en él solo se ponen en gris
opciones enteras, no valores sueltos de una lista:

- **"GPIO del casete" con un pin ocupado**: se puede elegir, pero al arrancar sale
  el aviso "ese GPIO ya se usa" y el casete no se activa. Deja "automático", que
  elige siempre un pin libre (20, o 15 con el teclado del +3).
- **"Salida de sonido" = jack (PWM) en la Pi 5 con teclado por GPIO**: se puede
  elegir, pero el emulador pasa solo al HDMI (ver la tabla de arriba).

Los cambios de estas opciones se aplican al reiniciar la Pi (llevan `*` en el menú).

---

## Montaje

### Teclado real y joysticks por GPIO

La tabla completa de pines GPIO (teclado, sonido, casete, puerto serie) y qué opciones comparten pines está en [docs/03_gpio_es.md](#).

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

### Entrada de cinta: casete real

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
