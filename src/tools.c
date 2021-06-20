/* tools.c: various tools
 *
 * Copyright (c) 2019 Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <rgu/log.h>
#include <rgu/tools.h>

void clean_round_rect(struct round_rect *rect)
{
	if (rect->alloc) {
		free(rect->verts);
		free(rect->uvs);
		free(rect->indices);
	}

	rect->verts_num = 0;
	rect->verts = NULL;
	rect->uvs = NULL;
	rect->indices = NULL;
	rect->alloc = 0;
}

#define convert_x(x) ((1 + (x)) * .5)
#define convert_y(y) ((1 - (y)) * .5)

uint8_t make_round_rect(float sx, float sy, float r, struct round_rect *rect)
{
	if (r > 1) {
		ww("radius should be in range (0,1); set to 1\n");
		r = 1;
	}

	const uint8_t rn = 3;
	uint8_t step = 90 / rn;
	float cx = sx - r;
	float cy = sy - r;
	uint16_t i;
	uint16_t size;

	rect->alloc = 1;
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
		clean_round_rect(rect);
		return 0;
	}

	if (!(rect->indices = (element_t *) malloc(rect->verts_num))) {
		ee("failed to allocate indices %u bytes\n", rect->verts_num);
		clean_round_rect(rect);
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

	for (uint16_t a = 90; a < 181; a += step) {
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

uint8_t make_round_icon(uint8_t rn, float r, struct round_rect *rect)
{
	if (!rn || r == 0) {
		ww("decline making round rectangle with zero roundness\n");
		return 0;
	}

	uint8_t step = 90 / rn;
	float cx = 1 - r;
	float cy = 1 - r;
	uint16_t i;
	uint16_t size;

	rect->alloc = 1;
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

	if (!(rect->indices = (element_t *) malloc(rect->verts_num))) {
		ee("failed to allocate indices %u bytes\n", rect->verts_num);
		return 0;
	}

	rect->verts[0].x = 0;
	rect->verts[0].y = 0;
	rect->uvs[0].x = .5;
	rect->uvs[0].y = .5;
	rect->indices[0] = 0;

	i = 1;

	for (uint8_t a = 0; a < 91; a += step) {
		rect->verts[i].x = cx + r * cos(radians(a));
		rect->verts[i].y = cy + r * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
	}

	for (uint16_t a = 90; a < 181; a += step) {
		rect->verts[i].x = -cx + r * cos(radians(a));
		rect->verts[i].y = cy + r * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
	}

	for (uint16_t a = 180; a < 271; a += step) {
		rect->verts[i].x = -cx + r * cos(radians(a));
		rect->verts[i].y = -cy + r * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
	}

	for (uint16_t a = 270; a < 361; a += step) {
		rect->verts[i].x = cx + r * cos(radians(a));
		rect->verts[i].y = -cy + r * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
	}

	rect->verts[i].x = rect->verts[1].x;
	rect->verts[i].y = rect->verts[1].y;
	rect->uvs[i].x = rect->uvs[1].x;
	rect->uvs[i].y = rect->uvs[1].y;
	rect->indices[i] = i;

	return 1;
}

uint8_t make_callout(const struct callout_info *info, struct round_rect *rect)
{
	float r = info->roundness;
	if (r > 1) {
		ww("radius should be in range (0,1); set to 1\n");
		r = 1;
	}

	float pin_left = info->pin_left;
	if (pin_left < -1 || pin_left > 1)
		pin_left = -.1;

	float pin_right = info->pin_right;
	if (pin_right < -1 || pin_right > 1)
		pin_right = .1;

	float pin_center = info->pin_center;
	if (pin_center < -1 || pin_center > 1)
		pin_center = (pin_left + pin_right) * .5;

	float pin_height = info->pin_height;
	if (pin_height < -1 || pin_height > 1)
		pin_height = .2;

	const uint8_t rn = 3;
	uint8_t pins = 3;
	uint8_t step = 90 / rn;
	float cx = info->w - r;
	float cy = info->h - r;
	uint16_t i;
	uint16_t size;

	if (info->skew != 0) {
		pins = 0;
	} else if (info->pin_symmetric) {
		pins = 6;
	} else {
		pins = 3;
	}

	rect->alloc = 1;
	rect->verts_num = 4 * (rn + 1) + 1 + 1 + pins; /* +1 for center +1 for closure */
	size = sizeof(*rect->verts) * rect->verts_num;

#ifdef SHOW_STAT
	ii("need %u vertices %u bytes (%zu) | cxy { %.4f %.4f }\n",
	  rect->verts_num, size, sizeof(*rect->verts), cx, cy);
