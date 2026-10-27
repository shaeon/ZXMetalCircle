#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
CZXKeyboard kb; CTape tape; CSpectrum*S;
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
void press(std::initializer_list<int> keys,int hold=6){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(hold);u8 z[8]={0};kb.SetGPIOMatrix(z);run(6);}
#define K(r,b) (((r)<<3)|(b))
#define CS K(0,0)
#define SS K(7,1)
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
void type(const char*t){ for(;*t;t++){ char c=*t; if(c=='"') press({SS,K(5,0)}); else if(c==':') press({SS,K(0,1)}); else if(c=='.') press({SS,K(7,2)}); else if(c>='A'&&c<='Z') press({CS,key(c-'A'+'a')}); else press({key(c)}); } }
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
int main(int argc,char**argv){ S=new CSpectrum(&kb,&tape); S->SetLECConnected(TRUE); S->SetIF1Connected(TRUE); S->SetTapeMode(TapeModeFlash); run(150);
 for(int d=0;d<2;d++) S->GetInterface1()->GetDrive(d)->InsertBlank();
 if(getenv("LOAD")){ auto m=rd(getenv("LOAD")); S->GetInterface1()->GetDrive(0)->Eject(); bool ok= strstr(getenv("LOAD"),".mdv")? S->GetInterface1()->GetDrive(0)->InsertMDV(m.data(),m.size()) : S->GetInterface1()->GetDrive(0)->InsertMDR(m.data(),m.size()); printf("cargado %s: %d\n",getenv("LOAD"),ok); }
 auto t=rd("cpm.tzx"); tape.InsertTZX(t.data(),t.size(),"cpm"); S->TapeInserted();
 press({K(6,3)}); press({SS,K(5,0)}); press({SS,K(5,0)}); press({K(6,0)});
 run(1500); dump("c0.ppm");
 const char*cmds[16]={0}; int nc=argc-1; for(int i=0;i<nc&&i<16;i++) cmds[i]=argv[i+1];
 for(int c=0;c<nc;c++){ if(!cmds[c][0]) continue; if(strcmp(cmds[c],"E")) type(cmds[c]); press({K(6,0)});
   for(int i=0;i<(!strcmp(cmds[c],"E")||c==nc-1?40:6);i++){ run(250); } char f[32]; sprintf(f,"c%d.ppm",c+1); dump(f);
   printf("tras '%s': motor %d, modificado %d\n", cmds[c], S->GetInterface1()->GetRunningDrive(), S->GetInterface1()->GetDrive(0)->IsModified()); }
 if(getenv("MDV")){ static u8 r[MDR_FILE_SIZE]; unsigned nr=S->GetInterface1()->GetDrive(0)->ExportMDR(r,sizeof r); FILE*q=fopen("cpm.mdr","wb"); fwrite(r,1,nr,q); fclose(q); printf("MDR %u bytes\n",nr); static u8 b[4<<20]; unsigned n=S->GetInterface1()->GetDrive(0)->ExportMDV(b,sizeof b); FILE*o=fopen("cpm.mdv","wb"); fwrite(b,1,n,o); fclose(o); printf("MDV %u bytes, Sinclair %d\n",n,S->GetInterface1()->GetDrive(0)->IsSinclairFormat()); }
}
