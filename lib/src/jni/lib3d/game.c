/* game.c: game renderer
 *
 * Copyright (c) 2019, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define TAG "game"

#include <utils/log.h>
#include <utils/utils.h>
#include <utils/image.h>
#include <utils/time.h>

#include "gm.h"
#include "gl.h"
#include "bg.h"
#include "font.h"
#include "draw.h"
#include "plotter.h"

#ifdef HAVE_GAME
void handle_setup(void *assets);
void handle_frame(void);
void handle_touch(uint16_t x, uint16_t y);
#else
#define handle_setup(a) ;
#define handle_frame() ;
#define handle_touch(x, y) ;
#endif

static uint8_t resize_;

static pthread_mutex_t resize_lock_ = PTHREAD_MUTEX_INITIALIZER;

#define lock_resize(void) pthread_mutex_lock(&resize_lock_)
#define unlock_resize(void) pthread_mutex_unlock(&resize_lock_)

static uint8_t touch_;

static pthread_mutex_t touch_lock_ = PTHREAD_MUTEX_INITIALIZER;

#define lock_touch(void) pthread_mutex_lock(&touch_lock_)
#define unlock_touch(void) pthread_mutex_unlock(&touch_lock_)

static struct font *font0_;
static struct font *font1_;

static uint16_t img_w_;
static uint16_t img_h_;
static uint8_t *img_;

static uint16_t touch_x_;
static uint16_t touch_y_;

void game_resize(uint16_t w, uint16_t h)
{
	lock_resize();

	img_w_ = w;
	img_h_ = h;
	resize_ = 1;

	unlock_resize();
}

void game_touch(uint16_t x, uint16_t y)
{
	lock_touch();

	touch_x_ = x;
	touch_y_ = y;

	unlock_touch();

	handle_touch(touch_x_, touch_y_);
}

void game_open(struct font *f0, struct font *f1, void *assets)
{
	GLint wh[4];

	glGetIntegerv(GL_VIEWPORT, wh);

	img_w_ = wh[2];
	img_h_ = wh[3];

	font0_ = f0;
	font1_ = f1;

	draw_init(); /* for debug drawing */
	gm_open(img_w_ / 4);
	plotter_open(f0, f1, img_w_, img_h_, 0);

	handle_setup(assets);

	ii("init ok\n");
}

void game_close(void)
{
	gm_close();
}

static inline void draw_axis(void)
{
	draw_line2d(-1, 0, 1, 0, .8, .8, .8);
	draw_line2d(0, -1, 0, 1, .8, .8, .8);

	for (float i = .1; i < 1.;) {
		draw_point2d(0, i, .8, .8, .8, 8);
		draw_point2d(0, -i, .8, .8, .8, 8);
		draw_point2d(i, 0, .8, .8, .8, 8);
		draw_point2d(-i, 0, .8, .8, .8, 8);
		i += .1;
	}
}

void game_render(void)
{
	touch_x_ = 0;
	touch_y_ = 0;

	draw_axis();
	handle_frame();
}

/* this should provide actual implementation of a game */

#ifdef HAVE_GAME
#include <game/game.h>
#endif
