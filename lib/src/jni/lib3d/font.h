/* font.h: font rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>

struct font_info {
	const char *path;
	const uint32_t *codes;
	uint8_t codes_num;
	float font_size;
};

struct bitmap {
	uint8_t *data;
	uint32_t size;
	uint16_t w;
	uint16_t h;
};

struct font;

void close_font(struct font **);
struct font *open_font(const char *path, float font_size, const uint32_t *codes,
  uint8_t codes_num, void *assets);
void draw_text(struct font *, const char *str, uint16_t len, float x, float y,
  uint32_t fg, uint32_t bg, struct bitmap *bmp);
const uint32_t *default_codes(uint8_t *codes_num);
