/* jni.c: java native interfaces
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <jni.h>
#include <android/asset_manager_jni.h>
#include <list>
#include <mutex>

#ifdef HAVE_LIBAR
#include <libar/tracking.h>

#define SCALE_FACTOR 4
static uint16_t img_w_;
static uint16_t img_h_;
static uint8_t *img_;
static uint16_t frames_;
static uint16_t touch_x_;
static uint16_t touch_y_;
#endif

#define TAG LIB_TAG": jni"

#include "gl.h"
#include "bg.h"
#include "font.h"

#include <common/writepng.h>

static struct font *font_;
static float x_;
static float y_;

#ifdef __cplusplus
extern "C" {
#endif

#define jnicall(ret, fn, ...)\
	JNIEXPORT ret JNICALL Java_lib3d_scene_##fn(__VA_ARGS__)

jnicall(int, open, JNIEnv *env, jclass, jobject asset_manager)
{
	if (!(font_ = font_open("fonts/Shure-Tech-Mono-Nerd-Font-Complete.ttf", 128,
	  AAssetManager_fromJava(env, asset_manager))))
		return -1;

	tr_init(font_);

	return bg_open();
}

jnicall(void, render, JNIEnv *, jclass)
{
	bg_render();

#ifdef HAVE_LIBAR
	if (img_ && tr_ready()) {
		bg_render_offscreen(img_, img_w_, img_h_, true);
		tr_detect(img_, img_w_, img_h_, SCALE_FACTOR, touch_x_, touch_y_);
#if 0
		if (touch_x_ && touch_y_) {
			write_png("/storage/0000-0000/1.png", img_, img_w_,
			  img_h_);
		}
#endif

		touch_x_ = 0;
		touch_y_ = 0;
	}

	tr_render();
#endif

	fps(scene);
}

jnicall(void, resize, JNIEnv *, jclass, int w, int h)
{
	ii("new wh { %d, %d }\n", w, h);

#ifdef HAVE_LIBAR
	img_w_ = w / SCALE_FACTOR;
	img_h_ = h / SCALE_FACTOR;

	free(img_);
	img_ = (uint8_t *) malloc(img_w_ * img_h_ * 4); /* rgba */
#endif

	bg_resize(w, h);
}

jnicall(void, rotate, JNIEnv *, jclass, int r)
{
}

jnicall(void, close, JNIEnv *, jclass)
{
	font_close(&font_);
	bg_close();
}

jnicall(void, pause, JNIEnv *, jclass)
{
}

jnicall(void, resume, JNIEnv *env, jclass, jobject ctx, jobject act)
{
}

jnicall(void, input, JNIEnv *, jclass, float x, float y)
{
	ii("touch at { %f, %f }", x, y);
	touch_x_ = x;
	touch_y_ = y;
}

#ifdef __cplusplus
}
#endif
