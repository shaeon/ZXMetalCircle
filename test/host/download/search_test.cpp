#include "download/httpsclient.h"
#include "download/zxinfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
class CBSD : public ITransport { int fd=-1; public:
 bool Connect(const char*h,unsigned p){ addrinfo hints{},*r; hints.ai_socktype=SOCK_STREAM; char ps[8]; sprintf(ps,"%u",p); if(getaddrinfo(h,ps,&hints,&r)) return false; fd=socket(r->ai_family,SOCK_STREAM,0); bool ok=connect(fd,r->ai_addr,r->ai_addrlen)==0; freeaddrinfo(r); return ok; }
 int Send(const unsigned char*d,size_t n){ return (int)send(fd,d,n,0); }
 int Receive(unsigned char*b,size_t n){ return (int)recv(fd,b,n,0); }
 void Close(){ if(fd>=0) close(fd); fd=-1; } };
int main(int argc,char**argv){
 FILE*f=fopen("/etc/ssl/certs/ca-certificates.crt","rb"); static char pem[1<<20]; size_t n=fread(pem,1,sizeof pem-1,f); fclose(f); pem[n]=0; HTTPSSetRootCerts(pem,n+1);
 CBSD t; CHTTPSClient c(&t); static unsigned char work[4<<20], out[1<<20];
 static TZXInfoResult R[20]; const char*err;
 int nr=ZXInfoSearch(&c, argc>1?argv[1]:"manic miner", R, 10, work, sizeof work, &err);
 printf("resultados: %d %s\n", nr, err);
 for(int i=0;i<nr;i++) printf("  %-8s %-32s %4u %-24s %-18s %s %s\n", R[i].ID, R[i].Title, R[i].nYear, R[i].Publisher, R[i].Machine, R[i].Format, R[i].Path);
 for(int i=0;i<nr;i++) if(R[i].Path[0]){ char name[128]; size_t sz=ZXInfoDownload(&c,R[i].Path,out,sizeof out,name,sizeof name,work,sizeof work,&err);
   printf("descarga de '%s': %zu bytes, fichero '%s' %s\n", R[i].Title, sz, name, err); if(sz){ FILE*o=fopen(name,"wb"); fwrite(out,1,sz,o); fclose(o);} break; }
}
