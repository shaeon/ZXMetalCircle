#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
CZXKeyboard kb; CTape tape; CSpectrum*S; CEarInput ear;
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
int main(int argc,char**argv){
 // 1) Señal del "casete": flancos de la cinta reproducida en tiempo real (t-states absolutos)
 CTape src; auto t=rd(argv[1]); src.InsertTAP(t.data(),t.size(),"x"); src.SetMachine(3500000,true);
 src.Play(0); std::vector<std::pair<u64,bool>> edges; bool last=src.GetEarBit(0);
 u64 T=0; for(T=0; T<3500000ULL*40 && !src.IsAtEnd(); T+=4){ bool b=src.GetEarBit(T); if(b!=last){ edges.push_back({T,b}); last=b; } }
 printf("casete: %zu flancos en %.1f s\n", edges.size(), T/3500000.0);
 int jitter=argc>2?atoi(argv[2]):0;   // ruido de temporización (t-states) como el de la interrupción
 // 2) Spectrum 48K con la entrada EAR externa: LOAD ""
 S=new CSpectrum(&kb,&tape); S->SetModel(Model48K); ear.SetEnabled(TRUE); S->SetEarInput(&ear); run(120);
 auto key=[&](std::initializer_list<int> k){u8 m[8]={0};for(int x:k)m[x>>3]|=1<<(x&7);kb.SetGPIOMatrix(m);run(6);u8 z[8]={0};kb.SetGPIOMatrix(z);run(6);};
 key({6*8+3}); key({7*8+1,5*8+0}); key({7*8+1,5*8+0}); key({6*8+0});
 // la cinta empieza "ahora"
 const u64 F=69888; size_t k=0; u64 start=0; srand(1);
 for(int f=0; f<50*45; f++){
   ear.BeginFrame(); u64 w0=start+(u64)f*F;
   while(k<edges.size() && edges[k].first < w0+F){ long long o=(long long)(edges[k].first-w0); if(jitter) o+= (rand()%(2*jitter+1))-jitter; if(o<0)o=0; if(o>=(long long)F)o=F-1; ear.AddEdge((unsigned)o, edges[k].second); k++; }
   S->RunFrame(); S->GetAudio()->ClearBuffer(); }
 run(100); dump("ear_result.ppm"); printf("flancos entregados: %zu\n", k);
}
