/*
 * YiLocal RTSP v1.0.0
 * Freestanding ARMv7 Linux binary, no libc/vendor SDK.
 *
 * Purpose:
 *   Read the Yi/QiGan fshare encoded-frame ring and expose either the
 *   high or low H.264 stream over RTSP/RTP interleaved TCP.
 *
 * Paths:
 *   rtsp://CAMERA_IP:8554/main
 *   rtsp://CAMERA_IP:8554/sub
 *
 * Deliberately alpha/minimal:
 *   - /main: H.264 video + AAC-LC 16 kHz mono microphone
 *   - /sub: low-resolution H.264 video
 *   - RTSP-over-TCP interleaved transport only
 *   - one client at a time
 *   - no authentication yet
 *   - no filesystem writes
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long usize;

#define NULL ((void*)0)

/* ARM EABI syscall numbers from Linux arch/arm/tools/syscall.tbl */
#define NR_exit        1
#define NR_fork        2
#define NR_read        3
#define NR_write       4
#define NR_open        5
#define NR_close       6
#define NR_nanosleep   162
#define NR_mmap2       192
#define NR_munmap      91
#define NR_wait4       114
#define NR_socket      281
#define NR_bind        282
#define NR_listen      284
#define NR_accept      285
#define NR_send        289
#define NR_recv        291
#define NR_setsockopt  294

#define O_RDONLY 0
#define PROT_READ 1
#define MAP_SHARED 1
#define AF_INET 2
#define SOCK_STREAM 1
#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define MSG_DONTWAIT 0x40
#define MSG_NOSIGNAL 0x4000

#define FSHARE_SIZE 1786224u
#define RING_OFF 368u
#define RING_LEN (FSHARE_SIZE - RING_OFF)
#define FRAME_HDR 28u
#define FRAME_HIGH 0x0400u
#define FRAME_LOW  0x0800u
#define FRAME_AAC  0x0100u
#define FRAME_SPS  0x0002u
#define FRAME_PPS  0x0004u
#define FRAME_IDR  0x0001u

#define MAX_FRAME 262144u
#define RTP_PAYLOAD 1200u

struct timespec32 { s32 tv_sec; s32 tv_nsec; };
struct sockaddr_in_min {
    u16 family;
    u16 port;
    u32 addr;
    u8 zero[8];
};

static inline s32 syscall0(s32 nr) {
    register s32 r7 __asm__("r7") = nr;
    register s32 r0 __asm__("r0");
    __asm__ volatile("svc 0" : "=r"(r0) : "r"(r7) : "memory");
    return r0;
}
static inline s32 syscall1(s32 nr, s32 a0) {
    register s32 r7 __asm__("r7") = nr;
    register s32 r0 __asm__("r0") = a0;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r7) : "memory");
    return r0;
}
static inline s32 syscall2(s32 nr, s32 a0, s32 a1) {
    register s32 r7 __asm__("r7") = nr;
    register s32 r0 __asm__("r0") = a0;
    register s32 r1 __asm__("r1") = a1;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r1), "r"(r7) : "memory");
    return r0;
}
static inline s32 syscall3(s32 nr, s32 a0, s32 a1, s32 a2) {
    register s32 r7 __asm__("r7") = nr;
    register s32 r0 __asm__("r0") = a0;
    register s32 r1 __asm__("r1") = a1;
    register s32 r2 __asm__("r2") = a2;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory");
    return r0;
}
static inline s32 syscall4(s32 nr, s32 a0, s32 a1, s32 a2, s32 a3) {
    register s32 r7 __asm__("r7") = nr;
    register s32 r0 __asm__("r0") = a0;
    register s32 r1 __asm__("r1") = a1;
    register s32 r2 __asm__("r2") = a2;
    register s32 r3 __asm__("r3") = a3;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r7) : "memory");
    return r0;
}
static inline s32 syscall5(s32 nr, s32 a0, s32 a1, s32 a2, s32 a3, s32 a4) {
    register s32 r7 __asm__("r7") = nr;
    register s32 r0 __asm__("r0") = a0;
    register s32 r1 __asm__("r1") = a1;
    register s32 r2 __asm__("r2") = a2;
    register s32 r3 __asm__("r3") = a3;
    register s32 r4 __asm__("r4") = a4;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r7) : "memory");
    return r0;
}
static inline s32 syscall6(s32 nr, s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5) {
    register s32 r7 __asm__("r7") = nr;
    register s32 r0 __asm__("r0") = a0;
    register s32 r1 __asm__("r1") = a1;
    register s32 r2 __asm__("r2") = a2;
    register s32 r3 __asm__("r3") = a3;
    register s32 r4 __asm__("r4") = a4;
    register s32 r5 __asm__("r5") = a5;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r7) : "memory");
    return r0;
}

