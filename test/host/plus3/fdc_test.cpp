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
void cmd(std::vector<u8> b){ for(u8 x:b){ if(!waitmsr(0xC0,0x80)) {printf("  timeout escribiendo comando\n"); return;} F->fifo_w(x); adv(10);} }
std::vector<u8> results(){ std::vector<u8> r; while(waitmsr(0xD0,0xD0,3000000)){ r.push_back(F->fifo_r()); adv(10);} return r; }
std::vector<u8> readexec(unsigned n){ std::vector<u8> d; while(d.size()<n){ if(!waitmsr(0xE0,0xE0,500000)) break; d.push_back(F->fifo_r()); adv(1);} return d; }
void writeexec(const std::vector<u8>&d){ for(u8 x:d){ if(!waitmsr(0xE0,0xA0,500000)){printf("  timeout escribiendo datos\n");break;} F->fifo_w(x); adv(1);} }
int main(){
 machine_config mc; upd765a_device fdc(mc,"fdc",nullptr,4000000,true,false); F=&fdc; fdc.start();
 floppy_image_device fl(0,42,false); CDskImage img; chk("disco +3 en blanco",img.CreateBlank3()); fl.insert(&img);
 fdc.set_floppy(&fl); fl.mon_w(0); adv(500000);
 cmd({0x03,0xAF,0x03}); adv(100);
 cmd({0x07,0x00}); adv(200000); cmd({0x08}); auto r=results(); chk("recalibrar: ST0 seek end", r.size()==2 && (r[0]&0x20) && r[1]==0);
 cmd({0x4A,0x00}); r=results(); chk("READ ID: 7 bytes", r.size()==7); if(r.size()==7) printf("  ST0=%02X ST1=%02X ST2=%02X C=%d H=%d R=%d N=%d\n",r[0],r[1],r[2],r[3],r[4],r[5],r[6]);
 chk("READ ID: sector N=2 en pista 0", r.size()==7 && r[3]==0 && r[6]==2 && (r[0]&0xC0)==0);
 cmd({0x46,0x00,0x00,0x00,0x01,0x02,0x01,0x2A,0xFF}); auto d=readexec(512); r=results();
 chk("READ DATA: 512 bytes E5", d.size()==512 && d[0]==0xE5 && d[511]==0xE5); if(r.size()==7) printf("  resultado ST0=%02X ST1=%02X ST2=%02X R=%d\n",r[0],r[1],r[2],r[5]);
 // seek a pista 5 y escribir sector 3
 cmd({0x0F,0x00,0x05}); adv(300000); cmd({0x08}); r=results(); chk("SEEK a 5", r.size()==2 && r[1]==5);
 std::vector<u8> w(512); for(int i=0;i<512;i++) w[i]=(i*7+3)&255;
 cmd({0x45,0x00,0x05,0x00,0x03,0x02,0x03,0x2A,0xFF}); writeexec(w); r=results(); if(r.size()==7) printf("  write ST0=%02X ST1=%02X ST2=%02X\n",r[0],r[1],r[2]);
 chk("WRITE DATA sin error de escritura", r.size()==7 && (r[1]&0x02)==0);
 cmd({0x46,0x00,0x05,0x00,0x03,0x02,0x03,0x2A,0xFF}); d=readexec(512); r=results(); chk("releer lo escrito", d==w);
 cmd({0x46,0x00,0x05,0x00,0x02,0x02,0x02,0x2A,0xFF}); d=readexec(512); r=results(); chk("sector vecino intacto (E5)", d.size()==512 && d[0]==0xE5 && d[511]==0xE5);
 // guardar, recargar y comprobar
 fl.mon_w(1); static u8 buf[400000]; unsigned n=img.Save(buf,sizeof buf); chk("guardar DSK extendido", n>0);
 CDskImage img2; chk("recargar DSK", img2.Load(buf,n)); fl.insert(&img2); fl.mon_w(0); adv(500000);
 cmd({0x0F,0x00,0x05}); adv(300000); cmd({0x08}); results();
 cmd({0x46,0x00,0x05,0x00,0x03,0x02,0x03,0x2A,0xFF}); d=readexec(512); r=results(); chk("dato persistente tras guardar y recargar", d==w);
 FILE*o=fopen("fdc_out.dsk","wb"); if(o){ fwrite(buf,1,n,o); fclose(o);}
 printf(fails?"%d fallos\n":"todo OK\n",fails); return fails; }
