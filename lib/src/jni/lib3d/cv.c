/* cv.c: image processing utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#define TAG "cv"

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

//#define DEBUG_VIEW

#ifdef DEBUG_VIEW
#include "pip.h"
static struct pip_prog debug_view_;
#endif

#ifdef HAVE_ALGO
void process_image(uint8_t *buf, uint16_t w, uint16_t h);
#else
#define process_image(a, b, c) ;
#endif

static uint8_t job_done_;

static pthread_mutex_t job_state_ = PTHREAD_MUTEX_INITIALIZER;

#define lock_job_state(void) pthread_mutex_lock(&job_state_)
#define unlock_job_state(void) pthread_mutex_unlock(&job_state_)

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
static pthread_t thread_;
static uint8_t close_;

static uint16_t img_w_;
static uint16_t img_h_;
static uint8_t *img_;

static uint16_t frames_;
static uint16_t touch_x_;
static uint16_t touch_y_;

static uint8_t async_;

static sem_t run_;

static void do_process_image(void)
{
	lock_resize();

	if (resize_) {
		resize_ = 0;
		free(img_);
		img_ = (uint8_t *) malloc(img_w_ * img_h_ * 4); /* rgba */
	}

	unlock_resize();

	process_image(img_, img_w_, img_h_);
}

static void *thread_work(void *arg)
{
        while (!close_) {
                sem_wait(&run_);
		do_process_image();
		plotter_swap();

		lock_job_state();
		job_done_ = 1;
		unlock_job_state();
        }

	free(img_);
	img_ = NULL;

        return NULL;
}

void cv_resize(uint16_t w, uint16_t h)
{
	lock_resize();

	img_w_ = w;
	img_h_ = h;
	resize_ = 1;

	unlock_resize();
}

void cv_touch(uint16_t x, uint16_t y)
{
	lock_touch();

	touch_x_ = x;
	touch_y_ = y;

	unlock_touch();
}

void cv_open(struct font *f0, struct font *f1, uint8_t async)
{
	GLint wh[4];

	glGetIntegerv(GL_VIEWPORT, wh);

	img_w_ = wh[2];
	img_h_ = wh[3];

	font0_ = f0;
	font1_ = f1;
	async_ = async;

	draw_init();
	gm_open(img_w_ / 4);
	plotter_open(f0, f1, img_w_, img_h_, async);

	if (async_) {
	        sem_init(&run_, 0, 0);
	        pthread_create(&thread_, NULL, thread_work, NULL);
	}

#ifdef DEBUG_VIEW
	pip_init(&debug_view_);
#endif

	ii("init ok\n");
}

void cv_close(void)
{
#ifdef DEBUG_VIEW
	pip_close(&debug_view_);
#endif
	gm_close();

	if (async_) {
		plotter_close();
		close_ = 1; /* tell the worker thread to release resource and exit */
	}
}

/*
 * this will run on rendering thread; make sure to call cv_open before
 * */

void cv_render(void)
{
	lock_job_state();

	if (img_ && job_done_) {
		bg_render_offscreen(img_, img_w_, img_h_, 1);
		job_done_ = 0;
	}

	unlock_job_state();

#ifdef DEBUG_VIEW
	pip_render(&debug_view_, img_, img_w_, img_h_);
#endif

	if (!async_) {
		do_process_image();
		lock_job_state();
		job_done_ = 1;
		unlock_job_state();
	} else {
		plotter_render();

		int val = 1; /* to skip error checking */

		sem_getvalue(&run_, &val);

		if (!val)
			sem_post(&run_);
	}

	touch_x_ = 0;
	touch_y_ = 0;
}

/* this should provide actual implementation for image processing algorithm */

#ifdef HAVE_ALGO
#include <algo/algo.h>
#endif
