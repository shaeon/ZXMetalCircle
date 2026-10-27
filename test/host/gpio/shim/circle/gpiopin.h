#pragma once
#include <circle/types.h>
enum TGPIOMode { GPIOModeInputPullUp, GPIOModeOutput };
// Simulación: matriz de contactos [fila][columna] entre pines BCM
extern bool g_Contact[32][32]; extern u32 g_OutLow;
class CGPIOPin { public: CGPIOPin(unsigned, TGPIOMode) {}
 static void WriteAll(u32, u32) {}
 static void SetModeAll(u32 nInputMask, u32 nOutputMask) { g_OutLow = nOutputMask; }
 static u32 ReadAll() { u32 v = 0xFFFFFFFF; for (int r = 0; r < 32; r++) if (g_OutLow & (1u << r)) for (int c = 0; c < 32; c++) if (g_Contact[r][c]) v &= ~(1u << c); return v; } };
