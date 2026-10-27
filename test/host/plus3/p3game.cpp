#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
CZXKeyboard kb; CTape tape; CSpectrum*S;
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
void press(std::initializer_list<int> keys,int hold=6){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(hold);u8 z[8]={0};kb.SetGPIOMatrix(z);run(8);}
#define K(r,b) (((r)<<3)|(b))
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
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
int main(int argc,char**argv){ S=new CSpectrum(&kb,&tape); SetPlus3ROM(S); S->SetModel(ModelPlus3);
 FILE*f=fopen(argv[1],"rb"); static u8 d[1<<20]; int n=fread(d,1,sizeof d,f); fclose(f); printf("insertar: %d\n", S->GetDisk()->InsertDSK(0,d,n));
 run(150);
 if(argc>4){ press({K(0,0),K(4,4)}); press({K(6,0)}); run(80);
   const char*t="cat\n"; for(;*t;t++){ int k= *t=='c'?K(0,3): *t=='a'?K(1,0): *t=='t'?K(2,4):K(6,0); press({k}); } run(400); dump("cat_dbg.ppm"); return 0; }
 press({K(6,0)});   // Loader
 int secs=atoi(argv[3]);
 for(int s=0;s<secs;s+=10){ run(500); char nm[64]; sprintf(nm,"%s_%03d.ppm",argv[2],s+10); dump(nm);} }
