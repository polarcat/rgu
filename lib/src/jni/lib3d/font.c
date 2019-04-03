/* font.c: truetype font rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <ctype.h>
#include <unistd.h>

#ifdef ANDROID
#include <android/asset_manager_jni.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#define TAG "ttf"

#include <utils/image.h>
#include <utils/time.h>
#include <utils/list.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "gl.h"
#include "gm.h"
#include "font.h"

struct text {
	uint8_t state;
	char *str;
	uint16_t len;
	uint32_t fg;
	uint32_t bg;
	struct bitmap bitmap;
};

struct glyph {
	uint32_t code;
	uint8_t *bmp;
	int16_t w;
	int16_t h;
	int16_t bl; /* baseline */
};

struct font {
	stbtt_fontinfo stbtt;

	uint32_t prog;
	int a_pos;
	int a_uv;
	int u_tex;
	GLuint tex;
	int fd;
	const uint8_t *data_buf;
	size_t data_len;
	int ascent;
	int descent;
	float scale;
	uint16_t size;

	struct text text;

	struct glyph *glyphs;
	uint16_t glyphs_num;
#ifdef ANDROID
	void *asset;
#endif
};

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

#define spacing(glyph_width, size) ((uint8_t) (glyph_width + size / 10))

static uint16_t text_width(struct font *font, const char *str, const size_t len)
{
	int x0, y0, x1, y1;
	uint16_t w = 0;
	const char *str_ptr = str;

	while (str_ptr < str + len) {
		int g = stbtt_FindGlyphIndex(&font->stbtt, (uint8_t) *str_ptr++);

		stbtt_GetGlyphBitmapBox(&font->stbtt, g, font->scale,
		  font->scale, &x0, &y0, &x1, &y1);
		w += spacing(x1 - x0, font->size);
	}

	return w;
}

