#include "re_guest_dl_bridge.h"
#include "re_guest_memory_bus.h"
#include <string.h>

#define TOKEN_TAG_MASK 0xff000000u
#define TOKEN_GEN_MASK 0x000fffffu
#define TOKEN_SLOT_MASK 0x0000000fu

static void set_error(REGUESTDLBridge *bridge, const char *message) {
    size_t n;
    if (!bridge) return;
    n = strlen(message);
    if (n >= sizeof(bridge->last_error)) n = sizeof(bridge->last_error) - 1u;
    memcpy(bridge->last_error, message, n);
    bridge->last_error[n] = '\0';
    bridge->error_pending = 1u;
}

static int read_guest_string(const REGUESTMEMBUS *bus, uint32_t address,
                             char *out, size_t capacity) {
    if (!bus || !out || !capacity || !address) return 0;
    for (size_t i = 0; i < capacity; i++) {
        uint8_t byte;
        if ((uint64_t)address + i > UINT32_MAX ||
            re_guest_memory_read8(bus, address + (uint32_t)i, &byte) != RE_GUEST_MEM_OK)
            return 0;
        if (byte == 0) {
            out[i] = '\0';
            return i != 0;
        }
        if (byte < 0x20u || byte > 0x7eu) return 0;
        out[i] = (char)byte;
    }
    out[capacity - 1u] = '\0';
    return 0;
}

static int write_guest_error(REGUESTDLBridge *bridge, REGUESTMEMBUS *bus) {
    size_t n;
    if (!bridge || !bus || !bridge->error_guest_address) return 0;
    n = strlen(bridge->last_error) + 1u;
    for (size_t i = 0; i < n; i++) {
        if ((uint64_t)bridge->error_guest_address + i > UINT32_MAX ||
            re_guest_memory_write8(bus, bridge->error_guest_address + (uint32_t)i,
                (uint8_t)bridge->last_error[i]) != RE_GUEST_MEM_OK)
            return 0;
    }
    return 1;
}

static REGUESTDLHandleSlot *find_handle(REGUESTDLBridge *bridge,
                                        uint32_t token) {
    uint32_t slot_field, generation;
    REGUESTDLHandleSlot *slot;
    if (!bridge || (token & TOKEN_TAG_MASK) != RE_GUEST_DL_TOKEN_TAG)
        return NULL;
    slot_field = token & TOKEN_SLOT_MASK;
    generation = (token >> 4) & TOKEN_GEN_MASK;
    if (!slot_field || slot_field > RE_GUEST_DL_MAX_HANDLES || !generation)
        return NULL;
    slot = &bridge->handles[slot_field - 1u];
    return slot->active && slot->token == token &&
        slot->generation == generation ? slot : NULL;
}

REGUESTDLStatus re_guest_dl_bridge_init(
    REGUESTDLBridge *bridge, REGUESTDLOpenCallback open_callback,
    REGUESTDLSymbolCallback symbol_callback,
    REGUESTDLCloseCallback close_callback, void *user,
    uint32_t error_guest_address) {
    if (!bridge || !open_callback || !symbol_callback || !close_callback)
        return RE_GUEST_DL_E_ARGUMENT;
    memset(bridge, 0, sizeof(*bridge));
    bridge->open_callback = open_callback;
    bridge->symbol_callback = symbol_callback;
    bridge->close_callback = close_callback;
    bridge->user = user;
    bridge->error_guest_address = error_guest_address;
    return RE_GUEST_DL_OK;
}

