/* game.h: game renderer
 *
 * Copyright (c) 2019, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

#include "font.h"

void game_open(void *assets);
void game_close(void);
void game_pause(void);
void game_resume(void);
int game_render(void);
void game_resize(uint16_t w, uint16_t h);
void game_touch(float x, float y);
