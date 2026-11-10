#include "re_guest_dl_elf32_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define SHN_ABS 0xfff1u
int main(void) {
    REGUESTELF32LINKSYMBOL gles[2] = {0};
    REGUESTELF32LINKSYMBOL egl[1] = {0};
    REGUESTELF32PROVIDERMODULE modules[2] = {
        {"libGLESv1_CM.so", gles, 2u, 0x10000000u},
        {"libEGL.so", egl, 1u, 0x20000000u}
    };
    REGUESTSYMBOL storage[3];
    REGUESTDLProviderCatalog catalog, before;
    size_t count = 99u, failed_module = 99u, failed_symbol = 99u;
    uintptr_t handle = 0;
    uint32_t address = 0;

    strcpy(gles[0].name, "glTexImage2D");
    gles[0].binding = RE_GUEST_STB_GLOBAL;
    gles[0].visibility = RE_GUEST_STV_DEFAULT;
    gles[0].section_index = 1u;
    gles[0].value = 0x00001235u;
    strcpy(gles[1].name, "gles_state");
    gles[1].binding = RE_GUEST_STB_GLOBAL;
    gles[1].visibility = RE_GUEST_STV_DEFAULT;
    gles[1].section_index = 1u;
    gles[1].value = 0x00002000u;
    strcpy(egl[0].name, "eglSwapBuffers");
    egl[0].binding = RE_GUEST_STB_GLOBAL;
    egl[0].visibility = RE_GUEST_STV_DEFAULT;
    egl[0].section_index = SHN_ABS;
    egl[0].value = 0x20000001u;

    memset(&catalog, 0x5a, sizeof(catalog));
    assert(re_guest_dl_provider_catalog_init_from_elf32(
        &catalog, modules, 2u, storage, 3u, &count,
        &failed_module, &failed_symbol) == RE_GUEST_DL_PROVIDER_OK);
    assert(count == 3u && failed_module == 2u && failed_symbol == 0u);
    assert(catalog.count == 2u);
    assert(storage[0].value == 0x10001235u);
    assert(storage[1].value == 0x10002000u);
    assert(storage[2].value == 0x20000001u);
    assert(strcmp(storage[0].version_library, "libGLESv1_CM.so") == 0);

    assert(re_guest_dl_provider_catalog_open_callback(
        "libGLESv1_CM.so", 2u, &handle, &catalog) == 0);
    assert(re_guest_dl_provider_catalog_symbol_callback(
        handle, "glTexImage2D", &address, &catalog) == 0);
    assert(address == 0x10001235u);
    assert(re_guest_dl_provider_catalog_symbol_callback(
        handle, "eglSwapBuffers", &address, &catalog) != 0);
    assert(address == 0u);
    assert(re_guest_dl_provider_catalog_close_callback(handle, &catalog) == 0);

    before = catalog;
    assert(re_guest_dl_provider_catalog_init_from_elf32(
        &catalog, modules, 2u, storage, 2u, &count,
        &failed_module, &failed_symbol) == RE_GUEST_DL_PROVIDER_E_ELF32);
    assert(count == 0u && failed_module == 1u);
    assert(memcmp(&catalog, &before, sizeof(catalog)) == 0);

    {
        REGUESTELF32PROVIDERMODULE duplicate[2] = {modules[0], modules[1]};
        duplicate[1].soname = "libGLESv1_CM.so";
        assert(re_guest_dl_provider_catalog_init_from_elf32(
            &catalog, duplicate, 2u, storage, 3u, &count,
            &failed_module, &failed_symbol) == RE_GUEST_DL_PROVIDER_E_NAME);
        assert(failed_module == 1u);
        assert(memcmp(&catalog, &before, sizeof(catalog)) == 0);
    }
    puts("Guest ELF32-to-dlopen provider adapter tests: PASS");
    return 0;
}
