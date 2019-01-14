/* gm.c: graphics math helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#include <stdlib.h>

#define TAG "math"

#include <utils/log.h>

#include "gm.h"

/* column-major matrix4 ops */

void gm_mat4_identity(double m[16])
{
	m[0] = m[5] = m[10] = m[15] = 1;
	m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = 0;
	m[12] = m[13] = m[14] = 0;
}

#define m4e(r, e1, e2, e3, e4, e5, e6, e7, e8)\
	r = e1 * e2 + e3 * e4 + e5 * e6 + e7 * e8

void gm_mat4_mulmm(double r[16], const double m0[16], const double m1[16])
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

void gm_mat4_mulmv(double r[4], const double m[16], const double v[4])
{
	r[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
	r[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
	r[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
	r[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
}

void gm_mat4_invert(double r[16], const double m[16])
{
	double inv[16];
	double det;
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

void gm_vec2_init(union gm_vec2 *v, const double p0[2], const double p1[2])
{
	v->x = p1[0] - p0[0];
	v->y = p1[1] - p0[1];
	v->len = sqrt(v->x * v->x + v->y * v->y);
}

double gm_vec2_crossprod(const union gm_vec2 *v0, const union gm_vec2 *v1)
{
	return (v0->x * v1->y - v0->y * v1->x) / (v0->len * v1->len);
}

double gm_vec2_dotprod(const union gm_vec2 *v0, const union gm_vec2 *v1)
{
	return v0->x * v1->x + v0->y * v1->y;
}

double gm_vec2_cos(const union gm_vec2 *v0, const union gm_vec2 *v1)
{
	return gm_vec2_dotprod(v0, v1) / (v0->len * v1->len);
}

double gm_vec2_angle(const union gm_vec2 *v0,
  const union gm_vec2 *v1)
{
	return acos(gm_vec2_dotprod(v0, v1) / (v0->len * v1->len));
}

void gm_vec2_normalize(union gm_vec2 *v)
{
	v->x = v->x / v->len;
	v->y = v->y / v->len;
	v->len = 1;
}

void gm_vec2_len(union gm_vec2 *v)
{
	v->len = sqrt(v->x * v->x + v->y * v->y);
}

/* vector3 ops */

void gm_vec3_len(union gm_vec3 *v)
{
	v->len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

/**
 * init vector3 from source and target points
 */
void gm_vec3_init(union gm_vec3 *v, const double p0[3], const double p1[3])
{
	v->x = p1[0] - p0[0];
	v->y = p1[1] - p0[1];
	v->z = p1[2] - p0[2];
	v->len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

void gm_vec3_crossprod(union gm_vec3 *v, const union gm_vec3 *v0,
  const union gm_vec3 *v1)
{
	v->x = v0->y * v1->z - v0->z * v1->y;
	v->y = v0->z * v1->x - v0->x * v1->z;
	v->z = v0->x * v1->y - v0->y * v1->x;
	v->len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

double gm_vec3_dotprod(const union gm_vec3 *v0, const union gm_vec3 *v1)
{
	return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

void gm_vec3_normalize(union gm_vec3 *v)
{
	v->x = v->x / v->len;
	v->y = v->y / v->len;
	v->z = v->z / v->len;
	v->len = 1;
}

/**
 * compute angle between two vectors
 */
double gm_vec3_angle(const union gm_vec3 *v0,
  const union gm_vec3 *v1)
{
	return acos(gm_vec3_dotprod(v0, v1) / (v0->len * v1->len));
}

/* plane ops */

void gm_plane_init(union gm_plane3 *p, const double p0[3],
  const double p1[3], const double p2[3])
{
	union gm_vec3 v0;
	union gm_vec3 v1;

	gm_vec3_init(&v0, p0, p1);
	gm_vec3_init(&v1, p0, p2);

	gm_vec3_crossprod(&p->n, &v0, &v1);
	gm_vec3_normalize(&p->n);
	p->d = -1 * (p->n.x * v0.x + p->n.y * v0.y + p->n.z * v0.z);
}

void gm_plane3_intersect(const union gm_plane3 *p, const double dir[3],
  const double origin[3], union gm_vec3 *v)
{
	double x1 = origin[0];
	double y1 = origin[1];
	double z1 = origin[2];
	double ax = dir[0];
	double ay = dir[1];
	double az = dir[2];

	v->len = -1 * (p->a * x1 + p->b * y1 + p->c * z1 + p->d) /
	  (p->a * ax + p->b * ay + p->c * az);
	v->x = x1 + ax * v->len;
	v->y = y1 + ay * v->len;
	v->z = z1 + az * v->len;
}

void gm_ray_intersect(const union gm_plane3 *p, double x, double y,
  double w, double h, const double vp[16], union gm_vec3 *v)
{
	double xx = x * 2. / w - 1.;
	double yy = (h - y) * 2. / h - 1.;
	double far_screen[4] = { xx, yy, 1., 1., };
	double near_screen[4] = { xx, yy, -1., 1., };
	double near_plane[4];
	double far_plane[4];
	double inv_vp[16];
	double origin[3];
	double dir[3];

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

	double norm = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);

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

void gm_line_perp(union gm_line *l)
{
	float x = l->x0;
	float y = l->y0;

	l->x0 = y;
	l->y0 = -x;
	x = l->x1;
	y = l->y1;
	l->x1 = y;
	l->y1 = -x;
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

float gm_line_length(union gm_line *l)
{
	float x = l->x1 - l->x0;
	float y = l->y1 - l->y0;

	return sqrt(x * x + y * y);
}

double gm_cos_[360];
double gm_sin_[360];
double *gm_rx_ = NULL;
double *gm_ry_ = NULL;
uint16_t gm_max_x_ = 0;

void gm_open(uint16_t x_max)
{
    for (uint16_t a = 0; a < 360; ++a) {
        gm_cos_[a] = cos(radians(a));
        gm_sin_[a] = sin(radians(a));
    }

    size_t size = 360 * x_max * sizeof(double);

    if (!(gm_rx_ = (double *) malloc(size))) {
        ee("failed to allocate %zu bytes\n", size);
        return;
    }

    if (!(gm_ry_ = (double *) malloc(size))) {
        ee("failed to allocate %zu bytes\n", size);
        free(gm_rx_);
        gm_rx_ = NULL;
        return;
    }

    uint32_t i = 0;

    for (uint16_t a = 0; a < 360; ++a) {
        for (uint16_t r = 0; r < x_max; ++r) {
            gm_rx_[i] = r * gm_cos_[a];
            gm_ry_[i] = r * gm_sin_[a];
            i++;
        }
    }

    gm_max_x_ = x_max;

    ii("init ok, %zu bytes allocated for pre-multiplied values\n", size);
}

void gm_close(void)
{
	free(gm_rx_);
	free(gm_ry_);
}
