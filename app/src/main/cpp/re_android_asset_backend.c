#include "re_android_asset_backend.h"

#include <limits.h>
#include <stdint.h>
#include <android/asset_manager.h>

static void *asset_open(void *user, const char *path, const char *mode) {
    REAndroidAssetBackend *state = (REAndroidAssetBackend *)user;
    AAsset *asset;
    int android_mode;
    if (!state || !state->manager || !path || !mode) return NULL;
    if (mode[0] != 'r' || mode[1] != 'b' || mode[2] != '\0') return NULL;
    android_mode = AASSET_MODE_STREAMING;
    asset = AAssetManager_open(state->manager, path, android_mode);
    return (void *)asset;
}

static int64_t asset_read(void *user, void *stream, void *destination,
                          uint64_t requested) {
    uint64_t total = 0;
    AAsset *asset = (AAsset *)stream;
    (void)user;
    if (!asset || (!destination && requested != 0)) return -1;
    while (total < requested) {
        uint64_t left = requested - total;
        size_t chunk = left > (uint64_t)INT_MAX ? (size_t)INT_MAX : (size_t)left;
        int got = AAsset_read(asset, (uint8_t *)destination + (size_t)total, chunk);
        if (got < 0) return total ? (int64_t)total : -1;
        if (got == 0) break; /* EOF: return the bytes already transferred. */
        if ((size_t)got > chunk) return total ? (int64_t)total : -1;
        total += (uint64_t)got;
        /* A short positive read is valid; ask again until full or EOF. */
    }
    return (int64_t)total;
}

static int32_t asset_get_char(void *user, void *stream) {
    uint8_t byte = 0;
    int got;
    (void)user;
    if (!stream) return -2;
    got = AAsset_read((AAsset *)stream, &byte, 1);
    if (got == 1) return (int32_t)byte;
    if (got == 0) return -1;
    return -2;
}

static void asset_close(void *user, void *stream) {
    (void)user;
    if (stream) AAsset_close((AAsset *)stream);
}

REFileStatus re_android_asset_backend_init(REAndroidAssetBackend *state,
                                           AAssetManager *manager,
                                           REFileBackend *out_backend) {
    if (!state || !manager || !out_backend) return RE_FILE_E_ARGUMENT;
    state->manager = manager;
    out_backend->open = asset_open;
    out_backend->read = asset_read;
    out_backend->get_char = asset_get_char;
    out_backend->close = asset_close;
    return RE_FILE_OK;
}
