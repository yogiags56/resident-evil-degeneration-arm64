#include "re_guest_dl_libc_catalog.h"
#include "re_guest_heap.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void *stub_open(void *u,const char *p,const char *m){(void)u;(void)p;(void)m;return NULL;}
static int64_t stub_read(void *u,void *s,void *d,uint64_t n){(void)u;(void)s;(void)d;(void)n;return -1;}
static int64_t stub_write(void *u,void *s,const void *d,uint64_t n){(void)u;(void)s;(void)d;(void)n;return -1;}
static int stub_close(void *u,void *s){(void)u;(void)s;return 0;}
static int log_write(int p,const char *t,const char *m,void *u){(void)p;(void)t;(void)m;(void)u;return 0;}

int main(void) {
    static const char *const libc_names[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT] = {
        "malloc","free","realloc","valloc",
        "memcpy","memmove","memset","strlen","strcmp","strncmp","strcpy",
        "strncpy","strncat","strchr","strstr","strcasecmp","strncasecmp",
        "atoi","strtok","snprintf","sprintf","printf",
        "fopen","fclose","fread","fwrite","fseek","ftell","fgets",
        "fprintf","feof","ferror","fflush","sscanf","fscanf","ungetc"
    };
    static const char *const log_names[2] = {"__android_log_print","__android_log_write"};
    static const char *const zlib_names[4] = {"inflate","inflateEnd","inflateInit2_","inflateInit_"};
    static const char *const dl_names[4] = {"dlopen","dlsym","dlclose","dlerror"};
    REGUESTELF32LINKSYMBOL symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+11u];
    REGUESTSYMBOL libc_exports[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTSYMBOL log_exports[2],zlib_exports[4],dl_exports[4];
    REGUESTSYMBOL sample_export={"sample_entry",0x71001001u,
        RE_GUEST_STB_GLOBAL,RE_GUEST_STV_DEFAULT,1u,NULL,"libsample.so"};
    REGUESTDLProviderSource module={"libsample.so",&sample_export,1u};
    REGUESTDLProviderCatalog catalog;
    REGUESTDLBridge bridge;
    REGUESTLIBCAllocProvider alloc;
    REGUESTLIBCStringProvider string;
    REGUESTLIBCFormatProvider format;
    REGUESTLIBCStdioProvider stdio;
    REGUESTLIBCStdioBackend backend={stub_open,stub_read,stub_write,NULL,NULL,stub_close,NULL};
    REGUESTHEAP heap;
    REGUESTLIBCProviderSet set={&alloc,&string,&format,&stdio};
    REGUESTLOGProvider log_provider={log_write,NULL};
    REGUESTZLIBProvider zlib_provider;
    REGUESTIMPORTTable imports;
    size_t libc_count=0u,log_count=0u,zlib_count=0u,dl_count=0u;
    size_t failed_symbol=0u,failed_module=0u;
    uint32_t token=0u,address=0u;
    uintptr_t libdl_handle=0u;

    memset(symbols,0,sizeof(symbols));
    for(size_t i=0;i<RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT;i++){
        strcpy(symbols[i+1u].name,libc_names[i]);
        symbols[i+1u].binding=RE_GUEST_STB_GLOBAL;
        symbols[i+1u].visibility=RE_GUEST_STV_DEFAULT;
        symbols[i+1u].type=2u;
    }
    for(size_t i=0;i<2u;i++){
        size_t at=RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+1u+i;
        strcpy(symbols[at].name,log_names[i]);
        symbols[at].binding=RE_GUEST_STB_GLOBAL;
        symbols[at].visibility=RE_GUEST_STV_DEFAULT;
        symbols[at].type=2u;
    }
    for(size_t i=0;i<4u;i++){
        size_t at=RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+3u+i;
        strcpy(symbols[at].name,zlib_names[i]);
        symbols[at].binding=RE_GUEST_STB_GLOBAL;
        symbols[at].visibility=RE_GUEST_STV_DEFAULT;
        symbols[at].type=2u;
        at=RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+7u+i;
        strcpy(symbols[at].name,dl_names[i]);
        symbols[at].binding=RE_GUEST_STB_GLOBAL;
        symbols[at].visibility=RE_GUEST_STV_DEFAULT;
        symbols[at].type=2u;
    }

    assert(re_guest_heap_init(&heap,0x50000000u,0x10000u)==RE_GUEST_HEAP_OK);
    assert(re_guest_libc_alloc_provider_init(&alloc,&heap)==RE_GUEST_LIBC_ALLOC_OK);
    assert(re_guest_libc_string_provider_init(&string)==RE_GUEST_LIBC_STRING_OK);
    assert(re_guest_libc_format_provider_init(&format)==RE_GUEST_LIBC_FORMAT_OK);
    assert(re_guest_libc_stdio_provider_init(&stdio,&backend,NULL)==RE_GUEST_LIBC_STDIO_OK);
    assert(re_guest_zlib_provider_init(&zlib_provider,0x72000000u)==RE_GUEST_ZLIB_OK);
    assert(re_guest_import_table_init(&imports,0x30000000u)==RE_IMPORT_OK);

    assert(re_guest_dl_provider_catalog_init_with_libc_log_zlib_dl(
        &catalog,&bridge,&module,1u,&set,&log_provider,&zlib_provider,&imports,
        symbols,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+11u,
        libc_exports,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&libc_count,
        log_exports,2u,&log_count,zlib_exports,4u,&zlib_count,
        dl_exports,4u,&dl_count,0x40000000u,&failed_symbol,&failed_module)
        ==RE_GUEST_DL_PROVIDER_OK);
    assert(catalog.count==5u);
    assert(libc_count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT);
    assert(log_count==2u && zlib_count==4u && dl_count==4u);
    assert(imports.count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+10u);
    assert(strcmp(catalog.entries[4].soname,"libdl.so")==0);
    for(size_t i=0;i<4u;i++){
        assert(strcmp(dl_exports[i].name,dl_names[i])==0);
        assert(dl_exports[i].value!=0u);
        assert(strcmp(dl_exports[i].version_library,"libdl.so")==0);
    }

    /* The installed bridge resolves against the catalog after atomic publish. */
    assert(re_guest_dl_bridge_open(&bridge,"libsample.so",0u,&token)==RE_GUEST_DL_OK);
    assert(token!=0u && (token&0xff000000u)==RE_GUEST_DL_TOKEN_TAG);
    assert(re_guest_dl_bridge_symbol(&bridge,token,"sample_entry",&address)==RE_GUEST_DL_OK);
    assert(address==sample_export.value);
    assert(re_guest_dl_bridge_close(&bridge,token)==RE_GUEST_DL_OK);

    /* The virtual libdl.so exports resolve to the four generated guest stubs. */
    assert(re_guest_dl_provider_catalog_open_callback("libdl.so",0u,
        &libdl_handle,&catalog)==0);
    for(size_t i=0;i<4u;i++){
        assert(re_guest_dl_provider_catalog_symbol_callback(libdl_handle,
            dl_names[i],&address,&catalog)==0);
        assert(address==dl_exports[i].value);
    }
    assert(re_guest_dl_provider_catalog_close_callback(libdl_handle,&catalog)==0);

    assert(re_guest_dl_bridge_close_all(&bridge)==RE_GUEST_DL_OK);
    re_guest_zlib_provider_shutdown(&zlib_provider);
    re_guest_heap_shutdown(&heap);
    puts("Guest libc + liblog + libz + libdl catalog integration tests: PASS");
    return 0;
}
