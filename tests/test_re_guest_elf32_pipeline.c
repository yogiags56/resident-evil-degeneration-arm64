#include "re_guest_elf32_pipeline.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void p16(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void p32(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static void ph(uint8_t*p,uint32_t t,uint32_t o,uint32_t a,uint32_t fs,uint32_t ms,uint32_t f,uint32_t al){
 p32(p,t);p32(p+4,o);p32(p+8,a);p32(p+12,a);p32(p+16,fs);p32(p+20,ms);p32(p+24,f);p32(p+28,al);
}
static void dyn(uint8_t*p,uint32_t t,uint32_t v){p32(p,t);p32(p+4,v);}
static void make_module(uint8_t*b,int provider){
 memset(b,0,0x244);b[0]=0x7f;b[1]='E';b[2]='L';b[3]='F';b[4]=1;b[5]=1;b[6]=1;
 p16(b+16,RE_GUEST_ELF32_ET_DYN);p16(b+18,RE_GUEST_ELF32_EM_ARM);p32(b+20,1);
 p32(b+24,0x1000);p32(b+28,52);p16(b+40,52);p16(b+42,32);p16(b+44,3);
 ph(b+52,RE_GUEST_ELF32_PT_LOAD,0,0x1000,0x240,0x240,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_X,0x1000);
 ph(b+84,RE_GUEST_ELF32_PT_LOAD,0x240,0x2000,4,8,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_W,4);
 ph(b+116,RE_GUEST_ELF32_PT_DYNAMIC,0x100,0x1100,provider?56:80,provider?56:80,RE_GUEST_ELF32_PF_R,4);
 dyn(b+0x100,4,0x1220);dyn(b+0x108,5,0x1180);dyn(b+0x110,10,20);
 dyn(b+0x118,6,0x11c0);dyn(b+0x120,11,16);
 if(provider){
   dyn(b+0x128,14,5);dyn(b+0x130,0,0);
 }else{
   dyn(b+0x128,17,0x1238);dyn(b+0x130,18,8);dyn(b+0x138,19,8);
   dyn(b+0x140,1,5);dyn(b+0x148,0,0);
 }
 memcpy(b+0x180,"\0foo\0libprovider.so\0",20);
 p32(b+0x1c0+16,1);b[0x1c0+16+12]=0x10;
 p16(b+0x1c0+16+14,provider?1:0);
 if(provider)p32(b+0x1c0+16+4,0x2000);
 p32(b+0x220,1);p32(b+0x224,2);p32(b+0x228,1);
 if(!provider){
   p32(b+0x238,0x2000);p32(b+0x23c,(1u<<8)|RE_ARM_R_ABS32);
   p32(b+0x240,4);
 }
}
int main(void){
 uint8_t root_elf[0x244],provider_elf[0x244];
 uint8_t root_code[0x240],root_data[8],provider_code[0x240],provider_data[8];
 REGUESTELF32LOADBUFFER root_buffers[2]={{root_code,sizeof(root_code)},{root_data,sizeof(root_data)}};
 REGUESTELF32LOADBUFFER provider_buffers[2]={{provider_code,sizeof(provider_code)},{provider_data,sizeof(provider_data)}};
 REGUESTELF32LINKSYMBOL root_symbols[4],provider_symbols[4];
 REGUESTELF32LINKRELOCATION root_decoded[4],provider_decoded[4];
 REGUESTRELOCATION root_prepared[4],provider_prepared[4];
 REGUESTELF32PIPELINEMODULE modules[2];
 REGUESTSYMBOL provider_scratch[8];
 REGUESTELF32PIPELINERESULT result;
 REGUESTMEMORY memory;
 size_t failed_module=99,failed_index=99;int inner=0;
 make_module(root_elf,0);make_module(provider_elf,1);
 memset(modules,0,sizeof(modules));
 modules[0]=(REGUESTELF32PIPELINEMODULE){"game.so",root_elf,sizeof(root_elf),0x100000,
   root_buffers,2,root_symbols,4,root_decoded,4,root_prepared,4};
 modules[1]=(REGUESTELF32PIPELINEMODULE){"libprovider.so",provider_elf,sizeof(provider_elf),0x200000,
   provider_buffers,2,provider_symbols,4,provider_decoded,4,provider_prepared,4};
 re_guest_memory_init(&memory);
 assert(re_guest_elf32_load_pipeline(&memory,modules,2,"game.so",
   provider_scratch,8,&result,&failed_module,&failed_index,&inner)==RE_GUEST_ELF32_PIPELINE_OK);
 assert(result.module_count==2&&result.provider_count==1);
 assert(result.graph.order_count==2&&result.graph.order[0]==1&&result.graph.order[1]==0);
 assert(result.loaded.module_count==2&&re_guest_memory_region_count(&memory)==4);
 assert(root_data[0]==0x04&&root_data[1]==0x20&&root_data[2]==0x20&&root_data[3]==0x00);
 assert(!strcmp(provider_scratch[0].name,"foo")&&provider_scratch[0].value==0x202000);
 assert(failed_module==2&&inner==0);
 puts("Guest ELF32 unified multi-module pipeline tests: PASS");
 return 0;
}