#endif

	if (!(rect->verts = (union gm_point2 *) malloc(size))) {
		ee("failed to allocate vertices %u bytes\n", size);
		return 0;
	}

	if (!(rect->uvs = (union gm_point2 *) malloc(size))) {
		ee("failed to allocate uvs %u bytes\n", size);
		return 0;
	}

	if (!(rect->indices = (element_t *) malloc(rect->verts_num))) {
		ee("failed to allocate indices %u bytes\n", rect->verts_num);
		return 0;
	}

	rect->verts[0].x = 0;
	rect->verts[0].y = 0;
	rect->uvs[0].x = .5;
	rect->uvs[0].y = .5;
	rect->indices[0] = 0;

	i = 1;

	for (uint8_t a = 0; a < 91; a += step) {
		rect->verts[i].x = cx + r * cos(radians(a)) + info->skew;
		rect->verts[i].y = cy + r * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
	}

	for (uint16_t a = 90; a < 181; a += step) {
		rect->verts[i].x = -cx + r * cos(radians(a));
		rect->verts[i].y = cy + r * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;

		if (pins && info->pin_symmetric) {
			float y = rect->verts[i].y;

			if (rect->verts[i].x == -cx && rect->verts[i].y > 0) {
				i++;
				rect->verts[i].x = pin_right;
				rect->verts[i].y = y;
				rect->uvs[i].x = convert_x(rect->verts[i].x);
				rect->uvs[i].y = convert_y(rect->verts[i].y);
				rect->indices[i] = i;

				i++;
				rect->verts[i].x = pin_center;
				if (pin_height == 0)
					rect->verts[i].y = y;
				else
					rect->verts[i].y = cy + pin_height;
				rect->uvs[i].x = convert_x(rect->verts[i].x);
				rect->uvs[i].y = convert_y(rect->verts[i].y);
				rect->indices[i] = i;

				i++;
				rect->verts[i].x = pin_left;
				rect->verts[i].y = y;
				rect->uvs[i].x = convert_x(rect->verts[i].x);
				rect->uvs[i].y = convert_y(rect->verts[i].y);
				rect->indices[i] = i;
			}
		}

		i++;
	}

	for (uint16_t a = 180; a < 271; a += step) {
		rect->verts[i].x = -cx + r * cos(radians(a)) - info->skew;
		rect->verts[i].y = -cy + r * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;

		if (pins) {
			float y = rect->verts[i].y;

			if (rect->verts[i].x == -cx && rect->verts[i].y < 0) {
				i++;
				rect->verts[i].x = pin_left;
				rect->verts[i].y = y;
				rect->uvs[i].x = convert_x(rect->verts[i].x);
				rect->uvs[i].y = convert_y(rect->verts[i].y);
				rect->indices[i] = i;

				i++;
				rect->verts[i].x = pin_center;
				if (pin_height == 0)
					rect->verts[i].y = y;
				else
					rect->verts[i].y = -cy - pin_height;
				rect->uvs[i].x = convert_x(rect->verts[i].x);
				rect->uvs[i].y = convert_y(rect->verts[i].y);
				rect->indices[i] = i;

				i++;
				rect->verts[i].x = pin_right;
				rect->verts[i].y = y;
				rect->uvs[i].x = convert_x(rect->verts[i].x);
				rect->uvs[i].y = convert_y(rect->verts[i].y);
				rect->indices[i] = i;
			}
		}

		i++;
	}

	for (uint16_t a = 270; a < 361; a += step) {
		rect->verts[i].x = cx + r * cos(radians(a));
		rect->verts[i].y = -cy + r * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
	}

	rect->verts[i].x = rect->verts[1].x;
	rect->verts[i].y = rect->verts[1].y;
	rect->uvs[i].x = rect->uvs[1].x;
	rect->uvs[i].y = rect->uvs[1].y;
	rect->indices[i] = i;

	return 1;
}

#define STEP_DEG_MIN 2
#define STEP_DEG_MAX 90

