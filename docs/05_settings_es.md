# Configuración

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

## Opciones de `cmdline.txt`

En la SD, el fichero `cmdline.hlp` (`cmdline-en.hlp` en inglés) explica todas las opciones con sus valores posibles. Casi todas se pueden cambiar también desde la configuración del emulador (F1, C).

Todas las opciones van en una sola línea:

| Opción | Valores | Por defecto |
| --- | --- | --- |
| `model=` | `16k`, `48k`, `128k`, `plus2`, `plus2a`, `plus3`, `plus3e`, `tk90x`, `tk95` | `128k` |
| `tkula=` | ULA del TK90X y el TK95: `50` (Microdigital a 50 Hz: Uruguay), `60` (Microdigital a 60 Hz: Brasil) o `ferranti` (los primeros de exportación; los tiempos del 48K) | `50` |
| `tkrom=` | ROM del TK90X: `v3` (la de Rodolfo Guerra, la más compatible), `v1` (original, la más difundida) o `v2` (original; el TK95 siempre la lleva). Se aplica al momento: con un TK, se reinicia con la ROM nueva | `v3` |
| `tklang=` | Idioma de la ROM del TK90X y del TK95: `auto` (el del menú), `es`, `en`, `pt`, `ca`, `cs`, `de`, `eo`, `eu`, `gl`, `it`, `nl`, `no`, `pl`, `ro`, `ru`, `sv`, `vl`. Las originales solo tienen español y portugués (los demás idiomas usan el español): la v1 elige con el diodo de los TK90X brasileños (el bit 7 del puerto 254 a 0 da el portugués) y la v2 y el TK95 con su ROM española o brasileña. La v3 tiene los 17. Se aplica al momento | `auto` |
| `updboot=` | `1`: buscar una versión nueva la primera vez que hay red. Se cambia también en la página 10 de la configuración | `0` |
| `updkeep=` | Paquetes que se dejan en `update/`: `2` (el instalado y el anterior) o `all` | `2` |
| `updconfig=` | Al actualizar, los ficheros de configuración: `keep` (se conservan; el nuevo queda como `.nuevo`) o `new` (se pone el nuevo; el tuyo queda como `.bak`) | `keep` |
| `tape=` | `flash` (instantánea), `fast` (rápida), `normal` (real) | `flash` |
| `tapeaccel=` | `1` (acelerar cargadores conocidos), `0` | `1` |
| `diskaccel=` | `1` (+3/+3e: emulación acelerada mientras se usa la disquetera; las protecciones no cambian), `0` | `0` |
| `joystick=` | `none`, `kempston`, `fuller`, `sinclair1`, `sinclair2`, `cursor`, `qaop` | `kempston` |
| `sounddev=` | `sndhdmi`, `sndpwm`, `sndusb` (Pi 4/400), `none` | `sndhdmi` |
| `speaker=` | `auto`, `beeper`, `tv`, `off` (modelo del altavoz del beeper) | `auto` |
| `beeperfilter=` | `1` (quita el pitido de 1 bit), `0` | `1` |
| `recreated=` | `0` (nunca), `auto` (por cable USB se reconoce al conectarlo; si no, detección automática), `1` (todos los teclados USB son Recreated) | `auto` |
| `cursors=` | `both` (cursores = CAPS SHIFT+5-8 y joystick), `keys` (solo teclas), `joystick` (solo joystick) | `both` |
| `kbdfire=` | Tecla del teclado que hace de disparo del joystick: `win` (las dos teclas Windows; los dos Ctrl son SYMBOL SHIFT), `rctrl` (el Ctrl derecho, como antes de la 1.08) o `none` | `win` |
| `joystick2=` | Joystick del jugador 2: los mismos valores que `joystick=`; no puede ser el del jugador 1, ni Cursor con Sinclair 1 | `none` |
| `pad1=` | Mando del jugador 1: `auto` (por orden de conexión) o un mando concreto (`fabricante:modelo`, por ejemplo `045e:028e`) | `auto` |
| `pad2=` | Mando del jugador 2, igual | `auto` |
| `padextra=` | Los demás mandos: `p1` o `p2` (se suman a ese jugador) o `menu` (solo sirven en los menús) | `p1` |
| `f5=` | `nmi` (F5 = NMI, Alt+F5 = ficheros), `files` (F5 = ficheros, Alt+F5 = NMI) | `nmi` |
| `aystereo=` | `abc`, `acb`, `bac`, `mono` | `abc` |
| `tapesound=` | `1` (se oye la carga), `0` (silencio) | `1` |
| `border=` | `none`, `standard`, `full`, `huge` | `full` |
| `scanlines=` | `1` (activadas), `0` | `0` |
| `pureblack=` | `1` (negro puro también en la paleta del 16K/48K), `0` | `0` |
| `indicators=` | `1` (la D, la H, M1-M8 y el punto de grabación arriba de la imagen), `0` (sin ellos; el icono del modo extendido del teclado sigue saliendo) | `1` |
| `multicore=` | `1` (ampliación de la imagen en el núcleo 1, Pi 2/3/4), `0` | `1` |
| `issue=` | `3` (Issue 3), `2` (Issue 2, solo afecta al 16K/48K) | `3` |
| `speed=` | `1`, `2`, `4`, `10` | `1` |
| `ulaplus=` | `1` (ULAplus disponible), `0` (desactivado) | `1` |
| `lang=` | `es` (español), `en` (inglés), `pt` (portugués de Portugal), `pt-br` (portugués de Brasil) o cualquier otro idioma de `locales/` | `es` |
| `net=` | `auto` (WiFi si hay `wpa_supplicant.conf`, si no cable), `wifi`, `eth`, `0` | `auto` |
| `tz=` | diferencia horaria en minutos (60 = España peninsular, 0 = Canarias) | `60` |
| `dst=` | `1` (horario de verano europeo automático), `0` | `1` |
| `ip=`, `mask=`, `gw=`, `dns=` | IP fija (si no se pone `ip=`, se usa DHCP) | — |
| `lec=` | `1` (memoria LEC conectada; en los 128K se usa desde 48 BASIC), `0` | `0` |
| `if1=` | `auto` (se conecta al insertar un microdrive), `1` (siempre), `0` (nunca) | `auto` |
| `rs232=`, `zxnet=` | `1`, `0` (RS232 y ZX Net del Interface 1) | `1` |
| `divmmc=` | `0`, `esxdos` | `0` |
| `esxdir=` | carpeta desde la que se importa a la tarjeta de esxDOS, `0` para ninguna | `esxdos_i` |
| `spectranet=` | `1`, `0` | `0` |
| `savefmt=` | `tap`, `tzx` (formato de las grabaciones con SAVE) | `tap` |
| `snapformat=` | `z80`, `sna`, `szx` (formato de F8) | `z80` |
| `savetrap=` | `1` (SAVE graba en `saves/` de la SD), `0` (desactivado) | `1` |
| `multiface=` | `1` (activo si está su ROM en la SD), `0` (desactivado) | `1` |
| `gpioaudio=` | `1` (con `sounddev=sndpwm`, el audio sale también por GPIO12, en mono), `0` | `1` |
| `gpiokbd=` | `48`, `plus3` (tipo de teclado por GPIO), `0` (ninguno) | `48` |
| `logdev=` | `ttyS1` (mensajes por el puerto serie) | |

