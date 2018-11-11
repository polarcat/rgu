/* cv.h: cv header
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

#include "font.h"

void cv_open(struct font *f0, struct font *f1);
void cv_close(void);
void cv_render(void);
void cv_resize(uint16_t w, uint16_t h);
void cv_touch(uint16_t x, uint16_t y);
