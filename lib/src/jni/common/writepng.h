/* writepng.h: write rgba to png
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static void write_png(const char *path, const uint8_t *buf, uint16_t w, uint16_t h)
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
			buf++;
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
