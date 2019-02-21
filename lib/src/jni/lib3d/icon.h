/* icon.h: icon renderer
 *
 * Copyright (C) 2019 by Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#pragma once

#include <stdint.h>

void icon_open(const char *path, void *assets);
void icon_close(void);
void icon_render(void);
