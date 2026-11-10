#include "re_guest_dl_bridge.h"
#include "re_guest_runtime.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t open_calls, symbol_calls, close_calls;
    uint32_t last_flags;
    uintptr_t last_handle;
    int fail_open, fail_close;
    char last_path[RE_GUEST_DL_PATH_CAP];
    char last_symbol[RE_GUEST_DL_SYMBOL_CAP];
} FakeDL;

static int fake_open(const char *path,uint32_t flags,uintptr_t *handle,void *user) {
    FakeDL *f=(FakeDL *)user;
    f->open_calls++;f->last_flags=flags;
    strncpy(f->last_path,path,sizeof(f->last_path)-1u);
    f->last_path[sizeof(f->last_path)-1u]='\0';
    if(f->fail_open)return -1;
    *handle=(uintptr_t)0x12345678u+f->open_calls;
    f->last_handle=*handle;
    return 0;
}
static int fake_symbol(uintptr_t handle,const char *name,uint32_t *address,void *user) {
    FakeDL *f=(FakeDL *)user;
    f->symbol_calls++;f->last_handle=handle;
    strncpy(f->last_symbol,name,sizeof(f->last_symbol)-1u);
    f->last_symbol[sizeof(f->last_symbol)-1u]='\0';
    if(strcmp(name,"glTexImage2D")==0) {
        *address=0x21000001u;
        return 0;
    }
    *address=0;
    return -1;
}
static int fake_close(uintptr_t handle,void *user) {
    FakeDL *f=(FakeDL *)user;
    f->close_calls++;f->last_handle=handle;
    return f->fail_close?-1:0;
}
static void call_import(REGUESTRuntime *runtime,uint32_t pointer,
                        uint32_t lr,uint32_t r0,uint32_t r1) {
    runtime->cpu.r[15]=pointer&~1u;
    runtime->cpu.thumb=1u;
    runtime->cpu.r[14]=lr|1u;
    runtime->cpu.r[13]=0x10001800u;
    runtime->cpu.r[0]=r0;
    runtime->cpu.r[1]=r1;
    assert(re_guest_runtime_step(runtime)==RE_RUNTIME_OK);
    assert(runtime->cpu.r[15]==lr);
    assert(runtime->cpu.thumb==1u);
}

