#include "settings.h"
#include <stdio.h>
#include <string.h>
int main(){ CSettings s; char out[600];
 const char *old="model=48k tape=fast joystick=kempston sounddev=sndpwn logdev=ttyS1 gpiokbd=1 hdd=hdd/mar.vhd tz=60 model=128k\r\n";
 s.SetValue(s.Find("model"),"+3"); s.SetValue(s.Find("gpiokbd"),"1"); s.SetValue(s.Find("sounddev"),"sndpwn");
 printf("modelo: %s (%s)  gpiokbd: %s\n", s.GetValue(s.Find("model")), s.GetValueName(s.Find("model")), s.GetValue(s.Find("gpiokbd")));
 s.Step(s.Find("sounddev"),1); printf("sounddev tras paso: %s\n", s.GetValue(s.Find("sounddev")));
 s.Step(s.Find("border"),1); s.Step(s.Find("spectranet"),1); s.Step(s.Find("tz"),-1);
 s.Merge(old,out,sizeof out); printf("nueva: %s", out);
 int i=s.Find("joystick"); for(int k=0;k<8;k++){ s.Step(i,-1); printf("%s ", s.GetValueName(i)); } printf("\n"); }
