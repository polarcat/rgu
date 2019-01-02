/* plotter.c: plotting utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#include <stdlib.h>
#include <pthread.h>

#define TAG "plotter"

#include <utils/log.h>

#include "gm.h"
#include "gl.h"
#include "draw.h"
#include "plotter.h"

#define COLOR_ATTR_SIZE (3 * 2)

static uint8_t async_;

static uint16_t w_;
static uint16_t h_;
static uint32_t max_points_;
static uint16_t max_lines_;

static float fps_;

static struct font *font0_;
static struct font *font1_;

static float *pcoords_;
static float *pcolors_;
static uint8_t *psizes_;
static struct points points_0_;
static struct points points_1_;
static struct points *points_front_;
static struct points *points_back_;

static pthread_mutex_t points_lock_ = PTHREAD_MUTEX_INITIALIZER;

#define lock_points(void) pthread_mutex_lock(&points_lock_)
#define unlock_points(void) pthread_mutex_unlock(&points_lock_)

static float *lcoords_;
static float *lcolors_;
static struct lines lines_0_;
static struct lines lines_1_;
static struct lines *lines_front_;
static struct lines *lines_back_;

struct text {
	char str[MAX_STRLEN];
	uint16_t len;
};

static struct text text_0_;
static struct text text_1_;

static struct text *text_front_;
static struct text *text_back_;

void plotter_close(void)
{
	if (!async_)
		return;

	points_0_.count = 0;
	free(points_0_.coords);
	free(points_0_.colors);
	free(points_0_.sizes);

	points_1_.count = 0;
	free(points_1_.coords);
	free(points_1_.colors);
	free(points_1_.sizes);

	pcoords_ = pcolors_ = NULL;
	psizes_ = NULL;

	lines_0_.count = 0;
	free(lines_0_.coords);
	free(lines_0_.colors);

	lines_1_.count = 0;
	free(lines_1_.coords);
	free(lines_1_.colors);

	lcoords_ = lcolors_ = NULL;
}

static uint8_t allocate_points(uint16_t w, uint16_t h)
{
	max_points_ = (w * h) / 4;
	size_t size = sizeof(float) * max_points_;

	if (!(points_0_.coords = (float *) calloc(1, size * 2))) { /* xy */
		ee("failed to allocate %zu bytes\n", size * 2);
		goto err;
	}

	ii("allocated %zu bytes for coords | %u points\n", size * 2, max_points_);

	if (!(points_0_.colors = (float *) calloc(1, size * 3))) { /* rgb */
		ee("failed to allocate %zu bytes\n", size * 3);
		goto err;
	}

	ii("allocated %zu bytes for colors\n", size * 3);

	if (!(points_0_.sizes = (uint8_t *) calloc(1, max_points_))) {
		ee("failed to allocate %u bytes\n", max_points_);
		goto err;
	}

	ii("allocated %zu bytes for point sizes\n", size);

	if (!(points_1_.coords = (float *) calloc(1, size * 2))) {
		ee("failed to allocate %zu bytes\n", size * 2);
		goto err;
	}

	if (!(points_1_.colors = (float *) calloc(1, size * 3))) {
		ee("failed to allocate %zu bytes\n", size * 3);
		goto err;
	}

	if (!(points_1_.sizes = (uint8_t *) calloc(1, max_points_))) {
		ee("failed to allocate %u bytes\n", max_points_);
		goto err;
	}

	points_front_ = &points_0_;
	points_back_ = &points_1_;

	points_front_->count = 0;
	points_back_->count = 0;

	pcoords_ = points_back_->coords;
	pcolors_ = points_back_->colors;
	psizes_ = points_back_->sizes;

	return 1;

err:
	plotter_close();
	return 0;
}

static uint8_t allocate_lines(uint16_t w, uint16_t h)
{
	max_lines_ = (w * h) / 4;
	size_t size = sizeof(float) * max_lines_;

	if (!(lines_0_.coords = (float *) calloc(1, size * 4))) { /* x0,y0 x1,y1 */
		ee("failed to allocate %zu bytes\n", size * 4);
		goto err;
	}

	ii("allocated %zu bytes for coords | %u lines\n", size * 4, max_lines_);

	if (!(lines_0_.colors = (float *) calloc(1, size * COLOR_ATTR_SIZE))) { /* rgb */
		ee("failed to allocate %zu bytes\n", size * COLOR_ATTR_SIZE);
		goto err;
	}

	ii("allocated %zu bytes for colors\n", size * COLOR_ATTR_SIZE);

	if (!(lines_1_.coords = (float *) calloc(1, size * 4))) {
		ee("failed to allocate %zu bytes\n", size * 4);
		goto err;
	}

	if (!(lines_1_.colors = (float *) calloc(1, size * COLOR_ATTR_SIZE))) {
		ee("failed to allocate %zu bytes\n", size * COLOR_ATTR_SIZE);
		goto err;
	}

	lines_front_ = &lines_0_;
	lines_back_ = &lines_1_;

	lines_front_->count = 0;
	lines_back_->count = 0;

	lcoords_ = lines_back_->coords;
	lcolors_ = lines_back_->colors;

	return 1;

err:
	plotter_close();
	return 0;
}

int plotter_open(struct font *f0, struct font *f1, uint16_t w, uint16_t h, uint8_t async)
{
	async_ = async;

	w_ = w;
	h_ = h;

	font0_ = f0;
	font1_ = f1;

	if (async_) {
		lock_points();

		plotter_close();

		if (!allocate_points(w, h))
			goto err;

		if (!allocate_lines(w, h))
			goto err;

		text_front_ = &text_0_;
		text_front_->len = 0;

		text_back_ = &text_1_;
		text_back_->len = 0;

		unlock_points();
	}

	return 0;

err:
	unlock_points();
	return -1;
}

