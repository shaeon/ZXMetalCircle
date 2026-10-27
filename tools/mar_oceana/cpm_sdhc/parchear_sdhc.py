#!/usr/bin/env python3
"""
Parche SDSC/SDHC para el controlador CP/M 3 de Otivax (DIVMMC.BIN / ZXMMC.BIN).

Uso:  python3 parchear_sdhc.py ORIGINAL.BIN SALIDA.BIN

- Comprueba que el fichero es el controlador esperado (PRL, longitud 0x4D0 en
  DIVMMC.BIN o 0x4CF en ZXMMC.BIN,
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

  05D0  06 00     ld b,0           ; hasta 256 intentos (05CF en ZXMMC.BIN)
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
    # DIVMMC.BIN mide 0x4D0 y ZXMMC.BIN 0x4CF (un byte menos al final)
    assert L in (0x4CF, 0x4D0), f"longitud de código inesperada: {L:#x}"
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
