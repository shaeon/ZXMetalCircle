#include "videolayout.h"
#include <stdio.h>
int main(){ unsigned d[][2]={{1920,1080},{1920,1200},{1600,1200},{1280,1024},{1280,720},{1024,768}};
 struct {const char*n; unsigned w,h;} b[]={{"sin borde",256,192},{"estandar",320,240},{"completo",352,296},{"enorme",384,304}};
 for(auto&p:d){ printf("%ux%u\n",p[0],p[1]);
  for(auto&i:b){ TScanlineLayout L; bool ok=ComputeScanlineLayout(p[0],p[1],i.w,i.h,&L,true);
   double sx=(double)p[0]/i.w, sy=(double)p[1]/i.h, f=sx<sy?sx:sy; unsigned hn=(unsigned)(i.h*f);
   unsigned h= !ok?0 : L.nRepeat? i.h*L.nRepeat*L.nGPUScale : L.nOutHeight;
   printf("  %-9s sin scanlines %4u | con scanlines %4u (%3u%%) %s\n", i.n, hn, h, h*100/hn, ok&&L.nRepeat==0?"fraccionario":""); } } }
