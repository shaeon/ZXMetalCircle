# Pruebas en el PC

Compila la máquina (CPU, ULA, cinta, audio) para Linux/x86 sin Circle, con cabeceras mínimas
en `circle/`, y vuelca la pantalla a ficheros PPM (384x304, borde enorme).

```
sed 's#"roms/#"../../roms/#' ../../src/roms.S > roms_host.S
g++ -O2 -std=c++14 -DWITH_BREAKPOINT_SUPPORT -I. -I../../src -I../../third_party/z80cpp \
    -o harness harness.cpp ../../src/spectrum.cpp ../../src/ula.cpp ../../src/zxmemory.cpp ../../src/pagers.cpp ../../src/divmmc.cpp ../../src/ay8912.cpp ../../src/audiomixer.cpp \
    ../../src/tape.cpp ../../src/zxkeyboard.cpp ../../third_party/z80cpp/z80.cpp roms_host.S
./harness <modelo 0-2> <modo cinta 0-2> fichero.tap|tzx <cuadros>   # genera boot.ppm y end.ppm
```

## Pruebas de referencia (golden_test)

`golden_test.cpp` ejecuta siempre lo mismo en todos los modelos (arrancar, unas teclas, el
programa de `bench=1`, cargar una cinta, grabar en el disco del +3, un snapshot con ULAplus)
y saca de cada prueba una huella de la pantalla, la RAM, el sonido, los registros y los
puertos. Tienen que coincidir con `golden/huellas.txt`: es la red de seguridad de la
reestructuración en una capa de máquinas (la Action la pasa en cada subida).

```
./golden_test            # compara
./golden_test --ppm      # y vuelca la pantalla final de cada prueba (golden_*.ppm)
./golden_test --grabar   # rehace golden/huellas.txt: solo si el cambio es a propósito
```

## Periféricos que tapan la memoria

Con comprobaciones (`ok` / `FALLO`) y en la Action. Usan `testutil.h` (un Spectrum con su
teclado y su cinta, teclas, ROMs de `roms/` y lo que hay en la pantalla) y se ejecutan desde
`test/host`.

- `multiface_test.cpp`: MF1, MF128 y MF3 con sus ROMs de `roms/`: la NMI saca el menú y R
  vuelve a lo de antes; sin ROM o con `multiface=0` no hay Multiface.
- `lec_test.cpp`: las páginas de la LEC, la 15 (la RAM alta), el reset y la desconexión.
- `cartridge_test.cpp`: cartuchos del Interface 2 (`test_if2.rom`, y sus 8 KB) en el 48K, el
  128K y el +2; el +2A los rechaza.
- `divmmc_test.cpp`: la divMMC con `roms/esxmmc.bin` (CONMEM, bancos, MAPRAM, paginación
  automática, reset), la tarjeta por SPI con la divMMC y con la ZXMMC del +3e (una imagen en
  memoria: CMD0, ACMD41, leer y escribir un sector), el reloj de esxDOS y el arranque de
  esxDOS sin tarjeta. Las pruebas de `divmmc/` son herramientas a mano con una imagen de
  tarjeta (ver sus comentarios).
- `if1_snet_test.cpp`: cómo entran y salen la ROM del Interface 1 (0x0008, 0x1708, 0x0700;
  reset, desconexión, +3, divMMC) y la Spectranet (arranque, 0x0008, 0x3FF8, 0x007C, la NMI
  en 0x0066, RETN frente a RETI y la trampa programable).
- `tapedeck_test.cpp`: el casete con una cinta .TAP hecha en la prueba: carga instantánea,
  rápida (con y sin aceleración de cargadores) y normal, arranque y parada automáticos,
  parada manual, SAVE instantáneo y grabación por pulsos de la salida MIC.
- `filetypes_test.cpp`: la lista de ficheros del Spectrum (`filetypes.h`) da lo mismo que las
  listas que había antes en el navegador, la web, los ZIP, la pantalla de unidades y
  `LoadAnyFile`.
- `state_test.cpp`: el estado a través de `CMachine` (F8 y cargar snapshots): en el 48K, el 128K
  y el +3, con .Z80, .SNA y .SZX, grabar, cargar en otra máquina y seguir igual; con la LEC en
  uso no se graba.
- `input_test.cpp`: la entrada (`hostinput.h` y `zxkeyboard.h`) con 20000 pasos fijos de teclados
  USB, errores, Alt, teclas de función, menús, joysticks y mandos; la huella de todo lo que sale
  es la del teclado de antes de separar la parte genérica (fase 3d).
- `cpm_test.cpp`: el CP/M de LEC (`boot/cpm/cpm22_lec.tzx`) arranca en el 48K y en el 128K
  en modo 48K, y responde a DIR.
