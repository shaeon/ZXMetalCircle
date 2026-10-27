# DIVMMC.BIN (CP/M 3 de Otivax) con detección SDSC/SDHC — trabajo terminado

Documento para pasar a otra conversación. Contiene el parche ya hecho y probado
en emulación, el fichero resultante, el script que lo aplica (a DIVMMC.BIN o
ZXMMC.BIN) y el banco de pruebas.

## Resumen

- Controlador: "DivMMC driver v0.8 (c) otivax 2017", formato PRL (FID del CP/M
  Plus del +3e). Antes direccionaba siempre en bytes (sector × 512), así que con
  SDHC leía sectores equivocados (`DIR` vacío).
- Ahora, antes de cada lectura/escritura, manda **CMD58 (READ_OCR)** y mira el
  bit **CCS** (bit 30 del OCR): SDHC → argumento = nº de sector; SDSC → nº × 512.
- **El fichero sigue midiendo 1664 bytes**: el código crece 19 bytes
  (0x4D0 → 0x4E3) y el mapa de reubicación (157 bytes) cabe en el relleno que
  sobraba. El cargador no nota diferencia de tamaño.
- Original:   MD5 `d70e9a6a98aec2007acc8063c4df06b6`
- Parcheado:  MD5 `821f898137beefb49d607110aa13e09c`

## Código nuevo (direcciones antes de reubicar = posición en el fichero)

```
; 0x03AA: sustituye a la multiplicación DEHL*512 (mismos 16 bytes)
03AA  C5        push bc
03AB  CD D0 05  call 05D0        ; vuelve con S=1 -> SDHC, S=0 -> SDSC
03AE  C1        pop bc
03AF  F8        ret m            ; SDHC: DEHL = nº de sector, tal cual
03B0  29        add hl,hl        ; SDSC: DEHL = DEHL * 512
03B1  CB 13     rl e
03B3  53        ld d,e
03B4  5C        ld e,h
03B5  65        ld h,l
03B6  2E 00     ld l,0
03B8  C9        ret
03B9  C9        (sin usar)

; 0x05D0: añadido al final del código
05D0  06 00     ld b,0           ; hasta 256 intentos
05D2  3E 7A     ld a,7Ah         ; CMD58 (el argumento da igual: DEHL tal cual)
05D4  CD EF 03  call 03EF        ; A = R1, flags de "cp 0"
05D7  C0        ret nz           ; R1<>0 (MMC / v1, orden ilegal): S=0 -> SDSC
05D8  DB EB     in a,(EBh)       ; OCR[31:24]  (0x3F en ZXMMC)
05DA  87        add a,a          ; C = bit 31 (tarjeta lista), S = bit 30 (CCS)
05DB  38 02     jr c,05DF        ; OCR válido
05DD  10 F3     djnz 05D2        ; tarjeta ocupada (tras escribir): repetir
05DF  CD 25 04  call 0425        ; 4 lecturas: vacía el resto del OCR (no toca flags)
05E2  C9        ret
```

Bits nuevos en el mapa de reubicación (bytes altos de las CALL nuevas):
**0x03AD, 0x05D6, 0x05E1**. Se conservan todos los originales (98). Orden de
bits PRL comprobado: el bit 7 del primer byte corresponde al primer byte de
código (0x100). Los únicos bytes de código cambiados son 0x03AB–0x03B8.

### Por qué así

- **Sin byte de estado**: no cabía en 1664 bytes y además evita que un tipo mal
  detectado quede guardado. CMD58 en cada acceso cuesta ~16 bytes SPI por cada
  512 de datos (≤ 3 % más lento). La ROM del +3e hace lo mismo.
- **Comprobación del bit 31**: el controlador original no espera al fin de la
  escritura. Con la tarjeta ocupada, la línea se lee 00, lo que parecería
  "R1 = 0, CCS = 0" y haría tratar una SDHC como SDSC justo tras escribir
  (riesgo de escribir en otro sector). Con el bit 31 a 0 se repite CMD58.
- **BC**: se guarda alrededor de la detección; la lectura y la escritura ya lo
  destruían (inir/otir), pero así el camino de error lo conserva como antes.
- **A** no hace falta conservarlo: quien llama a 0x03AA carga A con la orden.
- La multiplicación nueva es equivalente para todo sector < 2^23 (≤ 4 GB en
  bytes; las SDSC llegan a 2 GB).

