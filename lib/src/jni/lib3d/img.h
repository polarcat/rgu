/* img.h: static background rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>

int img_open(const char *path, void *assets);
void img_close(void);
void img_render(void);
void img_resize(uint16_t w, uint16_t h);
