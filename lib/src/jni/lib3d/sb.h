/* sb.c: simple status bar
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

#include "font.h"

void sb_open(struct font *f0, struct font *f1, uint8_t async);
void sb_close(void);
void sb_render(void);
