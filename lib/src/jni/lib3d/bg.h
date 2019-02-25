/* bg.h: background rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>

#ifdef STATIC_BG
int bg_open(const char *path, void *assets, uint16_t *w, uint16_t *h);
#else
int bg_open(void);
#endif
void bg_close(void);
void bg_render(uint8_t grey);
void bg_render_offscreen(uint8_t *buf, uint16_t w, uint16_t h, uint8_t grey);
void bg_resize(uint16_t w, uint16_t h);
