#ifndef STAGE88_MOCK_ASSET_MANAGER_H
#define STAGE88_MOCK_ASSET_MANAGER_H
#include <stddef.h>
typedef struct AAssetManager AAssetManager;
typedef struct AAsset AAsset;
#define AASSET_MODE_STREAMING 2
AAsset *AAssetManager_open(AAssetManager *, const char *, int);
int AAsset_read(AAsset *, void *, size_t);
void AAsset_close(AAsset *);
#endif
