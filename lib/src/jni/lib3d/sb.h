/* sb.c: simple status bar
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

#include "font.h"

void sb_init(struct font *f0, struct font *f1);
void sb_render(void);
