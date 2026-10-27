#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
CZXKeyboard kb; CTape tape; CSpectrum*S;
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
void press(std::initializer_list<int> keys,int hold=6){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(hold);u8 z[8]={0};kb.SetGPIOMatrix(z);run(8);}
#define K(r,b) (((r)<<3)|(b))
#define SS K(7,1)
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
void IF1Stats();
int main(){ S=new CSpectrum(&kb,&tape);
 static u8 rom[8192]; FILE*f=fopen("/home/claude/zxcircle/roms/if1-2.rom","rb"); if(!f) f=fopen("/home/claude/zxcircle/sdcard/roms/if1.rom","rb"); fread(rom,1,8192,f); fclose(f);
 S->SetIF1ROM(rom); S->SetModel(Model48K); S->SetIF1Connected(TRUE); S->Reset(); run(120);
 static u8 t[4096]; f=fopen("/tmp/rs/rs232.tap","rb"); int n=fread(t,1,sizeof t,f); fclose(f);
 tape.InsertTAP(t,n,"rs"); S->SetTapeMode(TapeModeFlash);
 press({K(6,3)}); press({SS,K(5,0)}); press({SS,K(5,0)}); press({K(6,0)}); run(250);
 CByteQueue *in=S->GetInterface1()->GetRS232In(); for(const char*p="ZX OK\r";*p;p++) in->Put(*p);
 run(600); IF1Stats(); dump("/tmp/rs/rs.ppm");
 CByteQueue *out=S->GetInterface1()->GetRS232Out(); u8 c; printf("salida RS232: \""); while(out->Get(&c)) printf(c>=32&&c<127?"%c":"\\x%02X",c); printf("\"\n"); }
