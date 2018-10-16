/* log.h: logging utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <errno.h>
#include <string.h>

#ifndef LIB_TAG
#define LIB_TAG "lib3d"
#endif

#ifndef TAG
#define TAG "log"
#endif

#define LOG_TAG "\033[0;35m"LIB_TAG"/"TAG "\033[0m : "

#ifdef ANDROID
#include <android/log.h>
#define ee(...) {\
	int errno__ = errno;\
	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);\
	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, " ^^ %s:%d | %s",\
	  __func__, __LINE__, __FILE__);\
	if (errno__)\
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, " %s",\
		  strerror(errno__));\
}

#define ii(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ww(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#ifdef DEBUG
#define dd(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#else
#define dd(...) ;
#endif
#else /* not ANDROID */
#include <stdio.h>
#define ee(fmt, arg...) {\
	int errno__ = errno;\
	fprintf(stderr, "\033[1;31m(ee)\033[0m " LOG_TAG fmt, ##arg);\
	fprintf(stderr, "\033[1;31m(ee)\033[0m " LOG_TAG "^^ %s:%d | %s\n",\
	  __func__, __LINE__, __FILE__);\
	if (errno__)\
		fprintf(stderr, "\033[1;31m(ee)\033[0m " LOG_TAG "%s\n",\
		  strerror(errno__));\
}

#define ii(fmt, arg...) printf("\033[0;32m(ii)\033[0m " LOG_TAG fmt, ##arg);
#define ww(fmt, arg...) printf("\033[1;33m(ww)\033[0m " LOG_TAG fmt, ##arg);

#ifdef DEBUG
#define dd(fmt, arg...) {\
        printf("\033[0;33m(dd)\033[0m " LOG_TAG fmt, ##arg);\
        printf("\033[0;33m(dd)\033[0m ^^ %s:%d | %s\n", __func__, __LINE__, __FILE__);\
}
#else
#define dd(...) ;
#endif

#endif /* ANDROID */

#define logv4(msg, v)\
	ii("%s: { %+.2f %+.2f %+.2f %+.2f }\n", msg, v[0], v[1], v[2], v[3])

#define logm4(msg, m) {\
	ii("%s:\n", msg);\
	ii("  { %+.2f %+.2f %+.2f %+.2f }\n", m[0], m[1], m[2], m[3]);\
	ii("  { %+.2f %+.2f %+.2f %+.2f }\n", m[4], m[5], m[6], m[7]);\
	ii("  { %+.2f %+.2f %+.2f %+.2f }\n", m[8], m[9], m[10], m[11]);\
	ii("  { %+.2f %+.2f %+.2f %+.2f }\n", m[12], m[13], m[14], m[15]);\
}

