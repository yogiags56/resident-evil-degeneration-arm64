#include "re_guest_dl_libc_catalog.h"
#include "re_guest_heap.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void *stub_open(void *u,const char *p,const char *m){(void)u;(void)p;(void)m;return NULL;}
static int64_t stub_read(void *u,void *s,void *d,uint64_t n){(void)u;(void)s;(void)d;(void)n;return -1;}
static int64_t stub_write(void *u,void *s,const void *d,uint64_t n){(void)u;(void)s;(void)d;(void)n;return -1;}
static int stub_close(void *u,void *s){(void)u;(void)s;return 0;}
static int log_write(int p,const char *t,const char *m,void *u){
    (void)p;(void)t;(void)m;(void)u;return 0;
}

int main(void) {
    static const char *const libc_names[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT] = {
        "malloc","free","realloc","valloc",
        "memcpy","memmove","memset","strlen","strcmp","strncmp","strcpy",
        "strncpy","strncat","strchr","strstr","strcasecmp","strncasecmp",
        "atoi","strtok","snprintf","sprintf","printf",
        "fopen","fclose","fread","fwrite","fseek","ftell","fgets",
        "fprintf","feof","ferror","fflush","sscanf","fscanf","ungetc"
    };
    static const char *const log_names[2] = {
        "__android_log_print","__android_log_write"
    };
    static const char *const zlib_names[4] = {
        "inflate","inflateEnd","inflateInit2_","inflateInit_"
    };
    REGUESTELF32LINKSYMBOL symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+6u];
    REGUESTSYMBOL libc_exports[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTSYMBOL log_exports[2],zlib_exports[4];
    REGUESTSYMBOL module_export={"module_entry",0x71001000u,
        RE_GUEST_STB_GLOBAL,RE_GUEST_STV_DEFAULT,1u,NULL,"libsample.so"};
    REGUESTDLProviderSource module={"libsample.so",&module_export,1u};
    REGUESTDLProviderCatalog catalog,before;
    REGUESTLIBCAllocProvider alloc;
    REGUESTLIBCStringProvider string;
    REGUESTLIBCFormatProvider format;
    REGUESTLIBCStdioProvider stdio;
    REGUESTLIBCStdioBackend backend={stub_open,stub_read,stub_write,NULL,NULL,stub_close,NULL};
    REGUESTHEAP heap;
    REGUESTLIBCProviderSet set={&alloc,&string,&format,&stdio};
    REGUESTLOGProvider log_provider={log_write,NULL};
    REGUESTZLIBProvider zlib_provider;
    REGUESTIMPORTTable imports,import_before;
    size_t libc_count=0u,log_count=0u,zlib_count=0u,failed_symbol=0u,failed_module=0u;
    uintptr_t handle=0u;
    uint32_t address=0u;

    memset(symbols,0,sizeof(symbols));
    for(size_t i=0;i<RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT;i++){
        strcpy(symbols[i].name,libc_names[i]);
        symbols[i].binding=RE_GUEST_STB_GLOBAL;
        symbols[i].visibility=RE_GUEST_STV_DEFAULT;
        symbols[i].type=2u;
    }
    for(size_t i=0;i<2u;i++){
        strcpy(symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+i].name,log_names[i]);
        symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+i].binding=RE_GUEST_STB_GLOBAL;
        symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+i].visibility=RE_GUEST_STV_DEFAULT;
        symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+i].type=2u;
    }
    for(size_t i=0;i<4u;i++){
        size_t at=RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+2u+i;
        strcpy(symbols[at].name,zlib_names[i]);
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

    assert(re_guest_dl_provider_catalog_init_with_libc_log_zlib(
        &catalog,&module,1u,&set,&log_provider,&zlib_provider,&imports,
        symbols,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+6u,
        libc_exports,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&libc_count,
        log_exports,2u,&log_count,zlib_exports,4u,&zlib_count,
        &failed_symbol,&failed_module)==RE_GUEST_DL_PROVIDER_OK);
    assert(catalog.count==4u);
    assert(libc_count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT);
    assert(log_count==2u && zlib_count==4u);
    assert(imports.count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+6u);
    assert(strcmp(catalog.entries[1].soname,"libc.so")==0);
    assert(strcmp(catalog.entries[2].soname,"liblog.so")==0);
    assert(strcmp(catalog.entries[3].soname,"libz.so")==0);

    assert(re_guest_dl_provider_catalog_open_callback("libz.so",0u,&handle,&catalog)==0);
    for(size_t i=0;i<4u;i++){
        assert(re_guest_dl_provider_catalog_symbol_callback(handle,zlib_names[i],
            &address,&catalog)==0);
        assert(address==zlib_exports[i].value && address!=0u);
    }
    assert(re_guest_dl_provider_catalog_close_callback(handle,&catalog)==0);

    /* Missing final zlib symbol fails after libc and liblog binds; no output
       or import-table state is published. */
    before=catalog;
    assert(re_guest_import_table_init(&imports,0x32000000u)==RE_IMPORT_OK);
    import_before=imports;
    memset(libc_exports,0x5a,sizeof(libc_exports));
    memset(log_exports,0x5a,sizeof(log_exports));
    memset(zlib_exports,0x5a,sizeof(zlib_exports));
    symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+5u].name[0]='x';
    libc_count=log_count=zlib_count=99u;
    assert(re_guest_dl_provider_catalog_init_with_libc_log_zlib(
        &catalog,&module,1u,&set,&log_provider,&zlib_provider,&imports,
        symbols,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+6u,
        libc_exports,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&libc_count,
        log_exports,2u,&log_count,zlib_exports,4u,&zlib_count,
        &failed_symbol,&failed_module)==RE_GUEST_DL_PROVIDER_E_LIBZ);
    assert(memcmp(&catalog,&before,sizeof(catalog))==0);
    assert(memcmp(&imports,&import_before,sizeof(imports))==0);
    assert(libc_count==0u && log_count==0u && zlib_count==0u);
    for(size_t i=0;i<sizeof(libc_exports);i++)assert(((unsigned char *)libc_exports)[i]==0x5a);
    for(size_t i=0;i<sizeof(log_exports);i++)assert(((unsigned char *)log_exports)[i]==0x5a);
    for(size_t i=0;i<sizeof(zlib_exports);i++)assert(((unsigned char *)zlib_exports)[i]==0x5a);

    re_guest_zlib_provider_shutdown(&zlib_provider);
    re_guest_heap_shutdown(&heap);
    puts("Guest libc + liblog + libz DSO catalog integration tests: PASS");
    return 0;
}
