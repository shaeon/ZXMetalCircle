#!/usr/bin/env python3
#
# mkdisk.py - Disquete de +3 con la prueba del modo "all RAM"
#
# Crea allram.dsk con dos ficheros: DISK (BASIC que arranca solo con
# "Loader") y ALLRAM (la rutina de allram.asm). El BASIC la ejecuta y
# compara lo que ve en cada configuración especial con la tabla del manual.
#
#   pasmo allram.asm allram.bin && python3 mkdisk.py
#
import struct, sys, os

HERE = os.path.dirname (os.path.abspath (__file__))

# --- BASIC del Spectrum -----------------------------------------------------

TOKENS = {'PEEK': 0xBE, 'USR': 0xC0, 'AND': 0xC6, '<>': 0xC9, 'THEN': 0xCB,
	  'TO': 0xCC, 'CODE': 0xAF, 'STOP': 0xE2, 'READ': 0xE3, 'DATA': 0xE4,
	  'RESTORE': 0xE5, 'BORDER': 0xE7, 'REM': 0xEA, 'FOR': 0xEB,
	  'LOAD': 0xEF, 'LET': 0xF1, 'NEXT': 0xF3, 'PRINT': 0xF5,
	  'RANDOMIZE': 0xF9, 'IF': 0xFA, 'CLS': 0xFB, 'CLEAR': 0xFD, '>': 0x3E}

def num (n):
	"""Un número entero en un programa: sus cifras y su forma oculta."""
	return str (n).encode () + bytes ([0x0E, 0, 0, n & 0xFF, n >> 8, 0])

def line (nLine, *Parts):
	"""Una línea: cada parte es un token (cadena en TOKENS), un número (int)
	   o texto tal cual (bytes)."""
	Body = b''
	for p in Parts:
		if isinstance (p, int):
			Body += num (p)
		elif isinstance (p, str):
			Body += bytes ([TOKENS[p]])
		else:
			Body += p
	Body += b'\x0D'
	return struct.pack ('>H', nLine) + struct.pack ('<H', len (Body)) + Body

RES = 0x8800
Program = b''.join ([
	line (10, 'CLEAR', 24999, b':', 'LOAD', b'"allram"', 'CODE', 32768),
	line (20, 'BORDER', 7, b':', 'CLS', b':', 'PRINT', b'"Prueba del modo all RAM (+2A/+3)"', b':', 'PRINT'),
	line (30, 'PRINT', b'"Conf 0000 4000 8000 C000"', b':', 'RANDOMIZE', 'USR', 32768,
	      b':', 'RESTORE', 100, b':', 'LET', b' t=', 0),
	line (40, 'FOR', b' m=', 0, 'TO', 3, b':', 'LET', b' k=', 0, b':', 'PRINT', b'" ";m;"   ";'),
	line (50, 'FOR', b' s=', 0, 'TO', 3, b':', 'READ', b' e:', 'LET', b' v=', 'PEEK', b'(', RES,
	      b'+m*', 8, b'+s)-', 160, b':', 'PRINT', b'v;"    ";:', 'IF', b' v', '<>', b'e ', 'THEN', 'LET', b' k=', 1),
	line (60, 'NEXT', b' s:', 'IF', 'PEEK', b'(', RES, b'+m*', 8, b'+', 4, b')', '<>', 90, 'THEN', 'LET', b' k=', 1),
	line (70, 'IF', 'PEEK', b'(', RES, b'+m*', 8, b'+', 5, b')', '<>', 1, 'THEN', 'LET', b' k=', 1),
	line (80, 'PRINT', b'("OK" ', 'AND', b' k=', 0, b')+("FALLO" ', 'AND', b' k=', 1, b'):', 'LET', b' t=t+k:', 'NEXT', b' m'),
	line (90, 'PRINT', b':', 'PRINT', b'("Todo correcto" ', 'AND', b' t=', 0, b')+("Hay fallos" ', 'AND', b' t', '>', 0, b'):', 'STOP'),
	line (100, 'DATA', 0, b',', 1, b',', 2, b',', 3, b',', 4, b',', 5, b',', 6, b',', 7, b',',
	      4, b',', 5, b',', 6, b',', 3, b',', 4, b',', 7, b',', 6, b',', 3),
	line (110, 'REM', b' Columnas: banco que hay en cada tramo; Prueba: escribir en 0000-3FFF y la ROM vuelve'),
])

