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

struct round_rect_req {
    uint8_t roundness;
    float corner_radius;
    float radius_x;
    float radius_y;
    float origin_x;
    float origin_y;
};

struct round_rect {
    uint8_t verts_num;
    union gm_point2 *verts;
    union gm_point2 *uvs;
    uint8_t *indices;
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

uint8_t make_round_rect(uint8_t rn, float r, struct round_rect *rect);
void clean_round_rect(struct round_rect *rect);
