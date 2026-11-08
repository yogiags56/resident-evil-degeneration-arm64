#ifndef RE_ANDROID_ASSET_BACKEND_H
#define RE_ANDROID_ASSET_BACKEND_H

#include "re_file_adapter.h"

/* Android NDK AAssetManager backend. The manager is borrowed, never owned.
 * It must remain valid until re_file_adapter_close_all() has completed and
 * the backend is no longer used. Create/use/destroy on a coordinated runtime
 * thread unless the caller provides external synchronization.
 */
typedef struct AAssetManager AAssetManager;
typedef struct {
    AAssetManager *manager;
} REAndroidAssetBackend;

REFileStatus re_android_asset_backend_init(REAndroidAssetBackend *state,
                                           AAssetManager *manager,
                                           REFileBackend *out_backend);

#endif
