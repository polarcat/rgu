/* bg.c: background rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#define TAG "bg"

#include <utils/log.h>

#include "gl.h"

static GLuint fbo_;
static GLuint img_;
static GLuint prog_;
static GLuint texid_;
static GLint u_tex_;
static GLint u_grey_;
static GLint a_pos_;
static GLint a_uv_;
static uint16_t w_;
static uint16_t h_;
static uint8_t fbook_;

#ifdef ANDROID
static GLenum textype_ = GL_TEXTURE_EXTERNAL_OES;
#else
static GLenum textype_ = GL_TEXTURE_2D;
#endif

static const uint8_t indices_[] = {
	0, 1, 2,
	2, 3, 0,
};

#ifdef ANDROID
static const float verts_[] = {
	-1, +1,
	+1, +1,
	+1, -1,
	-1, -1,
};
#else
static const float verts_[] = {
	+1, -1,
	+1, +1,
	-1, +1,
	-1, -1,
};
#endif

static const float offscr_verts_[] = {
	-1, -1,
	1, -1,
	1, 1,
	-1, 1,
};

static const float *vertices_ = verts_;

static const float coords_[] = {
	0, 1,
	0, 0,
	1, 0,
	1, 1,
};

static inline uint8_t prepare_offscreen(uint8_t *buf, uint16_t w, uint16_t h)
{
	ii("prepare fbo %u; offscreen texture %u buffer at %p\n", fbo_, img_,
	  buf);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	glBindTexture(GL_TEXTURE_2D, img_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
	  GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	  GL_TEXTURE_2D, img_, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
	  GL_FRAMEBUFFER_COMPLETE) {
		ee("incomplete fbo %u; texture %u error %#x\n", fbo_, img_,
		  glGetError());
		return 0;
	}

	ii("complete fbo %u; texture %u\n", fbo_, img_);
	fbook_ = 1;

	return 1;
}

void bg_close(void)
{
	glDeleteTextures(1, &texid_);
	glDeleteProgram(prog_);
}

int bg_open(void)
{
	const char *fsrc =
#ifdef ANDROID
		"#extension GL_OES_EGL_image_external : require\n"
#endif
		"precision lowp float;\n"
		"varying vec2 v_uv;\n"
		"uniform int u_grey;\n"
#ifdef ANDROID
		"uniform samplerExternalOES u_tex;\n"
#else
		"uniform sampler2D u_tex;\n"
#endif
		"void main() {\n"
			"if (u_grey==0) {\n"
				"gl_FragColor=texture2D(u_tex,v_uv);\n"
			"} else {\n"
				"vec3 rgb=texture2D(u_tex,v_uv).rgb;\n"
#if 0 /* relative luminance */
				"float y=.2126*rgb.r+.7152*rgb.g+.0722*rgb.b;\n"
#else /* perceived luminance */
				"float y=.299*rgb.r+.587*rgb.g+.114*rgb.b;\n"
#endif
				"gl_FragColor=vec4(y,y,y,1);\n"
			"}\n"
		"}\0";

	const char *vsrc =
		"attribute vec4 a_pos;\n"
		"attribute vec2 a_uv;\n"
		"varying vec2 v_uv;\n"
		"void main() {\n"
		"gl_Position=a_pos;\n"
		"v_uv=a_uv;\n"
		"}\0";

	if (!(prog_ = gl_make_prog(vsrc, fsrc))) {
		ee("failed to create program\n");
		return -1;
	}

	glBindTexture(textype_, texid_);
	glTexParameteri(textype_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(textype_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glUseProgram(prog_);

	a_pos_ = glGetAttribLocation(prog_, "a_pos");
	a_uv_ = glGetAttribLocation(prog_, "a_uv");
	u_tex_ = glGetUniformLocation(prog_, "u_tex");
	u_grey_ = glGetUniformLocation(prog_, "u_grey");

	ii("background init ok, texture %u\n", texid_);

	/* these are for offscreen rendering */

	glGenFramebuffers(1, &fbo_);
	glGenTextures(1, &img_);

	return texid_;
}

void bg_render(uint8_t grey)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_disable_features();

	glBindTexture(textype_, texid_);
	glUseProgram(prog_);
	glUniform1i(u_grey_, grey);
	glVertexAttribPointer(a_pos_, 2, GL_FLOAT, GL_FALSE, 0, vertices_);
	glEnableVertexAttribArray(a_pos_);
	glVertexAttribPointer(a_uv_, 2, GL_FLOAT, GL_FALSE, 0, coords_);
	glEnableVertexAttribArray(a_uv_);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices_);
	gl_error("glDrawElements");

	glDisableVertexAttribArray(a_uv_);
	glDisableVertexAttribArray(a_pos_);
	glBindTexture(textype_, 0);

	gl_enable_features();
}

void bg_render_offscreen(uint8_t *buf, uint16_t w, uint16_t h, uint8_t grey)
{
	if (fbook_) {
		dd("bind fbo %u; offscreen texture %u buffer at %p\n", fbo_,
		  img_, buf);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	} else if (!prepare_offscreen(buf, w, h)) {
		return;
	}

	glViewport(0, 0, w, h);

	vertices_ = offscr_verts_;
	bg_render(grey);
	vertices_ = verts_;

	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w_, h_);
}

void bg_resize(uint16_t w, uint16_t h)
{
	glViewport(0, 0, w, h);
	w_ = w;
	h_ = h;
}
