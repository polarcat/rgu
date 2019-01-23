/* sb.c: simple status bar
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#define TAG "sb"

#include <utils/log.h>
#include <utils/sensors.h>

#include "gl.h"
#include "gm.h"
#include "font.h"
#include "plotter.h"


#define plot_statusbar(fmt, arg...) {\
	char str[MAX_STRLEN];\
	uint16_t pos = snprintf(str, MAX_STRLEN, fmt, ##arg);\
	str[pos] = '\0';\
	plot_text(str, pos);\
}

void sb_open(struct font *f0, struct font *f1, uint8_t async)
{
	GLint wh[4];

	glGetIntegerv(GL_VIEWPORT, wh);

	plotter_open(f0, f1, wh[2], wh[3], async);
	ii("init ok\n");
}

void sb_close(void)
{
	plotter_close();
}

void sb_render(void)
{
	union orientation orient;

	sensors_orientation(&orient);

	plot_statusbar(" azimuth %.2f | pitch %.2f | roll %.2f ",
	  degrees(orient.azimuth), degrees(orient.pitch), degrees(orient.roll));
}
