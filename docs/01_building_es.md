# Compilar ZXMetalCircle en Linux y Windows

Esta guía explica cómo preparar un ordenador para compilar el emulador y generar la tarjeta SD, igual que hace la Action de GitHub.

- **Linux:** se compila directamente (probado en Ubuntu y Debian).
- **Windows:** se compila dentro de **WSL** (el Linux integrado en Windows 10 y 11). Los pasos dentro de WSL son los mismos que en Linux.
- **Atajo, en cualquiera de los dos:** si tienes **Docker**, puedes saltarte los pasos 1 a 3 y compilar con la misma imagen que usa la Action (ver «Compilar con Docker», más abajo).

Se necesitan unos **3 GB de disco** y conexión a internet: la primera compilación descarga Circle y el firmware de la Raspberry Pi.

---

## 1. Solo en Windows: instalar WSL

1. Abre **PowerShell como administrador** y escribe:
   ```
   wsl --install -d Ubuntu-24.04
   ```
2. Reinicia el PC si te lo pide.
3. Abre **Ubuntu** desde el menú Inicio. La primera vez te pide un nombre de usuario y una contraseña para Linux (no tienen por qué ser los de Windows).

A partir de aquí, **todo se escribe en la ventana de Ubuntu**, igual que en Linux.

> **Importante:** trabaja dentro de la carpeta de Linux (`~`, tu carpeta personal de Ubuntu), **no** en `/mnt/c/...` (los discos de Windows). Desde `/mnt/c` la compilación es muchísimo más lenta, y Git puede cambiar los finales de línea de los scripts y hacer que fallen con errores como `bad interpreter`.

---

## 2. Instalar las herramientas

En Linux (o en la ventana de Ubuntu de WSL):

```bash
sudo apt update
sudo apt install -y git wget make python3 mtools dosfstools zip xz-utils
```

| Herramienta | Para qué |
| --- | --- |
| git | descargar el código y Circle |
| wget | descargar el firmware de la Raspberry Pi |
| make | compilar |
| python3 | las herramientas de traducción (`tools/i18n`) |
| mtools, dosfstools | preparar las imágenes FAT de la SD |
| zip | empaquetar la SD |
| xz-utils | descomprimir el compilador |

---

## 3. Instalar el compilador (Arm GNU Toolchain 14.2)

El emulador se compila con el **Arm GNU Toolchain 14.2.rel1**: uno para 32 bits (`arm-none-eabi`) y otro para 64 bits (`aarch64-none-elf`). Es el mismo que usa la Action.

```bash
mkdir -p ~/arm-toolchain
cd ~/arm-toolchain
base=https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel
for t in arm-none-eabi aarch64-none-elf; do
  wget "$base/arm-gnu-toolchain-14.2.rel1-x86_64-$t.tar.xz"
  tar xf "arm-gnu-toolchain-14.2.rel1-x86_64-$t.tar.xz"
  rm "arm-gnu-toolchain-14.2.rel1-x86_64-$t.tar.xz"
done
```

Añádelo al `PATH` para que se encuentre siempre, también al abrir otra ventana:

```bash
for d in ~/arm-toolchain/*/bin; do echo "export PATH=\"$d:\$PATH\"" >> ~/.bashrc; done
source ~/.bashrc
```

Comprueba que responden los dos:

```bash
arm-none-eabi-gcc --version
aarch64-none-elf-gcc --version
```

Los dos deben decir `14.2.Rel1`.

> Si tu ordenador es un ARM (por ejemplo, un Mac con Apple Silicon o un Linux en ARM), cambia `x86_64` por `aarch64` en el nombre de los ficheros.

---

## 4. Descargar el código

```bash
cd ~
git clone https://github.com/shaeon/ZXMetalCircle.git
cd ZXMetalCircle
```

Para compilar la rama de desarrollo:

```bash
git checkout dev
```

---

## 5. Compilar