REGUESTDLStatus re_guest_dl_bridge_open(
    REGUESTDLBridge *bridge, const char *path, uint32_t flags,
    uint32_t *guest_handle_token) {
    REGUESTDLHandleSlot *slot = NULL;
    uintptr_t native_handle = 0;
    size_t index = 0;
    int rc;
    if (guest_handle_token) *guest_handle_token = 0;
    if (!bridge || !path || !path[0] || !guest_handle_token ||
        !bridge->open_callback) return RE_GUEST_DL_E_ARGUMENT;
    for (index = 0; index < RE_GUEST_DL_MAX_HANDLES; index++) {
        if (!bridge->handles[index].active) {
            slot = &bridge->handles[index];
            break;
        }
    }
    if (!slot) {
        set_error(bridge, "dlopen: guest handle table full");
        return RE_GUEST_DL_E_CAPACITY;
    }
    rc = bridge->open_callback(path, flags, &native_handle, bridge->user);
    if (rc != 0 || native_handle == (uintptr_t)0) {
        if (native_handle != (uintptr_t)0)
            (void)bridge->close_callback(native_handle, bridge->user);
        set_error(bridge, "dlopen: library open failed");
        return RE_GUEST_DL_E_OPEN;
    }
    slot->generation = (slot->generation + 1u) & TOKEN_GEN_MASK;
    if (!slot->generation) slot->generation = 1u;
    slot->token = RE_GUEST_DL_TOKEN_TAG |
        (slot->generation << 4) | (uint32_t)(index + 1u);
    slot->native_handle = native_handle;
    slot->active = 1u;
    *guest_handle_token = slot->token;
    return RE_GUEST_DL_OK;
}

REGUESTDLStatus re_guest_dl_bridge_symbol(
    REGUESTDLBridge *bridge, uint32_t token, const char *name,
    uint32_t *guest_address) {
    REGUESTDLHandleSlot *slot;
    uint32_t resolved = 0;
    int rc;
    if (guest_address) *guest_address = 0;
    if (!bridge || !name || !name[0] || !guest_address ||
        !bridge->symbol_callback) return RE_GUEST_DL_E_ARGUMENT;
    slot = find_handle(bridge, token);
    if (!slot) {
        set_error(bridge, "dlsym: invalid guest handle");
        return RE_GUEST_DL_E_HANDLE;
    }
    rc = bridge->symbol_callback(slot->native_handle, name, &resolved,
                                 bridge->user);
    if (rc != 0 || resolved == 0) {
        set_error(bridge, "dlsym: symbol not found");
        return RE_GUEST_DL_E_SYMBOL;
    }
    *guest_address = resolved;
    return RE_GUEST_DL_OK;
}

REGUESTDLStatus re_guest_dl_bridge_close(
    REGUESTDLBridge *bridge, uint32_t token) {
    REGUESTDLHandleSlot *slot;
    int rc;
    if (!bridge || !bridge->close_callback) return RE_GUEST_DL_E_ARGUMENT;
    slot = find_handle(bridge, token);
    if (!slot) {
        set_error(bridge, "dlclose: invalid guest handle");
        return RE_GUEST_DL_E_HANDLE;
    }
    rc = bridge->close_callback(slot->native_handle, bridge->user);
    if (rc != 0) {
        set_error(bridge, "dlclose: close failed");
        return RE_GUEST_DL_E_CLOSE;
    }
    slot->native_handle = (uintptr_t)0;
    slot->token = 0;
    slot->active = 0u;
    return RE_GUEST_DL_OK;
}

REGUESTDLStatus re_guest_dl_bridge_close_all(REGUESTDLBridge *bridge) {
    int failed = 0;
    if (!bridge || !bridge->close_callback) return RE_GUEST_DL_E_ARGUMENT;
    for (size_t i = 0; i < RE_GUEST_DL_MAX_HANDLES; i++) {
        REGUESTDLHandleSlot *slot = &bridge->handles[i];
        if (!slot->active) continue;
        if (bridge->close_callback(slot->native_handle, bridge->user) != 0) {
            set_error(bridge, "dlclose: bridge shutdown close failed");
            failed = 1;
            continue;
        }
        slot->native_handle = (uintptr_t)0;
        slot->token = 0;
        slot->active = 0u;
    }
    return failed ? RE_GUEST_DL_E_CLOSE : RE_GUEST_DL_OK;
}

