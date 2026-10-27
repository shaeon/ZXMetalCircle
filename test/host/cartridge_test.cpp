#include "spectrum.h"
#include <stdio.h>
#include <vector>
CZXKeyboard kb; CTape tape;
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
void dump(CSpectrum*s,const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=s->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
void run(CSpectrum*s,int n){for(int i=0;i<n;i++){s->RunFrame();s->GetAudio()->ClearBuffer();}}
int main(){ auto r=rd("test_if2.rom"); auto r8=rd("test_if2_8k.rom");
 for(int m: {0,1,4,2}){ CSpectrum*s=new CSpectrum(&kb,&tape); s->SetModel((TModel)m);
  bool ok=s->InsertCartridge(m==1? r8.data():r.data(), m==1? r8.size():r.size()); run(s,50);
  char f[32]; sprintf(f,"if2_m%d.ppm",m); dump(s,f); printf("modelo %d: cartucho %s\n",m, ok?"insertado":"rechazado");
  if(ok){ s->EjectCartridge(); run(s,150); sprintf(f,"if2_m%d_out.ppm",m); dump(s,f);} delete s; } }