## Pruebas hechas (emulación, Z80 + tarjeta SD simulada por SPI)

| Caso | Original | Parcheado |
| --- | --- | --- |
| SDSC, lecturas | OK | OK |
| SDHC, lecturas | Lee sector × 512 (falla) | OK |
| Escritura + relectura inmediata con tarjeta ocupada | Falla la relectura (no espera) | OK |
| Tarjeta sin CMD58 (R1 = 05) | — | OK (tratada como SDSC) |
| PRL reubicado en 0x8000 y 0xC000 | OK | OK |
| Puerto de lectura "segmentado" (devuelve el byte anterior) y "directo" | — | OK |
| ZXMMC sintético (puertos 1F/3F) | — | OK |

**Falta probar en real**: core zxp3 de MiST (`allow_sdhc` 0 y 1), ZX-Uno con
tarjeta ≤ 2 GB y > 2 GB, y ZXMetalCircle.

## Requisitos para el emulador bare-metal (ZXMetalCircle)

Para que este controlador funcione, la tarjeta emulada debe:

1. Responder a **CMD58** con R1 = 0x00 seguido de 4 bytes de OCR, con el
   **bit 31 a 1** (tarjeta encendida y lista). Si falta, cada acceso hace 256
   reintentos y luego decide con el bit 30 que haya leído.
2. Poner el **bit 30 (CCS)** según el tipo que presente: 1 si es SDHC (imágenes
   > 2 GB, o forzado), 0 si es SDSC.
3. Interpretar el argumento de CMD17/CMD24 en **bytes** si es SDSC y en
   **sectores** si es SDHC, coherente con el CCS anunciado.
4. Tras CMD24, si emula "ocupado" (DO = 00), debe mantenerlo también al volver a
   seleccionar la tarjeta, y acabar en algún momento (el controlador reintenta).

Ejemplo de respuesta OCR: SDHC → `C0 FF 80 00`; SDSC → `80 FF 80 00`.

## Cómo instalarlo en la imagen

```
3e imagen.img get 1:DIVMMC.BIN          # copia de seguridad
3e imagen.img del 1:DIVMMC.BIN
3e imagen.img put DIVMMC.BIN 1:DIVMMC.BIN
```

(`3e`: https://github.com/ZXSpectrumVault/zx-3e; los .BIN van sin cabecera
+3DOS.) Arrancar el +3e, elegir "CP/M 3.0"; debe salir el mensaje del
controlador, "Drive C: installed…" y `DIR` debe listar `8BIT BIN`,
`BASIC COM`, `CPM BAS`… con los dos tipos de tarjeta.

## Fichero parcheado (DIVMMC.BIN), en base64

Recuperar con `base64 -d > DIVMMC.BIN` (MD5 `821f898137beefb49d607110aa13e09c`).

