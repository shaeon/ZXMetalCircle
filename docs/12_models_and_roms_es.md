# Modelos y ROM

*Guía de ZXMetalCircle. Índice de guías: [docs](README.md). Qué es el emulador: [README](../README_es.md).*

## Microdigital TK90X y TK95

Los TK90X y TK95 de Microdigital (Brasil) eran clones del ZX Spectrum 48K. Se eligen con F7 o con `model=tk90x` / `model=tk95`.

- **ULA** (`tkula=`): la de Microdigital a 50 Hz (Uruguay; por defecto), la de Microdigital a 60 Hz (Brasil) o la Ferranti original (los primeros TK90X exportados). Cambian los tiempos de la imagen y el reloj (unos 3,58 MHz en la de Microdigital).
- **ROM del TK90X** (`tkrom=`): la v3 de Rodolfo Guerra (por defecto), o las originales v1 (la más difundida) y v2. El TK95 siempre lleva la v2.
- **Idioma de la ROM** (`tklang=`): «automático» sigue al idioma del menú. Las originales solo tienen español y portugués: la v1 los lleva los dos en la misma ROM y elige al arrancar con un diodo que tenían los TK90X vendidos en Brasil (el emulador lo simula con el portugués); la v2 y el TK95 tienen una ROM para cada idioma. La v3 está en 17 idiomas.
- **Puerto 254**: en los TK, los bits 5 y 7 no están conectados y dan lo que haya en el bus de datos (el byte que la ULA está leyendo de la memoria de vídeo, o 1 cuando no lee); con el diodo de los TK90X brasileños, el bit 7 vale 0. Algunos juegos que leen ese puerto sin enmascarar esos bits no funcionan en un TK real, y tampoco aquí.

**La ROM v3 de Rodolfo Guerra** (2021, no oficial) es el resultado de tres años de trabajo a partir de la v1, la más consistente, incorporando lo que aportaba la v2. Es la ROM más compatible para los TK hasta la fecha, e incluye, entre otras muchas mejoras: carga turbo nativa a 4000 baudios (sin perder la carga normal: la ROM detecta cuál viene en la cinta), un editor BASIC mejorado y una lectura del teclado más rápida. Para ganar espacio cada ROM tiene un solo idioma, y por eso hay una para cada uno de los 17. Todas sus novedades están en los manuales (en español, inglés y portugués) de su página: https://sites.google.com/view/rodolfoguerra . En el emulador, la carga turbo debería funcionar con la carga real de cintas, que reproduce la cinta a su velocidad; con la instantánea o la rápida, de momento solo se acelera la carga normal.

## ROMs

Solo las ROMs del **16K/48K y del 128K** van dentro del kernel, para que el emulador arranque siempre. Las demás se cargan al encender desde la carpeta **`roms`** de la SD, donde vienen incluidas:

| Fichero | Tamaño | Máquina o periférico |
| --- | --- | --- |
| `plus2.rom` | 32 KB | +2 gris |
| `plus2a.rom` | 64 KB | +2A |
| `plus3.rom` | 64 KB | +3 (y discos .DSK) |
| `plus3e-es.rom` / `plus3e-en.rom` | 64 KB | +3e (y disco duro); se elige con `p3elang=` |
| `if1.rom` | 8 KB | Interface 1 (microdrives, CP/M de LEC) |
| `mf1.rom`, `mf128.rom`, `mfplus3.rom` | 8 KB | Multiface One, 128 y 3 (F5); sin ellas, F5 hace una NMI normal |
| `esxmmc.bin` | 8 KB | esxDOS 0.8.9 para la divMMC (`divmmc=esxdos`) |
| `spectranet.rom` | 128 KB | Flash de la Spectranet con el firmware instalado (`spectranet=1`) |

**Si falta alguna** (o tiene otro tamaño), lo que la necesita deja de estar disponible y el resto funciona:
- F7 se salta esa máquina;
- `model=` arranca en 48K;
- el selector y la web avisan al cargar un `.DSK`, una imagen de disco duro o un microdrive;
- los snapshots de esa máquina se cargan en el 128K, con aviso.

Al arrancar aparece "Faltan ROMs: …". Todo está explicado en `roms/LEEME.txt` (`roms/README.txt` en inglés), por si alguien borra una ROM sin querer.

**ROMs propias**: `48.rom` y `128.rom` sustituyen a las incluidas en el kernel, y `plus3e.rom` tiene preferencia sobre las del +3e por idioma. Con `rom48=`, `rom128=`, `romplus2=`, `romplus2a=`, `romplus3=`, `romplus3e=` y `romif1=` se puede indicar otro fichero de la carpeta (por ejemplo `romplus3e=zxp3.rom`). Las trampas de cinta solo actúan si la ROM tiene las rutinas estándar en su sitio.