Si falla el dispositivo de sonido elegido, el emulador prueba automáticamente el otro.

## Idiomas

El emulador está en **español** (el idioma principal, que va dentro del programa), **inglés** y **portugués**, de Brasil y de Portugal. Se elige con `lang=en` en `cmdline.txt` o en la página 1 de la configuración (F1, C), donde el cambio se ve al momento y se guarda con G. También cambia la página web.

- Las traducciones están en la carpeta `locales/` de la SD (se ve en la web para poder bajarlas y subirlas): un fichero por idioma, `en.json`, con el texto en español como clave y su traducción como valor. Si falta una traducción (o está vacía, `""`), se ve el español.
- `es.json` es solo de referencia: la lista completa de textos, para quien quiera traducir a otro idioma. Para añadir uno, copia `en.json` a, por ejemplo, `fr.json`, cambia `"_idioma"` por `"Français"` y traduce los valores: el emulador lo encuentra solo y aparece en la configuración.
- Los menús tienen las letras de Europa: acentos, diéresis, ñ, ç, ¡¿, «», €, æ, ø, ß y las más usadas de Europa central (č, š, ž, ł, ő…).
- La documentación está en español e inglés: `README_es.md` / `README.md`, `cmdline.hlp` / `cmdline-en.hlp` y los `LEEME.txt` / `README.txt`. El registro (`logdev=`) sigue en español.

