; RTC.SYS - reloj de ZXMetalCircle para esxDOS
;
; esxDOS lo carga en 0x2700 (RAM de la divMMC) y lo llama para saber la fecha
; y la hora. Devuelve BC = fecha y DE = hora en formato MS-DOS, con el acarreo
; a 0; con el acarreo a 1 si no hay hora.
;
; El emulador da la hora de la Pi (puesta en hora por NTP) por el puerto
; 0x7C3B: un OUT la fija y cuatro IN la leen (fecha byte bajo y alto, hora
; byte bajo y alto). Una fecha 0 quiere decir que la Pi aún no tiene hora.
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

	.org	0x2700		; (binutils rellena desde 0: build.sh se queda con lo de 0x2700)

	push	af
	push	hl
	ld	bc,0x7C3B
	out	(c),a		; fija la hora
	in	l,(c)		; fecha
	in	h,(c)
	in	e,(c)		; hora
	in	d,(c)
	ld	a,h
	or	l
	jr	z,nohora
	ld	b,h
	ld	c,l
	pop	hl
	pop	af
	or	a		; acarreo a 0: hora correcta
	ret
nohora:
	pop	hl
	pop	af
	scf
	ret
