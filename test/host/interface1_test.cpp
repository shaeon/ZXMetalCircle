#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
CZXKeyboard kb; CTape tape;
CSpectrum*S;
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
void press(std::initializer_list<int> keys){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(6);u8 z[8]={0};kb.SetGPIOMatrix(z);run(6);}
#define K(r,b) (((r)<<3)|(b))
#define CS K(0,0)
#define SS K(7,1)
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
// " -> SS+P ; ; -> SS+O ; * -> SS+B
void type(const char*t){ for(;*t;t++){ if(*t=='"') press({SS,K(5,0)}); else if(*t==';') press({SS,K(5,1)}); else if(*t=='*') press({SS,K(7,4)}); else press({key(*t)}); } }
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
static u8 mdr[MDR_FILE_SIZE];
int main(){ S=new CSpectrum(&kb,&tape); S->SetIF1Connected(TRUE); run(150);
 S->GetInterface1()->GetDrive(0)->InsertBlank();
 // FORMAT "m";1;"test"
 press({CS,SS}); press({SS,K(4,0)}); type("\"m\";1;\"test\"\n"); run(2500); dump("if1_format.ppm");
 // 10 PRINT 9 ; SAVE *"m";1;"prog"
 type("10p9\n"); type("s*\"m\";1;\"prog\"\n"); run(600); dump("if1_save.ppm");
 unsigned n=S->GetInterface1()->GetDrive(0)->ExportMDR(mdr,sizeof mdr);
 printf("MDR exportado: %u bytes = %u sectores\n", n, (n-1)/543);
 FILE*f=fopen("test.mdr","wb"); fwrite(mdr,1,n,f); fclose(f);
 // NEW + cargar desde un cartucho NUEVO hecho con el MDR exportado
 S->GetInterface1()->GetDrive(0)->Eject(); S->GetInterface1()->GetDrive(0)->InsertMDR(mdr,n);
 type("a\n"); run(100);
 type("j*\"m\";1;\"prog\""); run(10); dump("if1_line.ppm"); type("\n"); run(600); type("r\n"); run(50); dump("if1_load.ppm");
 // CAT 1
 press({CS,SS}); press({SS,K(4,1)}); type("1\n"); run(600); dump("if1_cat.ppm");
}
