/* log.h: logging utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <errno.h>
#include <string.h>
#include <inttypes.h>

#ifndef LIB_TAG
#define LIB_TAG "rgu"
#endif

#ifndef TAG
#define TAG "log"
#endif

#define LOG_TAG "\033[0;35m"LIB_TAG"/"TAG "\033[0m "

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
#define tt(...) dd(...)
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
#if 0
#define tt(fmt, arg...) {\
        struct timespec now;\
        clock_gettime(CLOCK_MONOTONIC, &now);\
	uint32_t hh = now.tv_sec / 60 / 60;\
	uint32_t mm = (now.tv_sec % 3600) / 60;\
	uint32_t ss = (now.tv_sec % 60);\
	float ms = (now.tv_nsec / 1000000.);\
	printf("\033[0;34m(tt)\033[0m " LOG_TAG "[%u:%02u:%02u.%03.f] " fmt,\
	  hh, mm, ss, ms, ##arg);\
}
#else
#define tt(fmt, arg...) {\
        struct timespec now;\
        clock_gettime(CLOCK_MONOTONIC, &now);\
	printf("\033[0;34m(tt)\033[0m " LOG_TAG "[%lu.%03.f] " fmt,\
	  now.tv_sec, (now.tv_nsec / 1000000.), ##arg);\
}
#endif

#ifdef DEBUG
#define dd(fmt, arg...) {\
        printf("\033[0;33m(dd)\033[0m " LOG_TAG fmt, ##arg);\
        printf("\033[0;33m(dd)\033[0m ^^ %s:%d | %s\n", __func__, __LINE__, __FILE__);\
}
#else
#define dd(...) ;
#endif

#endif /* ANDROID */

#ifdef RELEASE
#undef ww
#undef ii
#undef tt
#undef dd
#define ww(...) ;
#define ii(...) ;
#define tt(...) ;
#define dd(...) ;
#endif

#define logv4(msg, v)\
	ii("%s: { %+.2f %+.2f %+.2f %+.2f }\n", msg, v[0], v[1], v[2], v[3])

#define logm4(msg, m) {\
	ii("%s:\n"\
	  "  { %+.2f %+.2f %+.2f %+.2f }\n"\
	  "  { %+.2f %+.2f %+.2f %+.2f }\n"\
	  "  { %+.2f %+.2f %+.2f %+.2f }\n"\
	  "  { %+.2f %+.2f %+.2f %+.2f }\n",\
	  msg,\
	  m[0], m[4], m[8], m[12],\
	  m[1], m[5], m[9], m[13],\
	  m[2], m[6], m[10], m[14],\
	  m[3], m[7], m[11], m[15]);\
}
