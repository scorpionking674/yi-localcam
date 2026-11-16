/* YiLocal provisioning DHCP server v1.0.0. One-subnet tiny DHCP/BOOTP server.
 * Offers 192.168.10.2/24 with gateway/DNS 192.168.10.1. Freestanding ARM Linux.
 */
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef signed int s32; typedef unsigned long usize;
#define NR_exit 1
#define NR_write 4
#define NR_close 6
#define NR_socket 281
#define NR_bind 282
#define NR_sendto 290
#define NR_recvfrom 292
#define NR_setsockopt 294
#define AF_INET 2
#define SOCK_DGRAM 2
#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define SO_BROADCAST 6
struct sockaddr_in_min{u16 family;u16 port;u32 addr;u8 zero[8];};
static inline s32 sc1(s32 nr,s32 a0){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;__asm__ volatile("svc 0":"+r"(r0):"r"(r7):"memory");return r0;}
static inline s32 sc3(s32 nr,s32 a0,s32 a1,s32 a2){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;register s32 r2 __asm__("r2")=a2;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r7):"memory");return r0;}
static inline s32 sc5(s32 nr,s32 a0,s32 a1,s32 a2,s32 a3,s32 a4){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;register s32 r2 __asm__("r2")=a2;register s32 r3 __asm__("r3")=a3;register s32 r4 __asm__("r4")=a4;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r7):"memory");return r0;}
static inline s32 sc6(s32 nr,s32 a0,s32 a1,s32 a2,s32 a3,s32 a4,s32 a5){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;register s32 r2 __asm__("r2")=a2;register s32 r3 __asm__("r3")=a3;register s32 r4 __asm__("r4")=a4;register s32 r5 __asm__("r5")=a5;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5),"r"(r7):"memory");return r0;}
static usize slen(const char*s){usize n=0;while(s[n])n++;return n;} static void log_s(const char*s){sc3(NR_write,1,(s32)s,(s32)slen(s));}
static void zero(u8*p,u32 n){while(n--)*p++=0;} static void copy(u8*d,const u8*s,u32 n){while(n--)*d++=*s++;}
static int dhcp_type(const u8*b,u32 n){u32 i=240;if(n<244||b[236]!=99||b[237]!=130||b[238]!=83||b[239]!=99)return 0;while(i<n){u8 c=b[i++];if(c==255)break;if(c==0)continue;if(i>=n)break;u8 l=b[i++];if(i+l>n)break;if(c==53&&l==1)return b[i];i+=l;}return 0;}
static u32 option(u8*b,u32 o,u8 code,const u8*v,u8 n){b[o++]=code;b[o++]=n;copy(b+o,v,n);return o+n;}
void _start(void){
  s32 fd,one=1;struct sockaddr_in_min sa,dst;u8 in[600],out[600];u32 alen=16;
  fd=sc3(NR_socket,AF_INET,SOCK_DGRAM,0);if(fd<0){log_s("dhcpd: socket failed\n");sc1(NR_exit,2);}sc5(NR_setsockopt,fd,SOL_SOCKET,SO_REUSEADDR,(s32)&one,4);sc5(NR_setsockopt,fd,SOL_SOCKET,SO_BROADCAST,(s32)&one,4);
  sa.family=AF_INET;sa.port=0x4300;sa.addr=0;for(int i=0;i<8;i++)sa.zero[i]=0;/* UDP 67 */
  if(sc3(NR_bind,fd,(s32)&sa,16)<0){log_s("dhcpd: bind UDP/67 failed\n");sc1(NR_exit,3);}log_s("YiLocal DHCP v1.0.0: 192.168.10.2/24\n");
  dst.family=AF_INET;dst.port=0x4400;dst.addr=0xffffffffu;for(int i=0;i<8;i++)dst.zero[i]=0;
  for(;;){s32 n=sc6(NR_recvfrom,fd,(s32)in,sizeof(in),0,0,(s32)&alen);if(n<240)continue;int t=dhcp_type(in,(u32)n);if(t!=1&&t!=3)continue;zero(out,sizeof(out));out[0]=2;out[1]=in[1];out[2]=in[2];out[3]=in[3];copy(out+4,in+4,4);copy(out+10,in+10,2);out[16]=192;out[17]=168;out[18]=10;out[19]=2;out[20]=192;out[21]=168;out[22]=10;out[23]=1;copy(out+28,in+28,16);out[236]=99;out[237]=130;out[238]=83;out[239]=99;u32 o=240;u8 mt=(t==1)?2:5;u8 sid[4]={192,168,10,1},mask[4]={255,255,255,0},bc[4]={192,168,10,255},lease[4]={0,1,81,128};o=option(out,o,53,&mt,1);o=option(out,o,54,sid,4);o=option(out,o,51,lease,4);o=option(out,o,1,mask,4);o=option(out,o,3,sid,4);o=option(out,o,6,sid,4);o=option(out,o,28,bc,4);out[o++]=255;if(o<300)o=300;sc6(NR_sendto,fd,(s32)out,(s32)o,0,(s32)&dst,16);if(t==1)log_s("dhcpd: OFFER 192.168.10.2\n");else log_s("dhcpd: ACK 192.168.10.2\n");}
}
