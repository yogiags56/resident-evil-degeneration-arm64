#include "re_guest_dl_provider_bridge.h"

REGUESTDLStatus re_guest_dl_bridge_init_with_provider_catalog(
    REGUESTDLBridge *bridge,
    REGUESTDLProviderCatalog *catalog,
    uint32_t error_guest_address) {
    if (!bridge || !catalog || !catalog->count ||
        catalog->count > RE_GUEST_DL_PROVIDER_MAX)
        return RE_GUEST_DL_E_ARGUMENT;

    return re_guest_dl_bridge_init(
        bridge,
        re_guest_dl_provider_catalog_open_callback,
        re_guest_dl_provider_catalog_symbol_callback,
        re_guest_dl_provider_catalog_close_callback,
        catalog,
        error_guest_address);
}