```
AOMEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMOuAlNQRUNUUlVNRklECAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHAAAAUExVU0lERURPUwBDUC9NAFNPRlRXQVJFAAIqNQEB
IAAJESABAQgA7bAqJgHLPMsdyzzLHcs8yx0iMAEqNQEBQAAJ5d3h3X4Q/gMgHRFDAT4CMjcBzaAC
KEYRSAE+AzI3Ac2gAig53eXhAUAACTo0AT0yNAEgzCEwATUoGCEyATQRAAD9KjUBzWQDPggyNAEq
NQEYriGv+T4AvtghMwXJ3W4l3WYmOjcBR81MAtDNPwIRxgX9ISwBIbMFOjcB/gIgEBGjBf0hKAEy
2vkyr/khkAUGQYB33W4X3WYYzaAE3X4T3V4R3VYSzWIE/XUA/XQB/XMC/XIDIVEBNcKeAT4AMqsF
MqwFN8nd5eEBIAAJAREA7bDJ3eUjyzzLHRECA90hAAD9IQAAzQX/3eXR3eHY5gEh+ATAIQQFySEA
ABEAAP0qNQHNZAMqNQEROAHNoALIEQEAKiMBI/0qNQHNZAP95eEROAHNoALJ5esaviAGIxN+tyD2
4ck62vnm/ygFIU0Ft8k+/9PnzSUEAD720+cAAADb6/7/IAUh2QS3yREAAs0H/zgEIfgEySI1Ac1x
AigFIRMFGBPNUgEwDio1AREAAs0J/yFoBTfJt8nDEQPDFAPDMwPDUgPDVQOvN8ndISgBPgK4KATd
ISwBzT8EzWQDIAcGBT4B9gHJrzfJ3SEoAT4CuCgE3SEsAc0/BM26AyAHBgU+AfYBya83yTevyT4F
uCAFIc4EN8kh6gQ3yc2qAz5Rze8D/gDAzZgD/v4oA+YAyf3l4fMO6wYA7bIA7bL7ANvrAADb6z7/
0+fNJQT2AcnFBgrNLgT+/igG/v8gAhDzwcnFzdAFwfgpyxNTXGUuAMnJzaoDPljN7wP+AMA+/tPr
/eXh8w7rBgDtswDts/sAPpXT6wAA0+vNLgTmH/4FIAP2AcnmAMn1Pv/T580lBAA+9tPn8dPregDT
63sA0+t8ANPrfQDT6z7/ANPrzS4E/gAgAcn1Pv/T59vr8cnFBgTb6xD8wcnFATIA2+v+/yAFEPgN
IPXByQGAAOvlzYwE0esZ3VYB3V4AGeURAADdZgPdbgLtWuvh0BPJ1e1bIwEWAM16BNHl7UskAc2M
BMEJ0BPJxQYIIQAADzABGcsjyxIQ9sHJIQAAPhApyxPLEjAECTABEz0g8snFI8s8yx3NqwTByQHw
2M3DBAEY/M3DBAGc/83DBA72zcMESD4vPAk4/O1CEhPJSEQgdGltZW91dP9ObyBIRCBkZXRlY3Rl
ZA0K/1Vua25vd24gZXJyb3L/Tm8gbWVtb3J5DQr/RHJpdmUgaW4gdXNlDQr/Tm8gUExVU0lERURP
UyBwYXJ0aXRpb24gZm91bmQNCv9ObyBDUC9NIHBhcnRpdGlvbiBmb3VuZA0K/0RyaXZlciBhbHJl
YWR5IGluc3RhbGxlZA0K/0Rpdk1NQyBkcml2ZXIgdjAuOCCkIG90aXZheCAyMDE3DQpEcml2ZSAA
OiBpbnN0YWxsZWQuIFNpemU9MDAwMDBLDQoK/0RyaXZlIAA6IGluc3RhbGxlZC4gU2l6ZT0wMDAw
MEsNCgr/BgA+es3vA8Db64c4AhDzzSUEySAAAAAAAAAAAAAIEBAAJAACEhCQARCCAkJABAERIkgR
AIAEAEAAiEgAAAQAgBEAJJIEJBIAAAEAgABAQiQhCCQJJIEAkgACASQAAAhCEEAAAAAIBAAEAAhA
AAAAIAACAAAAAgAAAAAAAgAAAAIQSAAAAAAAAICCCEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAACAEA=
```

## Fichero original (DIVMMC.BIN), en base64

MD5 `d70e9a6a98aec2007acc8063c4df06b6`.

