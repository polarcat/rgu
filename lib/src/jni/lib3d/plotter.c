/* plotter.c: plotting utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#include <stdlib.h>

#define TAG "plotter"

#include <utils/log.h>

#include "gm.h"
#include "gl.h"
#include "draw.h"
#include "plotter.h"

static uint16_t w_;
static uint16_t h_;
static struct font *font0_;
static struct font *font1_;

int init_plotter(struct font *f0, struct font *f1, uint16_t w, uint16_t h)
{
	w_ = w;
	h_ = h;

	font0_ = f0;
	font1_ = f1;

	return 0;
}

void plot_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
  float r, float g, float b)
{
	float coords[4] = { gm_norm_x(x0, w_), gm_norm_y(y0, h_),
		gm_norm_x(x1, w_), gm_norm_y(y1, h_),
	};

	float colors[6] = { r, g, b, r, g, b, };
	struct lines line = {
		.count = 1,
		.coords = coords,
		.colors = colors,
	};

	draw_lines2d(&line);
	return;
}

void plot_point(uint16_t x, uint16_t y, float r, float g, float b, uint8_t size)
{
	float coords[2] = { gm_norm_x(x, w_), gm_norm_y(y, h_), };
	float colors[3] = { r, g, b, };
	uint8_t sizes[1] = { size, };
	struct points point = {
		.count = 1,
		.coords = coords,
		.colors = colors,
		.sizes = sizes,
	};

	draw_points2d(&point);
	return;
}

void plot_text0_ndc(float x, float y, const char *str, size_t len,
  uint32_t fg, uint32_t bg)
{
	gl_disable_features();
	draw_text(font0_, str, len, x, y, fg, bg, NULL);
	gl_enable_features();
}

void plot_text0(uint16_t x, uint16_t y, const char *str, size_t len,
  uint32_t fg, uint32_t bg)
{
	gl_disable_features();
	draw_text(font0_, str, len, gm_norm_x(x, w_), gm_norm_y(y, h_), fg, bg,
	  NULL);
	gl_enable_features();
}

void plot_text1_ndc(float x, float y, const char *str, size_t len,
  uint32_t fg, uint32_t bg)
{
	gl_disable_features();
	draw_text(font1_, str, len, x, y, fg, bg, NULL);
	gl_enable_features();
}

void plot_text1(uint16_t x, uint16_t y, const char *str, size_t len,
  uint32_t fg, uint32_t bg)
{
	gl_disable_features();
	draw_text(font1_, str, len, gm_norm_x(x, w_), gm_norm_y(y, h_), fg, bg,
	  NULL);
	gl_enable_features();
}

void plot_text(const char *str, uint16_t len)
{
	plot_text1(0, 0, str, len, 0xffffffff, 0x404040ff);
}
