/* time.h: time related helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "log.h"

#define THRESHOLD 10000

#define tc(f, fmt, arg...) {\
	struct timeval __prev__;\
	struct timeval __curr__;\
        const char *slow_color__ = "1;31";\
        const char *good_color__ = "0;32";\
        const char *color__;\
        double time_diff__;\
	gettimeofday(&__prev__, NULL);\
	f;\
	gettimeofday(&__curr__, NULL);\
        time_diff__ = ((double) __curr__.tv_sec + (double) __curr__.tv_usec * 1.0e-6) -\
	     ((double) __prev__.tv_sec + (double) __prev__.tv_usec * 1.0e-6);\
        if (time_diff__ < THRESHOLD)\
            color__ = good_color__;\
        else\
            color__ = slow_color__;\
	ii("%ld.%06ld \033[%sm%.06lf\033[0m \033[1;33m" fmt"\033[0m | " #f "\n",\
             __curr__.tv_sec, __curr__.tv_usec, color__, time_diff__, ##arg);\
}

/* p must be same for both tc0 and tc1 */

#define tc0(p) \
	struct timeval prev_##p;\
	gettimeofday(&prev_##p, NULL)

#define tc1(p, fmt, arg...) \
	struct timeval cur_##p;\
        const char *slow_color_##p = "1;31";\
        const char *good_color_##p = "0;32";\
        const char *color_##p;\
        double time_diff_##p;\
	gettimeofday(&cur_##p, NULL);\
        time_diff_##p = ((double) cur_##p.tv_sec + (double) cur_##p.tv_usec * 1.0e-6) -\
	     ((double) prev_##p.tv_sec + (double) prev_##p.tv_usec * 1.0e-6);\
        if (time_diff_##p < THRESHOLD)\
            color_##p = good_color_##p;\
        else\
            color_##p = slow_color_##p;\
	ii("%ld.%06ld \033[%sm%.06lf\033[0m \033[1;33m" #p "\033[0m " fmt " | %s:%d\n",\
             cur_##p.tv_sec, cur_##p.tv_usec, color_##p, time_diff_##p, ##arg, __func__, __LINE__)

static inline uint64_t time_ms(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1000 + (uint32_t) (now.tv_nsec * .000001);
}

struct timeinfo {
	uint32_t hh;
	uint8_t mm;
	uint8_t ss;
};

static inline void time_info(time_t ss, struct timeinfo *ti)
{
	ti->hh = ss / 60 / 60;
	ti->mm = (ss % 3600) / 60;
	ti->ss = (ss % 60);
}

struct fps_info {
	float fps;
	uint8_t frames_max;
	uint8_t frames;
	long prev_ms;
};

static inline void print_fps(struct fps_info *info)
{
	uint64_t now_ms = time_ms();

	if (info->frames == info->frames_max) {
		float secs = (now_ms - info->prev_ms) / 1000.;
		info->fps = info->frames / secs;

		ii("\033[0;33m%.2f fps (%u frames in %.2f seconds)\033[0m\n",
		  info->fps, info->frames, secs);

		info->prev_ms = now_ms;
		info->frames = 0;
	}

	info->frames++;
}

static inline void count_fps(struct fps_info *info)
{
	uint64_t now_ms = time_ms();

	if (info->frames == info->frames_max) {
		float secs = (now_ms - info->prev_ms) / 1000.;
		info->fps = info->frames / secs;
		info->frames = 0;
		info->prev_ms = now_ms;
	}

	info->frames++;
}

#define sleep_ms(ms) {\
	struct timespec t = { .tv_sec = 0, .tv_nsec = ms * 1000 * 1000, };\
	clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);\
	clock_gettime(CLOCK_MONOTONIC, &t);\
}
