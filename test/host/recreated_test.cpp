#include "recreated.h"
#include <stdio.h>
#include <string.h>
int fails=0; unsigned now=0; int passed=0;
void chk(const char*n,int got,int exp){ if(got!=exp){printf("FALLO %s: %d != %d\n",n,got,exp);fails++;} else printf("ok %s\n",n);}
u8 hid(char c,bool*sh){ *sh=false;
 if(c>='a'&&c<='z') return 4+c-'a'; if(c>='A'&&c<='Z'){*sh=true;return 4+c-'A';}
 const char*d="1234567890"; const char*ds="!@#$%^&*()"; for(int i=0;i<10;i++){if(d[i]==c)return 0x1E + i; if(ds[i]==c){*sh=true;return 0x1E + i;}}
 struct{char n,s;u8 k;}t[]={{'-','_',0x2D},{'=','+',0x2E},{'[','{',0x2F},{']','}',0x30},{';',':',0x33},{',','<',0x36},{'.','>',0x37},{'/','?',0x38}};
 for(auto&e:t){if(e.n==c)return e.k; if(e.s==c){*sh=true;return e.k;}} return 0;}
void tick(CRecreatedKeyboard&r,unsigned dt){ // avanza el tiempo con comprobaciones periódicas (cada 20 ms)
 while(dt){unsigned s=dt>20000?20000:dt; now+=s; dt-=s; CRecreatedKeyboard::TOutput o; r.CheckTimeout(now,&o); passed+=o.nCount;} }
void send(CRecreatedKeyboard&r,char c,unsigned hold=8000){ bool sh; u8 k=hid(c,&sh); u8 keys[6]={k,0,0,0,0,0}; u8 none[6]={0};
 CRecreatedKeyboard::TOutput o; r.ProcessReport(sh?0x02:0,keys,now,&o); passed+=o.nCount; tick(r,hold);
 r.ProcessReport(0,none,now,&o); passed+=o.nCount; tick(r,20000); }
int main(){ CRecreatedKeyboard r; r.Reset(RecreatedAuto);
 passed=0; send(r,'h',100000); chk("normal: pulsacion y liberacion llegan", passed, 2);
 for(const char*p="olamundo";*p;p++) send(r,*p,100000);
 chk("normal no detectado", r.IsActive(), 0);
 passed=0; send(r,'a',5000); send(r,'b',5000); chk("tras 8 normales ya no retiene ni detecta", r.IsActive(), 0); chk("pasan los 4 informes", passed, 4);
 CRecreatedKeyboard q; q.Reset(RecreatedAuto); passed=0;
 send(q,'a'); send(q,'b'); send(q,'a'); send(q,'b'); send(q,'a'); send(q,'b');
 chk("recreated detectado", q.IsActive(), 1); chk("sin caracteres sueltos", passed, 0);
 send(q,'8'); send(q,'A'); chk("CAPS+R", (q.GetMatrix()[0]&1) && ((q.GetMatrix()[2]>>3)&1), 1);
 send(q,'B'); send(q,'9'); chk("suelto", q.GetMatrix()[0]|q.GetMatrix()[2], 0);
 send(q,'0'); chk("J", (q.GetMatrix()[6]>>3)&1, 1); send(q,'1');
 printf(fails?"%d fallos\n":"todo OK\n",fails); return fails; }
