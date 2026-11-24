#!/usr/bin/env python3
#
# origen_simbolos.py - De dónde sale cada función de la biblioteca de C del kernel
#
# Uso: origen_simbolos.py kernel8.map kernel8.elf aarch64-none-elf-nm
#
# Busca en el ELF la dirección de cada función y, en el mapa del enlazador
# (-Map, lo genera Rules.mk de Circle), la sección de entrada que la contiene:
# así se ve si memcpy, malloc, printf... vienen de newlib (libc.a), de Circle
# (libcircle.a) o del propio emulador. Si alguna de las que debe dar newlib
# sale de Circle, lo marca con "<-- Circle" (no detiene la compilación).
# memcpy, memset, memmove, malloc, free, calloc y realloc son siempre de
# Circle (circle-newlib se compila sin ellas): esas no se marcan.
#
import os
import re
import subprocess
import sys

# Las que interesan: las que Circle también define (memoria, cadenas, malloc)
# y las de E/S con formato
SIMBOLOS = [
	"memcpy", "memset", "memmove", "memcmp", "memchr",
	"strlen", "strcmp", "strncmp", "strcpy", "strncpy", "strcat", "strchr",
	"strrchr", "strstr", "strcasecmp", "strtoul", "strtol", "atoi",
	"malloc", "free", "calloc", "realloc",
	"printf", "sprintf", "snprintf", "vsnprintf", "sscanf", "qsort",
	"_sbrk", "_write", "_read", "_open",
]

# Las que circle-newlib deja a Circle a propósito (newlib/libc/machine/*/
# Makefile.inc y MALLOC_PROVIDED en newlib/configure.host)
DE_CIRCLE = {"memcpy", "memset", "memmove", "malloc", "free", "calloc", "realloc"}

def secciones (fmap):
	"""Secciones de entrada del mapa: (inicio, fin, fichero)"""
	res = []
	lineas = open (fmap, errors="replace").read ().splitlines ()
	try:
		i = next (n for n, l in enumerate (lineas) if l.startswith ("Linker script and memory map"))
	except StopIteration:
		return res
	pend = None
	for l in lineas[i:]:
		# " .text.memcpy   0x...  0x...  fichero" o, si el nombre es largo,
		# " .text.memcpy" y en la línea siguiente "  0x...  0x...  fichero"
		m = re.match (r"^ (\.\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S.*)$", l)
		if m:
			pend = None
			ini, tam = int (m.group (2), 16), int (m.group (3), 16)
			if tam:
				res.append ((ini, ini + tam, m.group (4).strip ()))
			continue
		m = re.match (r"^ (\.\S+)$", l)
		if m:
			pend = m.group (1)
			continue
		m = re.match (r"^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S.*)$", l)
		if m and pend:
			ini, tam = int (m.group (1), 16), int (m.group (2), 16)
			if tam:
				res.append ((ini, ini + tam, m.group (3).strip ()))
		pend = None
	return res

def corto (f):
	"""/ruta/install/aarch64-none-circle/lib/libc.a(libc_a-memcpy.o) -> libc.a(libc_a-memcpy.o)"""
	m = re.match (r"^(.*?)(\(.*\))$", f)
	if m:
		return os.path.basename (m.group (1)) + m.group (2)
	return os.path.relpath (f) if os.path.isabs (f) else f

def main ():
	if len (sys.argv) != 4:
		print (__doc__ or "uso: origen_simbolos.py fichero.map fichero.elf nm", file=sys.stderr)
		return 2
	fmap, elf, nm = sys.argv[1:]
	dirs = {}
	sal = subprocess.run ([nm, "--defined-only", elf], capture_output=True, text=True, check=True).stdout
	for l in sal.splitlines ():
		p = l.split ()
		if len (p) == 3 and p[1] in "TtWw":
			dirs.setdefault (p[2], int (p[0], 16))
	secs = secciones (fmap)
	ancho = max (len (s) for s in SIMBOLOS)
	for s in SIMBOLOS:
		if s not in dirs:
			continue
		a = dirs[s]
		f = next ((c[2] for c in secs if c[0] <= a < c[1]), "?")
		f = corto (f)
		aviso = "  <-- Circle" if "libcircle.a" in f and s not in DE_CIRCLE else ""
		print (f"{s:<{ancho}}  {f}{aviso}")
	return 0

if __name__ == "__main__":
	sys.exit (main ())
