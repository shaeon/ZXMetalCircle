#!/usr/bin/env python3
#
# actualizar.py - Pone al día las traducciones de ZXMetalCircle
#
# Busca en src/ todos los textos marcados con T ("...") o N_ ("...") y:
#   - rehace locales/es.json (de referencia: el español va dentro del programa);
#   - en los demás locales/xx.json añade los textos nuevos sin traducir (""),
#     conserva los ya traducidos y guarda como "_huerfano|texto" los que ya no
#     están en el programa (por si el español solo ha cambiado un poco);
#   - avisa de los textos sin traducir y de las traducciones cuyos %u, %s...
#     no coinciden con los del español (el emulador podría fallar).
#
# Solo avisa: nunca para la compilación. Uso: python3 tools/i18n/actualizar.py
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

import glob, json, os, re, sys

ROOT = os.path.abspath (os.path.join (os.path.dirname (__file__), '..', '..'))
LOCALES = os.path.join (ROOT, 'locales')
VERSION = 1

MARK = re.compile (r'\b(?:T|N_)\s*\(\s*"((?:[^"\\]|\\.)*)"\s*\)')
FORMAT = re.compile (r'%[-+ 0#]*\d*(?:\.\d+)?(?:ll|l|h)?[diouxXcsp%]')

def unescape_c (text):
	out = []
	i = 0
	while i < len (text):
		c = text[i]
		if c == '\\' and i + 1 < len (text):
			i += 1
			c = {'n': '\n', 't': '\t', 'r': '\r', '"': '"', '\\': '\\', "'": "'"}.get (text[i], '\\' + text[i])
		out.append (c)
		i += 1
	return ''.join (out)

def source_texts ():
	texts = []
	seen = set ()
	files = sorted (glob.glob (os.path.join (ROOT, 'src', '**', '*.cpp'), recursive = True)
			+ glob.glob (os.path.join (ROOT, 'src', '**', '*.h'), recursive = True))
	for path in files:
		with open (path, encoding = 'utf-8') as f:
			code = f.read ()
			# sin comentarios (// ... hasta el final de la línea, fuera de cadenas)
			code = re.sub (r'^(?:[^"\n/]|"(?:[^"\\\n]|\\.)*"|/(?!/))*//[^\n]*',
				       lambda m: m.group (0)[:m.group (0).rfind ('//')], code, flags = re.M)
			for m in MARK.finditer (code):
				text = unescape_c (m.group (1))
				if text and text not in seen and text.strip ():
					seen.add (text)
					texts.append (text)
	return texts

def write_json (path, data):
	with open (path, 'w', encoding = 'utf-8') as f:
		json.dump (data, f, ensure_ascii = False, indent = '\t')
		f.write ('\n')

def formats (text):
	return sorted (FORMAT.findall (text))

def main ():
	texts = source_texts ()
	os.makedirs (LOCALES, exist_ok = True)

	ref = {'_idioma': 'Español', '_version': VERSION}
	for t in texts:
		ref[t] = t
	write_json (os.path.join (LOCALES, 'es.json'), ref)
	print ('i18n: %d textos en el programa' % len (texts))

	for path in sorted (glob.glob (os.path.join (LOCALES, '*.json'))):
		code = os.path.splitext (os.path.basename (path))[0]
		if code == 'es':
			continue
		with open (path, encoding = 'utf-8') as f:
			old = json.load (f)

		new = {'_idioma': old.get ('_idioma', code), '_version': VERSION}
		missing = []
		bad = []
		for t in texts:
			value = old.get (t, '')
			new[t] = value
			if value == '':
				missing.append (t)
			elif formats (value) != formats (t):
				bad.append (t)

		orphans = [k for k in old if not k.startswith ('_') and k not in new]
		for k in orphans:
			new['_huerfano|' + k] = old[k]
		for k in old:			# huérfanos de antes que siguen sin usarse
			if k.startswith ('_huerfano|') and k[10:] not in new:
				new[k] = old[k]

		write_json (path, new)
		print ('i18n: %s: %d sin traducir, %d huérfanos nuevos, %d con formato distinto'
		       % (code, len (missing), len (orphans), len (bad)))
		for t in missing[:20]:
			print ('   sin traducir: %r' % t)
		if len (missing) > 20:
			print ('   ... y %d más' % (len (missing) - 20))
		for t in bad:
			print ('   AVISO, formato distinto: %r -> %r' % (t, new[t]))
		for t in orphans:
			print ('   huérfano: %r' % t)

if __name__ == '__main__':
	main ()
