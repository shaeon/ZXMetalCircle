// CVideo de prueba: comprueba que Render, Flip y SetMode no se pisan
#pragma once
#include <circle/types.h>
#include <atomic>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
typedef u16 TPixel;
#define SCREEN_WIDTH 352
#define SCREEN_HEIGHT 296
struct CVideo {
  std::atomic<int> nRendering{0}, nSetMode{0}; std::atomic<unsigned> nRenders{0}, nFlips{0}; std::atomic<int> nLastRendered{-1};
  bool bRenderedNotFlipped=false; int nErrors=0;
  void Render (const TPixel *p) { nRendering++; if (nSetMode) { printf("ERROR: Render durante SetMode\n"); nErrors++; }
    if (bRenderedNotFlipped) { printf("ERROR: Render sin Flip del anterior\n"); nErrors++; }
    int v=p[0]; for (int i=1;i<SCREEN_WIDTH*SCREEN_HEIGHT;i+=997) if (p[i]!=v) { printf("ERROR: cuadro mezclado\n"); nErrors++; break; }
    std::this_thread::sleep_for (std::chrono::microseconds (rand()%3000)); nLastRendered=v; bRenderedNotFlipped=true; nRenders++; nRendering--; }
  void Flip () { if (nRendering) { printf("ERROR: Flip durante Render\n"); nErrors++; } if (!bRenderedNotFlipped) { printf("ERROR: Flip sin Render\n"); nErrors++; } bRenderedNotFlipped=false; nFlips++; }
  void SetMode () { nSetMode++; if (nRendering) { printf("ERROR: SetMode durante Render\n"); nErrors++; } std::this_thread::sleep_for (std::chrono::microseconds (500)); nSetMode--; }
};
