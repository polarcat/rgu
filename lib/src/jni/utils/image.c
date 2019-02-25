/* image.c: image manipulation utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <png.h>
#include <jpeglib.h>

#define TAG "image"

#include "log.h"
#include "image.h"

void fillrect(uint32_t *buf, uint32_t *end, uint32_t color)
{
	while (buf < end) {
		uint8_t *ptr = (uint8_t *) buf++;
		*ptr++ = color >> 24 & 0xff;
		*ptr++ = color >> 16 & 0xff;
		*ptr++ = color >> 8 & 0xff;
		*ptr++ = color & 0xff;
	}
}

void writepng(const char *path, const uint8_t *buf, uint16_t w, uint16_t h)
{
	FILE *fp;
	uint16_t x;
	uint16_t y;
	png_structp png;
	png_infop info;
	png_byte **rows;
	png_byte *row;

	if (!(fp = fopen (path, "wb"))) {
		ee("failed to create %s\n", path);
		return;
	}

	if (!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	  NULL, NULL))) {
		ee("failed to create png write struct\n");
		goto err_png_create_write_struct;
	}

	if (!(info = png_create_info_struct(png))) {
		ee("failed to create png info struct\n");
		goto err_png_create_info_struct;
	}

	if (setjmp(png_jmpbuf(png))) {
		ee("failed to png setjmp\n");
		goto err_png_jumpbuf;
	}

	png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGB,
	  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	  PNG_FILTER_TYPE_DEFAULT);

	if (!(rows = (png_byte **) png_malloc(png, h * sizeof (png_byte *)))) {
		ee("failed to allocate png rows\n");
		goto err_png_rows;
	}

	for (y = 0; y < h; ++y) {
		size_t size = sizeof (uint8_t) * w * 3;

		if (!(row = (png_byte *) png_malloc(png, size))) {
			ee("failed to allocate png row\n");
			goto err_png_row;
		}

		rows[y] = row;

		for (x = 0; x < w; ++x) {
			*row++ = *buf++;
			*row++ = *buf++;
			*row++ = *buf++;
		}
	}

	png_init_io(png, fp);
	png_set_rows(png, info, rows);
	png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);

err_png_row:

	for (y = 0; y < h; y++)
		png_free(png, rows[y]);

	png_free(png, rows);

err_png_rows:
err_png_jumpbuf:
err_png_create_info_struct:
	png_destroy_write_struct(&png, &info);

err_png_create_write_struct:
	fclose (fp);

	ii("written ok %s\n", path);
}

#define PNG_FLAGS (PNG_TRANSFORM_STRIP_16 |\
		   PNG_TRANSFORM_GRAY_TO_RGB |\
		   PNG_TRANSFORM_PACKING |\
		   PNG_TRANSFORM_EXPAND)

static int fd_;

static void pngio(png_structp png, png_bytep data, png_size_t size)
{
	read(fd_, data, size);
}

uint8_t readpng(const char *path, struct image *img)
{
	uint8_t rc;
	uint8_t n;
	int16_t y;
	size_t size;
	uint8_t *dst;
	png_structp png;
	png_infop info;
	png_bytep *rows;

	if ((fd_ = open(path, O_RDONLY)) < 0) {
		ee("open(%s) failed\n", path);
		return 0;
	}

	rc = 0;
	info = NULL;

	if (!(png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0))) {
		ee("png_create_read_struct() failed\n");
		goto out;
	}

        if (!(info = png_create_info_struct(png))) {
		ee("png_create_info_struct() failed\n");
		goto out;
	}

	if (setjmp(png_jmpbuf(png))) {
		ee("setjmp() failed\n");
		goto out;
	}

        png_set_read_fn(png, (void *) &fd_, pngio);
        png_read_png(png, info, PNG_FLAGS, 0);

	if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGBA) {
		n = 4;
		img->format = GL_RGBA;
	} else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB) {
		n = 3;
		img->format = GL_RGB;
	} else {
		ee("unsupported color type, expect RGB\n");
		goto out;
	}

	img->w = png_get_image_width(png, info);
	img->h = png_get_image_height(png, info);
	size = img->w * img->h * n;

	ii("image %ux%u components %u bytes %zu\n", img->w, img->h, n, size);

	if (!(img->data = calloc(1, size))) {
		ee("failed to allocate %zu bytes\n", size);
		goto out;
	}

        rows = png_get_rows(png, info);
	dst = img->data;

	for (y = 0; y < img->h; y++) {
		const uint8_t *row = rows[y];
		const uint8_t *end = row + img->w * n;

		while (row < end) {
			*dst++ = *row++;
			*dst++ = *row++;
			*dst++ = *row++;

			if (n == 4)
				*dst++ = *row++;
		}

		dst = img->data + n * img->w * y;
	}

	rc = 1;

out:
	if (png && info)
		png_destroy_read_struct(&png, &info, 0);

	close(fd_);
	return rc;
}

static void pngerr(png_structp png_ptr, png_const_charp msg)
{
	ee("png: %s\n", msg);
}

static const uint8_t *imgbuf_;

static void pngcpy(png_structp png, png_bytep data, png_size_t size)
{
	memcpy(data, imgbuf_, size);
	imgbuf_ += size;
}

#define PNG_SIGNATURE_LEN 8

uint8_t buf2png(const uint8_t *buf, struct image *img)
{
	uint8_t n;
	int16_t y;
	size_t retsize;
	uint8_t *ret;
	uint8_t *dst;
	png_structp png;
	png_bytep *rows;
	png_infop info = NULL;

	if (!png_check_sig(buf, PNG_SIGNATURE_LEN)) {
		ee("bad PNG signature: %02x %02x %02x %02x %02x %02x %02x %02x\n",
				buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
		return 0;
	}

	ret = NULL;

	if (!(png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, pngerr, 0))) {
		ee("png_create_read_struct() failed\n");
		goto out;
	}

	if (!(info = png_create_info_struct(png))) {
		ee("png_create_info_struct() failed\n");
		goto out;
	}

	imgbuf_ = buf + PNG_SIGNATURE_LEN;

	if (setjmp(png_jmpbuf(png))) {
		ee("error reading png file\n");
		goto out;
	}

	png_set_sig_bytes(png, PNG_SIGNATURE_LEN);
	png_set_read_fn(png, NULL, pngcpy);
	png_read_png(png, info, PNG_FLAGS, 0);

	if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGBA) {
		n = 4;
		img->format = GL_RGBA;
	} else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB) {
		n = 3;
		img->format = GL_RGB;
	} else {
		ee("unsupported color type, expect RGB\n");
		goto out;
	}

	uint16_t w = png_get_image_width(png, info);
	uint16_t h = png_get_image_height(png, info);

	retsize = w * h * n;

	ii("image %ux%u components %u | need %zu bytes\n", w, h, n, retsize);

	if (!(ret = (uint8_t *) calloc(1, retsize)))
		goto out;

	rows = png_get_rows(png, info);

	dst = ret;

	for (y = 0; y < h; y++) {
		const uint8_t *row = rows[y];
		const uint8_t *end = row + w * n;

		while (row < end) {
			*dst++ = *row++;
			*dst++ = *row++;
			*dst++ = *row++;

			if (n == 4)
				*dst++ = *row++;
		}

		dst = ret + n * w * y;
	}

	img->data = ret;
	img->w = w;
	img->h = h;

out:
	if (png && info)
		png_destroy_read_struct(&png, &info, 0);

	return !!ret;
}

struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

static void my_error_exit (j_common_ptr info)
{
	struct my_error_mgr *err = (struct my_error_mgr *) info->err;

	(*info->err->output_message) (info);
	longjmp(err->setjmp_buffer, 1);
}

uint8_t readjpg(const char *path, struct image *img)
{
	uint8_t rc;
	struct my_error_mgr jerr = {0};
	struct jpeg_decompress_struct inf;
	FILE *f;
	JSAMPARRAY buf;
	uint16_t stride;
	size_t size;
	uint8_t *out;
	uint8_t *end;

	if (!(f = fopen(path, "rb"))) {
		ee("fopen(%s) failed\n", path);
		return 0;
	}

	inf.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&inf);
		fclose(f);
		return 0;
	}

	jpeg_create_decompress(&inf);
	jpeg_stdio_src(&inf, f);
	jpeg_read_header(&inf, TRUE);
	jpeg_start_decompress(&inf);

	stride = inf.output_width * inf.output_components;
	size = inf.output_width * inf.output_height * inf.output_components;
	rc = 0;

	ii("image %ux%u components %u stride %u bytes %zu\n", inf.output_width,
	   inf.output_height, inf.output_components, stride, size);

	if (!(img->data = calloc(1, size))) {
		ee("failed to allocate %zu bytes\n", size);
		goto out;
	}

	out = img->data;
	buf = (*inf.mem->alloc_sarray)((j_common_ptr) &inf, JPOOL_IMAGE, stride, 1);

	while (inf.output_scanline < inf.output_height) {
		jpeg_read_scanlines(&inf, buf, 1);

#if 1
		/* convert to grayscale */

		uint8_t *scanptr = buf[0];
		uint8_t *scanend = scanptr + stride;

		while (scanptr < scanend) {
			uint8_t gray = (*scanptr + *(scanptr + 1) + *(scanptr + 2)) / 3;
			*scanptr = *(scanptr + 1) = *(scanptr + 2) = gray;
			scanptr += 3;
		}
#endif

		memcpy(out, buf[0], stride);
		out += stride;
	}

	img->format = GL_RGB;
	img->w = inf.output_width;
	img->h = inf.output_height;
	rc = 1;

out:
	jpeg_finish_decompress(&inf);
	jpeg_destroy_decompress(&inf);
	fclose(f);

	return rc;
}
