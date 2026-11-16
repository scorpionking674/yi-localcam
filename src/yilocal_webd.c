/*
 * YiLocal webd v1.0.0
 * Tiny freestanding ARMv7 HTTP server for YiLocal.
 * No libc, no Yi SDK. Static UI is served from SD; dynamic requests are
 * passed to the shell network/system manager through /tmp request files.
 */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long usize;
#define NULL ((void*)0)

#define NR_exit       1
#define NR_fork       2
#define NR_read       3
#define NR_write      4
#define NR_open       5
#define NR_close      6
#define NR_wait4      114
#define NR_nanosleep  162
#define NR_socket     281
#define NR_bind       282
#define NR_listen     284
#define NR_accept     285
#define NR_send       289
#define NR_recv       291
#define NR_setsockopt 294

#define O_RDONLY 0
#define O_WRONLY 1
#define O_CREAT 0100
#define O_TRUNC 01000
#define AF_INET 2
#define SOCK_STREAM 1
#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define MSG_NOSIGNAL 0x4000

struct timespec32 { s32 tv_sec; s32 tv_nsec; };
struct sockaddr_in_min { u16 family; u16 port; u32 addr; u8 zero[8]; };

static inline s32 sc0(s32 nr){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0");__asm__ volatile("svc 0":"=r"(r0):"r"(r7):"memory");return r0;}
static inline s32 sc1(s32 nr,s32 a0){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;__asm__ volatile("svc 0":"+r"(r0):"r"(r7):"memory");return r0;}
static inline s32 sc2(s32 nr,s32 a0,s32 a1){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r7):"memory");return r0;}
static inline s32 sc3(s32 nr,s32 a0,s32 a1,s32 a2){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;register s32 r2 __asm__("r2")=a2;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r7):"memory");return r0;}
static inline s32 sc4(s32 nr,s32 a0,s32 a1,s32 a2,s32 a3){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;register s32 r2 __asm__("r2")=a2;register s32 r3 __asm__("r3")=a3;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r3),"r"(r7):"memory");return r0;}
static inline s32 sc5(s32 nr,s32 a0,s32 a1,s32 a2,s32 a3,s32 a4){register s32 r7 __asm__("r7")=nr;register s32 r0 __asm__("r0")=a0;register s32 r1 __asm__("r1")=a1;register s32 r2 __asm__("r2")=a2;register s32 r3 __asm__("r3")=a3;register s32 r4 __asm__("r4")=a4;__asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r7):"memory");return r0;}

static usize slen(const char*s){usize n=0;while(s[n])n++;return n;}
static int memeq(const char*a,const char*b,usize n){while(n--)if(*a++!=*b++)return 0;return 1;}
static const char* finds(const char*b,usize n,const char*x){usize m=slen(x),i;if(!m||m>n)return NULL;for(i=0;i+m<=n;i++)if(memeq(b+i,x,m))return b+i;return NULL;}
static void sleep_ms(u32 ms){struct timespec32 t;t.tv_sec=ms/1000;t.tv_nsec=(s32)((ms%1000)*1000000u);sc2(NR_nanosleep,(s32)&t,0);}
static int sendall(s32 fd,const void*buf,usize len){const u8*p=(const u8*)buf;while(len){s32 r=sc4(NR_send,fd,(s32)p,(s32)len,MSG_NOSIGNAL);if(r<=0)return -1;p+=r;len-=r;}return 0;}
static int sends(s32 fd,const char*s){return sendall(fd,s,slen(s));}
static void log_s(const char*s){sc3(NR_write,1,(s32)s,(s32)slen(s));}

static int write_file(const char*path,const char*buf,usize n){s32 fd=sc3(NR_open,(s32)path,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)return -1;while(n){s32 r=sc3(NR_write,fd,(s32)buf,(s32)n);if(r<=0){sc1(NR_close,fd);return -1;}buf+=r;n-=r;}sc1(NR_close,fd);return 0;}
static int touch_file(const char*path){return write_file(path,"1\n",2);}

