#include "re_guest_elf32_init_exec.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
 REGUESTMEMORY memory;REGUESTMEMBUS bus;REGUESTIMPORTTable imports;
 REGUESTELF32INITEXECSTATE state={0};REGUESTELF32INITEXECRESULT result;
 REGUESTELF32INITCALL call={0x1001,0,RE_GUEST_ELF32_INIT_KIND_DT_INIT,1};
 uint8_t code[4]={0x01,0x20,0x70,0x47},stack[512]={0};
 uint32_t trampoline=0;
 re_guest_memory_init(&memory);
 assert(re_guest_memory_map_ex(&memory,0x1000,sizeof(code),code,RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&memory,0x3000,sizeof(stack),stack,RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 re_guest_memory_bus_init(&bus,&memory);
 assert(re_guest_import_table_init(&imports,0x70000000)==RE_IMPORT_OK);
 assert(re_guest_elf32_init_bind_return(&imports,&state,&trampoline)==RE_IMPORT_OK);
 assert((trampoline&1u)==1u);
 assert(re_guest_elf32_execute_init_sequence(&memory,&bus,&imports,&call,1,
   0x3200,0x6ffff000,trampoline,16,&state,&result)==RE_GUEST_ELF32_INIT_EXEC_OK);
 assert(result.completed_calls==1&&result.total_steps==3&&result.runtime_status==RE_RUNTIME_OK);
 assert(state.return_count==1&&!state.active&&!state.completed);
 puts("Guest ELF32 bounded constructor execution tests: PASS");
 return 0;
}
