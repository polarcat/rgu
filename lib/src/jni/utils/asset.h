/* asset.h: assets manager
 *
 * Copyright (c) 2019 Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#pragma once

#include <stdint.h>

struct asset_info {
	const unsigned char *buf;
	uint32_t len;
	const void *asset;
	int fd;
};

void put_asset(struct asset_info *);
uint8_t get_asset(const char *path, struct asset_info *, const void *amgr);

struct image_info {
	void *image;
	int w;
	int h;
	int planes;
};

void put_image(struct image_info *);
uint8_t get_image(struct asset_info *, struct image_info *);
