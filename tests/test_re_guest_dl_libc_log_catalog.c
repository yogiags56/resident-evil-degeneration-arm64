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
    int *calls=(int *)u;(void)p;(void)t;(void)m;++*calls;return 0;
}

int main(void) {
    static const char *const names[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT] = {
        "malloc","free","realloc","valloc",
        "memcpy","memmove","memset","strlen","strcmp","strncmp","strcpy",
        "strncpy","strncat","strchr","strstr","strcasecmp","strncasecmp",
        "atoi","strtok","snprintf","sprintf","printf",
        "fopen","fclose","fread","fwrite","fseek","ftell","fgets",
        "fprintf","feof","ferror","fflush","sscanf","fscanf","ungetc"
    };
    REGUESTELF32LINKSYMBOL symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+2u];
    REGUESTSYMBOL libc_exports[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTSYMBOL log_exports[2];
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
    REGUESTLOGProvider log_provider;
    REGUESTIMPORTTable imports,import_before;
    size_t libc_count=0u,log_count=0u,failed_symbol=0u,failed_module=0u;
    uintptr_t libc_handle=0u,log_handle=0u;
    uint32_t address=0u;
    int log_calls=0;

    memset(symbols,0,sizeof(symbols));
    for(size_t i=0;i<RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT;i++){
        strcpy(symbols[i].name,names[i]);
        symbols[i].binding=RE_GUEST_STB_GLOBAL;
        symbols[i].visibility=RE_GUEST_STV_DEFAULT;
        symbols[i].type=2u;
        symbols[i].section_index=0u;
    }
    strcpy(symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT].name,"__android_log_print");
    strcpy(symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+1u].name,"__android_log_write");
    for(size_t i=RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT;i<RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+2u;i++){
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
    log_provider.write_callback=log_write;
    log_provider.user=&log_calls;
    assert(re_guest_import_table_init(&imports,0x30000000u)==RE_IMPORT_OK);

    assert(re_guest_dl_provider_catalog_init_with_libc_and_log(&catalog,&module,1u,
        &set,&log_provider,&imports,symbols,
        RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+2u,
        libc_exports,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&libc_count,
        log_exports,2u,&log_count,&failed_symbol,&failed_module)==RE_GUEST_DL_PROVIDER_OK);
    assert(catalog.count==3u && libc_count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT);
    assert(log_count==2u && imports.count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+2u);
    assert(strcmp(catalog.entries[1].soname,"libc.so")==0);
    assert(strcmp(catalog.entries[2].soname,"liblog.so")==0);
    assert(catalog.entries[1].symbols==libc_exports);
    assert(catalog.entries[2].symbols==log_exports);

    assert(re_guest_dl_provider_catalog_open_callback("libc.so",0u,&libc_handle,&catalog)==0);
    assert(re_guest_dl_provider_catalog_symbol_callback(libc_handle,"malloc",&address,&catalog)==0);
    assert(address==libc_exports[0].value && address!=0u);
    assert(re_guest_dl_provider_catalog_close_callback(libc_handle,&catalog)==0);

    assert(re_guest_dl_provider_catalog_open_callback("liblog.so",0u,&log_handle,&catalog)==0);
    assert(re_guest_dl_provider_catalog_symbol_callback(log_handle,"__android_log_print",&address,&catalog)==0);
    assert(address==log_exports[0].value && address!=0u);
    assert(re_guest_dl_provider_catalog_symbol_callback(log_handle,"__android_log_write",&address,&catalog)==0);
    assert(address==log_exports[1].value && address!=0u);
    assert(re_guest_dl_provider_catalog_close_callback(log_handle,&catalog)==0);

    /* Force a failure after libc binding: liblog's second required symbol is
       absent. Every published object must remain unchanged. */
    before=catalog;
    import_before=imports;
    memset(libc_exports,0x5a,sizeof(libc_exports));
    memset(log_exports,0x5a,sizeof(log_exports));
    symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+1u].name[0]='x';
    libc_count=99u;log_count=99u;
    assert(re_guest_dl_provider_catalog_init_with_libc_and_log(&catalog,&module,1u,
        &set,&log_provider,&imports,symbols,
        RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT+2u,
        libc_exports,RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&libc_count,
        log_exports,2u,&log_count,&failed_symbol,&failed_module)==RE_GUEST_DL_PROVIDER_E_LIBLOG);
    assert(memcmp(&catalog,&before,sizeof(catalog))==0);
    assert(memcmp(&imports,&import_before,sizeof(imports))==0);
    assert(libc_count==0u && log_count==0u);
    for(size_t i=0;i<sizeof(libc_exports);i++)assert(((unsigned char *)libc_exports)[i]==0x5a);
    for(size_t i=0;i<sizeof(log_exports);i++)assert(((unsigned char *)log_exports)[i]==0x5a);

    re_guest_heap_shutdown(&heap);
    puts("Guest libc + liblog DSO catalog integration tests: PASS");
    return 0;
}
