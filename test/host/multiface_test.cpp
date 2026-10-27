#include <string>
#include "spectrum.h"
#include <stdio.h>
static const u32 pal[16]={0x060800,0x0D13A7,0xBD0707,0xC312AF,0x07BA0C,0x0DC6B4,0xBCB914,0xC2C4BC,0x060800,0x161CB0,0xCE1818,0xDC2CC8,0x28DC2D,0x36EFDE,0xEEEB46,0xFDFFF7};
void dump(CSpectrum&s,const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=s.GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
CZXKeyboard kb; CTape tape;
void run(CSpectrum&s,int n){for(int i=0;i<n;i++){s.RunFrame();s.GetAudio()->ClearBuffer();}}
void key(CSpectrum&s,int k){u8 m[8]={0};m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(s,10);u8 z[8]={0};kb.SetGPIOMatrix(z);run(s,25);}
u8 rom[3][8192];
int main(int argc,char**argv){ const char*f[3]={"mf1.rom","mf128.rom","mfplus3.rom"};
 CSpectrum*s=new CSpectrum(&kb,&tape);
 for(int i=0;i<3;i++){FILE*r=fopen((std::string("/tmp/JSpeccy/src/main/resources/roms/")+f[i]).c_str(),"rb");fread(rom[i],1,8192,r);fclose(r);s->SetMultifaceROM((TCodeModel)i,rom[i]);}
 for(int m=0;m<3;m++){ s->SetModel((TModel)m); run(*s,150);
   if(m>0){ u8 mm[8]={0};mm[6]=1;kb.SetGPIOMatrix(mm);run(*s,10);u8 z[8]={0};kb.SetGPIOMatrix(z);run(*s,100);} // enter 128 BASIC/ Loader? -> just Enter on menu
   s->TriggerNMI(); run(*s,100); printf("modelo %d: MF paginado=%d\n",m,s->IsMultifacePaged());
   char fn[32]; sprintf(fn,"mf_%d_a.ppm",m); dump(*s,fn);
   // R = return (row2 bit3)
   key(*s,(2<<3)|3); run(*s,50); printf("   tras R: paginado=%d\n",s->IsMultifacePaged());
   sprintf(fn,"mf_%d_b.ppm",m); dump(*s,fn); }
}
