#include "re_guest_elf32_boot.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void){
 REGUESTMEMORY memory;REGUESTMEMBUS bus;REGUESTIMPORTTable imports;
 REGUESTELF32PIPELINEMODULE modules[2]={0};
 REGUESTELF32PIPELINERESULT pipeline={0};REGUESTRuntime runtime;
 REGUESTELF32BOOTINFO info;uint8_t code[8]={0},stack[512]={0};
 re_guest_memory_init(&memory);
 assert(re_guest_memory_map_ex(&memory,0x101000,sizeof(code),code,
   RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&memory,0x800000,sizeof(stack),stack,
   RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 re_guest_memory_bus_init(&bus,&memory);
 assert(re_guest_import_table_init(&imports,0x70000000)==RE_IMPORT_OK);
 modules[0].load_bias=0x100000;
 pipeline.module_count=2;pipeline.root_module_index=0;
 pipeline.graph.complete=1;pipeline.graph.order_count=2;
 pipeline.graph.order[0]=1;pipeline.graph.order[1]=0;
 pipeline.graph.reachable[0]=pipeline.graph.reachable[1]=1;
 pipeline.loaded.module_count=2;
 pipeline.loaded.modules[1].entry_valid=1;
 pipeline.loaded.modules[1].entry_point=0x101001;
 pipeline.loaded.modules[1].thumb_entry=1;
 pipeline.metadata[0].image.entry=0x1001;
 assert(re_guest_elf32_start_root_runtime(&bus,&imports,modules,2,&pipeline,
   0x800200,0x6ffff000,&runtime,&info)==RE_GUEST_ELF32_BOOT_OK);
 assert(info.root_module_index==0&&info.entry_point==0x101001&&info.thumb_state==1);
 assert(runtime.cpu.r[15]==0x101000&&runtime.cpu.thumb==1&&runtime.cpu.r[13]==0x800200);
 puts("Guest ELF32 root boot handoff tests: PASS");
 return 0;
}
