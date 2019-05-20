/* color.c: color manipulation helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <stddef.h>
#include <math.h>

#include <rgu/color.h>

void rgb2hsi(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *i)
{
	*i = (r + g + b) / 3 / 255.;

	uint8_t max;

	if ((r > g || r == g) && r > b)
		max = r;
	else if ((g > r || g == r) && g > b)
		max = g;
	else
		max = b;

	uint8_t min;

	if ((r < g || r == g) && r < b)
		min = r;
	else if ((g < r || g == r) && g < b)
		min = g;
	else
		min = b;

	float c = (max - min) / 255.;

	if (c == 0) {
		*h = *s = 0;
		return;
	} else if (max == r) {
		*h = (g - b) / 255. / c;
		*h = fmodf(*h, 6);
	} else if (max == g) {
		*h = (b - r) / 255. / c + 2;
	} else {
		*h = (r - g) / 255. / c + 4;
	}

	*h *= 60;

	if (*h <= 0)
		*h += 360.;

	if (*i == 0)
		*s = 0;
	else
		*s = (1 - min / 255. / *i) * 100.;

	*i *= 100.;
}

uint8_t rgb2hsv(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v)
{
	uint8_t max;

	if ((r > g || r == g) && r > b)
		max = r;
	else if ((g > r || g == r) && g > b)
		max = g;
	else
		max = b;

	*v = max / 255.;

	uint8_t min;

	if ((r < g || r == g) && r < b)
		min = r;
	else if ((g < r || g == r) && g < b)
		min = g;
	else
		min = b;

	float c = (max - min) / 255.;

	if (c == 0) {
		*h = *s = 0;
		return 0;
	} else if (max == r) {
		*h = (g - b) / 255. / c;
		*h = fmodf(*h, 6);
	} else if (max == g) {
		*h = (b - r) / 255. / c + 2;
	} else {
		*h = (r - g) / 255. / c + 4;
	}

	*h *= 60;

	if (*h < 0)
		*h += 360.;

	if (max == 0)
		*s = 0;
	else
		*s = c / *v * 100.;

	*v *= 100.;

	return 1;
}

/* color format #ffffff */
void str2rgb(const char *color, uint8_t *r, uint8_t *g, uint8_t *b)
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
