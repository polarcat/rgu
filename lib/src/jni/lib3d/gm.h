/* gm.h: graphics math helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>
#include <math.h>

union gm_point2i {
	int data[2];
	struct {
		int x;
		int y;
	};
};

union gm_point2 {
	float data[2];
	struct {
		float x;
		float y;
	};
};

union gm_point3 {
	float data[3];
	struct {
		float x;
		float y;
		float z;
	};
	struct {
		union gm_point2 xy;
	};
};

union gm_line {
	float data[4];
	struct {
		float x0;
		float y0;
		float x1;
		float y1;
		float cx;
		float cy;
	};
	struct {
		union gm_point2 p0;
		union gm_point2 p1;
	};
};

union gm_vec2 {
	float data[3];
	struct {
		float x;
		float y;
		float len;
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
	union gm_vec3 n; /* x ==> A, y ==> B, z ==> C */
	struct {
		float a;
		float b;
		float c;
		float len;
		float d;
	};
};

union gm_mat4 { /* column-major */
	float data[16];
	struct {
		float sx;
		float a1;
		float a2;
		float a3;

		float a4;
		float sy;
		float a6;
		float a7;

		float a8;
		float a9;
		float sz;
		float a11;

		float x;
		float y;
		float z;
		float w;
	};
};

/* marix4 ops */

void gm_mat4_identity(float m[16]);
void gm_mat4_mulmm(float r[16], const float m0[16], const float m1[16]);
void gm_mat4_mulmv(float r[4], const float m[16], const float v[4]);
void gm_mat4_invert(float r[16], const float m[16]);

/* vector2 ops */

void gm_vec2_init(union gm_vec2 *v, const union gm_point2 *p0,
  const union gm_point2 *p1);
float gm_vec2_dot(const union gm_vec2 *v0, const union gm_vec2 *v1);
float gm_vec2_cos(const union gm_vec2 *v0, const union gm_vec2 *v1);
float gm_vec2_angle(const union gm_vec2 *v0, const union gm_vec2 *v1);
void gm_vec2_perp(union gm_vec2 *in, union gm_vec2 *out);
void gm_vec2_normalize(union gm_vec2 *v);
void gm_vec2_len(union gm_vec2 *v);
void gm_vec2_rotate(union gm_vec2 *v, float a);

/* vector3 ops */

void gm_vec3_init(union gm_vec3 *v, const union gm_point3 *p0,
  const union gm_point3 *p1);
void gm_vec3_cross(union gm_vec3 *v, const union gm_vec3 *v0,
  const union gm_vec3 *v1);
float gm_vec3_dot(const union gm_vec3 *v0, const union gm_vec3 *v1);
void gm_vec3_len(union gm_vec3 *v);
float gm_vec3_angle(const union gm_vec3 *v0, const union gm_vec3 *v1);
void gm_vec3_normalize(union gm_vec3 *v);

/* plane ops */

void gm_plane_init(union gm_plane3 *p, const union gm_point3 *p1,
  const union gm_point3 *p2, const union gm_point3 *p3);
void gm_plane_intersect(const union gm_plane3 *p, const float dir[3],
  const float origin[3], union gm_vec3 *res);
void gm_ray_intersect(const union gm_plane3 *p, float x, float y, float w,
  float h, const float vp[16], union gm_vec3 *res);

/* line ops */

float gm_line_fx(const union gm_line *l, float x);
float gm_perp_fx(const union gm_line *l, float x);
float gm_line_angle(const union gm_line *l, uint8_t perp);
float gm_line_len(union gm_line *l);
void gm_line_perp(union gm_line *in, union gm_line *out);
void gm_reflect_line(union gm_line *l);
void gm_line_center(union gm_line *l);
float gm_line_slope(union gm_line *l);
void gm_line_div2(union gm_line *l);
uint8_t gm_line_intersect(union gm_line *l1, union gm_line *l2,
  union gm_point2 *p);
uint8_t gm_circle_intersect(union gm_line *l, float r, union gm_point2 *p);

#define radians(angle) ((angle) * (M_PI) / 180)
#define degrees(angle) ((angle) * 180 / (M_PI))

/* arbitrary radius and angle */

#define gm_circle_x(center_x, radius, radians)\
  (center_x) + (radius) * cos(radians)

#define gm_circle_y(center_y, radius, radians)\
  (center_y) + (radius) * sin(radians)

/* pre-multiplied radius and angle */

#define gm_circle_rx(center_x, radius, degrees)\
    (center_x) + gm_rx_[radius + degrees * gm_max_x_]

#define gm_circle_ry(center_y, radius, degrees)\
    (center_y) + gm_ry_[radius + degrees * gm_max_x_]

extern float gm_cos_[360];
extern float gm_sin_[360];
extern float *gm_rx_;
extern float *gm_ry_;
extern uint16_t gm_max_x_;

void gm_open(uint16_t x_max);
void gm_close(void);

#define gm_norm_x(x, w) ((x) / ((w) * .5) - 1.)
#define gm_norm_y(y, h) (1. - (y) / ((h) * .5))
#define gm_norm_z(z, h) (1. / (h) * (z))
