#include "re_guest_dl_provider_bridge.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static const REGUESTSYMBOL gles_symbols[] = {
        {"glTexImage2D", 0x31000001u, RE_GUEST_STB_GLOBAL,
         RE_GUEST_STV_DEFAULT, 1u, NULL, "libGLESv1_CM.so"},
        {"gles_state", 0x32000000u, RE_GUEST_STB_GLOBAL,
         RE_GUEST_STV_DEFAULT, 1u, NULL, "libGLESv1_CM.so"}
    };
    const REGUESTDLProviderSource sources[] = {
        {"libGLESv1_CM.so", gles_symbols,
         sizeof(gles_symbols) / sizeof(gles_symbols[0])}
    };
    REGUESTDLProviderCatalog catalog;
    REGUESTDLBridge bridge;
    size_t failed = 0;
    uint32_t first = 0, second = 0, address = 0;

    memset(&catalog, 0, sizeof(catalog));
    assert(re_guest_dl_provider_catalog_init(
        &catalog, sources, 1u, &failed) == RE_GUEST_DL_PROVIDER_OK);
    assert(failed == 1u);

    assert(re_guest_dl_bridge_init_with_provider_catalog(
        &bridge, &catalog, 0u) == RE_GUEST_DL_OK);
    assert(re_guest_dl_bridge_open(
        &bridge, "libGLESv1_CM.so", 2u, &first) == RE_GUEST_DL_OK);
    assert(first != 0u && (first & 0xff000000u) == RE_GUEST_DL_TOKEN_TAG);
    assert(first != 1u); /* Host-private provider handle never escapes. */

    assert(re_guest_dl_bridge_symbol(
        &bridge, first, "glTexImage2D", &address) == RE_GUEST_DL_OK);
    assert(address == 0x31000001u); /* Thumb state bit is preserved. */
    assert(re_guest_dl_bridge_symbol(
        &bridge, first, "gles_state", &address) == RE_GUEST_DL_OK);
    assert(address == 0x32000000u); /* Even data address is preserved. */

    assert(re_guest_dl_bridge_open(
        &bridge, "libGLESv1_CM.so", 2u, &second) == RE_GUEST_DL_OK);
    assert(second != first);
    assert(catalog.entries[0].open_references == 2u);

    assert(re_guest_dl_bridge_close(&bridge, first) == RE_GUEST_DL_OK);
    assert(catalog.entries[0].open_references == 1u);
    assert(re_guest_dl_bridge_symbol(
        &bridge, second, "glTexImage2D", &address) == RE_GUEST_DL_OK);
    assert(address == 0x31000001u);

    assert(re_guest_dl_bridge_open(
        &bridge, "libNotRegistered.so", 2u, &address) == RE_GUEST_DL_E_OPEN);
    assert(address == 0u);
    assert(strcmp(bridge.last_error, "dlopen: library open failed") == 0);

    assert(re_guest_dl_bridge_close_all(&bridge) == RE_GUEST_DL_OK);
    assert(catalog.entries[0].open_references == 0u);
    assert(re_guest_dl_bridge_symbol(
        &bridge, second, "glTexImage2D", &address) == RE_GUEST_DL_E_HANDLE);
    assert(address == 0u);

    /* Empty and oversized catalogs are rejected before callback installation. */
    {
        REGUESTDLProviderCatalog empty;
        memset(&empty, 0, sizeof(empty));
        assert(re_guest_dl_bridge_init_with_provider_catalog(
            &bridge, &empty, 0u) == RE_GUEST_DL_E_ARGUMENT);
        empty.count = RE_GUEST_DL_PROVIDER_MAX + 1u;
        assert(re_guest_dl_bridge_init_with_provider_catalog(
            &bridge, &empty, 0u) == RE_GUEST_DL_E_ARGUMENT);
    }

    puts("Guest dlopen bridge/provider catalog integration tests: PASS");
    return 0;
}
