/* pip.h: draw picture in picture
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <utils/log.h>

#include "pip.h"

static const uint8_t indices_[] = {
	0, 1, 2,
	2, 3, 0,
};

static const float coords_[] = {
	0, 1,
	0, 0,
	1, 0,
	1, 1,
};

void pip_init(struct pip_prog *prog)
{
	const char *fsrc =
		"precision highp float;\n"
		"varying vec2 v_uv;\n"
		"uniform sampler2D u_tex;\n"
		"void main() {\n"
			"gl_FragColor=texture2D(u_tex,v_uv)*1.1;\n"
		"}\0";

	const char *vsrc =
		"attribute vec2 a_pos;\n"
		"attribute vec2 a_uv;\n"
		"varying vec2 v_uv;\n"
		"void main() {\n"
			"gl_Position=vec4(a_pos.xy,0,1);\n"
			"v_uv=a_uv;\n"
		"}\0";

	if (!(prog->id = gl_make_prog(vsrc, fsrc))) {
		ee("failed to create program\n");
		return;
	}

	glGenTextures(1, &prog->tex);
	glBindTexture(GL_TEXTURE_2D, prog->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glUseProgram(prog->id);

	prog->a_pos = glGetAttribLocation(prog->id, "a_pos");
	prog->a_uv = glGetAttribLocation(prog->id, "a_uv");
	prog->u_tex = glGetUniformLocation(prog->id, "u_tex");

	ii("view init ok, texture %u\n", prog->tex);
}

void pip_close(struct pip_prog *prog)
{
	glDeleteTextures(1, &prog->tex);
	glDeleteProgram(prog->id);
}

void pip_render(struct pip_prog *prog, const uint8_t *buf, uint16_t w, uint16_t h)
{
	float sw = w;
	float sh = h;
	float verts[8];
	GLint wh[4];

	glGetIntegerv(GL_VIEWPORT, wh);

	verts[0] = 1 - sw / wh[2];
	verts[1] = -1;

	verts[2] = verts[0];
	verts[3] = sh / wh[3] - 1;

	verts[4] = 1;
	verts[5] = verts[3];

	verts[6] = 1;
	verts[7] = -1;

	glBindTexture(GL_TEXTURE_2D, prog->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
	  GL_UNSIGNED_BYTE, buf);

	glUseProgram(prog->id);
	glVertexAttribPointer(prog->a_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glEnableVertexAttribArray(prog->a_pos);
	glVertexAttribPointer(prog->a_uv, 2, GL_FLOAT, GL_FALSE, 0, coords_);
	glEnableVertexAttribArray(prog->a_uv);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices_);

	glDisableVertexAttribArray(prog->a_uv);
	glDisableVertexAttribArray(prog->a_pos);
	glBindTexture(GL_TEXTURE_2D, 0);
}