static usize slen(const char *s) { usize n=0; while (s[n]) n++; return n; }
static void mem_copy(void *vd, const void *vs, usize n) { u8 *d=(u8*)vd; const u8 *s=(const u8*)vs; while(n--) *d++=*s++; }
static int mem_eq(const u8 *a, const u8 *b, usize n) { while(n--) if(*a++ != *b++) return 0; return 1; }
static int contains(const char *buf, usize n, const char *needle) {
    usize m=slen(needle), i;
    if(m==0 || m>n) return 0;
    for(i=0;i+m<=n;i++) if(mem_eq((const u8*)buf+i,(const u8*)needle,m)) return 1;
    return 0;
}
static const char *finds(const char *buf, usize n, const char *needle) {
    usize m=slen(needle), i;
    if(m==0 || m>n) return NULL;
    for(i=0;i+m<=n;i++) if(mem_eq((const u8*)buf+i,(const u8*)needle,m)) return buf+i;
    return NULL;
}
static u16 rd16le(const u8 *p) { return (u16)p[0] | ((u16)p[1]<<8); }
static u32 rd32le(const u8 *p) { return (u32)p[0]|((u32)p[1]<<8)|((u32)p[2]<<16)|((u32)p[3]<<24); }
static void be16(u8 *p,u16 v){p[0]=(u8)(v>>8);p[1]=(u8)v;}
static void be32(u8 *p,u32 v){p[0]=(u8)(v>>24);p[1]=(u8)(v>>16);p[2]=(u8)(v>>8);p[3]=(u8)v;}

static void log_s(const char *s) { syscall3(NR_write,1,(s32)s,(s32)slen(s)); }
static void sleep_ms(u32 ms) {
    struct timespec32 t;
    t.tv_sec = 0;
    t.tv_nsec = (s32)(ms * 1000000u);
    syscall2(NR_nanosleep,(s32)&t,0);
}

