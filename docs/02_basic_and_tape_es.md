# Escribir programas y grabarlos en cinta con ZXMetalCircle

Esta guía explica cómo escribir un programa en BASIC en el Spectrum emulado, grabarlo en una «cinta» y volver a cargarlo, con un ejemplo de «Hola, mundo».

## 1. Antes de empezar: el teclado

El teclado del PC hace de teclado del Spectrum:

| Tecla del PC | En el Spectrum |
| --- | --- |
| Mayúsculas | CAPS SHIFT |
| **Ctrl izquierdo** | **SYMBOL SHIFT** |
| Retroceso | DELETE (borrar) |
| Cursores | CAPS SHIFT + 5, 6, 7, 8 |
| `,` `;` `.` `:` `+` `*` `-` `_` `'` `?` `<` `>` | se escriben directamente con su tecla del PC: el emulador pulsa por ti SYMBOL SHIFT + la tecla que toque |
| **Comillas `"`** | **Ctrl izquierdo + P** (SYMBOL SHIFT + P) |
| **Exclamación `!`** | **Ctrl izquierdo + 1** (SYMBOL SHIFT + 1) |

Si te pierdes, **Alt+K** muestra la ayuda del teclado.

## 2. Elegir el modelo

La forma de escribir depende del modelo (se cambia con **F7**):

- **48K (y 16K, TK90X, TK95)**: cada instrucción está en una tecla. Al empezar una línea, el cursor está en modo **K** (*keyword*), y al pulsar una tecla se escribe la palabra entera: `P` escribe `PRINT`, `J` escribe `LOAD`, `S` escribe `SAVE`… Las palabras de cada tecla están impresas en el teclado del Spectrum, y también en la ayuda del teclado (Alt+K).
- **128K, +2, +2A y +3**: en «128 BASIC» o «+3 BASIC» se escriben las palabras letra a letra, como en un ordenador actual: `p`, `r`, `i`, `n`, `t` escribe `print`.

Para empezar es más cómodo un **128K o un +2** (o el +3 en «+3 BASIC»): no hace falta saberse las teclas.

## 3. Escribir el programa: «Hola, mundo»

El programa:

```basic
10 BORDER 1: PAPER 1: INK 7: CLS
20 PRINT AT 10,9;"Hola, mundo!"
30 PAUSE 0
```

Pone el borde y el fondo en azul con letras blancas, borra la pantalla, escribe el mensaje en el centro y espera a que se pulse una tecla.

### En un 128K, +2 o +3

1. Arranca y elige **«128 BASIC»** (o «+3 BASIC») con los cursores y ENTER.
2. Escribe cada línea tal cual, con su número delante, y pulsa **ENTER** al final de cada una. Recuerda: las comillas son **Ctrl + P** y la exclamación **Ctrl + 1**.
   ```
   10 border 1: paper 1: ink 7: cls
   20 print at 10,9;"Hola, mundo!"
   30 pause 0
   ```
   Si una línea tiene un error, el Spectrum no la acepta y marca dónde está el problema; corrígela y vuelve a pulsar ENTER.
3. Escribe `run` y ENTER para ejecutarlo. Cuando termine (pulsa una tecla para salir del `PAUSE 0`), espera a que vuelva a aparecer el cursor antes de seguir escribiendo.

### En un 48K

En el 48K cada palabra clave es una tecla. La línea 20, por ejemplo, se escribe así:

| Pulsas | Aparece |
| --- | --- |
| `2` `0` | `20` |
| `P` | `PRINT` (el cursor estaba en modo K) |
| SYMBOL SHIFT + `I` | `AT` |
| `1` `0` `,` `9` `;` | `10,9;` |
| Ctrl + `P` | `"` |
| Mayúsculas + `H`, luego `o`, `l`, `a`, `,`, espacio, `m`, `u`, `n`, `d`, `o` | `Hola, mundo` |
| Ctrl + `1` | `!` |
| Ctrl + `P` | `"` |
| ENTER | la línea pasa al programa |

Las otras palabras de la línea 10 están en estas teclas: `BORDER` es la `B`; `PAPER` e `INK` se escriben en el modo extendido (pulsa a la vez Mayúsculas y Ctrl izquierdo, y luego Ctrl + `C` para `PAPER` o Ctrl + `X` para `INK`); `CLS` es la `V`, y `PAUSE` (línea 30) es la `M`. Para ejecutar, `RUN` es la `R`.

## 4. Grabar en cinta

Cuando el programa funcione, grábalo con:

```basic
SAVE "hola" LINE 10
```

(En el 48K: `S` para `SAVE`, Ctrl + `P` para las comillas, `hola`, otra vez Ctrl + `P`, y `LINE` en el modo extendido: Mayúsculas + Ctrl izquierdo a la vez y luego Ctrl + `3`.)

`LINE 10` hace que, al cargarlo, el programa **arranque solo** en la línea 10. Sin `LINE`, se carga y espera a que escribas `RUN`.

El Spectrum contesta **«Start tape, then press any key»**: pulsa **ENTER** (o cualquier otra tecla **menos ESPACIO**: en ese momento el ESPACIO hace de BREAK y cancela la grabación, con el mensaje «D BREAK - CONT repeats»). Al momento aparece abajo el aviso **«Grabado en saves/hola.tap»** y el Spectrum responde «0 OK».

**Qué hace el emulador:** en lugar de esperar el tiempo real de una grabación, escribe los bloques al momento en la tarjeta SD:

