/* tools.h: various tools
 *
 * Copyright (c) 2019 Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#pragma once

#include <stdint.h>

#include "gm.h"

union geometry {
	int data[4];
	struct {
		int x;
		int y;
		int w;
		int h;
	};
};

struct callout_info {
	float roundness;
	float w;
	float h;
	float pin_height;
	float pin_left;
	float pin_center;
	float pin_right;
	uint8_t pin_symmetric:1;
	float skew;
};

#define ELEMENTS_TYPE GL_UNSIGNED_BYTE
typedef uint8_t element_t;

struct round_rect {
	element_t verts_num;
	union gm_point2 *verts;
	union gm_point2 *uvs;
	element_t *indices;
	uint8_t alloc:1;
};

struct shape {
	element_t verts_num;
	union gm_point2 *verts;
	union gm_point2 *uvs;
	element_t *indices;
	element_t indices_num;
	uint8_t alloc:1;
};

/*
 * make unit rectangle with 0,0 origin
 *
 * @arg rn    roundness > 0
 * @arg r     radius > 0
 * @arg rect  prepared structure with vertices, uvs and indices
 * @ret       1 upon success, 0 on failure
 *
 * */

uint8_t make_round_rect(float sx, float sy, float r, struct round_rect *);
uint8_t make_round_icon(uint8_t rn, float r, struct round_rect *);
uint8_t make_callout(const struct callout_info *, struct round_rect *);
void clean_round_rect(struct round_rect *);

uint8_t make_circle(struct shape *, uint8_t step);
uint8_t make_rrect(float rx, float ry, uint8_t steps, struct round_rect *);

void round_rect_extents(struct round_rect *, union gm_point2 *);