**Mantener las traducciones al día** (para quien programa): todo texto que se vea en pantalla se escribe en español dentro de `T ("…")`, o de `N_ ("…")` en las tablas (y se traduce con `T ()` al usarlo). `build.sh` ejecuta `tools/i18n/actualizar.py`, que:
- rehace `locales/es.json`;
- añade a los demás idiomas los textos nuevos, sin traducir;
- guarda como `"_huerfano|texto"` los que ya no se usan (por si el español solo ha cambiado un poco);
- avisa de lo que falta y de las traducciones cuyos `%u`, `%s`… no coinciden con el español. Solo avisa: la compilación sigue.

## Configuración desde el emulador

Desde la ayuda (F1), la tecla **C** abre la configuración: diez páginas con las opciones de `cmdline.txt` que se pueden cambiar sin tocar el fichero. Las opciones que no se usan con lo elegido salen en gris, y el cursor se las salta: no se pueden cambiar hasta que vuelvan a tener sentido (por ejemplo, al elegir otro modelo): por ejemplo, el teclado Issue 2/3 fuera del 16K y el 48K, el idioma y el disco duro del +3e en los demás modelos, el formato de SAVE sin SAVE a la SD, o la máscara, la puerta de enlace y el DNS con la IP automática.

| Página | Opciones |
| --- | --- |
| 1 Máquina | idioma, modelo, teclado Issue 2/3, velocidad, ULAplus, idioma del +3e*, disco duro al inicio* |
| 2 Imagen y sonido | borde, scanlines, sonido AY, oír la carga, altavoz, quitar pitido 1 bit, salida de sonido*, sonido por GPIO12*, imagen en el núcleo 1* |
| 3 Cintas y grabación | modo de carga, SAVE a la SD, formato de SAVE, formato de F8 |
| 4 Teclado | F5 / Alt+F5, Recreated ZX, teclado por GPIO* |
| 5 Joysticks y mandos | joystick de los jugadores 1 y 2, mando de cada jugador, mandos de más, cursores del PC, disparo con el teclado, botones del mando (ver [04_usb_gamepads_es.md](04_usb_gamepads_es.md)) |
| 6 Periféricos | Spectranet, divMMC con esxDOS, memoria LEC, Interface 1, Multiface, RS232*, ZX Net* |
| 7 Red, hora y casete | red*, zona horaria*, horario de verano*, IP fija*, máscara*, puerta de enlace*, DNS*, comprobar HTTPS*, casete real*, invertir casete*, GPIO del casete* |
| 8 ROMs y carpetas | ROM de cada máquina*, del Interface 1*, de esxDOS* y flash de la Spectranet* (las de la carpeta `roms` con el tamaño correcto), carpeta de esxDOS* |
| 9 Servidor web | página web* (sí, solo lectura o no) y su contraseña* |
| 10 Actualización | buscar e instalar, paquete guardado, buscar al arrancar, paquetes guardados, configuración al actualizar |

