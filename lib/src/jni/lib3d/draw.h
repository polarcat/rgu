/* draw.h: utils for drawing geometrical primitives
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

struct lines {
	uint32_t count;
	float *coords;
	float *colors;
	uint8_t n:2; /* 2 for 2d lines; 3 for 3d lines */
};

struct points {
	uint32_t count;
	float *coords;
	float *colors;
	uint8_t *sizes;
	uint8_t n:2; /* 2 for 2d points; 3 for 3d points */
};

void draw_init(void);
void draw_points2d(struct points *points);
void draw_points3d(struct points *points);
void draw_lines2d(struct lines *lines);
void draw_lines3d(struct lines *lines);
