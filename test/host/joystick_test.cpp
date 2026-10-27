#include "zxkeyboard.h"
#include <stdio.h>
int fails=0;
void chk(const char*n,unsigned got,unsigned exp){ if(got!=exp){printf("FALLO %s: %02X != %02X\n",n,got,exp);fails++;} }
int main(){ CZXKeyboard kb;
 kb.SetJoystickMode(JoystickKempston); kb.SetGamePadState(1, JOY_UP|JOY_FIRE2); chk("kempston",kb.ReadKempston(),0x28);
 kb.SetJoystickMode(JoystickFuller); chk("fuller", kb.ReadFuller(), 0xFF & ~0x01 & ~0x80);
 kb.SetJoystickMode(JoystickSinclair1); kb.SetGamePadState(1, JOY_LEFT|JOY_FIRE);
 chk("s1 row EF (6..0)", kb.ReadPort(0xEF)&0x1F, 0x1F & ~0x10 & ~0x01);  // 6=bit4, 0=bit0
 kb.SetGamePadState(1, JOY_FIRE2); chk("s1 fire2 X (row FE bit2)", kb.ReadPort(0xFE)&0x1F, 0x1F & ~0x04);
 kb.SetJoystickMode(JoystickSinclair2); kb.SetGamePadState(1, JOY_UP|JOY_FIRE); chk("s2 row F7", kb.ReadPort(0xF7)&0x1F, 0x1F & ~0x08 & ~0x10);
 kb.SetGamePadState(1, JOY_FIRE2); chk("s2 fire2 Z", kb.ReadPort(0xFE)&0x1F, 0x1F & ~0x02);
 kb.SetJoystickMode(JoystickCursor); kb.SetGamePadState(1, JOY_RIGHT|JOY_DOWN); chk("cursor 8,6", kb.ReadPort(0xEF)&0x1F, 0x1F & ~0x04 & ~0x10);
 kb.SetGamePadState(1, JOY_LEFT); chk("cursor 5", kb.ReadPort(0xF7)&0x1F, 0x1F & ~0x10);
 kb.SetJoystickMode(JoystickQAOP); kb.SetGamePadState(1, JOY_UP|JOY_RIGHT|JOY_FIRE);
 chk("qaop Q", kb.ReadPort(0xFB)&0x1F, 0x1E); chk("qaop P", kb.ReadPort(0xDF)&0x1F, 0x1E); chk("qaop space", kb.ReadPort(0x7F)&0x1F, 0x1E);
 kb.SetGamePadState(1, JOY_DOWN|JOY_LEFT|JOY_FIRE2); chk("qaop A",kb.ReadPort(0xFD)&0x1F,0x1E); chk("qaop O",kb.ReadPort(0xDF)&0x1F,0x1D); chk("qaop M",kb.ReadPort(0x7F)&0x1F,0x1B);
 kb.SetJoystickMode(JoystickNone); chk("none", kb.ReadPort(0x00)&0x1F, 0x1F);
 kb.SetCaptured(TRUE); kb.SetJoystickMode(JoystickKempston); chk("captured", kb.ReadKempston(), 0);
 printf(fails? "%d fallos\n":"todo OK\n",fails); return fails; }
