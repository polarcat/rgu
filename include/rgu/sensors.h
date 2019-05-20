/* sensors.h: sensors data helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

union orientation {
	float data[3];
	struct {
		float azimuth;
		float pitch;
		float roll;
	};
};

void sensors_update_rmatrix(float a0, float a1, float a2,
	float a3, float a4, float a5, float a6, float a7, float a8);
void sensors_update_imatrix(float a0, float a1, float a2,
	float a3, float a4, float a5, float a6, float a7, float a8);
void sensors_update_orientation(float azimuth, float pitch, float roll);
void sensors_orientation(union orientation *res);
