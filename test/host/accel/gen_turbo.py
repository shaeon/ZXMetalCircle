# Genera turbo.tzx: BASIC + código (estándar) y luego:
#  bloque A: estándar (0x10), 6000 bytes al azar -> 0xA000, cargado con la ROM (0x0556)
#  bloque B: turbo (0x11, bit 0 = 570, bit 1 = 1140 T), 8000 bytes -> 0xC000, cargado
#            con una copia reubicada de LD-BYTES con la espera y el umbral cambiados
# En 0x8FFE y 0x8FFF quedan los acarreos (1 = bien) de los dos bloques.
import struct, random, sys
random.seed(1234)
rom = open('../../../roms/spectrum.rom', 'rb').read()
BASE = 0x8000
lb = bytearray(rom[0x0556:0x0605])
d = BASE - 0x0556
for off in (0x056C, 0x057B, 0x0582, 0x0591, 0x059B, 0x05CA, 0x05D5, 0x05E3):
    i = off - 0x0556 + 1
    a = lb[i] | lb[i+1] << 8
    a += d
    lb[i] = a & 0xFF; lb[i+1] = a >> 8
lb[0x05E7 - 0x0556 + 1] = 0x05     # espera de LD-EDGE-1: 0x16 -> 0x05
lb[0x05CE - 0x0556 + 1] = 0xCA     # umbral del bit: 0xCB -> 0xCA
dataA = bytes(random.randrange(256) for _ in range(6000))
dataB = bytes(random.randrange(256) for _ in range(8000))
stub = bytes([
  0xDD,0x21,0x00,0xA0, 0x11]) + struct.pack('<H',len(dataA)) + bytes([0x3E,0xFF,0x37, 0xCD,0x56,0x05,
  0x3E,0x00,0x17, 0x32,0xFE,0x8F,
  0xDD,0x21,0x00,0xC0, 0x11]) + struct.pack('<H',len(dataB)) + bytes([0x3E,0xFF,0x37, 0xCD,0x00,0x80,
  0x3E,0x00,0x17, 0x32,0xFF,0x8F, 0xC9])
code = bytes(lb) + bytes(0x100 - len(lb)) + stub
STUB = BASE + 0x100
def num(n): return bytes([0x0E,0,0,n&0xFF,n>>8,0])
line = bytes([0xEF,0x22,0x22,0xAF,0x3A,0xF9,0xC0]) + str(STUB).encode() + num(STUB) + bytes([0x0D])
prog = struct.pack('>H',10) + struct.pack('<H',len(line)) + line
def blk(flag, data):
    b = bytes([flag]) + data; x = 0
    for c in b: x ^= c
    return b + bytes([x])
def hdr(t, name, length, p1, p2):
    return blk(0, bytes([t]) + name.ljust(10).encode() + struct.pack('<HHH', length, p1, p2))
def std(b, pause=1000): return bytes([0x10]) + struct.pack('<HH', pause, len(b)) + b
def turbo(b, zero=570, one=1140, pause=1000):
    return bytes([0x11]) + struct.pack('<HHHHHHBH', 2168, 667, 735, zero, one, 3223, 8, pause) + struct.pack('<I', len(b))[:3] + b
t = b'ZXTape!\x1a\x01\x14'
t += std(hdr(0,'turbo',len(prog),10,len(prog))) + std(blk(0xFF,prog))
t += std(hdr(3,'code',len(code),BASE,0)) + std(blk(0xFF,code))
t += std(blk(0xFF,dataA)) + turbo(blk(0xFF,dataB))
open('turbo.tzx','wb').write(t)
open('turbo_A.bin','wb').write(dataA); open('turbo_B.bin','wb').write(dataB)
print('ok', len(t))