```
ANAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMOuAlNQRUNUUlVNRklECAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHAAAAUExVU0lERURPUwBDUC9NAFNPRlRXQVJFAAIqNQEB
IAAJESABAQgA7bAqJgHLPMsdyzzLHcs8yx0iMAEqNQEBQAAJ5d3h3X4Q/gMgHRFDAT4CMjcBzaAC
KEYRSAE+AzI3Ac2gAig53eXhAUAACTo0AT0yNAEgzCEwATUoGCEyATQRAAD9KjUBzWQDPggyNAEq
NQEYriGv+T4AvtghMwXJ3W4l3WYmOjcBR81MAtDNPwIRxgX9ISwBIbMFOjcB/gIgEBGjBf0hKAEy
2vkyr/khkAUGQYB33W4X3WYYzaAE3X4T3V4R3VYSzWIE/XUA/XQB/XMC/XIDIVEBNcKeAT4AMqsF
MqwFN8nd5eEBIAAJAREA7bDJ3eUjyzzLHRECA90hAAD9IQAAzQX/3eXR3eHY5gEh+ATAIQQFySEA
ABEAAP0qNQHNZAMqNQEROAHNoALIEQEAKiMBI/0qNQHNZAP95eEROAHNoALJ5esaviAGIxN+tyD2
4ck62vnm/ygFIU0Ft8k+/9PnzSUEAD720+cAAADb6/7/IAUh2QS3yREAAs0H/zgEIfgEySI1Ac1x
AigFIRMFGBPNUgEwDio1AREAAs0J/yFoBTfJt8nDEQPDFAPDMwPDUgPDVQOvN8ndISgBPgK4KATd
ISwBzT8EzWQDIAcGBT4B9gHJrzfJ3SEoAT4CuCgE3SEsAc0/BM26AyAHBgU+AfYBya83yTevyT4F
uCAFIc4EN8kh6gQ3yc2qAz5Rze8D/gDAzZgD/v4oA+YAyf3l4fMO6wYA7bIA7bL7ANvrAADb6z7/
0+fNJQT2AcnFBgrNLgT+/igG/v8gAhDzwcnFBgm3yxXLFMsTyxIQ9sHJzaoDPljN7wP+AMA+/tPr
/eXh8w7rBgDtswDts/sAPpXT6wAA0+vNLgTmH/4FIAP2AcnmAMn1Pv/T580lBAA+9tPn8dPregDT
63sA0+t8ANPrfQDT6z7/ANPrzS4E/gAgAcn1Pv/T59vr8cnFBgTb6xD8wcnFATIA2+v+/yAFEPgN
IPXByQGAAOvlzYwE0esZ3VYB3V4AGeURAADdZgPdbgLtWuvh0BPJ1e1bIwEWAM16BNHl7UskAc2M
BMEJ0BPJxQYIIQAADzABGcsjyxIQ9sHJIQAAPhApyxPLEjAECTABEz0g8snFI8s8yx3NqwTByQHw
2M3DBAEY/M3DBAGc/83DBA72zcMESD4vPAk4/O1CEhPJSEQgdGltZW91dP9ObyBIRCBkZXRlY3Rl
ZA0K/1Vua25vd24gZXJyb3L/Tm8gbWVtb3J5DQr/RHJpdmUgaW4gdXNlDQr/Tm8gUExVU0lERURP
UyBwYXJ0aXRpb24gZm91bmQNCv9ObyBDUC9NIHBhcnRpdGlvbiBmb3VuZA0K/0RyaXZlciBhbHJl
YWR5IGluc3RhbGxlZA0K/0Rpdk1NQyBkcml2ZXIgdjAuOCCkIG90aXZheCAyMDE3DQpEcml2ZSAA
OiBpbnN0YWxsZWQuIFNpemU9MDAwMDBLDQoK/0RyaXZlIAA6IGluc3RhbGxlZC4gU2l6ZT0wMDAw
MEsNCgr/IAAAAAAAAAAAAAgQEAAkAAISEJABEIICQkAEAREiSBEAgAQAQACISAAABACAEQAkkgQk
EgAAAQCAAEBCJCEIJAkkgQCSAAIBJAAACEIQQAAAAAgEAAAACEAAAAAgAAIAAAACAAAAAAACAAAA
AhBIAAAAAAAAgIIIQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAA=
```

## Script de parcheo: parchear_sdhc.py

Uso: `python3 parchear_sdhc.py ORIGINAL.BIN SALIDA.BIN`. Vale para DIVMMC.BIN
y ZXMMC.BIN (saca el puerto de datos del propio fichero, en 0x02CA) y se niega
a tocar un fichero que no coincida con lo esperado o ya esté parcheado.

