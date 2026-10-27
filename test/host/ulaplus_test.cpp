#include "snapshot.h"
#include <stdio.h>
#include <vector>
CZXKeyboard kb; CTape tape; static u8 buf[SNAPSHOT_MAX_SIZE];
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
int main(int argc,char**argv){ CSpectrum*s=new CSpectrum(&kb,&tape); if(argc>1) s->SetULAplusEnabled(0);
 auto d=rd("ulaplus.z80"); const char*e=CSnapshot::Load(s,d.data(),d.size(),"ulaplus.z80"); if(e){printf("%s\n",e);return 1;}
 for(int i=0;i<5;i++){s->RunFrame();s->GetAudio()->ClearBuffer();}
 printf("ULAplus activo: %d\n", s->IsULAplusActive());
 unsigned n=CSnapshot::SaveZ80(s,buf,sizeof buf); FILE*o=fopen("u_after.z80","wb");fwrite(buf,1,n,o);fclose(o);
 o=fopen(argc>1?"u_off.ppm":"u_on.ppm","wb");fprintf(o,"P6 %d %d 255\n",SCREEN_WIDTH,SCREEN_HEIGHT);u16*p=s->GetScreenBuffer();for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++){u16 c=p[i];fputc(((c>>11)&31)*255/31,o);fputc(((c>>5)&63)*255/63,o);fputc((c&31)*255/31,o);}fclose(o); }
