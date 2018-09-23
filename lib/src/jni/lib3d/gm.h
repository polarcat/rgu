/* gm.h: graphics math helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>
#include <math.h>

union gm_line {
	float data[4];
	struct {
		float x0;
		float y0;
		float x1;
		float y1;
	};
};

union gm_vec3 {
	float data[4];
	struct {
		float x;
		float y;
		float z;
		float len;
	};
};

union gm_plane3 {
	union gm_vec3 normal; /* x ==> A, y ==> B, z ==> C */
	struct {
		float a;
		float b;
		float c;
		float len;
		float d;
	};
};

/* NB: column major */

#define gm_trans_x(m) (m)[11]
#define gm_trans_y(m) (m)[13]
#define gm_trans_z(m) (m)[14]

#define gm_scale_x(m) (m)[0]
#define gm_scale_y(m) (m)[5]
#define gm_scale_z(m) (m)[10]

#define gm_pow2(a) fabs((a) * (a))

/* marix4 ops */

void gm_mat4_identity(float m[16]);
void gm_mat4_mulmm(float r[16], const float m0[16], const float m1[16]);
void gm_mat4_mulmv(float r[4], const float m[16], const float v[4]);
void gm_mat4_invert(float r[16], const float m[16]);

/* vector3 ops */

void gm_vec3_init(union gm_vec3 *v, const float v0[3], const float v1[3]);
void gm_vec3_crossprod(union gm_vec3 *v, const float v0[3],
  const float v1[3]);
float gm_vec3_dotprod(const union gm_vec3 *v0,
  const union gm_vec3 *v1);
float gm_vec3_angle(const union gm_vec3 *v0, const union gm_vec3 *v1);
void gm_vec3_normalize(union gm_vec3 *v);

/* plane ops */

void gm_plane_init(union gm_plane3 *p, const float point1[3],
  const float point2[3], const float point3[3]);
void gm_plane_intersect(const union gm_plane3 *p, const float dir[3],
  const float origin[3], union gm_vec3 *res);
void gm_ray_intersect(const union gm_plane3 *p, float x, float y, float w,
  float h, const float vp[16], union gm_vec3 *res);

/* line ops */

float gm_line_fx(const union gm_line *l, float x);
float gm_perp_fx(const union gm_line *l, float x);
float gm_line_angle(const union gm_line *l, uint8_t perp);

#define radians(angle) (angle) * M_PI / 180
#define degrees(angle) (angle) * 180 / M_PI

#define gm_circle_x(center_x, radius, angle)\
  (center_x) + (radius) * cos(angle)

#define gm_circle_y(center_y, radius, angle)\
  (center_y) + (radius) * sin(angle)

#define gm_norm_x(x, w) (x) / ((w) * .5) - 1.
#define gm_norm_y(y, h) (y) / ((h) * .5) - 1.
