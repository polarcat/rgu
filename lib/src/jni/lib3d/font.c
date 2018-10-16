/* font.c: truetype font rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

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
#include "font.h"

enum {
	TEXT_DISPLAYED,
	TEXT_DISPLAY,
	TEXT_PREPARED,
	TEXT_PREPARE,
};

struct bitmap {
	uint8_t *data;
	uint32_t size;
	uint16_t w;
};

#define TEXT_QUEUE_SIZE 4

struct text {
	uint8_t state;
	char *str;
	uint16_t len;
	float x;
	float y;
	uint32_t fg;
	uint32_t bg;
	struct bitmap bitmap;
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

	pthread_t thread;
	sem_t prepare;

	pthread_mutex_t text_lock;
	struct text text[TEXT_QUEUE_SIZE];
	struct text *text_end;

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

static uint32_t *glchar(const struct font *font, uint32_t code, uint32_t *rgba,
  uint16_t row_len, uint8_t *gbuf_ptr, uint32_t color)
{
	int x0, y0, x1, y1;
	int gw;
	int gh;
	int g = stbtt_FindGlyphIndex(&font->stbtt, code);

	stbtt_GetGlyphBitmapBox(&font->stbtt, g, font->scale, font->scale,
	  &x0, &y0, &x1, &y1);

	gw = x1 - x0;
	gh = y1 - y0;

	int baseline = font->ascent + y0;

	stbtt_MakeGlyphBitmap(&font->stbtt, gbuf_ptr, gw, gh, gw, font->scale,
	  font->scale, g);

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

	return rgba + spacing(gw, font->size);
}

static void prepare_text(struct font *font, struct text *text)
{
	uint16_t w;
	uint32_t size;
	uint8_t *bitmap;

	w = text_width(font, text->str, text->len);

	if (w < font->size) {
		text->state = TEXT_DISPLAYED; /* init again */
		return;
	}

	size = font->size * w * 4;

	if (size != text->bitmap.size) {
		free(text->bitmap.data);

		if (!(text->bitmap.data = (uint8_t *) malloc(size))) {
			text->state = TEXT_DISPLAYED;
			return;
		}

		text->bitmap.size = size;
		text->bitmap.w = w;
	}

	uint32_t *buf = (uint32_t *) text->bitmap.data;
	const char *str = text->str;
	uint8_t glyph[font->size * font->size];

	fillrect(buf, (uint32_t *) (text->bitmap.data + size), text->bg);

	while (str < text->str + text->len)
		buf = glchar(font, *str++, buf, w, glyph, text->fg);

	text->state = TEXT_PREPARED;
}

static void *thread_work(void *arg)
{
	struct font *font = (struct font *) arg;
	struct text *text_cur;
	uint8_t found;

	while (1) {
		found = 0;
		text_cur = font->text;
		sem_wait(&font->prepare);

		pthread_mutex_lock(&font->text_lock);

		do {
			if (text_cur->state == TEXT_PREPARE) {
				found = 1;
				break;
			}
		} while (++text_cur < font->text_end);

		pthread_mutex_unlock(&font->text_lock);

		if (found)
			prepare_text(font, text_cur);
	}

	return NULL;
}

void font_close(struct font **ptr)
{
	struct font *font = *ptr;
#ifdef ANDROID
	AAsset_close((AAsset *) font->asset);
	font->asset = NULL;
#else
	if (font->data_buf) {
		munmap(font->data_buf, font->data_len);
		font->data_buf = NULL;
	}

	if (font->fd > -1) {
		close(font->fd);
		font->fd = -1;
	}
#endif
	glDeleteTextures(1, &font->tex);

	for (uint8_t i = 0; i < TEXT_QUEUE_SIZE; ++i) {
		free(font->text[i].bitmap.data);
		free(font->text[i].str);
	}

	free(font);
	*ptr = NULL;
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

	return 1;
}

struct font *font_open(const char *path, float size, void *assets)
{
	struct font *font = (struct font *) calloc(1, sizeof(*font));