int main(void) {
    static uint8_t bytes[0x2000];
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTDLBridge bridge;
    REGUESTDLImportIDs ids={0xD100u,0xD101u,0xD102u,0xD103u};
    REGUESTDLImportPointers pointers, elf_pointers;
    REGUESTELF32LINKSYMBOL elf_symbols[5];
    REGUESTIMPORTTable elf_imports, rejected_imports;
    size_t failed_symbol=0;
    REGUESTSYMBOL provider_symbols[4];
    size_t provider_count=0;
    REGUESTSYMBOLREQUEST provider_request;
    REGUESTSYMBOLRESULT provider_result;
    FakeDL fake={0};
    uint32_t token,token2;

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory,0x10000000u,sizeof(bytes),bytes,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus,&memory);
    memcpy(bytes+0x100,"libGLESv1_CM.so",16);
    memcpy(bytes+0x140,"glTexImage2D",13);
    memcpy(bytes+0x160,"missing_symbol",15);

    assert(re_guest_dl_bridge_init(&bridge,fake_open,fake_symbol,fake_close,
        &fake,0x10000200u)==RE_GUEST_DL_OK);

    memset(elf_symbols,0,sizeof(elf_symbols));
    {
        static const char *const libdl_names[4] = {
            "dlopen","dlsym","dlclose","dlerror"
        };
        for(size_t i=0;i<4u;i++) {
            elf_symbols[i+1u].section_index=0u;
            elf_symbols[i+1u].binding=RE_GUEST_STB_GLOBAL;
            elf_symbols[i+1u].type=2u;
            elf_symbols[i+1u].visibility=RE_GUEST_STV_DEFAULT;
            strcpy(elf_symbols[i+1u].name,libdl_names[i]);
        }
    }
    assert(re_guest_import_table_init(&elf_imports,0x41000000u)==RE_IMPORT_OK);
    assert(re_guest_dl_bridge_bind_elf_symbols(&bridge,&elf_imports,
        elf_symbols,5u,&elf_pointers,&failed_symbol)==RE_GUEST_DL_OK);
    assert(failed_symbol==5u && elf_imports.count==4u);
    for(size_t i=0;i<4u;i++) assert(elf_imports.entries[i].symbol_index==i+1u);
    assert((elf_pointers.dlopen_pointer&1u)&&
           (elf_pointers.dlsym_pointer&1u)&&
           (elf_pointers.dlclose_pointer&1u)&&
           (elf_pointers.dlerror_pointer&1u));

    /* Duplicate undefined names and missing names must fail before binding. */
    elf_symbols[4]=elf_symbols[2];
    assert(re_guest_import_table_init(&rejected_imports,0x42000000u)==RE_IMPORT_OK);
    assert(re_guest_dl_bridge_bind_elf_symbols(&bridge,&rejected_imports,
        elf_symbols,5u,&elf_pointers,&failed_symbol)==
        RE_GUEST_DL_E_SYMBOL_INDEX);
    assert(failed_symbol==4u && rejected_imports.count==0u);
    assert(re_guest_import_table_init(&rejected_imports,0x42000000u)==RE_IMPORT_OK);
    assert(re_guest_dl_bridge_bind_elf_symbols(&bridge,&rejected_imports,
        elf_symbols,4u,&elf_pointers,&failed_symbol)==
        RE_GUEST_DL_E_SYMBOL_INDEX);
    assert(failed_symbol==4u && rejected_imports.count==0u);

    assert(re_guest_import_table_init(&imports,0x30000000u)==RE_IMPORT_OK);
    assert(re_guest_dl_bridge_bind_imports(&bridge,&imports,&ids,&pointers)==RE_GUEST_DL_OK);
    assert(imports.count==4);
    assert((pointers.dlopen_pointer&1u)&&(pointers.dlsym_pointer&1u)&&
           (pointers.dlclose_pointer&1u)&&(pointers.dlerror_pointer&1u));
    assert(re_guest_dl_bridge_build_provider_symbols("libdl.so",&pointers,
        provider_symbols,4u,&provider_count)==RE_GUEST_DL_OK);
    assert(provider_count==4u);
    assert(strcmp(provider_symbols[0].name,"dlopen")==0);
    assert(strcmp(provider_symbols[1].name,"dlsym")==0);
    assert(strcmp(provider_symbols[2].name,"dlclose")==0);
    assert(strcmp(provider_symbols[3].name,"dlerror")==0);
    assert(provider_symbols[0].value==pointers.dlopen_pointer);
    assert(provider_symbols[1].value==pointers.dlsym_pointer);
    assert(provider_symbols[2].value==pointers.dlclose_pointer);
    assert(provider_symbols[3].value==pointers.dlerror_pointer);
    provider_request=(REGUESTSYMBOLREQUEST){
        "dlsym",NULL,"libdl.so",0u
    };
    assert(re_guest_symbol_resolve(&provider_request,provider_symbols,
        provider_count,&provider_result)==RE_GUEST_SYMBOL_OK);
    assert(provider_result.value==pointers.dlsym_pointer);
    for(size_t i=0;i<4u;i++) {
        assert(provider_symbols[i].binding==RE_GUEST_STB_GLOBAL);
        assert(provider_symbols[i].visibility==RE_GUEST_STV_DEFAULT);
        assert(provider_symbols[i].defined==1u);
        assert(strcmp(provider_symbols[i].version_library,"libdl.so")==0);
    }
    assert(re_guest_dl_bridge_build_provider_symbols("libdl.so",&pointers,
        provider_symbols,3u,&provider_count)==RE_GUEST_DL_E_CAPACITY);
    assert(provider_count==0u);

    re_guest_runtime_init_mapped(&runtime,&bus,&imports,0x40000000u,
        0x10000000u,1);
    call_import(&runtime,pointers.dlopen_pointer,0x10000020u,0x10000100u,2u);
    token=runtime.cpu.r[0];
    assert(token!=0 && (token&0xff000000u)==RE_GUEST_DL_TOKEN_TAG);
    assert(token!=(uint32_t)fake.last_handle);
    assert(fake.open_calls==1 && fake.last_flags==2u);
    assert(strcmp(fake.last_path,"libGLESv1_CM.so")==0);

    call_import(&runtime,pointers.dlsym_pointer,0x10000040u,token,0x10000140u);
    assert(runtime.cpu.r[0]==0x21000001u);
    assert(fake.symbol_calls==1 && strcmp(fake.last_symbol,"glTexImage2D")==0);

    call_import(&runtime,pointers.dlsym_pointer,0x10000060u,token,0x10000160u);
    assert(runtime.cpu.r[0]==0u);
    call_import(&runtime,pointers.dlerror_pointer,0x10000080u,0u,0u);
    assert(runtime.cpu.r[0]==0x10000200u);
    assert(strcmp((const char *)(bytes+0x200),"dlsym: symbol not found")==0);
    call_import(&runtime,pointers.dlerror_pointer,0x100000a0u,0u,0u);
    assert(runtime.cpu.r[0]==0u);

    call_import(&runtime,pointers.dlclose_pointer,0x100000c0u,token,0u);
    assert(runtime.cpu.r[0]==0u && fake.close_calls==1);
    call_import(&runtime,pointers.dlsym_pointer,0x100000e0u,token,0x10000140u);
    assert(runtime.cpu.r[0]==0u);
    call_import(&runtime,pointers.dlerror_pointer,0x10000100u,0u,0u);
    assert(runtime.cpu.r[0]==0x10000200u);
    assert(strcmp((const char *)(bytes+0x200),"dlsym: invalid guest handle")==0);

    call_import(&runtime,pointers.dlopen_pointer,0x10000120u,0x10000100u,2u);
    token2=runtime.cpu.r[0];
    assert(token2!=0 && token2!=token);
    call_import(&runtime,pointers.dlclose_pointer,0x10000140u,token2,0u);
    assert(runtime.cpu.r[0]==0u && fake.close_calls==2);

    {
        uint32_t tokens[RE_GUEST_DL_MAX_HANDLES];
        uint32_t address=0;
        for(size_t i=0;i<RE_GUEST_DL_MAX_HANDLES;i++)
            assert(re_guest_dl_bridge_open(&bridge,"libGLESv1_CM.so",2u,
                &tokens[i])==RE_GUEST_DL_OK);
        token2=1u;
        assert(re_guest_dl_bridge_open(&bridge,"libhgl.so",2u,&token2)==
            RE_GUEST_DL_E_CAPACITY);
        assert(token2==0u);
        fake.fail_close=1;
        assert(re_guest_dl_bridge_close_all(&bridge)==RE_GUEST_DL_E_CLOSE);
        for(size_t i=0;i<RE_GUEST_DL_MAX_HANDLES;i++)
            assert(bridge.handles[i].active);
        fake.fail_close=0;
        assert(re_guest_dl_bridge_close_all(&bridge)==RE_GUEST_DL_OK);
        for(size_t i=0;i<RE_GUEST_DL_MAX_HANDLES;i++) {
            assert(!bridge.handles[i].active);
            assert(re_guest_dl_bridge_symbol(&bridge,tokens[i],
                "glTexImage2D",&address)==RE_GUEST_DL_E_HANDLE);
        }
    }

    fake.fail_open=1;
    call_import(&runtime,pointers.dlopen_pointer,0x10000160u,0x10000100u,2u);
    assert(runtime.cpu.r[0]==0u);
    call_import(&runtime,pointers.dlerror_pointer,0x10000180u,0u,0u);
    assert(runtime.cpu.r[0]==0x10000200u);
    assert(strcmp((const char *)(bytes+0x200),"dlopen: library open failed")==0);

    puts("Guest dlopen/dlsym/dlclose/dlerror bridge tests: PASS");
    return 0;
}
