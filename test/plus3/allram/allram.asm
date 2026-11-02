; allram.asm - prueba del modo especial "all RAM" del +2A/+3 (bit 0 de 0x1FFD)
;
; Escribe una marca (A0h + banco) en cada uno de los 8 bancos de RAM, activa
; una a una las cuatro configuraciones especiales y, en cada una, lee qué
; banco hay en cada tramo de 16 KB y prueba a escribir en 0000h-3FFFh (donde
; normalmente está la ROM). Después vuelve al modo normal y comprueba que la
; ROM está otra vez. Los resultados quedan en RESULT (8 bytes por configuración)
; y los muestra el BASIC del disquete.
;
; En el modo especial no hay ROM ni se puede usar la pila (la memoria cambia
; bajo el código), así que la parte que conmuta va copiada en los 8 bancos en
; la misma dirección (ROUT) y no usa la pila. Todo lo que se toca de cada
; banco se guarda antes y se restaura al final (+3DOS usa el banco 7).
;
;   pasmo allram.asm allram.bin
;
; Uso: CLEAR 24999: LOAD "allram" CODE 32768: RANDOMIZE USR 32768

BANKM	equ	5B5Ch		; copia de lo último escrito en 7FFDh
BANK678	equ	5B67h		; copia de lo último escrito en 1FFDh
SIG	equ	3C00h		; posición de la marca dentro de cada banco
ROUT	equ	0FE00h		; la rutina que conmuta, en todos los bancos
RESULT	equ	8800h		; resultados: 8 bytes por configuración
SAVE	equ	9000h		; lo que se guarda de cada banco (128 bytes por banco)

	org	32768

start:	di
	ld	a,(BANKM)
	ld	(bm),a
	ld	a,(BANK678)
	ld	(b678),a
	ld	a,(0)
	ld	(rom0),a		; primer byte de la ROM que hay ahora

	; En cada banco: guardar lo que se va a tocar, poner la marca y copiar
	; la rutina
	xor	a
	ld	(bank),a
banks:	call	pagebank
	call	slot			; DE = SAVE + banco * 128
	ld	hl,0C000h+SIG
	ldi
	ldi
	ld	hl,ROUT
	ld	bc,RLEN
	ldir
	ld	a,(bank)
	add	a,0A0h
	ld	(0C000h+SIG),a
	ld	hl,routine
	ld	de,ROUT
	ld	bc,RLEN
	ldir
	ld	hl,bank
	inc	(hl)
	ld	a,(hl)
	cp	8
	jr	nz,banks

	ld	a,(bm)			; el banco de 0C000h que tenía BASIC
	ld	bc,7FFDh
	out	(c),a

	; Las cuatro configuraciones
	xor	a
	ld	(cfg),a
tests:	ld	a,(b678)
	and	18h			; motor e impresora como estaban
	ld	d,a			; D = 1FFDh en modo normal
	ld	a,(cfg)
	rlca
	or	1
	or	d
	ld	e,a			; E = 1FFDh en modo especial
	ld	a,(b678)
	ld	d,a
	call	ROUT			; vuelve en modo normal
	ld	a,(cfg)			; IY = RESULT + cfg * 8
	rlca
	rlca
	rlca
	push	iy
	ld	iy,RESULT
	ld	b,0
	ld	c,a
	add	iy,bc
	ld	(iy+0),h		; banco en 0000h
	ld	(iy+1),l		; banco en 4000h
	ld	a,ixh
	ld	(iy+2),a		; banco en 8000h
	ld	a,ixl
	ld	(iy+3),a		; banco en C000h
	ld	(iy+4),e		; lo escrito y leído en 0000h-3FFFh (5Ah)
	ld	a,(0)
	ld	hl,rom0
	cp	(hl)
	ld	a,1
	jr	z,romok
	xor	a
romok:	ld	(iy+5),a		; 1: la ROM ha vuelto
	pop	iy
	ld	hl,cfg
	inc	(hl)
	ld	a,(hl)
	cp	4
	jr	nz,tests

	; Restaurar lo que se tocó en cada banco
	xor	a
	ld	(bank),a
rest:	call	pagebank
	call	slot
	ex	de,hl			; HL = lo guardado
	ld	de,0C000h+SIG
	ldi
	ldi
	ld	de,ROUT
	ld	bc,RLEN
	ldir
	ld	hl,bank
	inc	(hl)
	ld	a,(hl)
	cp	8
	jr	nz,rest

	ld	a,(bm)
	ld	bc,7FFDh
	out	(c),a
	ld	a,(b678)
	ld	bc,1FFDh
	out	(c),a
	ei
	ret

; Pone el banco (bank) en 0C000h, con la ROM y la pantalla de BANKM
pagebank:
	ld	a,(bm)
	and	0F8h
	ld	hl,bank
	or	(hl)
	ld	bc,7FFDh
	out	(c),a
	ret

; DE = SAVE + banco * 128
slot:	ld	a,(bank)
	ld	h,0
	ld	l,a
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	de,SAVE
	add	hl,de
	ex	de,hl
	ret

; La rutina que conmuta: se ejecuta en ROUT. Entra con E = 1FFDh especial y
; D = 1FFDh normal; sale con H, L, IXh, IXl = las marcas leídas en 0000h,
; 4000h, 8000h y C000h, y E = lo leído tras escribir 5Ah en 0000h+SIG+1.
; No usa la pila mientras está en modo especial, ni tiene saltos ni
; referencias a sí misma: funciona igual copiada en cualquier dirección.
routine:
	ld	bc,1FFDh
	out	(c),e
	ld	a,(0000h+SIG)
	ld	h,a
	ld	a,(4000h+SIG)
	ld	l,a
	ld	a,(8000h+SIG)
	ld	ixh,a
	ld	a,(0C000h+SIG)
	ld	ixl,a
	ld	a,5Ah
	ld	(0000h+SIG+1),a
	ld	a,(0000h+SIG+1)
	ld	e,a
	ld	bc,1FFDh
	out	(c),d
	ret
RLEN	equ	$-routine

bm:	db	0
b678:	db	0
rom0:	db	0
bank:	db	0
cfg:	db	0

	end	start