static uint8_t prepare_program(struct font *font)
{
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

	if (!(font->prog = gl_make_prog(vsrc, fsrc)))
		return 0;

	glUseProgram(font->prog);

	font->a_pos = glGetAttribLocation(font->prog, "a_pos");
	font->a_uv = glGetAttribLocation(font->prog, "a_uv");
	font->u_tex = glGetUniformLocation(font->prog, "u_tex");

	glGenTextures(1, &font->tex);
	glBindTexture(GL_TEXTURE_2D, font->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return 1;
}

static void render_text(struct font *font, struct text *text, float x, float y)
{
	float verts[8];
	GLint wh[4];

	glGetIntegerv(GL_VIEWPORT, wh);

	float norm_w = (float) text->bitmap.w / wh[2];
	float norm_h = (float) text->bitmap.h / wh[3];

	verts[0] = x;
	verts[1] = y - norm_h;

	verts[2] = x;
	verts[3] = y;

	verts[4] = x + norm_w;
	verts[5] = y;

	verts[6] = x + norm_w;
	verts[7] = y - norm_h;

#if 0
	ii("%s |"
	   "text %p bitmap %p | wh %ux%u | "
	   "norm wh %f,%f | "
	   "v0 %f,%f v1 %f,%f v2 %f,%f v3 %f,%f | "
	   "disp %d,%d\n",
	  text->str,
	  text, text->bitmap.data,
	  text->bitmap.w, font->size,
	  norm_w, norm_h,
	  verts[0], verts[1],
	  verts[2], verts[3],
	  verts[4], verts[5],
	  verts[6], verts[7],
	  wh[2], wh[3]
	);
#endif

	glBindTexture(GL_TEXTURE_2D, font->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, text->bitmap.w, text->bitmap.h,
	  0, GL_RGBA, GL_UNSIGNED_BYTE, text->bitmap.data);

	glUseProgram(font->prog);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(font->u_tex, 0);
	glVertexAttribPointer(font->a_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glEnableVertexAttribArray(font->a_pos);
	glVertexAttribPointer(font->a_uv, 2, GL_FLOAT, GL_FALSE, 0, uvs_);
	glEnableVertexAttribArray(font->a_uv);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices_);

	glDisableVertexAttribArray(font->a_uv);
	glDisableVertexAttribArray(font->a_pos);

	glBindTexture(GL_TEXTURE_2D, 0);

	free(text->bitmap.data);
	text->bitmap.data = NULL;
}

static void release_buffer(int fd, const unsigned char *buf, size_t len,
  void *assets)
{
#ifdef ANDROID
	if (assets)
		AAsset_close((AAsset *) assets);
#else
	if (buf)
		munmap((void *) buf, len);

	close(fd);
#endif
}

void close_font(struct font **ptr)
{
	struct font *font = *ptr;

	if (!font)
		return;

	glDeleteProgram(font->prog);
	glDeleteTextures(1, &font->tex);

	if (font->glyphs) {
		for (uint8_t i = 0; i < font->glyphs_num; ++i) {
			if (font->glyphs[i].bmp)
				free(font->glyphs[i].bmp);
		}

		free(font->glyphs);
	}

	free(font);
	*ptr = NULL;
}

static inline void prepare_glyph(struct font *font, uint32_t code, struct glyph *glyph)
{
	union gm_point2i min;
	union gm_point2i max;
	int g = stbtt_FindGlyphIndex(&font->stbtt, code);

	stbtt_GetGlyphBitmapBox(&font->stbtt, g, font->scale, font->scale,
	  &min.x, &min.y, &max.x, &max.y);

	glyph->w = max.x - min.x;
	glyph->h = max.y - min.y;
	glyph->bl = font->ascent + min.y;
	glyph->code = code;

	stbtt_MakeGlyphBitmap(&font->stbtt, glyph->bmp, glyph->w, glyph->h,
	  glyph->w, font->scale, font->scale, g);
}

static inline uint8_t prepare_glyphs(struct font *font, const uint32_t *codes,
  uint8_t codes_num)
{
	uint16_t size = sizeof(*font->glyphs) * codes_num;

	ii("allocate %u ? %zu\n", size, sizeof(struct glyph) * codes_num);

	if (!(font->glyphs = calloc(1, size))) {
		ee("failed to allocate %u bytes\n", size);
		return 0;
	}

	font->glyphs_num = codes_num;
	size = font->size * font->size;

	for (uint8_t i = 0; i < codes_num; ++i) {
		if (!(font->glyphs[i].bmp = calloc(1, size))) {
			ee("failed to allocate %u bytes\n", size);
			return 0;
		}

		prepare_glyph(font, codes[i], &font->glyphs[i]);
	}

	ii("prepared %u glyphs\n", codes_num);
	return 1;
}

struct font *open_font(const char *path, float font_size, const uint32_t *codes,
  uint8_t codes_num, void *assets)
{
	struct font *font = (struct font *) calloc(1, sizeof(*font));

	if (!font) {
		ee("failed to allocate %zu bytes\n", sizeof(*font));
		return NULL;
	}

	const unsigned char *buf = NULL;
	size_t len = 0;
	int fd = -1;

#ifdef ANDROID
	AAsset *asset;

	if (!assets) {
		ee("bad assets manager\n");
		goto err;
	}

	asset = AAssetManager_open((AAssetManager *) assets, path,
	  AASSET_MODE_BUFFER);

	if (!asset) {
		ee("failed to open assets %s\n", path);
		goto err;
	}

	buf = (const unsigned char *) AAsset_getBuffer(asset);
	len = AAsset_getLength(asset);
	font->asset = asset;
#else
	struct stat st;

	if (stat(path, &st) < 0) {
		ee("failed to stat file %s\n", path);
		goto err;
	}

	if ((fd = open(path, O_RDONLY)) < 0) {
		ee("failed to open file %s\n", path);
		goto err;
	}

	len = st.st_size;
	buf = (uint8_t *) mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);

	if (!buf) {
		ee("failed to map file %s\n", path);
		goto err;
	}
#endif
	if (!prepare_program(font))
		goto err;

	stbtt_InitFont(&font->stbtt, buf, 0);

	font->size = (uint16_t) ceil(font_size);
	font->scale = stbtt_ScaleForPixelHeight(&font->stbtt, font->size);

	stbtt_GetFontVMetrics(&font->stbtt, &font->ascent, &font->descent, 0);

	font->ascent *= font->scale;
	font->descent *= font->scale;

	ii("font %s ok: scale %f | ascent %d | descent %d\n", path,
	  font->scale, font->ascent, font->descent);

	if (!prepare_glyphs(font, codes, codes_num))
		goto err;

	return font;

err:
	release_buffer(fd, buf, len, assets);
	close_font(&font);
	return NULL;
}

