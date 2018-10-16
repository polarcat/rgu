/* color.h: color manipulation helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>

void rgb2hsi(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *i);
uint8_t rgb2hsv(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v);

void str2rgb(const char *color, uint8_t *r, uint8_t *g, uint8_t *b);
