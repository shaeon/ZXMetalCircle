#include "spectrum.h"
#include <stdio.h>
CZXKeyboard kb; CTape tape;
int main(){ int fails=0;
 struct { int model; bool issue2; u8 out; u8 bit6; const char*n; } t[]={
  {0,false,0x08,0,"48K Issue 3, OUT 8 (solo MIC)"}, {0,true,0x08,1,"48K Issue 2, OUT 8 (solo MIC)"},
  {0,false,0x10,1,"48K Issue 3, OUT 16 (EAR)"}, {0,true,0x00,0,"48K Issue 2, OUT 0"},
  {1,true,0x08,0,"128K con issue=2 (no aplica), OUT 8"}, {3,true,0x08,1,"16K Issue 2, OUT 8"} };
 for(auto&c:t){ CSpectrum*s=new CSpectrum(&kb,&tape); s->SetModel((TModel)c.model); s->SetIssue2(c.issue2);
  s->outPort(0x00FE,c.out); u8 v=s->inPort(0xFFFE); int b=(v>>6)&1;
  printf("%-40s bit6=%d %s\n",c.n,b,b==c.bit6?"ok":"FALLO"); if(b!=c.bit6)fails++; delete s; }
 return fails; }
