/* log.h: logging utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <android/log.h>
#include <errno.h>
#include <string.h>

#ifndef TAG
#define TAG LIB_TAG
#endif

#define ee(...) {\
	int errno__ = errno;\
	__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);\
	__android_log_print(ANDROID_LOG_ERROR, TAG, "^^ %s:%d | %s",\
	  __func__, __LINE__, __FILE__);\
	if (errno__)\
		__android_log_print(ANDROID_LOG_ERROR, TAG, "%s",\
		  strerror(errno__));\
}

#define ii(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define ww(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

#ifdef DEBUG
#define dd(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#else
#define dd(...) ;
#endif

#define logv4(msg, v)\
	ii("%s: { %+.2f %+.2f %+.2f %+.2f }\n", msg, v[0], v[1], v[2], v[3])

#define logm4(msg, m) {\
	ii("%s:\n", msg);\
	ii("  { %+.2f %+.2f %+.2f %+.2f }\n", m[0], m[1], m[2], m[3]);\
	ii("  { %+.2f %+.2f %+.2f %+.2f }\n", m[4], m[5], m[6], m[7]);\
	ii("  { %+.2f %+.2f %+.2f %+.2f }\n", m[8], m[9], m[10], m[11]);\
	ii("  { %+.2f %+.2f %+.2f %+.2f }\n", m[12], m[13], m[14], m[15]);\
}

