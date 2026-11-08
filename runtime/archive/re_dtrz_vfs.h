#ifndef RE_DTRZ_VFS_H
#define RE_DTRZ_VFS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RE_DTRZ_MAGIC "DTRZ"
#define RE_DTRZ_HEADER_SIZE 9u
#define RE_DTRZ_ATTRIBUTE_SIZE 6u
#define RE_DTRZ_LOCATION_HEADER_SIZE 4u
#define RE_DTRZ_LOCATION_RECORD_SIZE 16u
#define RE_DTRZ_METHOD_STORED 0x00000100u
#define RE_DTRZ_METHOD_LZMA_ALONE 0x00000200u
#define RE_DTRZ_PATH_CAPACITY 512u
#define RE_DTRZ_MAX_DECODED_ENTRY_SIZE (64u * 1024u * 1024u)

typedef enum re_dtrz_status {
    RE_DTRZ_OK = 0,
    RE_DTRZ_E_ARGUMENT = -1,
    RE_DTRZ_E_FORMAT = -2,
    RE_DTRZ_E_BOUNDS = -3,
    RE_DTRZ_E_MEMORY = -4,
    RE_DTRZ_E_NOT_FOUND = -5,
    RE_DTRZ_E_UNSUPPORTED = -6,
    RE_DTRZ_E_DECODE = -7,
    RE_DTRZ_E_CLOSED = -8
} re_dtrz_status;

typedef struct re_dtrz_entry {
    char *path;
    uint32_t index;
    uint32_t folder_index;
    uint32_t file_index;
    uint32_t flags;
    uint64_t offset;
    uint64_t packed_size;
    uint32_t decoded_size_a;
    uint32_t decoded_size_b;
    uint32_t method;
} re_dtrz_entry;

typedef struct re_dtrz_archive {
    const uint8_t *bytes;
    size_t byte_count;
    uint32_t file_count;
    uint32_t folder_count;
    uint32_t location_header_unknown;
    uint64_t attribute_offset;
    uint64_t location_header_offset;
    uint64_t location_table_offset;
    uint64_t payload_offset;
    re_dtrz_entry *entries;
} re_dtrz_archive;

/* Optional codec callback. The callback must write no more than dst_capacity
 * bytes and set *written on success. The stored method is handled internally. */
typedef re_dtrz_status (*re_dtrz_decode_fn)(
    void *context, uint32_t method,
    const uint8_t *src, size_t src_size,
    uint8_t *dst, size_t dst_capacity, size_t *written);

typedef struct re_dtrz_vfs {
    const re_dtrz_archive *archive;
    re_dtrz_decode_fn decode;
    void *decode_context;
} re_dtrz_vfs;

typedef struct re_dtrz_file {
    uint8_t *decoded;
    size_t size;
    size_t cursor;
    int is_open;
} re_dtrz_file;

re_dtrz_status re_dtrz_archive_parse(
    re_dtrz_archive *out, const uint8_t *bytes, size_t byte_count);
void re_dtrz_archive_destroy(re_dtrz_archive *archive);
const re_dtrz_entry *re_dtrz_find(const re_dtrz_archive *archive, const char *path);
re_dtrz_status re_dtrz_vfs_init(
    re_dtrz_vfs *vfs, const re_dtrz_archive *archive,
    re_dtrz_decode_fn decode, void *decode_context);
re_dtrz_status re_dtrz_open(
    const re_dtrz_vfs *vfs, const char *path, re_dtrz_file *out_file);
re_dtrz_status re_dtrz_read(
    re_dtrz_file *file, void *destination, size_t requested, size_t *read_count);
re_dtrz_status re_dtrz_seek(re_dtrz_file *file, uint64_t offset);
re_dtrz_status re_dtrz_tell(const re_dtrz_file *file, uint64_t *offset);
void re_dtrz_close(re_dtrz_file *file);
const char *re_dtrz_status_string(re_dtrz_status status);

#ifdef __cplusplus
}
#endif
#endif