static int send_all(s32 fd, const void *buf, usize len) {
    const u8 *p=(const u8*)buf;
    while(len) {
        s32 r=syscall4(NR_send,fd,(s32)p,(s32)len,MSG_NOSIGNAL);
        if(r<=0) return -1;
        p += (usize)r; len -= (usize)r;
    }
    return 0;
}
static int send_s(s32 fd, const char *s) { return send_all(fd,s,slen(s)); }
static usize get_cseq(const char *req, usize n, char *out) {
    const char *p=finds(req,n,"CSeq:"); usize k=0;
    if(!p){out[0]='1';return 1;}
    p += 5;
    while((usize)(p-req)<n && (*p==' '||*p=='\t')) p++;
    while((usize)(p-req)<n && *p>='0'&&*p<='9'&&k<15) out[k++]=*p++;
    if(!k){out[0]='1';return 1;}
    return k;
}
static int rtsp_status(s32 fd,const char *cseq,const char *extra) {
    if(send_s(fd,"RTSP/1.0 200 OK\r\nCSeq: ")<0) return -1;
    if(send_s(fd,cseq)<0) return -1;
    if(send_s(fd,"\r\nServer: YiLocal-v1.0.0\r\n")<0) return -1;
    if(extra && send_s(fd,extra)<0) return -1;
    return send_s(fd,"\r\n");
}
static int rtsp_describe(s32 fd,const char *cseq,int with_audio) {
    static const char sdp_main[]=
        "v=0\r\n"
        "o=- 0 0 IN IP4 0.0.0.0\r\n"
        "s=YiLocal\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "m=video 0 RTP/AVP 96\r\n"
        "c=IN IP4 0.0.0.0\r\n"
        "a=rtpmap:96 H264/90000\r\n"
        "a=fmtp:96 packetization-mode=1\r\n"
        "a=control:trackID=0\r\n"
        "m=audio 0 RTP/AVP 97\r\n"
        "c=IN IP4 0.0.0.0\r\n"
        "a=rtpmap:97 MPEG4-GENERIC/16000/1\r\n"
        "a=fmtp:97 streamtype=5;profile-level-id=1;mode=AAC-hbr;config=1408;SizeLength=13;IndexLength=3;IndexDeltaLength=3\r\n"
        "a=control:trackID=1\r\n";
    static const char sdp_sub[]=
        "v=0\r\n"
        "o=- 0 0 IN IP4 0.0.0.0\r\n"
        "s=YiLocal\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "m=video 0 RTP/AVP 96\r\n"
        "c=IN IP4 0.0.0.0\r\n"
        "a=rtpmap:96 H264/90000\r\n"
        "a=fmtp:96 packetization-mode=1\r\n"
        "a=control:trackID=0\r\n";
    const char *sdp=with_audio?sdp_main:sdp_sub;
    const char *cl=with_audio?"389":"178";
    if(send_s(fd,"RTSP/1.0 200 OK\r\nCSeq: ")<0) return -1;
    if(send_s(fd,cseq)<0) return -1;
    if(send_s(fd,"\r\nServer: YiLocal-v1.0.0\r\nContent-Type: application/sdp\r\nContent-Length: ")<0) return -1;
    if(send_s(fd,cl)<0) return -1;
    if(send_s(fd,"\r\n\r\n")<0) return -1;
    return send_all(fd,sdp,slen(sdp));
}

struct frame_h { u32 len,counter,u1,u2,time; u16 type,stream_counter; u32 u4; };
static u8 *g_map;
static u8 g_frame[MAX_FRAME];
static u8 g_sps[256],g_pps[128];
static u32 g_sps_len,g_pps_len;

static u32 rel_add(u32 rel,u32 add){ u32 x=rel+add; if(x>=RING_LEN) x-=RING_LEN; return x; }
static void ring_copy(u32 rel, void *vd, u32 n) {
    u8 *d=(u8*)vd;
    u32 first=RING_LEN-rel; if(first>n) first=n;
    mem_copy(d,g_map+RING_OFF+rel,first);
    if(first<n) mem_copy(d+first,g_map+RING_OFF,n-first);
}
static int get_end(u32 *end) {
    u32 b1=rd32le(g_map+16), span=rd32le(g_map+4), e=rd32le(g_map+12), b2=rd32le(g_map+16);
    if(b1!=b2 || b1>=RING_LEN || e>=RING_LEN || span>RING_LEN) return -1;
    if(rel_add(b1,span)!=e) return -1;
    *end=e; return 0;
}
static int read_h(u32 rel,struct frame_h *h) {
    u8 b[28]; ring_copy(rel,b,28);
    h->len=rd32le(b); h->counter=rd32le(b+4); h->u1=rd32le(b+8); h->u2=rd32le(b+12); h->time=rd32le(b+16);
    h->type=rd16le(b+20); h->stream_counter=rd16le(b+22); h->u4=rd32le(b+24);
    if(h->len==0 || h->len>MAX_FRAME || h->len>RING_LEN-FRAME_HDR) return -1;
    return 0;
}
static u32 strip_start(const u8 *p,u32 n) {
    if(n>=4 && p[0]==0&&p[1]==0&&p[2]==0&&p[3]==1) return 4;
    if(n>=3 && p[0]==0&&p[1]==0&&p[2]==1) return 3;
    return 0;
}

