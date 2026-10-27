#include "videocore.h"
int main () { srand (1); static CVideo V; CVideoCore C (&V); C.Start ();
  static TPixel Frame[SCREEN_WIDTH*SCREEN_HEIGHT]; unsigned nSubmitted=0, nDropped=0;
  for (int f = 0; f < 3000; f++) {
    for (int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++) Frame[i]=f&0xFFFF;   // el cuadro f entero
    if (C.Submit (Frame)) nSubmitted++; else nDropped++;
    if (f % 97 == 0) { C.Pause (); V.SetMode (); C.Resume (); }
    for (int k=0;k<5;k++) { C.Poll (); std::this_thread::sleep_for (std::chrono::microseconds (400)); }
  }
  for (int k=0;k<100;k++) { C.Poll (); std::this_thread::sleep_for (std::chrono::milliseconds (1)); }
  printf ("cuadros: %u enviados, %u descartados; %u ampliados, %u mostrados; errores: %d\n", nSubmitted, nDropped, (unsigned) V.nRenders, (unsigned) V.nFlips, V.nErrors);
  return V.nErrors != 0; }
