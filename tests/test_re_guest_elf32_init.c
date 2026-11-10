#include "re_guest_elf32_init.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void put32(uint8_t *p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
int main(void){
 REGUESTMEMORY memory;REGUESTELF32PIPELINEMODULE modules[2]={0};
 REGUESTELF32PIPELINERESULT pipeline={0};REGUESTELF32INITCALL calls[8],before[8];
 uint8_t dep_code[16]={0},root_code[16]={0},dep_array[4],root_array[8];
 size_t count=0,fm=99,fc=99;
 put32(dep_array,0x201001);put32(root_array,0x101000);put32(root_array+4,0x101004);
 re_guest_memory_init(&memory);
 assert(re_guest_memory_map_ex(&memory,0x201000,sizeof(dep_code),dep_code,RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&memory,0x101000,sizeof(root_code),root_code,RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&memory,0x202000,sizeof(dep_array),dep_array,RE_GUEST_MEM_READ,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&memory,0x102000,sizeof(root_array),root_array,RE_GUEST_MEM_READ,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 modules[0].load_bias=0x100000;modules[1].load_bias=0x200000;
 pipeline.module_count=2;pipeline.graph.complete=1;pipeline.graph.order_count=2;
 pipeline.graph.order[0]=1;pipeline.graph.order[1]=0;
 pipeline.graph.reachable[0]=pipeline.graph.reachable[1]=1;
 pipeline.metadata[1].image.little_endian=1;
 pipeline.metadata[1].dynamic.has_init=1;pipeline.metadata[1].dynamic.init_address=0x1000;
 pipeline.metadata[1].dynamic.has_init_array=1;pipeline.metadata[1].dynamic.has_init_array_size=1;
 pipeline.metadata[1].dynamic.init_array_address=0x2000;pipeline.metadata[1].dynamic.init_array_size=4;
 pipeline.metadata[0].image.little_endian=1;
 pipeline.metadata[0].dynamic.has_init=1;pipeline.metadata[0].dynamic.init_address=0x1000;
 pipeline.metadata[0].dynamic.has_init_array=1;pipeline.metadata[0].dynamic.has_init_array_size=1;
 pipeline.metadata[0].dynamic.init_array_address=0x2000;pipeline.metadata[0].dynamic.init_array_size=8;
 assert(re_guest_elf32_build_init_sequence(&memory,modules,2,&pipeline,calls,8,&count,&fm,&fc)==RE_GUEST_ELF32_INIT_OK);
 assert(count==5&&fm==2);
 assert(calls[0].module_index==1&&calls[0].kind==RE_GUEST_ELF32_INIT_KIND_DT_INIT&&calls[0].address==0x201000);
 assert(calls[1].module_index==1&&calls[1].kind==RE_GUEST_ELF32_INIT_KIND_INIT_ARRAY&&calls[1].address==0x201001&&calls[1].thumb);
 assert(calls[2].module_index==0&&calls[2].kind==RE_GUEST_ELF32_INIT_KIND_DT_INIT&&calls[2].address==0x101000);
 assert(calls[3].module_index==0&&calls[3].address==0x101000);
 assert(calls[4].module_index==0&&calls[4].address==0x101004);
 memcpy(before,calls,sizeof(calls));calls[0].address=0xdeadbeef;
 assert(re_guest_elf32_build_init_sequence(&memory,modules,2,&pipeline,calls,2,&count,&fm,&fc)==RE_GUEST_ELF32_INIT_E_CAPACITY);
 assert(count==0&&calls[0].address==0xdeadbeef);
 calls[0].address=0xfeedface;put32(root_array+4,0xdead0000);
 assert(re_guest_elf32_build_init_sequence(&memory,modules,2,&pipeline,calls,8,&count,&fm,&fc)==RE_GUEST_ELF32_INIT_E_ENTRY);
 assert(count==0&&calls[0].address==0xfeedface&&fm==0);
 puts("Guest ELF32 constructor sequence tests: PASS");
 return 0;
}
