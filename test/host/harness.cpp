#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
static const u32 pal[16]={0x060800,0x0D13A7,0xBD0707,0xC312AF,0x07BA0C,0x0DC6B4,0xBCB914,0xC2C4BC,0x060800,0x161CB0,0xCE1818,0xDC2CC8,0x28DC2D,0x36EFDE,0xEEEB46,0xFDFFF7};
void dump(CSpectrum&s,const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=s.GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
CZXKeyboard kb; CTape tape;
void key(u8 mods, std::initializer_list<u8> k){u8 r[6]={0};int i=0;for(u8 x:k)r[i++]=x;kb.UpdateFromReport(mods,r);}
void run(CSpectrum&s,int n){for(int i=0;i<n;i++){s.RunFrame();s.GetAudio()->ClearBuffer();}}
void typeLoad(CSpectrum&s,bool is128){
 if(is128){ key(0,{0x28}); run(s,5); key(0,{}); run(s,100); return; } // Tape Loader option = ENTER on 128 menu
 key(0,{0x0D}); run(s,8); key(0,{}); run(s,8);            // J -> LOAD
 for(int i=0;i<2;i++){ key(0x01,{0x13}); run(s,8); key(0,{}); run(s,8);} // SS+P = "
 key(0,{0x28}); run(s,8); key(0,{}); run(s,8);}
int main(int argc,char**argv){
 int model=atoi(argv[1]); int mode=atoi(argv[2]); const char*tap=argv[3]; int frames=atoi(argv[4]);
 CSpectrum *s=new CSpectrum(&kb,&tape); s->SetModel((TModel)model); s->SetTapeMode((TTapeMode)mode);
 run(*s,150); dump(*s,"boot.ppm");
 FILE*f=fopen(tap,"rb");static u8 buf[2<<20];int n=fread(buf,1,sizeof buf,f);fclose(f);
 { int L=strlen(tap); bool z=L>4&&!strcasecmp(tap+L-4,".tzx"); bool c=L>4&&!strcasecmp(tap+L-4,".csw"); bool ok= z? tape.InsertTZX(buf,n,"t") : (c? tape.InsertCSW(buf,n,"t") : tape.InsertTAP(buf,n,"t")); printf("insert %d blocks %u unsupported %u\n",ok,tape.GetBlockCount(),tape.GetUnsupportedBlocks()); } s->TapeInserted();
 typeLoad(*s, model!=0);
 int accel=0; for(int i=0;i<frames;i++){ s->RunFrame(); s->GetAudio()->ClearBuffer(); if(s->IsTapeAccelerating())accel++;
   if(i%500==0) printf("frame %d block %u/%u playing %d\n",i,tape.GetBlock(),tape.GetBlockCount(),tape.IsPlaying()); }
 printf("accel frames %d\n",accel);
 dump(*s,"end.ppm"); }
