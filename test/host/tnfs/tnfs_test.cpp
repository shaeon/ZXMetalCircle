#include "tnfs.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
class CUDP : public CTNFSTransport { int s=-1; sockaddr_in a{}; public:
 boolean Open(const char*h,u16 p){ hostent*he=gethostbyname(h); if(!he) return false; s=socket(AF_INET,SOCK_DGRAM,0); a.sin_family=AF_INET; a.sin_port=htons(p); memcpy(&a.sin_addr,he->h_addr,4); return s>=0; }
 boolean Send(const u8*d,unsigned n){ return sendto(s,d,n,0,(sockaddr*)&a,sizeof a)==(int)n; }
 unsigned Receive(u8*b,unsigned n,unsigned ms){ pollfd p{s,POLLIN,0}; if(poll(&p,1,ms)<=0) return 0; int r=recv(s,b,n,0); return r>0?r:0; }
 void Close(){ if(s>=0) close(s); s=-1; } };
struct L : CTNFSListener { std::vector<std::pair<std::string,bool>> v; void OnEntry(const char*n, boolean d){ v.push_back({n,(bool)d}); } };
int fails=0; void chk(const char*n,bool ok){printf("%s %s\n",ok?"ok  ":"FALLO",n); if(!ok) fails++;}
std::vector<std::string> list(CTNFSClient&c,const char*p){ std::vector<std::string> v; if(!c.OpenDir(p)) return v; char n[256]; while(c.ReadDir(n,sizeof n)) v.push_back(n); c.CloseDir(); return v; }
bool has(const std::vector<std::string>&v,const char*n){ for(auto&x:v) if(x==n) return true; return false; }
int main(){ CUDP t; CTNFSClient c(&t);
 chk("MOUNT en localhost", c.Mount("127.0.0.1"));
 auto r=list(c,"/"); printf("  raiz:"); for(auto&x:r) printf(" [%s]",x.c_str()); printf("\n");
 chk("la raiz lista juegos, utilidades y LEEME.txt", has(r,"juegos")&&has(r,"utilidades")&&has(r,"LEEME.txt"));
 boolean d; u32 sz; chk("STAT de una carpeta", c.Stat("/juegos",&d,&sz) && d);
 chk("STAT de un fichero con su tamano", c.Stat("/juegos/speedtest.tap",&d,&sz) && !d && sz==937);
 auto j=list(c,"/juegos"); chk("subcarpeta con fichero y carpeta", has(j,"speedtest.tap")&&has(j,"clasicos"));
 static u8 buf[1<<20]; unsigned n=c.ReadFile("/juegos/clasicos/robocop.dsk",buf,sizeof buf);
 FILE*f=fopen("/tmp/tnfsroot/juegos/clasicos/robocop.dsk","rb"); static u8 orig[1<<20]; unsigned m=fread(orig,1,sizeof orig,f); fclose(f);
 chk("descarga de 128 KB identica (252 bloques)", n==m && memcmp(buf,orig,n)==0);
 n=c.ReadFile("/utilidades/InDev Tester.tap",buf,sizeof buf); chk("nombre con espacios", n==11619);
 chk("OPENDIR de un fichero falla", !c.OpenDir("/LEEME.txt"));
 chk("fichero inexistente", c.ReadFile("/nada.tap",buf,sizeof buf)==0);
 for(int old=0;old<2;old++){ L l; bool ok=c.ListDir("/juegos",&l,old); printf("  ListDir(%s):", old?"clasico":"READDIRX"); for(auto&e:l.v) printf(" %s%s",e.first.c_str(),e.second?"/":""); printf("\n");
   bool dirok=false,fileok=false; for(auto&e:l.v){ if(e.first=="clasicos"&&e.second) dirok=true; if(e.first=="speedtest.tap"&&!e.second) fileok=true; }
   chk(old?"ListDir clasico distingue carpetas":"ListDir READDIRX distingue carpetas", ok&&dirok&&fileok&&l.v.size()==2); }
 c.Unmount(); chk("UMOUNT", !c.IsMounted());
 printf(fails?"%d fallos\n":"todo OK\n",fails); return fails; }
