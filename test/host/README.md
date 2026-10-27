# Pruebas en el PC

Compila la máquina (CPU, ULA, cinta, audio) para Linux/x86 sin Circle, con cabeceras mínimas
en `circle/`, y vuelca la pantalla a ficheros PPM (384x304, borde enorme).

```
sed 's#"roms/#"../../roms/#' ../../src/roms.S > roms_host.S
g++ -O2 -std=c++14 -DWITH_BREAKPOINT_SUPPORT -I. -I../../src -I../../third_party/z80cpp \
    -o harness harness.cpp ../../src/spectrum.cpp ../../src/ay8912.cpp ../../src/audiomixer.cpp \
    ../../src/tape.cpp ../../src/zxkeyboard.cpp ../../third_party/z80cpp/z80.cpp roms_host.S
./harness <modelo 0-2> <modo cinta 0-2> fichero.tap|tzx <cuadros>   # genera boot.ppm y end.ppm
```