# --- Ficheros de +3DOS (cabecera de 128 bytes) ------------------------------

def plus3file (Data, nType, nParam1, nParam2):
	h = bytearray (128)
	h[0:8] = b'PLUS3DOS'
	h[8] = 0x1A; h[9] = 1; h[10] = 0
	struct.pack_into ('<I', h, 11, len (Data) + 128)
	h[15] = nType
	struct.pack_into ('<HHH', h, 16, len (Data), nParam1, nParam2)
	h[127] = sum (h[:127]) & 0xFF
	return bytes (h) + Data

Code = open (os.path.join (HERE, 'allram.bin'), 'rb').read ()
Files = [('DISK    ', '   ', plus3file (Program, 0, 10, len (Program))),
	 ('ALLRAM  ', '   ', plus3file (Code, 3, 32768, 0x8000))]

# --- Disco +3: 40 pistas, 1 cara, 9 sectores de 512, 1 pista reservada,
# bloques de 1K, 2 bloques de directorio (64 entradas) ------------------------

TRACKS, SECTORS, SECSIZE = 40, 9, 512
Disk = [[bytearray (b'\xE5' * SECSIZE) for s in range (SECTORS)] for t in range (TRACKS)]
Disk[0][0][0:16] = bytes ([0, 0, 40, 9, 2, 1, 3, 2, 0x2A, 0x52, 0, 0, 0, 0, 0, 0])	# especificación

def block (n):
	"""Los dos sectores (pista, sector) del bloque n, tras la pista reservada."""
	L = n * 2
	return [(1 + (L + i) // SECTORS, (L + i) % SECTORS) for i in range (2)]

Dir = bytearray (b'\xE5' * 2048)
nNextBlock = 2
for i, (Name, Ext, Data) in enumerate (Files):
	Blocks = []
	for o in range (0, len (Data), 1024):
		Chunk = Data[o:o + 1024].ljust (1024, b'\x1A')
		for j, (t, s) in enumerate (block (nNextBlock)):
			Disk[t][s][:] = Chunk[j * 512:(j + 1) * 512]
		Blocks.append (nNextBlock)
		nNextBlock += 1
	assert len (Blocks) <= 16, 'un fichero de más de 16K necesita más extensiones'
	e = bytearray (32)
	e[0] = 0
	e[1:9] = Name.encode (); e[9:12] = Ext.encode ()
	e[12] = 0; e[13] = 0; e[14] = 0
	e[15] = (len (Data) + 127) // 128
	for j, b in enumerate (Blocks):
		e[16 + j] = b
	Dir[i * 32:(i + 1) * 32] = e
for n in range (2):
	for j, (t, s) in enumerate (block (n)):
		Disk[t][s][:] = Dir[(n * 2 + j) * 512:(n * 2 + j + 1) * 512]

# --- Imagen DSK estándar -----------------------------------------------------

Out = bytearray (256)
Out[0:34] = b'MV - CPCEMU Disk-File\r\nDisk-Info\r\n'
Out[34:48] = b'ZXMetalCircle '
Out[48] = TRACKS; Out[49] = 1
struct.pack_into ('<H', Out, 50, 256 + SECTORS * SECSIZE)
for t in range (TRACKS):
	h = bytearray (256)
	h[0:12] = b'Track-Info\r\n'
	h[16] = t; h[17] = 0
	h[20] = 2; h[21] = SECTORS; h[22] = 0x52; h[23] = 0xE5
	for s in range (SECTORS):
		h[24 + s * 8:32 + s * 8] = bytes ([t, 0, s + 1, 2, 0, 0, 0, 0])
	Out += h
	for s in range (SECTORS):
		Out += Disk[t][s]

open (os.path.join (HERE, 'allram.dsk'), 'wb').write (Out)
print ('allram.dsk: %d bytes, BASIC de %d bytes, rutina de %d bytes' % (len (Out), len (Program), len (Code)))
