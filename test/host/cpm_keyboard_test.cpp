#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
#include <vector>
CZXKeyboard kb; CTape tape; CSpectrum*S;
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
#define K(r,b) (((r)<<3)|(b))
void setm(std::vector<int> keys){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);}
void tap(int k,int hold,int gap){ setm({k}); run(hold); setm({}); run(gap); }
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
int main(){ S=new CSpectrum(&kb,&tape); S->SetLECConnected(TRUE); S->SetIF1Connected(TRUE); S->SetTapeMode(TapeModeFlash); run(150);
 auto t=rd("cpm.tzx"); tape.InsertTZX(t.data(),t.size(),"cpm"); S->TapeInserted();
 tap(K(6,3),6,8); setm({K(7,1),K(5,0)}); run(6); setm({}); run(8); setm({K(7,1),K(5,0)}); run(6); setm({}); run(8); tap(K(6,0),6,8); run(1500);
 // ENTER combinado con CAPS SHIFT, con SYMBOL SHIFT y con ambos
 tap(key('d'),5,4); setm({K(0,0),K(6,0)}); run(5); setm({}); run(40);
 tap(key('d'),5,4); setm({K(7,1),K(6,0)}); run(5); setm({}); run(40);
 tap(key('d'),5,4); setm({K(0,0),K(7,1),K(6,0)}); run(5); setm({}); run(40);
 // TAB: CS+SS+I (Ctrl-I?) para comparar
 tap(key('d'),5,4); setm({K(0,0),K(7,1),K(5,2)}); run(5); setm({}); run(10); tap(key('x'),5,4); tap(K(6,0),5,40);
 dump("cpmkb.ppm"); }
