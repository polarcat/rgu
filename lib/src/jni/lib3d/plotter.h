/* plotter.h: plotting utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

#include <lib3d/font.h>
#include <utils/time.h>

float fps_;

int init_plotter(struct font *f0, struct font *f1, uint16_t w, uint16_t h);

void plot_point(uint16_t x, uint16_t y, float r, float g, float b, uint8_t size);
void plot_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
  float r, float g, float b);
void plot_text0(uint16_t x, uint16_t y, const char *str, size_t len,
  uint32_t fg, uint32_t bg);
void plot_text0_ndc(float x, float y, const char *str, size_t len,
  uint32_t fg, uint32_t bg);
void plot_text1(uint16_t x, uint16_t y, const char *str, size_t len,
  uint32_t fg, uint32_t bg);
void plot_text1_ndc(float x, float y, const char *str, size_t len,
  uint32_t fg, uint32_t bg);
void plot_text(const char *str, uint16_t len);

#define MAX_STRLEN 1024

#define plot_status(fmt, arg...) {\
	char str[MAX_STRLEN];\
	uint16_t pos = snprintf(str, MAX_STRLEN, " %03.1f fps | " fmt, fps_, ##arg);\
	str[pos] = '\0';\
	plot_text(str, pos);\
}
