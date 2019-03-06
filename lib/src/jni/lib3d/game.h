/* game.h: game renderer
 *
 * Copyright (c) 2019, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

#include "font.h"

void game_open(struct font *f0, struct font *f1, void *assets);
void game_close(void);
void game_render(void);
void game_resize(uint16_t w, uint16_t h);
void game_touch(uint16_t x, uint16_t y);
