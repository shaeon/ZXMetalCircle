#include "interface1.h"
#include <stdio.h>
#include <string.h>
#include <vector>
std::vector<u8> rd(const char*f){FILE*h=fopen(f,"rb");std::vector<u8> v;int c;while((c=fgetc(h))!=EOF)v.push_back(c);fclose(h);return v;}
static u8 buf[8<<20], buf2[8<<20];
int main(){ int fails=0;
 auto mdr=rd("test.mdr"); CMicrodrive a; a.InsertMDR(mdr.data(),mdr.size());
 printf("MDR de Sinclair: IsSinclairFormat=%d\n", a.IsSinclairFormat());
 unsigned n=a.ExportMDV(buf,sizeof buf); printf("MDV: %u bytes\n",n);
 FILE*f=fopen("test.mdv","wb"); fwrite(buf,1,n,f); fclose(f);
 CMicrodrive b; printf("reimportado: %d\n", b.InsertMDV(buf,n));
 unsigned n1=a.ExportMDR(buf,sizeof buf), n2=b.ExportMDR(buf2,sizeof buf2); bool same = n1==n2 && !memcmp(buf,buf2,n1);
 printf("MDR identico tras pasar por MDV: %d\n", same); if(!same) fails++;
 CMicrodrive c; c.InsertBlank(); printf("cartucho en blanco: Sinclair=%d\n", c.IsSinclairFormat());
 // simular un formato no Sinclair: escribir un preámbulo y datos sin cabecera Sinclair
 c.Selected(); for(int i=0;i<10;i++) c.WriteData(0); c.WriteData(0xFF); c.WriteData(0xFF); for(int i=0;i<300;i++) c.WriteData(i&0xFE);
 printf("formato propio: Sinclair=%d\n", c.IsSinclairFormat());
 unsigned m=c.ExportMDV(buf,sizeof buf); CMicrodrive d; printf("MDV propio %u bytes, reimportado %d\n", m, d.InsertMDV(buf,m));
 unsigned m2=d.ExportMDV(buf2,sizeof buf2); bool s2 = m==m2 && !memcmp(buf,buf2,m); printf("MDV estable: %d\n", s2); if(!s2) fails++;
 // fichero MDV de JSpeccy comprimido: generar uno con python+zlib (si existe)
 FILE*z=fopen("jsp.mdv","rb"); if(z){ fclose(z); auto j=rd("jsp.mdv"); CMicrodrive e; bool ok=e.InsertMDV(j.data(),j.size()); unsigned k=e.ExportMDR(buf,sizeof buf); bool eq = k==n1 && !memcmp(buf,(a.ExportMDR(buf2,sizeof buf2),buf2),k); printf("MDV comprimido estilo JSpeccy: carga %d, contenido igual %d\n",ok,eq); if(!ok||!eq) fails++; }
 return fails; }
