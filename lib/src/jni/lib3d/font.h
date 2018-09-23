/* font.h: font rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>

struct text {
	const char *str;
	uint16_t len;
	uint8_t normxy:1;
	uint16_t x;
	uint16_t y;
	uint32_t fg;
	uint32_t bg;
};

struct font_info {
	uint32_t prog;
	int a_pos;
	int a_uv;
	int u_tex;
	int fd;
	const uint8_t *data_buf;
	size_t data_len;
	void *font;
	int font_ascent;
	int font_descent;
	float font_scale;
	uint16_t font_size;
#ifdef ANDROID
	void *asset_manager;
	void *asset;
#endif
};

void font_close(struct font_info *info);
void font_open(struct font_info *info, float size, const char *path);
void font_render(const struct font_info *info, const struct text *text);