- Cursores arriba y abajo para elegir; **izquierda y derecha** (o ENTER) para cambiar el valor; **RePág y AvPág** (o Tab) para cambiar de página.
- **En las opciones de muchos valores** (modelo, idioma de la ROM del TK, joysticks, zona horaria, GPIO del casete), **ENTER abre la lista entera**, con el valor puesto marcado: se elige con los cursores (o 1-9) y ENTER, y se vuelve a la configuración; ESC vuelve sin cambiarlo. Las flechas siguen pasando de uno en uno.
- **Las direcciones IP se escriben**: ENTER en "IP fija", "Máscara de red", "Puerta de enlace" o "Servidor DNS" y se teclean los números y los puntos (también con el teclado numérico); Retroceso borra, SUPR la deja vacía (automática, con DHCP), ENTER acepta si es una IP válida y ESC la deja como estaba.
- En las listas de ROMs y de discos duros solo salen los ficheros sin espacios en el nombre (`cmdline.txt` separa las opciones con espacios).
- Los cambios se aplican **al momento**, salvo los marcados con `*`, que necesitan reiniciar la Pi.
- **G** guarda los valores en `cmdline.txt`. Conserva el resto de opciones del fichero (`hdd=`, `ip=`, `logdev=`…) y su orden, cambia las que ya estaban y añade las que no tienen su valor por defecto. Antes deja una copia en `cmdline.bak`.
- **R** reinicia la Raspberry Pi. Pide confirmación ("No" es la opción marcada) y, si hay cambios sin guardar, ofrece guardarlos antes. Antes de reiniciar se guarda todo lo pendiente: discos del +3, cartuchos de microdrive, la flash de la Spectranet, lo grabado por esxDOS y la salida del RS232.
- La configuración muestra lo que hay en `cmdline.txt` (se vuelve a leer al abrirla si no hay cambios pendientes, por si se ha cambiado desde la web), más lo que se cambie con las teclas: F4, F6, F7, F9, F10, Alt+B, Alt+S y los periféricos (Alt+T/X/L). Así G también sirve para guardar esos cambios. Lo que cambia solo, como el modelo al cargar un snapshot de 128K, no pasa a la configuración, para no guardarlo sin querer. Si hay algo sin guardar, se avisa en la pantalla.

## Ocultar ficheros: excluir.txt

`excluir.txt`, en la raíz de la SD, dice qué ficheros y carpetas **no se muestran** en el selector (F1) ni en la web. Viene preparado para ocultar lo que necesita la Raspberry para arrancar: `bootcode.bin`, `start*.elf`, `fixup*.dat`, los `kernel*.img` (que si no aparecerían en el selector como imágenes de disco duro), `armstub*.bin`, los `*.dtb`, y las carpetas `firmware` y `certs`.

Formato, una entrada por línea (lo que va tras `#` se ignora):
- Si termina en **`/` o `\`**, es una **carpeta**; si no, un fichero.
- Si **empieza** por `/` o `\`, es una ruta desde la raíz de la SD (puede tener varias carpetas: `/juegos/viejos/`); si no, vale para ese nombre en cualquier carpeta.
- Se admiten los comodines `*` y `?`, y no distingue mayúsculas.

El fichero se vuelve a leer cada vez que se abre el selector, así que los cambios (también los hechos desde la web) se aplican sin reiniciar.

## Opciones mal escritas en cmdline.txt

Un valor mal escrito en `cmdline.txt` (por ejemplo `sounddev=sndpwn`) no da error: el emulador usa el valor por defecto. Al arrancar, ZXMetalCircle comprueba el fichero y avisa en pantalla durante unos 10 segundos ("sounddev=sndpwn no valido"; con "(+N)" si hay más problemas). También avisa de las opciones que no existen y de si `cmdline.txt` tiene más de una línea (la Raspberry solo lee la primera). Todos los problemas se escriben en el registro (`logdev=ttyS1`).