```python
#!/usr/bin/env python3
"""
Parche SDSC/SDHC para el controlador CP/M 3 de Otivax (DIVMMC.BIN / ZXMMC.BIN).

Uso:  python3 parchear_sdhc.py ORIGINAL.BIN SALIDA.BIN

- Comprueba que el fichero es el controlador esperado (PRL, longitud 0x4D0,
  bytes originales de 0x03AA, 0x03EF y 0x0425).
- Saca el puerto de datos del propio fichero (0xEB en DivMMC, 0x3F en ZXMMC).
- Reescribe 0x03AA y añade una rutina de 19 bytes al final del código.
- Regenera cabecera y mapa de reubicación. El tamaño sigue siendo 1664 bytes.

Rutina nueva (las direcciones son las de antes de reubicar):

  03AA  C5        push bc
  03AB  CD D0 05  call 05D0        ; S=1 -> SDHC, S=0 -> SDSC
  03AE  C1        pop bc
  03AF  F8        ret m            ; SDHC: DEHL = nº de sector, tal cual
  03B0  29        add hl,hl        ; SDSC: DEHL = DEHL * 512
  03B1  CB 13     rl e
  03B3  53        ld d,e
  03B4  5C        ld e,h
  03B5  65        ld h,l
  03B6  2E 00     ld l,0
  03B8  C9        ret
  03B9  C9        (sin usar)

  05D0  06 00     ld b,0           ; hasta 256 intentos
  05D2  3E 7A     ld a,7Ah         ; CMD58 READ_OCR (el argumento da igual)
  05D4  CD EF 03  call 03EF        ; A = R1, flags de "cp 0"
  05D7  C0        ret nz           ; R1<>0 (p. ej. orden ilegal en MMC): S=0 -> SDSC
  05D8  DB pp     in a,(datos)     ; OCR[31:24]
  05DA  87        add a,a          ; C = bit 31 (encendida/lista), S = bit 30 (CCS)
  05DB  38 02     jr c,05DF        ; OCR válido
  05DD  10 F3     djnz 05D2        ; si no (tarjeta ocupada tras escribir), repetir
  05DF  CD 25 04  call 0425        ; 4 lecturas: vacía el resto del OCR (no toca flags)
  05E2  C9        ret
"""
import sys, hashlib

ORIG_03AA = bytes.fromhex("C50609B7CB15CB14CB13CB1210F6C1C9")


def bit(bm, i):
    return (bm[i >> 3] >> (7 - (i & 7))) & 1


def parchear(d):
    d = bytearray(d)
    L = d[1] | d[2] << 8
    assert len(d) % 128 == 0, "tamaño no múltiplo de 128"
    assert L == 0x4D0, f"longitud de código inesperada: {L:#x}"
    assert d[0x103:0x10E] == b"SPECTRUMFID", "falta la firma SPECTRUMFID"
    assert d[0x3AA:0x3BA] == ORIG_03AA, "0x03AA no es la rutina original (¿ya parcheado?)"
    assert d[0x3EF] == 0xF5 and d[0x425] == 0xC5, "0x03EF / 0x0425 no son lo esperado"
    assert d[0x2C9] == 0xDB, "no encuentro IN A,(n) en 0x02C9"
    puerto = d[0x2CA]
    assert d[0x3FD:0x3FF] == bytes([0xD3, puerto]), "puerto de datos incoherente"

    code = bytearray(d[0x100:0x100 + L])
    bm_old = d[0x100 + L:0x100 + L + (L + 7) // 8]
    rel = {i for i in range(L) if bit(bm_old, i)}

    # Ninguna reubicación debe caer en 0x03AA-0x03B9
    assert not any(0x3AA - 0x100 <= i <= 0x3B9 - 0x100 for i in rel)

    nuevo_det = 0x100 + L  # 0x05D0
    dlo, dhi = nuevo_det & 0xFF, nuevo_det >> 8

    p03aa = bytes([0xC5, 0xCD, dlo, dhi, 0xC1, 0xF8,
                   0x29, 0xCB, 0x13, 0x53, 0x5C, 0x65, 0x2E, 0x00, 0xC9, 0xC9])
    code[0x3AA - 0x100:0x3BA - 0x100] = p03aa
    rel.add(0x3AD - 0x100)   # byte alto de CALL 05D0

    det = bytes([0x06, 0x00,
                 0x3E, 0x7A,
                 0xCD, 0xEF, 0x03,
                 0xC0,
                 0xDB, puerto,
                 0x87,
                 0x38, 0x02,
                 0x10, 0xF3,
                 0xCD, 0x25, 0x04,
                 0xC9])
    base = len(code)
    code += det
    rel.add(base + 6)    # byte alto de CALL 03EF
    rel.add(base + 17)   # byte alto de CALL 0425

    L2 = len(code)
    bm = bytearray((L2 + 7) // 8)
    for i in rel:
        bm[i >> 3] |= 0x80 >> (i & 7)

    hdr = bytearray(256)
    hdr[1], hdr[2] = L2 & 0xFF, L2 >> 8
    out = hdr + code + bm
    out += bytes((-len(out)) % 128)
    return bytes(out), puerto


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src = open(sys.argv[1], "rb").read()
    out, puerto = parchear(src)
    open(sys.argv[2], "wb").write(out)
    print(f"{sys.argv[1]}: puerto de datos {puerto:#04x}, {len(src)} -> {len(out)} bytes")
    print(f"MD5 {hashlib.md5(out).hexdigest()}  {sys.argv[2]}")
```

