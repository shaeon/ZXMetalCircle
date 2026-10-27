#include "spectrum.h"
#include <stdio.h>
CZXKeyboard kb; CTape tape; int fails=0;
void chk(const char*n,unsigned got,unsigned exp){ if(got!=exp){printf("FALLO %s: %02X != %02X\n",n,got,exp);fails++;} else printf("ok %s\n",n);}
u8 v(int p){ return 0x80 | ((p&7)<<4) | (p&8); }
int main(){ CSpectrum*s=new CSpectrum(&kb,&tape); s->SetModel(Model48K); s->SetLECConnected(TRUE);
 u8 rom0=s->peek8(0x0000); s->poke8(0x8000,0x11); s->poke8(0xC000,0x22); s->poke8(0x4000,0x33);
 s->outPort(0x00FD, v(3)); chk("pagina 3 activa", s->IsLECPaged(),1);
 s->poke8(0x0000,0xA3); s->poke8(0x7FFF,0xB3);
 s->outPort(0x00FD, v(12)); s->poke8(0x0000,0xAC);
 s->outPort(0x00FD, v(3)); chk("pagina 3 conserva 0x0000", s->peek8(0x0000),0xA3); chk("pagina 3 conserva 0x7FFF", s->peek8(0x7FFF),0xB3);
 s->outPort(0x00FD, v(12)); chk("pagina 12 (b3 activo)", s->peek8(0x0000),0xAC);
 s->outPort(0x00FD, v(15)); chk("pagina 15 = RAM 0x8000", s->peek8(0x0000),0x11); chk("pagina 15 0x4000 = RAM 0xC000", s->peek8(0x4000),0x22);
 s->poke8(0x0001,0x55); s->outPort(0x00FD,0x00); chk("despaginada: ROM", s->peek8(0x0000),rom0); chk("escritura via pagina 15 llega a 0x8001", s->peek8(0x8001),0x55);
 chk("pantalla intacta", s->peek8(0x4000),0x33);
 s->outPort(0x00FD, v(3)); s->Reset(); chk("reset despagina", s->IsLECPaged(),0);
 CSpectrum*t=new CSpectrum(&kb,&tape); t->SetModel(Model128K); t->SetLECConnected(TRUE); t->outPort(0x7FFD,0x83); chk("128K: LEC no actua", t->IsLECPaged(),0);
 return fails; }
