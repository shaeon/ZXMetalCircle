#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
#include <vector>
CZXKeyboard kb; CTape tape;
CTape tzx; struct Saver : CTapeSaveHandler { std::vector<std::vector<u8>> blocks;
  boolean SaveTapeBlock(const u8*p,unsigned n) override { blocks.emplace_back(p,p+n); tzx.AppendTZXBlock(p,n,"x.tzx"); return TRUE; } } saver;
void run(CSpectrum&s,int n){for(int i=0;i<n;i++){s.RunFrame();s.GetAudio()->ClearBuffer();}}
void press(CSpectrum&s,std::initializer_list<int> keys){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(s,6);u8 z[8]={0};kb.SetGPIOMatrix(z);run(s,6);}
#define K(r,b) (((r)<<3)|(b))
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
void type(CSpectrum&s,const char*t){ for(;*t;t++){ if(*t=='"') press(s,{K(7,1),K(5,0)}); else press(s,{key(*t)}); } }
static const u32 pal[16]={0x060800,0x0D13A7,0xBD0707,0xC312AF,0x07BA0C,0x0DC6B4,0xBCB914,0xC2C4BC,0x060800,0x161CB0,0xCE1818,0xDC2CC8,0x28DC2D,0x36EFDE,0xEEEB46,0xFDFFF7};
void dump(CSpectrum&s,const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=s.GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
int main(){ CSpectrum*s=new CSpectrum(&kb,&tape); s->SetSaveHandler(&saver); run(*s,150);
 type(*s,"10p1\n");          // 10 PRINT 1
 type(*s,"s\"prueba\"\n");   // SAVE "prueba"
 run(*s,30); press(*s,{key('m')}); run(*s,100); // "press any key"
 printf("bloques grabados: %zu\n",saver.blocks.size());
 for(auto&b:saver.blocks){ printf("  flag %02X, %zu bytes", b[0], b.size()); if(b[0]==0) printf(", nombre '%.10s'", (const char*)&b[2]); u8 x=0; for(u8 v:b) x^=v; printf(", paridad %s\n", x==0?"ok":"MAL"); }
 // volver a cargar en una máquina nueva
 std::vector<u8> tap; for(auto&b:saver.blocks){ tap.push_back(b.size()&255); tap.push_back(b.size()>>8); tap.insert(tap.end(),b.begin(),b.end()); }
 CZXKeyboard kb2; (void)kb2;
 printf("cinta TZX en memoria: %u bloques, es TZX %d\n", tzx.GetBlockCount(), tzx.IsTZX());
 CSpectrum*s2=new CSpectrum(&kb,&tzx); s2->SetTapeMode(TapeModeFast); run(*s2,150); tzx.Rewind(); s2->TapeInserted(); s=s2;
 type(*s,"j\"\"\n"); run(*s,1500); type(*s,"r\n"); run(*s,50); dump(*s,"save_run.ppm");
}
