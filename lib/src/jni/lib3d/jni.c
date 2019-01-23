/* jni.c: java native interfaces
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <semaphore.h>

#include <jni.h>
#include <android/asset_manager_jni.h>

#define TAG "jni"

#include <utils/time.h>
#include <utils/log.h>
#include <utils/sensors.h>

#include "cv.h"
#include "sb.h"
#include "gl.h"
#include "bg.h"
#include "img.h"
#include "font.h"

static struct font *font0_;
static struct font *font1_;
static float x_;
static float y_;

static sem_t run_;
static uint8_t pause_;

//#define FRAME_BY_FRAME

#ifdef __cplusplus
extern "C" {
#endif

#define jnicall(ret, fn, ...)\
  JNIEXPORT ret JNICALL Java_lib3d_scene_##fn(__VA_ARGS__)

jnicall(int, open, JNIEnv *env, jclass class, jobject asset_manager)
{
	if (!(font0_ = font_open("fonts/Shure-Tech-Mono-Nerd-Font-Complete.ttf", 64,
	  AAssetManager_fromJava(env, asset_manager))))
		return -1;

	if (!(font1_ = font_open("fonts/Shure-Tech-Mono-Nerd-Font-Complete.ttf", 128,
	  AAssetManager_fromJava(env, asset_manager))))
		return -1;

	sem_init(&run_, 0, 0);

#ifdef IMAGE_VIEWER
#error "IMAGE_VIEWER"
	sb_open(font0_, font1_, CV_BLOCK);
	return img_open(AAssetManager_fromJava(env, asset_manager));
#else
	cv_open(font0_, font1_, CV_BLOCK);
	return bg_open();
#endif
}

jnicall(void, render, JNIEnv *env, jclass class)
{
#ifdef FRAME_BY_FRAME
	sem_wait(&run_);
#else
	if (pause_)
		sem_wait(&run_);
#endif

#ifdef IMAGE_VIEWER
	img_render();
	sb_render();
#else
	bg_render(0);
	cv_render();
#endif
//	print_fps(render);
}

jnicall(void, resize, JNIEnv *env, jclass class, int w, int h)
{
	ii("new wh { %d, %d }\n", w, h);

#ifdef IMAGE_VIEWER
	img_resize(w, h);
#else
	bg_resize(w, h);
	cv_resize(w, h);
#endif
}

jnicall(void, rotate, JNIEnv *env, jclass class, int r)
{
}

jnicall(void, close, JNIEnv *env, jclass class)
{
	font_close(&font0_);
	font_close(&font1_);
#ifdef IMAGE_VIEWER
	sb_close();
	img_close();
#else
	bg_close();
	cv_close();
#endif
}

jnicall(void, pause, JNIEnv *env, jclass class)
{
}

jnicall(void, resume, JNIEnv *env, jclass class, jobject ctx, jobject act)
{
}

static void post_sem()
{
	int val = 1; /* to skip error checking */

	sem_getvalue(&run_, &val);

	if (!val)
		sem_post(&run_);
}

jnicall(void, input, JNIEnv *env, jclass class, float x, float y)
{
	dd("touch at { %f, %f }", x, y);

	cv_touch(x, y);

#ifdef FRAME_BY_FRAME
	post_sem();
#else
	if (!pause_) {
		pause_ = 1;
	} else {
		pause_ = 0;
		post_sem();
	}
#endif
}

jnicall(void, rmatrix, JNIEnv *env, jclass class, float a0, float a1, float a2,
  float a3, float a4, float a5, float a6, float a7, float a8)
{
	if (!pause_)
		sensors_update_rmatrix(a0, a1, a2, a3, a4, a5, a6, a7, a8);
}

jnicall(void, imatrix, JNIEnv *env, jclass class, float a0, float a1, float a2,
  float a3, float a4, float a5, float a6, float a7, float a8)
{
	if (!pause_)
		sensors_update_imatrix(a0, a1, a2, a3, a4, a5, a6, a7, a8);
}

jnicall(void, orientation, JNIEnv *env, jclass class, float azimuth,
  float pitch, float roll)
{
	if (!pause_)
		sensors_update_orientation(azimuth, pitch, roll);
}

#ifdef __cplusplus
}
#endif
