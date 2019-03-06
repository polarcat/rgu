/* color.h: color manipulation helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>

union color_rgb {
	float data[3];
	struct {
		float r;
		float g;
		float b;
	};
};

union color_rgba {
	float data[4];
	struct {
		float r;
		float g;
		float b;
		float a;
	};
};

void rgb2hsi(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *i);
uint8_t rgb2hsv(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v);

void str2rgb(const char *color, uint8_t *r, uint8_t *g, uint8_t *b);
