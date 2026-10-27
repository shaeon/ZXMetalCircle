#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
CZXKeyboard kb; CTape tape;
void run(CSpectrum&s,int n){for(int i=0;i<n;i++){s.RunFrame();s.GetAudio()->ClearBuffer();}}
void press(CSpectrum&s,std::initializer_list<int> keys){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(s,6);u8 z[8]={0};kb.SetGPIOMatrix(z);run(s,6);}
#define K(r,b) (((r)<<3)|(b))
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
void type(CSpectrum&s,const char*t){ for(;*t;t++){ if(*t=='"') press(s,{K(7,1),K(5,0)}); else press(s,{key(*t)}); } }
struct Saver : CTapeSaveHandler { int n=0; boolean SaveTapeBlock(const u8*,unsigned) override { n++; return TRUE; } } saver;
int main(int argc,char**argv){ int model=argc>1?atoi(argv[1]):0;
 CSpectrum*s=new CSpectrum(&kb,&tape); s->SetModel((TModel)model); s->SetSaveHandler(&saver); run(*s,150);
 if(model){ press(*s,{K(4,2)}); press(*s,{K(4,2)}); press(*s,{K(6,0)}); run(*s,50);} // 128 BASIC (cursor abajo x1 -> "128 BASIC")
 type(*s,model? "10 print 7\n" : "10p7\n");
 s->StartRecording();
 type(*s,model? "save \"rec\"\n" : "s\"rec\"\n"); run(*s,30); press(*s,{key('m')});
 run(*s,400); s->StopRecording();
 unsigned len,pulses,rate; const u8*d=s->GetRecording(&len,&pulses,&rate);
 printf("modelo %d: grabado: %u pulsos, %u bytes RLE, %u Hz; trap de SAVE usado %d veces\n",model,pulses,len,rate,saver.n);
 // construir TZX con bloque 0x18
 std::vector<u8> t={'Z','X','T','a','p','e','!',0x1A,1,20};
 u32 bl=10+len; t.push_back(0x18); for(int i=0;i<4;i++) t.push_back(bl>>(8*i)); t.push_back(0);t.push_back(0);
 for(int i=0;i<3;i++) t.push_back(rate>>(8*i)); t.push_back(1); for(int i=0;i<4;i++) t.push_back(pulses>>(8*i)); t.insert(t.end(),d,d+len);
 FILE*f=fopen(model?"rec128.tzx":"rec48.tzx","wb"); fwrite(t.data(),1,t.size(),f); fclose(f);
 // cargar en máquina nueva, modo real
 CSpectrum*b=new CSpectrum(&kb,&tape); b->SetModel((TModel)model); b->SetTapeMode(TapeModeNormal); run(*b,150);
 tape.InsertTZX(t.data(),t.size(),"rec"); b->TapeInserted();
 if(model){ press(*b,{K(6,0)}); } else type(*b,"j\"\"\n");
 run(*b,1500); if(model){ press(*b,{K(7,1),K(6,0)}); } // nada
 type(*b, model? "" : "r\n"); run(*b,50);
 FILE*o=fopen(model?"rec128.ppm":"rec48.ppm","wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=b->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);
}
