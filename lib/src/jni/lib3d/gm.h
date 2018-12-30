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
	double data[4];
	struct {
		double x0;
		double y0;
		double x1;
		double y1;
	};
};

union gm_vec2 {
	double data[3];
	struct {
		double x;
		double y;
		double len;
	};
};

union gm_vec3 {
	double data[4];
	struct {
		double x;
		double y;
		double z;
		double len;
	};
};

union gm_plane3 {
	union gm_vec3 n; /* x ==> A, y ==> B, z ==> C */
	struct {
		double a;
		double b;
		double c;
		double len;
		double d;
	};
};

/* NB: column major */

#define gm_trans_x(m) (m)[11]
#define gm_trans_y(m) (m)[13]
#define gm_trans_z(m) (m)[14]

#define gm_scale_x(m) (m)[0]
#define gm_scale_y(m) (m)[5]
#define gm_scale_z(m) (m)[10]

#define gm_pow2(a) ((a) * (a))

/* marix4 ops */

void gm_mat4_identity(double m[16]);
void gm_mat4_mulmm(double r[16], const double m0[16], const double m1[16]);
void gm_mat4_mulmv(double r[4], const double m[16], const double v[4]);
void gm_mat4_invert(double r[16], const double m[16]);

/* vector2 ops */

void gm_vec2_init(union gm_vec2 *v, const double v0[3], const double v1[3]);
double gm_vec2_crossprod(const union gm_vec2 *v0, const union gm_vec2 *v1);
double gm_vec2_dotprod(const union gm_vec2 *v0, const union gm_vec2 *v1);
double gm_vec2_angle(const union gm_vec2 *v0,
  const union gm_vec2 *v1);
void gm_vec2_normalize(union gm_vec2 *v);

/* vector3 ops */

void gm_vec3_len(union gm_vec3 *v);
void gm_vec3_init(union gm_vec3 *v, const double p0[3], const double p1[3]);
void gm_vec3_crossprod(union gm_vec3 *v, const union gm_vec3 *v0,
  const union gm_vec3 *v1);
double gm_vec3_dotprod(const union gm_vec3 *v0, const union gm_vec3 *v1);
double gm_vec3_angle(const union gm_vec3 *v0, const union gm_vec3 *v1);
void gm_vec3_normalize(union gm_vec3 *v);

/* plane ops */

void gm_plane_init(union gm_plane3 *p, const double p1[3],
  const double p2[3], const double p3[3]);
void gm_plane_intersect(const union gm_plane3 *p, const double dir[3],
  const double origin[3], union gm_vec3 *res);
void gm_ray_intersect(const union gm_plane3 *p, double x, double y, double w,
  double h, const double vp[16], union gm_vec3 *res);

/* line ops */

double gm_line_fx(const union gm_line *l, double x);
double gm_perp_fx(const union gm_line *l, double x);
double gm_line_angle(const union gm_line *l, uint8_t perp);

#define radians(angle) (angle) * M_PI / 180
#define degrees(angle) (angle) * 180 / M_PI

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

extern double gm_cos_[360];
extern double gm_sin_[360];
extern double *gm_rx_;
extern double *gm_ry_;
extern uint16_t gm_max_x_;

void gm_open(uint16_t x_max);
void gm_close(void);

#define gm_norm_x(x, w) (x) / ((w) * .5) - 1.
#define gm_norm_y(y, h) 1. - (y) / ((h) * .5)
