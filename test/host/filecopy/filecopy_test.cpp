// Copiar ficheros a trozos (src/filecopy.cpp) con el FatFs de verdad sobre
// una imagen FAT pequeña: varios pasos, copia idéntica, sin temporal al
// terminar ni al cancelar, sustituir, fichero vacío, "nombre (2)" y "no cabe".
// No está en la Action: necesita el FatFs de Circle (se baja al compilar).
//
//   W=../web; C=<circle>/addon/fatfs
//   gcc -c -O1 -I$W/shim/fatfs -I$C -I$W/shim $C/ff.c $C/ffunicode.c $W/diskio_host.c
//   g++ -O1 -std=c++17 -I$W/shim -I$W/shim/fatfs -I../../../src -I.. -o filecopy_test \
//       filecopy_test.cpp ../../../src/filecopy.cpp ff.o ffunicode.o diskio_host.o
//   mkfs.fat -C img.bin 1536 && IMG=img.bin ./filecopy_test
#include "filecopy.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int fails=0; void chk(const char*n,bool ok){printf("%s %s\n",ok?"ok   ":"FALLO",n); if(!ok)fails++;}
static void put(const char*p,unsigned n,int seed){FIL f; f_open(&f,p,FA_WRITE|FA_CREATE_ALWAYS); for(unsigned i=0;i<n;i++){BYTE b=(BYTE)(i*7+seed); UINT w; f_write(&f,&b,1,&w);} f_close(&f);}
static bool same(const char*a,const char*b){FIL x,y; if(f_open(&x,a,FA_READ)||f_open(&y,b,FA_READ))return false; bool ok=f_size(&x)==f_size(&y); BYTE p,q; UINT r1,r2; while(ok){f_read(&x,&p,1,&r1); f_read(&y,&q,1,&r2); if(r1!=r2||p!=q)ok=false; if(r1==0)break;} f_close(&x); f_close(&y); return ok;}
static bool exists(const char*p){FILINFO i; return f_stat(p,&i)==FR_OK;}
int main(){
 FATFS fs; chk("montar", f_mount(&fs,"SD:",1)==FR_OK);
 f_mkdir("SD:/a"); f_mkdir("SD:/b");
 put("SD:/a/juego.tzx", 300000, 1);
 CFileCopy C;
 chk("empezar", C.Start("SD:/a/juego.tzx","SD:/b/juego.tzx","~zxcopia.tmp",FALSE)==0);
 int r; int steps=0; while((r=C.Step(65536))==0) steps++;
 chk("termina en varios pasos", r==1 && steps>=4);
 chk("copia identica", same("SD:/a/juego.tzx","SD:/b/juego.tzx"));
 chk("sin temporal", !exists("SD:/b/~zxcopia.tmp"));
 // cancelar
 chk("empezar 2", C.Start("SD:/a/juego.tzx","SD:/b/otro.tzx","~zxcopia.tmp",FALSE)==0);
 C.Step(65536); chk("a medias hay temporal", exists("SD:/b/~zxcopia.tmp"));
 C.Cancel(); chk("cancelar: sin temporal ni destino", !exists("SD:/b/~zxcopia.tmp") && !exists("SD:/b/otro.tzx"));
 // sustituir
 put("SD:/b/viejo.tzx", 10, 9);
 chk("sustituir", C.Start("SD:/a/juego.tzx","SD:/b/viejo.tzx","~zxcopia.tmp",TRUE)==0);
 while((r=C.Step(1<<20))==0); chk("sustituido", r==1 && same("SD:/a/juego.tzx","SD:/b/viejo.tzx"));
 // vacio
 put("SD:/a/vacio.txt",0,0);
 chk("vacio", C.Start("SD:/a/vacio.txt","SD:/b/vacio.txt","~zxcopia.tmp",FALSE)==0 && C.Step(65536)==1 && exists("SD:/b/vacio.txt"));
 // no existe / carpeta
 chk("no existe", C.Start("SD:/a/nada","SD:/b/nada","~zxcopia.tmp",FALSE)!=0);
 chk("una carpeta no", C.Start("SD:/a","SD:/b/a","~zxcopia.tmp",FALSE)!=0);
 // nombre libre
 char N[128];
 chk("nombre (2)", CFileCopy::FreeName("SD:/b","juego.tzx",N,sizeof N) && strcmp(N,"juego (2).tzx")==0);
 put("SD:/b/juego (2).tzx",1,0);
 chk("nombre (3)", CFileCopy::FreeName("SD:/b/","juego.tzx",N,sizeof N) && strcmp(N,"juego (3).tzx")==0);
 chk("sin extension", CFileCopy::FreeName("SD:/b","carpeta",N,sizeof N) && strcmp(N,"carpeta (2)")==0);
 // no cabe
 put("SD:/a/grande.bin", 900*1024, 3);
 const char*e=C.Start("SD:/a/grande.bin","SD:/b/grande.bin","~zxcopia.tmp",FALSE);
 chk("no cabe", e!=0 && strstr(e,"cabe")!=0);
 printf(fails?"%d FALLOS\n":"todo bien\n",fails); return fails;
}

extern "C" { int ff_mutex_create (int){return 1;} void ff_mutex_delete (int){} int ff_mutex_take (int){return 1;} void ff_mutex_give (int){} }
