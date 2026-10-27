#include <stdlib.h>
#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
#include <vector>
CZXKeyboard kb; CTape tape; CSpectrum*S;
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
void press(std::initializer_list<int> keys,int hold=5){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(hold);u8 z[8]={0};kb.SetGPIOMatrix(z);run(5);}
#define K(r,b) (((r)<<3)|(b))
#define CS K(0,0)
#define SS K(7,1)
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
void type(const char*t){ for(;*t;t++){ char c=*t; if(c=='"') press({SS,K(5,0)}); else if(c==':') press({SS,K(0,1)}); else if(c=='.') press({SS,K(7,2)}); else press({key(c)}); if(c=='\n') run(40); } }
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
static u8 buf[1<<20];
// La ROM del +3 no va dentro del kernel (se carga de la SD): sin ella el
// modelo +3 arranca como un 128K. Se arma con roms/plus3-0..3.rom.
#ifndef ROMDIR
#define ROMDIR "../../roms/"
#endif
static void SetPlus3ROM (CSpectrum *p)
{
	static u8 r[65536];
	for (int i = 0; i < 4; i++)
	{
		char n[256]; snprintf (n, sizeof n, ROMDIR "plus3-%d.rom", i);
		FILE *f = fopen (n, "rb");
		if (f == 0) { printf ("falta %s\n", n); exit (1); }
		if (fread (r + i * 16384, 1, 16384, f) != 16384) { printf ("%s incompleta\n", n); exit (1); }
		fclose (f);
	}
	p->SetModelROM (ModelPlus3, r);
}
int main(int argc,char**argv){ S=new CSpectrum(&kb,&tape); SetPlus3ROM(S); S->SetModel(ModelPlus3); run(150); dump("p3_boot.ppm");
 S->GetDisk()->InsertBlank(0);
 press({CS,K(4,4)}); press({K(6,0)}); run(80);
 dump("p3_menu2.ppm");
 type("10 print 42\n"); type("save \"prueba\"\n"); run(300); dump("p3_save.ppm");
 type("cat\n"); run(300); dump("p3_cat.ppm");
 unsigned n=S->GetDisk()->Save(0,buf,sizeof buf); FILE*o=fopen("p3_saved.dsk","wb"); fwrite(buf,1,n,o); fclose(o); printf("DSK guardado: %u bytes, modificado %d\n",n,S->GetDisk()->IsModified(0));
 type("new\n"); run(50); type("load \"prueba\"\n"); run(300); type("run\n"); run(50); dump("p3_load.ppm");
}
