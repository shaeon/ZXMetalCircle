#include "optcheck.h"
#include <stdio.h>
#include <string.h>
int main(){ char m[60];
 const char *cases[] = {
  "model=48k tape=fast joystick=kempston sounddev=sndhdmi logdev=ttyS1 gpiokbd=1 gpioaudio=1 recreated=0 cursors=both savetrap=1 ulaplus=1 aystereo=abc tapesound=1 issue=3 speed=1 border=full scanlines=0 savefmt=tap snapformat=z80 if1=auto lec=0 net=auto tz=60 dst=1\r\n",
  "model=48k sounddev=sndpwn tz=60",
  "model=48k bordre=full",
  "MODEL=+3E divmmc=esxdos hdd=hdd/mar_oceana.vhd",
  "model=48k\nsounddev=sndpwm" };
 for (auto c: cases){ unsigned n=CheckOptions(c,m,sizeof m,[](const char*t){ printf("    registro: %s\n",t); }); printf("%u problema(s): \"%s\"\n", n, m); } }
