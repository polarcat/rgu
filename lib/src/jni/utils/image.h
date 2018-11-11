/* image.h: image manipulation tools
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <stdint.h>
#include <GLES2/gl2.h>

struct image {
	uint8_t *data;
	uint16_t w;
	uint16_t h;
	GLuint format;
};

uint8_t readpng(const char *path, struct image *image);
uint8_t readjpg(const char *path, struct image *image);

void writepng(const char *path, const uint8_t *buf, uint16_t w, uint16_t h);

void fillrect(uint32_t *buf, uint32_t *end, uint32_t color);
