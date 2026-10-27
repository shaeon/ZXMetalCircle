#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
CZXKeyboard kb; CTape tape; CSpectrum*S;
class CFileDev : public CBlockDevice { FILE*f; u32 n; public:
 CFileDev(const char*p){ f=fopen(p,"r+b"); fseek(f,0,SEEK_END); n=ftell(f)/512; }
 u32 GetSectors() const { return n; }
 boolean ReadSector(u32 s,u8*b){ fseek(f,(long)s*512,SEEK_SET); return fread(b,1,512,f)==512; }
 boolean WriteSector(u32 s,const u8*b){ fseek(f,(long)s*512,SEEK_SET); bool ok=fwrite(b,1,512,f)==512; fflush(f); return ok; } };
void run(int n){for(int i=0;i<n;i++){S->RunFrame();S->GetAudio()->ClearBuffer();}}
void press(std::initializer_list<int> keys,int hold=6){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(hold);u8 z[8]={0};kb.SetGPIOMatrix(z);run(8);}
#define K(r,b) (((r)<<3)|(b))
#define CS K(0,0)
#define SS K(7,1)
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
void type(const char*t){ for(;*t;t++){ char c=*t; if(c=='\\' && t[1]=='n'){ t++; c='\n'; } if(c=='"') press({SS,K(5,0)}); else if(c==':') press({SS,K(0,1)}); else if(c=='.') press({SS,K(7,2)}); else if(c==',') press({SS,K(7,3)}); else if(c=='*') press({SS,K(7,4)}); else if(c=='-') press({SS,K(6,3)}); else if(c>='A'&&c<='Z') press({CS,key(c-'A'+'a')}); else press({key(c)}); if(c=='\n') run(60); } }
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
int main(int argc,char**argv){ S=new CSpectrum(&kb,&tape);
 CFileDev*d=argc>1?new CFileDev(argv[1]):0; if(d) S->GetSDCard()->Insert(d);
 { static u8 r[65536]; for(int i=0;i<4;i++){ char n[128]; sprintf(n,"/home/claude/zxcircle/roms/plus3e-es-%d.rom",i); FILE*f=fopen(n,"rb"); fread(r+i*16384,1,16384,f); fclose(f);} S->SetModelROM(ModelPlus3E,r); }
 S->SetModel(ModelPlus3E); run(250); dump("p3e_boot.ppm");
 for(int i=2;i<argc;i++){ if(!strncmp(argv[i],"tap:",4)){ static u8 tb[65536]; FILE*h=fopen(argv[i]+4,"rb"); int n=fread(tb,1,sizeof tb,h); fclose(h); tape.InsertTAP(tb,n,"t"); } else if(!strcmp(argv[i],"reset")){ S->Reset(); run(250);} else if(!strcmp(argv[i],"loader")){ press({K(6,0)}); run(150);} else if(!strcmp(argv[i],"down")){ press({CS,K(4,4)}); } else if(!strcmp(argv[i],"enter")){ press({K(6,0)}); } else if(!strcmp(argv[i],"basic")){ press({CS,K(4,4)}); press({K(6,0)}); run(120);} else if(!strncmp(argv[i],"dump:",5)) dump(argv[i]+5); else if(!strncmp(argv[i],"wait:",5)) run(atoi(argv[i]+5)); else type(argv[i]); }
}
