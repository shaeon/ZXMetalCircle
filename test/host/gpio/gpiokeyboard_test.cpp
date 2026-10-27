#include "gpiokeyboard.h"
#include <stdio.h>
#include <string.h>
bool g_Contact[32][32]; u32 g_OutLow;
static const unsigned R[13] = {18, 23, 24, 25, 22, 27, 17, 4, 20, 21, 16, 12, 7};
static const unsigned C[11] = {26, 19, 13, 6, 5, 8, 9, 10, 11, 2, 3};
int fails=0;
void chk(const char*n, CGPIOKeyboard&k, int r1,int b1,int r2,int b2){ k.Scan(); const u8*m=k.GetMatrix(); u8 e[8]={0}; if(r1>=0) e[r1]|=b1; if(r2>=0) e[r2]|=b2;
 bool ok=memcmp(m,e,8)==0; printf("%s %s\n", ok?"ok  ":"FALLO", n); if(!ok){ for(int i=0;i<8;i++) printf(" %02X",m[i]); printf("\n"); fails++; } }
void press(int r,int c,bool on){ g_Contact[R[r]][C[c]]=on; }
int main(){
 CGPIOKeyboard k48; k48.Initialize(GPIOKeyboard48);
 press(3,0,1); chk("48: tecla 1 (A11, D0)", k48, 3,0x01,-1,0); press(3,0,0);
 press(10,10,1); chk("48: las lineas del +3 no se leen", k48, -1,0,-1,0); press(10,10,0);
 CGPIOKeyboard k3; k3.Initialize(GPIOKeyboardPlus3);
 press(7,4,1); chk("+3: tecla B normal", k3, 7,0x10,-1,0); press(7,4,0);
 struct { int r,c; const char*n; int r1,b1,r2,b2; } T[] = {
  {8,9,"comillas = SS+P",7,0x02,5,0x01}, {8,10,"; = SS+O",7,0x02,5,0x02}, {9,5,". = SS+M",7,0x06,-1,0}, {9,6,", = SS+N",7,0x0A,-1,0},
  {10,5,"TRUE VIDEO = CS+3",0,0x01,3,0x04}, {10,6,"INV VIDEO = CS+4",0,0x01,3,0x08}, {10,7,"EDIT = CS+1",0,0x01,3,0x01}, {10,8,"CAPS LOCK = CS+2",0,0x01,3,0x02},
  {10,10,"izquierda = CS+5",0,0x01,3,0x10}, {11,5,"derecha = CS+8",0,0x01,4,0x04}, {11,6,"arriba = CS+7",0,0x01,4,0x08}, {11,7,"DELETE = CS+0",0,0x01,4,0x01},
  {11,8,"GRAPH = CS+9",0,0x01,4,0x02}, {11,10,"abajo = CS+6",0,0x01,4,0x10}, {12,7,"BREAK = CS+SPACE",0,0x01,7,0x01}, {12,8,"EXTEND = CS+SS",0,0x01,7,0x02} };
 for(auto&t:T){ press(t.r,t.c,1); chk(t.n,k3,t.r1,t.b1,t.r2,t.b2); press(t.r,t.c,0); }
 printf(fails?"%d fallos\n":"todo OK\n",fails); return fails; }