uint8_t make_circle(struct shape *circle, uint8_t step)
{
	if (step < STEP_DEG_MIN || step > STEP_DEG_MAX) {
		step = STEP_DEG_MIN;
	} else {
		while (360 % step)
			step++;
	}

	uint16_t size;

	circle->alloc = 1;
	circle->verts_num = 360 / step + 1;
	size = sizeof(*circle->verts) * circle->verts_num;

	ii("need %u vertices %u bytes (%zu) | step %u | %u\n",
	  circle->verts_num, size, sizeof(*circle->verts), step, 360 / step);

	if (!(circle->verts = (union gm_point2 *) malloc(size))) {
		ee("failed to allocate vertices %u bytes\n", size);
		return 0;
	}

	if (!(circle->uvs = (union gm_point2 *) malloc(size))) {
		ee("failed to allocate uvs %u bytes\n", size);
		return 0;
	}

	circle->indices_num = (circle->verts_num  - 1) * 3;

	if (!(circle->indices = (element_t *) malloc(circle->indices_num))) {
		ee("failed to allocate indices %u bytes\n",
		  circle->indices_num);
		return 0;
	}

	circle->verts[0].x = 0;
	circle->verts[0].y = 0;
	circle->uvs[0].x = .5;
	circle->uvs[0].y = .5;

	dd("v %u (%.4f %.4f) | center\n", 0,
	 circle->verts[0].x, circle->verts[0].y);

	uint16_t n = 1;
	uint16_t i = 0;

	for (uint16_t a = 0; a < 360; a += step) {
		circle->verts[n].x = cos(radians(a));
		circle->verts[n].y = sin(radians(a));
		circle->uvs[n].x = convert_x(circle->verts[n].x);
		circle->uvs[n].y = convert_y(circle->verts[n].y);

		circle->indices[i++] = 0;
		circle->indices[i++] = n;

		if (n >= circle->verts_num - 1)
			circle->indices[i++] = 1;
		else
			circle->indices[i++] = n + 1;

		dd("v %u (%.4f %.4f) | a %u | i %u\n", n,
		  circle->verts[n].x, circle->verts[n].y, a, i);
		n++;
	}

#if 0
	n = 1;

	for (uint16_t i = 0; i < circle->indices_num; i += 3) {
		ii("tri %u (%u %u %u)\n", n++,
		  circle->indices[i + 0],
		  circle->indices[i + 1],
		  circle->indices[i + 2]);
	}
#endif
	return 1;
}

uint8_t make_rrect(float rx, float ry, uint8_t steps, struct round_rect *rect)
{
	uint16_t limit = 4 * (steps + 1) + 1 + 1;

	if (limit >= UINT8_MAX) {
		ee("too many vertices %u\n", limit);
		return 0;
	}

	if (rx < .01 || rx > .99)
		rx = .1;

	if (ry < .01 || ry > .99)
		ry = .1;

	float step = 90. / steps;
	float cx = 1 - rx;
	float cy = 1 - ry;
	uint16_t size;

	rect->alloc = 1;
	rect->verts_num = limit;

	size = sizeof(*rect->verts) * rect->verts_num;

	ii("need %f/%u steps %u vertices %u bytes (%zu) | cxy { %.4f %.4f }\n",
	  step, steps, rect->verts_num, size, sizeof(*rect->verts), cx, cy);

	if (!(rect->verts = (union gm_point2 *) malloc(size))) {
		ee("failed to allocate vertices %u bytes\n", size);
		return 0;
	}

	if (!(rect->uvs = (union gm_point2 *) malloc(size))) {
		ee("failed to allocate uvs %u bytes\n", size);
		return 0;
	}

	size = sizeof(*rect->indices) * rect->verts_num;

	dd("%u indices %u bytes (%zu)\n", rect->verts_num, size,
	  sizeof(*rect->indices));

	if (!(rect->indices = (element_t *) malloc(size))) {
		ee("failed to allocate indices %u bytes\n", size);
		return 0;
	}

	rect->verts[0].x = 0;
	rect->verts[0].y = 0;
	rect->uvs[0].x = .5;
	rect->uvs[0].y = .5;
	rect->indices[0] = 0;

	element_t i = 1;
	float a = 0;

	while (a <= 90) {
		rect->verts[i].x = cx + rx * cos(radians(a));
		rect->verts[i].y = cy + ry * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
		a += step;
	}

	a = 90;
	while (a <= 180) {
		rect->verts[i].x = -cx + rx * cos(radians(a));
		rect->verts[i].y = cy + ry * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
		a += step;
	}

	a = 180;
	while (a <= 270) {
		rect->verts[i].x = -cx + rx * cos(radians(a));
		rect->verts[i].y = -cy + ry * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
		a += step;
	}

	a = 270;
	while (a <= 360) {
		rect->verts[i].x = cx + rx * cos(radians(a));
		rect->verts[i].y = -cy + ry * sin(radians(a));
		rect->uvs[i].x = convert_x(rect->verts[i].x);
		rect->uvs[i].y = convert_y(rect->verts[i].y);
		rect->indices[i] = i;
		i++;
		a += step;
	}

	rect->verts[i].x = rect->verts[1].x;
	rect->verts[i].y = rect->verts[1].y;
	rect->uvs[i].x = rect->uvs[1].x;
	rect->uvs[i].y = rect->uvs[1].y;
	rect->indices[i] = i;

	return 1;
}

void round_rect_extents(struct round_rect *shape, union gm_point2 *extents)
{
	extents->x = 0;
	extents->y = 0;

	for (uint16_t i = 0; i < shape->verts_num; ++i) {
		if (extents->x < shape->verts[i].x)
			extents->x = shape->verts[i].x;

		if (extents->y < shape->verts[i].y)
			extents->y = shape->verts[i].y;
	}
}
