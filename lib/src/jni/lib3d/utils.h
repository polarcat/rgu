/* utils.h: common utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <log.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MAX_LINE
#define MAX_LINE 1024
#endif

#ifndef MAX_NAME
#define MAX_NAME 64
#endif

#include <time.h>

#define fps(tag) {\
	static unsigned int frames_##tag;\
	static unsigned int acc_ms_##tag;\
	static struct timespec acc_##tag;\
	unsigned int now_ms;\
	struct timespec now;\
	clock_gettime(CLOCK_MONOTONIC, &now);\
	now_ms =\
	(now.tv_sec - acc_##tag.tv_sec) * 1000 +\
	(now.tv_nsec - acc_##tag.tv_nsec) * .000001;\
	acc_##tag.tv_sec = now.tv_sec;\
	acc_##tag.tv_nsec = now.tv_nsec;\
	if (acc_ms_##tag >= 5000) {\
		if (now_ms < 10000) {\
			float secs = acc_ms_##tag / 1000;\
			ii(#tag\
			  " \033[0;33m%.02f fps (%u frames in %.02f seconds)\033[0m\n",\
			  frames_##tag / secs,\
			  frames_##tag, secs);\
		}\
		frames_##tag = 0;\
		acc_ms_##tag = 0;\
	}\
	frames_##tag++;\
	acc_ms_##tag += now_ms;\
}

void utils_str2rgb(const char *color, uint8_t *r, uint8_t *g, uint8_t *b);
void utils_fill2d(uint32_t *buf, uint32_t *end, uint32_t color);
