#include "emu.h"
#include "upd765.h"
#include "imagedev/floppy.h"
#include "dskimage.h"
#include <stdio.h>
#include <vector>
upd765a_device *F; int fails=0;
void chk(const char*n,bool ok){printf("%s %s\n",ok?"ok  ":"FALLO",n); if(!ok)fails++;}
void adv(int us){ mame_scheduler::advance_to(mame_scheduler::now()+attotime::from_usec(us)); }
bool waitmsr(u8 mask,u8 val,int maxus=2000000){ for(int t=0;t<maxus;t+=5){ if((F->msr_r()&mask)==val) return true; adv(5);} return false; }
void cmd(std::vector<u8> b){ for(u8 x:b){ if(!waitmsr(0xC0,0x80)) return; F->fifo_w(x); adv(10);} }
std::vector<u8> results(){ std::vector<u8> r; while(waitmsr(0xD0,0xD0,3000000)){ r.push_back(F->fifo_r()); adv(10);} return r; }
std::vector<u8> readexec(unsigned n){ std::vector<u8> d; while(d.size()<n){ if(!waitmsr(0xE0,0xE0,500000)) break; d.push_back(F->fifo_r()); adv(1);} return d; }
int main(){
 machine_config mc; upd765a_device fdc(mc,"fdc",nullptr,4000000,true,false); F=&fdc; fdc.start();
 static u8 buf[100000]; FILE*f=fopen("plus3/prot.dsk","rb"); int n=fread(buf,1,sizeof buf,f); fclose(f);
 floppy_image_device fl(0,42,false); CDskImage img; chk("cargar DSK de prueba", img.Load(buf,n)); fl.insert(&img);
 fdc.set_floppy(&fl); fl.mon_w(0); adv(500000);
 cmd({0x03,0xAF,0x03}); adv(100); cmd({0x07,0x00}); adv(200000); cmd({0x08}); results();
 cmd({0x46,0x00,0x00,0x00,0x01,0x02,0x01,0x2A,0xFF}); auto d=readexec(512); auto r=results();
 chk("R1 normal se lee (sin errores de CRC)", d.size()==512 && d[0]==0x11 && r.size()==7 && (r[1]&0x25)==0 && (r[2]&0x20)==0);
 cmd({0x46,0x00,0x00,0x00,0x02,0x02,0x02,0x2A,0xFF}); d=readexec(512); r=results();
 if(r.size()==7) printf("  R2: %zu bytes, ST0=%02X ST1=%02X ST2=%02X\n",d.size(),r[0],r[1],r[2]);
 chk("R2 (CRC mal en la cabecera): no se lee, ST1 con DE y ST2 sin DD", d.size()==0 && r.size()==7 && (r[0]&0xC0)==0x40 && (r[1]&0x20) && (r[2]&0x20)==0);
 std::vector<std::vector<u8>> reads;
 for(int k=0;k<3;k++){ cmd({0x46,0x00,0x00,0x00,0x03,0x02,0x03,0x2A,0xFF}); d=readexec(512); r=results(); reads.push_back(d);
   if(k==0 && r.size()==7) printf("  R3: ST0=%02X ST1=%02X ST2=%02X\n",r[0],r[1],r[2]); adv(50000); }
 bool fixed=true, differ=false;
 for(auto&x:reads){ if(x.size()!=512){fixed=false;continue;} for(int i=0;i<512;i++) if((i<100||i>=120) && x[i]!=0x33) fixed=false; }
 for(int i=100;i<120;i++) if(reads[0].size()==512 && reads[1].size()==512 && reads[0][i]!=reads[1][i]) differ=true;
 chk("R3 débil: fuera de 100..119 siempre 0x33", fixed);
 chk("R3 débil: dentro de 100..119 cambia entre lecturas", differ);
 printf(fails?"%d fallos\n":"todo OK\n",fails); return fails; }
