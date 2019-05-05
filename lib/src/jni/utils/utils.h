/* utils.h: common utils
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#include <stddef.h>
#include <semaphore.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef container_of
#define container_of(ptr, type, member) __extension__ ({\
	const __typeof__(((type *) 0)->member) * __mptr = (ptr);\
	(type *) ((char *) __mptr - offsetof(type, member));\
})
#endif

#define dealloc(mem) {\
	free((void *) mem);\
	mem = NULL;\
}

static inline void sem_post_checked(sem_t *sem)
{
	int val = 1; /* to skip error checking */

	sem_getvalue(sem, &val);

	if (!val)
		sem_post(sem);
}