static int send_interleaved_rtp(s32 fd,const u8 *nal,u32 n,u16 *seq,u32 ts,int marker) {
    u8 pkt[4+12+RTP_PAYLOAD+2];
    u8 *rtp=pkt+4;
    if(n==0) return 0;
    if(n<=RTP_PAYLOAD) {
        pkt[0]='$'; pkt[1]=0;
        be16(pkt+2,(u16)(12+n));
        rtp[0]=0x80; rtp[1]=(u8)(96|(marker?0x80:0));
        be16(rtp+2,*seq); (*seq)++;
        be32(rtp+4,ts); be32(rtp+8,0x4445524bu); /* DERK */
        mem_copy(rtp+12,nal,n);
        return send_all(fd,pkt,4+12+n);
    }
    {
        u8 hdr=nal[0], nri=(u8)(hdr&0xe0), typ=(u8)(hdr&0x1f);
        u32 off=1;
        int first=1;
        while(off<n) {
            u32 chunk=n-off; if(chunk>RTP_PAYLOAD-2) chunk=RTP_PAYLOAD-2;
            int last=(off+chunk>=n);
            pkt[0]='$'; pkt[1]=0;
            be16(pkt+2,(u16)(12+2+chunk));
            rtp[0]=0x80; rtp[1]=(u8)(96|((marker&&last)?0x80:0));
            be16(rtp+2,*seq); (*seq)++;
            be32(rtp+4,ts); be32(rtp+8,0x4445524bu);
            rtp[12]=(u8)(nri|28);
            rtp[13]=(u8)(typ|(first?0x80:0)|(last?0x40:0));
            mem_copy(rtp+14,nal+off,chunk);
            if(send_all(fd,pkt,4+12+2+chunk)<0) return -1;
            off+=chunk; first=0;
        }
    }
    return 0;
}

static int send_interleaved_aac(s32 fd,const u8 *adts,u32 n,u16 *seq,u32 ts) {
    /* RFC 3640 MPEG4-GENERIC, AAC-hbr. Ring AAC records are ADTS framed. */
    u8 pkt[4+12+4+2048];
    u8 *rtp=pkt+4;
    u32 hdrlen,rawlen,auh;
    if(n<7 || adts[0]!=0xff || (adts[1]&0xf0)!=0xf0) return 0;
    hdrlen=(adts[1]&1u)?7u:9u;
    if(n<=hdrlen) return 0;
    rawlen=n-hdrlen;
    if(rawlen>2048u || rawlen>8191u) return 0;
    pkt[0]='$'; pkt[1]=2;
    be16(pkt+2,(u16)(12u+4u+rawlen));
    rtp[0]=0x80; rtp[1]=(u8)(0x80|97); /* marker + PT 97 */
    be16(rtp+2,*seq); (*seq)++;
    be32(rtp+4,ts); be32(rtp+8,0x41554430u); /* AUD0 */
    rtp[12]=0x00; rtp[13]=0x10; /* AU-headers-length = 16 bits */
    auh=rawlen<<3;             /* 13-bit AU-size + 3-bit AU-index=0 */
    be16(rtp+14,(u16)auh);
    mem_copy(rtp+16,adts+hdrlen,rawlen);
    return send_all(fd,pkt,4u+12u+4u+rawlen);
}

static int send_annexb_record(s32 fd,const u8 *p,u32 n,u16 *seq,u32 ts,int marker) {
    u32 sc=strip_start(p,n);
    if(sc){p+=sc;n-=sc;}
    if(!n) return 0;
    /* Current live ring has one Annex-B NAL per record. If a later firmware
       groups NALs, this alpha still sends the record as one NAL; parser upgrade follows. */
    return send_interleaved_rtp(fd,p,n,seq,ts,marker);
}

