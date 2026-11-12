#include "re_guest_elf32_dynamic.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void p16(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void p32(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static void ph(uint8_t*p,uint32_t t,uint32_t o,uint32_t a,uint32_t fs,uint32_t ms,uint32_t f,uint32_t al){
 p32(p,t);p32(p+4,o);p32(p+8,a);p32(p+12,a);p32(p+16,fs);p32(p+20,ms);p32(p+24,f);p32(p+28,al);
}
static void dyn(uint8_t*p,uint32_t tag,uint32_t value){p32(p,tag);p32(p+4,value);}
static void fixture(uint8_t*b){
 memset(b,0,256);b[0]=0x7f;b[1]='E';b[2]='L';b[3]='F';b[4]=1;b[5]=1;b[6]=1;
 p16(b+16,RE_GUEST_ELF32_ET_EXEC);p16(b+18,RE_GUEST_ELF32_EM_ARM);p32(b+20,1);
 p32(b+24,0x1000);p32(b+28,52);p16(b+40,52);p16(b+42,32);p16(b+44,2);
 ph(b+52,RE_GUEST_ELF32_PT_LOAD,0,0x1000,256,256,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_X,0x1000);
 ph(b+84,RE_GUEST_ELF32_PT_DYNAMIC,160,0x10a0,48,48,RE_GUEST_ELF32_PF_R,4);
 dyn(b+160,RE_GUEST_ELF32_DT_STRTAB,0x10d0);
 dyn(b+168,RE_GUEST_ELF32_DT_STRSZ,32);
 dyn(b+176,RE_GUEST_ELF32_DT_NEEDED,1);
 dyn(b+184,RE_GUEST_ELF32_DT_NEEDED,9);
 dyn(b+192,RE_GUEST_ELF32_DT_SONAME,17);
 dyn(b+200,RE_GUEST_ELF32_DT_NULL,0);
 memcpy(b+208,"\0libc.so\0libm.so\0libroot.so\0",28);
}