- El fichero va a la carpeta **`saves`** de la SD y se llama como el programa: `saves/hola.tap`. Si ya existe, se crea `hola_2.tap`, `hola_3.tap`…
- Ese fichero pasa a ser **la cinta actual**. Lo que grabes después se añade al final, como en una cinta real.
- **Las cintas de los juegos nunca se modifican**: si la cinta puesta es de un juego, se empieza un fichero nuevo.
- Con la opción `savefmt=tzx` (o en la configuración, «Formato del SAVE») se graba en `.tzx` en lugar de `.tap`.

### En el +3: elegir la cinta

El +3 graba y carga por defecto en **disco** (la unidad A:). Para usar la cinta, antes escribe:

```basic
SAVE "t:"
LOAD "t:"
```

`SAVE "t:"` hace que `SAVE` grabe en cinta, y `LOAD "t:"`, que `LOAD` cargue de cinta. Con `SAVE "a:"` y `LOAD "a:"` se vuelve al disco.

## 5. Volver a cargarlo

1. Borra el programa de la memoria con `NEW` (en el 48K, la `A`), o reinicia con F12.
2. Pulsa **F1**, entra en la carpeta `saves` y elige **`hola.tap`**: queda puesta como cinta.
3. Cárgala:
   - **48K:** escribe `LOAD ""` (la `J` y dos veces Ctrl + `P`) y ENTER.
   - **128K o +2:** elige **«Tape Loader»** en el menú, o escribe `load ""`.
   - **+3:** elige **«Loader»** en el menú (sin disco puesto, carga de cinta), o escribe `load "t:"` y luego `load ""`.
4. Como se grabó con `LINE 10`, el programa arranca solo y aparece «Hola, mundo!».

Si acabas de grabar y quieres cargar sin cambiar de cinta, rebobínala antes con **F3** (Inicio y ENTER), porque después de grabar queda al final.

## 6. Llevarlo al PC (o traerlo)

- **Desde el PC**, en la página web del emulador (la dirección aparece en la ayuda, F1), entra en la carpeta `saves` y descarga `hola.tap`. Funciona en cualquier emulador de Spectrum.
- **Al revés**, puedes subir un `.tap` desde el PC por la web y cargarlo en el Spectrum.

## 7. Más cosas que se pueden grabar

| Orden | Graba |
| --- | --- |
| `SAVE "pantalla" SCREEN$` | la pantalla actual |
| `SAVE "codigo" CODE 32768,1000` | 1000 bytes de memoria desde la dirección 32768 |
| `SAVE "datos" DATA a()` | la matriz `a()` |

Los programas con **rutinas de grabación propias** (cargas turbo, juegos que graban partidas) no usan la ROM y no pasan por este sistema. Para esos está **Alt+W**: graba la señal de cinta tal cual, como una cinta virgen. Se pulsa antes de grabar y otra vez al terminar, y crea `saves/rec_001.tzx`.

---

*Probado en ZXMetalCircle 1.06 con un 128K: el programa se escribe, se ejecuta, se graba en `saves/hola.tap` (109 bytes) y, al cargarlo con «Tape Loader», arranca solo.*

*ZXMetalCircle, de Shaeon (Carlos Palmero). Más guías en [docs](README.md).*

---

## Referencia: grabar con SAVE

### Grabar con SAVE

Cuando la ROM graba un bloque (SAVE, SAVE ... CODE, SAVE ... SCREEN$, el "Tape" de los menús del 128K/+2A…), el emulador intercepta la rutina SA-BYTES y escribe el bloque directamente en la SD, sin esperar el tiempo real de grabación.

- Los ficheros van a la carpeta `saves` de la SD, que se crea sola. El nombre sale de la cabecera: `SAVE "prueba"` crea `saves/prueba.tap`, y si ya existe, `prueba_2.tap`, `prueba_3.tap`…
- **Nunca se modifican las cintas de los juegos.** Si la cinta actual no es de grabaciones, se crea un fichero nuevo, que pasa a ser la cinta actual. Las siguientes grabaciones se añaden a ese fichero, como en una cinta real.
- Si cargas un .TAP de la carpeta `saves` desde el selector, lo que grabes después se añade a él.
- Después de grabar, la cinta queda al final. Para volver a cargar, rebobina con F3 (Inicio y ENTER) o elígela otra vez en el selector.
- Con `savefmt=tzx` las grabaciones van a ficheros .TZX con bloques estándar (0x10) en lugar de .TAP.
- Las rutinas de grabación propias (turbo, cargadores especiales) no pasan por la ROM: para esas está la grabación de la señal (abajo).

### Grabar la señal de cinta (Alt+W)

Funciona como poner una cinta virgen a grabar: se registra cada cambio de la salida MIC del Spectrum con una precisión de 16 t-states. Sirve con cualquier rutina de grabación, incluidas las turbo y las de los juegos que graban partidas.

1. Pulsa **Alt+W**. Aparece un punto rojo arriba a la izquierda.
2. Haz que el programa grabe (SAVE, opción de grabar del juego…).
3. Pulsa **Alt+W** otra vez para parar.

Se crea `saves/rec_001.tzx` (con un bloque CSW 0x18), que pasa a ser la cinta actual, rebobinada y lista para cargar. Mientras se graba, el trap de SAVE está desactivado, así que también las grabaciones de la ROM se registran como señal.
