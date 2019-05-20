/* sensors.c: sensors data helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#include <math.h>

#include <rgu/gm.h>
#include <rgu/log.h>
#include <rgu/sensors.h>

static float rmatrix_[9] = {
	1, 0, 0,
	0, 1, 0,
	0, 0, 1,
};

static float imatrix_[9] = {
	1, 0, 0,
	0, 1, 0,
	0, 0, 1,
};

void sensors_update_rmatrix(float a0, float a1, float a2,
	float a3, float a4, float a5, float a6, float a7, float a8)
{
#if 0
	ii("R: %+.4f %+.4f %+.4f\n   %+.4f %+.4f %+.4f\n   %+.4f %+.4f %+.4f\n",
	  a0, a1, a2, a3, a4, a5, a6, a7, a8);
#else
	dd("R: %+.2f %+.2f %+.2f\n   %+.2f %+.2f %+.2f\n   %+.2f %+.2f %+.2f\n",
	  degrees(a0), degrees(a1), degrees(a2),
	  degrees(a3), degrees(a4), degrees(a5),
	  degrees(a6), degrees(a7), degrees(a8));
#endif
}

void sensors_update_imatrix(float a0, float a1, float a2,
	float a3, float a4, float a5, float a6, float a7, float a8)
{
#if 0
	ii("I: %+.4f %+.4f %+.4f\n   %+.4f %+.4f %+.4f\n   %+.4f %+.4f %+.4f\n",
	  a0, a1, a2, a3, a4, a5, a6, a7, a8);
#else
	dd("I: %+.2f %+.2f %+.2f\n   %+.2f %+.2f %+.2f\n   %+.2f %+.2f %+.2f\n",
	  degrees(a0), degrees(a1), degrees(a2),
	  degrees(a3), degrees(a4), degrees(a5),
	  degrees(a6), degrees(a7), degrees(a8));
#endif
}

static float azimuth_;
static float pitch_;
static float roll_;

void sensors_update_orientation(float azimuth, float pitch, float roll)
{
	dd("azimuth %+.2f | pitch %+.2f | roll %+.2f\n", degrees(azimuth),
	  degrees(pitch), degrees(roll));
	azimuth_ = azimuth;
	pitch_ = pitch;
	roll_ = roll;
}

void sensors_orientation(union orientation *res)
{
	res->azimuth = azimuth_;
	res->pitch = pitch_;
	res->roll = roll_;
}
