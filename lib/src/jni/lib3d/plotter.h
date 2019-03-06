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

void plotter_close(void);
int plotter_open(struct font *f0, struct font *f1, uint16_t w, uint16_t h,
  uint8_t async);
void plotter_swap(void);
void plotter_render(void);

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

char *plot_get_text_buffer(size_t *len);

float plot_getfps(void);

#define MAX_STRLEN 1024

#define plot_status(fmt, arg...) {\
	char str[MAX_STRLEN];\
	float fps = plot_getfps();\
	uint16_t pos = snprintf(str, MAX_STRLEN, " %03.1f fps | " fmt, fps, ##arg);\
	str[pos] = '\0';\
	plot_text(str, pos);\
}
