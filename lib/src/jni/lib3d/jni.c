/* jni.c: java native interfaces
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <jni.h>
#include <android/asset_manager_jni.h>

#define TAG "jni"

#include <utils/time.h>
#include <utils/log.h>

#include "gl.h"
#include "bg.h"
#include "font.h"

#ifdef HAVE_LIBAR
#define PG_CONFIG
#include <libar/playground.h>
#endif

static struct font *font_;
static float x_;
static float y_;

#ifdef __cplusplus
extern "C" {
#endif

#define jnicall(ret, fn, ...)\
	JNIEXPORT ret JNICALL Java_lib3d_scene_##fn(__VA_ARGS__)

jnicall(int, open, JNIEnv *env, jclass class, jobject asset_manager)
{
	if (!(font_ = font_open("fonts/Shure-Tech-Mono-Nerd-Font-Complete.ttf", 128,
	  AAssetManager_fromJava(env, asset_manager))))
		return -1;

#ifdef HAVE_LIBAR
#define PG_INIT
#include <libar/playground.h>
#endif

	return bg_open();
}

jnicall(void, render, JNIEnv *env, jclass class)
{
	bg_render(0);

#ifdef HAVE_LIBAR
#define PG_RENDER
#include <libar/playground.h>
#endif

//	print_fps(render);
}

jnicall(void, resize, JNIEnv *env, jclass class, int w, int h)
{
	ii("new wh { %d, %d }\n", w, h);

#ifdef HAVE_LIBAR
#define PG_RESIZE
#include <libar/playground.h>
#endif

	bg_resize(w, h);
}

jnicall(void, rotate, JNIEnv *env, jclass class, int r)
{
}

jnicall(void, close, JNIEnv *env, jclass class)
{
	font_close(&font_);
	bg_close();
}

jnicall(void, pause, JNIEnv *env, jclass class)
{
}

jnicall(void, resume, JNIEnv *env, jclass class, jobject ctx, jobject act)
{
}

jnicall(void, input, JNIEnv *env, jclass class, float x, float y)
{
	ii("touch at { %f, %f }", x, y);
#ifdef HAVE_LIBAR
	touch_x_ = x;
	touch_y_ = y;
#endif
}

#ifdef __cplusplus
}
#endif
