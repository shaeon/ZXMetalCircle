# Actualizar el emulador

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

## Actualizar el emulador

Desde la versión 1.07, el emulador se puede actualizar solo, desde las versiones publicadas en GitHub. Está en la **página 10 de la configuración** (Alt+F1 y luego RePág/AvPág):

| Fila | Qué hace |
| --- | --- |
| Versión instalada | la que tienes |
| Buscar actualización | con ENTER, consulta GitHub; si hay una versión nueva, la fila pasa a decir «ENTER: instalar 1.07» |
| Paquete guardado | con ENTER, lista los paquetes de la carpeta `update` para volver a instalar uno (también una versión anterior) |
| Buscar al arrancar | `updboot=`: con `1`, la primera vez que hay red busca sola, en segundo plano (el Spectrum sigue funcionando mientras tanto), y dice el resultado: «Ya tienes la última», «Hay una versión nueva» o «No se pudo consultar GitHub». Desactivado por defecto |
| Paquetes guardados | `updkeep=`: `2` deja en `update` el de la versión instalada y el de la anterior (para poder volver); `all`, todos |
| Config. al actualizar | `updconfig=`: `keep` conserva tus ficheros de configuración; `new` pone los nuevos |

Cuando hay una versión nueva, la ayuda (F1) lo dice en la última línea: «Nueva versión: 1.07 (Alt+F1, pág. 10)».

**Cómo se instala:**
- Antes de empezar se graba lo pendiente (discos del +3, Microdrives…) y, si has cambiado la configuración sin guardarla, se pregunta.
- El paquete (el mismo zip de la release) se descarga a `update/` y se comprueba entero antes de tocar nada.
- Solo se copian los ficheros que han cambiado. Cada uno se escribe con otro nombre y luego se renombra, y los kernels se copian los últimos.
- **Tus ficheros de configuración no se sobrescriben**: `cmdline.txt`, `config.txt`, `excluir.txt`, `tnfs.txt` y la configuración de esxDOS. Si el paquete trae uno distinto, se deja al lado como `.nuevo` (por ejemplo `cmdline.txt.nuevo`) para que lo compares. Con «poner la nueva», se pone el del paquete y el tuyo queda como `.bak`.
- La carpeta `esxdos_i` se compara con el paquete de la versión anterior, para no volver a importar esxDOS si no ha cambiado.
- Al terminar, una pantalla dice cuántos ficheros han cambiado y qué ficheros de configuración se han dejado como `.nuevo` o `.bak`, y **la Pi se reinicia sola** a los 5 segundos: lo que hay en la SD ya no es lo que está en marcha. Si no ha cambiado nada, no se reinicia.

**Si algo falla:**
- Si la descarga falla o el paquete está dañado, no se toca nada.
- Si falla a mitad de la copia (la SD llena, un error de escritura), no se reinicia: se ofrece reintentar. Mientras no termine, existe `update/EN_CURSO` y al arrancar se avisa: «Actualización a medias: Alt+F1, pág. 10». Desde «Paquete guardado» se puede volver a instalar.
- Como último recurso, el paquete de `update/` es el mismo zip de la release: se puede descomprimir a mano en la SD desde un PC.

GitHub limita las consultas sin cuenta a 60 por hora; si se llega al límite, el emulador averigua la versión por la página de la release.
