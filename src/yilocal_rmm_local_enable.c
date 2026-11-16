/*
 * YiLocal v1.0.0 - local RMM media enable helper
 *
 * Reversed directly from vendor rmm main():
 *   mmap_info + 0x6dc : start_with_reset
 *     == 1 keeps rmm in the factory-reset / onboarding branch.
 *   mmap_info + 0x678 : wifi_connected
 *     == 1 enters the normal "wifi connected goto venc and aenc" path
 *     once start_with_reset is cleared.
 *
 * Both fields live in /tmp/mmap.info (tmpfs/RAM). No MTD or /backup writes.
 */
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long usize;
#define NR_exit 1
#define NR_read 3
#define NR_write 4
#define NR_open 5
#define NR_close 6
#define NR_lseek 19
#define O_RDWR 2
#define SEEK_SET 0
static inline s32 sc1(s32 nr,s32 a0){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;__asm__ volatile("svc 0":"+r"(r0):"r"(r7):"memory");return r0;}
static inline s32 sc3(s32 nr,s32 a0,s32 a1,s32 a2){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;register s32 r2 __asm__("r2")=a2;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r7):"memory");return r0;}
static usize slen(const char*s){usize n=0;while(s[n])n++;return n;}
static void out(const char*s){sc3(NR_write,1,(s32)s,(s32)slen(s));}
static void out_u32(u32 v){char b[11];static const char h[]="0123456789abcdef";int i;out("0x");for(i=7;i>=0;i--)b[7-i]=h[(v>>(i*4))&15];b[8]=0;out(b);}
static int read32(s32 fd,u32 off,u32 *v){if(sc3(NR_lseek,fd,(s32)off,SEEK_SET)<0)return -1;return sc3(NR_read,fd,(s32)v,4)==4?0:-1;}
static int write32(s32 fd,u32 off,u32 v){if(sc3(NR_lseek,fd,(s32)off,SEEK_SET)<0)return -1;return sc3(NR_write,fd,(s32)&v,4)==4?0:-1;}
static void show(const char *name,u32 v){out("YiLocal: ");out(name);out("=");out_u32(v);out("\n");}
void _start(void){
 static const char path[]="/tmp/mmap.info";
 s32 fd; u32 reset=0,wifi=0,wifistat=0,ready=0,v=0;
 out("YiLocal: opening /tmp/mmap.info\n");
 fd=sc3(NR_open,(s32)path,O_RDWR,0);
 if(fd<0){out("YiLocal: ERROR cannot open /tmp/mmap.info read/write\n");sc1(NR_exit,2);}
 if(read32(fd,0x6dc,&reset)||read32(fd,0x678,&wifi)){out("YiLocal: ERROR reading rmm state fields\n");sc1(NR_close,fd);sc1(NR_exit,3);}
 (void)read32(fd,0x680,&wifistat);
 (void)read32(fd,0xb54,&ready);
 show("start_with_reset before",reset);
 show("wifi_connected before",wifi);
 show("wifi_stat",wifistat);
 show("rmm_ready_b54",ready);
 if(reset!=0){if(write32(fd,0x6dc,0)){out("YiLocal: ERROR clearing start_with_reset\n");sc1(NR_close,fd);sc1(NR_exit,4);}}
 if(wifi!=1){if(write32(fd,0x678,1)){out("YiLocal: ERROR setting wifi_connected\n");sc1(NR_close,fd);sc1(NR_exit,5);}}
 if(read32(fd,0x6dc,&v)){sc1(NR_close,fd);sc1(NR_exit,6);} show("start_with_reset after",v); if(v!=0){out("YiLocal: ERROR start_with_reset did not clear\n");sc1(NR_close,fd);sc1(NR_exit,7);}
 if(read32(fd,0x678,&v)){sc1(NR_close,fd);sc1(NR_exit,8);} show("wifi_connected after",v); if(v!=1){out("YiLocal: ERROR wifi_connected did not become 1\n");sc1(NR_close,fd);sc1(NR_exit,9);}
 sc1(NR_close,fd);
 out("YiLocal: factory-reset gate cleared + local Wi-Fi state enabled (RAM only)\n");
 sc1(NR_exit,0);
}
