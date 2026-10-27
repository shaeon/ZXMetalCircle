#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <math.h>
#include <vector>
CZXKeyboard kb; CTape tape;
void energy(CSpectrum&s,int frames,double*l,double*r){ *l=*r=0; long n=0;
 for(int f=0;f<frames;f++){ s.RunFrame(); CAudioMixer*a=s.GetAudio(); const s16*b=a->GetBuffer();
   for(unsigned i=0;i<a->GetFrameCount();i++){ *l+=b[2*i]*(double)b[2*i]; *r+=b[2*i+1]*(double)b[2*i+1]; n++; } a->ClearBuffer(); }
 *l=sqrt(*l/n); *r=sqrt(*r/n); }
void ay(CSpectrum&s,int reg,int v){ s.GetAY()->SetAddressLatch(reg); s.GetAY()->WriteRegister(v); }
int main(){ CSpectrum*s=new CSpectrum(&kb,&tape); s->SetModel(Model128K);
 for(int i=0;i<100;i++){s->RunFrame();s->GetAudio()->ClearBuffer();}
 const char*names[]={"ABC","ACB","BAC","mono"};
 for(int ch=0;ch<3;ch++){ printf("canal %c:",'A'+ch);
  for(int m=0;m<4;m++){ s->GetAY()->SetStereoMode((TAYStereoMode)m);
   for(int r=0;r<14;r++) ay(*s,r,0); ay(*s,7,0x3F & ~(1<<ch)); ay(*s,ch*2,100); ay(*s,8+ch,15);
   double l,r; energy(*s,25,&l,&r); printf("  %s I=%5.0f D=%5.0f",names[m],l,r); }
  printf("\n"); }
 // sonido de carga
 for(int on=1;on>=0;on--){ CSpectrum*t=new CSpectrum(&kb,&tape); t->SetTapeMode(TapeModeNormal); t->GetAudio()->SetTapeSound(on);
  for(int i=0;i<150;i++){t->RunFrame();t->GetAudio()->ClearBuffer();}
  FILE*f=fopen("indevzx.tap","rb"); static u8 buf[65536]; int n=fread(buf,1,sizeof buf,f); fclose(f); tape.InsertTAP(buf,n,"t"); t->TapeInserted();
  t->TapePlay(); double l,r; energy(*t,50,&l,&r); printf("cinta en marcha, tapesound=%d: nivel %5.0f\n",on,l); tape.Stop(); delete t; }
}