static struct glyph *lookup_glyph(const struct font *font, uint32_t code)
{
	for (uint8_t i = 0; i < font->glyphs_num; ++i) {
		if (font->glyphs[i].code == code)
			return &font->glyphs[i];
	}

	return NULL;
}

static uint32_t *prepare_char(const struct font *font, uint32_t code,
  uint32_t *rgba, uint16_t row_len)
{
	if (!code)
		return rgba;

	struct glyph *glyph = lookup_glyph(font, code);

	if (!glyph) {
		ii("glyph for code %u '%c' not found\n", code, (char) code);
		return rgba; /* skip symbol */
	}

	uint8_t *rgba_ptr = (uint8_t *) (rgba + glyph->bl * row_len);
	uint8_t *bmp = glyph->bmp;

	for (uint16_t row = 0; row < glyph->h; ++row) {
		for (uint16_t col = 0; col < glyph->w; ++col) {
			if (*bmp == 0) {
				rgba_ptr += 4;
			} else {
				*rgba_ptr++ = *bmp;
				*rgba_ptr++ = *bmp;
				*rgba_ptr++ = *bmp;
				*rgba_ptr++ = 0xff;
			}

			bmp++;
		}

		rgba_ptr = (uint8_t *) (rgba + (row + glyph->bl) * row_len);
	}

	return rgba + spacing(glyph->w, font->size);
}

static void prepare_text(struct font *font, struct text *text)
{
	uint16_t w = text_width(font, text->str, text->len);
	uint32_t size = font->size * w * 4;

	if (!(text->bitmap.data = (uint8_t *) malloc(size)))
		return;

	text->bitmap.size = size;
	text->bitmap.w = w;
	text->bitmap.h = font->size;

	uint32_t *buf = (uint32_t *) text->bitmap.data;
	const char *str = text->str;

	fillrect(buf, (uint32_t *) (text->bitmap.data + size), text->bg);

	while (str < text->str + text->len)
		buf = prepare_char(font, *str++, buf, w);
}

void draw_text(struct font *font, const char *str, uint16_t len, float x,
  float y, uint32_t fg, uint32_t bg, struct bitmap *bmp)
{
	struct text text = {
		.str = (char *) str,
		.len = len,
		.fg = fg,
		.bg = bg,
	};

	prepare_text(font, &text);

	if (bmp) {
		bmp->data = text.bitmap.data;
		bmp->size = text.bitmap.size;
		bmp->w = text.bitmap.w;
		bmp->h = text.bitmap.h;
		return;
	}

	render_text(font, &text, x, y);
}

#define MAX_CHARS (0x7f - 0x20)

const uint32_t *default_codes(uint8_t *codes_num)
{
	uint32_t *codes;

	if (!(codes = calloc(1, MAX_CHARS * sizeof(*codes))))
		return 0;

	*codes_num = 0;

	for (uint8_t i = 0x20; i < 0x7f; ++i)
		codes[(*codes_num)++] = i;

	ii("prepared %u printable characters at %p\n", *codes_num, codes);
	return codes;
}
