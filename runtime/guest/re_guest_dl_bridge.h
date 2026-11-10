#ifndef RE_GUEST_DL_BRIDGE_H
#define RE_GUEST_DL_BRIDGE_H
#include <stddef.h>
#include <stdint.h>
#include "re_guest_imports.h"
#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_DL_MAX_HANDLES 8u
#define RE_GUEST_DL_PATH_CAP 256u
#define RE_GUEST_DL_SYMBOL_CAP 128u
#define RE_GUEST_DL_ERROR_CAP 96u
#define RE_GUEST_DL_TOKEN_TAG 0xd1000000u

typedef enum {
    RE_GUEST_DL_OK = 0,
    RE_GUEST_DL_E_ARGUMENT = -900,
    RE_GUEST_DL_E_CAPACITY = -901,
    RE_GUEST_DL_E_HANDLE = -902,
    RE_GUEST_DL_E_OPEN = -903,
    RE_GUEST_DL_E_SYMBOL = -904,
    RE_GUEST_DL_E_CLOSE = -905,
    RE_GUEST_DL_E_STRING = -906,
    RE_GUEST_DL_E_MEMORY = -907,
    RE_GUEST_DL_E_BIND = -908
} REGUESTDLStatus;

/* The host side owns native_handle. It is never copied into guest memory.
 * resolve() must return a 32-bit guest-callable address (normally a Thumb
 * trampoline), never a raw 64-bit host function pointer.
 */
typedef int (*REGUESTDLOpenCallback)(
    const char *path, uint32_t flags, uintptr_t *native_handle, void *user);
typedef int (*REGUESTDLSymbolCallback)(
    uintptr_t native_handle, const char *name,
    uint32_t *guest_address, void *user);
typedef int (*REGUESTDLCloseCallback)(uintptr_t native_handle, void *user);

typedef struct {
    uintptr_t native_handle;
    uint32_t token;
    uint32_t generation;
    uint8_t active;
} REGUESTDLHandleSlot;

typedef struct {
    REGUESTDLHandleSlot handles[RE_GUEST_DL_MAX_HANDLES];
    REGUESTDLOpenCallback open_callback;
    REGUESTDLSymbolCallback symbol_callback;
    REGUESTDLCloseCallback close_callback;
    void *user;
    uint32_t error_guest_address;
    char last_error[RE_GUEST_DL_ERROR_CAP];
    uint8_t error_pending;
} REGUESTDLBridge;

typedef struct {
    uint32_t dlopen_id;
    uint32_t dlsym_id;
    uint32_t dlclose_id;
    uint32_t dlerror_id;
} REGUESTDLImportIDs;

typedef struct {
    uint32_t dlopen_pointer;
    uint32_t dlsym_pointer;
    uint32_t dlclose_pointer;
    uint32_t dlerror_pointer;
} REGUESTDLImportPointers;

REGUESTDLStatus re_guest_dl_bridge_init(
    REGUESTDLBridge *bridge,
    REGUESTDLOpenCallback open_callback,
    REGUESTDLSymbolCallback symbol_callback,
    REGUESTDLCloseCallback close_callback,
    void *user,
    uint32_t error_guest_address);

REGUESTDLStatus re_guest_dl_bridge_open(
    REGUESTDLBridge *bridge, const char *path, uint32_t flags,
    uint32_t *guest_handle_token);
REGUESTDLStatus re_guest_dl_bridge_symbol(
    REGUESTDLBridge *bridge, uint32_t guest_handle_token,
    const char *name, uint32_t *guest_address);
REGUESTDLStatus re_guest_dl_bridge_close(
    REGUESTDLBridge *bridge, uint32_t guest_handle_token);
/* Best-effort shutdown: closes every active handle and retains slots whose
 * host close callback fails so the caller can retry or report the leak.
 */
REGUESTDLStatus re_guest_dl_bridge_close_all(REGUESTDLBridge *bridge);
REGUESTDLStatus re_guest_dl_bridge_bind_imports(
    REGUESTDLBridge *bridge, REGUESTIMPORTTable *imports,
    const REGUESTDLImportIDs *ids, REGUESTDLImportPointers *pointers);

#ifdef __cplusplus
}
#endif
#endif