**Todos los modelos de Raspberry Pi** (lo mismo que la Action):

```bash
./build.sh
```

La primera vez descarga y compila Circle para cada modelo, y descarga el firmware: tarda bastante (de 15 a 30 minutos, según el ordenador). Las siguientes veces solo se recompila lo que ha cambiado.

**Solo algunos modelos**, mucho más rápido para probar:

```bash
./build.sh 3-64            # solo uno
./build.sh "1 3-64 5"      # varios
```

| Objetivo | Modelos | Bits | Kernel |
| --- | --- | --- | --- |
| `1` | Pi 1, Zero, Zero W | 32 | `kernel.img` |
| `2` | Pi 2 | 32 | `kernel7.img` |
| `3-32` | Pi 3, Zero 2 W | 32 | `kernel8-32.img` |
| `4-32` | Pi 4, 400 | 32 | `kernel7l.img` |
| `3-64` | Pi 3, Zero 2 W | 64 | `kernel8.img` |
| `4-64` | Pi 4, 400 | 64 | `kernel8-rpi4.img` |
| `5` | Pi 5, 500 | 64 | `kernel_2712.img` |

Al terminar dice:

```
Listo: copia el contenido de .../sdcard a una tarjeta FAT32.
```

y de cada kernel muestra cuánto ocupa (`Tamaño del kernel en memoria: ... (47 % de 4 MB)`).

Opciones útiles:

| Variable | Para qué |
| --- | --- |
| `JOBS=4 ./build.sh` | compilar con 4 procesos (por defecto, todos los del ordenador) |
| `QEMU=1 ./build.sh 3-64` | kernels para probar en QEMU (en la carpeta `qemu/`; no van a la SD) |

---

## 6. Copiar el resultado a la tarjeta SD

El contenido de la carpeta **`sdcard`** es lo que va en la raíz de una tarjeta formateada en **FAT32**.

- **En Linux:** copia **el contenido** de `sdcard` (no la carpeta) a la raíz de la tarjeta.
- **En Windows (WSL):** la carpeta está en `\\wsl$\Ubuntu-24.04\home\TU_USUARIO\ZXMetalCircle\sdcard`. Puedes abrirla en el Explorador de Windows escribiendo, dentro de la carpeta del proyecto en Ubuntu:
  ```bash
  explorer.exe sdcard
  ```
  y copiar **todo su contenido** a la raíz de la tarjeta.

Lo que **no** va en la SD que se genera: la tarjeta de esxDOS (`esxdos.img`), que crea el propio emulador.

---

## 7. Opcional: las pruebas en el PC

Las mismas pruebas que pasa la Action, sin necesidad de la Pi:

```bash
cd test/host
S=../../src
g++ -O2 -std=c++17 -I. -I$S -o update_test update_test.cpp $S/updater.cpp $S/zipfile.cpp $S/inflate.cpp
./update_test
```

Debe acabar con `todo OK`. Necesita el compilador normal del PC (`sudo apt install -y g++`). El resto de pruebas, y cómo se compilan, están en `.github/workflows/build.yml`.

## 8. Opcional: probar en QEMU

```bash
sudo apt install -y qemu-system-arm
QEMU=1 ./build.sh 3-64
```

QEMU necesita una imagen de tarjeta SD. Se crea una vez, con el contenido de `sdcard` (y los juegos que quieras añadir después con `mcopy`):

```bash
dd if=/dev/zero of=sd.img bs=1M count=256
mkfs.vfat -F 32 sd.img
mcopy -s -i sd.img sdcard/* ::/
```

Y se arranca así (las opciones de `cmdline.txt` van en `-append`):

```bash
qemu-system-aarch64 -M raspi3b -kernel qemu/kernel8.img -drive file=sd.img,if=sd,format=raw -device usb-kbd -append "model=128k"
```
 QEMU no emula el sonido ni la red, y es mucho más lento que una Pi real; sirve para comprobar menús, carga de ficheros y arranque.

