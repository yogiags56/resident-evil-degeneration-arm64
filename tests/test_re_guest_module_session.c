#include "re_guest_module_session.h"
#include "re_guest_elf32_identity.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct { const uint8_t *bytes; size_t size; size_t offset; int closed; } SOURCE;
static void p16(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void p32(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static void ph(uint8_t*p,uint32_t t,uint32_t o,uint32_t v,uint32_t f,uint32_t m,uint32_t fl,uint32_t a){p32(p,t);p32(p+4,o);p32(p+8,v);p32(p+12,v);p32(p+16,f);p32(p+20,m);p32(p+24,fl);p32(p+28,a);}
static void fixture(uint8_t*b){
 memset(b,0,256);b[0]=0x7f;b[1]='E';b[2]='L';b[3]='F';b[4]=1;b[5]=1;b[6]=1;
 p16(b+16,RE_GUEST_ELF32_ET_DYN);p16(b+18,RE_GUEST_ELF32_EM_ARM);p32(b+20,1);
 p32(b+24,0x1010);p32(b+28,52);p32(b+36,0x05000000u);
 p16(b+40,52);p16(b+42,32);p16(b+44,2);
 ph(b+52,RE_GUEST_ELF32_PT_LOAD,0,0x1000,256,256,5,0x1000);
 ph(b+84,RE_GUEST_ELF32_PT_DYNAMIC,0x80,0x1080,32,32,6,4);
 p32(b+0x80,RE_GUEST_ELF32_DT_STRTAB);p32(b+0x84,0x10e0);
 p32(b+0x88,RE_GUEST_ELF32_DT_STRSZ);p32(b+0x8c,11);
 p32(b+0x90,RE_GUEST_ELF32_DT_SONAME);p32(b+0x94,1);
 p32(b+0x98,RE_GUEST_ELF32_DT_NULL);p32(b+0x9c,0);
 memcpy(b+0xe0,"\0libfoo.so\0",11);
}
static void *src_open(void*u,const char*p){SOURCE*s=u;(void)p;s->offset=0;return s;}
static int64_t src_length(void*u,void*stream){SOURCE*s=u;(void)stream;return (int64_t)s->size;}
static int64_t src_read(void*u,void*stream,void*d,uint64_t n){SOURCE*s=u;size_t left=s->size-s->offset;size_t take=(size_t)n;if(take>left)take=left;if(take>37u)take=37u;memcpy(d,s->bytes+s->offset,take);s->offset+=take;return (int64_t)take;}
static void src_close(void*u,void*stream){SOURCE*s=u;(void)stream;s->closed++;}
int main(void){
 static const char manifest[]="RE-GUEST-MODULES 1\nroot libfoo.so\nmodule libfoo.so payload/libfoo.so 256\n";
 uint8_t bytes[256];SOURCE state;REGUESTMODULESOURCE source={src_open,src_length,src_read,src_close};
 REGUESTMODULESESSION session;size_t failed=99,line=99;
 memset(&session,0,sizeof(session));fixture(bytes);
 state=(SOURCE){bytes,sizeof(bytes),0,0};
 assert(re_guest_module_session_prepare(manifest,sizeof(manifest)-1,&source,&state,&session,&failed,&line)==RE_GUEST_SESSION_OK);
 assert(session.state==RE_GUEST_SESSION_READY);
 assert(session.manifest.module_count==1u&&session.preflight.graph.complete);
 assert(session.materialized.module_count==1u&&session.materialized.modules[0].size==sizeof(bytes));
 assert(state.closed==1);
 re_guest_module_session_release(&session);
 assert(session.state==RE_GUEST_SESSION_EMPTY&&session.materialized.module_count==0u);
 state.closed=0;
 assert(re_guest_module_session_prepare("bad",3,&source,&state,&session,&failed,&line)==RE_GUEST_SESSION_E_MANIFEST);
 assert(session.state==RE_GUEST_SESSION_EMPTY&&state.closed==0);
 puts("Guest module session lifecycle tests: PASS");
 return 0;
}
