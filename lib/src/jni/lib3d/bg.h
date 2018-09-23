/* bg.h: background rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>

int bg_open(void);
void bg_close(void);
void bg_render(void);
void bg_render_offscreen(uint8_t *buf, uint16_t w, uint16_t h);
void bg_resize(uint16_t w, uint16_t h);
