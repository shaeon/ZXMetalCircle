#include <initializer_list>
#include "spectrum.h"
#include "posixnet.h"
#include <stdlib.h>
__attribute__((weak)) unsigned g_Hist[65536]; __attribute__((weak)) int g_HistOn;
int g_Frames=0; int g_NetDelay=0;
CZXKeyboard kb; CTape tape; CSpectrum*S; CPosixNet net;
void run(int n){for(int i=0;i<n;i++){g_Frames++;S->RunFrame();S->GetAudio()->ClearBuffer();S->GetSpectranet()->Poll();}}
void press(std::initializer_list<int> keys,int hold=6){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(hold);u8 z[8]={0};kb.SetGPIOMatrix(z);run(8);}
#define K(r,b) (((r)<<3)|(b))
#define CS K(0,0)
#define SS K(7,1)
int key(char c){ const char*rows[8]={"#zxcv","asdfg","qwert","12345","09876","poiuy","\nlkjh"," $mnb"};
 for(int r=0;r<8;r++)for(int b=0;b<5;b++) if(rows[r][b]==c) return K(r,b); return -1;}
void type(const char*t){ for(;*t;t++){ char c=*t; if(c=='\\'&&t[1]=='n'){t++;c='\n';}
 if(c=='"') press({SS,K(5,0)}); else if(c=='#') press({SS,K(3,2)}); else if(c==';') press({SS,K(5,1)}); else if(c=='$') press({SS,K(3,3)}); else if(c=='%') press({SS,K(3,4)}); else if(c==',') press({SS,K(7,3)}); else if(c=='.') press({SS,K(7,2)}); else if(c==':') press({SS,K(0,1)}); else if(c>='A'&&c<='Z') press({CS,key(c-'A'+'a')}); else press({key(c)}); if(c=='\n') run(30);} }
void dump(const char*f){FILE*o=fopen(f,"wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=S->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o);}
int main(int argc,char**argv){ S=new CSpectrum(&kb,&tape); if(getenv("NETDELAY")) g_NetDelay=atoi(getenv("NETDELAY"));
 static u8 fl[131072]; memset(fl,0xFF,sizeof fl); bool install=!strcmp(argv[1],"vacia"); if(!install){ FILE*f=fopen(argv[1],"rb"); fread(fl,1,sizeof fl,f); fclose(f);} 
 S->GetSpectranet()->SetNet(&net); S->GetSpectranet()->LoadFlash(fl,sizeof fl);
 S->GetSpectranet()->SetTrapsDisabled(install);
 S->SetModel(getenv("M128")?Model128K:Model48K); S->SetSpectranetEnabled(TRUE); S->Reset();
 for(int i=2;i<argc;i++){ if(!strncmp(argv[i],"dump:",5)) dump(argv[i]+5); else if(!strncmp(argv[i],"wait:",5)) run(atoi(argv[i]+5)); else if(!strcmp(argv[i],"nmi")){ S->TriggerNMI(); run(50);} else if(!strncmp(argv[i],"key:",4)) { press({key(argv[i][4])}); }
   else if(!strncmp(argv[i],"poke:",5)) { FILE*b=fopen(argv[i]+5,"rb"); int a=27000,c; while((c=fgetc(b))!=EOF) S->poke8(a++,c); fclose(b); fprintf(stderr,"cargado hasta %d\n",a); }
   else if(!strcmp(argv[i],"estado")) { fprintf(stderr,"paginada=%d A=%02X B=%02X\n", S->GetSpectranet()->IsPaged(), S->GetSpectranet()->GetPage(1), S->GetSpectranet()->GetPage(2)); }
   else if(!strcmp(argv[i],"histon")) { extern int g_HistOn; g_HistOn=1; }
   else if(!strcmp(argv[i],"hist")) { extern unsigned g_Hist[]; extern int g_HistOn; g_HistOn=0; for(int a=0;a<65536;a++) if(g_Hist[a]>2000) fprintf(stderr,"%04X %u\n",a,g_Hist[a]); }
   else if(!strcmp(argv[i],"peek")) { for(int a=0x22C0;a<0x22F0;a++) fprintf(stderr,"%02X%s",S->peek8(a), (a&15)==15?"\n":" "); }
   else if(!strcmp(argv[i],"del")) { press({CS,K(4,0)}); }
   else if(!strcmp(argv[i],"usr")) { press({key('t')}); press({CS,SS}); press({key('l')}); type("27000\n"); }
   else if(!strncmp(argv[i],"saveflash:",10)) { FILE*o=fopen(argv[i]+10,"wb"); fwrite(S->GetSpectranet()->GetFlash(),1,131072,o); fclose(o); } else type(argv[i]); }
}
