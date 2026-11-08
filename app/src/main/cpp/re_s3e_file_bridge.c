#include "re_s3e_file_bridge.h"

#include "re_s3e_file_api_audit.h"
#include "re_s3e_file_read_contract.h"

static int valid_bridge(const RES3EFileBridge *bridge) {
    return bridge && bridge->adapter;
}

RES3EFileBridgeStatus re_s3e_file_bridge_init(
    RES3EFileBridge *bridge, REFileAdapter *adapter) {
    if (!bridge || !adapter) return RE_S3E_BRIDGE_E_ARGUMENT;
    bridge->adapter = adapter;
    return RE_S3E_BRIDGE_OK;
}

RES3EFileBridgeStatus re_s3e_file_bridge_open(
    RES3EFileBridge *bridge, const REGUESTMEM *memory,
    const uint32_t args[4], uint32_t *guest_handle) {
    RES3EFileOpenArgs captured;
    REFileStatus status;
    if (!valid_bridge(bridge) || !memory || !args || !guest_handle)
        return RE_S3E_BRIDGE_E_ARGUMENT;
    *guest_handle = 0u;
    if (re_s3e_file_open_capture(args, &captured) != RE_S3E_FILE_API_OK)
        return RE_S3E_BRIDGE_E_ARGUMENT;
    status = re_file_adapter_open(bridge->adapter, memory,
                                  captured.path_guest_address,
                                  captured.mode_guest_address, guest_handle);
    if (status != RE_FILE_OK) {
        *guest_handle = 0u;
        return RE_S3E_BRIDGE_E_OPEN;
    }
    return RE_S3E_BRIDGE_OK;
}

RES3EFileBridgeStatus re_s3e_file_bridge_read(
    RES3EFileBridge *bridge, const REGUESTMEM *memory,
    const uint32_t args[4], uint32_t *items_read) {
    RES3EFileReadArgs captured;
    uint64_t bytes_read = 0;
    uint64_t complete_items;
    REFileStatus status;
    if (!valid_bridge(bridge) || !memory || !args || !items_read)
        return RE_S3E_BRIDGE_E_ARGUMENT;
    *items_read = 0u;
    if (re_s3e_file_read_capture(args, &captured) != RE_S3E_READ_CONTRACT_OK)
        return RE_S3E_BRIDGE_E_ARGUMENT;

    status = re_file_adapter_read_bytes(
        bridge->adapter, memory, captured.file_handle,
        captured.destination, captured.requested_bytes, &bytes_read);
    if (status == RE_FILE_E_BAD_HANDLE) return RE_S3E_BRIDGE_E_BAD_HANDLE;
    if (status != RE_FILE_OK) return RE_S3E_BRIDGE_E_READ;

    /* The original guest compares this with the requested item count.
     * Return only complete items; any trailing bytes have already been
     * transferred by the byte-oriented backend, as with fread semantics.
     */
    complete_items = bytes_read / captured.item_size;
    if (complete_items > captured.item_count || complete_items > UINT32_MAX)
        return RE_S3E_BRIDGE_E_READ;
    *items_read = (uint32_t)complete_items;
    return RE_S3E_BRIDGE_OK;
}

RES3EFileBridgeStatus re_s3e_file_bridge_get_char(
    RES3EFileBridge *bridge, const uint32_t args[4],
    int32_t *guest_character) {
    RES3EFileHandleArgs captured;
    REFileStatus status;
    if (!valid_bridge(bridge) || !args || !guest_character)
        return RE_S3E_BRIDGE_E_ARGUMENT;
    if (re_s3e_file_get_char_capture(args, &captured) != RE_S3E_FILE_API_OK)
        return RE_S3E_BRIDGE_E_ARGUMENT;
    status = re_file_adapter_get_char(bridge->adapter, captured.file_handle,
                                      guest_character);
    if (status == RE_FILE_E_BAD_HANDLE) return RE_S3E_BRIDGE_E_BAD_HANDLE;
    if (status != RE_FILE_OK) return RE_S3E_BRIDGE_E_GET_CHAR;
    return RE_S3E_BRIDGE_OK;
}

RES3EFileBridgeStatus re_s3e_file_bridge_close(
    RES3EFileBridge *bridge, const uint32_t args[4]) {
    RES3EFileHandleArgs captured;
    REFileStatus status;
    if (!valid_bridge(bridge) || !args)
        return RE_S3E_BRIDGE_E_ARGUMENT;
    if (re_s3e_file_close_capture(args, &captured) != RE_S3E_FILE_API_OK)
        return RE_S3E_BRIDGE_E_ARGUMENT;
    status = re_file_adapter_close(bridge->adapter, captured.file_handle);
    if (status == RE_FILE_E_BAD_HANDLE) return RE_S3E_BRIDGE_E_BAD_HANDLE;
    if (status != RE_FILE_OK) return RE_S3E_BRIDGE_E_CLOSE;
    return RE_S3E_BRIDGE_OK;
}

static REGUESTIMPORTStatus map_status(RES3EFileBridgeStatus status) {
    return status == RE_S3E_BRIDGE_OK ? RE_IMPORT_OK : RE_IMPORT_E_HANDLER;
}

REGUESTIMPORTStatus re_s3e_file_bridge_open_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result) {
    uint32_t handle = 0;
    RES3EFileBridgeStatus status;
    if (!frame || !memory || !result || !valid_bridge((const RES3EFileBridge *)user))
        return RE_IMPORT_E_HANDLER;
    status = re_s3e_file_bridge_open((RES3EFileBridge *)user, memory,
                                     frame->r, &handle);
    /* Original guest callers explicitly test the Open result against null.
     * Map an ordinary not-found/open failure to a null handle; malformed ABI
     * arguments remain a handler error. */
    if (status == RE_S3E_BRIDGE_E_OPEN) {
        result->r0 = 0u;
        result->r1 = 0u;
        return RE_IMPORT_OK;
    }
    if (status != RE_S3E_BRIDGE_OK) return map_status(status);
    result->r0 = handle;
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

REGUESTIMPORTStatus re_s3e_file_bridge_read_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result) {
    uint32_t items = 0;
    RES3EFileBridgeStatus status;
    if (!frame || !memory || !result || !valid_bridge((const RES3EFileBridge *)user))
        return RE_IMPORT_E_HANDLER;
    status = re_s3e_file_bridge_read((RES3EFileBridge *)user, memory,
                                     frame->r, &items);
    if (status != RE_S3E_BRIDGE_OK) return map_status(status);
    result->r0 = items;
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

REGUESTIMPORTStatus re_s3e_file_bridge_get_char_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result) {
    int32_t value = -1;
    RES3EFileBridgeStatus status;
    (void)memory;
    if (!frame || !result || !valid_bridge((const RES3EFileBridge *)user))
        return RE_IMPORT_E_HANDLER;
    status = re_s3e_file_bridge_get_char((RES3EFileBridge *)user,
                                         frame->r, &value);
    if (status != RE_S3E_BRIDGE_OK) return map_status(status);
    result->r0 = (uint32_t)value;
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

REGUESTIMPORTStatus re_s3e_file_bridge_close_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result) {
    RES3EFileBridgeStatus status;
    (void)memory;
    if (!frame || !result || !valid_bridge((const RES3EFileBridge *)user))
        return RE_IMPORT_E_HANDLER;
    status = re_s3e_file_bridge_close((RES3EFileBridge *)user, frame->r);
    if (status != RE_S3E_BRIDGE_OK) return map_status(status);
    result->r0 = 0u;
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

int re_s3e_file_bridge_production_imports_authorized(void) { return 0; }
