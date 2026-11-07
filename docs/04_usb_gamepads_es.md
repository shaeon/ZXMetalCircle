# Joysticks y mandos USB

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

Cómo se juega con mandos USB y con el teclado: los dos jugadores, qué mando es de cada uno y qué hace cada botón. Todo está en la **página 5 de la configuración, «Joysticks y mandos»** (Alt+F1 y AvPág).

## Qué mandos reconoce

- **Con su mapeo conocido**: PS3, PS4 DualShock, Xbox 360, Xbox One, Switch Pro y 8BitDo.
- **Mandos HID genéricos**, incluidos los adaptadores Mayflash en modo DirectInput.
- **Hasta 4 a la vez**, conectando y desconectando en caliente.

## Los jugadores

Cada jugador tiene su **joystick emulado**:

| Fila de la página 5 | Opción | Qué es |
| --- | --- | --- |
| Joystick jugador 1 | `joystick=` | ninguno, Kempston (por defecto), Fuller, Sinclair 1, Sinclair 2, Cursor o QAOP. F4 lo cambia en marcha |
| Joystick jugador 2 | `joystick2=` | lo mismo, o ninguno (por defecto: sin jugador 2) |

El Spectrum solo puede leer dos joysticks a la vez si no se pisan, así que **no se puede elegir el mismo para los dos, ni Cursor con Sinclair 1** (comparten las teclas 6, 7, 8 y 0). Las combinaciones que no valen se saltan al elegir. Las más habituales:

| Jugador 1 | Jugador 2 |
| --- | --- |
| Kempston | Sinclair 1, Sinclair 2, Cursor, Fuller o QAOP |
| Sinclair 1 | Sinclair 2 (el clásico de los juegos de dos jugadores del 128K) |

## Qué mando es de cada jugador

| Fila de la página 5 | Opción | Valores |
| --- | --- | --- |
| Mando del jugador 1 | `pad1=` | automático (el primero conectado) o un mando concreto |
| Mando del jugador 2 | `pad2=` | automático (el siguiente) o un mando concreto. En gris si no hay jugador 2 |
| Mandos de más | `padextra=` | se suman al jugador 1 (por defecto) o al 2, o solo sirven en los menús |

- **Un mando concreto** se guarda por fabricante y modelo (`045e:028e` es un mando de Xbox 360). Con las flechas se pasa por los mandos conectados; si el elegido no está, se ve su código con «(no conectado)».
- **Con dos mandos del mismo modelo**, el primero conectado es el del jugador 1. Para elegirlos por separado, **el segundo lleva su número**: `045e:028e#2` (y se ve «Mando 2» o, con alias, «XBOX 2»). Ese número sale del orden de conexión, porque dos mandos iguales no tienen nada que los distinga: si desconectas el primero, el segundo pasa a ser el primero.
- **Un alias** (por ejemplo «XBOX») se pone con la **A** en la pantalla «Botones del mando» y se ve en lugar de «Mando 1 (045E:028E)» en la página 5, en la lista para elegir mando, en esa pantalla y en Alt+D. Se guarda por modelo, en `joysticks/VVVV-PPPP/alias.txt`, así que lo comparten los mandos iguales, cada uno con su número. Dejándolo vacío se quita.
- **En los mandos con luces de jugador** (PS3, PS4…), la luz dice de qué jugador es: 1 o 2, y apagada si solo sirve en los menús.
- **F4** cambia el joystick del jugador 1. Un botón con la acción `joystick` (ver los perfiles) cambia el del jugador al que pertenece su mando.
- **Sin tocar nada**, todo funciona como siempre: un solo jugador, y todos los mandos son suyos.

## El teclado

- **Los cursores** son del jugador 1 (`cursors=`: teclas, joystick o los dos).
- **El disparo con el teclado** (`kbdfire=`): **las dos teclas Windows** (por defecto), el Ctrl derecho o ninguna. Con las teclas Windows, **los dos Ctrl son SYMBOL SHIFT**. El 0 del teclado numérico es siempre disparo.

## Qué hace cada botón: los perfiles

**Por defecto**, cada mando hace lo de siempre:

| Botón del mando | Kempston | Fuller | Sinclair 1 | Sinclair 2 | Cursor | QAOP |
| --- | --- | --- | --- | --- | --- | --- |
| Cruceta y stick izquierdo | direcciones | direcciones | 6-9 | 1-5 | 5-8 | Q A O P |
| **A / Cruz, Y / Triángulo, L1, R1** | disparo | disparo | disparo (0) | disparo (5) | disparo (0) | ESPACIO |
| **B / Círculo, X / Cuadrado, L2, R2** | disparo 2 | disparo | X | Z | 9 | M |

- **Start** abre la configuración (como Alt+F1) y **Select** (Back en los de Xbox) el navegador de ficheros.
- **En los menús**, la cruceta navega, el disparo hace de ENTER, el disparo 2 de ESC, y **L y R** (LB y RB en los de Xbox) cambian de página, como RePág y AvPág.
- **En los mandos genéricos**, que no se sabe qué botón es cuál: los botones 1, 4, 5 y 6 son disparo; el 2, 3, 7 y 8, disparo 2; el 9, Select (ficheros), y el 10, Start (configuración). En los menús, el 5 y el 6 cambian de página.