static REGUESTIMPORTStatus dlopen_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTDLBridge *bridge = (REGUESTDLBridge *)user;
    char path[RE_GUEST_DL_PATH_CAP];
    uint32_t token = 0;
    if (!frame || !bus || !bridge || !result) return RE_IMPORT_E_HANDLER;
    if (!read_guest_string(bus, frame->r[0], path, sizeof(path))) {
        set_error(bridge, "dlopen: invalid guest path");
        result->r0 = 0; result->r1 = 0;
        return RE_IMPORT_OK;
    }
    if (re_guest_dl_bridge_open(bridge, path, frame->r[1], &token) != RE_GUEST_DL_OK)
        token = 0;
    result->r0 = token; result->r1 = 0;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus dlsym_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTDLBridge *bridge = (REGUESTDLBridge *)user;
    char name[RE_GUEST_DL_SYMBOL_CAP];
    uint32_t address = 0;
    if (!frame || !bus || !bridge || !result) return RE_IMPORT_E_HANDLER;
    if (!read_guest_string(bus, frame->r[1], name, sizeof(name))) {
        set_error(bridge, "dlsym: invalid guest symbol name");
        result->r0 = 0; result->r1 = 0;
        return RE_IMPORT_OK;
    }
    if (re_guest_dl_bridge_symbol(bridge, frame->r[0], name, &address) != RE_GUEST_DL_OK)
        address = 0;
    result->r0 = address; result->r1 = 0;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus dlclose_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTDLBridge *bridge = (REGUESTDLBridge *)user;
    REGUESTDLStatus status;
    (void)bus;
    if (!frame || !bridge || !result) return RE_IMPORT_E_HANDLER;
    status = re_guest_dl_bridge_close(bridge, frame->r[0]);
    result->r0 = status == RE_GUEST_DL_OK ? 0u : UINT32_MAX;
    result->r1 = 0;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus dlerror_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTDLBridge *bridge = (REGUESTDLBridge *)user;
    (void)frame;
    if (!bridge || !bus || !result) return RE_IMPORT_E_HANDLER;
    result->r0 = 0;
    result->r1 = 0;
    if (!bridge->error_pending) return RE_IMPORT_OK;
    if (bridge->error_guest_address && !write_guest_error(bridge, (REGUESTMEMBUS *)bus))
        return RE_IMPORT_E_MEMORY;
    result->r0 = bridge->error_guest_address;
    bridge->error_pending = 0u;
    return RE_IMPORT_OK;
}

REGUESTDLStatus re_guest_dl_bridge_bind_imports(
    REGUESTDLBridge *bridge, REGUESTIMPORTTable *imports,
    const REGUESTDLImportIDs *ids, REGUESTDLImportPointers *pointers) {
    REGUESTIMPORTTable backup;
    uint32_t values[4] = {0,0,0,0};
    REGUESTIMPORTStatus status;
    const uint32_t id_values[4] = {
        ids ? ids->dlopen_id : 0, ids ? ids->dlsym_id : 0,
        ids ? ids->dlclose_id : 0, ids ? ids->dlerror_id : 0
    };
    if (pointers) memset(pointers, 0, sizeof(*pointers));
    if (!bridge || !imports || !ids || !pointers) return RE_GUEST_DL_E_ARGUMENT;
    for (size_t i=0;i<4;i++) {
        if (!id_values[i]) return RE_GUEST_DL_E_ARGUMENT;
        for (size_t j=0;j<i;j++)
            if (id_values[i] == id_values[j]) return RE_GUEST_DL_E_ARGUMENT;
        for (size_t j=0;j<imports->count;j++)
            if (imports->entries[j].symbol_index == id_values[i])
                return RE_GUEST_DL_E_BIND;
    }
    if (imports->count > RE_IMPORT_MAX_BINDINGS - 4u)
        return RE_GUEST_DL_E_CAPACITY;
    backup = *imports;
    status = re_guest_import_bind_mapped(imports, ids->dlopen_id,
        dlopen_handler, bridge, &values[0]);
    if (status == RE_IMPORT_OK)
        status = re_guest_import_bind_mapped(imports, ids->dlsym_id,
            dlsym_handler, bridge, &values[1]);
    if (status == RE_IMPORT_OK)
        status = re_guest_import_bind_mapped(imports, ids->dlclose_id,
            dlclose_handler, bridge, &values[2]);
    if (status == RE_IMPORT_OK)
        status = re_guest_import_bind_mapped(imports, ids->dlerror_id,
            dlerror_handler, bridge, &values[3]);
    if (status != RE_IMPORT_OK) {
        *imports = backup;
        return RE_GUEST_DL_E_BIND;
    }
    pointers->dlopen_pointer = values[0];
    pointers->dlsym_pointer = values[1];
    pointers->dlclose_pointer = values[2];
    pointers->dlerror_pointer = values[3];
    return RE_GUEST_DL_OK;
}