static void http_head(s32 fd,const char*status,const char*ctype){sends(fd,"HTTP/1.0 ");sends(fd,status);sends(fd,"\r\nServer: YiLocal-v1.0.0\r\nConnection: close\r\nCache-Control: no-store\r\nContent-Type: ");sends(fd,ctype);sends(fd,"\r\n\r\n");}
static void reply_text(s32 fd,const char*status,const char*ctype,const char*body){http_head(fd,status,ctype);sends(fd,body);}
static void serve_file(s32 fd,const char*path,const char*ctype,const char*fallback){s32 f=sc3(NR_open,(s32)path,O_RDONLY,0);char b[2048];if(f<0){reply_text(fd,"404 Not Found","text/plain; charset=utf-8",fallback?fallback:"not found\n");return;}http_head(fd,"200 OK",ctype);for(;;){s32 n=sc3(NR_read,f,(s32)b,sizeof(b));if(n<=0)break;if(sendall(fd,b,(usize)n)<0)break;}sc1(NR_close,f);}

static int hexval(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static usize url_decode(const char*s,usize n,char*out,usize cap){usize i=0,o=0;while(i<n&&o+1<cap){char c=s[i++];if(c=='+')c=' ';else if(c=='%'&&i+1<n){int a=hexval(s[i]),b=hexval(s[i+1]);if(a>=0&&b>=0){c=(char)((a<<4)|b);i+=2;}}if(c=='\r'||c=='\n'||c==0)continue;out[o++]=c;}out[o]=0;return o;}
static int safe_wifi(const char*s,usize n,int is_psk){usize i;if(!n)return 0;if(is_psk && (n<8||n>63))return 0;if(!is_psk && n>32)return 0;for(i=0;i<n;i++){unsigned char c=(unsigned char)s[i];if(c<32||c>126||c=='"'||c=='\\')return 0;}return 1;}
static int field(const char*body,usize n,const char*name,char*out,usize cap){usize namelen=slen(name),i=0;while(i<n){usize k=i;while(k<n&&body[k]!='='&&body[k]!='&')k++;if(k<n&&body[k]=='='&&k-i==namelen&&memeq(body+i,name,namelen)){usize v=k+1,e=v;while(e<n&&body[e]!='&')e++;url_decode(body+v,e-v,out,cap);return 1;}while(i<n&&body[i]!='&')i++;if(i<n)i++;}return 0;}

static int request_body(char*req,usize n,char**body,usize*blen){const char*p=finds(req,n,"\r\n\r\n");if(!p)return 0;p+=4;*body=(char*)p;*blen=n-(usize)(p-req);return 1;}
static u32 content_length(const char*req,usize n){const char*p=finds(req,n,"Content-Length:");u32 v=0;if(!p)return 0;p+=15;while((usize)(p-req)<n&&(*p==' '||*p=='\t'))p++;while((usize)(p-req)<n&&*p>='0'&&*p<='9'){v=v*10u+(u32)(*p-'0');p++;if(v>4096)return 4096;}return v;}

static void handle(s32 fd){
    char req[8192];s32 n=sc4(NR_recv,fd,(s32)req,sizeof(req)-1,0);char *body=NULL;usize blen=0;u32 want=0;
    if(n<=0)return;req[n]=0;
    while(!finds(req,(usize)n,"\r\n\r\n") && (usize)n<sizeof(req)-1){
        s32 r=sc4(NR_recv,fd,(s32)(req+n),(s32)(sizeof(req)-1-(usize)n),0);
        if(r<=0)break;n+=r;req[n]=0;
    }
    want=content_length(req,(usize)n);
    if(request_body(req,(usize)n,&body,&blen) && want>blen){
        while(want>blen && (usize)n<sizeof(req)-1){
            s32 r=sc4(NR_recv,fd,(s32)(req+n),(s32)(sizeof(req)-1-(usize)n),0);
            if(r<=0)break;n+=r;req[n]=0;request_body(req,(usize)n,&body,&blen);
        }
    } else request_body(req,(usize)n,&body,&blen);

    if(memeq(req,"GET / ",6)||memeq(req,"GET /index.html ",16))serve_file(fd,"/tmp/sd/yilocal/www/index.html","text/html; charset=utf-8","YiLocal UI missing\n");
    else if(memeq(req,"GET /app.js ",12))serve_file(fd,"/tmp/sd/yilocal/www/app.js","application/javascript; charset=utf-8","// missing\n");
    else if(memeq(req,"GET /style.css ",15))serve_file(fd,"/tmp/sd/yilocal/www/style.css","text/css; charset=utf-8","/* missing */\n");
    else if(memeq(req,"GET /api/status ",16))serve_file(fd,"/tmp/yilocal_status.json","application/json; charset=utf-8","{\"ok\":false,\"message\":\"status not ready\"}\n");
    else if(memeq(req,"GET /api/wifi/scan ",19))serve_file(fd,"/tmp/yilocal_wifi_scan.txt","text/plain; charset=utf-8","\n");
    else if(memeq(req,"GET /api/wifi/result ",21))serve_file(fd,"/tmp/yilocal_wifi_result.json","application/json; charset=utf-8","{\"state\":\"idle\"}\n");
    else if(memeq(req,"GET /api/log ",13))serve_file(fd,"/tmp/sd/yilocal/yilocal.log","text/plain; charset=utf-8","log unavailable\n");
    else if(memeq(req,"GET /api/rtsp-log ",18))serve_file(fd,"/tmp/sd/yilocal/rtspd.log","text/plain; charset=utf-8","RTSP log unavailable\n");
    else if(memeq(req,"POST /api/wifi/scan ",20)){
        touch_file("/tmp/yilocal_wifi_scan.req");reply_text(fd,"202 Accepted","application/json; charset=utf-8","{\"ok\":true,\"message\":\"scan requested\"}\n");
    }
    else if(memeq(req,"POST /api/wifi/connect ",23)){
        char ssid[96],psk[128];int hs=0,hp=0;usize sn,pn;
        if(body){hs=field(body,blen,"ssid",ssid,sizeof(ssid));hp=field(body,blen,"psk",psk,sizeof(psk));}
        sn=hs?slen(ssid):0;pn=hp?slen(psk):0;
        if(!hs||!hp||!safe_wifi(ssid,sn,0)||!safe_wifi(psk,pn,1)){
            reply_text(fd,"400 Bad Request","application/json; charset=utf-8","{\"ok\":false,\"message\":\"SSID must be 1-32 printable chars; WPA2 password must be 8-63 printable chars. Quotes/backslashes are not supported yet.\"}\n");
        } else {
            write_file("/tmp/yilocal_ssid.req",ssid,sn);write_file("/tmp/yilocal_psk.req",psk,pn);touch_file("/tmp/yilocal_wifi_connect.req");
            reply_text(fd,"202 Accepted","application/json; charset=utf-8","{\"ok\":true,\"message\":\"connection requested\"}\n");
        }
    }
    else if(memeq(req,"POST /api/restart-rtsp ",23)){touch_file("/tmp/yilocal_restart_rtsp.req");reply_text(fd,"202 Accepted","application/json; charset=utf-8","{\"ok\":true}\n");}
    else if(memeq(req,"POST /api/reboot ",17)){touch_file("/tmp/yilocal_reboot.req");reply_text(fd,"202 Accepted","application/json; charset=utf-8","{\"ok\":true,\"message\":\"reboot requested\"}\n");}
    else reply_text(fd,"404 Not Found","application/json; charset=utf-8","{\"ok\":false,\"message\":\"not found\"}\n");
}

void _start(void){s32 sfd,one=1;struct sockaddr_in_min sa;sfd=sc3(NR_socket,AF_INET,SOCK_STREAM,0);if(sfd<0){log_s("webd: socket failed\n");sc1(NR_exit,2);}sc5(NR_setsockopt,sfd,SOL_SOCKET,SO_REUSEADDR,(s32)&one,sizeof(one));sa.family=AF_INET;sa.port=0x5000;sa.addr=0;for(int i=0;i<8;i++)sa.zero[i]=0;/* port 80 => network bytes 00 50; LE u16 0x5000 */if(sc3(NR_bind,sfd,(s32)&sa,sizeof(sa))<0){log_s("webd: bind :80 failed\n");sc1(NR_exit,3);}if(sc2(NR_listen,sfd,4)<0){log_s("webd: listen failed\n");sc1(NR_exit,4);}log_s("YiLocal webd v1.0.0 listening on :80\n");for(;;){s32 cfd,pid;while(sc4(NR_wait4,-1,0,1,0)>0){}cfd=sc3(NR_accept,sfd,0,0);if(cfd<0){sleep_ms(20);continue;}pid=sc0(NR_fork);if(pid==0){sc1(NR_close,sfd);handle(cfd);sc1(NR_close,cfd);sc1(NR_exit,0);}sc1(NR_close,cfd);}}
