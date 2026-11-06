# Mandos USB en ZXMetalCircle

Cómo se usan los mandos USB en el emulador (versión 1.06) y qué no se puede hacer todavía. Esta guía cambiará cuando se amplíe el soporte de mandos.

## Qué mandos reconoce

- **Con su mapeo conocido**: PS3, PS4 DualShock, Xbox 360, Xbox One, Switch Pro y 8BitDo.
- **Mandos HID genéricos**, incluidos los adaptadores Mayflash en modo DirectInput.
- **Hasta 4 a la vez**, conectando y desconectando en caliente.

## Cómo se mapean

**Todos los mandos manejan el mismo joystick emulado**: el que esté elegido (con F4, el botón Select o la opción `joystick=` de `cmdline.txt`).

| Botón del mando | Kempston | Fuller | Sinclair 1 | Sinclair 2 | Cursor | QAOPSpaceM |
| --- | --- | --- | --- | --- | --- | --- |
| Cruceta y stick izquierdo | direcciones | direcciones | 6-9 | 1-5 | 5-8 | Q A O P |
| **A / Cruz, Y / Triángulo, L1, R1** | disparo | disparo | disparo (0) | disparo (5) | disparo (0) | SPACE |
| **B / Círculo, X / Cuadrado, L2, R2** | disparo 2 | disparo | X | Z | 9 | M |

- **Disparo 2** es un segundo botón con una tecla útil en cada tipo de joystick: en el Kempston, el bit del segundo disparo; en los demás, una tecla que suelen usar los juegos.
- **Select / Share / View / Capture** cambia el joystick, en el mismo orden que ZXBaremulator: ninguno → Kempston → Fuller → Sinclair 1 → Sinclair 2 → Cursor → QAOPSpaceM.
- **Start / Options** abre la ayuda y el selector de cintas.
- **En los menús**, la cruceta navega, el disparo hace de ENTER y el disparo 2 de ESC.

### Mandos genéricos

Como no se sabe qué botón es cuál, se usa un reparto fijo por número:

- las direcciones salen del primer «hat» (la cruceta) o de los dos primeros ejes;
- los botones 1, 4, 5 y 6 son disparo; el 2, 3, 7 y 8, disparo 2;
- el 9 es Select y el 10, Start.

## Lo que no se puede hacer todavía

1. **Elegir qué hace cada botón.** El reparto es fijo: no se puede asignar a un botón una tecla concreta del Spectrum (por ejemplo, un botón a la `H` de un juego que pide «pulsa H para hiperespacio»).
2. **Dos jugadores.** Con dos mandos, los dos mueven el mismo joystick. Para dos jugadores haría falta, por ejemplo, que el primer mando fuera Sinclair 1 y el segundo Sinclair 2, o Kempston y Cursor.
3. **Corregir un mando genérico que no encaja con el reparto por número**, por ejemplo uno con los botones numerados en otro orden.

---

## Más detalles

### Mandos USB

Se conectan y desconectan en caliente, hasta 4 a la vez. Todos manejan el joystick emulado que esté seleccionado.

| Botón | Kempston | Fuller | Sinclair 1 | Sinclair 2 | Cursor | QAOPSpaceM |
| --- | --- | --- | --- | --- | --- | --- |
| A / Cruz, Y / Triángulo, L1, R1 | Disparo | Disparo | Disparo | Disparo | Disparo | SPACE |
| B / Círculo, X / Cuadrado, L2, R2 | Disparo 2 | Disparo | X | Z | 9 | M |

- La cruceta y el stick izquierdo mueven el joystick.
- **Select / Share / View / Capture** cambia el joystick, en el mismo ciclo que ZXBaremulator: ninguno → Kempston → Fuller → Sinclair 1 → Sinclair 2 → Cursor → QAOPSpaceM.
- **Start / Options** abre la ayuda y el selector de cintas.
- En los menús, la cruceta navega, el disparo equivale a ENTER y el disparo 2 a ESC.
- Mandos genéricos: el primer "hat" o los dos primeros ejes son las direcciones. Los botones 1, 4, 5 y 6 son disparo; los botones 2, 3, 7 y 8 son disparo 2; el botón 9 es Select y el 10 es Start.
- Alimentación: varios mandos consumen bastante al arrancar. Usa una fuente de al menos 2,5 A.
