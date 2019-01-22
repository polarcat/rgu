/* img.c: static background rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#ifdef ANDROID
#include <android/asset_manager_jni.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#define TAG "img"

#include <utils/log.h>
#include <utils/image.h>

#include "gl.h"
#include "gm.h"

static GLuint prog_;
static GLuint texid_;
static GLint u_tex_;
static GLint a_pos_;
static GLint a_uv_;
static uint16_t w_;
static uint16_t h_;

static const uint8_t indices_[] = {
	0, 1, 2,
	2, 3, 0,
};

static float verts_[] = {
	-1, 1,
	1, 1,
	1, -1,
	-1, -1,
};

static const float coords_[] = {
	0, 1,
	0, 0,
	1, 0,
	1, 1,
};

void img_close(void)
{
	glDeleteTextures(1, &texid_);
	glDeleteProgram(prog_);
}

static void upload_image(struct image *img)
{
	const char *vsrc =
		"attribute vec2 a_pos;\n"
		"attribute vec2 a_uv;\n"
		"varying vec2 v_uv;\n"
		"void main() {\n"
			"gl_Position=vec4(a_pos,0,1);\n"
			"v_uv=a_uv;\n"
		"}\0";

	const char *fsrc =
		"precision mediump float;\n"
		"varying vec2 v_uv;\n"
		"uniform sampler2D u_tex;\n"
		"void main() {\n"
			"gl_FragColor=texture2D(u_tex,v_uv);\n"
		"}\0";

	if (!(prog_ = gl_make_prog(vsrc, fsrc))) {
		ee("failed to create program\n");
		return;
	}

	glGenTextures(1, &texid_);
	glBindTexture(GL_TEXTURE_2D, texid_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexImage2D(GL_TEXTURE_2D, 0, img->format, img->w, img->h, 0,
	  img->format, GL_UNSIGNED_BYTE, img->data);
	gl_error("glGenTextures(1, &texid_);");

	glUseProgram(prog_);

	a_pos_ = glGetAttribLocation(prog_, "a_pos");
	a_uv_ = glGetAttribLocation(prog_, "a_uv");
	u_tex_ = glGetUniformLocation(prog_, "u_tex");
	gl_error("glUseProgram(prog_);");

	if (img->w == img->h) {
		if (w_ < h_) {
			float aspect = (float) h_ / w_;
			verts_[1] = 1 / aspect;
			verts_[3] = 1 / aspect;
			verts_[5] = -1 / aspect;
			verts_[7] = -1 / aspect;
		} else {
			float aspect = (float) w_ / h_;
			verts_[0] = -1 / aspect;
			verts_[2] = 1 / aspect;
			verts_[4] = 1 / aspect;
			verts_[6] = -1 / aspect;
		}
	}

	dd("verts {%.4f %.4f}  {%.4f %.4f}  {%.4f %.4f}  {%.4f %.4f}\n",
	  verts_[0], verts_[1], verts_[2], verts_[3], verts_[4], verts_[5],
	  verts_[6], verts_[7]);
}

#ifndef BG_IMAGE
#define BG_IMAGE "images/bg.png"
#endif

uint8_t img_open(void *assets)
{
	struct image img;
	const uint8_t *buf;
	size_t len = 0;
	int fd = -1;

#ifdef ANDROID
	AAsset *asset;

	if (!assets) {
		ee("bad assets manager\n");
		return 0;
	}

	asset = AAssetManager_open((AAssetManager *) assets, BG_IMAGE,
	  AASSET_MODE_BUFFER);

	if (!asset) {
		ee("failed to open assets " BG_IMAGE "\n");
		return 0;
	}

	buf = (const uint8_t *) AAsset_getBuffer(asset);
	len = AAsset_getLength(asset);
#else
	struct stat st;

	if (stat(path, &st) < 0) {
		ee("failed to stat file %s\n", BG_IMAGE);
		return 0;
	}

	if ((fd = open(path, O_RDONLY)) < 0) {
		ee("failed to open file %s\n", BG_IMAGE);
		return 0;
	}

	buf = (uint8_t *) mmap(NULL, st.st_size, PROT_READ,
	  MAP_PRIVATE, font->fd, 0);

	if (!buf) {
		ee("failed to map file %s\n", BG_IMAGE);
		close(fd);
		return 0;
	}
#endif

	if (!w_ || !h_) {
		GLint wh[4];
		glGetIntegerv(GL_VIEWPORT, wh);
		w_ = wh[2];
		h_ = wh[3];
	}

	if (buf2png(buf, &img))
		upload_image(&img);

#ifndef ANDROID
	if (len)
		munmap(buf, len);
	else if (fd >= 0)
		close(fd);
#endif

	ii("background image " BG_IMAGE " init ok, texture %u\n", texid_);

	return texid_;
}

void img_render(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_disable_features();

	glBindTexture(GL_TEXTURE_2D, texid_);
	glUseProgram(prog_);
	glVertexAttribPointer(a_pos_, 2, GL_FLOAT, GL_FALSE, 0, verts_);
	glEnableVertexAttribArray(a_pos_);
	glVertexAttribPointer(a_uv_, 2, GL_FLOAT, GL_FALSE, 0, coords_);
	glEnableVertexAttribArray(a_uv_);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices_);
	gl_error("glDrawElements");

	glDisableVertexAttribArray(a_uv_);
	glDisableVertexAttribArray(a_pos_);
	glBindTexture(GL_TEXTURE_2D, 0);

	gl_enable_features();
}

void img_resize(uint16_t w, uint16_t h)
{
	glViewport(0, 0, w, h);
	w_ = w;
	h_ = h;
}
