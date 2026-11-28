#ifndef RE_GUEST_MODULE_MANIFEST_H
#define RE_GUEST_MODULE_MANIFEST_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RE_GUEST_MANIFEST_MAX_BYTES 16384u
#define RE_GUEST_MANIFEST_MAX_MODULES 32u
#define RE_GUEST_MANIFEST_SONAME_CAP 128u
#define RE_GUEST_MANIFEST_PATH_CAP 256u
#define RE_GUEST_MANIFEST_MAX_MODULE_BYTES (128u * 1024u * 1024u)
typedef struct {
    char soname[RE_GUEST_MANIFEST_SONAME_CAP];
    char asset_path[RE_GUEST_MANIFEST_PATH_CAP];
    uint32_t max_bytes;
} REGUESTMODULEMANIFESTENTRY;
typedef struct {
    uint32_t version;
    size_t module_count;
    size_t root_module_index;
    REGUESTMODULEMANIFESTENTRY modules[RE_GUEST_MANIFEST_MAX_MODULES];
} REGUESTMODULEMANIFEST;
typedef enum {
    RE_GUEST_MANIFEST_OK = 0,
    RE_GUEST_MANIFEST_E_ARGUMENT = -820,
    RE_GUEST_MANIFEST_E_SIZE = -821,
    RE_GUEST_MANIFEST_E_SYNTAX = -822,
    RE_GUEST_MANIFEST_E_VERSION = -823,
    RE_GUEST_MANIFEST_E_CAPACITY = -824,
    RE_GUEST_MANIFEST_E_NAME = -825,
    RE_GUEST_MANIFEST_E_PATH = -826,
    RE_GUEST_MANIFEST_E_LIMIT = -827,
    RE_GUEST_MANIFEST_E_DUPLICATE = -828,
    RE_GUEST_MANIFEST_E_ROOT = -829
} REGUESTMODULEMANIFESTStatus;
/* Strict UTF-8/ASCII-compatible v1 manifest parser. Grammar:
 * RE-GUEST-MODULES 1
 * root <soname>
 * module <soname> <relative-asset-path> <max-bytes>
 * Blank lines and full-line # comments are allowed. Parsing is transactional.
 */
REGUESTMODULEMANIFESTStatus re_guest_module_manifest_parse(
    const char *text, size_t length, REGUESTMODULEMANIFEST *manifest,
    size_t *failed_line);
#ifdef __cplusplus
}
#endif
#endif