## Banco de pruebas: prueba.py

Necesita `pip install z80` (emulador de Ivan Kosarev). Uso:
`python3 prueba.py DIVMMC.BIN:e7:eb` (fichero:puerto_control:puerto_datos;
para ZXMMC, `:1f:3f`). Prueba SDSC/SDHC, con y sin tarjeta ocupada tras
escribir, y los dos modos del puerto de lectura. Tarda unos minutos.

```python
#!/usr/bin/env python3
# Banco de pruebas: ejecuta el controlador reubicado en un Z80 emulado con una
# tarjeta SD simulada en SPI (SDSC o SDHC) y comprueba lecturas y escrituras.
import sys, random, z80

BASE = 0x8000          # página donde se carga el PRL
OFS = (BASE >> 8) - 1  # desplazamiento de reubicación PRL


def cargar(path, mem):
    d = open(path, "rb").read()
    L = d[1] | d[2] << 8
    code = bytearray(d[0x100:0x100 + L])
    bm = d[0x100 + L:]
    for i in range(L):
        if (bm[i >> 3] >> (7 - (i & 7))) & 1:
            code[i] = (code[i] + OFS) & 0xFF
    mem[BASE:BASE + L] = code


def dir_(a):  # dirección del fichero -> dirección reubicada
    return a - 0x100 + BASE


class SD:
    def __init__(self, sdhc, data_port, ctl_port, busy_after_write=0, pipelined=True):
        self.sdhc = sdhc
        self.dp, self.cp = data_port, ctl_port
        self.sel = False
        self.cmd = []
        self.out = []            # bytes pendientes de enviar
        self.busy = 0            # bytes de "ocupado" pendientes
        self.busy_after_write = busy_after_write
        self.rx = []             # recepción de datos de escritura
        self.wr_addr = None
        self.last = 0xFF
        self.pipelined = pipelined
        self.sectors = {}
        self.log = []

    def sector(self, n):
        if n not in self.sectors:
            r = random.Random(n)
            self.sectors[n] = bytes(r.randrange(256) for _ in range(512))
        return self.sectors[n]

    def xfer(self, b):
        if not self.sel:
            return 0xFF
        if self.busy:
            self.busy -= 1
            return 0x00
        if self.wr_addr is not None:          # fase de datos de escritura
            if self.out:
                return self.out.pop(0)
            if not self.rx and b != 0xFE:
                return 0xFF
            self.rx.append(b)
            if len(self.rx) == 1 + 512 + 2:
                self.sectors[self.wr_addr] = bytes(self.rx[1:513])
                self.log.append(("W", self.wr_addr))
                self.wr_addr, self.rx = None, []
                self.out = [0xE5]             # xxx0 0101 -> aceptado
                self.pend_busy = True
            return 0xFF
        r = self.out.pop(0) if self.out else 0xFF
        if r == 0xE5 and getattr(self, "pend_busy", False):
            self.pend_busy = False
            self.busy = self.busy_after_write
        if not self.cmd and (b & 0xC0) != 0x40:
            return r
        self.cmd.append(b)
        if len(self.cmd) == 6:
            self.ejecutar()
            self.cmd = []
        return r

    def ejecutar(self):
        c = self.cmd[0] & 0x3F
        arg = int.from_bytes(bytes(self.cmd[1:5]), "big")
        self.log.append((c, arg))
        if c == 58:
            ocr0 = 0x80 | (0x40 if self.sdhc else 0)
            self.out = [0xFF, 0x00, ocr0, 0xFF, 0x80, 0x00]
        elif c in (17, 24):
            if self.sdhc:
                n = arg
            else:
                assert arg % 512 == 0, f"dirección SDSC no alineada {arg:#x}"
                n = arg // 512
            if c == 17:
                self.out = [0xFF, 0x00, 0xFF, 0xFF, 0xFE] + list(self.sector(n)) + [0, 0]
                self.log.append(("R", n))
            else:
                self.out = [0xFF, 0x00]
                self.wr_addr = n
        else:
            self.out = [0xFF, 0x04]           # orden ilegal

    # puertos
    def outp(self, port, v):
        p = port & 0xFF
        if p == self.cp:
            sel = (v & 1) == 0
            if not sel:
                self.out = []
                self.cmd = []
            self.sel = sel
        elif p == self.dp:
            self.last = self.xfer(v)

    def inp(self, port):
        p = port & 0xFF
        if p != self.dp:
            return 0xFF
        if self.pipelined:
            r = self.last
            self.last = self.xfer(0xFF)
            return r
        return self.xfer(0xFF)


def llamar(m, addr, de, hl, iy):
    STOP = 0x7000
    m.memory[STOP] = 0x76  # HALT
    m.sp = 0x6FF0
    m.memory[0x6FF0] = STOP & 0xFF
    m.memory[0x6FF1] = STOP >> 8
    m.sp = 0x6FF0
    m.halted = False
    m.pc, m.de, m.hl, m.iy, m.bc = addr, de, hl, iy, 0x1234
    for _ in range(2000):
        m.ticks_to_stop = 100000
        m.run()
        if m.pc == STOP + 1 or (m.memory[m.pc - 1] == 0x76 and m.pc - 1 == STOP):
            return m.f, m.a, m.bc
    raise RuntimeError("no vuelve")


def probar(fich, sdhc, ctl, dat, busy=0, pipelined=True, verbose=False):
    m = z80.Z80Machine()
    cargar(fich, m.memory)
    sd = SD(sdhc, dat, ctl, busy, pipelined)
    m.set_input_callback(lambda port: sd.inp(port))
    m.set_output_callback(lambda port, v: sd.outp(port, v))
    BUF = 0x6000
    ok = True
    # 1) lecturas de varios sectores
    for n in (0, 1, 2, 0x1234, 0x3FFFFF if not sdhc else 0x01234567):
        m.memory[BUF:BUF + 512] = bytes(512)
        f, a, bc = llamar(m, dir_(0x364), n >> 16, n & 0xFFFF, BUF)
        got = bytes(m.memory[BUF:BUF + 512])
        good = got == sd.sector(n) and (f & 0x40) == 0
        ok &= good
        if verbose or not good:
            print(f"   leer {n:#x}: {'OK' if good else 'FALLO'}")
    # 2) escritura y relectura (con la tarjeta ocupada justo después de escribir)
    n = 0x777
    pat = bytes((i * 7 + 3) & 0xFF for i in range(512))
    m.memory[BUF:BUF + 512] = pat
    f, a, bc = llamar(m, dir_(0x3BA), n >> 16, n & 0xFFFF, BUF)
    wok = sd.sectors.get(n) == pat and (f & 0x40) == 0
    if verbose or not wok:
        print(f"   escribir {n:#x}: {'OK' if wok else 'FALLO'}")
    ok &= wok
    m.memory[BUF:BUF + 512] = bytes(512)
    # lectura inmediatamente después: si la tarjeta sigue ocupada puede fallar
    # (igual que el original), pero NUNCA debe leer un sector distinto
    for intento in range(5):
        f, a, bc = llamar(m, dir_(0x364), n >> 16, n & 0xFFFF, BUF)
        if (f & 0x40) == 0:
            break
    rok = bytes(m.memory[BUF:BUF + 512]) == pat
    if verbose or not rok:
        print(f"   releer {n:#x} tras escribir: {'OK' if rok else 'FALLO'} (intentos {intento + 1})")
    ok &= rok
    malos = [x for x in sd.log if x[0] == "R" and x[1] not in (0, 1, 2, 0x1234, 0x3FFFFF, 0x01234567, n)]
    if malos:
        print("   sectores leídos que no tocaban:", malos)
        ok = False
    return ok


if __name__ == "__main__":
    for fich, ctl, dat in [(a, int(b, 16), int(c, 16)) for a, b, c in
                           (x.split(":") for x in sys.argv[1:])]:
        for sdhc in (False, True):
            for busy in (0, 40, 5000):
                for pip in (True, False):
                    r = probar(fich, sdhc, ctl, dat, busy, pip)
                    print(f"{fich:22s} {'SDHC' if sdhc else 'SDSC'} ocupado={busy:<5d} "
                          f"{'segmentado' if pip else 'directo   '}: {'OK' if r else 'FALLA'}")
```
