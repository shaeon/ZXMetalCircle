#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
#include <vector>
CZXKeyboard kb; CTape tape; CSpectrum*S;
void run(int n){for(int i=0;i<n;i++){kb.SetBackspaceAsLeft(S->IsLECInUse());S->RunFrame();S->GetAudio()->ClearBuffer();}}
void usb(u8 k){u8 r[6]={k,0,0,0,0,0},z[6]={0};kb.UpdateFromReport(0,r);run(6);kb.UpdateFromReport(0,z);run(8);}
void pressv(std::vector<int> keys){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(6);u8 z[8]={0};kb.SetGPIOMatrix(z);run(8);}
#define K(r,b) (((r)<<3)|(b))
#define CS K(0,0)
#define SS K(7,1)
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
void type(const char*t){ for(;*t;t++) pressv({key(*t)}); }
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
int main(){ S=new CSpectrum(&kb,&tape); S->SetLECConnected(TRUE); S->SetIF1Connected(TRUE); S->SetTapeMode(TapeModeFlash); run(150);
 S->GetInterface1()->GetDrive(0)->InsertBlank();
 auto t=rd("cpm.tzx"); tape.InsertTZX(t.data(),t.size(),"cpm"); S->TapeInserted();
 pressv({K(6,3)}); pressv({SS,K(5,0)}); pressv({SS,K(5,0)}); pressv({K(6,0)}); run(1500);
 printf("LEC en uso: %d, paginada ahora: %d\n", S->IsLECInUse(), S->IsLECPaged());
 kb.SetJoystickMode(JoystickKempston);
 type("carlos"); usb(0x2A); usb(0x2A); type("z"); run(10); pressv({K(6,0)}); run(30);   // borrar x2 (tecla del PC)
 type("abc"); usb(0x50); type("z"); run(10); pressv({K(6,0)}); run(30);                // flecha izquierda del PC
 dump("cpmkeys.ppm"); }
