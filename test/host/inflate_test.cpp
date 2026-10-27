#include "inflate.h"
#include <stdio.h>
#include <string.h>
#include <vector>
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
int main(){ int fails=0; const char*names[]={"texto","aleatorio","rle"};
 for(auto n:names){ char raw[64]; sprintf(raw,"z_%s.raw",n); auto r=rd(raw);
  const char* lv[]={"0","1","9","f"};
  for(auto l:lv){ char fn[64]; sprintf(fn,"z_%s_%s.bin",n,l); FILE*t=fopen(fn,"rb"); if(!t) continue; fclose(t); auto z=rd(fn);
   std::vector<u8> out(r.size()+10); int k=ZlibInflate(z.data(),z.size(),out.data(),out.size());
   bool ok = k==(int)r.size() && !memcmp(out.data(),r.data(),r.size()); printf("%s nivel %s: %s\n",n,l,ok?"ok":"FALLO"); if(!ok)fails++; } }
 return fails; }
