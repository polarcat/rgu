/* tools.c: various tools
 *
 * Copyright (c) 2019 Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <utils/log.h>

#include "tools.h"

uint8_t make_round_rect(uint8_t rn, float r, struct round_rect *rect)
{
	if (!rn || r == 0) {
		ww("decline making round rectangle with zero roundness\n");
		return 0;
	}

	uint8_t step = 90 / rn;
	float cx = .5 - r;
	float cy = .5 - r;
	uint16_t i;
	uint16_t size;

	rect->verts_num = 4 * (rn + 1) + 1 + 1; /* +1 for center +1 for closure */
	size = sizeof(*rect->verts) * rect->verts_num;

	ii("need %u vertices %u bytes (%zu) | cxy { %.4f %.4f }\n",
	  rect->verts_num, size, sizeof(*rect->verts), cx, cy);

	if (!(rect->verts = (union gm_point2 *) malloc(size))) {
		ee("failed to allocate vertices %u bytes\n", size);
		return 0;
	}

	if (!(rect->uvs = (union gm_point2 *) malloc(size))) {
		ee("failed to allocate uvs %u bytes\n", size);
		return 0;
	}

	if (!(rect->indices = (uint8_t *) malloc(rect->verts_num))) {
		ee("failed to allocate indices %u bytes\n", rect->verts_num);
		return 0;
	}

	rect->verts[0].x = 0;
	rect->verts[0].y = 0;
	rect->indices[0] = 0;

	i = 1;

	for (uint8_t a = 0; a < 91; a += step) {
		rect->verts[i].x = cx + r * cos(radians(a));
		rect->verts[i].y = cy + r * sin(radians(a));
		rect->indices[i] = i;
		i++;
	}

	for (uint8_t a = 90; a < 181; a += step) {
		rect->verts[i].x = -cx + r * cos(radians(a));
		rect->verts[i].y = cy + r * sin(radians(a));
		rect->indices[i] = i;
		i++;
	}

	for (uint16_t a = 180; a < 271; a += step) {
		rect->verts[i].x = -cx + r * cos(radians(a));
		rect->verts[i].y = -cy + r * sin(radians(a));
		rect->indices[i] = i;
		i++;
	}

	for (uint16_t a = 270; a < 361; a += step) {
		rect->verts[i].x = cx + r * cos(radians(a));
		rect->verts[i].y = -cy + r * sin(radians(a));
		rect->indices[i] = i;
		i++;
	}

	rect->verts[i].x = rect->verts[1].x;
	rect->verts[i].y = rect->verts[1].y;
	rect->indices[i] = i;

	/* calculate uv coordinates */

	rect->uvs[0].x = .5;
	rect->uvs[0].y = .5;

	i = 1;

	for (uint16_t a = 360; a > 269; a -= step) {
		rect->uvs[i].x = 1 - r + r * cos(radians(a));
		rect->uvs[i].y = r + r * sin(radians(a));
		i++;
	}

	for (uint16_t a = 270; a > 179; a -= step) {
		rect->uvs[i].x = r + r * cos(radians(a));
		rect->uvs[i].y = r + r * sin(radians(a));
		i++;
	}

	for (uint8_t a = 180; a > 89; a -= step) {
		rect->uvs[i].x = r + r * cos(radians(a));
		rect->uvs[i].y = 1 - r + r * sin(radians(a));
		i++;
	}

	for (int8_t a = 90; a > -1; a -= step) {
		rect->uvs[i].x = 1 - r + r * cos(radians(a));
		rect->uvs[i].y = 1 - r + r * sin(radians(a));
		i++;
	}

	rect->uvs[i].x = rect->uvs[1].x;
	rect->uvs[i].y = rect->uvs[1].y;

	return 1;
}

void clean_round_rect(struct round_rect *rect)
{
	free(rect->verts);
	rect->verts = NULL;

	free(rect->uvs);
	rect->uvs = NULL;

	free(rect->indices);
	rect->indices = NULL;
}
