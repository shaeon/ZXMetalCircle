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
