/* draw.c: utils for drawing geometrical primitives
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#define TAG "draw"

#include <utils/log.h>

#include "gl.h"
#include "draw.h"

struct prog {
	GLuint id;
	GLint a_pos;
	GLint a_rgb;
	GLint a_size;
	GLint u_color;
	GLint u_size;
};

static struct prog prog2ds_;

static void init_prog_simple(struct prog *prog, const char *fsrc, const char *vsrc)
{
	if (!(prog->id = gl_make_prog(vsrc, fsrc))) {
		ee("failed to create program\n");
		return;
	}

	glUseProgram(prog->id);

	prog->a_pos = glGetAttribLocation(prog->id, "a_pos");
	prog->u_color = glGetUniformLocation(prog->id, "u_color");
	prog->u_size = glGetUniformLocation(prog->id, "u_size");
}

static void init2ds(void)
{
	const char *fsrc =
		"precision highp float;\n"
		"uniform vec3 u_color;\n"
		"void main() {\n"
			"gl_FragColor=vec4(u_color,1);\n"
		"}\0";

	const char *vsrc =
		"attribute vec2 a_pos;\n"
		"uniform float u_size;\n"
		"void main() {\n"
			"gl_PointSize=u_size;\n"
			"gl_Position=vec4(a_pos,0,1);\n"
		"}\0";

	init_prog_simple(&prog2ds_, fsrc, vsrc);
}

static struct prog prog2d_;
static struct prog prog3d_;

static void init_prog(struct prog *prog, const char *fsrc, const char *vsrc)
{
	if (!(prog->id = gl_make_prog(vsrc, fsrc))) {
		ee("failed to create program\n");
		return;
	}

	glUseProgram(prog->id);

	prog->a_pos = glGetAttribLocation(prog->id, "a_pos");
	prog->a_rgb = glGetAttribLocation(prog->id, "a_rgb");
	prog->a_size = glGetAttribLocation(prog->id, "a_size");
}

static void init2d(void)
{
	const char *fsrc =
		"precision mediump float;\n"
		"varying vec3 v_rgb;\n"
		"void main() {\n"
			"gl_FragColor=vec4(v_rgb,1);\n"
		"}\0";

	const char *vsrc =
		"attribute vec2 a_pos;\n"
		"attribute vec3 a_rgb;\n"
		"attribute float a_size;\n"
		"varying vec3 v_rgb;\n"
		"void main() {\n"
			"gl_PointSize=a_size;\n"
			"gl_Position=vec4(a_pos,0,1);\n"
			"v_rgb=a_rgb;\n"
		"}\0";

	init_prog(&prog2d_, fsrc, vsrc);
}

static void init3d(void)
{
	const char *fsrc =
		"precision mediump float;\n"
		"varying vec3 v_rgb;\n"
		"void main() {\n"
			"gl_FragColor=vec4(v_rgb,1);\n"
		"}\0";

	const char *vsrc =
		"attribute vec3 a_pos;\n"
		"attribute vec3 a_rgb;\n"
		"attribute float a_size;\n"
		"varying vec3 v_rgb;\n"
		"void main() {\n"
			"gl_PointSize=a_size;\n"
			"gl_Position=vec4(a_pos,1);\n"
			"v_rgb=a_rgb;\n"
		"}\0";

	init_prog(&prog3d_, fsrc, vsrc);
}

void draw_init(void)
{
	init2ds();
	init2d();
	init3d();

	ii("point init ok\n");
}

void draw_point2d(float x, float y, float r, float g, float b, float size)
{
	struct prog *prog = &prog2ds_;
	float pos[2] = { x, y, };

	gl_disable_features();

	glUseProgram(prog->id);

	glUniform3f(prog->u_color, r, g, b);
	glUniform1f(prog->u_size, size);
	glVertexAttribPointer(prog->a_pos, 2, GL_FLOAT, GL_FALSE, 0, pos);
	glEnableVertexAttribArray(prog->a_pos);

	glDrawArrays(GL_POINTS, 0, 1);

	glDisableVertexAttribArray(prog->a_pos);

	gl_enable_features();
}

void draw_line2d(float x0, float y0, float x1, float y1, float r, float g,
  float b)
{
	struct prog *prog = &prog2ds_;
	float pos[4] = { x0, y0, x1, y1, };

	gl_disable_features();

	glUseProgram(prog->id);

	glUniform3f(prog->u_color, r, g, b);
	glUniform1f(prog->u_size, 1);
	glVertexAttribPointer(prog->a_pos, 2, GL_FLOAT, GL_FALSE, 0, pos);
	glEnableVertexAttribArray(prog->a_pos);

	glDrawArrays(GL_LINES, 0, 2);

	glDisableVertexAttribArray(prog->a_pos);

	gl_enable_features();
}

static inline void draw_points(struct points *points, struct prog *prog)
{
	gl_disable_features();

	glUseProgram(prog->id);

	glVertexAttribPointer(prog->a_pos, points->n, GL_FLOAT, GL_FALSE, 0,
	  points->coords);
	glEnableVertexAttribArray(prog->a_pos);

	glVertexAttribPointer(prog->a_rgb, 3, GL_FLOAT, GL_FALSE, 0,
	  points->colors);
	glEnableVertexAttribArray(prog->a_rgb);

	glVertexAttribPointer(prog->a_size, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0,
	  points->sizes);
	glEnableVertexAttribArray(prog->a_size);

	glDrawArrays(GL_POINTS, 0, points->count);
	gl_error("glDrawElements");

	glDisableVertexAttribArray(prog->a_pos);
	glDisableVertexAttribArray(prog->a_rgb);
	glDisableVertexAttribArray(prog->a_size);

	gl_enable_features();
}

void draw_points2d(struct points *points)
{
	points->n = 2;
	draw_points(points, &prog2d_);
}

void draw_points3d(struct points *points)
{
	points->n = 3;
	draw_points(points, &prog3d_);
}

static inline void draw_lines(struct lines *lines, struct prog *prog)
{
	gl_disable_features();

	glUseProgram(prog->id);

	glVertexAttribPointer(prog->a_pos, lines->n, GL_FLOAT, GL_FALSE, 0,
	  lines->coords);
	glEnableVertexAttribArray(prog->a_pos);

	glVertexAttribPointer(prog->a_rgb, 3, GL_FLOAT, GL_FALSE, 0,
	  lines->colors);
	glEnableVertexAttribArray(prog->a_rgb);

	glDrawArrays(GL_LINES, 0, lines->count * 2);
	gl_error("glDrawElements");

	glDisableVertexAttribArray(prog->a_pos);
	glDisableVertexAttribArray(prog->a_rgb);

	gl_enable_features();
}

void draw_lines2d(struct lines *lines)
{
	lines->n = 2;
	draw_lines(lines, &prog2d_);
}

void draw_lines3d(struct lines *lines)
{
	lines->n = 3;
	draw_lines(lines, &prog3d_);
}
