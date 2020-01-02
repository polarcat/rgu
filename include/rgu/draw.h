/* draw.h: draw lines and dots
 *
 * Copyright (c) 2019, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#pragma once

#ifdef USE_DRAW

#include <rgu/log.h>
#include <rgu/gl.h>

struct draw_prog {
	GLuint id;
	GLint a_pos;
	GLint u_rgb;
	GLint u_size;
};

static struct draw_prog draw_prog_;

static void draw_init(void)
{
	if (draw_prog_.id)
		return;

	const char *fsrc =
		"precision mediump float;\n"
		"uniform vec3 u_rgb;\n"
		"void main() {\n"
			"gl_FragColor=vec4(u_rgb,1);\n"
		"}\0";

	const char *vsrc =
		"attribute vec2 a_pos;\n"
		"uniform float u_size;\n"
		"void main() {\n"
			"gl_PointSize=u_size;\n"
			"gl_Position=vec4(a_pos,0,1);\n"
		"}\0";

	if (!(draw_prog_.id = gl_make_prog(vsrc, fsrc))) {
		ee("failed to create program\n");
		return;
	}

	glUseProgram(draw_prog_.id);

	draw_prog_.a_pos = glGetAttribLocation(draw_prog_.id, "a_pos");
	draw_prog_.u_size = glGetUniformLocation(draw_prog_.id, "u_size");
	draw_prog_.u_rgb = glGetUniformLocation(draw_prog_.id, "u_rgb");
}

static void draw_point(float x, float y, float r, float g, float b, float size)
{
	float pos[2] = { x, y, };

//	glDisable(GL_CULL_FACE);
	glUseProgram(draw_prog_.id);

	glUniform3f(draw_prog_.u_rgb, r, g, b);
	glUniform1f(draw_prog_.u_size, size);
	glVertexAttribPointer(draw_prog_.a_pos, 2, GL_FLOAT, GL_FALSE, 0, pos);
	glEnableVertexAttribArray(draw_prog_.a_pos);

	glDrawArrays(GL_POINTS, 0, 1);

	glDisableVertexAttribArray(draw_prog_.a_pos);
//	glEnable(GL_CULL_FACE);
}

static void draw_line(float x0, float y0, float x1, float y1, float r, float g,
  float b)
{
	float pos[4] = { x0, y0, x1, y1, };

//	glDisable(GL_CULL_FACE);
	glUseProgram(draw_prog_.id);

	glUniform3f(draw_prog_.u_rgb, r, g, b);
	glVertexAttribPointer(draw_prog_.a_pos, 2, GL_FLOAT, GL_FALSE, 0, pos);
	glEnableVertexAttribArray(draw_prog_.a_pos);

	glDrawArrays(GL_LINES, 0, 2);

	glDisableVertexAttribArray(draw_prog_.a_pos);
//	glEnable(GL_CULL_FACE);
}
#else
#define draw_init() ;
#define draw_point(x, y, r, g, b, size) ;
#define draw_line(x0, y0, x1, y1, r, g, b) ;
#endif /* USE_DRAW */

#ifdef DRAW_AXIS
static inline void draw_axis(void)
{
	draw_line(-1, 0, 1, 0, .8, .8, .8);
	draw_line(0, -1, 0, 1, .8, .8, .8);

	for (float i = .1; i < 1.;) {
		draw_point(0, i, .8, .8, .8, 8);
		draw_point(0, -i, .8, .8, .8, 8);
		draw_point(i, 0, .8, .8, .8, 8);
		draw_point(-i, 0, .8, .8, .8, 8);
		i += .1;
	}
}
#else
#define draw_axis() ;
#endif
