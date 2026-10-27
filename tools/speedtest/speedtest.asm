; ===========================================================================
; speedtest.asm - Test de velocidad para ZXMetalCircle (o cualquier Spectrum)
;
; 1. Mide los t-states que dura un cuadro (entre dos interrupciones):
;    48K = 69888, 128K/+2/+2A = 70908. Así se comprueba la temporización.
; 2. Muestra un reloj con el tiempo transcurrido según el propio Spectrum
;    (cuadros x duración exacta del cuadro) y hace "clic" y cambia el borde
;    cada segundo. Compáralo con un cronómetro: si el emulador va a la
;    velocidad correcta, deben coincidir (1 s de desfase en 10 min = 0,17 %).
;    SPACE vuelve a medir.
;
; Ensamblar con:  pasmo --tapbas speedtest.asm speedtest.tap
; ===========================================================================

CLS		equ	$0D6B
CHAN_OPEN	equ	$1601
STACK_BC	equ	$2D2B
PRINT_FP	equ	$2DE3

TABLE		equ	$FE00		; tabla de vectores IM 2 (257 bytes de $FD)
JUMP		equ	$FDFD		; aquí salta la interrupción

FRAMES_MEASURE	equ	25

		org	32768

start:		call	CLS
		ld	a,2
		call	CHAN_OPEN
		ld	hl,txt_title
		call	print

		; tabla IM 2: 257 bytes $FD -> vector $FDFD
		di
		ld	hl,TABLE
		ld	de,TABLE+1
		ld	bc,256
		ld	(hl),$FD
		ldir
		ld	a,$C3			; JP isr_measure
		ld	(JUMP),a
		ld	a,TABLE/256
		ld	i,a
		im	2

; --- medida de t-states por cuadro ------------------------------------------
measure:	di
		ld	hl,isr_measure
		ld	(JUMP+1),hl
		xor	a
		ld	(ticks),a
		ei
		halt				; sincroniza con una interrupción
		xor	a			; 4
		ld	(ticks),a		; 13
		ld	hl,0			; 10
count:		inc	hl			; 6
		ld	a,(ticks)		; 13
		cp	FRAMES_MEASURE		; 7
		jr	nz,count		; 12  -> 38 t-states por vuelta
		ld	(iters),hl

		; T = (iteraciones * 38 + 25 * 98 + 27) / 25
		; (98 = aceptación IM2 19 + JP del vector 10 + ISR 69)
		ld	bc,(iters)
		call	STACK_BC
		ld	bc,38
		call	STACK_BC
		rst	$28
		defb	$04			; multiply
		defb	$38
		ld	bc,FRAMES_MEASURE*98+27
		call	STACK_BC
		rst	$28
		defb	$0F			; addition
		defb	$38
		ld	bc,FRAMES_MEASURE
		call	STACK_BC
		rst	$28
		defb	$05			; division
		defb	$27			; int
		defb	$38

		ld	hl,txt_tstates
		call	print
		call	PRINT_FP
		ld	hl,txt_spaces
		call	print

		; modelo según el resultado y duración del cuadro en microsegundos
		ld	hl,(iters)
		ld	de,46250		; entre 48K (~45920) y 128K (~46590)
		or	a
		sbc	hl,de
		jr	nc,is128
		ld	hl,19968		; 69888 / 3,5 MHz
		ld	de,txt_48
		jr	setmodel
is128:		ld	hl,19992		; 70908 / 3,5469 MHz
		ld	de,txt_128
setmodel:	ld	(period),hl
		ex	de,hl
		call	print

		; reinicia el reloj
		ld	hl,0
		ld	(usec),hl
		ld	(usec+2),hl
		ld	(seconds),hl
		xor	a
		ld	(pending),a
		ld	(border),a
		call	show_time

		di
		ld	hl,isr_clock
		ld	(JUMP+1),hl
		ei

; --- reloj -----------------------------------------------------------------
clock:		halt
		ld	a,$7F			; SPACE: volver a medir
		in	a,($FE)
		rra
		jp	nc,measure