static void cache_parameter_sets(u16 stream_flag) {
    u32 start=rd32le(g_map+16), used=rd32le(g_map+4), pos=start, traversed=0;
    g_sps_len=0; g_pps_len=0;
    if(start>=RING_LEN || used>RING_LEN) return;
    while(traversed<used) {
        struct frame_h h; u32 pay,plen,skip=0;
        if(read_h(pos,&h)<0) break;
        if(FRAME_HDR+h.len > used-traversed) break;
        if((h.type & stream_flag) && !(h.type&FRAME_AAC) && (h.type&(FRAME_SPS|FRAME_PPS))) {
            pay=rel_add(pos,FRAME_HDR); plen=h.len;
            if((h.type&FRAME_SPS) && plen>=6){pay=rel_add(pay,6);plen-=6;}
            if(plen<MAX_FRAME){ ring_copy(pay,g_frame,plen); skip=strip_start(g_frame,plen); }
            if((h.type&FRAME_SPS) && plen>skip && plen-skip<=sizeof(g_sps)){g_sps_len=plen-skip;mem_copy(g_sps,g_frame+skip,g_sps_len);}
            if((h.type&FRAME_PPS) && plen>skip && plen-skip<=sizeof(g_pps)){g_pps_len=plen-skip;mem_copy(g_pps,g_frame+skip,g_pps_len);}
        }
        pos=rel_add(pos,FRAME_HDR+h.len); traversed+=FRAME_HDR+h.len;
    }
}

static int find_latest_idr(u16 stream_flag,u32 *out_pos,u32 *out_time) {
    u32 start=rd32le(g_map+16), used=rd32le(g_map+4), pos=start, traversed=0;
    int found=0;
    if(start>=RING_LEN || used>RING_LEN) return -1;
    while(traversed<used) {
        struct frame_h h;
        if(read_h(pos,&h)<0) break;
        if(FRAME_HDR+h.len > used-traversed) break;
        if((h.type & stream_flag) && !(h.type & FRAME_AAC) && (h.type & FRAME_IDR)) {
            *out_pos=pos; *out_time=h.time; found=1;
        }
        pos=rel_add(pos,FRAME_HDR+h.len); traversed+=FRAME_HDR+h.len;
    }
    return found?0:-1;
}

