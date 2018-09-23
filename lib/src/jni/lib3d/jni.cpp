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

#include <libar/tracking.h>

#define TAG LIB_TAG": jni"

#include "gl.h"
#include "bg.h"
#include "font.h"

#include "writepng.h"

static struct font_info font_;

#ifdef __cplusplus
extern "C" {
#endif

#define jnicall(ret, fn, ...)\
	JNIEXPORT ret JNICALL Java_lib3d_scene_##fn(__VA_ARGS__)

jnicall(int, open, JNIEnv *env, jclass, jobject asset_manager)
{
	font_.asset_manager = AAssetManager_fromJava(env, asset_manager);
	font_open(&font_, 64, "fonts/Shure-Tech-Mono-Nerd-Font-Complete.ttf");
	tr_init(&font_);

	return bg_open();
}

static uint16_t sample_w_;
static uint16_t sample_h_;
static uint8_t *buf_;
static uint16_t frames_;

static uint32_t once_;

jnicall(void, render, JNIEnv *, jclass)
{
	bg_render();

#ifdef HAVE_LIBAR
#define TRACKING_INTERVAL (60 * 5)

	if (buf_ && frames_++ % TRACKING_INTERVAL == TRACKING_INTERVAL - 1) {
		bg_render_offscreen(buf_, sample_w_, sample_h_);
		tr_detect_features(buf_, sample_w_, sample_h_);
#if 0
		if (once_++ < 1) {
			write_png("/storage/0000-0000/1.png", buf_,
			  sample_w_, sample_h_);
		}
#endif
	}

	tr_render();
#endif

	fps(scene);
}

jnicall(void, resize, JNIEnv *, jclass, int w, int h)
{
	ii("new wh { %d, %d }\n", w, h);

	sample_w_ = w / 4;
	sample_h_ = h / 4;

	free(buf_);
	buf_ = (uint8_t *) malloc(sample_w_ * sample_h_ * 4);

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

#ifdef __cplusplus
}
#endif
