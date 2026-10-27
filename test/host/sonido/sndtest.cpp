#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
CZXKeyboard kb; CTape tape; CSpectrum *S;
FILE *wav; unsigned nSamples=0;
void run(int n,bool rec){for(int i=0;i<n;i++){S->RunFrame(); CAudioMixer*a=S->GetAudio(); if(rec){fwrite(a->GetBuffer(),4,a->GetFrameCount(),wav); nSamples+=a->GetFrameCount();} a->ClearBuffer();}}
void press(std::initializer_list<int> keys){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(6,false);u8 z[8]={0};kb.SetGPIOMatrix(z);run(8,false);}
#define K(r,b) (((r)<<3)|(b))
#define SS K(7,1)
int main(int argc,char**argv){ S=new CSpectrum(&kb,&tape);
 static u8 t[65536]; FILE*f=fopen(argv[1],"rb"); int n=fread(t,1,sizeof t,f); fclose(f);
 { const char*sp=getenv("SPEAKER"); if(sp) S->GetAudio()->SetSpeaker(!strcmp(sp,"tv")?CAudioMixer::SpeakerTV:!strcmp(sp,"off")?CAudioMixer::SpeakerOff:CAudioMixer::SpeakerBeeper); if(getenv("NOLP")) S->GetAudio()->SetBeeperFilter(FALSE); }
 S->SetModel(argc>4&&!strcmp(argv[4],"128")?Model128K:Model48K); S->Reset(); run(150,false);
 if(argc>4&&!strcmp(argv[4],"128")){ press({K(6,0)}); run(50,false); } // Tape Loader
 tape.InsertTAP(t,n,"x"); S->SetTapeMode(TapeModeFlash);
 if(!(argc>4&&!strcmp(argv[4],"128"))){ press({K(6,3)}); press({SS,K(5,0)}); press({SS,K(5,0)}); press({K(6,0)}); }
 run(atoi(argv[3]),false);
 wav=fopen(argv[2],"wb"); fseek(wav,44,SEEK_SET); run(50*20,true);
 unsigned bytes=nSamples*4; u8 h[44]; memcpy(h,"RIFF",4); *(u32*)(h+4)=36+bytes; memcpy(h+8,"WAVEfmt ",8); *(u32*)(h+16)=16; *(u16*)(h+20)=1; *(u16*)(h+22)=2; *(u32*)(h+24)=48000; *(u32*)(h+28)=48000*4; *(u16*)(h+32)=4; *(u16*)(h+34)=16; memcpy(h+36,"data",4); *(u32*)(h+40)=bytes;
 fseek(wav,0,SEEK_SET); fwrite(h,1,44,wav); fclose(wav); printf("%u muestras\n",nSamples); }