static int service_stream(s32 fd,u16 stream_flag,int with_audio) {
    u32 cursor,end,idr_pos=0,idr_time=0; u16 seqv=1,seqa=1; int armed=0; u32 loops=0;
    u32 last_end=0, stagnant=0; int warned_stagnant=0;
    cache_parameter_sets(stream_flag);
    if(get_end(&end)<0) { log_s("YiLocal: ring metadata invalid at PLAY\n"); return -1; }
    last_end=end;

    /* Start from the newest complete IDR already present in the active ring.
       v1.0.0 started at ring end, forcing clients to wait for a future IDR. */
    if(find_latest_idr(stream_flag,&idr_pos,&idr_time)==0) {
        cursor=idr_pos;
        log_s("YiLocal: starting stream from cached latest IDR\n");
    } else {
        cursor=end;
        log_s("YiLocal: no cached IDR found; waiting for live IDR\n");
    }

    /* Give probing clients codec configuration immediately. */
    if(g_sps_len && send_interleaved_rtp(fd,g_sps,g_sps_len,&seqv,idr_time*90u,0)<0) return -1;
    if(g_pps_len && send_interleaved_rtp(fd,g_pps,g_pps_len,&seqv,idr_time*90u,0)<0) return -1;
    for(;;) {
        struct frame_h h; u32 next,pay,plen;
        char ctrl[1024];
        if((loops++ & 31u)==0) {
            s32 rr=syscall4(NR_recv,fd,(s32)ctrl,sizeof(ctrl),MSG_DONTWAIT);
            if(rr==0) return 0;
            if(rr>0) {
                char cs[16]; usize cn=get_cseq(ctrl,(usize)rr,cs); cs[cn]=0;
                if(contains(ctrl,(usize)rr,"TEARDOWN")) { rtsp_status(fd,cs,"Session: 12345678\r\n"); return 0; }
                if(contains(ctrl,(usize)rr,"GET_PARAMETER") || contains(ctrl,(usize)rr,"OPTIONS"))
                    rtsp_status(fd,cs,"Session: 12345678\r\n");
            }
        }
        if(get_end(&end)<0){sleep_ms(5);continue;}
        if(end!=last_end){ last_end=end; stagnant=0; warned_stagnant=0; }
        if(cursor==end){
            stagnant++;
            if(stagnant>=600u && !warned_stagnant){
                log_s("YiLocal: WARNING ring end has not advanced for ~3 seconds\n");
                warned_stagnant=1;
            }
            sleep_ms(5);continue;
        }
        if(read_h(cursor,&h)<0){cursor=end;armed=0;sleep_ms(5);continue;}
        next=rel_add(cursor,FRAME_HDR+h.len);

        if(with_audio && (h.type&FRAME_AAC)) {
            pay=rel_add(cursor,FRAME_HDR); plen=h.len;
            if(plen<=MAX_FRAME) {
                ring_copy(pay,g_frame,plen);
                if(send_interleaved_aac(fd,g_frame,plen,&seqa,h.time*16u)<0) return -1;
            }
        } else if((h.type & stream_flag) && !(h.type&FRAME_AAC)) {
            pay=rel_add(cursor,FRAME_HDR); plen=h.len;
            if((h.type&FRAME_SPS) && plen>=6){pay=rel_add(pay,6);plen-=6;}
            if(plen<=MAX_FRAME) {
                ring_copy(pay,g_frame,plen);
                if(h.type&FRAME_SPS){
                    u32 sk=strip_start(g_frame,plen);
                    if(plen>sk && plen-sk<=sizeof(g_sps)){g_sps_len=plen-sk;mem_copy(g_sps,g_frame+sk,g_sps_len);}
                } else if(h.type&FRAME_PPS){
                    u32 sk=strip_start(g_frame,plen);
                    if(plen>sk && plen-sk<=sizeof(g_pps)){g_pps_len=plen-sk;mem_copy(g_pps,g_frame+sk,g_pps_len);}
                } else if(h.type&FRAME_IDR) {
                    u32 ts=h.time*90u;
                    if(g_sps_len && send_interleaved_rtp(fd,g_sps,g_sps_len,&seqv,ts,0)<0) return -1;
                    if(g_pps_len && send_interleaved_rtp(fd,g_pps,g_pps_len,&seqv,ts,0)<0) return -1;
                    if(send_annexb_record(fd,g_frame,plen,&seqv,ts,1)<0) return -1;
                    armed=1;
                } else if(armed) {
                    if(send_annexb_record(fd,g_frame,plen,&seqv,h.time*90u,1)<0) return -1;
                }
            }
        }
        cursor=next;
    }
}

