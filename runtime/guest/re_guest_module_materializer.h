#ifndef RE_GUEST_MODULE_MATERIALIZER_H
#define RE_GUEST_MODULE_MATERIALIZER_H
#include "re_guest_module_manifest.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RE_GUEST_MATERIALIZER_MAX_TOTAL_BYTES (256u * 1024u * 1024u)
typedef struct {
    void *(*open)(void *user, const char *relative_path);
    int64_t (*length)(void *user, void *stream);
    int64_t (*read)(void *user, void *stream, void *destination, uint64_t bytes);
    void (*close)(void *user, void *stream);
} REGUESTMODULESOURCE;
typedef struct {
    uint8_t *bytes;
    size_t size;
} REGUESTMATERIALIZEDMODULE;
typedef struct {
    size_t module_count;
    REGUESTMATERIALIZEDMODULE modules[RE_GUEST_MANIFEST_MAX_MODULES];
} REGUESTMATERIALIZEDSET;
typedef enum {
    RE_GUEST_MATERIALIZE_OK = 0,
    RE_GUEST_MATERIALIZE_E_ARGUMENT = -840,
    RE_GUEST_MATERIALIZE_E_MANIFEST = -841,
    RE_GUEST_MATERIALIZE_E_OPEN = -842,
    RE_GUEST_MATERIALIZE_E_LENGTH = -843,
    RE_GUEST_MATERIALIZE_E_LIMIT = -844,
    RE_GUEST_MATERIALIZE_E_READ = -845,
    RE_GUEST_MATERIALIZE_E_MEMORY = -846,
    RE_GUEST_MATERIALIZE_E_TOTAL_LIMIT = -847
} REGUESTMATERIALIZESTATUS;
/* Output must be zero-initialized and empty. On failure it remains unchanged.
 * The source callbacks and user context must remain valid for the call.
 * All streams are closed and all temporary allocations freed on every failure.
 */
REGUESTMATERIALIZESTATUS re_guest_module_materialize(
    const REGUESTMODULEMANIFEST *manifest, const REGUESTMODULESOURCE *source,
    void *source_user, REGUESTMATERIALIZEDSET *out);
void re_guest_module_materialized_free(REGUESTMATERIALIZEDSET *set);
#ifdef __cplusplus
}
#endif
#endif
