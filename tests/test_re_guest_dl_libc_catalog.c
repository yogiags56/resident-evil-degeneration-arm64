#include "re_guest_dl_libc_catalog.h"
#include "re_guest_heap.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void *stub_open(void *u,const char *p,const char *m){(void)u;(void)p;(void)m;return NULL;}
static int64_t stub_read(void *u,void *s,void *d,uint64_t n){(void)u;(void)s;(void)d;(void)n;return -1;}
static int64_t stub_write(void *u,void *s,const void *d,uint64_t n){(void)u;(void)s;(void)d;(void)n;return -1;}
static int stub_close(void *u,void *s){(void)u;(void)s;return 0;}

int main(void) {
    static const char *const names[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT] = {
        "malloc","free","realloc","valloc",
        "memcpy","memmove","memset","strlen","strcmp","strncmp","strcpy",
        "strncpy","strncat","strchr","strstr","strcasecmp","strncasecmp",
        "atoi","strtok","snprintf","sprintf","printf",
        "fopen","fclose","fread","fwrite","fseek","ftell","fgets",
        "fprintf","feof","ferror","fflush","sscanf","fscanf","ungetc"
    };
    REGUESTELF32LINKSYMBOL symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTSYMBOL libc_exports[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTSYMBOL module_export = {
        "module_entry",0x71001000u,RE_GUEST_STB_GLOBAL,
        RE_GUEST_STV_DEFAULT,1u,NULL,"libsample.so"
    };
    REGUESTDLProviderSource module = {"libsample.so",&module_export,1u};
    REGUESTDLProviderCatalog catalog, before;
    REGUESTLIBCAllocProvider alloc;
    REGUESTLIBCStringProvider string;
    REGUESTLIBCFormatProvider format;
    REGUESTLIBCStdioProvider stdio;
    REGUESTLIBCStdioBackend backend = {
        stub_open,stub_read,stub_write,NULL,NULL,stub_close,NULL
    };
    REGUESTHEAP heap;
    REGUESTLIBCProviderSet set = {&alloc,&string,&format,&stdio};
    REGUESTIMPORTTable imports, import_before;
    size_t count=0u, failed_symbol=0u, failed_module=0u;
    uintptr_t handle=0u;
    uint32_t guest_address=0u;

    memset(symbols,0,sizeof(symbols));
    memset(libc_exports,0xa5,sizeof(libc_exports));
    for(size_t i=0;i<RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT;i++) {
        strcpy(symbols[i].name,names[i]);
        symbols[i].binding=RE_GUEST_STB_GLOBAL;
        symbols[i].visibility=RE_GUEST_STV_DEFAULT;
        symbols[i].type=2u;
        symbols[i].section_index=0u;
    }
    assert(re_guest_heap_init(&heap,0x50000000u,0x10000u)==RE_GUEST_HEAP_OK);
    assert(re_guest_libc_alloc_provider_init(&alloc,&heap)==RE_GUEST_LIBC_ALLOC_OK);
    assert(re_guest_libc_string_provider_init(&string)==RE_GUEST_LIBC_STRING_OK);
    assert(re_guest_libc_format_provider_init(&format)==RE_GUEST_LIBC_FORMAT_OK);
    assert(re_guest_libc_stdio_provider_init(&stdio,&backend,NULL)==RE_GUEST_LIBC_STDIO_OK);
    assert(re_guest_import_table_init(&imports,0x30000000u)==RE_IMPORT_OK);

    assert(re_guest_dl_provider_catalog_init_with_libc_set(&catalog,&module,1u,
        &set,&imports,symbols,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,
        libc_exports,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&count,
        &failed_symbol,&failed_module)==RE_GUEST_DL_PROVIDER_OK);
    assert(catalog.count==2u && count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT);
    assert(strcmp(catalog.entries[0].soname,"libsample.so")==0);
    assert(strcmp(catalog.entries[1].soname,"libc.so")==0);
    assert(catalog.entries[1].symbols==libc_exports);
    assert(imports.count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT);
    assert(failed_module==2u);
    assert(re_guest_dl_provider_catalog_open_callback("libc.so",0u,&handle,
        &catalog)==0);
    assert(handle==2u);
    assert(re_guest_dl_provider_catalog_symbol_callback(handle,"malloc",
        &guest_address,&catalog)==0);
    assert(guest_address==libc_exports[0].value && guest_address!=0u);
    assert(re_guest_dl_provider_catalog_close_callback(handle,&catalog)==0);

    /* Missing final stdio symbol must preserve the old catalog, imports and
       export storage even though earlier libc families bind before failure. */
    before=catalog;
    import_before=imports;
    memset(libc_exports,0x5a,sizeof(libc_exports));
    symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT-1u].name[0]='x';
    count=99u;
    assert(re_guest_dl_provider_catalog_init_with_libc_set(&catalog,&module,1u,
        &set,&imports,symbols,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,
        libc_exports,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&count,
        &failed_symbol,&failed_module)==RE_GUEST_DL_PROVIDER_E_LIBC);
    assert(memcmp(&catalog,&before,sizeof(catalog))==0);
    assert(memcmp(&imports,&import_before,sizeof(imports))==0);
    assert(count==0u);
    for(size_t i=0;i<sizeof(libc_exports);i++)
        assert(((const unsigned char *)libc_exports)[i]==0x5a);

    re_guest_heap_shutdown(&heap);
    puts("Guest DSO catalog + libc integration tests: PASS");
    return 0;
}
