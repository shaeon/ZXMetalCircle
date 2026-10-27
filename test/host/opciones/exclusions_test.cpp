#include <initializer_list>
#include "exclusions.h"
#include <stdio.h>
int fails=0;
void t(CExclusions&e,const char*d,const char*n,bool dir,bool exp){ bool r=e.IsExcluded(d,n,dir); printf("%s %-12s %-22s %-6s -> %s\n", r==exp?"ok  ":"FALLO", d, n, dir?"carp.":"fich.", r?"excluido":"visible"); if(r!=exp) fails++; }
int main(){ CExclusions e;
 for(const char*l: {"# comentario", "/bootcode.bin", "/kernel*.img   # los kernels", "/firmware/", "\\certs\\", "*.dtb", "/juegos/viejos/", "tmp/", "  LEEME.txt  ", "", "/start?.elf"}) e.AddLine(l);
 t(e,"SD:/","kernel8-32.img",false,true);
 t(e,"SD:/","KERNEL7L.IMG",false,true);
 t(e,"SD:/hdd","kernel.img",false,false);      // ruta desde la raíz: solo en la raíz
 t(e,"SD:/hdd","mar_oceana.img",false,false);
 t(e,"SD:/","firmware",true,true);
 t(e,"SD:/","firmware",false,false);           // termina en /: solo carpeta
 t(e,"SD:/","certs",true,true);                // con \ también es carpeta
 t(e,"SD:/","bcm2711-rpi-4-b.dtb",false,true); // sin / delante: en cualquier carpeta
 t(e,"SD:/otra","x.dtb",false,true);
 t(e,"SD:/juegos","viejos",true,true);
 t(e,"SD:/juegos","nuevos",true,false);
 t(e,"SD:/juegos/viejos","juego.tap",false,false);
 t(e,"SD:/utils","tmp",true,true);
 t(e,"SD:/","leeme.txt",false,true);
 t(e,"SD:/","start4.elf",false,true);
 t(e,"SD:/","start.elf",false,false);          // ? exige un carácter
 t(e,"SD:/","bootcode.bin",false,true);
 printf(fails?"%d fallos\n":"todo OK\n",fails); return fails; }
