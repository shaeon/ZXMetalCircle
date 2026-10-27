#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
CZXKeyboard kb; CTape tape; CSpectrum*S;
void DumpLecPorts();
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
void press(std::initializer_list<int> keys,int hold=6){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(hold);u8 z[8]={0};kb.SetGPIOMatrix(z);run(8);}
#define K(r,b) (((r)<<3)|(b))
#define CS K(0,0)
#define SS K(7,1)
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
int main(int argc,char**argv){ S=new CSpectrum(&kb,&tape);
 static u8 t[200000]; FILE*f=fopen(argv[1],"rb"); int n=fread(t,1,sizeof t,f); fclose(f);
 const char *m=argc>3?argv[3]:"48"; TModel M=!strcmp(m,"128")?Model128K:!strcmp(m,"plus3")?ModelPlus3:!strcmp(m,"plus2a")?ModelPlus2A:Model48K;
 S->SetModel(M); S->SetLECConnected(TRUE); S->Reset(); run(150);
 if(M!=Model48K){ int downs=3; for(int i=0;i<downs;i++) press({CS,K(4,4)}); press({K(6,0)}); run(100); }
 tape.InsertTZX(t,n,"cpm"); S->SetTapeMode(TapeModeFlash);
 press({K(6,3)}); press({SS,K(5,0)}); press({SS,K(5,0)}); press({K(6,0)}); run(1500); dump(argv[2]); DumpLecPorts(); }
