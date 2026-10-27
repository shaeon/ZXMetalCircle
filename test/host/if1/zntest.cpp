#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
#include <deque>
#include <vector>
// transporte en memoria: lo que manda uno lo recibe el otro
struct CMemNet : CZXNetTransport { std::deque<std::vector<u8>> *pIn, *pOut; int nSent=0;
  const char*name="?"; void Send(const u8*p,unsigned n){ pOut->push_back(std::vector<u8>(p,p+n)); nSent++; if(nSent<=20){ fprintf(stderr,"%s envia tipo %d, %u bytes, cab:",name,p[5],n); for(int i=6;i<14&&i<(int)n;i++) fprintf(stderr," %02X",p[i]); fprintf(stderr,"\n");} }
  unsigned Receive(u8*b,unsigned s,boolean){ if(pIn->empty()) return 0; auto v=pIn->front(); pIn->pop_front(); unsigned n=v.size()<s?v.size():s; memcpy(b,v.data(),n); return n; } };
std::deque<std::vector<u8>> qAB, qBA;
struct M { CZXKeyboard kb; CTape tape; CSpectrum *S; CMemNet net; };
void run(M&m,int n){ for(int i=0;i<n;i++){ m.S->RunFrame(); m.S->GetAudio()->ClearBuffer(); } }
void press(M&m,std::initializer_list<int> keys){u8 k[8]={0};for(int x:keys)k[x>>3]|=1<<(x&7);m.kb.SetGPIOMatrix(k);run(m,6);u8 z[8]={0};m.kb.SetGPIOMatrix(z);run(m,8);}
#define K(r,b) (((r)<<3)|(b))
#define SS K(7,1)
void dump(M&m,const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=m.S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
static u8 rom[8192];
extern void *g_DbgNet;
void setup(M&m,const char*tapname,std::deque<std::vector<u8>>*in,std::deque<std::vector<u8>>*out){
  m.S=new CSpectrum(&m.kb,&m.tape); m.S->SetIF1ROM(rom); m.S->SetModel(Model48K); m.S->SetIF1Connected(TRUE); m.S->Reset();
  m.net.pIn=in; m.net.pOut=out; m.S->GetInterface1()->GetZXNet()->GetSocket()->SetTransport(&m.net); run(m,120);
  static u8 t[2][4096]; static int k=0; FILE*f=fopen(tapname,"rb"); int n=fread(t[k],1,4096,f); fclose(f);
  m.tape.InsertTAP(t[k],n,"x"); k++; m.S->SetTapeMode(TapeModeFlash);
  press(m,{K(6,3)}); press(m,{SS,K(5,0)}); press(m,{SS,K(5,0)}); press(m,{K(6,0)}); }
int main(){ FILE*f=fopen("/home/claude/zxcircle/roms/if1-2.rom","rb"); fread(rom,1,8192,f); fclose(f);
 static M a,b; a.net.name="A"; b.net.name="B"; setup(b,"/tmp/zn/b.tap",&qAB,&qBA); g_DbgNet=b.S->GetInterface1()->GetZXNet(); run(b,100); setup(a,"/tmp/zn/a.tap",&qBA,&qAB);
 int found=-1; fprintf(stderr,"inicio bucle\n"); for(int i=0;i<1500;i++){ run(a,1); run(b,1); if(i==0) fprintf(stderr,"primer cuadro\n");
   if(found<0){ const char*t="PROGRAMA DE LA ESTACION 1"; for(int ad=0x5C00; ad<0xFF00 && found<0; ad++){ int k=0; while(t[k] && b.S->ReadMemory(ad+k)==(u8)t[k]) k++; if(!t[k]) found=i; } if(found>=0) printf("B tiene el programa de A en el cuadro %d\n", i); } }
 { u16 prog=b.S->ReadMemory(0x5C53)|(b.S->ReadMemory(0x5C54)<<8), vars=b.S->ReadMemory(0x5C4B)|(b.S->ReadMemory(0x5C4C)<<8); printf("B: PROG %04X VARS %04X: ",prog,vars); for(int ad=prog; ad<vars && ad<prog+120; ad++){ u8 c=b.S->ReadMemory(ad); printf(c>=32&&c<127?"%c":"[%02X]",c);} printf("\n"); }
 printf("paquetes enviados: A %d, B %d\n", a.net.nSent, b.net.nSent);
 dump(a,"/tmp/zn/a.ppm"); dump(b,"/tmp/zn/b.ppm");
 // en B: LIST
 press(b,{K(6,1)}); press(b,{K(6,0)}); run(b,50); dump(b,"/tmp/zn/b2.ppm"); }
