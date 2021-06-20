/* wfobj.h: rudimentary wavefront object loader
 *
 * Copyright (c) 2019 Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#pragma once

#include <stdint.h>

#include <rgu/gl.h>
#include <rgu/gm.h>
#include <rgu/color.h>
#include <rgu/list.h>

#define ARRAY_STRIDE_COLOR 44
#define ARRAY_STRIDE 32

struct wfobj {
	uint16_t id;
	uint8_t visible;
	char *name;

	GLuint tex;
	char *texname;

	GLuint ibo;
	uint16_t *indices;
	uint32_t indices_num;

	GLuint vbo;
	float *array;
	uint32_t array_size;
	uint8_t with_color;
	union color_rgb color; /* voxel object color */

	struct list_head head;
};

struct model {
	char *name;
	struct list_head shapes;
	union color_rgb rgb; /* if all planes are >= 0 then ignore mtl and use this color */
	union gm_point3 min;
	union gm_point3 max;
	uint8_t ignore_texture;
	void *ctx; /* privately owned context */
};

uint8_t prepare_model(char *path, struct model *model, void *amgr);
void upload_model(struct model *model);
void erase_model(struct model *model);
