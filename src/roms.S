//
// roms.S - ROMs del Spectrum embebidas en el binario
//
// Solo van dentro del kernel las ROMs básicas del 48K y del 128K, para que el
// emulador funcione siempre. Las demás (+2, +2A, +3, +3e e Interface 1) se
// cargan de la carpeta roms de la SD (ver boot/roms/LEEME.txt).
//
// Las ROMs de Sinclair/Amstrad se distribuyen con permiso de Amstrad para su
// uso en emuladores (ficheros tomados de JSpeccy).
//

	.section .rodata
	.balign	4

	.global	rom_48
rom_48:
	.incbin	"roms/spectrum.rom"

	.global	rom_128_0
rom_128_0:
	.incbin	"roms/128-0.rom"

	.global	rom_128_1
rom_128_1:
	.incbin	"roms/128-1.rom"