	if (!font) {
		ee("failed to allocate %zu bytes\n", sizeof(*font));
		return NULL;
	}

	pthread_mutex_init(&font->text_lock, NULL);

#ifdef ANDROID
	AAsset *asset;

	if (!assets) {
		ee("bad assets manager\n");
		goto err;
	}

	asset = AAssetManager_open((AAssetManager *) assets, path,
	  AASSET_MODE_BUFFER);

	if (!asset) {
		ee("failed to open assets\n");
		goto err;
	}

	font->data_buf = (const uint8_t *) AAsset_getBuffer(asset);
	font->data_len = AAsset_getLength(asset);
	font->asset = asset;
#else
	struct stat st;

	if (stat(path, &st) < 0) {
		ee("failed to stat file %s\n", path);
		goto err;
	}

	if ((font->fd = open(path, O_RDONLY)) < 0) {
		ee("failed to open file %s\n", path);
		goto err;
	}

	font->data_buf = (uint8_t *) mmap(NULL, st.st_size, PROT_READ,
	  MAP_PRIVATE, font->fd, 0);

	if (!font->data_buf) {
		ee("failed to map file %s\n", path);
		goto err;
	}
#endif

	if (!prepare_program(font))
		goto err;

	stbtt_InitFont(&font->stbtt, font->data_buf, 0);

	font->size = (uint16_t) ceil(size);
	font->scale = stbtt_ScaleForPixelHeight(&font->stbtt, size);

	stbtt_GetFontVMetrics(&font->stbtt, &font->ascent, &font->descent, 0);

	font->ascent *= font->scale;
	font->descent *= font->scale;

	ii("font %s ok: scale %f | ascent %d | descent %d\n", path,
	  font->scale, font->ascent, font->descent);

	font->text_end = font->text + TEXT_QUEUE_SIZE;

	sem_init(&font->prepare, 0, 0);
	pthread_create(&font->thread, NULL, thread_work, font);

	return font;

err:
	font_close(&font);
	return NULL;
}

void font_render(struct font *font, const char *str, uint16_t len,
  float x, float y, uint32_t fg, uint32_t bg)
{
	float verts[8];
	float norm_w;
	float norm_h;
	GLint wh[4];
	struct text *text_ptr = font->text;
	struct text *text_cur = NULL;

	pthread_mutex_lock(&font->text_lock);

	do {
		if (text_ptr->state == TEXT_PREPARED) {
			if (!text_cur) {
				text_ptr->state = TEXT_DISPLAY;
				text_cur = text_ptr;
			}
		} else if (text_ptr->state == TEXT_DISPLAYED) {
			if (text_ptr->len == len) {
				memcpy(text_ptr->str, str, len);
			} else {
				free(text_ptr->str);
				text_ptr->str = strdup(str);
			}

			text_ptr->len = len;
			text_ptr->fg = fg;
			text_ptr->bg = bg;
			text_ptr->state = TEXT_PREPARE;
		}
	} while (++text_ptr < font->text_end);

	pthread_mutex_unlock(&font->text_lock);

	int val = 1; /* to skip error checking */

	sem_getvalue(&font->prepare, &val);

	if (!val)
		sem_post(&font->prepare);

	if (!text_cur)
		return;

	glGetIntegerv(GL_VIEWPORT, wh);

	norm_w = (float) text_cur->bitmap.w / wh[2];
	norm_h = (float) font->size / wh[3];

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
	  text_cur->str,
	  text_cur, text_cur->bitmap.data,
	  text_cur->bitmap.w, font->size,
	  norm_w, norm_h,
	  verts[0], verts[1],
	  verts[2], verts[3],
	  verts[4], verts[5],
	  verts[6], verts[7],
	  wh[2], wh[3]
	);
#endif

	glBindTexture(GL_TEXTURE_2D, font->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, text_cur->bitmap.w, font->size,
	  0, GL_RGBA, GL_UNSIGNED_BYTE, text_cur->bitmap.data);

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

	text_cur->state = TEXT_DISPLAYED;
}
