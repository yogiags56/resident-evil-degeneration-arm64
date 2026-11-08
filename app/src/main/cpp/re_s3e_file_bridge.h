#ifndef RE_S3E_FILE_BRIDGE_H
#define RE_S3E_FILE_BRIDGE_H

#include <stdint.h>
#include "re_file_adapter.h"
#include "re_guest_imports.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Stage 98: backend-backed, host-testable guest file operations.
 *
 * This module deliberately does not register production imports or decide
 * undocumented guest error values. Operations return a host-side status;
 * successful API results are written separately. This keeps uncertain
 * Marmalade error/partial-read behavior out of guest r0 until verified.
 */
typedef struct {
    REFileAdapter *adapter;
} RES3EFileBridge;

typedef enum {
    RE_S3E_BRIDGE_OK = 0,
    RE_S3E_BRIDGE_E_ARGUMENT = -300,
    RE_S3E_BRIDGE_E_OPEN = -301,
    RE_S3E_BRIDGE_E_READ = -302,
    RE_S3E_BRIDGE_E_GET_CHAR = -303,
    RE_S3E_BRIDGE_E_CLOSE = -304,
    RE_S3E_BRIDGE_E_BAD_HANDLE = -305
} RES3EFileBridgeStatus;

RES3EFileBridgeStatus re_s3e_file_bridge_init(
    RES3EFileBridge *bridge, REFileAdapter *adapter);

/* Guest API argument words are AAPCS32 r0-r3. */
RES3EFileBridgeStatus re_s3e_file_bridge_open(
    RES3EFileBridge *bridge, const REGUESTMEM *memory,
    const uint32_t args[4], uint32_t *guest_handle);

RES3EFileBridgeStatus re_s3e_file_bridge_read(
    RES3EFileBridge *bridge, const REGUESTMEM *memory,
    const uint32_t args[4], uint32_t *items_read);

RES3EFileBridgeStatus re_s3e_file_bridge_get_char(
    RES3EFileBridge *bridge, const uint32_t args[4],
    int32_t *guest_character);

RES3EFileBridgeStatus re_s3e_file_bridge_close(
    RES3EFileBridge *bridge, const uint32_t args[4]);

/* These are host operation adapters, not registered guest import handlers. */
REGUESTIMPORTStatus re_s3e_file_bridge_open_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result);
REGUESTIMPORTStatus re_s3e_file_bridge_read_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result);
REGUESTIMPORTStatus re_s3e_file_bridge_get_char_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result);
REGUESTIMPORTStatus re_s3e_file_bridge_close_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result);

/* Stage 98 intentionally keeps this false pending error/partial-read closure. */
int re_s3e_file_bridge_production_imports_authorized(void);

#ifdef __cplusplus
}
#endif
#endif
