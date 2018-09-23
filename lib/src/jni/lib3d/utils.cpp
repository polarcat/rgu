/* utils.c: common utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <stdlib.h>

/* color format #ffffff */
void utils_str2rgb(const char *color, uint8_t *r, uint8_t *g, uint8_t *b)
{
	char plane[5] = { '0', 'x', 0, 0, 0, };

	plane[2] = color[0];
	plane[3] = color[1];
	*r = strtol(plane, NULL, 16);

	plane[2] = color[2];
	plane[3] = color[3];
	*g = strtol(plane, NULL, 16);

	plane[2] = color[4];
	plane[3] = color[5];
	*b = strtol(plane, NULL, 16);
}

void utils_fill2d(uint32_t *buf, uint32_t *end, uint32_t color)
{
	while (buf < end) {
		uint8_t *ptr = (uint8_t *) buf++;
		*ptr++ = color >> 24 & 0xff;
		*ptr++ = color >> 16 & 0xff;
		*ptr++ = color >> 8 & 0xff;
		*ptr++ = color & 0xff;
	}
}