### Qué botón es cada uno

En los mandos genéricos los botones solo tienen número, y no se ve cuál es cuál. Para saberlo:

- **Alt+D** (el diagnóstico) muestra una línea por mando: su código, a qué jugador va (`J1`, `J2` o `menú`), **los botones que se están pulsando**, con el mismo nombre o número que en los perfiles, y hacia dónde va el stick. Por ejemplo: `1 045E:028E J1: LB A stick ↑<`.
- **En la pantalla «Botones del mando»**, al pulsar un botón se marca su fila, y abajo sale **«Has pulsado: Botón 5»**.

**Con un perfil**, cada botón hace lo que se quiera. Cada modelo de mando tiene sus perfiles, **uno por tipo de joystick**, y puede tener **varios con nombre** para el mismo tipo (por ejemplo, uno para cada juego).

### La pantalla «Botones del mando»

Se abre con **ENTER en la última fila de la página 5**. Si hay varios mandos conectados, primero pregunta cuál. Edita el perfil del tipo de joystick del jugador de ese mando.

- **La primera fila es el perfil**: «por defecto» o uno con nombre. Con ← → se pasa por los guardados. **N** crea uno nuevo (copiando el que se ve), **R** le cambia el nombre y **SUPR** lo borra. **A** pone el alias del mando.
- **Las demás, un botón cada una.** Al pulsar un botón del mando, se marca su fila y abajo sale cuál es («Has pulsado: Botón 5»). Mientras la pantalla está abierta, los botones de ese mando no hacen nada más.
  - **← →**: ninguno, disparo, disparo 2, arriba, abajo, izquierda, derecha, joystick, ayuda, configuración o ficheros.
  - **ENTER**: una tecla del Spectrum, CAPS SHIFT + tecla, SYMBOL SHIFT + tecla, o una tecla del emulador (F1 a F12, Alt+F1 a Alt+F12, Enter, Esc, Tab, Pausa, RePág, AvPág, Inicio, Fin, Supr y los cursores).
  - **SUPR**: el botón no hace nada.
- **G** guarda el perfil, lo deja como el activo y se aplica al momento.
- **ESC** sale. Si hay cambios sin guardar, pregunta.

Las teclas del Spectrum se mantienen pulsadas mientras se pulsa el botón; las del emulador se envían una vez al pulsarlo. El stick (y el «hat» de los mandos genéricos) siempre mueve el joystick.

### Los ficheros de los perfiles

Se guardan en la SD, en la carpeta `joysticks`, una carpeta por modelo de mando (`fabricante-modelo`):

```
joysticks/045e-028e/              mando de Xbox 360
    kempston.txt                  el perfil por defecto del Kempston
    kempston-MATAMARCIANOS.txt    otro perfil del Kempston
    sinclair2.txt
    activo.txt                    kempston = MATAMARCIANOS
```

- **`<tipo>.txt`** es el perfil por defecto de ese tipo de joystick (`none`, `kempston`, `fuller`, `sinclair1`, `sinclair2`, `cursor` o `qaop`). Si no existe, el mando hace lo de la tabla de arriba.
- **`<tipo>-<nombre>.txt`** es un perfil con nombre, y **`activo.txt`** dice cuál se usa de cada tipo.

Son ficheros de texto que se pueden escribir a mano o copiar a otra tarjeta. Una línea por botón; lo que no se diga, se queda como por defecto:

```
nombre = MATAMARCIANOS
A = disparo
B = disparo 2
X = tecla H
Y = tecla SS+P
LB = pc F5
RB = pc Alt+F1
L3 = pc Enter
R3 = tecla CS+ESPACIO
Select = ficheros
Start = configuracion
```

| Botones | Nombres |
| --- | --- |
| Mandos conocidos | A, B, X, Y, LB, RB, LT, RT, Select, Start, L3, R3, Guia, Arriba, Abajo, Izquierda, Derecha, Plus, Minus, Touchpad (también Cruz, Circulo, Cuadrado, Triangulo, L1, R1, L2, R2…) |
| Mandos genéricos | 1 a 32 |

| Acción | Qué hace |
| --- | --- |
| `ninguno` | nada |
| `disparo`, `disparo 2` | los disparos del joystick |
| `arriba`, `abajo`, `izquierda`, `derecha` | las direcciones del joystick |
| `joystick` | cambiar de joystick (como F4) |
| `ayuda` | la ayuda (F1); también vale `menu` |
| `configuracion` | la configuración (Alt+F1; lo que hace Start) |
| `ficheros` | el navegador de ficheros (lo que hace Select) |
| `tecla <K>` | una tecla del Spectrum: A-Z, 0-9, ENTER, ESPACIO, CS o SS; con `CS+` o `SS+` delante, junto con CAPS SHIFT o SYMBOL SHIFT (`tecla CS+ESPACIO` es BREAK) |
| `pc <K>` | una tecla del emulador: F1-F12, Enter, Esc, Tab, Retroceso, Espacio, Pausa, RePag, AvPag, Inicio, Fin, Supr, los cursores, letras y números; con `Alt+`, `Ctrl+` o `Shift+` delante |

También valen en inglés (`fire`, `fire 2`, `key`, `up`…). Si un perfil tiene líneas que no se entienden, se aplican las demás y se avisa: «Perfil con errores».
