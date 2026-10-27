#include "spectrum.h"
#include <stdio.h>
int fails=0;
void chk(const char*n,unsigned got,unsigned exp){ if(got!=exp){printf("FALLO %s: %02X != %02X\n",n,got,exp);fails++;} else printf("ok %s = %02X\n",n,got);}
CZXKeyboard kb; CTape tape;
int main(){ CSpectrum*s=new CSpectrum(&kb,&tape);
 kb.SetJoystickMode(JoystickSinclair1); kb.SetMousePresent(TRUE);
 kb.MouseUpdate(0,10,5,0); chk("X",s->inPort(0xFBDF),10); chk("Y (sube al bajar)",s->inPort(0xFFDF),(u8)-5);
 chk("botones sueltos",s->inPort(0xFADF),0xFF);
 kb.MouseUpdate(1,-20,-10,1); chk("X tras -20",s->inPort(0xFBDF),(u8)(10-20)); chk("Y",s->inPort(0xFFDF),5);
 chk("izquierdo + rueda",s->inPort(0xFADF),0xED);
 kb.MouseUpdate(2,0,0,0); chk("derecho",s->inPort(0xFADF),0xEE);
 kb.SetJoystickMode(JoystickKempston); kb.SetGamePadState(0,JOY_FIRE); chk("kempston joy en 0x1F",s->inPort(0x001F),0x10); chk("kempston joy en 0xDF",s->inPort(0xFADF),0x10);
 kb.SetJoystickMode(JoystickNone); kb.SetMousePresent(FALSE); chk("sin raton: no responde (bus flotante)",s->inPort(0xFBDF),0xFF);
 printf(fails?"%d fallos\n":"todo OK\n",fails); return fails; }
