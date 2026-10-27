#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
CZXKeyboard kb; CTape tape; CSpectrum*S;
double now(){timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9;}
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
 if(argc>1){ FILE*f=fopen(argv[1],"rb"); static u8 d[1<<20]; int n=fread(d,1,sizeof d,f); fclose(f); S->GetDisk()->InsertDSK(0,d,n);}
 for(int i=0;i<150;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}
 u8 m[8]={0}; m[6]=1; kb.SetGPIOMatrix(m); for(int i=0;i<6;i++){S->RunFrame();S->GetAudio()->ClearBuffer();} u8 z[8]={0}; kb.SetGPIOMatrix(z);
 double t0=now(); for(int i=0;i<1000;i++){S->RunFrame();S->GetAudio()->ClearBuffer();} double t1=now();
 printf("%s: 1000 cuadros (20 s) en %.3f s -> %.0f us/cuadro\n", argc>1?argv[1]:"sin disco", t1-t0, (t1-t0)*1e6/1000); }
