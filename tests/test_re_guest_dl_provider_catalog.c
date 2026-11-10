#include "re_guest_dl_provider_catalog.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static const REGUESTSYMBOL gles_symbols[] = {
        {"glTexImage2D", 0x21000001u, RE_GUEST_STB_GLOBAL,
         RE_GUEST_STV_DEFAULT, 1u, NULL, "libGLESv1_CM.so"},
        {"gles_state", 0x22000000u, RE_GUEST_STB_GLOBAL,
         RE_GUEST_STV_DEFAULT, 1u, NULL, "libGLESv1_CM.so"}
    };
    static const REGUESTSYMBOL egl_symbols[] = {
        {"eglSwapBuffers", 0x23000001u, RE_GUEST_STB_GLOBAL,
         RE_GUEST_STV_DEFAULT, 1u, NULL, "libEGL.so"}
    };
    const REGUESTDLProviderSource sources[] = {
        {"libGLESv1_CM.so", gles_symbols,
         sizeof(gles_symbols) / sizeof(gles_symbols[0])},
        {"libEGL.so", egl_symbols,
         sizeof(egl_symbols) / sizeof(egl_symbols[0])}
    };
    REGUESTDLProviderCatalog catalog;
    size_t failed = 0;
    uintptr_t gles_handle = 0, gles_handle_second = 0, egl_handle = 0;
    uint32_t address = 0;

    memset(&catalog, 0xa5, sizeof(catalog));
    assert(re_guest_dl_provider_catalog_init(&catalog, sources, 2u,
        &failed) == RE_GUEST_DL_PROVIDER_OK);
    assert(failed == 2u && catalog.count == 2u);
    assert(catalog.entries[0].open_references == 0u);
    assert(strcmp(catalog.entries[0].soname, "libGLESv1_CM.so") == 0);

    /* Only exact registered SONAMEs are accepted; no path traversal/search. */
    assert(re_guest_dl_provider_catalog_open_callback(
        "../libGLESv1_CM.so", 2u, &gles_handle, &catalog) != 0);
    assert(gles_handle == 0u);
    assert(re_guest_dl_provider_catalog_open_callback(
        "libMissing.so", 2u, &gles_handle, &catalog) != 0);
    assert(gles_handle == 0u);
    assert(re_guest_dl_provider_catalog_open_callback(
        "libGLESv1_CM.so", 2u, &gles_handle, &catalog) == 0);
    assert(re_guest_dl_provider_catalog_open_callback(
        "libGLESv1_CM.so", 2u, &gles_handle_second, &catalog) == 0);
    assert(gles_handle == 1u && gles_handle_second == gles_handle);
    assert(catalog.entries[0].open_references == 2u);

    /* dlsym returns guest addresses, preserving Thumb and data-pointer bits. */
    assert(re_guest_dl_provider_catalog_symbol_callback(
        gles_handle, "glTexImage2D", &address, &catalog) == 0);
    assert(address == 0x21000001u);
    assert(re_guest_dl_provider_catalog_symbol_callback(
        gles_handle, "gles_state", &address, &catalog) == 0);
    assert(address == 0x22000000u);
    assert(re_guest_dl_provider_catalog_symbol_callback(
        gles_handle, "eglSwapBuffers", &address, &catalog) != 0);
    assert(address == 0u);
    assert(re_guest_dl_provider_catalog_symbol_callback(
        gles_handle, "missing_symbol", &address, &catalog) != 0);
    assert(address == 0u);

    assert(re_guest_dl_provider_catalog_open_callback(
        "libEGL.so", 2u, &egl_handle, &catalog) == 0);
    assert(re_guest_dl_provider_catalog_symbol_callback(
        egl_handle, "eglSwapBuffers", &address, &catalog) == 0);
    assert(address == 0x23000001u);

    /* Closing one duplicate open leaves the other handle usable. */
    assert(re_guest_dl_provider_catalog_close_callback(
        gles_handle, &catalog) == 0);
    assert(catalog.entries[0].open_references == 1u);
    assert(re_guest_dl_provider_catalog_symbol_callback(
        gles_handle_second, "glTexImage2D", &address, &catalog) == 0);
    assert(address == 0x21000001u);
    assert(re_guest_dl_provider_catalog_close_callback(
        gles_handle_second, &catalog) == 0);
    assert(re_guest_dl_provider_catalog_symbol_callback(
        gles_handle_second, "glTexImage2D", &address, &catalog) != 0);
    assert(address == 0u);
    assert(re_guest_dl_provider_catalog_close_callback(
        gles_handle_second, &catalog) != 0);
    assert(re_guest_dl_provider_catalog_close_callback(
        egl_handle, &catalog) == 0);

    /* Failed reinitialization must not destroy the working catalog. */
    {
        const REGUESTDLProviderSource duplicate[] = {
            {"libA.so", NULL, 0u}, {"libA.so", NULL, 0u}
        };
        assert(re_guest_dl_provider_catalog_init(&catalog, duplicate, 2u,
            &failed) == RE_GUEST_DL_PROVIDER_E_NAME);
        assert(failed == 1u && catalog.count == 2u);
        assert(strcmp(catalog.entries[1].soname, "libEGL.so") == 0);
    }

    puts("Guest DSO provider catalog tests: PASS");
    return 0;
}
