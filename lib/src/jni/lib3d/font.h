/* font.h: font rendering
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stdint.h>

struct font;

void font_close(struct font **font);
struct font *font_open(const char *path, float size, void *assets);
void font_render_async(struct font *font, const char *str, uint16_t len,
  float x, float y, uint32_t fg, uint32_t bg);
void font_render(struct font *font, const char *str, uint16_t len,
  float x, float y, uint32_t fg, uint32_t bg);
