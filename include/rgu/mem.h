#pragma once

#include <stdint.h>

//#define MEM_DEBUG

#ifdef MEM_DEBUG
#include <rgu/log.h>
#define alloc(size) alloc_dbg(size, __func__, __LINE__)
static inline void *alloc_dbg(size_t size, const char *func, uint16_t line)
{
	void *ret = calloc(1, size);
	ii("\033[0;32malloc\033[0m %p | %s:%d\n", ret, func, line);
	return ret;
}

#define dealloc(ptr) {\
	ii("\033[0;34mfree\033[0m %p | %s:%d\n", (ptr), __func__, __LINE__);\
	free((void *) (ptr));\
	(ptr) = NULL;\
}
#include <malloc.h> /* for mallinfo */

#define mallinfo_start(name) struct mallinfo name = mallinfo();

#define mallinfo_stop(name) {\
        struct mallinfo name_ = mallinfo();\
        ii(#name": mem %d --> %d, diff %d\n", name.uordblks, name_.uordblks,\
           name_.uordblks - name.uordblks);\
}

#define mallinfo_call(fn) {\
        struct mallinfo mi0__ = mallinfo(), mi1__;\
        ii("> %s:%d: "#fn"() mem before %d\n", __func__, __LINE__,\
           mi0__.uordblks);\
        fn;\
        mi1__ = mallinfo();\
        ii("< %s:%d: "#fn"() mem after %d, diff %d\n", __func__, __LINE__,\
           mi1__.uordblks, mi1__.uordblks - mi0__.uordblks);\
}
#else
#define alloc(size) calloc(1, size)
#define dealloc(ptr) {\
	free((void *) (ptr));\
	(ptr) = NULL;\
}
#define mallinfo_start(name) do {} while(0)
#define mallinfo_stop(name) do {} while(0)
#define mallinfo_call(fn) do {} while(0)
#endif /* MEM_DEBUG */
