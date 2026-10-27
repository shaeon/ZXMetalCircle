#include <initializer_list>
#include "spectrum.h"
#include <stdio.h>
#include <string.h>
CZXKeyboard kb; CTape tape;
void run(CSpectrum&s,int n){for(int i=0;i<n;i++){s.RunFrame();s.GetAudio()->ClearBuffer();}}
// press via GPIO matrix: row,bit list
void gp(CSpectrum&s,std::initializer_list<int> keys){u8 m[8]={0};for(int k:keys)m[k>>3]|=1<<(k&7);kb.SetGPIOMatrix(m);run(s,8);u8 z[8]={0};kb.SetGPIOMatrix(z);run(s,8);}
int main(){CSpectrum*s=new CSpectrum(&kb,&tape);run(*s,150);
 // P (row5 bit0), R(row2 bit3), I(row5 bit2), N(row7 bit3), T(row2 bit4), SPACE? PRINT is P key in K mode; then SS+P (") , 'O','K', SS+P, ENTER
 gp(*s,{5<<3|0}); gp(*s,{7<<3|1,5<<3|0}); gp(*s,{5<<3|1}); gp(*s,{6<<3|2}); gp(*s,{7<<3|1,5<<3|0}); gp(*s,{6<<3|0}); run(*s,20);
 FILE*o=fopen("g.ppm","wb");fprintf(o,"P5 %d %d 15\n",SCREEN_WIDTH,SCREEN_HEIGHT);fwrite(s->GetScreenBuffer(),1,SCREEN_WIDTH*SCREEN_HEIGHT,o);fclose(o);}
