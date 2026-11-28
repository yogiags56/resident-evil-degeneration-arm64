#include "re_guest_elf32_identity.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void p16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void p32(uint8_t *p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static void ph(uint8_t *p,uint32_t type,uint32_t off,uint32_t va,uint32_t fs,uint32_t ms,uint32_t flags,uint32_t align){
 p32(p,type);p32(p+4,off);p32(p+8,va);p32(p+12,va);p32(p+16,fs);p32(p+20,ms);p32(p+24,flags);p32(p+28,align);
}
static void fixture(uint8_t *b){
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
 memcpy(b+0xe0,"\\0libfoo.so\\0",11);
}
int main(void){
 uint8_t bytes[256];REGUESTELF32IDENTITY identity;size_t failed=99;
 fixture(bytes);
 assert(re_guest_elf32_verify_identity(bytes,sizeof(bytes),"libfoo.so",
  RE_GUEST_ELF32_IDENTITY_SHARED,&identity,&failed)==RE_GUEST_ELF32_IDENTITY_OK);
 assert(identity.image.machine==RE_GUEST_ELF32_EM_ARM);
 assert(identity.image.type==RE_GUEST_ELF32_ET_DYN);
 assert(identity.arm_eabi_version==5u);
 assert(identity.dynamic.has_soname && strcmp(identity.dynamic.soname,"libfoo.so")==0);
 assert(re_guest_elf32_verify_identity(bytes,sizeof(bytes),"libbar.so",
  RE_GUEST_ELF32_IDENTITY_SHARED,&identity,&failed)==RE_GUEST_ELF32_IDENTITY_E_SONAME);
 assert(re_guest_elf32_verify_identity(bytes,sizeof(bytes),"libfoo.so",
  RE_GUEST_ELF32_IDENTITY_EXEC,&identity,&failed)==RE_GUEST_ELF32_IDENTITY_E_TYPE);
 fixture(bytes);p32(bytes+36,0x06000000u);
 assert(re_guest_elf32_verify_identity(bytes,sizeof(bytes),"libfoo.so",
  RE_GUEST_ELF32_IDENTITY_SHARED,&identity,&failed)==RE_GUEST_ELF32_IDENTITY_E_ABI);
 puts("Guest ELF32 identity verification tests: PASS");
 return 0;
}