static void swap_points(void)
{
	if (points_front_ == &points_0_) {
		points_front_ = &points_1_;
		points_back_ = &points_0_;
	} else {
		points_front_ = &points_0_;
		points_back_ = &points_1_;
	}

	size_t size = sizeof(float) * points_back_->count;

	memset(points_back_->coords, 0, size * 2);
	memset(points_back_->colors, 0, size * 3);
	points_back_->count = 0;

	pcoords_ = points_back_->coords;
	pcolors_ = points_back_->colors;
	psizes_ = points_back_->sizes;
}

static void swap_lines(void)
{
	if (lines_front_ == &lines_0_) {
		lines_front_ = &lines_1_;
		lines_back_ = &lines_0_;
	} else {
		lines_front_ = &lines_0_;
		lines_back_ = &lines_1_;
	}

	size_t size = sizeof(float) * lines_back_->count;

	memset(lines_back_->coords, 0, size * 4);
	memset(lines_back_->colors, 0, size * COLOR_ATTR_SIZE);
	lines_back_->count = 0;

	lcoords_ = lines_back_->coords;
	lcolors_ = lines_back_->colors;
}

void plotter_swap(void)
{
	if (!async_)
		return;

	lock_points();

	swap_points();
	swap_lines();

	if (text_front_ == &text_0_) {
		text_front_ = &text_1_;
		text_back_ = &text_0_;
	} else {
		text_front_ = &text_0_;
		text_back_ = &text_1_;
	}

	unlock_points();
}

void plotter_render(void)
{
	if (!async_)
		return;

	lock_points();

	if (points_front_->count)
		draw_points2d(points_front_);

	if (lines_front_->count)
		draw_lines2d(lines_front_);

	font_render(font1_, text_front_->str, text_front_->len,
	  gm_norm_x(0, w_), gm_norm_y(0, h_), 0xffffffff, 0x404040ff);

	unlock_points();

	count_fps(plotter_render, fps_);

	if (fps_ > 999.9)
		fps_ = 999.9;
}

void plot_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
  float r, float g, float b)
{
	if (!async_) {
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

	lock_points();

	if (lines_back_->count >= max_lines_) {
		lines_back_->count = max_lines_;
		ww("cannot draw more than %u lines\n", max_lines_);
	} else {
		*lcoords_++ = gm_norm_x(x0, w_);
		*lcoords_++ = gm_norm_y(y0, h_);
		*lcoords_++ = gm_norm_x(x1, w_);
		*lcoords_++ = gm_norm_y(y1, h_);

		*lcolors_++ = r;
		*lcolors_++ = g;
		*lcolors_++ = b;

		/* FIXME: shader expects color per vertex */

		*lcolors_++ = r;
		*lcolors_++ = g;
		*lcolors_++ = b;

		lines_back_->count++;

		dd("%u/%u lines\n", lines_back_->count, max_lines_);
	}

	unlock_points();
}

void plot_point(uint16_t x, uint16_t y, float r, float g, float b, uint8_t size)
{
	if (!async_) {
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

	lock_points();

	if (points_back_->count >= max_points_) {
		points_back_->count = max_points_;
		ww("cannot draw more than %u points\n", max_points_);
	} else {
		*pcoords_++ = gm_norm_x(x, w_);
		*pcoords_++ = gm_norm_y(y, h_);

		*pcolors_++ = r;
		*pcolors_++ = g;
		*pcolors_++ = b;

		*psizes_++ = size;

		points_back_->count++;

		dd("%u/%u points\n", points_back_->count, max_points_);
	}

	unlock_points();
}

void plot_text0_ndc(float x, float y, const char *str, size_t len,
  uint32_t fg, uint32_t bg)
{
	gl_disable_features();

	if (async_)
		font_render_async(font0_, str, len, x, y, fg, bg);
	else
		font_render(font0_, str, len, x, y, fg, bg);

	gl_enable_features();
}

void plot_text0(uint16_t x, uint16_t y, const char *str, size_t len,
  uint32_t fg, uint32_t bg)
{
	gl_disable_features();

	if (async_)
		font_render_async(font0_, str, len, gm_norm_x(x, w_), gm_norm_y(y, h_), fg, bg);
	else
		font_render(font0_, str, len, gm_norm_x(x, w_), gm_norm_y(y, h_), fg, bg);

	gl_enable_features();
}

void plot_text1(uint16_t x, uint16_t y, const char *str, size_t len,
  uint32_t fg, uint32_t bg)
{
	gl_disable_features();

	if (async_)
		font_render_async(font1_, str, len, gm_norm_x(x, w_), gm_norm_y(y, h_), fg, bg);
	else
		font_render(font1_, str, len, gm_norm_x(x, w_), gm_norm_y(y, h_), fg, bg);

	gl_enable_features();
}

void plot_text(const char *str, uint16_t len)
{
	if (!async_) {
		plot_text1(0, 0, str, len, 0xffffffff, 0x404040ff);
		return;
	}

	lock_points();

	len < MAX_STRLEN ? (text_back_->len = len) : (text_back_->len = MAX_STRLEN);
	memcpy(text_back_->str, str, text_back_->len);

	unlock_points();
}

float plot_getfps(void)
{
	return fps_;
}
