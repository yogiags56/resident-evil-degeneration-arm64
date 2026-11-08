\
#ifndef RE_FILE_ADAPTER_H
#define RE_FILE_ADAPTER_H

#include <stddef.h>
#include <stdint.h>
#include "re_guest_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_FILE_ADAPTER_MAX_HANDLES 32u
#define RE_FILE_ADAPTER_MAX_PATH 256u

typedef enum {
    RE_FILE_OK = 0,
    RE_FILE_E_ARGUMENT = -200,
    RE_FILE_E_GUEST_MEMORY = -201,
    RE_FILE_E_PATH = -202,
    RE_FILE_E_MODE = -203,
    RE_FILE_E_NOT_FOUND = -204,
    RE_FILE_E_CAPACITY = -205,
    RE_FILE_E_BAD_HANDLE = -206,
    RE_FILE_E_BACKEND = -207,
    RE_FILE_E_OVERFLOW = -208
} REFileStatus;

/*
 * Backend contract:
 * - open receives a normalized, relative UTF-8 asset path and mode "rb".
 * - read returns bytes transferred, 0 for EOF, or a negative backend error.
 * - get_char returns 0..255, -1 for EOF, or <= -2 for backend error.
 * - close releases the opaque backend stream.
 * The backend owns Android AssetManager/private-storage mapping. This module
 * deliberately has no direct filesystem or Android dependency.
 */
typedef struct {
    void *(*open)(void *user, const char *relative_path, const char *mode);
    int64_t (*read)(void *user, void *stream, void *destination, uint64_t bytes);
    int32_t (*get_char)(void *user, void *stream);
    void (*close)(void *user, void *stream);
} REFileBackend;

typedef struct {
    void *stream;
    uint16_t generation;
    uint8_t active;
    uint8_t reserved;
} REFileHandleSlot;

typedef struct {
    REFileBackend backend;
    void *backend_user;
    REFileHandleSlot slots[RE_FILE_ADAPTER_MAX_HANDLES];
    uint16_t next_generation;
} REFileAdapter;

/* Translate a 32-bit guest virtual address to a checked host span. */
REFileStatus re_file_guest_span(const REGUESTMEM *memory,
                                uint32_t guest_address,
                                size_t length,
                                uint8_t **host_pointer);

/* Normalize a guest path to a safe relative path. Absolute paths and traversal
 * components are rejected; repeated separators and "." components are folded.
 */
REFileStatus re_file_normalize_path(const char *input,
                                    char output[RE_FILE_ADAPTER_MAX_PATH]);

REFileStatus re_file_adapter_init(REFileAdapter *adapter,
                                  const REFileBackend *backend,
                                  void *backend_user);

/* Open only supports the observed read-only "rb" mode. */
REFileStatus re_file_adapter_open(REFileAdapter *adapter,
                                  const REGUESTMEM *memory,
                                  uint32_t guest_path,
                                  uint32_t guest_mode,
                                  uint32_t *guest_handle);

/* Byte-oriented host adapter primitives. They intentionally do not claim the
 * original s3eFileRead item-count return convention. */
REFileStatus re_file_adapter_read_bytes(REFileAdapter *adapter,
                                        const REGUESTMEM *memory,
                                        uint32_t guest_handle,
                                        uint32_t guest_destination,
                                        uint64_t byte_count,
                                        uint64_t *bytes_read);

REFileStatus re_file_adapter_get_char(REFileAdapter *adapter,
                                      uint32_t guest_handle,
                                      int32_t *value);

REFileStatus re_file_adapter_close(REFileAdapter *adapter,
                                   uint32_t guest_handle);

void re_file_adapter_close_all(REFileAdapter *adapter);

#ifdef __cplusplus
}
#endif
#endif
