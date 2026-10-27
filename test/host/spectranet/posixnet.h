// Transporte de red para las pruebas en el PC (sockets POSIX sin bloqueo)
#include "spectranet.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
extern int g_Frames; extern int g_NetDelay;
class CPosixNet : public CSpectranetNet {
public:
  boolean IsAvailable() { return g_Frames >= g_NetDelay; }
  int Open(boolean bTCP) { int fd=socket(AF_INET, bTCP?SOCK_STREAM:SOCK_DGRAM, 0); int one=1; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof one); fprintf(stderr,"[net] open %s -> %d\n", bTCP?"TCP":"UDP", fd); return fd; }
  boolean Bind(int h, u16 p) { sockaddr_in a={}; a.sin_family=AF_INET; a.sin_port=htons(p); int r=bind(h,(sockaddr*)&a,sizeof a); fprintf(stderr,"[net] bind %d port %u -> %d\n",h,p,r); fcntl(h,F_SETFL,O_NONBLOCK); return r==0; }
  int m_Conn[64]={0};
  boolean ConnectStart(int h, const u8 *ip, u16 p) { if(ip[0]==127 && getenv("NOLOOP")) return FALSE; m_Conn[h]=Connect(h,ip,p)?1:-1; return TRUE; }
  int ConnectStatus(int h){ return m_Conn[h]; }
  boolean Connect(int h, const u8 *ip, u16 p) { sockaddr_in a={}; a.sin_family=AF_INET; a.sin_port=htons(p); memcpy(&a.sin_addr,ip,4); int r=connect(h,(sockaddr*)&a,sizeof a); fprintf(stderr,"[net] connect %d %u.%u.%u.%u:%u -> %d\n",h,ip[0],ip[1],ip[2],ip[3],p,r); fcntl(h,F_SETFL,O_NONBLOCK); return r==0; }
  boolean Listen(int h) { fcntl(h,F_SETFL,O_NONBLOCK); return listen(h,1)==0; }
  int Accept(int h) { int n=accept(h,0,0); if(n>=0) fcntl(n,F_SETFL,O_NONBLOCK); return n; }
  int Send(int h, const u8 *d, unsigned n) { int r=send(h,d,n,0); return r<0 ? (errno==EAGAIN?0:-1) : r; }
  int SendTo(int h, const u8 *d, unsigned n, const u8 *ip, u16 p) { sockaddr_in a={}; a.sin_family=AF_INET; a.sin_port=htons(p); memcpy(&a.sin_addr,ip,4); fcntl(h,F_SETFL,O_NONBLOCK); int r=sendto(h,d,n,0,(sockaddr*)&a,sizeof a); fprintf(stderr,"[net] sendto %u.%u.%u.%u:%u %u bytes -> %d\n",ip[0],ip[1],ip[2],ip[3],p,n,r); return r; }
  int Recv(int h, u8 *b, unsigned n) { int r=recv(h,b,n,0); if(r==0) return -1; if(r<0) return errno==EAGAIN||errno==EWOULDBLOCK?0:-1; return r; }
  int RecvFrom(int h, u8 *b, unsigned n, u8 *ip, u16 *p) { sockaddr_in a={}; socklen_t l=sizeof a; fcntl(h,F_SETFL,O_NONBLOCK); int r=recvfrom(h,b,n,0,(sockaddr*)&a,&l); if(r<=0) return 0; memcpy(ip,&a.sin_addr,4); *p=ntohs(a.sin_port); fprintf(stderr,"[net] recvfrom %u bytes de :%u\n",r,*p); return r; }
  void Close(int h) { close(h); }
  boolean GetConfig(u8 *ip, u8 *m, u8 *gw, u8 *dns) { if (g_Frames < g_NetDelay) return FALSE; u8 a[4]={127,0,0,1}, mk[4]={255,0,0,0}; memcpy(ip,a,4); memcpy(m,mk,4); memcpy(gw,a,4); memcpy(dns,a,4); return TRUE; }
};
