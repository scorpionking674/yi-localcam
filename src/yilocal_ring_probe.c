/* YiLocal v1.0.0 ring activity probe - freestanding ARM Linux. */
typedef unsigned char u8;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long usize;
#define NR_exit 1
#define NR_read 3
#define NR_write 4
#define NR_open 5
#define NR_close 6
#define NR_nanosleep 162
#define O_RDONLY 0
struct timespec32{s32 tv_sec;s32 tv_nsec;};
static inline s32 sc1(s32 nr,s32 a0){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;__asm__ volatile("svc 0":"+r"(r0):"r"(r7):"memory");return r0;}
static inline s32 sc2(s32 nr,s32 a0,s32 a1){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r7):"memory");return r0;}
static inline s32 sc3(s32 nr,s32 a0,s32 a1,s32 a2){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;register s32 r2 __asm__("r2")=a2;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r7):"memory");return r0;}
static usize slen(const char*s){usize n=0;while(s[n])n++;return n;}
static void out(const char*s){sc3(NR_write,1,(s32)s,(s32)slen(s));}
static void sleep1(void){struct timespec32 t;t.tv_sec=1;t.tv_nsec=0;sc2(NR_nanosleep,(s32)&t,0);}
static int scan_once(void){static const char p[]="/dev/shm/fshare_frame_buf";u8 b[2048];s32 fd,n;usize i;fd=sc3(NR_open,(s32)p,O_RDONLY,0);if(fd<0)return -1;for(;;){n=sc3(NR_read,fd,(s32)b,sizeof(b));if(n<=0)break;for(i=0;i<(usize)n;i++)if(b[i]){sc1(NR_close,fd);return 1;}}sc1(NR_close,fd);return 0;}
void _start(void){int i,r;for(i=0;i<70;i++){r=scan_once();if(r>0){out("YiLocal: fshare ring contains encoded/nonzero data\n");sc1(NR_exit,0);}if(r<0){out("YiLocal: fshare ring not present yet\n");}else{out("YiLocal: fshare ring still all-zero; waiting\n");}sleep1();}out("YiLocal: ERROR fshare ring remained all-zero for 70 seconds\n");sc1(NR_exit,10);}
