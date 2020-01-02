/* gm.c: graphics math helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#include <stdlib.h>

#define TAG "math"

#include <rgu/log.h>
#include <rgu/gm.h>

/* column-major matrix4 ops */

void gm_mat4_identity(float m[16])
{
	m[0] = m[5] = m[10] = m[15] = 1;
	m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = 0;
	m[12] = m[13] = m[14] = 0;
}

#define m4e(r, e1, e2, e3, e4, e5, e6, e7, e8)\
	r = e1 * e2 + e3 * e4 + e5 * e6 + e7 * e8

void gm_mat4_mulmm(float r[16], const float m0[16], const float m1[16])
{
	m4e(r[0], m0[0], m1[0], m0[1], m1[4], m0[2], m1[8], m0[3], m1[12]);
	m4e(r[1], m0[0], m1[1], m0[1], m1[5], m0[2], m1[9], m0[3], m1[13]);
	m4e(r[2], m0[0], m1[2], m0[1], m1[6], m0[2], m1[10], m0[3], m1[14]);
	m4e(r[3], m0[0], m1[3], m0[1], m1[7], m0[2], m1[11], m0[3], m1[15]);
	m4e(r[4], m0[4], m1[0], m0[5], m1[4], m0[6], m1[8], m0[7], m1[12]);
	m4e(r[5], m0[4], m1[1], m0[5], m1[5], m0[6], m1[9], m0[7], m1[13]);
	m4e(r[6], m0[4], m1[2], m0[5], m1[6], m0[6], m1[10], m0[7], m1[14]);
	m4e(r[7], m0[4], m1[3], m0[5], m1[7], m0[6], m1[11], m0[7], m1[15]);
	m4e(r[8], m0[8], m1[0], m0[9], m1[4], m0[10], m1[8], m0[11], m1[12]);
	m4e(r[9], m0[8], m1[1], m0[9], m1[5], m0[10], m1[9], m0[11], m1[13]);
	m4e(r[10], m0[8], m1[2], m0[9], m1[6], m0[10], m1[10], m0[11], m1[14]);
	m4e(r[11], m0[8], m1[3], m0[9], m1[7], m0[10], m1[11], m0[11], m1[15]);
	m4e(r[12], m0[12], m1[0], m0[13], m1[4], m0[14], m1[8], m0[15], m1[12]);
	m4e(r[13], m0[12], m1[1], m0[13], m1[5], m0[14], m1[9], m0[15], m1[13]);
	m4e(r[14], m0[12], m1[2], m0[13], m1[6], m0[14], m1[10], m0[15], m1[14]);
	m4e(r[15], m0[12], m1[3], m0[13], m1[7], m0[14], m1[11], m0[15], m1[15]);
}

#undef m4e