---

## Compilar con Docker

La Action de GitHub compila dentro de una imagen Docker con todo ya instalado (las herramientas y el compilador). La misma imagen sirve en casa: no hace falta instalar nada más que Docker (en Windows, **Docker Desktop**).

1. Descarga el código **sin convertir los finales de línea** (si no, el script falla con `$'\r': command not found`):
   ```
   git clone -c core.autocrlf=false https://github.com/shaeon/ZXMetalCircle.git
   cd ZXMetalCircle
   ```
2. Compila, desde esa carpeta. En Linux, o en Windows con PowerShell:
   ```
   docker run --rm -v "${PWD}:/src" ghcr.io/shaeon/zxmetalcircle-build:1 ./build.sh
   ```
   Igual que en el paso 5, puedes indicar un solo modelo (por ejemplo, `./build.sh 4-64`).

El resultado queda en la carpeta `sdcard`, como al compilar sin Docker. La primera vez, Docker descarga la imagen, que ocupa unos 2 GB en el disco.

## Si algo falla

| Mensaje | Qué hacer |
| --- | --- |
| `Falta el compilador ...gcc` | el compilador no está en el `PATH`: repite el final del paso 3 y abre una ventana nueva |
| `bad interpreter` o `$'\r': command not found` | el código se descargó con finales de línea de Windows: clónalo dentro de WSL (paso 4), no en `C:` |
| `mcopy: command not found` | falta `mtools` (paso 2) |
| Se corta al descargar Circle o el firmware | revisa la conexión y vuelve a lanzar `./build.sh`; si sigue fallando, borra la carpeta `build` (abajo) y empieza de cero |
| `ERROR: el kernel ocupa ... (máximo 4 MB)` | el kernel ha crecido demasiado: hay que mover búferes grandes a memoria dinámica |

Para empezar de cero (por ejemplo, tras cambiar de versión de Circle), borra la carpeta `build`:

```bash
rm -rf build sdcard
./build.sh
```

---

## Más detalles

### Compilar

Hacen falta los dos toolchains de la Arm GNU Toolchain (probado con la 14.2): `arm-none-eabi` para los kernels de 32 bits y `aarch64-none-elf` para los de 64 bits, además de `git`, `wget`, `make`, `python3`, `mtools` y `dosfstools`:

```
./build.sh              # compila todos los kernels y deja todo en sdcard/
./build.sh 3-64         # solo uno
./build.sh "1 3-64 5"   # varios
```

**En GitHub** (`.github/workflows/build.yml`) se compila solo: en cada cambio subido a `main` y en los pull requests pasa las pruebas en el PC, compila los siete kernels y deja la SD como zip en la pestaña *Actions* (apartado *Artifacts* de cada ejecución). También se puede lanzar a mano (*Actions → Build → Run workflow*). Al subir una etiqueta `vX.YY` crea además un borrador de release con ese zip: solo falta pegar las notas y publicarlo.

El script descarga Circle y el firmware de la Raspberry, compila las librerías de Circle para cada objetivo y genera estos kernels:

| Objetivo | Modelo | Bits | Kernel |
| --- | --- | --- | --- |
| `1` | Pi 1 / Zero / Zero W | 32 | `kernel.img` |
| `2` | Pi 2 | 32 | `kernel7.img` |
| `3-32` | Pi 3 / Zero 2 W | 32 | `kernel8-32.img` |
| `4-32` | Pi 4 / 400 | 32 | `kernel7l.img` |
| `3-64` | Pi 3 / Zero 2 W | 64 | `kernel8.img` |
| `4-64` | Pi 4 / 400 | 64 | `kernel8-rpi4.img` |
| `5` | Pi 5 / 500 | 64 | `kernel_2712.img` |

Las rutas de los compiladores se pueden cambiar con `PREFIX32=` y `PREFIX64=`.
