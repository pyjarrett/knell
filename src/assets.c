#include "assets.h"

#include "kn.h"
#include "log.h"
#include <string.h>

#include <spa_fu/spa_fu.h>

#define MAX_ASSET_PATH_LENGTH 256
static char assetsRoot[MAX_ASSET_PATH_LENGTH+1];
static uint32_t assetsRootLength = 0;
static uint32_t LogSysAssets;

/**
 * Initial the asset system with the top level directory where assets should
 * be found.
 */
void assets_init(const char* newAssetsRoot)
{
	if (strlen(newAssetsRoot) >= MAX_ASSET_PATH_LENGTH) {
		KN_FATAL_ERROR("Asset path root is too long.  Cannot initialize asset path with %s", newAssetsRoot);
	}
	strcpy(assetsRoot, newAssetsRoot);
	assetsRootLength = strlen(assetsRoot);

	if (!SPA_IsDir(assetsRoot)) {
		KN_FATAL_ERROR("Assets root directory doesn't exist: %s", assetsRoot);
	}

	LOG_RegisterSystem(&LogSysAssets, "Assets", KN_LOG_TRACE);

	KN_TRACE(LogSysAssets, "Assets initialized with root at: '%s'", assetsRoot);
}

/**
 * Gets the path for accessing a specific resource within the assets system.
 */
bool assets_pathFor(const char* assetName, char* buffer, uint32_t bufferSize)
{
	if (assetsRootLength == 0) {
		KN_ERROR(LogSysAssets, "Asset system not initialized, cannot get path for %s", assetName);
		return false;
	}

	// Output buffer cannot hold root + '/' + assetName + '\0'.
	if (assetsRootLength + 1 + strlen(assetName) + 1 >= bufferSize) {
		return false;
	}

	memcpy(buffer, assetsRoot, assetsRootLength);
	buffer[assetsRootLength] = '/';
	strcpy(buffer + assetsRootLength + 1, assetName);

	KN_TRACE(LogSysAssets, "Resolved asset path '%s' -> '%s'", assetName, buffer);

	return true;
}
