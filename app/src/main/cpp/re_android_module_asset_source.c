#include "re_android_module_asset_source.h"
#include <android/asset_manager.h>
#include <limits.h>

static void *module_open(void *user, const char *path) {
    const REAndroidAssetBackend *state =
        (const REAndroidAssetBackend *)user;
    if (!state || !state->manager || !path) return NULL;
    return (void *)AAssetManager_open(state->manager, path, AASSET_MODE_STREAMING);
}
static int64_t module_length(void *user, void *stream) {
    off_t length;
    (void)user;
    if (!stream) return -1;
    length = AAsset_getLength((AAsset *)stream);
    if (length < 0) return -1;
    return (int64_t)length;
}
static int64_t module_read(void *user, void *stream, void *destination,
                           uint64_t requested) {
    size_t chunk;
    int got;
    (void)user;
    if (!stream || (!destination && requested)) return -1;
    if (requested > (uint64_t)INT_MAX) chunk = (size_t)INT_MAX;
    else chunk = (size_t)requested;
    got = AAsset_read((AAsset *)stream, destination, chunk);
    return (int64_t)got;
}
static void module_close(void *user, void *stream) {
    (void)user;
    if (stream) AAsset_close((AAsset *)stream);
}
REFileStatus re_android_module_asset_source_init(
    const REAndroidAssetBackend *android_backend,
    REGUESTMODULESOURCE *out_source) {
    if (!android_backend || !android_backend->manager || !out_source)
        return RE_FILE_E_ARGUMENT;
    out_source->open = module_open;
    out_source->length = module_length;
    out_source->read = module_read;
    out_source->close = module_close;
    return RE_FILE_OK;
}
