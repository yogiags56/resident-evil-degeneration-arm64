#ifndef RE_ANDROID_MODULE_ASSET_SOURCE_H
#define RE_ANDROID_MODULE_ASSET_SOURCE_H
#include "re_android_asset_backend.h"
#include "re_guest_module_materializer.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Creates read-only callbacks over an already initialized AssetManager backend.
 * The backend manager must outlive every materialization call.
 */
REFileStatus re_android_module_asset_source_init(
    const REAndroidAssetBackend *android_backend,
    REGUESTMODULESOURCE *out_source);
#ifdef __cplusplus
}
#endif
#endif
