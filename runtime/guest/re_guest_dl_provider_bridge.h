#ifndef RE_GUEST_DL_PROVIDER_BRIDGE_H
#define RE_GUEST_DL_PROVIDER_BRIDGE_H

#include <stdint.h>

#include "re_guest_dl_bridge.h"
#include "re_guest_dl_provider_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize the guest libdl bridge with the bounded exact-SONAME provider
 * catalog as its callback backend. The catalog must remain alive and must not
 * be replaced while the bridge has open handles or is servicing guest calls.
 */
REGUESTDLStatus re_guest_dl_bridge_init_with_provider_catalog(
    REGUESTDLBridge *bridge,
    REGUESTDLProviderCatalog *catalog,
    uint32_t error_guest_address);

#ifdef __cplusplus
}
#endif
#endif