next_frame:	ld	a,(pending)
		or	a
		jr	z,clock
		di
		dec	a
		ld	(pending),a
		ei

		; usec += period (32 bits)
		ld	hl,(usec)
		ld	de,(period)
		add	hl,de
		ld	(usec),hl
		ld	hl,(usec+2)
		ld	de,0
		adc	hl,de
		ld	(usec+2),hl

		; ¿usec >= 1.000.000 ($000F4240)?
		ld	hl,(usec)
		ld	de,$4240
		or	a
		sbc	hl,de
		ld	b,h
		ld	c,l
		ld	hl,(usec+2)
		ld	de,$000F
		sbc	hl,de
		jr	c,next_frame		; aún no
		ld	(usec),bc		; usec -= 1.000.000
		ld	(usec+2),hl

		ld	hl,(seconds)
		inc	hl
		ld	(seconds),hl
		call	click
		call	show_time
		jr	next_frame

; --- interrupciones --------------------------------------------------------
isr_measure:	push	af			; 11  (coste fijo: 69)
		ld	a,(ticks)		; 13
		inc	a			; 4
		ld	(ticks),a		; 13
		pop	af			; 10
		ei				; 4
		reti				; 14

isr_clock:	push	af
		ld	a,(pending)
		inc	a
		ld	(pending),a
		pop	af
		ei
		reti

; --- utilidades ------------------------------------------------------------
; "clic" de unos milisegundos y cambio de color del borde
click:		ld	a,(border)
		xor	1			; alterna negro / azul
		ld	(border),a
		ld	c,a
		ld	b,40
click1:		ld	a,c
		or	$10			; EAR a 1
		out	($FE),a
		ld	e,30
click2:		dec	e
		jr	nz,click2
		ld	a,c
		out	($FE),a
		ld	e,30
click3:		dec	e
		jr	nz,click3
		djnz	click1
		ret

; Tiempo: mm:ss
show_time:	ld	hl,txt_time
		call	print
		ld	hl,(seconds)
		ld	de,60
		call	div16			; hl = minutos, de = segundos
		push	de
		ld	b,h
		ld	c,l
		call	STACK_BC
		call	PRINT_FP
		ld	a,':'
		rst	$10
		pop	de
		ld	a,e
		cp	10
		jr	nc,st1
		ld	a,'0'
		rst	$10
st1:		ld	b,0
		ld	c,e
		call	STACK_BC
		call	PRINT_FP
		ld	hl,txt_spaces
		jp	print

; hl / de -> hl = cociente, de = resto
div16:		ld	bc,0
div1:		or	a
		sbc	hl,de
		jr	c,div2
		inc	bc
		jr	div1
div2:		add	hl,de
		ex	de,hl
		ld	h,b
		ld	l,c
		ret

; imprime una cadena terminada en $FF
print:		ld	a,(hl)
		cp	$FF
		ret	z
		rst	$10
		inc	hl
		jr	print

; --- textos ($16 fila columna = AT) ----------------------------------------
txt_title:	defb	$16,1,1,"ZXMetalCircle: test velocidad"
		defb	$16,4,0,"T-states por cuadro:"
		defb	$16,5,0,"(48K: 69888  128K/+2/+2A: 70908)"
		defb	$16,9,0,"Tiempo segun el Spectrum:"
		defb	$16,12,0,"Comparalo con un cronometro."
		defb	$16,13,0,"Cada segundo suena un clic y"
		defb	$16,14,0,"cambia el borde."
		defb	$16,16,0,"Si va a la velocidad correcta,"
		defb	$16,17,0,"tras 10 minutos no debe haber"
		defb	$16,18,0,"mas de 1 segundo de diferencia."
		defb	$16,20,0,"SPACE: volver a medir"
		defb	$FF
txt_tstates:	defb	$16,4,21,$FF
txt_48:		defb	$16,7,0,"Modelo: 48K (50,08 cuadros/s) ",$FF
txt_128:	defb	$16,7,0,"Modelo: 128K (50,02 cuadros/s)",$FF
txt_time:	defb	$16,10,4,$FF
txt_spaces:	defb	"    ",$FF

; --- variables -------------------------------------------------------------
ticks:		defb	0
pending:	defb	0
border:		defb	0
iters:		defw	0
period:		defw	19968
usec:		defw	0,0
seconds:	defw	0

		end	start
