/* pip.h: draw picture in picture
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include "gl.h"

#ifndef GL_CLAMP_TO_BORDER
#ifdef GL_CLAMP_TO_BORDER_NV
#define GL_CLAMP_TO_BORDER GL_CLAMP_TO_BORDER_NV
#else
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#endif /* GL_CLAMP_TO_BORDER */

struct pip_prog {
	GLuint id;
	GLuint tex;
	GLint u_tex;
	GLint a_pos;
	GLint a_uv;
};

void pip_init(struct pip_prog *prog);
void pip_render(struct pip_prog *prog, const uint8_t *buf, uint16_t w, uint16_t h);
void pip_close(struct pip_prog *prog);
