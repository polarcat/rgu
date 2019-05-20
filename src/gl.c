/* gl.c: graphics helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#include <stdlib.h>

#define TAG "gl"

#include <rgu/log.h>
#include <rgu/gl.h>

static GLuint make_shader(GLenum type, const char *src)
{
	GLuint shader = glCreateShader(type);

	if (!shader) {
		gl_error("create shader");
		return 0;
	}

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		GLint len = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

		if (!len)
			return 0;

		char *buf = (char *) malloc(len);

		if (!buf)
			return 0;

		errno = 0;
		glGetShaderInfoLog(shader, len, NULL, buf);
		ee("could not compile %s shader: %s",
		  type == GL_VERTEX_SHADER ? "vertex" : "fragment", buf);
		free(buf);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint gl_make_prog(const char *vsrc, const char *fsrc)
{
	GLuint prog = glCreateProgram();

	if (!prog) {
		gl_error("create program");
		return 0;
	}

	GLuint vsh = make_shader(GL_VERTEX_SHADER, vsrc);

	if (!vsh)
		return 0;

	GLuint fsh = make_shader(GL_FRAGMENT_SHADER, fsrc);

	if (!fsh)
		return 0;

	glAttachShader(prog, vsh);
	glAttachShader(prog, fsh);
	glLinkProgram(prog);
	GLint status = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);

	if (status != GL_TRUE) {
		GLint len = 0;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);

		if (len) {
			char *buf = (char *) malloc(len);

			if (buf) {
				errno = 0;
				glGetProgramInfoLog(prog, len, NULL, buf);
				ee("%s", buf);
				free(buf);
			}
		}

		glDeleteProgram(prog);
		ee("failed to link program %u\n", prog);
		return 0;
	}

	return prog;
}
