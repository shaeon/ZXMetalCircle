# Pruebas en el PC

Compila la máquina (CPU, ULA, cinta, audio) para Linux/x86 sin Circle, con cabeceras mínimas
en `circle/`, y vuelca la pantalla a ficheros PPM (384x304, borde enorme).

```
sed 's#"roms/#"../../roms/#' ../../src/roms.S > roms_host.S
g++ -O2 -std=c++14 -DWITH_BREAKPOINT_SUPPORT -I. -I../../src -I../../third_party/z80cpp \
    -o harness harness.cpp ../../src/spectrum.cpp ../../src/ula.cpp ../../src/zxmemory.cpp ../../src/pagers.cpp ../../src/ay8912.cpp ../../src/audiomixer.cpp \
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
