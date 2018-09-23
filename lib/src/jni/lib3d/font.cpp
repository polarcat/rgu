/* font.c: truetype font rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <unistd.h>

#ifdef ANDROID
#include <android/asset_manager_jni.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define TAG LIB_TAG": ttf"

#include "gl.h"
#include "font.h"

static const GLubyte indices_[] = {
        0, 1, 2,
        2, 3, 0,
};

static const float uvs_[] = {
        0, 1,
        0, 0,
        1, 0,
        1, 1,
};

void font_close(struct font_info *info)
{
#ifdef ANDROID
	AAsset_close((AAsset *) info->asset);
	info->asset = NULL;
#else
	if (info->data_buf) {
		munmap(info->data_buf, info->data_len);
		info->data_buf = NULL;
	}

	if (info->fd > -1) {
		close(info->fd);
		info->fd = -1;
	}
#endif

	free(info->font);
	info->font = NULL;
}

void font_open(struct font_info *info, float size, const char *path)
{
#ifdef ANDROID
	AAsset *asset = AAssetManager_open((AAssetManager *) info->asset_manager,
	  path, AASSET_MODE_BUFFER);

	info->data_buf = (const uint8_t *) AAsset_getBuffer(asset);
	info->data_len = AAsset_getLength(asset);
	info->asset = asset;
#else
	struct stat st;

	if (stat(path, &st) < 0) {
		ee("failed to stat file %s\n", path);
		return;
	}

	if ((info->fd = open(path, O_RDONLY)) < 0) {
		ee("failed to open file %s\n", path);
		return;
	}

	info->data_buf = (uint8_t *) mmap(NULL, st.st_size, PROT_READ,
	  MAP_PRIVATE, info->fd, 0);

	if (!info->data_buf) {
		ee("failed to map file %s\n", path);
		return;
	}
#endif
	if (!(info->font = calloc(1, sizeof(stbtt_fontinfo)))) {
		ee("failed to allocate %zu bytes\n", sizeof(stbtt_fontinfo));
		return;
	}

	stbtt_fontinfo *font = (stbtt_fontinfo *) info->font;

	stbtt_InitFont(font, info->data_buf, 0);

	info->font_size = (uint16_t) ceil(size);
	info->font_scale = stbtt_ScaleForPixelHeight(font, size);

	stbtt_GetFontVMetrics(font, &info->font_ascent, &info->font_descent, 0);

	info->font_ascent *= info->font_scale;
	info->font_descent *= info->font_scale;

	ii("font %s ok: scale %f | ascent %d | descent %d\n", path,
	  info->font_scale, info->font_ascent, info->font_descent);

	const char *fsrc =
		"precision highp float;\n"
		"uniform sampler2D u_tex;\n"
		"varying vec2 v_uv;\n"
		"void main() {\n"
			"gl_FragColor=texture2D(u_tex,v_uv);\n"
		"}\0";

	const char *vsrc =
		"attribute vec2 a_pos;\n"
		"attribute vec2 a_uv;\n"
		"varying vec2 v_uv;\n"
		"void main() {\n"
			"gl_Position=vec4(a_pos,0,1);\n"
			"v_uv=a_uv;\n"
		"}\0";

	if (!(info->prog = gl_make_prog(vsrc, fsrc))) {
		font_close(info);
		return;
	}

	glUseProgram(info->prog);

	info->a_pos = glGetAttribLocation(info->prog, "a_pos");
	info->a_uv = glGetAttribLocation(info->prog, "a_uv");
	info->u_tex = glGetUniformLocation(info->prog, "u_tex");
}

#define spacing(glyph_width, font_size) ((uint8_t) (glyph_width + font_size / 10))

static uint16_t text_width(const struct font_info *info, const char *str, const
  size_t len)
{
	stbtt_fontinfo *font = (stbtt_fontinfo *) info->font;
	int x0, y0, x1, y1;
	uint16_t w = 0;
	const char *str_ptr = str;

	while (str_ptr < str + len) {
		int g = stbtt_FindGlyphIndex(font, (uint8_t) *str_ptr++);

		stbtt_GetGlyphBitmapBox(font, g, info->font_scale,
		  info->font_scale, &x0, &y0, &x1, &y1);
		w += spacing(x1 - x0, info->font_size);
	}

	return w;
}

static uint32_t *glchar(const struct font_info *info, uint32_t code, uint32_t *rgba,
  uint16_t row_len, uint8_t *gbuf_ptr, uint32_t color)
{
	stbtt_fontinfo *font = (stbtt_fontinfo *) info->font;
	int x0, y0, x1, y1;
	int gw;
	int gh;
	int g = stbtt_FindGlyphIndex(font, code);

	stbtt_GetGlyphBitmapBox(font, g, info->font_scale, info->font_scale,
	  &x0, &y0, &x1, &y1);

	gw = x1 - x0;
	gh = y1 - y0;

	int baseline = info->font_ascent + y0;

	stbtt_MakeGlyphBitmap(font, gbuf_ptr, gw, gh, gw, info->font_scale,
	  info->font_scale, g);

	uint8_t *rgba_ptr = (uint8_t *) (rgba + baseline * row_len);

	for (uint16_t row = 0; row < gh; ++row) {
		for (uint16_t col = 0; col < gw; ++col) {
			if (*gbuf_ptr == 0) {
				rgba_ptr += 4;
			} else {
				*rgba_ptr++ = *gbuf_ptr;
				*rgba_ptr++ = *gbuf_ptr;
				*rgba_ptr++ = *gbuf_ptr;
				*rgba_ptr++ = 0xff;
			}

			gbuf_ptr++;
		}

		rgba_ptr = (uint8_t *) (rgba + (row + baseline) * row_len);
	}

	return rgba + spacing(gw, info->font_size);
}

void font_render(const struct font_info *info, const struct text *text)
{
	if (!info->data_buf)
		return;

	uint16_t x = text->x;
	uint16_t y = text->y;
	uint16_t w = text_width(info, text->str, text->len);

	if (w < info->font_size)
		return;

	GLuint tex;
	GLuint bg;
	uint16_t h = info->font_size;
	uint32_t size = w * h * 4;
	uint8_t rgba[size];
	uint32_t *ptr = (uint32_t *) rgba;
	const char *str_ptr = text->str;
	uint8_t glyph[info->font_size * info->font_size];

	utils_fill2d((uint32_t *) rgba, (uint32_t *) (rgba + size), text->bg);

	while (str_ptr < text->str + text->len)
		ptr = glchar(info, *str_ptr++, ptr, w, glyph, text->fg);

	GLint wh[4];
	glGetIntegerv(GL_VIEWPORT, wh);
	float xx;
	float yy;

	if (text->normxy) {
		xx = text->x;
		yy = text->y;
	} else {
		xx = x / (wh[2] * .5) - 1;
		yy = 1 - y / (wh[3] * .5);
	}

	float ww = (float) w / wh[2];
	float hh = (float) h / wh[3];

	float vertices[] = {
		xx, yy - hh,
		xx, yy,
		xx + ww, yy,
		xx + ww, yy - hh,
	};

#if 0
	ii("text %dx%d | "
	   "xx %f, ww %f, yy %f, hh %f | "
	   "v0 %f,%f v1 %f,%f v2 %f,%f v3 %f,%f | "
	   "disp %d,%d\n",
	  w, h,
	  xx, ww, yy, hh,
	  vertices[0], vertices[1],
	  vertices[2], vertices[3],
	  vertices[4], vertices[5],
	  vertices[6], vertices[7],
	  wh[2], wh[3]
	);
#endif

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
	  GL_UNSIGNED_BYTE, rgba);

	glUseProgram(info->prog);

	glUniform1i(info->u_tex, 0);
	glVertexAttribPointer(info->a_pos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(info->a_pos);
	glVertexAttribPointer(info->a_uv, 2, GL_FLOAT, GL_FALSE, 0, uvs_);
	glEnableVertexAttribArray(info->a_uv);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices_);

	glDisableVertexAttribArray(info->a_uv);
	glDisableVertexAttribArray(info->a_pos);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &tex);
}