static void handle_client(s32 fd) {
    char req[2048]; int stream_flag=FRAME_HIGH; int with_audio=1;
    for(;;) {
        s32 n=syscall4(NR_recv,fd,(s32)req,sizeof(req)-1,0);
        char cs[16]; usize cn;
        if(n<=0) return;
        req[n]=0; cn=get_cseq(req,(usize)n,cs); cs[cn]=0;
        if(contains(req,(usize)n,"DESCRIBE")) {
            if(contains(req,(usize)n,"/sub")){stream_flag=FRAME_LOW;with_audio=0;log_s("YiLocal: DESCRIBE /sub\n");}
            else {stream_flag=FRAME_HIGH;with_audio=1;log_s("YiLocal: DESCRIBE /main\n");}
            if(rtsp_describe(fd,cs,with_audio)<0) return;
        } else if(contains(req,(usize)n,"OPTIONS")) {
            if(rtsp_status(fd,cs,"Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN, GET_PARAMETER\r\n")<0) return;
        } else if(contains(req,(usize)n,"SETUP")) {
            if(!contains(req,(usize)n,"RTP/AVP/TCP")) {
                log_s("YiLocal: SETUP rejected (TCP interleaved required)\n");
                send_s(fd,"RTSP/1.0 461 Unsupported Transport\r\nCSeq: "); send_s(fd,cs); send_s(fd,"\r\n\r\n");
            } else if(contains(req,(usize)n,"trackID=1")) {
                log_s("YiLocal: SETUP audio TCP interleaved 2-3\n");
                if(rtsp_status(fd,cs,"Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: 12345678\r\n")<0) return;
            } else {
                log_s("YiLocal: SETUP video TCP interleaved 0-1\n");
                if(rtsp_status(fd,cs,"Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\nSession: 12345678\r\n")<0) return;
            }
        } else if(contains(req,(usize)n,"PLAY")) {
            log_s("YiLocal: PLAY accepted\n");
            if(rtsp_status(fd,cs,"Session: 12345678\r\nRange: npt=0.000-\r\n")<0) return;
            service_stream(fd,(u16)stream_flag,with_audio);
            return;
        } else if(contains(req,(usize)n,"TEARDOWN")) {
            rtsp_status(fd,cs,"Session: 12345678\r\n"); return;
        } else {
            if(rtsp_status(fd,cs,NULL)<0) return;
        }
    }
}

void _start(void) {
    s32 ffd,sfd,one=1; struct sockaddr_in_min sa;
    ffd=syscall3(NR_open,(s32)"/dev/shm/fshare_frame_buf",O_RDONLY,0);
    if(ffd<0){log_s("YiLocal: cannot open fshare_frame_buf\n");syscall1(NR_exit,2);}
    g_map=(u8*)syscall6(NR_mmap2,0,FSHARE_SIZE,PROT_READ,MAP_SHARED,ffd,0);
    if((u32)g_map>=0xfffff001u){log_s("YiLocal: mmap failed\n");syscall1(NR_exit,3);}
    sfd=syscall3(NR_socket,AF_INET,SOCK_STREAM,0);
    if(sfd<0){log_s("YiLocal: socket failed\n");syscall1(NR_exit,4);}
    syscall5(NR_setsockopt,sfd,SOL_SOCKET,SO_REUSEADDR,(s32)&one,sizeof(one));
    sa.family=AF_INET; sa.port=0x6a21; sa.addr=0; /* htons(8554) on LE */
    for(int i=0;i<8;i++) sa.zero[i]=0;
    if(syscall3(NR_bind,sfd,(s32)&sa,sizeof(sa))<0){log_s("YiLocal: bind 8554 failed\n");syscall1(NR_exit,5);}
    if(syscall2(NR_listen,sfd,2)<0){log_s("YiLocal: listen failed\n");syscall1(NR_exit,6);}
    log_s("YiLocal RTSP v1.0.0 listening on :8554 (/main, /sub)\n");
    for(;;) {
        s32 cfd,pid;
        /* Reap any exited client workers without blocking. */
        while(syscall4(NR_wait4,-1,0,1,0)>0) { }
        cfd=syscall3(NR_accept,sfd,0,0);
        if(cfd<0){sleep_ms(50);continue;}
        pid=syscall0(NR_fork);
        if(pid==0) {
            syscall1(NR_close,sfd);
            log_s("YiLocal: RTSP client worker started\n");
            handle_client(cfd);
            syscall1(NR_close,cfd);
            log_s("YiLocal: RTSP client worker exiting\n");
            syscall1(NR_exit,0);
        }
        syscall1(NR_close,cfd);
        if(pid<0) log_s("YiLocal: fork failed\n");
    }
}
