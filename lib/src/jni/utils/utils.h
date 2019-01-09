/* utils.h: common utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define container_of(ptr, type, member) __extension__ ({\
	const __typeof__(((type *) 0)->member) * __mptr = (ptr);\
	(type *) ((char *) __mptr - offsetof(type, member));\
})
