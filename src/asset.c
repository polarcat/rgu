/* asset.c: assets manager
 *
 * Copyright (c) 2019 Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#ifdef ANDROID
#include <android/asset_manager_jni.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define STB_IMAGE_IMPLEMENTATION
#include <rgu/stb_image.h>
#include <rgu/log.h>
#include <rgu/asset.h>

static void unmap_asset(struct asset_info *ainfo)
{
	if (ainfo->buf) {
		munmap((void *) ainfo->buf, ainfo->len);
		ainfo->buf = NULL;
	}

	if (ainfo->fd >= 0) {
		close(ainfo->fd);
		ainfo->fd = -1;
	}
}

static uint8_t map_asset(const char *path, struct asset_info *ainfo)
{
	struct stat st;

	ainfo->buf = NULL;
	ainfo->fd = -1;

	if (stat(path, &st) < 0 || S_ISDIR(st.st_mode)) {
		ee("failed to get file size | path '%s'\n", path);
		return 0;
	}

	if ((ainfo->fd = open(path, O_RDWR)) < 0) {
		ee("failed to open file '%s'\n", path);
		return 0;
	}

	uint32_t prot = PROT_READ | PROT_WRITE;
	uint32_t flags = MAP_PRIVATE;

	ainfo->buf = (unsigned char *) mmap(NULL, st.st_size, prot, flags,
	  ainfo->fd, 0);

	if (!ainfo->buf) {
		ee("failed to map file %s\n", path);
		close(ainfo->fd);
		ainfo->fd = -1;
		return 0;
	}

	ainfo->len = st.st_size;
	return 1;
}


void put_asset(struct asset_info *ainfo)
{
#ifdef ANDROID
	AAsset_close((AAsset *) ainfo->asset);
#else
	unmap_asset(ainfo);
#endif
}

uint8_t get_asset(const char *path, struct asset_info *ainfo, const void *amgr)
{
#ifdef ANDROID
	if (amgr) {
		AAsset *asset = AAssetManager_open((AAssetManager *) amgr,
		  path, AASSET_MODE_STREAMING);

		if (asset == NULL) {
			ee("failed to open asset '%s'\n", path);
			return 0;
		}

		ainfo->buf = AAsset_getBuffer(asset);
		ainfo->len = AAsset_getLength(asset);
		ainfo->asset = asset;

		return 1;
	}
#endif

	return map_asset(path, ainfo);
}

void put_image(struct image_info *iinfo)
{
	stbi_image_free(iinfo->image);
	iinfo->image = NULL;
}

uint8_t get_image(struct asset_info *ainfo, struct image_info *iinfo)
{
	iinfo->image = stbi_load_from_memory(ainfo->buf, ainfo->len,
	  &iinfo->w, &iinfo->h, &iinfo->planes, STBI_default);

	if (!iinfo->image)
		return 0;

	return 1;
}
