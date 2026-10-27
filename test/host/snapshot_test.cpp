#include <initializer_list>
#include "snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
int FMT=0;
CZXKeyboard kb; CTape tape;
void run(CSpectrum&s,int n){for(int i=0;i<n;i++){s.RunFrame();s.GetAudio()->ClearBuffer();}}
void press(CSpectrum&s,std::initializer_list<int> keys){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(s,8);u8 z[8]={0};kb.SetGPIOMatrix(z);run(s,8);}
#define K(r,b) (((r)<<3)|(b))
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
void wr(const char*f,const u8*d,unsigned n){FILE*h=fopen(f,"wb");fwrite(d,1,n,h);fclose(h);}
static u8 buf[SNAPSHOT_MAX_SIZE];
int fails=0;
void test(int model, const char*tapf, int setup){
 CSpectrum *a=new CSpectrum(&kb,&tape); a->SetModel((TModel)model); a->SetTapeMode(TapeModeFlash); run(*a,150);
 auto t=rd(tapf); tape.InsertTAP(t.data(),t.size(),"t"); a->TapeInserted();
 if(model==0){ press(*a,{K(6,3)}); press(*a,{K(7,1),K(5,0)}); press(*a,{K(7,1),K(5,0)}); press(*a,{K(6,0)}); }
 else press(*a,{K(6,0)});
 run(*a,setup);
 unsigned n= FMT==0? CSnapshot::SaveZ80(a,buf,sizeof buf) : FMT==1? CSnapshot::SaveSNA(a,buf,sizeof buf) : CSnapshot::SaveSZX(a,buf,sizeof buf); char fn[32]; sprintf(fn,"snap_m%d.%s",model,FMT==0?"z80":FMT==1?"sna":"szx"); wr(fn,buf,n);
 printf("modelo %d: %s de %u bytes\n",model,fn,n);
 run(*a,300);
 CSpectrum *b=new CSpectrum(&kb,&tape); b->SetModel(Model48K); run(*b,10);
 const char*err=CSnapshot::Load(b,buf,n,fn); if(err){printf("  error: %s\n",err);fails++;return;}
 printf("  modelo cargado %d\n", b->GetModel());
 run(*b,300);
 bool same=!memcmp(a->GetScreenBuffer(),b->GetScreenBuffer(),SCREEN_WIDTH*SCREEN_HEIGHT*sizeof(TPixel));
 printf("  pantalla identica 300 cuadros despues: %s\n", same?"si":"NO"); if(!same)fails++;
 sprintf(fn,"snap_m%d.ppm",model); FILE*o=fopen(fn,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);{u16*p=b->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}}fclose(o);
 delete a; delete b; }
int main(int argc,char**argv){
 if(argc>2){ // cargar un fichero cualquiera y volcar pantalla
   CSpectrum *b=new CSpectrum(&kb,&tape); auto d=rd(argv[1]); const char*err=CSnapshot::Load(b,d.data(),d.size(),argv[1]);
   if(err){printf("%s: error %s\n",argv[1],err);return 1;} run(*b,100); printf("%s: modelo %d ok\n",argv[1],b->GetModel());
   FILE*o=fopen(argv[2],"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);{u16*p=b->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}}fclose(o); return 0; }
 if(argc==2) FMT=atoi(argv[1]);
 test(0,"indevzx.tap",300); test(1,"zxbtoolkit.tap",300); test(2,"zxbtoolkit.tap",300); test(4,"zxbtoolkit.tap",300);
 printf(fails?"%d fallos\n":"todo OK\n",fails); return fails; }