static void init_fixture(uint8_t*b){
 memset(b,0,512);b[0]=0x7f;b[1]='E';b[2]='L';b[3]='F';b[4]=1;b[5]=1;b[6]=1;
 p16(b+16,RE_GUEST_ELF32_ET_EXEC);p16(b+18,RE_GUEST_ELF32_EM_ARM);p32(b+20,1);
 p32(b+24,0x1000);p32(b+28,52);p16(b+40,52);p16(b+42,32);p16(b+44,2);
 ph(b+52,RE_GUEST_ELF32_PT_LOAD,0,0x1000,512,512,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_X,0x1000);
 ph(b+84,RE_GUEST_ELF32_PT_DYNAMIC,160,0x10a0,64,64,RE_GUEST_ELF32_PF_R,4);
 dyn(b+160,RE_GUEST_ELF32_DT_INIT,0x1010);
 dyn(b+168,RE_GUEST_ELF32_DT_FINI,0x1014);
 dyn(b+176,RE_GUEST_ELF32_DT_INIT_ARRAY,0x1190);
 dyn(b+184,RE_GUEST_ELF32_DT_INIT_ARRAYSZ,4);
 dyn(b+192,RE_GUEST_ELF32_DT_FINI_ARRAY,0x1194);
 dyn(b+200,RE_GUEST_ELF32_DT_FINI_ARRAYSZ,4);
 dyn(b+208,RE_GUEST_ELF32_DT_NULL,0);
 dyn(b+216,RE_GUEST_ELF32_DT_NULL,0);
 p32(b+400,0x1018);p32(b+404,0x101c);
}
int main(void){
 uint8_t b[256];REGUESTELF32IMAGE image;REGUESTELF32DYNAMIC d;size_t failed=999;
 fixture(b);assert(re_guest_elf32_parse_image(b,sizeof b,&image,&failed)==RE_GUEST_ELF32_OK);
 assert(re_guest_elf32_parse_dynamic(b,sizeof b,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_OK);
 assert(d.present&&d.has_string_table&&d.has_soname&&d.needed_count==2);
 assert(strcmp(d.needed[0],"libc.so")==0&&strcmp(d.needed[1],"libm.so")==0);
 assert(strcmp(d.soname,"libroot.so")==0&&d.entry_count==6);

 fixture(b);b[200]=1;assert(re_guest_elf32_parse_image(b,sizeof b,&image,&failed)==0);
 assert(re_guest_elf32_parse_dynamic(b,sizeof b,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_TERMINATOR);
 fixture(b);p32(b+176+4,31);assert(re_guest_elf32_parse_image(b,sizeof b,&image,&failed)==0);
 assert(re_guest_elf32_parse_dynamic(b,sizeof b,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_STRING);
 fixture(b);p32(b+160+4,0x2000);assert(re_guest_elf32_parse_image(b,sizeof b,&image,&failed)==0);
 assert(re_guest_elf32_parse_dynamic(b,sizeof b,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_STRING_TABLE);
 fixture(b);p16(b+44,1);assert(re_guest_elf32_parse_image(b,sizeof b,&image,&failed)==0);
 assert(re_guest_elf32_parse_dynamic(b,sizeof b,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_OK&&!d.present);

 /* PT_DYNAMIC file offset and virtual address must identify the same bytes. */
 fixture(b);p32(b+84+8,0x10a4);
 assert(re_guest_elf32_parse_image(b,sizeof b,&image,&failed)==RE_GUEST_ELF32_OK);
 assert(re_guest_elf32_parse_dynamic(b,sizeof b,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_RANGE);
 {
  uint8_t init_bytes[512];init_fixture(init_bytes);
  assert(re_guest_elf32_parse_image(init_bytes,sizeof init_bytes,&image,&failed)==RE_GUEST_ELF32_OK);
  assert(re_guest_elf32_parse_dynamic(init_bytes,sizeof init_bytes,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_OK);
  assert(d.has_init&&d.init_address==0x1010&&d.has_fini&&d.fini_address==0x1014);
  assert(d.has_init_array&&d.has_init_array_size&&d.init_array_address==0x1190&&d.init_array_size==4);
  assert(d.has_fini_array&&d.has_fini_array_size&&d.fini_array_address==0x1194&&d.fini_array_size==4);
  init_fixture(init_bytes);dyn(init_bytes+184,RE_GUEST_ELF32_DT_INIT_ARRAYSZ,6);
  assert(re_guest_elf32_parse_image(init_bytes,sizeof init_bytes,&image,&failed)==RE_GUEST_ELF32_OK);
  assert(re_guest_elf32_parse_dynamic(init_bytes,sizeof init_bytes,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_SIZE);
  init_fixture(init_bytes);dyn(init_bytes+208,RE_GUEST_ELF32_DT_INIT_ARRAY,0x1198);
  assert(re_guest_elf32_parse_image(init_bytes,sizeof init_bytes,&image,&failed)==RE_GUEST_ELF32_OK);
  assert(re_guest_elf32_parse_dynamic(init_bytes,sizeof init_bytes,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG);

  /* Init/fini pointers must land in executable file-backed PT_LOAD bytes. */
  init_fixture(init_bytes);dyn(init_bytes+160,RE_GUEST_ELF32_DT_INIT,0x1190);
  p32(init_bytes+52+24,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_W);
  assert(re_guest_elf32_parse_image(init_bytes,sizeof init_bytes,&image,&failed)==RE_GUEST_ELF32_OK);
  assert(re_guest_elf32_parse_dynamic(init_bytes,sizeof init_bytes,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_RANGE);
  init_fixture(init_bytes);dyn(init_bytes+168,RE_GUEST_ELF32_DT_FINI,0x1194);
  p32(init_bytes+52+24,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_W);
  assert(re_guest_elf32_parse_image(init_bytes,sizeof init_bytes,&image,&failed)==RE_GUEST_ELF32_OK);
  assert(re_guest_elf32_parse_dynamic(init_bytes,sizeof init_bytes,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_RANGE);

  /* Non-empty ARM32 init/fini pointer arrays require 4-byte alignment. */
  init_fixture(init_bytes);dyn(init_bytes+176,RE_GUEST_ELF32_DT_INIT_ARRAY,0x1191);
  assert(re_guest_elf32_parse_image(init_bytes,sizeof init_bytes,&image,&failed)==RE_GUEST_ELF32_OK);
  assert(re_guest_elf32_parse_dynamic(init_bytes,sizeof init_bytes,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_ALIGNMENT);
  init_fixture(init_bytes);dyn(init_bytes+192,RE_GUEST_ELF32_DT_FINI_ARRAY,0x1195);
  assert(re_guest_elf32_parse_image(init_bytes,sizeof init_bytes,&image,&failed)==RE_GUEST_ELF32_OK);
  assert(re_guest_elf32_parse_dynamic(init_bytes,sizeof init_bytes,&image,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_E_ALIGNMENT);
 }
 puts("Guest ELF32 PT_DYNAMIC parser tests: PASS");return 0;
}
