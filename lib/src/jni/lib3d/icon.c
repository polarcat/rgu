/* icon.c: icon renderer
 *
 * Copyright (C) 2019 by Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#ifdef ANDROID
#include <android/asset_manager_jni.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#define TAG "icon"

#include <utils/log.h>
#include <utils/image.h>

#include "gl.h"
#include "gm.h"
#include "tools.h"

struct icon {
	GLuint prog;
	GLuint tex;
	GLint u_tex;
	GLint a_pos;
	GLint a_uv;
	struct round_rect rect;
};

struct icon icon_;

static void make_texture(struct image *img)
{
	glGenTextures(1, &icon_.tex);
	glBindTexture(GL_TEXTURE_2D, icon_.tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexImage2D(GL_TEXTURE_2D, 0, img->format, img->w, img->h, 0,
	  img->format, GL_UNSIGNED_BYTE, img->data);
}

static uint8_t load_image(const char *path, void *assets)
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

	asset = AAssetManager_open((AAssetManager *) assets, path,
	  AASSET_MODE_BUFFER);

	if (!asset) {
		ee("failed to open assets '%s'\n", path);
		return 0;
	}

	buf = (const uint8_t *) AAsset_getBuffer(asset);
	len = AAsset_getLength(asset);
#else
	struct stat st;

	if (stat(path, &st) < 0) {
		ee("failed to stat file '%s'\n", path);
		return 0;
	}

	if ((fd = open(path, O_RDONLY)) < 0) {
		ee("failed to open file '%s'\n", path);
		return 0;
	}

	buf = (uint8_t *) mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	if (!buf) {
		ee("failed to map file %s\n", path);
		close(fd);
		return 0;
	}
#endif

	if (buf2png(buf, &img))
		make_texture(&img);

#ifndef ANDROID
	if (len)
		munmap((void *) buf, len);
	else if (fd >= 0)
		close(fd);
#endif

	return icon_.tex;
}

static uint8_t make_prog(void)
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
		"precision lowp float;\n"
		"varying vec2 v_uv;\n"
		"uniform sampler2D u_tex;\n"
		"void main() {\n"
			"gl_FragColor=texture2D(u_tex,v_uv);\n"
		"}\0";

	if (!(icon_.prog = gl_make_prog(vsrc, fsrc))) {
		ee("failed to create program\n");
		return 0;
	}

	glUseProgram(icon_.prog);

	icon_.a_pos = glGetAttribLocation(icon_.prog, "a_pos");
	icon_.a_uv = glGetAttribLocation(icon_.prog, "a_uv");
	icon_.u_tex = glGetUniformLocation(icon_.prog, "u_tex");

	return 1;
}

void icon_open(const char *path, void *assets)
{
	if (!make_round_rect(10, .1, &icon_.rect))
		return;

	if (!load_image(path, assets))
		return;

	if (!make_prog())
		return;

	ii("icon init ok, texture %u image '%s'\n", icon_.tex, path);
}

void icon_close(void)
{
	clean_round_rect(&icon_.rect);

	glDeleteTextures(1, &icon_.tex);
	glDeleteProgram(icon_.prog);
}

void icon_render(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_disable_features();

	glBindTexture(GL_TEXTURE_2D, icon_.tex);
	glUseProgram(icon_.prog);
	glVertexAttribPointer(icon_.a_pos, 2, GL_FLOAT, GL_FALSE, 0,
	  icon_.rect.verts);
	glEnableVertexAttribArray(icon_.a_pos);
	glVertexAttribPointer(icon_.a_uv, 2, GL_FLOAT, GL_FALSE, 0,
	  icon_.rect.uvs);
	glEnableVertexAttribArray(icon_.a_uv);

	glDrawElements(GL_TRIANGLE_FAN, icon_.rect.verts_num, GL_UNSIGNED_BYTE,
	  icon_.rect.indices);

	glDisableVertexAttribArray(icon_.a_uv);
	glDisableVertexAttribArray(icon_.a_pos);
	glBindTexture(GL_TEXTURE_2D, 0);

	gl_enable_features();
}