void gm_mat4_mulmv(float r[4], const float m[16], const float v[4])
{
	r[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
	r[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
	r[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
	r[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
}

void gm_mat4_invert(float r[16], const float m[16])
{
	float inv[16];
	float det;
	int i;

	inv[0] = m[5] * m[10] * m[15] -
	  m[5] * m[11] * m[14] -
	  m[9] * m[6] * m[15] +
	  m[9] * m[7] * m[14] +
	  m[13] * m[6] * m[11] -
	  m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
	  m[4] * m[11] * m[14] +
	  m[8] * m[6] * m[15] -
	  m[8] * m[7] * m[14] -
	  m[12] * m[6] * m[11] +
	  m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
	  m[4] * m[11] * m[13] -
	  m[8] * m[5] * m[15] +
	  m[8] * m[7] * m[13] +
	  m[12] * m[5] * m[11] -
	  m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
	  m[4] * m[10] * m[13] +
	  m[8] * m[5] * m[14] -
	  m[8] * m[6] * m[13] -
	  m[12] * m[5] * m[10] +
	  m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
	  m[1] * m[11] * m[14] +
	  m[9] * m[2] * m[15] -
	  m[9] * m[3] * m[14] -
	  m[13] * m[2] * m[11] +
	  m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
	  m[0] * m[11] * m[14] -
	  m[8] * m[2] * m[15] +
	  m[8] * m[3] * m[14] +
	  m[12] * m[2] * m[11] -
	  m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
	  m[0] * m[11] * m[13] +
	  m[8] * m[1] * m[15] -
	  m[8] * m[3] * m[13] -
	  m[12] * m[1] * m[11] +
	  m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
	  m[0] * m[10] * m[13] -
	  m[8] * m[1] * m[14] +
	  m[8] * m[2] * m[13] +
	  m[12] * m[1] * m[10] -
	  m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
	  m[1] * m[7] * m[14] -
	  m[5] * m[2] * m[15] +
	  m[5] * m[3] * m[14] +
	  m[13] * m[2] * m[7] -
	  m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
	  m[0] * m[7] * m[14] +
	  m[4] * m[2] * m[15] -
	  m[4] * m[3] * m[14] -
	  m[12] * m[2] * m[7] +
	  m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
	  m[0] * m[7] * m[13] -
	  m[4] * m[1] * m[15] +
	  m[4] * m[3] * m[13] +
	  m[12] * m[1] * m[7] -
	  m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
	  m[0] * m[6] * m[13] +
	  m[4] * m[1] * m[14] -
	  m[4] * m[2] * m[13] -
	  m[12] * m[1] * m[6] +
	  m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
	  m[1] * m[7] * m[10] +
	  m[5] * m[2] * m[11] -
	  m[5] * m[3] * m[10] -
	  m[9] * m[2] * m[7] +
	  m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
	  m[0] * m[7] * m[10] -
	  m[4] * m[2] * m[11] +
	  m[4] * m[3] * m[10] +
	  m[8] * m[2] * m[7] -
	  m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
	  m[0] * m[7] * m[9] +
	  m[4] * m[1] * m[11] -
	  m[4] * m[3] * m[9] -
	  m[8] * m[1] * m[7] +
	  m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
	  m[0] * m[6] * m[9] -
	  m[4] * m[1] * m[10] +
	  m[4] * m[2] * m[9] +
	  m[8] * m[1] * m[6] -
	  m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det != 0) {
		det = 1. / det;

		for (i = 0; i < 16; ++i)
			r[i] = inv[i] * det;
	}
}

/* vector2 ops */

void gm_vec2_init(union gm_vec2 *v, const union gm_point2 *p0,
  const union gm_point2 *p1)
{
	v->x = p1->x - p0->x;
	v->y = p1->y - p0->y;
}

float gm_vec2_dot(const union gm_vec2 *v0, const union gm_vec2 *v1)
{
	return v0->x * v1->x + v0->y * v1->y;
}

float gm_vec2_cos(const union gm_vec2 *v0, const union gm_vec2 *v1)
{
	return gm_vec2_dot(v0, v1) / (v0->len * v1->len);
}

float gm_vec2_angle(const union gm_vec2 *v0, const union gm_vec2 *v1)
{
	return acos(gm_vec2_dot(v0, v1) / (v0->len * v1->len));
}

void gm_vec2_len(union gm_vec2 *v)
{
	v->len = sqrt(v->x * v->x + v->y * v->y);
}

void gm_vec2_normalize(union gm_vec2 *v)
{
	float len = sqrt(v->x * v->x + v->y * v->y);

	v->x = v->x / len;
	v->y = v->y / len;
	v->len = 1;
}

void gm_vec2_rotate(union gm_vec2 *v, float a)
{
	v->x = v->x * cos(a) - v->y * sin(a);
	v->y = v->x * sin(a) + v->y * cos(a);
}

void gm_vec2_perp_cc(union gm_vec2 *in, union gm_vec2 *out)
{
	/* counterclockwise */
	out->x = -in->y;
	out->y = in->x;
}

void gm_vec2_perp_cw(union gm_vec2 *in, union gm_vec2 *out)
{
	/* clockwise */
	out->x = in->y;
	out->y = -in->x;
}

/* vector3 ops */

void gm_vec3_init(union gm_vec3 *v, const union gm_point3 *p0,
  const union gm_point3 *p1)
{
	v->x = p1->x - p0->x;
	v->y = p1->y - p0->y;
	v->z = p1->z - p0->z;
}

void gm_vec3_cross(union gm_vec3 *v, const union gm_vec3 *v0,
  const union gm_vec3 *v1)
{
	v->x = v0->y * v1->z - v0->z * v1->y;
	v->y = v0->z * v1->x - v0->x * v1->z;
	v->z = v0->x * v1->y - v0->y * v1->x;
}

float gm_vec3_dot(const union gm_vec3 *v0, const union gm_vec3 *v1)
{
	return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

void gm_vec3_len(union gm_vec3 *v)
{
	v->len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

void gm_vec3_normalize(union gm_vec3 *v)
{
	float len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);

	v->x = v->x / len;
	v->y = v->y / len;
	v->z = v->z / len;
	v->len = 1;
}

float gm_vec3_angle(const union gm_vec3 *v0, const union gm_vec3 *v1)
{
	return acos(gm_vec3_dot(v0, v1) / (v0->len * v1->len));
}

/* plane ops */

void gm_plane_init(union gm_plane3 *p, const union gm_point3 *p0,
  const union gm_point3 *p1, const union gm_point3 *p2)
{
	union gm_vec3 v0;
	union gm_vec3 v1;

	gm_vec3_init(&v0, p0, p1);
	gm_vec3_init(&v1, p0, p2);

	gm_vec3_cross(&p->n, &v0, &v1);
	gm_vec3_normalize(&p->n);
	p->d = -1 * (p->n.x * v0.x + p->n.y * v0.y + p->n.z * v0.z);
}

void gm_plane3_intersect(const union gm_plane3 *p, const float dir[3],
  const float origin[3], union gm_vec3 *v)
{
	float x1 = origin[0];
	float y1 = origin[1];
	float z1 = origin[2];
	float ax = dir[0];
	float ay = dir[1];
	float az = dir[2];

	v->len = -1 * (p->a * x1 + p->b * y1 + p->c * z1 + p->d) /
	  (p->a * ax + p->b * ay + p->c * az);
	v->x = x1 + ax * v->len;
	v->y = y1 + ay * v->len;
	v->z = z1 + az * v->len;
}

void gm_ray_intersect(const union gm_plane3 *p, float x, float y,
  float w, float h, const float vp[16], union gm_vec3 *v)
{
	float xx = x * 2. / w - 1.;
	float yy = (h - y) * 2. / h - 1.;
	float far_screen[4] = { xx, yy, 1., 1., };
	float near_screen[4] = { xx, yy, -1., 1., };
	float near_plane[4];
	float far_plane[4];
	float inv_vp[16];
	float origin[3];
	float dir[3];

	gm_mat4_identity(inv_vp);
	gm_mat4_invert(inv_vp, vp);

	gm_mat4_mulmv(near_plane, inv_vp, near_screen);
	gm_mat4_mulmv(far_plane, inv_vp, far_screen);

	dir[0] = far_plane[0] / far_plane[3];
	dir[1] = far_plane[1] / far_plane[3];
	dir[2] = far_plane[2] / far_plane[3];

	origin[0] = near_plane[0] / near_plane[3];
	origin[1] = near_plane[1] / near_plane[3];
	origin[2] = near_plane[2] / near_plane[3];

	dir[0] -= origin[0];
	dir[1] -= origin[1];
	dir[2] -= origin[2];

	float norm = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);

	dir[0] /= norm;
	dir[1] /= norm;
	dir[2] /= norm;

	gm_plane3_intersect(p, dir, origin, v);
}

float gm_perp_fx(const union gm_line *l, float x)
{
	/*
	 *  f(x) = (y1 - y0) / (x1 - x0) * (x - x0) + y0
	 *  p(x) = (x0 - x1) / (y1 - y0) * (x - x0) + y0
	 */

	if (l->x0 == l->x1 || l->y0 == l->y1)
		return l->y0;
	else
		return (l->x0 - l->x1) / (l->y1 - l->y0) * (x - l->x0) + l->y0;
}

float gm_line_fx(const union gm_line *l, float x)
{
	/* f(x) = (x - x0) * (y1 - y0) / (x1 - x0) + y0 */

	if (l->x0 == l->x1 || l->y0 == l->y1)
		return l->y0;
	else
		return (l->y1 - l->y0) / (l->x1 - l->x0) * (x - l->x0) + l->y0;
}

float gm_line_slope(union gm_line *l)
{
	return (l->y0 - l->y1) / (l->x0 - l->x1);
}

float gm_line_angle(const union gm_line *l, uint8_t perp)
{
	float dx = l->x1 - l->x0;

	if (dx == 0.) { /* slope undefined */
		return 0.;
	} else {
		float slope;
		float angle;

		if (perp)
			slope = (l->x0 - l->x1) / (l->y1 - l->y0);
		else
			slope = (l->y1 - l->y0) / (l->x1 - l->x0);

		/* spare atan calculations if line is horizontal */

		if (slope == 0.)
			angle = 0.;
		else
			angle = atan(slope);

		return angle; /* of incline in radians */
	}
}

void gm_line_perp(union gm_line *in, union gm_line *out)
{
	out->x0 = in->y0;
	out->y0 = -in->x0;
	out->x1 = in->y1;
	out->y1 = -in->x1;
}

void gm_reflect_line(union gm_line *l)
{
	float x0 = l->x1;
	float y0 = l->y1;

	l->x1 = -1 * (l->x0 - 2 * l->x1);
	l->y1 = -1 * (l->y0 - 2 * l->y1);
	l->x0 = x0;
	l->y0 = y0;
}

void gm_line_center(union gm_line *l)
{
	l->cx = (l->x0 + l->x1) * .5;
	l->cy = (l->y0 + l->y1) * .5;
}

void gm_line_div2(union gm_line *l)
{
	gm_line_center(l);
	l->x0 = l->cx;
	l->y0 = l->cy;
}

float gm_line_len(union gm_line *l)
{
	float x = l->x1 - l->x0;
	float y = l->y1 - l->y0;

	return sqrt(x * x + y * y);
}

float gm_point_dist(union gm_point2 *p0, union gm_point2 *p1)
{
	float x = p1->x - p0->x;
	float y = p1->y - p0->y;

	return sqrt(x * x + y * y);
}

uint8_t gm_line_intersect(union gm_line *l1, union gm_line *l2,
  union gm_point2 *p)
{
	float denom = (l1->x0 - l1->x1) * (l2->y0 - l2->y1) -
	  (l1->y0 - l1->y1) * (l2->x0 - l2->x1);

	if (!denom)
		return 0; /* lines are parallel or coincident */

	float a = l1->x0 * l1->y1 - l1->y0 * l1->x1;
	float b = l2->x0 * l2->y1 - l2->y0 * l2->x1;

	p->x = (a * (l2->x0 - l2->x1) - b * (l1->x0 - l1->x1)) / denom;
	p->y = (a * (l2->y0 - l2->y1) - b * (l1->y0 - l1->y1)) / denom;

	return 1;
}

/* http://mathworld.wolfram.com/Circle-LineIntersection.html */

uint8_t gm_circle_intersect(union gm_line *l, float r, union gm_point2 *p)
{
	float r2 = r * r;
	float dx = l->x1 - l->x0;
	float dy = l->y1 - l->y0;
	float dr2 = dx * dx + dy * dy;
	float d = l->x0 * l->y1 - l->x1 * l->y0;
	float delta = r2 * dr2 - d * d;

	if (delta < 0)
		return 0; /* no intersection */

	/* return tangent line point or one of the intersections */

	p->x = (d * dy - dx * sqrt(delta)) / dr2;
	p->y = (-d * dx - fabs(dy) * sqrt(delta)) / dr2;

	return 1;
}

void gm_open(uint16_t x_max)
{
    ii("init ok\n");
}

void gm_close(void)
{
}
